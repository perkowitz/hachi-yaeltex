#include <extEEPROM.h>

#ifdef __arm__
// should use uinstd.h to define sbrk but Due causes a conflict
extern "C" char* sbrk(int incr);
#else  // __ARM__
extern char *__brkval;
#endif  // __arm__

int FreeMemory() {
  char top;
#ifdef __arm__
  return &top - reinterpret_cast<char*>(sbrk(0));
#elif defined(CORE_TEENSY) || (ARDUINO > 103 && ARDUINO != 151)
  return &top - __brkval;
#else  // __arm__
  return __brkval ? &top - __brkval : &top - __malloc_heap_start;
#endif  // __arm__
}

memoryHost::memoryHost(extEEPROM *pEEP, uint8_t blocks)
{
  blocksCount = blocks;

  eepIndex = 0;
  bankSize = 0;
  
  descriptors = (blockDescriptor *)malloc(sizeof(blockDescriptor)*blocksCount);

  eep = pEEP;
}

void memoryHost::ConfigureBlock(uint8_t block, uint8_t sectionCount, uint8_t sectionSize, bool unique)
{
  descriptors[block].sectionSize = sectionSize;
  descriptors[block].sectionCount = sectionCount;
  descriptors[block].unique = unique;
  
  if(unique)
  {
    uint16_t blockSize = sectionSize*sectionCount;
    descriptors[block].ramBaseAddress = malloc(blockSize);
    descriptors[block].eepBaseAddress = eepIndex;
    
    eep->read(descriptors[block].eepBaseAddress,(uint8_t *)descriptors[block].ramBaseAddress,blockSize);
    
    eepIndex += blockSize;
  }
}
void* memoryHost::Block(uint8_t block)
{ 
  return descriptors[block].ramBaseAddress;
}

uint8_t memoryHost::SectionSize(uint8_t block)
{
  return descriptors[block].sectionSize;
}

uint8_t memoryHost::SectionCount(uint8_t block)
{
  return descriptors[block].sectionCount;
}
void* memoryHost::Address(uint8_t block,uint8_t section)
{
  return (void*)(descriptors[block].sectionSize*section + (uint32_t)descriptors[block].ramBaseAddress);
}

void memoryHost::LayoutBanks()
{
  for(uint8_t i=0;i<blocksCount;i++)
  {
    if(!descriptors[i].unique)
    {
      uint16_t blockSize = descriptors[i].sectionSize*descriptors[i].sectionCount;
      descriptors[i].eepBaseAddress = bankSize;
      bankSize += blockSize;
    }
  }

  //reserve entire bank chunk
  bankChunk = malloc(bankSize);
  SerialUSB.print("Bank size: "); SerialUSB.println(bankSize);
  
  for(uint8_t i=0;i<blocksCount;i++)
  {
    if(!descriptors[i].unique)
    {
      descriptors[i].ramBaseAddress = (void*)(descriptors[i].eepBaseAddress + (uint32_t)bankChunk);
      descriptors[i].eepBaseAddress += eepIndex;
    }
  }
}

void memoryHost::ReadFromEEPROM(uint8_t bank,uint8_t block,uint8_t section,void *data)
{
  uint16_t address = descriptors[block].eepBaseAddress + descriptors[block].sectionSize*section;
  
  if(!descriptors[block].unique)
    address +=  bank*bankSize;
  
  eep->read(address,(byte*)(data),descriptors[block].sectionSize);
}

void memoryHost::WriteToEEPROM(uint8_t bank,uint8_t block,uint8_t section,void *data)
{
  uint16_t address = bank*bankSize + descriptors[block].eepBaseAddress + descriptors[block].sectionSize*section;
  
  if(!descriptors[block].unique)
    address += bankSize;
    
  eep->write(address,(byte*)(data),descriptors[block].sectionSize);
}

uint8_t memoryHost::LoadBank(uint8_t bank)
{
  eep->read(eepIndex+bankSize*bank, (byte*)bankChunk, bankSize);
  bankNow = bank;
  return bank;
}

int8_t memoryHost::GetCurrentBank()
{
  return bankNow;
}

uint8_t memoryHost::LoadBankSingleSection(uint8_t bank, uint8_t block, uint8_t sectionIndex)
{
  bankNow = -1;
//  eep->read(eepIndex+bankSize*bank, (byte*)bankChunk, bankSize);
  switch(block){
    case ytxIOBLOCK::Encoder:{
      ReadFromEEPROM(bank, ytxIOBLOCK::Encoder, sectionIndex, &encoder[sectionIndex]);
    }break;
    case ytxIOBLOCK::Digital:{
      ReadFromEEPROM(bank, ytxIOBLOCK::Digital, sectionIndex, &digital[sectionIndex]);
    }break;
    case ytxIOBLOCK::Analog:{
      ReadFromEEPROM(bank, ytxIOBLOCK::Analog, sectionIndex, &analog[sectionIndex]);
    }break;
    case ytxIOBLOCK::Feedback:{
      ReadFromEEPROM(bank, ytxIOBLOCK::Feedback, sectionIndex, &feedback[sectionIndex]);
    }break;
  }
  return bank;
}

void memoryHost::SaveBank(uint8_t bank)
{
  eep->write(eepIndex+bankSize*bank, (byte*)bankChunk, bankSize);
}

void* memoryHost::AllocateRAM(uint16_t size)
{
  if(size)
    return malloc(size);
  else
    return NULL;
}
