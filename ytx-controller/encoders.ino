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

#include "headers/EncoderInputs.h"

//----------------------------------------------------------------------------------------------------
// ENCODER FUNCTIONS
//----------------------------------------------------------------------------------------------------
void EncoderInputs::Init(uint8_t maxBanks, uint8_t maxEncoders, SPIClass *spiPort){
  nBanks = 0;
  nEncoders = 0;
  nModules = 0;    
  
  if(!maxBanks || !maxEncoders) return;    // If number of encoders is zero, return;

  // CHECK WHETHER AMOUNT OF DIGITAL INPUTS IN MODULES COMBINED MATCH THE AMOUNT OF DIGITAL INPUTS IN CONFIG
  // AMOUNT OF DIGITAL MODULES
  uint8_t encodersInConfig = 0;
  
  for (int nMod = 0; nMod < MAX_ENCODER_MODS; nMod++) {
    if (config->hwMapping.encoder[nMod]) {
      modulesInConfig.encoders++;
    }
    switch (config->hwMapping.encoder[nMod]) {
      case EncoderModuleTypes::E41H:
      case EncoderModuleTypes::E41V:
      case EncoderModuleTypes::E41H_D:
      case EncoderModuleTypes::E41V_D:{
          encodersInConfig += defE41module.components.nEncoders;
        } break;
      default: break;
    }
  }
  
  if (encodersInConfig != maxEncoders) {
    SerialUSB.println("Error in config: Number of encoders does not match modules in config");
    SerialUSB.print("nEncoders: "); SerialUSB.println(maxEncoders);
    SerialUSB.print("Modules: "); SerialUSB.println(encodersInConfig);
    return;
  } else {
    SerialUSB.println("nEncoders and module config match");
  }

  nBanks = maxBanks;
  nEncoders = maxEncoders;
  nModules = nEncoders/4;     // HARDCODE: N° of encoders in module

//  SerialUSB.print("N ENCODERS MODULES: ");
//  SerialUSB.println(nModules);
  
  // First dimension is an array of pointers (banks), each pointing to N encoder structs - https://www.eskimo.com/~scs/cclass/int/sx9b.html
  eBankData = (encoderBankData**) memHost->AllocateRAM(nBanks*sizeof(encoderBankData*));
  eData = (encoderData*) memHost->AllocateRAM(nEncoders*sizeof(encoderData));
  encMData = (moduleData*) memHost->AllocateRAM(nModules*sizeof(moduleData));

//  SerialUSB.print("Size of encoder data ");
//  SerialUSB.println(sizeof(encoderData));

  for (int b = 0; b < nBanks; b++){
    eBankData[b] = (encoderBankData*) memHost->AllocateRAM(nEncoders*sizeof(encoderBankData));

    for(int e = 0; e < nEncoders; e++){
       eBankData[b][e].encoderValue = random(encoder[e].rotaryConfig.parameter[rotary_maxLSB]);
       eBankData[b][e].encoderShiftValue = 0;
       eBankData[b][e].encoderValue2cc = 0;
       eBankData[b][e].pulseCounter = 0;
       eBankData[b][e].switchLastValue = 0;
       eBankData[b][e].encoderValue2cc = 0;
       eBankData[b][e].switchInputState = 0;
       eBankData[b][e].switchInputStatePrev = false;
       eBankData[b][e].shiftRotaryAction = false;
       eBankData[b][e].encFineAdjust = false;
       eBankData[b][e].doubleCC = false;
    }
  }

  for(int e = 0; e < nEncoders; e++){
    eData[e].encoderDirection = 0;
    eData[e].encoderDirectionPrev = 0;
    eData[e].encoderValuePrev = 0;
    eData[e].encoderValuePrev2cc = 0;
    eData[e].encoderChange = false;
    eData[e].millisUpdatePrev = 0;
    eData[e].switchHWState = 0;
    eData[e].switchHWStatePrev = 0;
    eData[e].encoderState = RFS_START;
    eData[e].a = 0;
    eData[e].b = 0;
    eData[e].thisEncoderBank = 0;
    eData[e].bankShifted = false;
  }

  pinMode(encodersMCPChipSelect, OUTPUT);

  // DISABLE HARDWARE ADDRESSING FOR ALL CHIPS - ONLY NEEDED FOR RESET
  DisableHWAddress();
  // Set pullups on all pins
  SetPullUps();
  EnableHWAddress();
  
//  SerialUSB.println("ENCODERS");
  for (int n = 0; n < nModules; n++){
    if(config->hwMapping.encoder[n] != EncoderModuleTypes::ENCODER_NONE){
      encMData[n].moduleOrientation = (config->hwMapping.encoder[n]-1)%2;    // 0 -> HORIZONTAL , 1 -> VERTICAL
      encMData[n].detent =  (config->hwMapping.encoder[n] == EncoderModuleTypes::E41H_D) ||
                            (config->hwMapping.encoder[n] == EncoderModuleTypes::E41V_D);   
//      SerialUSB.print("encMData[0].moduleOrientation: "); SerialUSB.println(encMData[n].moduleOrientation);
    }else{
//      encMData[n].moduleOrientation = e41orientation::HORIZONTAL;
//      SerialUSB.print("encMData[0].moduleOrientation - b: "); SerialUSB.println(encMData[n].moduleOrientation);
      SetStatusLED(STATUS_BLINK, 1, STATUS_FB_ERROR);
      return;
    }
    
    encodersMCP[n].begin(spiPort, encodersMCPChipSelect, n);  
//    printPointer(&encodersMCP[n]);
    
    encMData[n].mcpState = 0;
    encMData[n].mcpStatePrev = 0;
    
    // AFTER INITIALIZATION SET NEXT ADDRESS ON EACH MODULE (EXCEPT 7 and 15, cause they don't have a module next on the chain)
    uint8_t mcpAddress = n % 8;
    if (nModules > 1) {
      SetNextAddress(&encodersMCP[n], mcpAddress + 1);
    }
    
    for(int i=0; i<16; i++){
      if(i != defE41module.nextAddressPin[0] && i != defE41module.nextAddressPin[1] && 
         i != defE41module.nextAddressPin[2] && i != (defE41module.nextAddressPin[2]+1)){       // HARDCODE: Only E41 module exists for now
        encodersMCP[n].pullUp(i, HIGH);
      }
    }
    delay(1); // settle pullups
    encMData[n].mcpState = encodersMCP[n].digitalRead();
    for (int e = 0; e < 4; e++){
      uint8_t pinState = 0;
      if (encMData[e].mcpState & (1 << defE41module.encPins[e%(defE41module.components.nEncoders)][0])){
        pinState |= 2; // Get new state for pin A
        eData[e].a = 1; // Get new state for pin A
      }else  eData[e].a = 0;
      
      if (encMData[e].mcpState & (1 << defE41module.encPins[e%(defE41module.components.nEncoders)][1])){
        pinState |= 1; // Get new state for pin B
        eData[e].b = 1; // Get new state for pin B
      }else  eData[e].b = 0;
  
      eData[e].encoderState = pinState;
    } 
//      SerialUSB.print("MODULE ");SerialUSB.print(n);SerialUSB.print(": ");
//      for (int i = 0; i < 16; i++) {
//        SerialUSB.print( (encMData[n].mcpState >> (15 - i)) & 0x01, BIN);
//        if (i == 9 || i == 6) SerialUSB.print(" ");
//      }
//      SerialUSB.print("\n");
  }  
//  SerialUSB.print(encMData[0].mcpState,BIN); SerialUSB.print(" ");
//  SerialUSB.print(encMData[1].mcpState,BIN); SerialUSB.print(" ");
//  SerialUSB.print(encMData[2].mcpState,BIN); SerialUSB.print(" ");
//  SerialUSB.print(encMData[3].mcpState,BIN); SerialUSB.print(" ");
//  SerialUSB.print(encMData[4].mcpState,BIN); SerialUSB.print(" ");
//  SerialUSB.print(encMData[5].mcpState,BIN); SerialUSB.print(" ");
//  SerialUSB.print(encMData[6].mcpState,BIN); SerialUSB.print(" ");
//  SerialUSB.print(encMData[7].mcpState,BIN); SerialUSB.println(" ");
//  while(1);
  begun = true;
  
  return;
}

//#define PRINT_MODULE_STATE

void EncoderInputs::Read(){
  if(!nBanks || !nEncoders || !nModules) return;    // If number of encoders is zero, return;

  if(!begun) return;    // If didn't go through INIT, return;

  // The priority system adds to a priority list up to 2 modules to read if they are being used.
  // If the list is empty, the first two modules that are detected to have a change are added to the list
  // If the list is not empty, the only modules that are read via SPI are the ones that are on the priority list
  if(priorityCount && (millis()-priorityTime > PRIORITY_ELAPSE_TIME_MS)){
    priorityCount = 0;
//    SerialUSB.print("Priority list emptied");
  }
  if(priorityCount >= 1){
    uint8_t priorityModule = priorityList[0];
    encMData[priorityModule].mcpState = encodersMCP[priorityModule].digitalRead();
//    SerialUSB.print("Priority Read Module: ");SerialUSB.println(priorityModule);
  }
  if(priorityCount == 2){
    uint8_t priorityModule = priorityList[1];
    encMData[priorityModule].mcpState = encodersMCP[priorityModule].digitalRead();
//    SerialUSB.print("Priority Read Module: ");SerialUSB.println(priorityModule);
  }else{  
    // READ ALL MODULE'S STATE
    for (int n = 0; n < nModules; n++){  
      encMData[n].mcpState = encodersMCP[n].digitalRead();
      #if defined(PRINT_MODULE_STATE)
      for (int i = 0; i < 16; i++) {
        SerialUSB.print( (encMData[n].mcpState >> (15 - i)) & 0x01, BIN);
        if (i == 9 || i == 6) SerialUSB.print(" ");
      }
      SerialUSB.print("  ");
      #endif
    }
    #if defined(PRINT_MODULE_STATE)
    SerialUSB.print("\n"); 
    #endif
  } 
 
  uint8_t encNo = 0; 
  uint8_t nEncodInMod = 0;
  for(uint8_t mcpNo = 0; mcpNo < nModules; mcpNo++){
    switch(config->hwMapping.encoder[mcpNo]){
      case EncoderModuleTypes::E41H:
      case EncoderModuleTypes::E41V:
      case EncoderModuleTypes::E41H_D:
      case EncoderModuleTypes::E41V_D:{
          nEncodInMod = defE41module.components.nEncoders;              // HARDCODE: CHANGE WHEN DIFFERENT MODULES ARE ADDED
        } break;
      default: break;
    }

    if( encMData[mcpNo].mcpState != encMData[mcpNo].mcpStatePrev){
      encMData[mcpNo].mcpStatePrev = encMData[mcpNo].mcpState; 
      
      // READ N° OF ENCODERS IN ONE MCP
      for(int n = 0; n < nEncodInMod; n++){
        EncoderCheck(mcpNo, encNo+n);
        SwitchCheck(mcpNo, encNo+n);
      }
    }
    // CHECK TIMEOUT FOR DOUBLE CLICK
    for(int n = 0; n < nEncodInMod; n++){  
      if( doubleClickIndex > 0){
        if( doubleClickIndex == 1 &&
            doubleClickSet[0] == (encNo+n) && 
           ((millis()-antMillisDoubleClick[0]) >= DOUBLE_CLICK_WAIT)){
          doubleClickIndex = 0;
          SerialUSB.print(encNo+n); SerialUSB.println(" double click not detected (1)"); 
        }
        if( doubleClickIndex == 2 &&
            doubleClickSet[1] == (encNo+n) && 
          ((millis()-antMillisDoubleClick[1]) >= DOUBLE_CLICK_WAIT)){
          doubleClickIndex = 1;
          SerialUSB.print(encNo+n); SerialUSB.println(" double click not detected (2)"); 
        }
      }
    }
    encNo = encNo + nEncodInMod;    // Next module
  }
  
  return;
}

void EncoderInputs::SwitchCheck(uint8_t mcpNo, uint8_t encNo){  
  if(encoder[encNo].switchConfig.mode == switchModes::switch_mode_none) return;
  
  eData[encNo].switchHWState = !(encMData[mcpNo].mcpState & (1 << defE41module.encSwitchPins[encNo%(defE41module.components.nEncoders)]));

  
  if(eData[encNo].switchHWState != eData[encNo].switchHWStatePrev){
    eData[encNo].switchHWStatePrev = eData[encNo].switchHWState;
   
    if (CheckIfBankShifter(encNo, eData[encNo].switchHWState)){
      // IF IT IS BANK SHIFTER, RETURN, DON'T DO ACTION FOR THIS SWITCH
      //SerialUSB.println("IS SHIFTER");
      return;
    }   

    // DOUBLE CLICK FEATURE
    bool doubleClickDetected = false;

    if(eData[encNo].switchHWState && encoder[encNo].switchConfig.doubleClick != switchDoubleClickModes::switch_doubleClick_none){
      if(!doubleClickIndex){
        doubleClickSet[0] = encNo;
        antMillisDoubleClick[0] = millis();
        doubleClickIndex++;
        SerialUSB.print(encNo); SerialUSB.println(" waiting for double click (1)"); 
        return;
      }else if(doubleClickIndex > 0){
        if(doubleClickSet[0] == encNo && ((millis()-antMillisDoubleClick[0]) < DOUBLE_CLICK_WAIT)){
          doubleClickDetected = true;
          doubleClickIndex = 0;
          SerialUSB.print(encNo); SerialUSB.println(" double click happened (1)"); 
        }
        if(doubleClickSet[0] != encNo && doubleClickIndex == 1){
          doubleClickSet[1] = encNo;
          antMillisDoubleClick[1] = millis();
          doubleClickIndex++;
          SerialUSB.print(encNo); SerialUSB.println(" waiting for double click (2)"); 
          return;
        }else if(doubleClickSet[1] == encNo && doubleClickIndex == 2 && ((millis()-antMillisDoubleClick[1]) < DOUBLE_CLICK_WAIT)){
          doubleClickDetected = true;
          doubleClickIndex = 1;
          SerialUSB.print(encNo); SerialUSB.println(" double click happened (2)"); 
        }
      }

      if(doubleClickDetected){
        uint16_t minValue = encoder[encNo].rotaryConfig.parameter[rotary_minMSB]<<7 | 
                            encoder[encNo].rotaryConfig.parameter[rotary_minLSB];

        uint16_t maxValue = encoder[encNo].rotaryConfig.parameter[rotary_maxMSB]<<7 | 
                            encoder[encNo].rotaryConfig.parameter[rotary_maxLSB];
        uint8_t msgType = encoder[encNo].rotaryConfig.message;

        if(!( msgType == rotary_msg_nrpn || msgType == rotary_msg_rpn || msgType == rotary_msg_pb || 
              msgType == switch_msg_nrpn || msgType == switch_msg_rpn || msgType == switch_msg_pb)){
          minValue = minValue & 0x7F;
          maxValue = maxValue & 0x7F;
        }

        if(encoder[encNo].switchConfig.doubleClick == switchDoubleClickModes::switch_doubleClick_2min){
          eBankData[eData[encNo].thisEncoderBank][encNo].encoderValue = minValue;
        }else if(encoder[encNo].switchConfig.doubleClick == switchDoubleClickModes::switch_doubleClick_2max){
          eBankData[eData[encNo].thisEncoderBank][encNo].encoderValue = maxValue;
        }else if(encoder[encNo].switchConfig.doubleClick == switchDoubleClickModes::switch_doubleClick_2center){
          // Get center value for even and odd ranges differently
          if(!(abs(maxValue-minValue)%2)){
            eBankData[eData[encNo].thisEncoderBank][encNo].encoderValue = (minValue + maxValue)/2;
          }else{
            eBankData[eData[encNo].thisEncoderBank][encNo].encoderValue = (minValue + maxValue)/2 + 1;
          }
          SerialUSB.print("Center value: "); SerialUSB.println(eBankData[eData[encNo].thisEncoderBank][encNo].encoderValue);
        }
        feedbackHw.SetChangeEncoderFeedback(FB_ENCODER, 
                                            encNo, 
                                            eBankData[eData[encNo].thisEncoderBank][encNo].encoderValue, 
                                            encMData[mcpNo].moduleOrientation, 
                                            false);
      }
      return;
    }
    // END OF DOUBLE CLICK CHECK

    // SINGLE CLICK ACTION 
    if (eData[encNo].switchHWState &&
        encoder[encNo].switchConfig.action == switchActions::switch_toggle){   
      eBankData[eData[encNo].thisEncoderBank][encNo].switchInputState = !eBankData[eData[encNo].thisEncoderBank][encNo].switchInputState;
    } else if ( eData[encNo].switchHWState && 
                encoder[encNo].switchConfig.action == switchActions::switch_momentary){
      eBankData[eData[encNo].thisEncoderBank][encNo].switchInputState = 1;
    } else if(!eData[encNo].switchHWState && 
              encoder[encNo].switchConfig.action == switchActions::switch_momentary ||
              encoder[encNo].switchConfig.message == switchMessageTypes::switch_msg_key){
      eBankData[eData[encNo].thisEncoderBank][encNo].switchInputState = 0;
    }
//    SerialUSB.print("Encoder switch "); SerialUSB.print(encNo); SerialUSB.print(" changed to value: ");SerialUSB.println(eBankData[currentBank][encNo].switchInputState);
    SwitchAction(mcpNo, encNo);
  }
}


void EncoderInputs::SwitchAction(uint8_t mcpNo, uint8_t encNo) { 
  bool newSwitchState = eBankData[currentBank][encNo].switchInputState;
  
  if(newSwitchState != eBankData[currentBank][encNo].switchInputStatePrev){
    eBankData[currentBank][encNo].switchInputStatePrev = newSwitchState;  // update previous
    
    // Get config parameters for switch action
    uint16_t paramToSend = encoder[encNo].switchConfig.parameter[switch_parameter_MSB] << 7 |
                           encoder[encNo].switchConfig.parameter[switch_parameter_LSB];
    uint8_t channelToSend = encoder[encNo].switchConfig.channel + 1;
    uint16_t minValue = encoder[encNo].switchConfig.parameter[switch_minValue_MSB] << 7 |
                        encoder[encNo].switchConfig.parameter[switch_minValue_LSB];
    uint16_t maxValue = encoder[encNo].switchConfig.parameter[switch_maxValue_MSB] << 7 |
                        encoder[encNo].switchConfig.parameter[switch_maxValue_LSB];
  
    uint16_t valueToSend = 0;
    bool updateSwitchFb = false;
  
    if (newSwitchState) {
      valueToSend = maxValue;
    } else {
      valueToSend = minValue;
    }

    if (encoder[encNo].switchConfig.mode == switchModes::switch_mode_shift_rot){  // SHIFT ROTARY ACTION
      eBankData[eData[encNo].thisEncoderBank][encNo].shiftRotaryAction = newSwitchState;
      if(eBankData[eData[encNo].thisEncoderBank][encNo].shiftRotaryAction){
        eData[encNo].encoderValuePrev = eBankData[eData[encNo].thisEncoderBank][encNo].encoderShiftValue;
        feedbackHw.SetChangeEncoderFeedback(FB_ENCODER, 
                                            encNo, 
                                            eBankData[eData[encNo].thisEncoderBank][encNo].encoderShiftValue, 
                                            encMData[mcpNo].moduleOrientation, 
                                            false);           
      }else{
        eData[encNo].encoderValuePrev = eBankData[eData[encNo].thisEncoderBank][encNo].encoderValue;
        feedbackHw.SetChangeEncoderFeedback(FB_ENCODER, 
                                            encNo, 
                                            eBankData[eData[encNo].thisEncoderBank][encNo].encoderValue, 
                                            encMData[mcpNo].moduleOrientation, 
                                            false);
      }

      updateSwitchFb = true;
//      return;
    }else if(encoder[encNo].switchConfig.mode == switchModes::switch_mode_fine){ // ENCODER FINE ADJUST
      eBankData[eData[encNo].thisEncoderBank][encNo].encFineAdjust = newSwitchState;
      updateSwitchFb = true;
//      return;
    }else if(encoder[encNo].switchConfig.mode == switchModes::switch_mode_2cc){ // DOUBLE CC
      eBankData[eData[encNo].thisEncoderBank][encNo].doubleCC = newSwitchState;
      updateSwitchFb = true;
      // SerialUSB.print("ENCODER ");SerialUSB.print(encNo);
      // SerialUSB.print(": 2cc "); SerialUSB.println(newSwitchState ? "ON":"OFF");
//      return;
    }else if(encoder[encNo].switchConfig.mode == switchModes::switch_mode_quick_shift){ // QUICK SHIFT TO BANK #
      byte nextBank = 0;
      updateSwitchFb = true;
      if(eData[encNo].bankShifted){
        nextBank = currentBank;
      }else{
        nextBank = paramToSend & 0xFF;
      }
      // Check just in case
      if(nextBank >= nBanks) return;
      
      // SET SAME VALUE FOR SWITCH INPUT
      eBankData[nextBank][encNo].switchInputState = eBankData[eData[encNo].thisEncoderBank][encNo].switchInputState;
      eBankData[nextBank][encNo].switchInputStatePrev = eBankData[eData[encNo].thisEncoderBank][encNo].switchInputStatePrev;
      
      eData[encNo].thisEncoderBank = nextBank;
      eData[encNo].bankShifted = !eData[encNo].bankShifted;
      
      // SHIFT BANK (PARAMETER LSB) ONLY FOR THIS ENCODER 
      memHost->LoadBankSingleSection(nextBank, ytxIOBLOCK::Encoder, encNo);
      
      // UPDATE FEEDBACK FOR NEW BANK
      feedbackHw.SetChangeEncoderFeedback(FB_ENCODER, 
                                          encNo, 
                                          eBankData[eData[encNo].thisEncoderBank][encNo].encoderValue, 
                                          encMData[encNo/4].moduleOrientation,
                                          false);
      
    }else if(encoder[encNo].switchConfig.mode == switchModes::switch_mode_quick_shift_note){ // QUICK SHIFT TO BANK # + NOTE
      eData[encNo].bankShifted = !eData[encNo].bankShifted;
      updateSwitchFb = true;
      // SHIFT BANK (PARAMETER MSB) ONLY FOR THIS ENCODER 
      byte nextBank = 0;
      if(eData[encNo].bankShifted){
        nextBank = currentBank;
      }else{
        nextBank = paramToSend & 0xFF;
      }
      // Check just in case
      if(nextBank >= nBanks) return;
      
      // SET SAME VALUE FOR SWITCH INPUT
//      eBankData[nextBank][encNo].switchInputState = eBankData[eData[encNo].thisEncoderBank][encNo].switchInputState;
//      eBankData[nextBank][encNo].switchInputStatePrev = eBankData[eData[encNo].thisEncoderBank][encNo].switchInputStatePrev;
      
      eData[encNo].thisEncoderBank = nextBank;
      eData[encNo].bankShifted = !eData[encNo].bankShifted;
      
      // SHIFT BANK (PARAMETER LSB) ONLY FOR THIS ENCODER 
      memHost->LoadBankSingleSection(nextBank, ytxIOBLOCK::Encoder, encNo);
      
      // UPDATE FEEDBACK FOR NEW BANK
      feedbackHw.SetChangeEncoderFeedback(FB_ENCODER, 
                                          encNo, 
                                          eBankData[eData[encNo].thisEncoderBank][encNo].encoderValue, 
                                          encMData[encNo/4].moduleOrientation,
                                          false);
                                          
      // SEND NOTE
      if (encoder[encNo].switchConfig.midiPort & 0x01)
        MIDI.sendNoteOn( (paramToSend >> 7) & 0x7f, valueToSend & 0x7f, channelToSend);
      if (encoder[encNo].switchConfig.midiPort & 0x02)
        MIDIHW.sendNoteOn( (paramToSend >> 7) & 0x7f, valueToSend & 0x7f, channelToSend);
      return;
    }else if(encoder[encNo].switchConfig.mode == switchModes::switch_mode_message){  // SEND NORMAL MIDI MESSAGE
      switch (encoder[encNo].switchConfig.message) {
        case switchMessageTypes::switch_msg_note: {
            if (encoder[encNo].switchConfig.midiPort & 0x01){
              if(valueToSend) MIDI.sendNoteOn( paramToSend & 0x7f, valueToSend & 0x7f, channelToSend);
              else            MIDI.sendNoteOff( paramToSend & 0x7f, valueToSend & 0x7f, channelToSend);
            }
            if (encoder[encNo].switchConfig.midiPort & 0x02){
              if(valueToSend) MIDIHW.sendNoteOn( paramToSend & 0x7f, valueToSend & 0x7f, channelToSend);
              else            MIDIHW.sendNoteOff( paramToSend & 0x7f, valueToSend & 0x7f, channelToSend);
            }
          } break;
        case switchMessageTypes::switch_msg_cc: {
            if (encoder[encNo].switchConfig.midiPort & 0x01)
              MIDI.sendControlChange( paramToSend & 0x7f, valueToSend & 0x7f, channelToSend);
            if (encoder[encNo].switchConfig.midiPort & 0x02)
              MIDIHW.sendControlChange( paramToSend & 0x7f, valueToSend & 0x7f, channelToSend);
          } break;
        case switchMessageTypes::switch_msg_pc: {
            if (encoder[encNo].switchConfig.midiPort & 0x01 && valueToSend != minValue)
              MIDI.sendProgramChange( paramToSend & 0x7f, channelToSend);
            if (encoder[encNo].switchConfig.midiPort & 0x02 && valueToSend)
              MIDIHW.sendProgramChange( paramToSend & 0x7f, channelToSend);
          } break;
        case switchMessageTypes::switch_msg_pc_m: {
            if (encoder[encNo].switchConfig.midiPort & 0x01) {
              if (currentProgram[midi_usb - 1][channelToSend - 1] > 0 && newSwitchState) {
                currentProgram[midi_usb - 1][channelToSend - 1]--;
                MIDI.sendProgramChange(currentProgram[midi_usb - 1][channelToSend - 1], channelToSend);
              }
            }
            if (encoder[encNo].switchConfig.midiPort & 0x02) {
              if (currentProgram[midi_hw - 1][channelToSend - 1] > 0 && newSwitchState) {
                currentProgram[midi_hw - 1][channelToSend - 1]--;
                MIDIHW.sendProgramChange(currentProgram[midi_hw - 1][channelToSend - 1], channelToSend);
              }
            }
          } break;
        case switchMessageTypes::switch_msg_pc_p: {
            if (encoder[encNo].switchConfig.midiPort & 0x01) {
              if (currentProgram[midi_usb - 1][channelToSend - 1] < 127 && newSwitchState) {
                currentProgram[midi_usb - 1][channelToSend - 1]++;
                MIDI.sendProgramChange(currentProgram[midi_usb - 1][channelToSend - 1], channelToSend);
              }
            }
            if (encoder[encNo].switchConfig.midiPort & 0x02) {
              if (currentProgram[midi_hw - 1][channelToSend - 1] < 127 && newSwitchState) {
                currentProgram[midi_hw - 1][channelToSend - 1]++;
                MIDIHW.sendProgramChange(currentProgram[midi_hw - 1][channelToSend - 1], channelToSend);
              }
            }
          } break;
        case switchMessageTypes::switch_msg_nrpn: {
            if (encoder[encNo].switchConfig.midiPort & 0x01) {
              MIDI.sendControlChange( 99, (paramToSend >> 7) & 0x7F, channelToSend);
              MIDI.sendControlChange( 98, (paramToSend & 0x7F), channelToSend);
              MIDI.sendControlChange( 6, (valueToSend >> 7) & 0x7F, channelToSend);
              MIDI.sendControlChange( 38, (valueToSend & 0x7F), channelToSend);
            }
            if (encoder[encNo].switchConfig.midiPort & 0x02) {
              MIDIHW.sendControlChange( 99, (paramToSend >> 7) & 0x7F, channelToSend);
              MIDIHW.sendControlChange( 98, (paramToSend & 0x7F), channelToSend);
              MIDIHW.sendControlChange( 6, (valueToSend >> 7) & 0x7F, channelToSend);
              MIDIHW.sendControlChange( 38, (valueToSend & 0x7F), channelToSend);
            }
          } break;
        case switchMessageTypes::switch_msg_rpn: {
            if (encoder[encNo].switchConfig.midiPort & 0x01) {
              MIDI.sendControlChange( 101, (paramToSend >> 7) & 0x7F, channelToSend);
              MIDI.sendControlChange( 100, (paramToSend & 0x7F), channelToSend);
              MIDI.sendControlChange( 6, (valueToSend >> 7) & 0x7F, channelToSend);
              MIDI.sendControlChange( 38, (valueToSend & 0x7F), channelToSend);
            }
            if (encoder[encNo].switchConfig.midiPort & 0x02) {
              MIDIHW.sendControlChange( 101, (paramToSend >> 7) & 0x7F, channelToSend);
              MIDIHW.sendControlChange( 100, (paramToSend & 0x7F), channelToSend);
              MIDIHW.sendControlChange( 6, (valueToSend >> 7) & 0x7F, channelToSend);
              MIDIHW.sendControlChange( 38, (valueToSend & 0x7F), channelToSend);
            }
          } break;
        case switchMessageTypes::switch_msg_pb: {
            int16_t valuePb = mapl(valueToSend, minValue, maxValue,((int16_t) minValue)-8192, ((int16_t) maxValue)-8192);
            if (encoder[encNo].switchConfig.midiPort & 0x01)
              MIDI.sendPitchBend( valuePb, channelToSend);
            if (encoder[encNo].switchConfig.midiPort & 0x02)
              MIDIHW.sendPitchBend( valuePb, channelToSend);
          } break;
        case switchMessageTypes::switch_msg_key: {
            if(valueToSend == maxValue){
              if (encoder[encNo].switchConfig.parameter[switch_modifier])
                Keyboard.press(encoder[encNo].switchConfig.parameter[switch_modifier]);
              if (encoder[encNo].switchConfig.parameter[switch_key])
                Keyboard.press(encoder[encNo].switchConfig.parameter[switch_key]);
            }else{
              Keyboard.releaseAll();
            }
          } break;
        default: break;
      }
      SetStatusLED(STATUS_BLINK, 1, statusLEDtypes::STATUS_FB_MIDI_OUT);
  
      if(componentInfoEnabled && (GetHardwareID(ytxIOBLOCK::Encoder, encNo) != lastComponentInfoId)){
        SendComponentInfo(ytxIOBLOCK::Encoder, encNo);
      }
    }
  
    if( encoder[encNo].switchFeedback.source == fb_src_local || 
        encoder[encNo].switchConfig.message == switchMessageTypes::switch_msg_key ||
        updateSwitchFb){
      uint16_t fbValue = 0;
      if(encoder[encNo].switchFeedback.source == fb_src_local && encoder[encNo].switchFeedback.localBehaviour == fb_lb_always_on){
        fbValue = true;
      }else{
        fbValue = valueToSend;
      } 
      eBankData[currentBank][encNo].switchLastValue = valueToSend;

      feedbackHw.SetChangeEncoderFeedback(FB_ENCODER_SWITCH, encNo, fbValue, encMData[encNo/4].moduleOrientation, false);   
    }
  }
}

void EncoderInputs::EncoderCheck(uint8_t mcpNo, uint8_t encNo){
  if(encoder[encNo].rotaryConfig.message == rotaryMessageTypes::rotary_msg_none) return;
  static unsigned long antMicrosEncoder = 0;
  
  // ENCODER CHANGED?
  uint8_t s = eData[encNo].encoderState & 3;    // Get last state

  uint8_t pinState = 0;
  
  if (encMData[mcpNo].mcpState & (1 << defE41module.encPins[encNo%(defE41module.components.nEncoders)][0])){  // If the pin A for this encoder is HIGH
    pinState |= 2; // Save new state for pin A
    eData[encNo].a = 1; // Save new state for pin A
    //s |= 8;
  }else  eData[encNo].a = 0;
  
  if (encMData[mcpNo].mcpState & (1 << defE41module.encPins[encNo%(defE41module.components.nEncoders)][1])){
    pinState |= 1; // Get new state for pin B
    eData[encNo].b = 1; // Get new state for pin B
    //s |= 4;
  }else  eData[encNo].b = 0;

//  // DEBOUNCE - If A or B change, we check the other pin. If it didn't change, then it´s bounce noise.
//  // Debounce algorithm from http://www.technoblogy.com/show?1YHJ
//  if (eData[encNo].a != eData[encNo].a0) {              // A changed
//    eData[encNo].a0 = eData[encNo].a;
//    if (eData[encNo].b != eData[encNo].c0) {
//      eData[encNo].c0 = eData[encNo].b;
//      eData[encNo].encoderChange = true;
//    }else{
//      eData[encNo].millisUpdatePrev = millis();
//      return;
//    }
//  }else if (eData[encNo].b != eData[encNo].b0) {       // B changed
//    eData[encNo].b0 = eData[encNo].b;
//    if (eData[encNo].a != eData[encNo].d0) {
//      eData[encNo].d0 = eData[encNo].a;
//      eData[encNo].encoderChange = true;
//    }else{
//      eData[encNo].millisUpdatePrev = millis();
//      return;
//    }
//  }else{
//    return;
//  }
  
//  // Check state in table
  if(encMData[mcpNo].detent && !eBankData[eData[encNo].thisEncoderBank][encNo].encFineAdjust){
    // IF DETENTED ENCODER AND FINE ADJUST IS NOT SELECTED - SELECT FROM TABLE BASED ON SPEED CONFIGURATION
    if(encoder[encNo].mode.speed == encoderRotarySpeed::rot_slow_speed ||
       encoder[encNo].mode.speed == encoderRotarySpeed::rot_variable_speed)
      eData[encNo].encoderState = pgm_read_byte(&fullStepTable[eData[encNo].encoderState & 0x0f][pinState]);
    else if(encoder[encNo].mode.speed == encoderRotarySpeed::rot_mid_speed)
      eData[encNo].encoderState = pgm_read_byte(&halfStepTable[eData[encNo].encoderState & 0x0f][pinState]);  
    else if(encoder[encNo].mode.speed == encoderRotarySpeed::rot_fast_speed)
      eData[encNo].encoderState = pgm_read_byte(&quarterStepTable[eData[encNo].encoderState & 0x0f][pinState]);  
  }else if(encMData[mcpNo].detent && eBankData[eData[encNo].thisEncoderBank][encNo].encFineAdjust){
    // IF FINE ADJUST AND DETENTED ENCODER - FULL STEP TABLE
    eData[encNo].encoderState = pgm_read_byte(&fullStepTable[eData[encNo].encoderState & 0x0f][pinState]);
  }else{
    // IF NON DETENTED ENCODER - QUARTER STEP TABLE
    eData[encNo].encoderState = pgm_read_byte(&quarterStepTable[eData[encNo].encoderState & 0x0f][pinState]); 
  }

//  SerialUSB.print(eData[encNo].a); SerialUSB.println(eData[encNo].b);
 

  // if at a valid state, check direction
  switch (eData[encNo].encoderState & 0x30) {
    case DIR_CW:{
        eData[encNo].encoderDirection = 1; 
        eData[encNo].encoderChange = true;
    }   break;
    case DIR_CCW:{
        eData[encNo].encoderDirection = -1;
        eData[encNo].encoderChange = true;
    }   break;
    case DIR_NONE:
    default:{
        eData[encNo].encoderDirection = 0; 
        eData[encNo].encoderChange = false; 
        return;
    }   break;
  }

  if(eData[encNo].encoderChange){
    // Reset flag
    eData[encNo].encoderChange = false;      
    // Check if current module is in read priority list
    AddToPriority(mcpNo); 
    
    ///////////////////////////////////////////////  
    //////// ENCODER SPEED  ///////////////////////
    ///////////////////////////////////////////////
    
    // VARIABLE SPEED
    unsigned long timeLastChange = millis() - eData[encNo].millisUpdatePrev;
    //uint8_t currentSpeed = 1;
//    SerialUSB.println(timeLastChange); 
    if (!eBankData[eData[encNo].thisEncoderBank][encNo].encFineAdjust){
      if (encoder[encNo].mode.speed == encoderRotarySpeed::rot_variable_speed){  
        switch(eData[encNo].currentSpeed){
          case SLOW_SPEED:{
            //eData[encNo].currentSpeed = SLOW_SPEED;
            if(timeLastChange < (encMData[mcpNo].detent ? D_MID1_SPEED_MILLIS : MID1_SPEED_MILLIS)){
              eData[encNo].currentSpeed = MID1_SPEED;
            }
          }
          break;
          case MID1_SPEED:{
            if(timeLastChange < (encMData[mcpNo].detent ? D_MID2_SPEED_MILLIS : MID3_SPEED_MILLIS)){
              eData[encNo].currentSpeed = MID2_SPEED;
            }else if(timeLastChange > (encMData[mcpNo].detent ? D_MID1_SPEED_MILLIS : MID1_SPEED_MILLIS)){
              eData[encNo].currentSpeed = SLOW_SPEED;
            }
          }
          break;
          case MID2_SPEED:{
            if(timeLastChange < (encMData[mcpNo].detent ? D_MID3_SPEED_MILLIS : MID3_SPEED_MILLIS)){
              eData[encNo].currentSpeed = MID3_SPEED;
            }else if(timeLastChange > (encMData[mcpNo].detent ? D_MID2_SPEED_MILLIS : MID2_SPEED_MILLIS)){
              eData[encNo].currentSpeed = MID1_SPEED;
            }
          }
          break;
          case MID3_SPEED:{
            if(timeLastChange < (encMData[mcpNo].detent ? D_MID4_SPEED_MILLIS : MID4_SPEED_MILLIS)){
              eData[encNo].currentSpeed = MID4_SPEED;
            }else if(timeLastChange > (encMData[mcpNo].detent ? D_MID3_SPEED_MILLIS : MID3_SPEED_MILLIS)){
              eData[encNo].currentSpeed = MID2_SPEED;
            }
          }
          break;
          case MID4_SPEED:{
            if(timeLastChange < (encMData[mcpNo].detent ? D_FAST_SPEED_MILLIS : FAST_SPEED_MILLIS)){
              eData[encNo].currentSpeed = FAST_SPEED;
            }else if(timeLastChange > (encMData[mcpNo].detent ? D_MID4_SPEED_MILLIS : MID4_SPEED_MILLIS)){
              eData[encNo].currentSpeed = MID3_SPEED;
            }
          }
          break;
          case FAST_SPEED:{
            if(timeLastChange > (encMData[mcpNo].detent ? D_FAST_SPEED_MILLIS : FAST_SPEED_MILLIS)){
              eData[encNo].currentSpeed = MID4_SPEED;
            }
          }
          break;
        }
      // SPEED 1
      }else if  (encoder[encNo].mode.speed == encoderRotarySpeed::rot_slow_speed && 
                (++eBankData[eData[encNo].thisEncoderBank][encNo].pulseCounter >= SLOW_SPEED_COUNT)){     
        eData[encNo].currentSpeed = SLOW_SPEED;
        eBankData[eData[encNo].thisEncoderBank][encNo].pulseCounter = 0;
      // SPEED 2
      }else if  (encoder[encNo].mode.speed == encoderRotarySpeed::rot_mid_speed && 
                (++eBankData[eData[encNo].thisEncoderBank][encNo].pulseCounter >= MID_SPEED_COUNT)){     
        eData[encNo].currentSpeed = 1;
        eBankData[eData[encNo].thisEncoderBank][encNo].pulseCounter = 0;
      // SPEED 3
      }else if (encoder[encNo].mode.speed == encoderRotarySpeed::rot_fast_speed){                                                  
        eData[encNo].currentSpeed = 1;
        eBankData[eData[encNo].thisEncoderBank][encNo].pulseCounter = 0;
      }else{
        eData[encNo].currentSpeed = 0;
      }
    }else{ // FINE ADJUST IS HALF SLOW SPEED
      if  (++eBankData[eData[encNo].thisEncoderBank][encNo].pulseCounter >= (encoder[encNo].mode.speed == encoderRotarySpeed::rot_slow_speed ? 
                                                                                                          2*SLOW_SPEED_COUNT :
                                                                                                          SLOW_SPEED_COUNT)){     
        eData[encNo].currentSpeed = 1;
        eBankData[eData[encNo].thisEncoderBank][encNo].pulseCounter = 0;
      }else{
        eData[encNo].currentSpeed = 0;
      }
    }
    eData[encNo].millisUpdatePrev = millis();
    
//    SerialUSB.println("CHANGE!");
    uint16_t paramToSend = 0;
    uint8_t channelToSend = 0;
    uint16_t minValue = 0, maxValue = 0; 
    uint8_t msgType = 0;
    
    uint16_t paramToSend2 = 0;
    uint8_t channelToSend2 = 0;
    uint16_t minValue2 = 0, maxValue2 = 0; 
    
    bool is14bits = false;
    
    // Get config info for this encoder
    if(eBankData[eData[encNo].thisEncoderBank][encNo].shiftRotaryAction){
      // IF SHIFT ROTARY ACTION FLAG IS ACTIVATED GET CONFIG FROM SWITCH CONFIG (SHIFT CONFIG)
      paramToSend = encoder[encNo].switchConfig.parameter[switch_parameter_MSB]<<7 | encoder[encNo].switchConfig.parameter[switch_parameter_LSB];
      channelToSend = encoder[encNo].switchConfig.channel + 1;
      minValue = encoder[encNo].switchConfig.parameter[switch_minValue_MSB]<<7 | encoder[encNo].switchConfig.parameter[switch_minValue_LSB];
      maxValue = encoder[encNo].switchConfig.parameter[switch_maxValue_MSB]<<7 | encoder[encNo].switchConfig.parameter[switch_maxValue_LSB];
      msgType = encoder[encNo].switchConfig.message;
    }else{
      paramToSend = encoder[encNo].rotaryConfig.parameter[rotary_MSB]<<7 | encoder[encNo].rotaryConfig.parameter[rotary_LSB];
      channelToSend = encoder[encNo].rotaryConfig.channel + 1;
      minValue = encoder[encNo].rotaryConfig.parameter[rotary_minMSB]<<7 | encoder[encNo].rotaryConfig.parameter[rotary_minLSB];
      maxValue = encoder[encNo].rotaryConfig.parameter[rotary_maxMSB]<<7 | encoder[encNo].rotaryConfig.parameter[rotary_maxLSB];
      msgType = encoder[encNo].rotaryConfig.message;
    }
    if(eBankData[eData[encNo].thisEncoderBank][encNo].doubleCC){
      paramToSend2 = encoder[encNo].switchConfig.parameter[switch_parameter_LSB];
      channelToSend2 = encoder[encNo].switchConfig.channel + 1;
      minValue2 = encoder[encNo].switchConfig.parameter[switch_minValue_LSB];
      maxValue2 = encoder[encNo].switchConfig.parameter[switch_maxValue_LSB];
    }

    if( msgType == rotary_msg_nrpn || msgType == rotary_msg_rpn || msgType == rotary_msg_pb || 
        msgType == switch_msg_nrpn || msgType == switch_msg_rpn || msgType == switch_msg_pb){
      is14bits = true;      
    }else{
      minValue = minValue & 0x7F;
      maxValue = maxValue & 0x7F;
    }
    
    //bool invert = false;
    if(eData[encNo].encoderDirection != eData[encNo].encoderDirectionPrev) eData[encNo].currentSpeed = 1;    // If direction changed, set speed to minimum

    int8_t normalDirection = eData[encNo].encoderDirection;
    int8_t doubleCCdirection = eData[encNo].encoderDirection;
    if(minValue > maxValue){    // If minValue is higher, invert behaviour
      normalDirection *= -1;   // Invert position
      //invert = true;                        // 
      uint16_t aux = minValue;              // Swap min and max values, to treat them equally henceforth
      minValue = maxValue;
      maxValue = aux;
    }
    
    uint8_t speedMultiplier = 1;

    // INCREASE SPEED FOR 14 BIT VALUES
    if(is14bits && eData[encNo].currentSpeed > 1){
      int16_t maxMinDiff = maxValue - minValue;
      speedMultiplier = abs(maxMinDiff) >> (13-eData[encNo].currentSpeed);
      // only valid if multiplier is > 0
      if(!speedMultiplier) speedMultiplier = 1;
      
    }

    // GET NEW ENCODER VALUE based on speed, direction and speed multiplier (14 bits config)
    if(eBankData[eData[encNo].thisEncoderBank][encNo].shiftRotaryAction){
      eBankData[eData[encNo].thisEncoderBank][encNo].encoderShiftValue += eData[encNo].currentSpeed*speedMultiplier*normalDirection;           // New value
      if(eBankData[eData[encNo].thisEncoderBank][encNo].encoderShiftValue > maxValue) eBankData[eData[encNo].thisEncoderBank][encNo].encoderShiftValue = maxValue;
      // if below min, stay in min
      if(eBankData[eData[encNo].thisEncoderBank][encNo].encoderShiftValue < minValue) eBankData[eData[encNo].thisEncoderBank][encNo].encoderShiftValue = minValue;
      // SerialUSB.print("SR - ");SerialUSB.print(encNo);SerialUSB.print(": ");SerialUSB.println(speedMultiplier);
    }else{
      eBankData[eData[encNo].thisEncoderBank][encNo].encoderValue += eData[encNo].currentSpeed*speedMultiplier*normalDirection;           // New value
      // If overflows max, stay in max
      if(eBankData[eData[encNo].thisEncoderBank][encNo].encoderValue > maxValue) eBankData[eData[encNo].thisEncoderBank][encNo].encoderValue = maxValue;
      // if below min, stay in min
      if(eBankData[eData[encNo].thisEncoderBank][encNo].encoderValue < minValue) eBankData[eData[encNo].thisEncoderBank][encNo].encoderValue = minValue;
      // SerialUSB.print("SR - ");SerialUSB.print(encNo);SerialUSB.print(": ");SerialUSB.println(speedMultiplier);
      // SerialUSB.print(encNo);SerialUSB.print(": ");SerialUSB.println(speedMultiplier);
    } 
    

    // If double CC is ON, process value for it
    if(eBankData[currentBank][encNo].doubleCC){
      if(minValue2 > maxValue2){    // If minValue is higher, invert behaviour
        doubleCCdirection = eData[encNo].encoderDirection * (-1);   // Invert position
        //invert = true;                        // 
        uint16_t aux = minValue2;              // Swap min and max values, to treat them equally henceforth
        minValue2 = maxValue2;
        maxValue2 = aux;
      }
      eBankData[eData[encNo].thisEncoderBank][encNo].encoderValue2cc += eData[encNo].currentSpeed*doubleCCdirection;    // New value
      // If overflows max, stay in max
      if(eBankData[eData[encNo].thisEncoderBank][encNo].encoderValue2cc >= maxValue2) eBankData[eData[encNo].thisEncoderBank][encNo].encoderValue2cc = maxValue2;
      // if below min, stay in min
      if(eBankData[eData[encNo].thisEncoderBank][encNo].encoderValue2cc < minValue2) eBankData[eData[encNo].thisEncoderBank][encNo].encoderValue2cc = minValue2;
//      SerialUSB.print("2cc value: "); SerialUSB.print(eBankData[eData[encNo].thisEncoderBank][encNo].encoderValue2cc);
//      SerialUSB.print("\t minValue2: "); SerialUSB.print(minValue2);
//      SerialUSB.print("\t maxValue2: "); SerialUSB.println(maxValue2);
    }

    // update previous direction
    eData[encNo].encoderDirectionPrev = eData[encNo].encoderDirection;   

    bool updateFb = false;
    // If value changed 
    //if(eBankData[eData[encNo].thisEncoderBank][encNo].encoderValue != eBankData[eData[encNo].thisEncoderBank][encNo].encoderValuePrev){     
    uint16_t valueToSend = 0;
    if(eBankData[eData[encNo].thisEncoderBank][encNo].shiftRotaryAction){
      valueToSend = eBankData[eData[encNo].thisEncoderBank][encNo].encoderShiftValue;
    }else{
      valueToSend = eBankData[eData[encNo].thisEncoderBank][encNo].encoderValue;
    }

    if(valueToSend != eData[encNo].encoderValuePrev){     
      
      eData[encNo].encoderValuePrev = valueToSend;
      
      if(!is14bits){
        paramToSend &= 0x7F;
        valueToSend &= 0x7F;
      }
      
      switch(msgType){
        case rotaryMessageTypes::rotary_msg_note:{
          if(encoder[encNo].rotaryConfig.midiPort & 0x01)
            MIDI.sendNoteOn( paramToSend, valueToSend, channelToSend);
          if(encoder[encNo].rotaryConfig.midiPort & 0x02)
            MIDIHW.sendNoteOn( paramToSend, valueToSend, channelToSend);
        }break;
        case rotaryMessageTypes::rotary_msg_cc:{
          if(encoder[encNo].rotaryConfig.midiPort & 0x01)
            MIDI.sendControlChange( paramToSend, valueToSend, channelToSend);
          if(encoder[encNo].rotaryConfig.midiPort & 0x02)
            MIDIHW.sendControlChange( paramToSend, valueToSend, channelToSend);
        }break;
        case rotaryMessageTypes::rotary_msg_pc_rel:{
          if(encoder[encNo].rotaryConfig.midiPort & 0x01)
            MIDI.sendProgramChange( valueToSend, channelToSend);
          if(encoder[encNo].rotaryConfig.midiPort & 0x02)
            MIDIHW.sendProgramChange( valueToSend, channelToSend);
        }break;
        case rotaryMessageTypes::rotary_msg_nrpn:{
          if(encoder[encNo].rotaryConfig.midiPort & 0x01){
            MIDI.sendControlChange( 99, (paramToSend >> 7) & 0x7F, channelToSend);
            MIDI.sendControlChange( 98, (paramToSend & 0x7F), channelToSend);
            MIDI.sendControlChange( 6, (valueToSend >> 7) & 0x7F, channelToSend);
            MIDI.sendControlChange( 38, (valueToSend & 0x7F), channelToSend);       
          }
          if(encoder[encNo].rotaryConfig.midiPort & 0x02){
            MIDIHW.sendControlChange( 99, (paramToSend >> 7) & 0x7F, channelToSend);
            MIDIHW.sendControlChange( 98, (paramToSend & 0x7F), channelToSend);
            MIDIHW.sendControlChange( 6, (valueToSend >> 7) & 0x7F, channelToSend);
            MIDIHW.sendControlChange( 38, (valueToSend & 0x7F), channelToSend);    
          }
        }break;
        case rotaryMessageTypes::rotary_msg_rpn:{
          if(encoder[encNo].rotaryConfig.midiPort & 0x01){
            MIDI.sendControlChange( 101, (paramToSend >> 7) & 0x7F, channelToSend);
            MIDI.sendControlChange( 100, (paramToSend & 0x7F), channelToSend);
            MIDI.sendControlChange( 6, (valueToSend >> 7) & 0x7F, channelToSend);
            MIDI.sendControlChange( 38, (valueToSend & 0x7F), channelToSend);       
          }
          if(encoder[encNo].rotaryConfig.midiPort & 0x02){
            MIDIHW.sendControlChange( 101, (paramToSend >> 7) & 0x7F, channelToSend);
            MIDIHW.sendControlChange( 100, (paramToSend & 0x7F), channelToSend);
            MIDIHW.sendControlChange( 6, (valueToSend >> 7) & 0x7F, channelToSend);
            MIDIHW.sendControlChange( 38, (valueToSend & 0x7F), channelToSend);    
          }
        }break;
        case rotaryMessageTypes::rotary_msg_pb:{
          int16_t valuePb = mapl(valueToSend, minValue, maxValue,((int16_t) minValue)-8192, ((int16_t) maxValue)-8192);
//          SerialUSB.println(valuePb);
          if(encoder[encNo].rotaryConfig.midiPort & 0x01)
            MIDI.sendPitchBend( valuePb, channelToSend);    
          if(encoder[encNo].rotaryConfig.midiPort & 0x02)
            MIDIHW.sendPitchBend( valuePb, channelToSend);    
        }break;
        case rotaryMessageTypes::rotary_msg_key:{
          if(eData[encNo].encoderDirection < 0){       // turned left
            if(encoder[encNo].rotaryConfig.parameter[rotary_modifierLeft])
              Keyboard.press(encoder[encNo].rotaryConfig.parameter[rotary_modifierLeft]);
            if(encoder[encNo].rotaryConfig.parameter[rotary_keyLeft])  
              Keyboard.press(encoder[encNo].rotaryConfig.parameter[rotary_keyLeft]);
          }else if(eData[encNo].encoderDirection > 0){ //turned right
            if(encoder[encNo].rotaryConfig.parameter[rotary_modifierRight])
              Keyboard.press(encoder[encNo].rotaryConfig.parameter[rotary_modifierRight]);
            if(encoder[encNo].rotaryConfig.parameter[rotary_keyRight])
              Keyboard.press(encoder[encNo].rotaryConfig.parameter[rotary_keyRight]);
          }
          millisKeyboardPress = millis();
          keyboardReleaseFlag = true; 
        }break;
      }
      
      SetStatusLED(STATUS_BLINK, 1, statusLEDtypes::STATUS_FB_MIDI_OUT);

      if(componentInfoEnabled && (GetHardwareID(ytxIOBLOCK::Encoder, encNo) != lastComponentInfoId)){
        SendComponentInfo(ytxIOBLOCK::Encoder, encNo);
      }
      
      //if(encoder[encNo].rotaryFeedback.source == fb_src_local){
      // aprox 90 us / 4 rings de 16 leds   // 120 us / 8 enc // 200 us / 16 enc
      updateFb = true;
      //}
    }

    if( eBankData[eData[encNo].thisEncoderBank][encNo].doubleCC && 
       (eBankData[eData[encNo].thisEncoderBank][encNo].encoderValue2cc != eData[encNo].encoderValuePrev2cc)){
      eData[encNo].encoderValuePrev2cc = eBankData[eData[encNo].thisEncoderBank][encNo].encoderValue2cc;
      uint16_t valueToSend2cc = eBankData[eData[encNo].thisEncoderBank][encNo].encoderValue2cc;
      
      if(encoder[encNo].rotaryConfig.midiPort & 0x01)
        MIDI.sendControlChange( paramToSend2, valueToSend2cc, channelToSend2);
      if(encoder[encNo].rotaryConfig.midiPort & 0x02)
        MIDIHW.sendControlChange( paramToSend2, valueToSend2cc, channelToSend2);

      SetStatusLED(STATUS_BLINK, 1, statusLEDtypes::STATUS_FB_MIDI_OUT);
      
      updateFb = true;      
    }
    if(updateFb){
      feedbackHw.SetChangeEncoderFeedback(FB_ENCODER, encNo, valueToSend, encMData[mcpNo].moduleOrientation, false);           
      if(encoder[encNo].switchConfig.mode == switchModes::switch_mode_2cc){
        feedbackHw.SetChangeEncoderFeedback(FB_2CC, encNo, eBankData[eData[encNo].thisEncoderBank][encNo].encoderValue2cc, encMData[mcpNo].moduleOrientation, false); 
      }
    }
  }
  return;
}

void EncoderInputs::SetEncoderValue(uint8_t bank, uint8_t encNo, uint16_t value){
  uint16_t minValue = 0, maxValue = 0;
  uint8_t msgType = 0;
  bool is14bits = false;
  
  // Get config info for this encoder
  // HOW TO KNOW IF INCOMING MIDI MESSAGE IS FOR DOUBLE CC OR SHIFTED ACTION???
  if(eBankData[bank][encNo].shiftRotaryAction || eBankData[bank][encNo].doubleCC){  
    minValue = encoder[encNo].switchConfig.parameter[switch_minValue_MSB]<<7 | encoder[encNo].switchConfig.parameter[switch_minValue_LSB];
    maxValue = encoder[encNo].switchConfig.parameter[switch_maxValue_MSB]<<7 | encoder[encNo].switchConfig.parameter[switch_maxValue_LSB];
    msgType = encoder[encNo].switchConfig.message;
  }else{
    minValue = encoder[encNo].rotaryConfig.parameter[rotary_minMSB]<<7 | encoder[encNo].rotaryConfig.parameter[rotary_minLSB];
    maxValue = encoder[encNo].rotaryConfig.parameter[rotary_maxMSB]<<7 | encoder[encNo].rotaryConfig.parameter[rotary_maxLSB];
    msgType = encoder[encNo].rotaryConfig.message;
  }

  // IF NOT 14 BITS, USE LOWER PART FOR MIN AND MAX
  if( msgType == rotary_msg_nrpn || msgType == rotary_msg_rpn || msgType == rotary_msg_pb || 
      msgType == switch_msg_nrpn || msgType == switch_msg_rpn || msgType == switch_msg_pb){
    is14bits = true;      
  }else{
    minValue = minValue & 0x7F;
    maxValue = maxValue & 0x7F;
  }

  bool invert = false;
  if(minValue > maxValue){    // If minValue is higher, invert behaviour
    invert = true;
  }
  // 
  if(value > (invert ? minValue : maxValue)){
    eBankData[bank][encNo].encoderValue = (invert ? minValue : maxValue);
  }
  else if(value < (invert ? maxValue : minValue)){
    eBankData[bank][encNo].encoderValue = (invert ? maxValue : minValue);
  }
  else{
    eBankData[bank][encNo].encoderValue = value;
  }
  // update prev value
  eData[encNo].encoderValuePrev = value;
    
  if (bank == currentBank && !eBankData[bank][encNo].shiftRotaryAction){
    feedbackHw.SetChangeEncoderFeedback(FB_ENCODER, encNo, eBankData[bank][encNo].encoderValue, encMData[encNo/4].moduleOrientation, false);
    if(encoder[encNo].switchConfig.mode == switchModes::switch_mode_2cc){
      feedbackHw.SetChangeEncoderFeedback(FB_2CC, encNo, eBankData[bank][encNo].encoderValue2cc, encMData[encNo/4].moduleOrientation, false);
    }
    
  }
}

void EncoderInputs::SetEncoderShiftValue(uint8_t bank, uint8_t encNo, uint16_t value){
  uint16_t minValue = 0, maxValue = 0;
  uint8_t msgType = 0;
  bool is14bits = false;
  
  // Get config info for this encoder
  // HOW TO KNOW IF INCOMING MIDI MESSAGE IS FOR DOUBLE CC OR SHIFTED ACTION???
  
  minValue = encoder[encNo].switchConfig.parameter[switch_minValue_MSB]<<7 | encoder[encNo].switchConfig.parameter[switch_minValue_LSB];
  maxValue = encoder[encNo].switchConfig.parameter[switch_maxValue_MSB]<<7 | encoder[encNo].switchConfig.parameter[switch_maxValue_LSB];
  msgType = encoder[encNo].switchConfig.message;


  // IF NOT 14 BITS, USE LOWER PART FOR MIN AND MAX
  if( msgType == rotary_msg_nrpn || msgType == rotary_msg_rpn || msgType == rotary_msg_pb || 
      msgType == switch_msg_nrpn || msgType == switch_msg_rpn || msgType == switch_msg_pb){
    is14bits = true;      
  }else{
    minValue = minValue & 0x7F;
    maxValue = maxValue & 0x7F;
  }

  bool invert = false;
  if(minValue > maxValue){    // If minValue is higher, invert behaviour
    invert = true;
  }
  // 
  if(value > (invert ? minValue : maxValue)){
    eBankData[bank][encNo].encoderShiftValue = (invert ? minValue : maxValue);
  }
  else if(value < (invert ? maxValue : minValue)){
    eBankData[bank][encNo].encoderShiftValue = (invert ? maxValue : minValue);
  }
  else{
    eBankData[bank][encNo].encoderShiftValue = value;
  }
  // update prev value
  eData[encNo].encoderValuePrev = value;
    
  if (bank == currentBank && eBankData[bank][encNo].shiftRotaryAction){
    feedbackHw.SetChangeEncoderFeedback(FB_ENCODER, encNo, eBankData[bank][encNo].encoderShiftValue, encMData[encNo/4].moduleOrientation, false);
    
  }
}

void EncoderInputs::SetEncoder2cc(uint8_t bank, uint8_t encNo, uint16_t value){
  uint16_t minValue = 0, maxValue = 0;
  uint8_t msgType = 0;
  bool is14bits = false;
  
  minValue = encoder[encNo].switchConfig.parameter[switch_minValue_LSB];
  maxValue = encoder[encNo].switchConfig.parameter[switch_maxValue_LSB];

  bool invert = false;
  if(minValue > maxValue){    // If minValue is higher, invert behaviour
    invert = true;
  }
  // 
  if(value > (invert ? minValue : maxValue)){
    eBankData[bank][encNo].encoderValue2cc = (invert ? minValue : maxValue);
  }
  else if(value < (invert ? maxValue : minValue)){
    eBankData[bank][encNo].encoderValue2cc = (invert ? maxValue : minValue);
  }
  else{
    eBankData[bank][encNo].encoderValue2cc = value;
  }
  // update prev value
  eData[encNo].encoderValuePrev2cc = value;
    
  if (bank == currentBank){
    feedbackHw.SetChangeEncoderFeedback(FB_ENCODER, encNo, eBankData[bank][encNo].encoderValue, encMData[encNo/4].moduleOrientation, false);
    feedbackHw.SetChangeEncoderFeedback(FB_2CC, encNo, eBankData[bank][encNo].encoderValue2cc, encMData[encNo/4].moduleOrientation, false);
  }
}

void EncoderInputs::SetEncoderSwitchValue(uint8_t bank, uint8_t encNo, uint16_t newValue){
//  uint16_t minValue = 0, maxValue = 0;
//    
//  minValue = encoder[encNo].switchConfig.parameter[switch_minValue_MSB]<<7 | encoder[encNo].switchConfig.parameter[switch_minValue_LSB];
//  maxValue = encoder[encNo].switchConfig.parameter[switch_maxValue_MSB]<<7 | encoder[encNo].switchConfig.parameter[switch_maxValue_LSB];

  eBankData[bank][encNo].switchLastValue = newValue & 0x3FFF;   // lastValue is 14 bit

  // if input is toggle, update prev state so next time a user presses, it will toggle correctly.
  if(encoder[encNo].switchConfig.action == switchActions::switch_toggle){
    eBankData[bank][encNo].switchInputStatePrev = eBankData[bank][encNo].switchInputState;
  }
  
//   SerialUSB.print("Set encoder switch "); SerialUSB.print(encNo); SerialUSB.print(" value: ");SerialUSB.println(eBankData[bank][encNo].switchLastValue);
  if (bank == currentBank){
    feedbackHw.SetChangeEncoderFeedback(FB_ENCODER_SWITCH, 
                                        encNo, 
                                        eBankData[bank][encNo].switchLastValue, 
                                        encMData[encNo/4].moduleOrientation, 
                                        false);
  }

}

void EncoderInputs::SetBankForEncoders(uint8_t newBank){
  for(int encNo = 0; encNo < nEncoders; encNo++){
     eData[encNo].thisEncoderBank = newBank;
     eData[encNo].bankShifted = false;
     eData[encNo].encoderValuePrev = eBankData[newBank][encNo].encoderValue;
  }
}

uint16_t EncoderInputs::GetEncoderValue(uint8_t encNo){
  if(encNo < nEncoders){
    if(eBankData[currentBank][encNo].shiftRotaryAction){
      return eBankData[currentBank][encNo].encoderShiftValue;
    }else{
      return eBankData[currentBank][encNo].encoderValue;
    }
    
  }   
}

uint16_t EncoderInputs::GetEncoderValue2(uint8_t encNo){
  if(encNo < nEncoders){
    return eBankData[currentBank][encNo].encoderValue2cc;
  }   
}

bool EncoderInputs::GetEncoderSwitchValue(uint8_t encNo){
  uint16_t retValue = 0;
  if(encNo < nEncoders){
    if(encoder[encNo].switchFeedback.source == fb_src_local || encoder[encNo].switchFeedback.localBehaviour == fb_lb_always_on){
      retValue =  encoder[encNo].switchConfig.parameter[switch_maxValue_MSB] << 7 |
                  encoder[encNo].switchConfig.parameter[switch_maxValue_LSB];
    }else{
      retValue = eBankData[currentBank][encNo].switchLastValue;
    }   
//    SerialUSB.print("Get encoder switch "); SerialUSB.print(encNo); SerialUSB.print(" value: ");SerialUSB.println(eBankData[currentBank][encNo].switchLastValue);
    return retValue;
  }       
}

bool EncoderInputs::GetEncoderSwitchState(uint8_t encNo){
  uint16_t retValue = 0;
  if(encNo < nEncoders){
    if(encoder[encNo].switchFeedback.source == fb_src_local || encoder[encNo].switchFeedback.localBehaviour == fb_lb_always_on){
      retValue = 1;
    }else{
      retValue = eBankData[currentBank][encNo].switchInputState;
    }   
//    SerialUSB.print("Get encoder switch "); SerialUSB.print(encNo); SerialUSB.print(" state: ");SerialUSB.println(eBankData[currentBank][encNo].switchInputState);
    return retValue;
  }       
}

bool EncoderInputs::IsShiftActionOn(uint8_t encNo){
  if(encNo < nEncoders)
    return  eBankData[currentBank][encNo].shiftRotaryAction;
}

bool EncoderInputs::IsFineAdj(uint8_t encNo){
  if(encNo < nEncoders)
    return  eBankData[currentBank][encNo].shiftRotaryAction;
}

bool EncoderInputs::IsBankShifted(uint8_t encNo){
  if(encNo < nEncoders)
    return  eData[encNo].bankShifted;
}

bool EncoderInputs::IsDoubleCC(uint8_t encNo){
  if(encNo < nEncoders)
    return  eBankData[currentBank][encNo].doubleCC;
}


uint8_t EncoderInputs::GetModuleOrientation(uint8_t mcpNo){
  if(mcpNo < nModules)
    return encMData[mcpNo].moduleOrientation;
}


void EncoderInputs::AddToPriority(uint8_t nMCP){
  if (!priorityCount){
    priorityCount++;
    priorityList[0] = nMCP;
    priorityTime = millis();
  }
  else if(priorityCount == 1 && nMCP != priorityList[0]){
    priorityCount++;
    priorityList[1] = nMCP;
    priorityTime = millis();
  }
}

void EncoderInputs::SetNextAddress(SPIExpander *mcpX, uint8_t addr){
  for (int i = 0; i<3; i++){
    mcpX->pinMode(defE41module.nextAddressPin[i],OUTPUT);   
    mcpX->digitalWrite(defE41module.nextAddressPin[i],(addr>>i)&1);
  }
  return;
}

void EncoderInputs::SetPullUps(){
  byte cmd = OPCODEW;
  SPI.beginTransaction(SPISettings(1000000,MSBFIRST,SPI_MODE0));
    digitalWrite(encodersMCPChipSelect, LOW);
    SPI.transfer(cmd);
    SPI.transfer(GPPUA);
    SPI.transfer(0xFF);
    digitalWrite(encodersMCPChipSelect, HIGH);
  SPI.endTransaction();
  delayMicroseconds(5);
  SPI.beginTransaction(SPISettings(1000000,MSBFIRST,SPI_MODE0));
    digitalWrite(encodersMCPChipSelect, LOW);
    SPI.transfer(cmd);
    SPI.transfer(GPPUB);
    SPI.transfer(0xFF);
    digitalWrite(encodersMCPChipSelect, HIGH);
  SPI.endTransaction();
}
void EncoderInputs::EnableHWAddress(){
  // ENABLE HARDWARE ADDRESSING MODE FOR ALL CHIPS
  digitalWrite(encodersMCPChipSelect, HIGH);
  byte cmd = OPCODEW;
  digitalWrite(encodersMCPChipSelect, LOW);
  SPI.transfer(cmd);
  SPI.transfer(IOCONA);
  SPI.transfer(ADDR_ENABLE);
  digitalWrite(encodersMCPChipSelect, HIGH);
}

void EncoderInputs::DisableHWAddress(){
  byte cmd = 0;
  // DISABLE HARDWARE ADDRESSING FOR ALL CHIPS - ONLY NEEDED FOR RESET
  for (int n = 0; n < nModules; n++) {
    byte digPort = n / 8;
    byte mcpAddress = n % 8;
    
    digitalWrite(encodersMCPChipSelect, HIGH);
    cmd = OPCODEW | ((mcpAddress & 0b111) << 1);
    digitalWrite(encodersMCPChipSelect, LOW);
    SPI.transfer(cmd);
    SPI.transfer(IOCONA);
    SPI.transfer(ADDR_DISABLE);
    digitalWrite(encodersMCPChipSelect, HIGH);
  }
}
