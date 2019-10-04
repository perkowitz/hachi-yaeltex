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
       eBankData[b][e].pulseCounter = 0;
       eBankData[b][e].switchInputState = false;
    }
  }

  for(int e = 0; e < nEncoders; e++){
    eData[e].encoderPosition = 0;
    eData[e].encoderPositionPrev = 0;
    eData[e].encoderChange = false;
    eData[e].millisUpdatePrev = 0;
    eData[e].switchHWState = 0;
    eData[e].switchHWStatePrev = 0;
    eData[e].swBounceMillisPrev = 0;
    eData[e].encoderState = RFS_START;
    eData[e].a = 0;
    eData[e].a0 = 0;
    eData[e].b = 0;
    eData[e].b0 = 0;
    eData[e].c0 = 0;
    eData[e].d0 = 0;
    FilterClear(e);
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
      SetStatusLED(STATUS_BLINK, 1, STATUS_FB_ERROR);
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
       if(n != 3 && n!=2){
        encodersMCP[n].pullUp(i, HIGH);
       }else{
        encodersMCP[n].pinMode(i, INPUT); 
       }
      }
    }
    delay(5);
    encMData[n].mcpState = encodersMCP[n].digitalRead();
    for (int e = 0; e < 4; e++){
      byte pinState = 0;
      if (encMData[e].mcpState & (1 << defE41module.encPins[e%(defE41module.components.nEncoders)][0])){
        pinState |= 2; // Get new state for pin A
        eData[e].a = 1; // Get new state for pin A
      }else  eData[e].a = 0;
      
      if (encMData[e].mcpState & (1 << defE41module.encPins[e%(defE41module.components.nEncoders)][1])){
        pinState |= 1; // Get new state for pin B
        eData[e].b = 1; // Get new state for pin B
      }else  eData[e].b = 0;
  
      eData[e].encoderState = pinState;
      SerialUSB.print("ENC "); SerialUSB.print(e); SerialUSB.print(": ");
      SerialUSB.println(eData[e].encoderState, BIN);
    } 
    
  }
  return;
}


void EncoderInputs::Read(){
  if(!nBanks || !nEncoders || !nModules) return;    // If number of encoders is zero, return;

  unsigned long antMicrosERead = micros();  
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
// SerialUSB.println(micros()-antMicrosERead); 
  
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
//      SerialUSB.print(encMData[0].mcpState,BIN); SerialUSB.println(" ");
      encMData[mcpNo].mcpStatePrev = encMData[mcpNo].mcpState; 
//      for (int i = 0; i < 16; i++) {
//        SerialUSB.print( (encMData[mcpNo].mcpState >> (15 - i)) & 0x01, BIN);
//        if (i == 9 || i == 6) SerialUSB.print(" ");
//      }
//      SerialUSB.println();
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
    SetStatusLED(STATUS_BLINK, 1, statusLEDtypes::STATUS_FB_INPUT_CHANGED);
//    SerialUSB.print("SWITCH "); SerialUSB.print(encNo); SerialUSB.print(": ");SerialUSB.println(eData[encNo].switchHWState ? "ON" : "OFF");
    if (eData[encNo].switchHWState){
//      if(encNo < nBanks && currentBank != encNo ){ // ADD BANK CONDITION
//       // currentBank = memHost->LoadBank(encNo);
//        //SerialUSB.print("Loaded Bank: "); SerialUSB.println(currentBank);
//      }
      eBankData[currentBank][encNo].switchInputState = !eBankData[currentBank][encNo].switchInputState;
      
      feedbackHw.SetChangeEncoderFeedback(FB_ENCODER_SWITCH, encNo, 
                                          eBankData[currentBank][encNo].switchInputState, 
                                          encMData[mcpNo].moduleOrientation);   
    }else if(!eData[encNo].switchHWState && encoder[encNo].switchConfig.action != switchActions::switch_toggle){
      eBankData[currentBank][encNo].switchInputState = 0;
      feedbackHw.SetChangeEncoderFeedback(FB_ENCODER_SWITCH, encNo, eBankData[currentBank][encNo].switchInputState, encMData[mcpNo].moduleOrientation);         
    }
    
    switch(encoder[encNo].switchConfig.mode){
      case 0:{
            
      }break;
      default: break;
    }
  }
  
}

void EncoderInputs::EncoderCheck(byte mcpNo, byte encNo){
  if(encoder[encNo].rotaryConfig.message == encoderMessageTypes::rotary_enc_none) return;
  static unsigned long antMicrosEncoder = 0;
  
  // ENCODER CHANGED?
  uint8_t s = eData[encNo].encoderState & 3;    // Get last state

  byte pinState = 0;
 
  
  if (encMData[mcpNo].mcpState & (1 << defE41module.encPins[encNo%(defE41module.components.nEncoders)][0])){
    pinState |= 2; // Get new state for pin A
    eData[encNo].a = 1; // Get new state for pin A
    s |= 8;
  }else  eData[encNo].a = 0;
  
  if (encMData[mcpNo].mcpState & (1 << defE41module.encPins[encNo%(defE41module.components.nEncoders)][1])){
    pinState |= 1; // Get new state for pin B
    eData[encNo].b = 1; // Get new state for pin B
    s |= 4;
  }else  eData[encNo].b = 0;

  // DEBOUNCE - If A or B change, we check the other pin. If it didn't change, then it´s bounce noise.
  // Debounce algorithm from http://www.technoblogy.com/show?1YHJ
  if (eData[encNo].a != eData[encNo].a0) {              // A changed
    eData[encNo].a0 = eData[encNo].a;
    if (eData[encNo].b != eData[encNo].c0) {
      eData[encNo].c0 = eData[encNo].b;
      eData[encNo].encoderChange = true;
//      if(eData[encNo].a == eData[encNo].b){
//        eData[encNo].encoderPosition = -1;
////        SerialUSB.println("Rotating Right");
//      }else{
//        eData[encNo].encoderPosition = 1;
////        SerialUSB.println("Rotating Left");
//      }
    }else{
      eData[encNo].millisUpdatePrev = millis();
      return;
    }
  }else if (eData[encNo].b != eData[encNo].b0) {       // B changed
    eData[encNo].b0 = eData[encNo].b;
    if (eData[encNo].a != eData[encNo].d0) {
      eData[encNo].d0 = eData[encNo].a;
      eData[encNo].encoderChange = true;
//      if(eData[encNo].a == eData[encNo].b){
//        eData[encNo].encoderPosition = 1;
////        SerialUSB.println("Rotating Left");
//      }else{
//        eData[encNo].encoderPosition = -1;
////        SerialUSB.println("Rotating Right");
//      }     
    }else{
      eData[encNo].millisUpdatePrev = millis();
      return;
    }
  }else{
    //eData[encNo].millisUpdatePrev = millis();
    return;
  }

//  // Based on the last and new state of the encoder pins, determine direction of change
//  switch (s) {
//    case 0: case 5: case 10: case 15:{
//      eData[encNo].encoderPosition = 0;
//    }break;
//    case 1: case 7: case 8: case 14:{
//      eData[encNo].encoderPosition = 1; eData[encNo].encoderChange = true; 
//    }break;
//    case 2: case 4: case 11: case 13:{
//      eData[encNo].encoderPosition = -1; eData[encNo].encoderChange = true;  
//    }break;
//    default:{
//      eData[encNo].encoderPosition = 0; eData[encNo].encoderChange = false; 
//    }break;
//  }
//  eData[encNo].encoderState = (s >> 2);  // save new state as last state

//  // Check state in table
  eData[encNo].encoderState = pgm_read_byte(&quarterStepTable[eData[encNo].encoderState & 0x0f][pinState]);
//  eData[encNo].encoderState = pgm_read_byte(&halfStepTable[eData[encNo].encoderState & 0x0f][pinState]);
//  eData[encNo].encoderState = pgm_read_byte(&fullStepTable[eData[encNo].encoderState & 0x0f][pinState]);
//  SerialUSB.print("ENC "); SerialUSB.print(encNo); SerialUSB.print(" pin state: "); SerialUSB.print(pinState,BIN);
//  SerialUSB.print(" Next state: ");SerialUSB.print(eData[encNo].encoderState&0xF, HEX);
//  SerialUSB.print(" DIR: ");SerialUSB.println((eData[encNo].encoderState&0x30) == 0x10 ? "1" : (eData[encNo].encoderState&0x30) == 0x20 ? "-1" : "0");
  
  // if at a valid state, check direction
  switch (eData[encNo].encoderState & 0x30) {
    case DIR_CW:{
        eData[encNo].encoderPosition = 1; 
    }   break;
    case DIR_CCW:{
        eData[encNo].encoderPosition = -1;
    }   break;
    case DIR_NONE:
    default:{
        eData[encNo].encoderPosition = 0; 
        eData[encNo].encoderChange = false; 
        eData[encNo].millisUpdatePrev = millis();
        return;
    }   break;
  }
  
  if(eData[encNo].encoderChange && (micros()-eData[encNo].antMicrosCheck > 1000)){
    eData[encNo].antMicrosCheck = micros();
    // Reset flag
    eData[encNo].encoderChange = false;  

   if(encNo == 0){
    SerialUSB.println(micros()-antMicrosEncoder);
    antMicrosEncoder = micros();
  }
    
    // Get config info for this encoder
    uint16_t paramToSend = encoder[encNo].rotaryConfig.parameter[rotary_MSB]<<7 | encoder[encNo].rotaryConfig.parameter[rotary_LSB];
    byte channelToSend = encoder[encNo].rotaryConfig.channel + 1;
    uint16_t minValue = encoder[encNo].rotaryConfig.parameter[rotary_minMSB]<<7 | encoder[encNo].rotaryConfig.parameter[rotary_minLSB];
    uint16_t maxValue = encoder[encNo].rotaryConfig.parameter[rotary_maxMSB]<<7 | encoder[encNo].rotaryConfig.parameter[rotary_maxLSB];
      
    if (( eData[encNo].encoderPosition > 0 &&         // only keep going if i am increasing values, and still below the maximum
          eBankData[currentBank][encNo].encoderValue < maxValue) ||
        ( eData[encNo].encoderPosition < 0 &&         // or if i am decreasing values, and still above zero
          eBankData[currentBank][encNo].encoderValue > 0)){
           
      //SerialUSB.print(mcpNo); SerialUSB.print(" "); SerialUSB.print(encNo); SerialUSB.print(" "); SerialUSB.println(eBankData[currentBank][encNo].encoderValue, HEX);   

      // Check if current module is in read priority list
      AddToPriority(mcpNo); 

      ///////////////////////////////////////////////  
      //////// ENCODER SPEED  ///////////////////////
      ///////////////////////////////////////////////
    
      // VARIABLE SPEED
      static byte currentSpeed = 0;
      unsigned long timeLastChange = millis() - eData[encNo].millisUpdatePrev;
//      SerialUSB.println(timeLastChange);
//      eBankData[currentBank][encNo].encoderValue += eData[encNo].encoderPosition;
      if (encoder[encNo].mode.speed == encoderRotarySpeed::rot_variable_speed){               
        if (timeLastChange < FAST_SPEED_MILLIS){                // if movement is faster, increase value
          currentSpeed = FAST_SPEED;
        }else if (timeLastChange < MID4_SPEED_MILLIS && (currentSpeed == FAST_SPEED || currentSpeed == MID3_SPEED)){ 
          if(++eBankData[currentBank][encNo].pulseCounter >= MID_SPEED_COUNT){            // if speed is medium, count to two, then increase encoder value
            currentSpeed = MID4_SPEED;
            eBankData[currentBank][encNo].pulseCounter = 0;
          }
        }else if (timeLastChange < MID3_SPEED_MILLIS && (currentSpeed == MID4_SPEED || currentSpeed == MID2_SPEED)){ 
          if(++eBankData[currentBank][encNo].pulseCounter >= MID_SPEED_COUNT){            // if speed is medium, count to two, then increase encoder value
            currentSpeed = MID3_SPEED;
            eBankData[currentBank][encNo].pulseCounter = 0;
          }
        }else if (timeLastChange < MID2_SPEED_MILLIS && (currentSpeed == MID3_SPEED || currentSpeed == MID1_SPEED)){ 
          if(++eBankData[currentBank][encNo].pulseCounter >= MID_SPEED_COUNT){            // if speed is medium, count to two, then increase encoder value
            currentSpeed = MID2_SPEED;
            eBankData[currentBank][encNo].pulseCounter = 0;
          }
        }else if (timeLastChange < MID1_SPEED_MILLIS && (currentSpeed == MID2_SPEED || currentSpeed == SLOW_SPEED)){ 
          if(++eBankData[currentBank][encNo].pulseCounter >= MID_SPEED_COUNT){            // if speed is medium, count to two, then increase encoder value
            currentSpeed = MID1_SPEED;
            eBankData[currentBank][encNo].pulseCounter = 0;
          }
        }else if(++eBankData[currentBank][encNo].pulseCounter >= SLOW_SPEED_COUNT){            // if speed is slow, count to four, then increase encoder value
          currentSpeed = SLOW_SPEED;
          eBankData[currentBank][encNo].pulseCounter = 0;
        }
      // SPEED 1
      }else if  (encoder[encNo].mode.speed == encoderRotarySpeed::rot_slow_speed && 
                (++eBankData[currentBank][encNo].pulseCounter >= SLOW_SPEED_COUNT)){     
        currentSpeed = 1;
        eBankData[currentBank][encNo].pulseCounter = 0;
      // SPEED 2
      }else if  (encoder[encNo].mode.speed == encoderRotarySpeed::rot_mid_speed && 
                (++eBankData[currentBank][encNo].pulseCounter >= MID_SPEED_COUNT)){     
        currentSpeed = 1;
        eBankData[currentBank][encNo].pulseCounter = 0;
      // SPEED 3
      }else if (encoder[encNo].mode.speed == encoderRotarySpeed::rot_fast_speed){                                                  
        currentSpeed = 1;
        eBankData[currentBank][encNo].pulseCounter = 0;
      }  
//      SerialUSB.print(encNo); SerialUSB.print(" "); SerialUSB.print(encNo);
      eData[encNo].millisUpdatePrev = millis();

      if(eData[encNo].encoderPosition != eData[encNo].encoderPositionPrev) currentSpeed = 1;    // If direction changed, set speed to minimum

      eBankData[currentBank][encNo].encoderValue += currentSpeed*eData[encNo].encoderPosition;    // New value
      eData[encNo].encoderPositionPrev = eData[encNo].encoderPosition;                            

      if(eBankData[currentBank][encNo].encoderValue > maxValue) eBankData[currentBank][encNo].encoderValue = maxValue;
      if(eBankData[currentBank][encNo].encoderValue < minValue) eBankData[currentBank][encNo].encoderValue = minValue;
  
//      SerialUSB.print("Before filter  ");
//      SerialUSB.print(encNo); SerialUSB.print(": "); 
//      SerialUSB.print(eBankData[currentBank][encNo].encoderValue);SerialUSB.print("    ");
//      int16_t filteredValue = FilterGetNewAverage(encNo, eBankData[currentBank][encNo].encoderValue);
      int16_t filteredValue = eBankData[currentBank][encNo].encoderValue;
//      SerialUSB.print("After filter  ");
//      SerialUSB.print(encNo); SerialUSB.print(": "); 
//      SerialUSB.print(filteredValue);SerialUSB.println(""); 
      
      if(eBankData[currentBank][encNo].encoderValuePrev != filteredValue){     // If value changed
        eBankData[currentBank][encNo].encoderValuePrev = filteredValue;
        eBankData[currentBank][encNo].encoderValue = filteredValue;

        uint16_t valueToSend = eBankData[currentBank][encNo].encoderValue;
  
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

        SetStatusLED(STATUS_BLINK, 1, statusLEDtypes::STATUS_FB_INPUT_CHANGED);
        
        if(encoder[encNo].rotaryFeedback.source == fb_src_local){
          feedbackHw.SetChangeEncoderFeedback(FB_ENCODER, encNo, eBankData[currentBank][encNo].encoderValue, encMData[mcpNo].moduleOrientation);           // aprox 90 us / 4 rings de 16 leds   // 120 us / 8 enc // 200 us / 16 enc
        }
      }
    }
  }
  return;
}

/*
    Filtro de media móvil para el sensor de ultrasonido (librería RunningAverage integrada) (http://playground.arduino.cc/Main/RunningAverage)
*/
int16_t EncoderInputs::FilterGetNewAverage(uint8_t encNo, uint16_t newVal) {
  unsigned int fIndex = eData[encNo].filterIndex;
  
  eData[encNo].filterSum -= eData[encNo].filterSamples[fIndex];
  eData[encNo].filterSamples[fIndex] = newVal;
  eData[encNo].filterSum += eData[encNo].filterSamples[fIndex];
  
  eData[encNo].filterIndex++;
  
  if (eData[encNo].filterIndex == FILTER_SIZE_ENCODER) eData[encNo].filterIndex = 0;  // faster than %
  
  // update count as last otherwise if( _cnt == 0) above will fail
  if (eData[encNo].filterCount < FILTER_SIZE_ENCODER)
    eData[encNo].filterCount++;
    
  if (eData[encNo].filterCount == 0)
    return NAN;
    
  return eData[encNo].filterSum / eData[encNo].filterCount;
}

/*
   Limpia los valores del filtro de media móvil para un nuevo uso.
*/
void EncoderInputs::FilterClear(uint8_t input) {
  eData[input].filterCount = 0;
  eData[input].filterIndex = 0;
  eData[input].filterSum = 0;
  for (uint8_t i = 0; i < FILTER_SIZE_ENCODER; i++) {
    eData[input].filterSamples[i] = 0; // keeps addValue simpler
  }
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

void EncoderInputs::SetNextAddress(SPIExpander *mcpX, byte addr){
  for (int i = 0; i<3; i++){
    mcpX->pinMode(defE41module.nextAddressPin[i],OUTPUT);   
    mcpX->digitalWrite(defE41module.nextAddressPin[i],(addr>>i)&1);
  }
  return;
}
