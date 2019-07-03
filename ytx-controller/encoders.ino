#include "headers/EncoderInputs.h"

//----------------------------------------------------------------------------------------------------
// ENCODER FUNCTIONS
//----------------------------------------------------------------------------------------------------
void EncoderInputs::Init(uint8_t maxBanks, uint8_t maxEncoders, SPIClass *spiPort){
  nBanks = maxBanks;
  nEncoders = maxEncoders;
  nModules = nEncoders/4;

  // First dimension is an array of pointers, each pointing to a column - https://www.eskimo.com/~scs/cclass/int/sx9b.html
  encoderValue = (uint16_t**) memHost->AllocateRAM(nBanks*sizeof(uint16_t*));
  encoderValuePrev = (uint16_t**) memHost->AllocateRAM(nBanks*sizeof(uint16_t*));
  encoderState = (uint8_t**) memHost->AllocateRAM(nBanks*sizeof(uint8_t*));
  pulseCounter = (uint16_t**) memHost->AllocateRAM(nBanks*sizeof(uint16_t*));
  switchInputState = (bool**) memHost->AllocateRAM(nBanks*sizeof(bool*));
  
  millisUpdatePrev = (uint32_t*) memHost->AllocateRAM(nEncoders*sizeof(uint32_t));
  swBounceMillisPrev = (uint32_t*) memHost->AllocateRAM(nEncoders*sizeof(uint32_t));
  encoderPosition = (int16_t*) memHost->AllocateRAM(nEncoders*sizeof(int16_t));
  encoderChange = (uint8_t*) memHost->AllocateRAM(nEncoders*sizeof(uint8_t));
  switchHWState = (uint8_t*) memHost->AllocateRAM(nEncoders*sizeof(uint8_t));
  switchHWStatePrev = (uint8_t*) memHost->AllocateRAM(nEncoders*sizeof(uint8_t));
 
  //encodersMCP = (MCP23S17*) memHost->AllocateRAM(nModules*sizeof(MCP23S17));
  mcpState = (uint16_t*) memHost->AllocateRAM(nModules*sizeof(uint16_t));
  mcpStatePrev = (uint16_t*) memHost->AllocateRAM(nModules*sizeof(uint16_t));
  moduleOrientation = (byte*) memHost->AllocateRAM(nModules*sizeof(byte));

  for (int b = 0; b < nBanks; b++){
    encoderValue[b] = (uint16_t*) memHost->AllocateRAM(nEncoders * sizeof(uint16_t));
    encoderValuePrev[b] = (uint16_t*) memHost->AllocateRAM(nEncoders * sizeof(uint16_t));
    encoderState[b] = (uint8_t*) memHost->AllocateRAM(nEncoders * sizeof(uint8_t));
    pulseCounter[b] = (uint16_t*) memHost->AllocateRAM(nEncoders * sizeof(uint16_t));
    switchInputState[b] = (bool*) memHost->AllocateRAM(nEncoders * sizeof(bool));
  }
  
  for(int e = 0; e < nEncoders; e++){
    encoderPosition[e] = 0;
    encoderChange[e] = false;
    millisUpdatePrev[e] = 0;
    switchHWState[e] = 0;
    switchHWStatePrev[e] = 0;
  }
  // Set all elements in arrays to 0
  for(int b = 0; b < nBanks; b++){
    for(int e = 0; e < nEncoders; e++){
       encoderValue[b][e] = 0;
       encoderValuePrev[b][e] = 0;
       encoderState[b][e] = 0;
       pulseCounter[b][e] = 0;
       switchInputState[b][e] = false;
    }
  }
  pinMode(encodersMCPChipSelect, OUTPUT);
  
  for (int n = 0; n < nModules; n++){
    moduleOrientation[n] = config->hwMapping.encoder[n]-1; // 0 is Horizontal - 1 is Vertical
    
    encodersMCP[n].begin(spiPort, encodersMCPChipSelect, n);  
    
    mcpState[n] = 0;
    mcpStatePrev[n] = 0;
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
  if(priorityCount && (millis()-priorityTime > 500)){
    priorityCount = 0;
//    SerialUSB.print("Priority list emptied");
  }
  if(priorityCount >= 1){
    byte priorityModule = priorityList[0];
    mcpState[priorityModule] = encodersMCP[priorityModule].digitalRead();
//    SerialUSB.print("Priority Read Module: ");SerialUSB.println(priorityModule);
  }
  if(priorityCount == 2){
    byte priorityModule = priorityList[1];
    mcpState[priorityModule] = encodersMCP[priorityModule].digitalRead();
//    SerialUSB.print("Priority Read Module: ");SerialUSB.println(priorityModule);
  }else{  
    for (int n = 0; n < nModules; n++){       
      mcpState[n] = encodersMCP[n].digitalRead();
    }
 } 
//  SerialUSB.print(mcpState[0],BIN); SerialUSB.print(" ");
//  SerialUSB.print(mcpState[1],BIN); SerialUSB.print(" ");
//  SerialUSB.print(mcpState[2],BIN); SerialUSB.print(" ");
//  SerialUSB.print(mcpState[3],BIN); SerialUSB.print(" ");
//  SerialUSB.println(mcpState[4],BIN);
  byte encNo = 0; 
  byte nEncodInMod = 0;
  for(byte mcpNo = 0; mcpNo < nModules; mcpNo++){
    nEncodInMod = defE41module.components.nEncoders;  // CHANGE WHEN DIFFERENT MODULES ARE ADDED
    if( mcpState[mcpNo] != mcpStatePrev[mcpNo]){
      mcpStatePrev[mcpNo] = mcpState[mcpNo]; 
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
  switchHWState[encNo] = !(mcpState[mcpNo] & (1 << defE41module.encSwitchPins[encNo%(defE41module.components.nEncoders)]));
  
  if(switchHWState[encNo] != switchHWStatePrev[encNo] && (swBounceMillisPrev[encNo] - millis() > BOUNCE_MILLIS)){
    swBounceMillisPrev[encNo] = millis();
    switchHWStatePrev[encNo] = switchHWState[encNo];
    
    IsInPriority(mcpNo);
    
    if (switchHWState[encNo]){
//      if(encNo < nBanks && currentBank != encNo ){ // ADD BANK CONDITION
//       // currentBank = memHost->LoadBank(encNo);
//        //SerialUSB.print("Loaded Bank: "); SerialUSB.println(currentBank);
//      }
      switchInputState[currentBank][encNo] = !switchInputState[currentBank][encNo];

//      feedbackHw.Update(FB_ENCODER_SWITCH, encNo, switchInputState[currentBank][encNo], moduleOrientation[mcpNo]);           // aprox 90 us / 4 rings de 16 leds   // 120 us / 8 enc // 200 us / 16 enc       
      feedbackHw.Update(FB_ENCODER_SWITCH, encNo, switchInputState[currentBank][encNo], VERTICAL);   

      
    }else if(!switchHWState[encNo]){
//    }else if(!switchHWState[encNo] && encoder[encNo].switchConfig.action != switchActions::switch_toggle){
      switchInputState[currentBank][encNo] = 0;
//      feedbackHw.Update(FB_ENCODER_SWITCH, encNo, switchInputState[currentBank][encNo], moduleOrientation[mcpNo]);           // aprox 90 us / 4 rings de 16 leds   // 120 us / 8 enc // 200 us / 16 enc       
      feedbackHw.Update(FB_ENCODER_SWITCH, encNo, switchInputState[currentBank][encNo], VERTICAL);         
    }
  }
  
}

void EncoderInputs::EncoderCheck(byte mcpNo, byte encNo){
  // ENCODER CHECK
  uint8_t s = encoderState[currentBank][encNo] & 3;        

  if (mcpState[mcpNo] & (1 << defE41module.encPins[encNo%(defE41module.components.nEncoders)][0])) s |= 4;
  if (mcpState[mcpNo] & (1 << defE41module.encPins[encNo%(defE41module.components.nEncoders)][1])) s |= 8;

  switch (s) {
    case 0: case 5: case 10: case 15:{
      encoderPosition[encNo] = 0;
    }break;
    case 1: case 7: case 8: case 14:{
      encoderPosition[encNo] = 1; encoderChange[encNo] = true; 
    }break;
    case 2: case 4: case 11: case 13:{
      encoderPosition[encNo] = -1; encoderChange[encNo] = true;  
    }break;
//    case 3: case 12:
//      encoderPosition[encNo] = 2; encoderChange[encNo] = true; break;
    default:
//      encoderPosition[encNo] = -2; encoderChange[encNo] = true;
    break;
  }
  encoderState[currentBank][encNo] = (s >> 2);
  
  antMicros = micros();
  if(encoderChange[encNo]){
    encoderChange[encNo] = false;

    if ((encoderPosition[encNo] > 0 && encoderValue[currentBank][encNo] < MAX_ENC_VAL) ||
        (encoderPosition[encNo] < 0 && encoderValue[currentBank][encNo] > 0)){
          
      if (millis() - millisUpdatePrev[encNo] < 10){
        encoderValue[currentBank][encNo] += encoderPosition[encNo];
      }else if (millis() - millisUpdatePrev[encNo] < 30){ 
        if(++pulseCounter[currentBank][encNo] >= 2){            // if movement is slow, count to four, then add
          encoderValue[currentBank][encNo] += encoderPosition[encNo];
          pulseCounter[currentBank][encNo] = 0;
        }
      }else if(++pulseCounter[currentBank][encNo] >= 4){            // if movement is slow, count to four, then add
        encoderValue[currentBank][encNo] += encoderPosition[encNo];
        pulseCounter[currentBank][encNo] = 0;
      }
      encoderValue[currentBank][encNo] = constrain(encoderValue[currentBank][encNo], 0, MAX_ENC_VAL);
    }
    
    IsInPriority(mcpNo);
    
    if(encoderValuePrev[currentBank][encNo] != encoderValue[currentBank][encNo]){      
      // STATUS LED SET BLINK
      feedbackHw.SetStatusLED(1, status_msg_fb);
      
//      SerialUSB.print(F("BANK N° ")); SerialUSB.print(currentBank);
//      SerialUSB.print(F(" Encoder N° ")); SerialUSB.print(encNo);
//      SerialUSB.print(F(" Value: ")); SerialUSB.println(encoderValue[currentBank][encNo]); SerialUSB.println();
        
      MIDI.sendControlChange(encNo+4*currentBank, encoderValue[currentBank][encNo], MIDI_CHANNEL);
      //MIDIHW.sendControlChange(encNo, encoderValue[currentBank][encNo], MIDI_CHANNEL+1);

      encoderValuePrev[currentBank][encNo] = encoderValue[currentBank][encNo];
      millisUpdatePrev[encNo] = millis();

//      feedbackHw.Update(FB_ENCODER, encNo, encoderValue[currentBank][encNo], moduleOrientation[mcpNo]);           // aprox 90 us / 4 rings de 16 leds   // 120 us / 8 enc // 200 us / 16 enc
      feedbackHw.Update(FB_ENCODER, encNo, encoderValue[currentBank][encNo], VERTICAL);           // aprox 90 us / 4 rings de 16 leds   // 120 us / 8 enc // 200 us / 16 enc

    }
  }
  return;
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
//  SerialUSB.print("Address: "); SerialUSB.println(addr);
//  SerialUSB.print("Pin 0: "); SerialUSB.println(addr&1);
//  SerialUSB.print("Pin 1: "); SerialUSB.println((addr>>1)&1);
//  SerialUSB.print("Pin 2: "); SerialUSB.println((addr>>2)&1);
  return;
}

//void ReadEncoders(){
//  static byte priorityCount = 0;
//  static byte priorityList[2] = {};
//  static unsigned long priorityTime = 0;
//
//  
//
//  unsigned int mcpState[N_ENC_MODULES];
//  static unsigned int mcpStatePrev[N_ENC_MODULES];
//
//  for (int n = 0; n < N_ENC_MODULES; n++){
//    mcpState[n] = encodersMCP[n].digitalRead();
//  }
//  
//  for(byte mcpNo = 0; mcpNo < N_ENC_MODULES; mcpNo++){
//    if( mcpState[mcpNo] != mcpStatePrev[mcpNo]){
//      mcpStatePrev[mcpNo] = mcpState[mcpNo]; 
//      
//      // READ N° OF ENCODERS IN ONE MCP
//      for(int n = 0; n < N_ENCODERS_X_MOD; n++){
//        
//        byte encNo = n+mcpNo*N_ENCODERS_X_MOD;
//        int pinState = 0;
//           
//        if (mcpState[mcpNo] & (1<<encoderPins[encNo%N_ENCODERS_X_MOD][0])) pinState |= 1;
//        if (mcpState[mcpNo] & (1<<encoderPins[encNo%N_ENCODERS_X_MOD][1])) pinState |= 2;
////        if(encNo == 3){
////          SerialUSB.print(" State: "); SerialUSB.println(encoderState[currentBank][encNo]&0xF); 
////          SerialUSB.print(" Pin State: "); SerialUSB.println(pinState);
////          
////        }
//         // Determine new state from the pins and state table.
//        encoderState[currentBank][encNo] = pgm_read_byte(&fullStepTable[encoderState[currentBank][encNo] & 0x0f][pinState]);
////        if(encNo == 3){
////          SerialUSB.print(" Next State: "); SerialUSB.println(encoderState[currentBank][encNo]&0xF); 
////          SerialUSB.println();
////          
////        }
//        
//        // Check rotary state
//        switch (encoderState[currentBank][encNo] & 0x30) {
//            case DIR_CW:{
//                encoderPosition[encNo] = 1; encoderChange[encNo] = true;
//            }   break;
//            case DIR_CCW:{
//                encoderPosition[encNo] = -1; encoderChange[encNo] = true;
//            }   break;
//            case DIR_NONE:
//            default:{
//                encoderPosition[encNo] = 0; encoderChange[encNo] = false;
//            }   break;
//        }
//        
//        if(encoderChange[encNo]){
//          encoderChange[encNo] = false;
//          
////          if (millis() - millisUpdatePrev[encNo] < 20){
////            encoderValue[currentBank][encNo] += 3*encoderPosition[encNo];
////          }else if (millis() - millisUpdatePrev[encNo] < 40){ 
////            encoderValue[currentBank][encNo] += 2*encoderPosition[encNo];
////          }else {            // if movement is slow, count to four, then add
//           // encoderValue[currentBank][encNo] += encoderPosition[encNo];
////          }
//          if (millis() - millisUpdatePrev[encNo] < 10){
//           // encoderValue[currentBank][encNo] += 2*encoderPosition[encNo];
//              encoderValue[currentBank][encNo] += encoderPosition[encNo];
//          }else if (millis() - millisUpdatePrev[encNo] < 20){ 
////            encoderValue[currentBank][encNo] += encoderPosition[encNo];
//            if(++pulseCounter[currentBank][encNo] == 2){            // if movement is slow, count to four, then add
//              encoderValue[currentBank][encNo] += encoderPosition[encNo];
//              pulseCounter[currentBank][encNo] = 0;
//            }
//          }else if(++pulseCounter[currentBank][encNo] == 4){            // if movement is slow, count to four, then add
//            encoderValue[currentBank][encNo] += encoderPosition[encNo];
//            pulseCounter[currentBank][encNo] = 0;
//          }
//        
//          // LIMIT TO 0 (lower) AND 127 (upper)
//          if(encoderValue[currentBank][encNo] >= 16384){ 
//            encoderValue[currentBank][encNo] = 0; 
//            encoderPosition[encNo] = 0;
//          }
//          else if(encoderValue[currentBank][encNo] >= MAX_ENC_VAL){
//            encoderValue[currentBank][encNo] = MAX_ENC_VAL; 
//            encoderPosition[encNo] = MAX_ENC_VAL << 2;
//          }
//          
//          if(encoderValuePrev[currentBank][encNo] != encoderValue[currentBank][encNo]){      
////            if (!priorityCount){
////              priorityCount++;
////              priorityList[0] = mcpNo;
////              priorityTime = millis();
////            }
////            else if(priorityCount == 1 && mcpNo != priorityList[0]){
////              priorityCount++;
////              priorityList[1] = mcpNo;
////              priorityTime = millis();
////            }
//            
//            // STATUS LED SET BLINK
//            if< (!flagBlinkStatusLED){
//              flagBlinkStatusLED = 1;
//              midiStatusLED = 1;
//              blinkCountStatusLED = 1;
//            }
//            SerialUSB.print("Encoder N° "); SerialUSB.print(encNo);
//            SerialUSB.print(" Value: "); SerialUSB.println(encoderValue[currentBank][encNo]); SerialUSB.println();
//            SerialUSB.println(" ----------------------------------------------------------- ");
////            MIDI.sendControlChange(encNo, encoderValue[currentBank][encNo], MIDI_CHANNEL);
////            MIDIHW.sendControlChange(encNo, encoderValue[currentBank][encNo], MIDI_CHANNEL+1);
//
//            updated = true;
//            encoderValuePrev[currentBank][encNo] = encoderValue[currentBank][encNo];
//            millisUpdatePrev[encNo] = millis();
//
//           // feedbackHw.Update(encNo, encoderValue[currentBank][encNo]);           // aprox 90 us / 4 rings de 16 leds   // 120 us / 8 enc // 200 us / 16 enc
//
//          }
//        }
//      }
//    }
//  }
//}
