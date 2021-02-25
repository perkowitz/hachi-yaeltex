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

void memoryHost::ConfigureBlock(uint8_t block, uint16_t sectionCount, uint16_t sectionSize, bool unique, bool allocate)
{
  descriptors[block].sectionSize = sectionSize;
  descriptors[block].sectionCount = sectionCount;
  descriptors[block].unique = unique;
  SerialUSB.print(F("******************** Block: "));SerialUSB.println(block);
  SerialUSB.print(F("******************** New Section size: "));SerialUSB.println(descriptors[block].sectionSize);
  SerialUSB.print(F("******************** New Section count: "));SerialUSB.println(descriptors[block].sectionCount);
  SerialUSB.print(F("******************** EEPROM address: "));SerialUSB.println(descriptors[block].eepBaseAddress);

  if (unique)
  {
    uint16_t blockSize = sectionSize * sectionCount;
    
    descriptors[block].eepBaseAddress = eepIndex;

    if(allocate){
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

uint16_t memoryHost::SectionSize(uint8_t block)
{
  return descriptors[block].sectionSize;
}

uint16_t memoryHost::SectionCount(uint8_t block)
{
  return descriptors[block].sectionCount;
}
void* memoryHost::GetSectionAddress(uint8_t block, uint16_t section)
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

  // SerialUSB.print(F("Bank size: ")); SerialUSB.println(bankSize);

  for (uint8_t i = 0; i < blocksCount; i++)
  {
    if (!descriptors[i].unique)
    {
      descriptors[i].ramBaseAddress = (void*)(descriptors[i].eepBaseAddress + (uint32_t)bankChunk);
      descriptors[i].eepBaseAddress += eepIndex;
    }
  }
}

void memoryHost::PrintEEPROM(uint8_t bank, uint8_t block, uint16_t section){
  SerialUSB.print(F("******************** Bank: "));SerialUSB.println(bank);
  SerialUSB.print(F("******************** Block: "));SerialUSB.println(block);
  SerialUSB.print(F("******************** Section: "));SerialUSB.println(section);
  SerialUSB.print(F("******************** Section size: "));SerialUSB.println(descriptors[block].sectionSize);
  SerialUSB.print(F("******************** Section count: "));SerialUSB.println(descriptors[block].sectionCount);
  SerialUSB.print(F("******************** EEPROM address: "));SerialUSB.println(descriptors[block].eepBaseAddress + section*descriptors[block].sectionSize);
  SerialUSB.print(F("******************** RAM address: ")); SerialUSB.println((uint32_t)descriptors[block].ramBaseAddress + section*descriptors[block].sectionSize);
  SerialUSB.println();

 // uint16_t address = descriptors[block].eepBaseAddress + descriptors[block].sectionSize * section;
 // byte data[256];

 //  if (!descriptors[block].unique)
 //    address +=  bank * bankSize;

 //  eep->read(address, (byte*)(data), descriptors[block].sectionSize); 

 //  SerialUSB.println(F("------------------------------------------"));
 //  switch(block){
 //    case ytxIOBLOCK::Configuration:{
 //      SerialUSB.println(F("BLOCK 0 - CONFIGURATION")); SerialUSB.print(F(" - SIZE: ")); SerialUSB.println(descriptors[block].sectionSize);
 //    }break;
 //    case ytxIOBLOCK::Encoder:{
 //      SerialUSB.print(F("BLOCK 1 - ENCODER - SECTION ")); SerialUSB.print(section); SerialUSB.print(F(" - SIZE: ")); SerialUSB.println(descriptors[block].sectionSize);
 //    }break;
 //    case ytxIOBLOCK::Digital:{
 //      SerialUSB.print(F("BLOCK 2 - DIGITAL - SECTION ")); SerialUSB.print(section); SerialUSB.print(F(" - SIZE: ")); SerialUSB.println(descriptors[block].sectionSize);
 //    }break;
 //    case ytxIOBLOCK::Analog:{
 //      SerialUSB.print(F("BLOCK 3 - ANALOG - SECTION ")); SerialUSB.print(section); SerialUSB.print(F(" - SIZE: ")); SerialUSB.println(descriptors[block].sectionSize);
 //    }break;
 //    case ytxIOBLOCK::Feedback:{
 //      SerialUSB.print(F("BLOCK 4 - FEEDBACK - SECTION ")); SerialUSB.print(section); SerialUSB.print(F(" - SIZE: ")); SerialUSB.println(descriptors[block].sectionSize);
 //    }break;
    
 //  }

 //  SerialUSB.println(F("DATA IN EEPROM"));
 //  for(int i = 0; i < descriptors[block].sectionSize; i++){
 //    SerialUSB.print(data[i], HEX);
 //    SerialUSB.print(F("\t"));
 //    if(i > 0 && !(i % 8)) SerialUSB.println();
 //  }
  
 //  // SerialUSB.println(F("\nDATA IN RAM"));

 //  // for(int i = 0; i < descriptors[block].sectionSize; i++){
 //  //   SerialUSB.print(*((byte*) (descriptors[i].ramBaseAddress + descriptors[block].sectionSize * section + i)), HEX);
 //  //   SerialUSB.print(F("\t"));
 //  //   if(i>0 && !(i%8)) SerialUSB.println();
 //  // }

 //  SerialUSB.println(F("\n------------------------------------------"));
 //  SerialUSB.println();
}

void memoryHost::ReadFromEEPROM(uint8_t bank, uint8_t block, uint16_t section, void *data, bool rotaryQSTB)
{
  uint16_t address = descriptors[block].eepBaseAddress + descriptors[block].sectionSize * section;
  if (!descriptors[block].unique)
    address +=  bank * bankSize;
  if(rotaryQSTB){
    uint32_t rotaryConfigSize = sizeof(encoder->rotBehaviour) + sizeof(encoder->rotaryConfig);
    uint32_t rotaryFbStart = sizeof(encoder->rotBehaviour) + sizeof(encoder->rotaryConfig) + sizeof(encoder->switchConfig);
    uint32_t rotaryFbSize = sizeof(encoder->rotaryFeedback);

    eep->read(address, (byte*)(data), rotaryConfigSize);
    eep->read(address+rotaryFbStart, (byte*)(data+rotaryFbStart), rotaryFbSize);
  }else{
    eep->read(address, (byte*)(data), descriptors[block].sectionSize);
  }
}

void memoryHost::WriteToEEPROM(uint8_t bank, uint8_t block, uint16_t section, void *data)
{
  uint16_t address = descriptors[block].eepBaseAddress + descriptors[block].sectionSize * section;
  //SerialUSB.println(F("\nWriting to address: "));SerialUSB.println(address); SerialUSB.println();
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

uint8_t memoryHost::LoadBankSingleSection(uint8_t bank, uint8_t block, uint16_t sectionIndex, bool rotaryQSTB)
{
  bankNow = -1;
  //  eep->read(eepIndex+bankSize*bank, (byte*)bankChunk, bankSize);
  switch (block) {
    case ytxIOBLOCK::Encoder: {
        ReadFromEEPROM(bank, ytxIOBLOCK::Encoder, sectionIndex, &encoder[sectionIndex], rotaryQSTB);
      } break;
    case ytxIOBLOCK::Digital: {
        ReadFromEEPROM(bank, ytxIOBLOCK::Digital, sectionIndex, &digital[sectionIndex], false);
      } break;
    case ytxIOBLOCK::Analog: {
        ReadFromEEPROM(bank, ytxIOBLOCK::Analog, sectionIndex, &analog[sectionIndex], false);
      } break;
    case ytxIOBLOCK::Feedback: {
        ReadFromEEPROM(bank, ytxIOBLOCK::Feedback, sectionIndex, &feedback[sectionIndex], false);
      } break;
  }
  return bank;
}

void memoryHost::SaveBank(uint8_t bank)
{
  eep->write(eepIndex + bankSize * bank, (byte*)bankChunk, bankSize);
}

void memoryHost::SaveBlockToEEPROM(uint8_t block)
{
  // TO DO: pasarle un address
  eep->write(0, (byte*) descriptors[block].ramBaseAddress, descriptors[block].sectionSize);
}


void memoryHost::ResetNewMemFlag(void){
  uint16_t address = CTRLR_STATE_GENERAL_SETT_ADDRESS;

  eep->read(address, (byte*) &genSettings, sizeof(genSettingsControllerState));   // read flags byte

  genSettings.flags |= CTRLR_STATE_NEW_MEM_MASK;      // set it

  eep->write(address,  (byte*) &genSettings, sizeof(genSettingsControllerState)); // save to eeprom
}

bool memoryHost::IsCtrlStateMemNew(void){
  uint16_t address = CTRLR_STATE_GENERAL_SETT_ADDRESS;

  eep->read(address, (byte*) &genSettings, sizeof(genSettingsControllerState));   // read flags byte

  if(genSettings.flags & CTRLR_STATE_NEW_MEM_MASK){    // check new memory flag
    memset(&genSettings, 0, sizeof(genSettingsControllerState));

    eep->write(address,  (byte*) &genSettings, sizeof(genSettingsControllerState)); // save to eeprom
    return true;
  }else{
    return false;
  }
}

void memoryHost::SaveControllerState(void){
  uint16_t address = 0;

  if(validConfigInEEPROM){
    // SAVE GENERAL SETTINGS
    address = CTRLR_STATE_GENERAL_SETT_ADDRESS;

    eep->read(address, (byte*) &genSettings, sizeof(genSettingsControllerState));   // read flags byte

    if(genSettings.lastCurrentBank != currentBank){    // check new memory flag
      genSettings.lastCurrentBank = currentBank;      // Save new currentBank
      eep->write(address,  (byte*) &genSettings, sizeof(genSettingsControllerState)); // save to eeprom
      SerialUSB.println("Saved current BANK");
    }

    // SAVE ELEMENTS
    address = CTRLR_STATE_ELEMENTS_ADDRESS;

    for (int bank = 0; bank < config->banks.count; bank++) { // Cycle all banks
      for (uint8_t encNo = 0; encNo < config->inputs.encoderCount; encNo++) {     // SWEEP ALL ENCODERS
        EncoderInputs::encoderBankData aux;
        eep->read(address, (byte*) &aux, sizeof(EncoderInputs::encoderBankData));
        EncoderInputs::encoderBankData* auxP = encoderHw.GetCurrentEncoderStateData(bank, encNo);
        
        if(auxP != NULL){
          if(memcmp(&aux, auxP, sizeof(aux))){
            eep->write(address, (byte*) encoderHw.GetCurrentEncoderStateData(bank, encNo), sizeof(EncoderInputs::encoderBankData));
            SerialUSB.print("BANK ");SerialUSB.print(bank);SerialUSB.print(" ENCODER ");SerialUSB.print(encNo); SerialUSB.print(" CHANGED. NEW VALUE: "); 
             eep->read(address, (byte*) &aux, sizeof(EncoderInputs::encoderBankData));
            SerialUSB.println(aux.encoderValue);
          }
        }
        
        address += sizeof(EncoderInputs::encoderBankData);
      }

      for (uint16_t digNo = 0; digNo < config->inputs.digitalCount; digNo++) {
       
      }

      for (uint8_t analogNo = 0; analogNo < config->inputs.analogCount; analogNo++) {
       
      }
    }

    // SAVE MIDI BUFFER 
    address = CTRLR_STATE_MIDI_BUFFER_ADDR;

    uint8_t auxMidiBufferValuesWrite[128];
    uint8_t auxMidiBufferValuesRead[128];
    int16_t elementsLeftToCopy = midiRxSettings.midiBufferSize7;
    uint16_t bufferIdx = 0;
    while(elementsLeftToCopy > 0){
      uint8_t w = (elementsLeftToCopy < 128 ? elementsLeftToCopy : 128);
      for(int i = 0; i < w; i++){
        auxMidiBufferValuesWrite[i] = midiMsgBuf7[bufferIdx++].value;
      }
      eep->read(address, (byte*) auxMidiBufferValuesRead, w);
      if(memcmp(auxMidiBufferValuesRead, auxMidiBufferValuesWrite, w)){
        eep->write(address, (byte*) auxMidiBufferValuesWrite, w);
        SerialUSB.println("Saved MIDI BUFFER");
      }
      
      elementsLeftToCopy -= w;
      address += w;
    }

    // for(int bufferIdx = 0; bufferIdx < midiRxSettings.midiBufferSize14; bufferIdx++){
      
    // }

  }
    
  return;
}

void memoryHost::LoadControllerState(){
  uint16_t address = 0;
  
  // LOAD ELEMENTS
  address = CTRLR_STATE_ELEMENTS_ADDRESS;

  for (int bank = 0; bank < config->banks.count; bank++) { // Cycle all banks
    // SerialUSB.print("--------------------------------------------"); 
    // SerialUSB.print("   LOADING BANK "); SerialUSB.println(bank);
    // SerialUSB.println("--------------------------------------------"); 
    for (uint8_t encNo = 0; encNo < config->inputs.encoderCount; encNo++) {     // SWEEP ALL ENCODERS
      // SerialUSB.print("--------------------------------------------"); 
      // SerialUSB.print("     Address: "); SerialUSB.println(address);
      // SerialUSB.print("--------------------------------------------"); 
      // SerialUSB.print("     Encoder "); SerialUSB.println(encNo);
      // SerialUSB.println("--------------------------"); 

      eep->read(address, (byte*) encoderHw.GetCurrentEncoderStateData(bank, encNo), sizeof(EncoderInputs::encoderBankData));

      // EncoderInputs::encoderBankData *aux = encoderHw.GetCurrentEncoderStateData(bank, encNo);

      // printPointer(aux);
 
      // SerialUSB.print("Value: "); SerialUSB.println(aux->encoderValue);
      // SerialUSB.print("Value 2cc: "); SerialUSB.println(aux->encoderValue2cc);
      // SerialUSB.print("Shift Value: "); SerialUSB.println(aux->encoderShiftValue);
      // SerialUSB.print("switch last value: "); SerialUSB.println(aux->switchLastValue);
      // SerialUSB.print("switch input state: "); SerialUSB.println(aux->switchInputState);
      // SerialUSB.print("switch input state prev: "); SerialUSB.println(aux->switchInputStatePrev);
      // SerialUSB.print("pulse counter: "); SerialUSB.println(aux->pulseCounter);
      // SerialUSB.print("Shift Rotary Action: "); SerialUSB.println(aux->shiftRotaryAction);
      // SerialUSB.print("Enc fine adjust: "); SerialUSB.println(aux->encFineAdjust);
      // SerialUSB.print("Double CC: "); SerialUSB.println(aux->doubleCC);
      // SerialUSB.print("BSCO: "); SerialUSB.println(aux->buttonSensitivityControlOn);
      // SerialUSB.println();

      address += sizeof(EncoderInputs::encoderBankData);
    }

    for (uint16_t digNo = 0; digNo < config->inputs.digitalCount; digNo++) {
      
    }

    for (uint8_t analogNo = 0; analogNo < config->inputs.analogCount; analogNo++) {
      
    }
  }

  // LOAD GENERAL SETTINGS
  address = CTRLR_STATE_GENERAL_SETT_ADDRESS;

  eep->read(address, (byte*) &genSettings, sizeof(genSettingsControllerState));   // read flags byte

  if(genSettings.lastCurrentBank != currentBank){    // check new memory flag
    ChangeToBank(genSettings.lastCurrentBank);
  }
  
  // LOAD MIDI BUFFER
  address = CTRLR_STATE_MIDI_BUFFER_ADDR;
  // uint8_t auxMidiBufferValues[128];
  uint8_t auxValue = 0;
  for(int bufferIdx = 0; bufferIdx < midiRxSettings.midiBufferSize7; bufferIdx++){
    eep->read(address+bufferIdx, (byte*) &auxValue, 1);
    midiMsgBuf7[bufferIdx].value = auxValue;
    midiMsgBuf7[bufferIdx].banksToUpdate = midiMsgBuf7[bufferIdx].banksPresent;
  }
  
  return;
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
