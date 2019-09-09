#include "headers/EncoderInputs.h"

//----------------------------------------------------------------------------------------------------
// ENCODER FUNCTIONS
//----------------------------------------------------------------------------------------------------
void EncoderInputs::Init(uint8_t maxBanks, uint8_t maxEncoders, SPIClass *spiPort){
  
  if(!maxBanks || !maxEncoders) return;    // If number of encoders is zero, return;

  byte encodersInConfig = 0;
  
  // CHECK WHETHER AMOUNT OF DIGITAL INPUTS IN MODULES COMBINED MATCH THE AMOUNT OF DIGITAL INPUTS IN CONFIG
  // AMOUNT OF DIGITAL MODULES
  for (int nMod = 0; nMod < MAX_ENCODER_MODS; nMod++) {
    //        SerialUSB.println(config->hwMapping.digital[nPort][nMod]);
    if (config->hwMapping.encoder[nMod]) {
      modulesInConfig.encoders++;
    }
    switch (config->hwMapping.encoder[nMod]) {
      case EncoderModuleTypes::E41H:
      case EncoderModuleTypes::E41V: {
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
  
  // First dimension is an array of pointers, each pointing to a column - https://www.eskimo.com/~scs/cclass/int/sx9b.html
  eBankData = (encoderBankData**) memHost->AllocateRAM(nBanks*sizeof(encoderBankData*));
  eData = (encoderData*) memHost->AllocateRAM(nEncoders*sizeof(encoderData));
  encMData = (moduleData*) memHost->AllocateRAM(nModules*sizeof(moduleData));

  for (int b = 0; b < nBanks; b++){
    eBankData[b] = (encoderBankData*) memHost->AllocateRAM(nEncoders*sizeof(encoderBankData));
    
    for(int e = 0; e < nEncoders; e++){
       eBankData[b][e].encoderValue = 0;
       eBankData[b][e].encoderValuePrev = 0;
       eBankData[b][e].encoderState = 0;
       eBankData[b][e].pulseCounter = 0;
       eBankData[b][e].switchInputState = false;
    }
  }

  for(int e = 0; e < nEncoders; e++){
    eData[e].encoderPosition = 0;
    eData[e].encoderChange = false;
    eData[e].millisUpdatePrev = 0;
    eData[e].switchHWState = 0;
    eData[e].switchHWStatePrev = 0;
    eData[e].swBounceMillisPrev = 0;
  }

  pinMode(encodersMCPChipSelect, OUTPUT);

//  SerialUSB.println("ENCODERS");
  for (int n = 0; n < nModules; n++){
    if(config->hwMapping.encoder[n] != EncoderModuleTypes::ENCODER_NONE){
      encMData[n].moduleOrientation = config->hwMapping.encoder[n]-1;    // 0 -> HORIZONTAL , 1 -> VERTICAL
//      SerialUSB.print("encMData[0].moduleOrientation: "); SerialUSB.println(encMData[n].moduleOrientation);
    }else{
//      encMData[n].moduleOrientation = e41orientation::HORIZONTAL;
//      SerialUSB.print("encMData[0].moduleOrientation - b: "); SerialUSB.println(encMData[n].moduleOrientation);
      feedbackHw.SetStatusLED(STATUS_BLINK, 1, STATUS_FB_ERROR);
      return;
    }
    
    encodersMCP[n].begin(spiPort, encodersMCPChipSelect, n);  
//    printPointer(&encodersMCP[n]);
    
    encMData[n].mcpState = 0;
    encMData[n].mcpStatePrev = 0;
    
    // AFTER INITIALIZATION SET NEXT ADDRESS ON EACH MODULE (EXCEPT 7 and 15, cause they don't have a module next on the chain)
    byte mcpAddress = n % 8;
    if (nModules > 1) {
      SetNextAddress(&encodersMCP[n], mcpAddress + 1);
    }
    
    for(int i=0; i<16; i++){
      if(i != defE41module.nextAddressPin[0] && i != defE41module.nextAddressPin[1] && 
         i != defE41module.nextAddressPin[2] && i != (defE41module.nextAddressPin[2]+1)){       // HARDCODE: Only E41 module exists for now
       encodersMCP[n].pullUp(i, HIGH);
      }
    }

//    for(int i = 0; i<16; i++){
//      SerialUSB.print(encodersMCP[n].digitalRead(i), BIN);  
//      if(i == 9 || i == 6) SerialUSB.print(" ");
//    }
//    SerialUSB.println();
  }
  
  
  
  return;
}


void EncoderInputs::Read(){
  if(!nBanks || !nEncoders || !nModules) return;    // If number of encoders is zero, return;
  
  if(priorityCount && (millis()-priorityTime > 500)){
    priorityCount = 0;
//    SerialUSB.print("Priority list emptied");
  }
  if(priorityCount >= 1){
    byte priorityModule = priorityList[0];
    encMData[priorityModule].mcpState = encodersMCP[priorityModule].digitalRead();
//    SerialUSB.print("Priority Read Module: ");SerialUSB.println(priorityModule);
  }
  if(priorityCount == 2){
    byte priorityModule = priorityList[1];
    encMData[priorityModule].mcpState = encodersMCP[priorityModule].digitalRead();
//    SerialUSB.print("Priority Read Module: ");SerialUSB.println(priorityModule);
  }else{  
    for (int n = 0; n < nModules; n++){       
      encMData[n].mcpState = encodersMCP[n].digitalRead();
    }
 } 
  
//  SerialUSB.print(encMData[0].mcpState,BIN); SerialUSB.println(" ");
//  SerialUSB.print(encMData[1].mcpState,BIN); SerialUSB.print(" ");
//  SerialUSB.print(encMData[2].mcpState,BIN); SerialUSB.print(" ");
//  SerialUSB.print(encMData[3].mcpState,BIN); SerialUSB.print(" ");
//  SerialUSB.println(encMData[4].mcpState,BIN);
  byte encNo = 0; 
  byte nEncodInMod = 0;
  for(byte mcpNo = 0; mcpNo < nModules; mcpNo++){
    switch(config->hwMapping.encoder[mcpNo]){
      case EncoderModuleTypes::E41H:
      case EncoderModuleTypes::E41V: {
          nEncodInMod = defE41module.components.nEncoders;              // HARDCODE: CHANGE WHEN DIFFERENT MODULES ARE ADDED
        } break;
      default: break;
    }
    
    if( encMData[mcpNo].mcpState != encMData[mcpNo].mcpStatePrev){
      encMData[mcpNo].mcpStatePrev = encMData[mcpNo].mcpState; 
      // READ N° OF ENCODERS IN ONE MCP
      for(int n = 0; n < nEncodInMod; n++){
        EncoderCheck(mcpNo,encNo+n);
        SwitchCheck(mcpNo,encNo+n);
      }
    }
    encNo = encNo + nEncodInMod;    // Next module
  }
  return;
}

void EncoderInputs::SwitchCheck(byte mcpNo, byte encNo){  
  if(encoder[encNo].switchConfig.message == digitalMessageTypes::digital_none) return;
  
  eData[encNo].switchHWState = !(encMData[mcpNo].mcpState & (1 << defE41module.encSwitchPins[encNo%(defE41module.components.nEncoders)]));
  
  if(eData[encNo].switchHWState != eData[encNo].switchHWStatePrev){
    eData[encNo].switchHWStatePrev = eData[encNo].switchHWState;
    
    AddToPriority(mcpNo);
    
    // STATUS LED SET BLINK
    feedbackHw.SetStatusLED(STATUS_BLINK, 1, statusLEDtypes::STATUS_FB_INPUT_CHANGED);
    
    if (eData[encNo].switchHWState){
//      if(encNo < nBanks && currentBank != encNo ){ // ADD BANK CONDITION
//       // currentBank = memHost->LoadBank(encNo);
//        //SerialUSB.print("Loaded Bank: "); SerialUSB.println(currentBank);
//      }
      eBankData[currentBank][encNo].switchInputState = !eBankData[currentBank][encNo].switchInputState;

//      feedbackHw.Update(FB_ENCODER_SWITCH, encNo, eBankData[currentBank][n].switchInputState, encMData[mcpNo].moduleOrientation);           // aprox 90 us / 4 rings de 16 leds   // 120 us / 8 enc // 200 us / 16 enc       
      feedbackHw.SetChangeEncoderFeedback(FB_ENCODER_SWITCH, encNo, 
                                          eBankData[currentBank][encNo].switchInputState, 
                                          encMData[mcpNo].moduleOrientation);   
    }else if(!eData[encNo].switchHWState && encoder[encNo].switchConfig.action != switchActions::switch_toggle){
      eBankData[currentBank][encNo].switchInputState = 0;
      feedbackHw.SetChangeEncoderFeedback(FB_ENCODER_SWITCH, encNo, eBankData[currentBank][encNo].switchInputState, encMData[mcpNo].moduleOrientation);         
    }
  }
  
}

void EncoderInputs::EncoderCheck(byte mcpNo, byte encNo){
  if(encoder[encNo].rotaryConfig.message == encoderMessageTypes::rotary_enc_none) return;

  
  // ENCODER CHANGED?
  uint8_t s = eBankData[currentBank][encNo].encoderState & 3;    // Get last state

  if (encMData[mcpNo].mcpState & (1 << defE41module.encPins[encNo%(defE41module.components.nEncoders)][0])) s |= 4; // Get new state for pin A
  if (encMData[mcpNo].mcpState & (1 << defE41module.encPins[encNo%(defE41module.components.nEncoders)][1])) s |= 8; // Get new state for pin B

  // Based on the last and new state of the encoder pins, determine direction of change
  switch (s) {
    case 0: case 5: case 10: case 15:{
      eData[encNo].encoderPosition = 0;
    }break;
    case 1: case 7: case 8: case 14:{
      eData[encNo].encoderPosition = 1; eData[encNo].encoderChange = true; 
    }break;
    case 2: case 4: case 11: case 13:{
      eData[encNo].encoderPosition = -1; eData[encNo].encoderChange = true;  
    }break;
    default:break;
  }
  eBankData[currentBank][encNo].encoderState = (s >> 2);  // save new state as last state

  if(eData[encNo].encoderChange){
    // Reset flag
    eData[encNo].encoderChange = false;
    // Get config info for this encoder
    uint16_t paramToSend = encoder[encNo].rotaryConfig.parameter[rotary_MSB]<<7 | encoder[encNo].rotaryConfig.parameter[rotary_LSB];
    byte channelToSend = encoder[encNo].rotaryConfig.channel + 1;
    uint16_t minValue = encoder[encNo].rotaryConfig.parameter[rotary_minMSB]<<7 | encoder[encNo].rotaryConfig.parameter[rotary_minLSB];
    uint16_t maxValue = encoder[encNo].rotaryConfig.parameter[rotary_maxMSB]<<7 | encoder[encNo].rotaryConfig.parameter[rotary_maxLSB];
      
    if (( eData[encNo].encoderPosition > 0 &&         // only keep going if i am increasing values, and still below the maximum
          eBankData[currentBank][encNo].encoderValue < maxValue) ||
        ( eData[encNo].encoderPosition < 0 &&         // or if i am decreasing values, and still above zero
          eBankData[currentBank][encNo].encoderValue > 0)){

      // Check if current module is in read priority list
      AddToPriority(mcpNo);
      
///////////////////////////////////////////////  
//////// ENCODER SPEED  ///////////////////////
///////////////////////////////////////////////
      
      // VARIABLE SPEED
      if (encoder[encNo].mode.speed == encoderRotarySpeed::rot_variable_speed){               
        if (millis() - eData[encNo].millisUpdatePrev < FAST_SPEED_MILLIS){                // if movement is faster, increase value
          eBankData[currentBank][encNo].encoderValue += eData[encNo].encoderPosition;
        }else if (millis() - eData[encNo].millisUpdatePrev < MID_SPEED_MILLIS){ 
          if(++eBankData[currentBank][encNo].pulseCounter >= MID_SPEED_COUNT){            // if speed is medium, count to two, then increase encoder value
            eBankData[currentBank][encNo].encoderValue += eData[encNo].encoderPosition;
            eBankData[currentBank][encNo].pulseCounter = 0;
          }
        }else if(++eBankData[currentBank][encNo].pulseCounter >= SLOW_SPEED_COUNT){            // if speed is slow, count to four, then increase encoder value
          eBankData[currentBank][encNo].encoderValue += eData[encNo].encoderPosition;
          eBankData[currentBank][encNo].pulseCounter = 0;
        }
      // SPEED 1
      }else if  (encoder[encNo].mode.speed == encoderRotarySpeed::rot_slow_speed && 
                (++eBankData[currentBank][encNo].pulseCounter >= SLOW_SPEED_COUNT)){     
        eBankData[currentBank][encNo].encoderValue += eData[encNo].encoderPosition;
        eBankData[currentBank][encNo].pulseCounter = 0;
      // SPEED 2
      }else if  (encoder[encNo].mode.speed == encoderRotarySpeed::rot_mid_speed && 
                (++eBankData[currentBank][encNo].pulseCounter >= MID_SPEED_COUNT)){     
        eBankData[currentBank][encNo].encoderValue += eData[encNo].encoderPosition;
        eBankData[currentBank][encNo].pulseCounter = 0;
      // SPEED 3
      }else if (encoder[encNo].mode.speed == encoderRotarySpeed::rot_fast_speed){                                                  
        eBankData[currentBank][encNo].encoderValue += eData[encNo].encoderPosition;
        eBankData[currentBank][encNo].pulseCounter = 0;
      }  
      eBankData[currentBank][encNo].encoderValue = constrain( eBankData[currentBank][encNo].encoderValue, 
                                                              minValue, 
                                                              maxValue);
    
      if(eBankData[currentBank][encNo].encoderValuePrev != eBankData[currentBank][encNo].encoderValue){     // If value changed
        // STATUS LED SET BLINK
       
        uint16_t valueToSend = eBankData[currentBank][encNo].encoderValue;
  //      SerialUSB.print(mcpNo); SerialUSB.print(" "); SerialUSB.print(encNo); SerialUSB.print(" "); SerialUSB.println(valueToSend); 
  //      
  //      SerialUSB.print(encNo); SerialUSB.print(": "); 
  //      SerialUSB.print(eBankData[currentBank][encNo].encoderValue);SerialUSB.println("");          
  
        switch(encoder[encNo].rotaryConfig.message){
          case encoderMessageTypes::rotary_enc_note:{
            if(encoder[encNo].rotaryConfig.midiPort & 0x01)
              MIDI.sendNoteOn( paramToSend&0x7f, valueToSend&0x7f, channelToSend);
            if(encoder[encNo].rotaryConfig.midiPort & 0x02)
              MIDIHW.sendNoteOn( paramToSend&0x7f, valueToSend&0x7f, channelToSend);
          }break;
          case encoderMessageTypes::rotary_enc_cc:{
            if(encoder[encNo].rotaryConfig.midiPort & 0x01)
              MIDI.sendControlChange( paramToSend&0x7f, valueToSend&0x7f, channelToSend);
            if(encoder[encNo].rotaryConfig.midiPort & 0x02)
              MIDIHW.sendControlChange( paramToSend&0x7f, valueToSend&0x7f, channelToSend);
          }break;
          case encoderMessageTypes::rotary_enc_pc_rel:{
            if(encoder[encNo].rotaryConfig.midiPort & 0x01)
              MIDI.sendProgramChange( valueToSend&0x7f, channelToSend);
            if(encoder[encNo].rotaryConfig.midiPort & 0x02)
              MIDIHW.sendProgramChange( valueToSend&0x7f, channelToSend);
          }break;
          case encoderMessageTypes::rotary_enc_nrpn:{
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
          case encoderMessageTypes::rotary_enc_rpn:{
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
          case encoderMessageTypes::rotary_enc_pb:{
            valueToSend = map(valueToSend, minValue, maxValue, -8192, 8191);
            if(encoder[encNo].rotaryConfig.midiPort & 0x01)
              MIDI.sendPitchBend( valueToSend, channelToSend);    
            if(encoder[encNo].rotaryConfig.midiPort & 0x02)
              MIDIHW.sendPitchBend( valueToSend, channelToSend);    
          }break;
          case encoderMessageTypes::rotary_enc_key:{
            if(eData[encNo].encoderPosition < 0){       // turned left
              if(encoder[encNo].rotaryConfig.parameter[rotary_modifierLeft])
                Keyboard.press(encoder[encNo].rotaryConfig.parameter[rotary_modifierLeft]);
              if(encoder[encNo].rotaryConfig.parameter[rotary_keyLeft])  
                Keyboard.press(encoder[encNo].rotaryConfig.parameter[rotary_keyLeft]);
            }else if(eData[encNo].encoderPosition > 0){ //turned right
              if(encoder[encNo].rotaryConfig.parameter[rotary_modifierRight])
                Keyboard.press(encoder[encNo].rotaryConfig.parameter[rotary_modifierRight]);
              if(encoder[encNo].rotaryConfig.parameter[rotary_keyRight])
                Keyboard.press(encoder[encNo].rotaryConfig.parameter[rotary_keyRight]);
            }
            millisKeyboardPress = millis();
            keyboardReleaseFlag = true; 
          }break;
        }
        eBankData[currentBank][encNo].encoderValuePrev = eBankData[currentBank][encNo].encoderValue;
        eData[encNo].millisUpdatePrev = millis();

        feedbackHw.SetStatusLED(STATUS_BLINK, 1, statusLEDtypes::STATUS_FB_INPUT_CHANGED);
        
        if(encoder[encNo].rotaryFeedback.source == fb_src_local){
          feedbackHw.SetChangeEncoderFeedback(FB_ENCODER, encNo, eBankData[currentBank][encNo].encoderValue, encMData[mcpNo].moduleOrientation);           // aprox 90 us / 4 rings de 16 leds   // 120 us / 8 enc // 200 us / 16 enc
        }
      }
    }
  }
  return;
}

uint16_t EncoderInputs::GetEncoderValue(uint8_t n){
  if(n < nEncoders)
    return eBankData[currentBank][n].encoderValuePrev;
}

bool EncoderInputs::GetEncoderSwitchValue(uint8_t n){
  if(n < nEncoders)
    return eBankData[currentBank][n].switchInputState;
}

uint8_t EncoderInputs::GetModuleOrientation(uint8_t n){
  if(n < nModules)
    return encMData[n].moduleOrientation;
}

void EncoderInputs::AddToPriority(byte nMCP){
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

void EncoderInputs::SetNextAddress(MCP23S17 *mcpX, byte addr){

  for (int i = 0; i<3; i++){
    mcpX->pinMode(defE41module.nextAddressPin[i],OUTPUT);
    mcpX->digitalWrite(defE41module.nextAddressPin[i],(addr>>i)&1);
  }
//  SerialUSB.print("Next: ");  SerialUSB.print(addr); SerialUSB.print(": ");
//  SerialUSB.print((addr>>2)&1, BIN);
//  SerialUSB.print((addr>>1)&1, BIN);
//  SerialUSB.println(addr&1, BIN);
  return;
}
