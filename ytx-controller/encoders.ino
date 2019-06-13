
#define DIR_NONE  0x00      //!< No complete step yet
#define DIR_CW    0x10      //!< Clockwise step
#define DIR_CCW   0x20      //!< Counter-clockwise step

// Use the full-step state table, clockwise and counter clockwise
// Emits a code at '00' only
#define RFS_START          0x00     //!< Rotary full step start
#define RFS_CW_FINAL       0x01     //!< Rotary full step clock wise final
#define RFS_CW_BEGIN       0x02     //!< Rotary full step clock begin
#define RFS_CW_NEXT        0x03     //!< Rotary full step clock next
#define RFS_CCW_BEGIN      0x04     //!< Rotary full step counter clockwise begin
#define RFS_CCW_FINAL      0x05     //!< Rotary full step counter clockwise final
#define RFS_CCW_NEXT       0x06     //!< Rotary full step counter clockwise next

static const PROGMEM uint8_t fullStepTable[7][4] = {
    // RFS_START
    {RFS_START,             RFS_CCW_BEGIN | DIR_CCW, RFS_CW_BEGIN | DIR_CW,  RFS_START},   //  0 -1  1  0
    // RFS_CW_FINAL
    {RFS_START | DIR_CW,    RFS_CW_FINAL,  RFS_START,     RFS_CW_NEXT | DIR_CCW},          //  1  0  0 -1 
    // RFS_CW_BEGIN
    {RFS_START | DIR_CCW,   RFS_START,     RFS_CW_BEGIN,  RFS_CW_NEXT | DIR_CW},           // -1  0  0  1
    // RFS_CW_NEXT
    {RFS_START,             RFS_CW_FINAL | DIR_CW,  RFS_CW_BEGIN | DIR_CCW,  RFS_CW_NEXT}, //  0  1 -1  0
    // RFS_CCW_BEGIN
    {RFS_START | DIR_CW,    RFS_CCW_BEGIN, RFS_START,     RFS_CCW_NEXT | DIR_CCW},         //  1  0  0 -1
    // RFS_CCW_FINAL
    {RFS_START | DIR_CCW,   RFS_START,     RFS_CCW_FINAL, RFS_CCW_NEXT | DIR_CW},          // -1  0  0  1
    // RFS_CCW_NEXT
    {RFS_START,             RFS_CCW_BEGIN | DIR_CW, RFS_CCW_FINAL | DIR_CCW, RFS_CCW_NEXT},//  0  1 -1  0
};

//----------------------------------------------------------------------------------------------------
// ENCODER FUNCTIONS
//----------------------------------------------------------------------------------------------------
void InitEncoders(){
  for (int n = 0; n < N_ENC_MODULES; n++){
    encodersMCP[n].begin(&SPI, encodersMCPChipSelect, n);
    for(int i=0; i<16; i++){
      if(i != a0pin && i != a1pin && i != a2pin && i != (a2pin+1)){
        encodersMCP[n].pullUp(i, HIGH);
      }
    }
  }
  // setup the pins using loops, saves coding when you have a lot of encoders and buttons
  for (int n = 0; n < NUM_ENCODERS; n++) {
    encoderPrevValue[n] = 0;  // default state
  }
  for (int n = 0; n < N_ENC_MODULES; n++){
    setNextAddress(encodersMCP[n], n+1);
  }
  SerialUSB.println(F("Encoders init"));
}

//void ReadEncoders(){
//  static byte priorityCount = 0;
//  static byte priorityList[2] = {};
//  static unsigned long priorityTime = 0;
//
//  
//
//  unsigned int mcpState[N_ENC_MODULES];
//  static unsigned int mcpPrevState[N_ENC_MODULES];
//
//  for (int n = 0; n < N_ENC_MODULES; n++){
//    mcpState[n] = encodersMCP[n].digitalRead();
//  }
//  
//  for(byte mcpNo = 0; mcpNo < N_ENC_MODULES; mcpNo++){
//    if( mcpState[mcpNo] != mcpPrevState[mcpNo]){
//      mcpPrevState[mcpNo] = mcpState[mcpNo]; 
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
////          SerialUSB.print(" State: "); SerialUSB.println(encoderState[encNo]&0xF); 
////          SerialUSB.print(" Pin State: "); SerialUSB.println(pinState);
////          
////        }
//         // Determine new state from the pins and state table.
//        encoderState[encNo] = pgm_read_byte(&fullStepTable[encoderState[encNo] & 0x0f][pinState]);
////        if(encNo == 3){
////          SerialUSB.print(" Next State: "); SerialUSB.println(encoderState[encNo]&0xF); 
////          SerialUSB.println();
////          
////        }
//        
//        // Check rotary state
//        switch (encoderState[encNo] & 0x30) {
//            case DIR_CW:{
//                encoderPosition[encNo] = 1; change[encNo] = true;
//            }   break;
//            case DIR_CCW:{
//                encoderPosition[encNo] = -1; change[encNo] = true;
//            }   break;
//            case DIR_NONE:
//            default:{
//                encoderPosition[encNo] = 0; change[encNo] = false;
//            }   break;
//        }
//        
//        if(change[encNo]){
//          change[encNo] = false;
//          
////          if (millis() - antMillisEncoderUpdate[encNo] < 20){
////            encoderValue[encNo] += 3*encoderPosition[encNo];
////          }else if (millis() - antMillisEncoderUpdate[encNo] < 40){ 
////            encoderValue[encNo] += 2*encoderPosition[encNo];
////          }else {            // if movement is slow, count to four, then add
//           // encoderValue[encNo] += encoderPosition[encNo];
////          }
//          if (millis() - antMillisEncoderUpdate[encNo] < 10){
//           // encoderValue[encNo] += 2*encoderPosition[encNo];
//              encoderValue[encNo] += encoderPosition[encNo];
//          }else if (millis() - antMillisEncoderUpdate[encNo] < 20){ 
////            encoderValue[encNo] += encoderPosition[encNo];
//            if(++pulseCounter[encNo] == 2){            // if movement is slow, count to four, then add
//              encoderValue[encNo] += encoderPosition[encNo];
//              pulseCounter[encNo] = 0;
//            }
//          }else if(++pulseCounter[encNo] == 4){            // if movement is slow, count to four, then add
//            encoderValue[encNo] += encoderPosition[encNo];
//            pulseCounter[encNo] = 0;
//          }
//        
//          // LIMIT TO 0 (lower) AND 127 (upper)
//          if(encoderValue[encNo] >= 16384){ 
//            encoderValue[encNo] = 0; 
//            encoderPosition[encNo] = 0;
//          }
//          else if(encoderValue[encNo] >= MAX_ENC_VAL){
//            encoderValue[encNo] = MAX_ENC_VAL; 
//            encoderPosition[encNo] = MAX_ENC_VAL << 2;
//          }
//          
//          if(encoderPrevValue[encNo] != encoderValue[encNo]){      
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
//            if(!flagBlinkStatusLED){
//              flagBlinkStatusLED = 1;
//              midiStatusLED = 1;
//              blinkCountStatusLED = 1;
//            }
//            SerialUSB.print("Encoder N° "); SerialUSB.print(encNo);
//            SerialUSB.print(" Value: "); SerialUSB.println(encoderValue[encNo]); SerialUSB.println();
//            SerialUSB.println(" ----------------------------------------------------------- ");
////            MIDI.sendControlChange(encNo, encoderValue[encNo], MIDI_CHANNEL);
////            MIDIHW.sendControlChange(encNo, encoderValue[encNo], MIDI_CHANNEL+1);
//
//            updated = true;
//            encoderPrevValue[encNo] = encoderValue[encNo];
//            antMillisEncoderUpdate[encNo] = millis();
//
//           // UpdateLeds(encNo, encoderValue[encNo]);           // aprox 90 us / 4 rings de 16 leds   // 120 us / 8 enc // 200 us / 16 enc
//
//          }
//        }
//      }
//    }
//  }
//}

void ReadEncoders(){
  static byte priorityCount = 0;
  static byte priorityList[2] = {};
  static unsigned long priorityTime = 0;

  unsigned int mcpState[N_ENC_MODULES];
  static unsigned int mcpPrevState[N_ENC_MODULES];

  if(priorityCount && (millis()-priorityTime > 500)){
    priorityCount = 0;
  }

//  if(priorityCount >= 1){
//    byte priorityModule = priorityList[0];
//    mcpState[priorityModule] = encodersMCP[priorityModule].digitalRead();
//  }
//  if(priorityCount == 2){
//    byte priorityModule = priorityList[1];
//    mcpState[priorityModule] = encodersMCP[priorityModule].digitalRead();
//  }else{
    for (int n = 0; n < N_ENC_MODULES; n++){
      mcpState[n] = encodersMCP[n].digitalRead();
    }
//  }
//  SerialUSB.print(mcpState[0],BIN); SerialUSB.println(" ");
//  SerialUSB.print(mcpState[1],BIN); SerialUSB.print(" ");
//  SerialUSB.print(mcpState[2],BIN); SerialUSB.print(" ");
//  SerialUSB.println(mcpState[3],BIN);
  
  for(byte mcpNo = 0; mcpNo < N_ENC_MODULES; mcpNo++){
    if( mcpState[mcpNo] != mcpPrevState[mcpNo]){
      mcpPrevState[mcpNo] = mcpState[mcpNo]; 
      
      // READ N° OF ENCODERS IN ONE MCP
      for(int n = 0; n < N_ENCODERS_X_MOD; n++){
        byte encNo = n+mcpNo*N_ENCODERS_X_MOD;
       
        uint8_t s = encoderState[encNo] & 3;
        
//        SerialUSB.println(encoderState[encNo],BIN);
//        SerialUSB.println();
        
        if (mcpState[mcpNo] & (1<<encoderPins[encNo%N_ENCODERS_X_MOD][0])) s |= 4;
        if (mcpState[mcpNo] & (1<<encoderPins[encNo%N_ENCODERS_X_MOD][1])) s |= 8;
      
        switch (s) {
          case 0: case 5: case 10: case 15:{
            encoderPosition[encNo] = 0;
          }break;
          case 1: case 7: case 8: case 14:{
            encoderPosition[encNo] = 1; change[encNo] = true; 
          }break;
          case 2: case 4: case 11: case 13:{
            encoderPosition[encNo] = -1; change[encNo] = true;  
          }break;
//          case 3: case 12:
//            encoderPosition[encNo] = 2; change[encNo] = true; break;
          default:
//            encoderPosition[encNo] = -2; change[encNo] = true;
          break;
        }
        encoderState[encNo] = (s >> 2);
        
        if(change[encNo]){
//          SerialUSB.print(encNo,DEC); SerialUSB.print(" ");
//          SerialUSB.print(mcpState[mcpNo],BIN); SerialUSB.print(" ");
//          SerialUSB.print(s,BIN); SerialUSB.print(" ");
//          SerialUSB.print("   Pos: "); SerialUSB.print(encoderPosition[encNo]);
//          SerialUSB.print(" Dif: "); SerialUSB.println(abs(encoderPosition[encNo]-encoderPrevPosition[encNo]));
          change[encNo] = false;
         
          if (millis() - antMillisEncoderUpdate[encNo] < 10){
           // encoderValue[encNo] += 2*encoderPosition[encNo];
              encoderValue[encNo] += encoderPosition[encNo];
          }else if (millis() - antMillisEncoderUpdate[encNo] < 20){ 
//            encoderValue[encNo] += encoderPosition[encNo];
            if(++pulseCounter[encNo] == 2){            // if movement is slow, count to four, then add
              encoderValue[encNo] += encoderPosition[encNo];
              pulseCounter[encNo] = 0;
            }
          }else if(++pulseCounter[encNo] == 4){            // if movement is slow, count to four, then add
            encoderValue[encNo] += encoderPosition[encNo];
            pulseCounter[encNo] = 0;
          }
          
          // LIMIT TO 0 (lower) AND 127 (upper)
          if(encoderValue[encNo] >= 16384){ 
            encoderValue[encNo] = 0; 
            encoderPosition[encNo] = 0;
          }
          else if(encoderValue[encNo] >= MAX_ENC_VAL){
            encoderValue[encNo] = MAX_ENC_VAL; 
            encoderPosition[encNo] = MAX_ENC_VAL << 2;
          }
          
          if(encoderPrevValue[encNo] != encoderValue[encNo]){      
//            if (!priorityCount){
//              priorityCount++;
//              priorityList[0] = mcpNo;
//              priorityTime = millis();
//            }
//            else if(priorityCount == 1 && mcpNo != priorityList[0]){
//              priorityCount++;
//              priorityList[1] = mcpNo;
//              priorityTime = millis();
//            }
            
            // STATUS LED SET BLINK
            if(!flagBlinkStatusLED){
              flagBlinkStatusLED = 1;
              midiStatusLED = 1;
              blinkCountStatusLED = 1;
            }
            SerialUSB.print("Encoder N° "); SerialUSB.print(encNo);
            SerialUSB.print(" Value: "); SerialUSB.println(encoderValue[encNo]); SerialUSB.println();

//            MIDI.sendControlChange(encNo, encoderValue[encNo], MIDI_CHANNEL);
//            MIDIHW.sendControlChange(encNo, encoderValue[encNo], MIDI_CHANNEL+1);

            updated = true;
            encoderPrevValue[encNo] = encoderValue[encNo];
            antMillisEncoderUpdate[encNo] = millis();

            UpdateLeds(encNo, encoderValue[encNo]);           // aprox 90 us / 4 rings de 16 leds   // 120 us / 8 enc // 200 us / 16 enc

          }
        }
      }
    }
  }
}
