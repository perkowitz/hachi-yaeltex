/*

Author/s: Franco Grassano - Franco Zaccra

MIT License

Copyright (c) 2020 - Yaeltex

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

*/

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
  eep = pEEP;

  blocksCount = blocks;

  descriptors = (blockDescriptor *)malloc(sizeof(blockDescriptor) * blocksCount);

  DisarmBlocks();
}

void memoryHost::ConfigureBlock(uint8_t block, uint16_t sectionCount, uint8_t sectionSize, bool unique,bool AllocateRAM)
{
  descriptors[block].sectionSize = sectionSize;
  descriptors[block].sectionCount = sectionCount;
  descriptors[block].unique = unique;

  if (unique)
  {
    uint16_t blockSize = sectionSize * sectionCount;
    
    descriptors[block].eepBaseAddress = eepIndex;

    if(AllocateRAM){
      descriptors[block].ramBaseAddress = malloc(blockSize);
      eep->read(descriptors[block].eepBaseAddress, (uint8_t *)descriptors[block].ramBaseAddress, blockSize);
    }

    eepIndex += blockSize;
  }
}


void memoryHost::DisarmBlocks()
{
  eepIndex = 0;
  bankSize = 0;
  bankNow = 255;
  bankChunk = 0;

  memset(descriptors,0,sizeof(blockDescriptor)*blocksCount);
}

void* memoryHost::Block(uint8_t block)
{
  return descriptors[block].ramBaseAddress;
}

uint8_t memoryHost::SectionSize(uint8_t block)
{
  return descriptors[block].sectionSize;
}

uint16_t memoryHost::SectionCount(uint8_t block)
{
  return descriptors[block].sectionCount;
}
void* memoryHost::Address(uint8_t block, uint8_t section)
{
  return (void*)(descriptors[block].sectionSize * section + (uint32_t)descriptors[block].ramBaseAddress);
}

void memoryHost::LayoutBanks(bool AllocateRAM)
{
  for (uint8_t i = 0; i < blocksCount; i++)
  {
    if (!descriptors[i].unique)
    {
      uint16_t blockSize = descriptors[i].sectionSize * descriptors[i].sectionCount;
      descriptors[i].eepBaseAddress = bankSize;
      bankSize += blockSize;
    }
  }

  //reserve entire bank chunk
  if(AllocateRAM)
    bankChunk = malloc(bankSize);

  SerialUSB.print("Bank size: "); SerialUSB.println(bankSize);

  for (uint8_t i = 0; i < blocksCount; i++)
  {
    if (!descriptors[i].unique)
    {
      descriptors[i].ramBaseAddress = (void*)(descriptors[i].eepBaseAddress + (uint32_t)bankChunk);
      descriptors[i].eepBaseAddress += eepIndex;
    }
  }
}

void memoryHost::PrintEEPROM(uint8_t bank, uint8_t block, uint8_t section){
 // uint16_t address = descriptors[block].eepBaseAddress + descriptors[block].sectionSize * section;
 // byte data[256];

 //  if (!descriptors[block].unique)
 //    address +=  bank * bankSize;

 //  eep->read(address, (byte*)(data), descriptors[block].sectionSize); 

 //  SerialUSB.println("------------------------------------------");
 //  switch(block){
 //    case ytxIOBLOCK::Configuration:{
 //      SerialUSB.println("BLOCK 0 - CONFIGURATION"); SerialUSB.print(" - SIZE: "); SerialUSB.println(descriptors[block].sectionSize);
 //    }break;
 //    case ytxIOBLOCK::Encoder:{
 //      SerialUSB.print("BLOCK 1 - ENCODER - SECTION "); SerialUSB.print(section); SerialUSB.print(" - SIZE: "); SerialUSB.println(descriptors[block].sectionSize);
 //    }break;
 //    case ytxIOBLOCK::Digital:{
 //      SerialUSB.print("BLOCK 2 - DIGITAL - SECTION "); SerialUSB.print(section); SerialUSB.print(" - SIZE: "); SerialUSB.println(descriptors[block].sectionSize);
 //    }break;
 //    case ytxIOBLOCK::Analog:{
 //      SerialUSB.print("BLOCK 3 - ANALOG - SECTION "); SerialUSB.print(section); SerialUSB.print(" - SIZE: "); SerialUSB.println(descriptors[block].sectionSize);
 //    }break;
 //    case ytxIOBLOCK::Feedback:{
 //      SerialUSB.print("BLOCK 4 - FEEDBACK - SECTION "); SerialUSB.print(section); SerialUSB.print(" - SIZE: "); SerialUSB.println(descriptors[block].sectionSize);
 //    }break;
    
 //  }

 //  SerialUSB.println("DATA IN EEPROM");
 //  for(int i = 0; i < descriptors[block].sectionSize; i++){
 //    SerialUSB.print(data[i], HEX);
 //    SerialUSB.print("\t");
 //    if(i > 0 && !(i % 8)) SerialUSB.println();
 //  }
  
 //  // SerialUSB.println("\nDATA IN RAM");

 //  // for(int i = 0; i < descriptors[block].sectionSize; i++){
 //  //   SerialUSB.print(*((byte*) (descriptors[i].ramBaseAddress + descriptors[block].sectionSize * section + i)), HEX);
 //  //   SerialUSB.print("\t");
 //  //   if(i>0 && !(i%8)) SerialUSB.println();
 //  // }

 //  SerialUSB.println("\n------------------------------------------");
 //  SerialUSB.println();
}

void memoryHost::ReadFromEEPROM(uint8_t bank, uint8_t block, uint8_t section, void *data)
{
  uint16_t address = descriptors[block].eepBaseAddress + descriptors[block].sectionSize * section;

  if (!descriptors[block].unique)
    address +=  bank * bankSize;

  eep->read(address, (byte*)(data), descriptors[block].sectionSize);
}

void memoryHost::WriteToEEPROM(uint8_t bank, uint8_t block, uint8_t section, void *data)
{
  uint16_t address = descriptors[block].eepBaseAddress + descriptors[block].sectionSize * section;
  SerialUSB.println("\nWriting to address: ");SerialUSB.println(address); SerialUSB.println();
  if (!descriptors[block].unique)
    address += bank*bankSize;

  eep->write(address, (byte*)(data), descriptors[block].sectionSize);
}

uint8_t memoryHost::LoadBank(uint8_t bank)
{
  if(bank != bankNow){
    eep->read(eepIndex+bankSize*bank, (byte*)bankChunk, bankSize);
    bankNow = bank;
    return bank;
  }else{
    return bankNow;
  }
    
}

int8_t memoryHost::GetCurrentBank()
{
  return bankNow;
}

uint8_t memoryHost::LoadBankSingleSection(uint8_t bank, uint8_t block, uint8_t sectionIndex)
{
  bankNow = -1;
  //  eep->read(eepIndex+bankSize*bank, (byte*)bankChunk, bankSize);
  switch (block) {
    case ytxIOBLOCK::Encoder: {
        ReadFromEEPROM(bank, ytxIOBLOCK::Encoder, sectionIndex, &encoder[sectionIndex]);
      } break;
    case ytxIOBLOCK::Digital: {
        ReadFromEEPROM(bank, ytxIOBLOCK::Digital, sectionIndex, &digital[sectionIndex]);
      } break;
    case ytxIOBLOCK::Analog: {
        ReadFromEEPROM(bank, ytxIOBLOCK::Analog, sectionIndex, &analog[sectionIndex]);
      } break;
    case ytxIOBLOCK::Feedback: {
        ReadFromEEPROM(bank, ytxIOBLOCK::Feedback, sectionIndex, &feedback[sectionIndex]);
      } break;
  }
  return bank;
}

void memoryHost::SaveBank(uint8_t bank)
{
  eep->write(eepIndex + bankSize * bank, (byte*)bankChunk, bankSize);
}

void* memoryHost::AllocateRAM(uint16_t size)
{
  if (size)
    return malloc(size);
  else
    return NULL;
}

void memoryHost::FreeRAM(void *pToFree)
{
  free(pToFree);
  return;
}
