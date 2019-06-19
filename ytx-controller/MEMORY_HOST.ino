#include <extEEPROM.h>

#ifdef __arm__
// should use uinstd.h to define sbrk but Due causes a conflict
extern "C" char* sbrk(int incr);
#else  // __ARM__
extern char *__brkval;
#endif  // __arm__

int freeMemory() {
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

void memoryHost::configureBlock(uint8_t block,uint8_t sectionCount,uint8_t sectionSize,bool unique)
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
void* memoryHost::block(uint8_t block)
{ 
  return descriptors[block].ramBaseAddress;
}

uint8_t memoryHost::sectionSize(uint8_t block)
{
  return descriptors[block].sectionSize;
}

uint8_t memoryHost::sectionCount(uint8_t block)
{
  return descriptors[block].sectionCount;
}
void* memoryHost::address(uint8_t block,uint8_t section)
{
  return (void*)(descriptors[block].sectionSize*section + (uint32_t)descriptors[block].ramBaseAddress);
}

void memoryHost::layoutBanks()
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
  
  for(uint8_t i=0;i<blocksCount;i++)
  {
    if(!descriptors[i].unique)
    {
      descriptors[i].ramBaseAddress = (void*)(descriptors[i].eepBaseAddress + (uint32_t)bankChunk);
      descriptors[i].eepBaseAddress += eepIndex;
    }
  }
}

void memoryHost::readFromEEPROM(uint8_t bank,uint8_t block,uint8_t section,void *data)
{
  uint16_t address = bank+descriptors[block].eepBaseAddress+descriptors[block].sectionSize*section;
  
  if(!descriptors[block].unique)
    address += bankSize;
    
  eep->read(address,(byte*)(data),descriptors[block].sectionSize);
}

void memoryHost::writeToEEPROM(uint8_t bank,uint8_t block,uint8_t section,void *data)
{
  uint16_t address = bank+descriptors[block].eepBaseAddress+descriptors[block].sectionSize*section;
  
  if(!descriptors[block].unique)
    address += bankSize;
  eep->write(address,(byte*)(data),descriptors[block].sectionSize);
}

void memoryHost::loadBank(uint8_t bank)
{
  eep->read(eepIndex+bankSize*bank, (byte*)bankChunk, bankSize);
  currBank = bank;
}


void* memoryHost::allocateRAM(uint16_t size)
{
  return malloc(size);
}
