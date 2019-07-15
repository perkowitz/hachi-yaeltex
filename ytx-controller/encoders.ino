#include "headers/EncoderInputs.h"

//----------------------------------------------------------------------------------------------------
// ENCODER FUNCTIONS
//----------------------------------------------------------------------------------------------------
void EncoderInputs::Init(uint8_t maxBanks, uint8_t maxEncoders, SPIClass *spiPort){
  nBanks = maxBanks;
  nEncoders = maxEncoders;
  nModules = nEncoders/4;
  
  if(!nBanks || !nEncoders || !nModules) return;    // If number of encoders is zero, return;
  
  // First dimension is an array of pointers, each pointing to a column - https://www.eskimo.com/~scs/cclass/int/sx9b.html
  eBankData = (encoderBankData**) memHost->AllocateRAM(nBanks*sizeof(encoderBankData*));
  eData = (encoderData*) memHost->AllocateRAM(nEncoders*sizeof(encoderData));
  mData = (moduleData*) memHost->AllocateRAM(nModules*sizeof(moduleData));

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
  
  for (int n = 0; n < nModules; n++){
    if(config->hwMapping.encoder[n] > EncoderModuleTypes::ENCODER_NONE){
      mData[n].moduleOrientation = config->hwMapping.encoder[n]-1;    // 0 -> HORIZONTAL , 1 -> VERTICAL
      SerialUSB.print("mData[0].moduleOrientation: "); SerialUSB.println(mData[n].moduleOrientation);
    }else{
//      mData[n].moduleOrientation = e41orientation::HORIZONTAL;
//      SerialUSB.print("mData[0].moduleOrientation - b: "); SerialUSB.println(mData[n].moduleOrientation);
      feedbackHw.SetStatusLED(STATUS_BLINK, 1, STATUS_FB_ERROR);
    }
    
    encodersMCP[n].begin(spiPort, encodersMCPChipSelect, n);  
//    printPointer(&encodersMCP[n]);
    
    mData[n].mcpState = 0;
    mData[n].mcpStatePrev = 0;
    for(int i=0; i<16; i++){
      if(i != defE41module.nextAddressPin[0] && i != defE41module.nextAddressPin[1] && 
        i != defE41module.nextAddressPin[2] && i != (defE41module.nextAddressPin[2]+1)){
       encodersMCP[n].pullUp(i, HIGH);
      }
    }
  }
  // AFTER INITIALIZATION SET NEXT ADDRESS ON EACH MODULE (EXCEPT 3 and 7, cause they don't have a next on the chain)
  for (int n = 0; n < nModules; n++){
    if(nModules > 1 && n != 3 && n != 7){
      SetNextAddress(encodersMCP[n], n+1);
    }
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
    mData[priorityModule].mcpState = encodersMCP[priorityModule].digitalRead();
//    SerialUSB.print("Priority Read Module: ");SerialUSB.println(priorityModule);
  }
  if(priorityCount == 2){
    byte priorityModule = priorityList[1];
    mData[priorityModule].mcpState = encodersMCP[priorityModule].digitalRead();
//    SerialUSB.print("Priority Read Module: ");SerialUSB.println(priorityModule);
  }else{  
    for (int n = 0; n < nModules; n++){       
      mData[n].mcpState = encodersMCP[n].digitalRead();
    }
 } 
//  SerialUSB.print(mData[0].mcpState,BIN); SerialUSB.println(" ");
//  SerialUSB.print(mData[1].mcpState,BIN); SerialUSB.print(" ");
//  SerialUSB.print(mData[2].mcpState,BIN); SerialUSB.print(" ");
//  SerialUSB.print(mData[3].mcpState,BIN); SerialUSB.print(" ");
//  SerialUSB.println(mData[4].mcpState,BIN);
  byte encNo = 0; 
  byte nEncodInMod = 0;
  for(byte mcpNo = 0; mcpNo < nModules; mcpNo++){
    nEncodInMod = defE41module.components.nEncoders;  // CHANGE WHEN DIFFERENT MODULES ARE ADDED
    if( mData[mcpNo].mcpState != mData[mcpNo].mcpStatePrev){
      mData[mcpNo].mcpStatePrev = mData[mcpNo].mcpState; 
//      SerialUSB.print("ENCODER # "); SerialUSB.println(encNo);
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
  eData[encNo].switchHWState = !(mData[mcpNo].mcpState & (1 << defE41module.encSwitchPins[encNo%(defE41module.components.nEncoders)]));
  
  if(eData[encNo].switchHWState != eData[encNo].switchHWStatePrev){
    eData[encNo].switchHWStatePrev = eData[encNo].switchHWState;
    
    IsInPriority(mcpNo);
    
    // STATUS LED SET BLINK
    feedbackHw.SetStatusLED(STATUS_BLINK, 1, STATUS_FB_INPUT_CHANGED);
    
    if (eData[encNo].switchHWState){
//      if(encNo < nBanks && currentBank != encNo ){ // ADD BANK CONDITION
//       // currentBank = memHost->LoadBank(encNo);
//        //SerialUSB.print("Loaded Bank: "); SerialUSB.println(currentBank);
//      }
      eBankData[currentBank][encNo].switchInputState = !eBankData[currentBank][encNo].switchInputState;

//      feedbackHw.Update(FB_ENCODER_SWITCH, encNo, eBankData[currentBank][n].switchInputState, mData[mcpNo].moduleOrientation);           // aprox 90 us / 4 rings de 16 leds   // 120 us / 8 enc // 200 us / 16 enc       
      feedbackHw.SetChangeEncoderFeedback(FB_ENCODER_SWITCH, encNo, 
                                          eBankData[currentBank][encNo].switchInputState, 
                                          mData[mcpNo].moduleOrientation);   
      
//    }else if(!eData[encNo].switchHWState){
    }else if(!eData[encNo].switchHWState && encoder[encNo].switchConfig.action != switchActions::switch_toggle){
      eBankData[currentBank][encNo].switchInputState = 0;
      feedbackHw.SetChangeEncoderFeedback(FB_ENCODER_SWITCH, encNo, eBankData[currentBank][encNo].switchInputState, mData[mcpNo].moduleOrientation);         
    }
  }
  
}

void EncoderInputs::EncoderCheck(byte mcpNo, byte encNo){
  // ENCODER CHECK
  uint8_t s = eBankData[currentBank][encNo].encoderState & 3;        

  if (mData[mcpNo].mcpState & (1 << defE41module.encPins[encNo%(defE41module.components.nEncoders)][0])) s |= 4;
  if (mData[mcpNo].mcpState & (1 << defE41module.encPins[encNo%(defE41module.components.nEncoders)][1])) s |= 8;

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
//    case 3: case 12:
//      eData[encNo].encoderPosition = 2; eData[encNo].encoderChange = true; break;
    default:
//      eData[encNo].encoderPosition = -2; eData[encNo].encoderChange = true;
    break;
  }
  eBankData[currentBank][encNo].encoderState = (s >> 2);
  
  if(eData[encNo].encoderChange){
    eData[encNo].encoderChange = false;

    if ((eData[encNo].encoderPosition > 0 && eBankData[currentBank][encNo].encoderValue < MAX_ENC_VAL) ||
        (eData[encNo].encoderPosition < 0 && eBankData[currentBank][encNo].encoderValue > 0)){

///////////////////////////////////////////////  
//////// ENCODER SPEED  ///////////////////////
///////////////////////////////////////////////
      
      // VARIABLE SPEED
      if (encoder[encNo].mode.speed == 0){               
        if (millis() - eData[encNo].millisUpdatePrev < 10){
          eBankData[currentBank][encNo].encoderValue += eData[encNo].encoderPosition;
        }else if (millis() - eData[encNo].millisUpdatePrev < 30){ 
          if(++eBankData[currentBank][encNo].pulseCounter >= 2){            // if movement is slow, count to four, then add
            eBankData[currentBank][encNo].encoderValue += eData[encNo].encoderPosition;
            eBankData[currentBank][encNo].pulseCounter = 0;
          }
        }else if(++eBankData[currentBank][encNo].pulseCounter >= 4){            // if movement is slow, count to four, then add
          eBankData[currentBank][encNo].encoderValue += eData[encNo].encoderPosition;
          eBankData[currentBank][encNo].pulseCounter = 0;
        }
      // SPEED 1
      }else if (encoder[encNo].mode.speed == 1 && (++eBankData[currentBank][encNo].pulseCounter >= 4)){     
        eBankData[currentBank][encNo].encoderValue += eData[encNo].encoderPosition;
        eBankData[currentBank][encNo].pulseCounter = 0;
      // SPEED 2
      }else if (encoder[encNo].mode.speed == 2 && (++eBankData[currentBank][encNo].pulseCounter >= 2)){     
        eBankData[currentBank][encNo].encoderValue += eData[encNo].encoderPosition;
        eBankData[currentBank][encNo].pulseCounter = 0;
      // SPEED 3
      }else if (encoder[encNo].mode.speed == 3){                                                  
        eBankData[currentBank][encNo].encoderValue += eData[encNo].encoderPosition;
        eBankData[currentBank][encNo].pulseCounter = 0;
      }  
      eBankData[currentBank][encNo].encoderValue = constrain(eBankData[currentBank][encNo].encoderValue, 0, MAX_ENC_VAL);
    }

    // Check if current module is in read priority list
    IsInPriority(mcpNo);
    
    if(eBankData[currentBank][encNo].encoderValuePrev != eBankData[currentBank][encNo].encoderValue){     // If value changed
         
      // STATUS LED SET BLINK
      feedbackHw.SetStatusLED(STATUS_BLINK, 1, STATUS_FB_INPUT_CHANGED);
      
      SerialUSB.print(F("BANK N° ")); SerialUSB.print(currentBank);
      SerialUSB.print(F(" Encoder N° ")); SerialUSB.print(encNo);
      SerialUSB.print(F(" Value: ")); SerialUSB.println(eBankData[currentBank][encNo].encoderValue); SerialUSB.println();
        
      MIDI.sendControlChange(encNo+4*currentBank, eBankData[currentBank][encNo].encoderValue, MIDI_CHANNEL);
      //MIDIHW.sendControlChange(encNo, eBankData[currentBank][encNo].encoderValue, MIDI_CHANNEL+1);

      eBankData[currentBank][encNo].encoderValuePrev = eBankData[currentBank][encNo].encoderValue;
      eData[encNo].millisUpdatePrev = millis();

//      feedbackHw.Update(FB_ENCODER, encNo, eBankData[currentBank][encNo].encoderValue, mData[mcpNo].moduleOrientation);           // aprox 90 us / 4 rings de 16 leds   // 120 us / 8 enc // 200 us / 16 enc
      feedbackHw.SetChangeEncoderFeedback(FB_ENCODER, encNo, eBankData[currentBank][encNo].encoderValue, mData[mcpNo].moduleOrientation);           // aprox 90 us / 4 rings de 16 leds   // 120 us / 8 enc // 200 us / 16 enc
    }
  }
  return;
}

uint16_t EncoderInputs::GetEncoderValue(uint8_t n){
  if(n <= nEncoders)
    return eBankData[currentBank][n].encoderValuePrev;
}

bool EncoderInputs::GetEncoderSwitchValue(uint8_t n){
  if(n <= nEncoders)
    return eBankData[currentBank][n].switchInputState;
}

uint8_t EncoderInputs::GetModuleOrientation(uint8_t n){
  if(n <= nModules)
    return mData[n].moduleOrientation;
}

void EncoderInputs::IsInPriority(byte nMCP){
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

void EncoderInputs::SetNextAddress(MCP23S17 mcpX, byte addr){
  mcpX.pinMode(defE41module.nextAddressPin[0],OUTPUT);
  mcpX.pinMode(defE41module.nextAddressPin[1],OUTPUT);
  mcpX.pinMode(defE41module.nextAddressPin[2],OUTPUT);
    
  mcpX.digitalWrite(defE41module.nextAddressPin[0], addr&1);
  mcpX.digitalWrite(defE41module.nextAddressPin[1],(addr>>1)&1);
  mcpX.digitalWrite(defE41module.nextAddressPin[2],(addr>>2)&1);
  return;
}
