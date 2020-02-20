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

#include "headers/FeedbackClass.h"

//----------------------------------------------------------------------------------------------------
// FEEDBACK FUNCTIONS
//----------------------------------------------------------------------------------------------------
void FeedbackClass::Init(uint8_t maxBanks, uint8_t maxEncoders, uint8_t maxDigital, uint8_t maxIndependent) {
  nBanks = maxBanks;
  nEncoders = maxEncoders;
  nDigitals = maxDigital;
  nIndependent = maxIndependent;

  if(!nBanks) return;    // If number of encoders is zero, return;
  
  feedbackUpdateWriteIdx = 0;
  feedbackUpdateReadIdx = 0;

  for(int f = 0; f < FEEDBACK_UPDATE_BUFFER_SIZE; f++){
    feedbackUpdateBuffer[f].indexChanged = 0;
    feedbackUpdateBuffer[f].newValue = 0;
    feedbackUpdateBuffer[f].newOrientation = 0;
  }
  
  flagBlinkStatusLED = 0;
  blinkCountStatusLED = 0;
  statusLEDfbType = 0;
  lastStatusLEDState = LOW;
  millisStatusPrev = 0;
    
  // First dimension is an array of pointers, each pointing to a column - https://www.eskimo.com/~scs/cclass/int/sx9b.html

  // ESTO ADELANTE DEL MALLOC DE LOS ENCODERS HACE QUE FUNCIONEN LOS ENCODERS PARA 1 BANCO CONFIGURADO
  // EL PRIMER PUNTERO QUE LLAMA A MALLOC, CAMBIA LA DIRECCIÓN A LA QUE APUNTA DESPUÉS DE LA INICIALIZACIÓN,
  // EN LA SEGUNDA VUELTA DE LOOP, SOLO CUANDO HAY 1 BANCO CONFIGURADO
  if(nDigitals){
    digFbData = (digFeedbackData**) memHost->AllocateRAM(nBanks*sizeof(digFeedbackData*));
  }
  if(nEncoders){
    encFbData = (encFeedbackData**) memHost->AllocateRAM(nBanks*sizeof(encFeedbackData*));
  }
  
  for (int b = 0; b < nBanks; b++) {
    if(nEncoders){
      encFbData[b] = (encFeedbackData*) memHost->AllocateRAM(nEncoders*sizeof(encFeedbackData));
      for (int e = 0; e < nEncoders; e++) {
        encFbData[b][e].encRingState = 0;
        encFbData[b][e].encRingStatePrev = 0;
  //      encFbData[b][e].ringStateIndex = 0;
        encFbData[b][e].colorIndexPrev = 0;
        encFbData[b][e].switchFbValue = 0;
      }
    }
    if(nDigitals){
      digFbData[b] = (digFeedbackData*) memHost->AllocateRAM(nDigitals*sizeof(digFeedbackData));
      for (int d = 0; d < nDigitals; d++) {
        digFbData[b][d].digitalFbValue = 0;
        digFbData[b][d].colorIndexPrev = 0;
      }
    }
  } 
}

void FeedbackClass::InitPower(){
  // POWER MANAGEMENT - READ FROM POWER PIN, IF POWER SUPPLY IS PRESENT AND SET LED BRIGHTNESS ACCORDINGLY
  feedbackHw.SendCommand(CMD_ALL_LEDS_OFF);
  delay(10);
  
  bool okToContinue = false;
  byte initFrameIndex = 0;
  
  if(digitalRead(pinExternalVoltage)){
    if(nEncoders >= 28)  currentBrightness = BRIGHTNESS_WO_POWER-15;
    else                 currentBrightness = BRIGHTNESS_WO_POWER;
  }else{
    currentBrightness = BRIGHTNESS_WITH_POWER;
  }
  // Set External ISR for the power adapter detector pin
  attachInterrupt(digitalPinToInterrupt(pinExternalVoltage), ChangeBrigthnessISR, CHANGE);
  
  // SEND INITIAL VALUES AND LED BRIGHTNESS TO SAMD11
  #define INIT_FRAME_SIZE 7
  byte initFrameArray[INIT_FRAME_SIZE] = {INIT_VALUES, 
                                          nEncoders,
                                          nIndependent,   // CHANGE TO AMOUNT OF ANALOG WITH FEEDBACK
                                          amountOfDigitalInConfig[0],
                                          amountOfDigitalInConfig[1],
                                          currentBrightness,
                                          config->board.rainbowOn};
  config->board.rainbowOn = 1;                                            
  byte bootFlagState = 0;
  eep.read(BOOT_SIGN_ADDR, (byte *) &bootFlagState, sizeof(bootFlagState));
  bootFlagState &= (~(1<<7));
  eep.write(BOOT_SIGN_ADDR, (byte *) &bootFlagState, sizeof(bootFlagState));
                            
  do{
//    SerialUSB.println("INIT SAMD11");
//    SerialUSB.println(initFrameArray[initFrameIndex]);
    SendCommand(initFrameArray[initFrameIndex++]); 
     
    if(initFrameIndex == INIT_FRAME_SIZE) okToContinue = true;
//    if(Serial.available()){
//      SerialUSB.print("Index: ");SerialUSB.println(initFrameIndex);
//      byte ack = Serial.read();
//      SerialUSB.print("ACK: ");SerialUSB.println(ack);
//      if(ack == initFrameArray[initFrameIndex]){
//        if(initFrameIndex >= 4)  
//          okToContinue = true;
//        else
//          initFrameIndex++;
//      }
//      
//    }else{
//      SerialUSB.println("no serial data");
//      delay(3);
//    }
  }while(!okToContinue);

//  while(1){
//    Rainbow(&statusLED, 20);
//  }

  begun = true;
}

void FeedbackClass::Update() {
//  static byte count = 0;
  if(!begun) return;    // If didn't go through INIT, return;
  
  while (feedbackUpdateReadIdx != feedbackUpdateWriteIdx  && !showInProgress) {  
    if((feedbackUpdateWriteIdx - feedbackUpdateReadIdx) > 1 && !burst){
      burst = true;
//      SerialUSB.println("BURST MODE ON");
    }
      
    byte fbUpdateType = feedbackUpdateBuffer[feedbackUpdateReadIdx].type;
    byte fbUpdateQueueIndex = feedbackUpdateReadIdx;
    
    if(++feedbackUpdateReadIdx >= FEEDBACK_UPDATE_BUFFER_SIZE)  
      feedbackUpdateReadIdx = 0;
      
    switch(fbUpdateType){
      case FB_ENCODER:
      case FB_2CC:
      case FB_ENCODER_SWITCH:{
        FillFrameWithEncoderData(fbUpdateQueueIndex);
        SendDataIfReady();
      }break;
      case FB_DIGITAL:{
        FillFrameWithDigitalData(fbUpdateQueueIndex);
        SendDataIfReady();

      }break;
      case FB_ANALOG:{
        
      }break;
      case FB_INDEPENDENT:{
        
      }break;
      case FB_BANK_CHANGED:{
        // 9ms para cambiar el banco - 32 encoders, 0 dig, 0 analog - 16/7/2009
        unsigned long antMicrosBank = micros();
        
        //feedbackHw.SendCommand(CMD_ALL_LEDS_OFF);
        feedbackHw.SendCommand(BANK_INIT);
        for(uint8_t n = 0; n < nEncoders; n++){
          SetChangeEncoderFeedback(FB_ENCODER, n, encoderHw.GetEncoderValue(n), 
                                                  encoderHw.GetModuleOrientation(n/4), false);   // HARDCODE: N° of encoders in module / is 
          if(encoder[n].switchConfig.mode == switchModes::switch_mode_2cc){
            SetChangeEncoderFeedback(FB_2CC, n, encoderHw.GetEncoderValue2(n), 
                                                encoderHw.GetModuleOrientation(n/4), false);   // HARDCODE: N° of encoders in module / is                                                   
          }
        }
        for(uint8_t n = 0; n < nEncoders; n++){
          SetChangeEncoderFeedback(FB_ENCODER_SWITCH, n, encoderHw.GetEncoderSwitchValue(n), 
                                                         encoderHw.GetModuleOrientation(n/4), false);  // HARDCODE: N° of encoders in module                                                
        }
        
        for(uint8_t n = 0; n < nDigitals; n++){
          SetChangeDigitalFeedback(n, digitalHw.GetDigitalValue(n), digitalHw.GetDigitalState(n), false);
        }

        if(config->banks.count > 1){    // If there is more than one bank
          for(int bank = 0; bank < config->banks.count; bank++){
            byte bankShifterIndex = config->banks.shifterId[bank];
            if(currentBank == bank){
              if(bankShifterIndex < (config->inputs.encoderCount - 1)){
  //              SerialUSB.print("FB SWITCH "); 
  //              SerialUSB.print(bankShifterIndex); 
  //              SerialUSB.print(" BANK ON: ");
                
                SetChangeEncoderFeedback(FB_ENCODER_SWITCH, bankShifterIndex, true, encoderHw.GetModuleOrientation(bankShifterIndex/4), true);  // HARDCODE: N° of encoders in module     
              }else{
                // DIGITAL
                SetChangeDigitalFeedback(bankShifterIndex - (config->inputs.encoderCount), true, true, true);
              }
            }else{
              if(bankShifterIndex < (config->inputs.encoderCount - 1)){
  //              SerialUSB.println("FB SWITCH BANK OFF");
                SetChangeEncoderFeedback(FB_ENCODER_SWITCH, bankShifterIndex, false, encoderHw.GetModuleOrientation(bankShifterIndex/4), true);  // HARDCODE: N° of encoders in module     
              }else{
                // DIGITAL
                SetChangeDigitalFeedback(bankShifterIndex - config->inputs.encoderCount, false, false, true);
              }
            }
          }
        }
        feedbackHw.SendCommand(BANK_END);
//        SerialUSB.println("******************************************");
        updatingBankFeedback = false;
//        SerialUSB.print(feedbackUpdateReadIdx);
//        SerialUSB.print(" F - ");
//        SerialUSB.println(micros()-antMicrosBank);
//        SerialUSB.println("******************************************");
      }break;
      default: break;
    }
    
  }
  
  
}

void FeedbackClass::FillFrameWithEncoderData(byte updateIndex){
//  SerialUSB.println(indexChanged);
  // FIX FOR SHIFT ROTARY ACTION
  uint8_t colorR = 0, colorG = 0, colorB = 0;
  uint8_t colorIndex = 0;
  bool colorIndexChanged = false;
  uint8_t ringStateIndex= false;
  bool encoderSwitchChanged = false;
  bool isActionShifted = false;
  bool is2cc = false;
  //bool isFineAdjOn = encoderHw.IsFineAdj(indexChanged);
  uint16_t minValue = 0, maxValue = 0;
  uint8_t msgType = 0;
  bool is14bits = false;
  
  uint8_t indexChanged = feedbackUpdateBuffer[updateIndex].indexChanged;
  uint8_t newOrientation = feedbackUpdateBuffer[updateIndex].newOrientation;
  uint16_t newValue = feedbackUpdateBuffer[updateIndex].newValue;
  uint8_t fbUpdateType = feedbackUpdateBuffer[updateIndex].type;
  bool isBank = feedbackUpdateBuffer[updateIndex].isBank;

  // Get state for alternate switch functions
  isActionShifted = encoderHw.IsShiftActionOn(indexChanged);
  is2cc = (fbUpdateType == FB_2CC);
    
  // Get config info for this encoder
  if(isActionShifted || is2cc){ // If encoder is shifted
    minValue = encoder[indexChanged].switchConfig.parameter[switch_minValue_MSB]<<7 | encoder[indexChanged].switchConfig.parameter[switch_minValue_LSB];
    maxValue = encoder[indexChanged].switchConfig.parameter[switch_maxValue_MSB]<<7 | encoder[indexChanged].switchConfig.parameter[switch_maxValue_LSB];
    msgType = encoder[indexChanged].switchConfig.message;
  }else{
    minValue = encoder[indexChanged].rotaryConfig.parameter[rotary_minMSB]<<7 | encoder[indexChanged].rotaryConfig.parameter[rotary_minLSB];
    maxValue = encoder[indexChanged].rotaryConfig.parameter[rotary_maxMSB]<<7 | encoder[indexChanged].rotaryConfig.parameter[rotary_maxLSB];
    msgType = encoder[indexChanged].rotaryConfig.message;
  }
  
  bool switchState = (newValue == minValue) ? 0 : 1 ;   // if new value is minValue, state of LED is OFF, otherwise is ON

  // IF NOT 14 BITS, USE LOWER PART FOR MIN AND MAX
  if( msgType == rotary_msg_nrpn || msgType == rotary_msg_rpn || msgType == rotary_msg_pb || 
      msgType == switch_msg_nrpn || msgType == switch_msg_rpn || msgType == switch_msg_pb){
    is14bits = true;      
  }else{
    minValue = minValue & 0x7F;
    maxValue = maxValue & 0x7F;
  }

  bool invert = false;
  if(minValue > maxValue){
    invert = true;
  }
  
  if(fbUpdateType == FB_ENCODER){
    switch(encoder[indexChanged].rotaryFeedback.mode){
      case encoderRotaryFeedbackMode::fb_walk: {
        ringStateIndex = mapl(newValue, 
                              minValue, 
                              maxValue, 
                              invert ? WALK_SIZE - 1 : 0, 
                              invert ? 0 : WALK_SIZE - 1);
        // If the whole range is smaller or equal to the amount of LEDs, the walk only shows 1 LED walking
        if (abs(maxValue - minValue) <= WALK_SIZE/2){
          if(!(ringStateIndex%2)  && ringStateIndex != 0){
            if(invert) ringStateIndex--;
            else       ringStateIndex++;
          }
        }                              
//        SerialUSB.print("\tRS INDEX: ");SerialUSB.println(ringStateIndex);                                                                  
        encFbData[currentBank][indexChanged].encRingState &= newOrientation ? ENCODER_SWITCH_V_ON : ENCODER_SWITCH_H_ON;
        encFbData[currentBank][indexChanged].encRingState |= pgm_read_word(&walk[newOrientation][ringStateIndex]);
      }
      break;
      case encoderRotaryFeedbackMode::fb_fill: {
        ringStateIndex = mapl(newValue, 
                              minValue, 
                              maxValue, 
                              invert ? FILL_SIZE - 1 : 0, 
                              invert ? 0 : FILL_SIZE - 1);
        encFbData[currentBank][indexChanged].encRingState &= newOrientation ? ENCODER_SWITCH_V_ON : ENCODER_SWITCH_H_ON;
        encFbData[currentBank][indexChanged].encRingState |= pgm_read_word(&fill[newOrientation][ringStateIndex]);
      }
      break;
      case encoderRotaryFeedbackMode::fb_eq: {
        ringStateIndex = mapl(newValue, 
                              minValue, 
                              maxValue, 
                              invert ? EQ_SIZE - 1 : 0, 
                              invert ? 0 : EQ_SIZE - 1);
        encFbData[currentBank][indexChanged].encRingState &= newOrientation ? ENCODER_SWITCH_V_ON : ENCODER_SWITCH_H_ON;
        encFbData[currentBank][indexChanged].encRingState |= pgm_read_word(&eq[newOrientation][ringStateIndex]);
      }
      break;
      case encoderRotaryFeedbackMode::fb_spread: {
        ringStateIndex = mapl(newValue, 
                              minValue, 
                              maxValue, 
                              invert ? SPREAD_SIZE - 1 : 0, 
                              invert ? 0 : SPREAD_SIZE - 1);
        encFbData[currentBank][indexChanged].encRingState &= newOrientation ? ENCODER_SWITCH_V_ON : ENCODER_SWITCH_H_ON;
        encFbData[currentBank][indexChanged].encRingState |= pgm_read_word(&spread[newOrientation][ringStateIndex]);
      }
      break;
      default: break;
    }
    colorR = pgm_read_byte(&gamma8[encoder[indexChanged].rotaryFeedback.color[R_INDEX]]);
    colorG = pgm_read_byte(&gamma8[encoder[indexChanged].rotaryFeedback.color[G_INDEX]]);
    colorB = pgm_read_byte(&gamma8[encoder[indexChanged].rotaryFeedback.color[B_INDEX]]);
  }else if (fbUpdateType == FB_2CC){  // Feedback for double CC 
//    SerialUSB.print("2CC ");SerialUSB.println(invert ? "INVERTED" : "NOT INVERTED");
    ringStateIndex = mapl(newValue, 
                          minValue, 
                          maxValue, 
                          0, 
                          S_WALK_SIZE - 1);
                               
        SerialUSB.print("\tRS INDEX: ");SerialUSB.println(ringStateIndex);                                                                  
    encFbData[currentBank][indexChanged].encRingState &= newOrientation ? ENCODER_SWITCH_V_ON : ENCODER_SWITCH_H_ON;
    if(invert)  encFbData[currentBank][indexChanged].encRingState |= pgm_read_word(&simpleWalkInv[newOrientation][ringStateIndex]);
    else        encFbData[currentBank][indexChanged].encRingState |= pgm_read_word(&simpleWalk[newOrientation][ringStateIndex]);
    
    colorR = 255-pgm_read_byte(&gamma8[encoder[indexChanged].rotaryFeedback.color[R_INDEX]]);
    colorG = 255-pgm_read_byte(&gamma8[encoder[indexChanged].rotaryFeedback.color[G_INDEX]]);
    colorB = 255-pgm_read_byte(&gamma8[encoder[indexChanged].rotaryFeedback.color[B_INDEX]]);
  }else if (fbUpdateType == FB_ENCODER_SWITCH){  // Feedback for encoder switch  
    if(encoder[indexChanged].switchFeedback.colorRangeEnable && !isBank){
      encFbData[currentBank][indexChanged].encRingState |= (newOrientation ? ENCODER_SWITCH_V_ON : ENCODER_SWITCH_H_ON);
      
      if      (!newValue)                                              colorIndex = encoder[indexChanged].switchFeedback.colorRange0;    // VALUE: 0
      else if (newValue > COLOR_RANGE_0 && newValue <= COLOR_RANGE_1)  colorIndex = encoder[indexChanged].switchFeedback.colorRange1;    // VALUE: 1-3
      else if (newValue > COLOR_RANGE_1 && newValue <= COLOR_RANGE_2)  colorIndex = encoder[indexChanged].switchFeedback.colorRange2;    // VALUE: 4-7
      else if (newValue > COLOR_RANGE_2 && newValue <= COLOR_RANGE_3)  colorIndex = encoder[indexChanged].switchFeedback.colorRange3;    // VALUE: 8-15
      else if (newValue > COLOR_RANGE_3 && newValue <= COLOR_RANGE_4)  colorIndex = encoder[indexChanged].switchFeedback.colorRange4;    // VALUE: 16-31
      else if (newValue > COLOR_RANGE_4 && newValue <= COLOR_RANGE_5)  colorIndex = encoder[indexChanged].switchFeedback.colorRange5;    // VALUE: 32-63
      else if (newValue > COLOR_RANGE_5 && newValue <= COLOR_RANGE_6)  colorIndex = encoder[indexChanged].switchFeedback.colorRange6;    // VALUE: 64-126
      else if (newValue == COLOR_RANGE_7)                              colorIndex = encoder[indexChanged].switchFeedback.colorRange7;    // VALUE: 127

//      SerialUSB.print("index: "); SerialUSB.print(indexChanged);  SerialUSB.print(" newValue: "); SerialUSB.print(newValue);  SerialUSB.print(" colorIndex: ");  SerialUSB.println(colorIndex);
      if(colorIndex != encFbData[currentBank][indexChanged].colorIndexPrev){
        colorIndexChanged = true;
        encFbData[currentBank][indexChanged].colorIndexPrev = colorIndex;
        colorR = pgm_read_byte(&gamma8[colorRangeTable[colorIndex][R_INDEX]]);
        colorG = pgm_read_byte(&gamma8[colorRangeTable[colorIndex][G_INDEX]]);
        colorB = pgm_read_byte(&gamma8[colorRangeTable[colorIndex][B_INDEX]]);
//        colorR = colorRangeTable[colorIndex][R_INDEX];
//        colorG = colorRangeTable[colorIndex][G_INDEX];
//        colorB = colorRangeTable[colorIndex][B_INDEX];
      }
//      else{
//        colorR = pgm_read_byte(&gamma8[colorRangeTable[colorIndex][R_INDEX]]);
//        colorG = pgm_read_byte(&gamma8[colorRangeTable[colorIndex][G_INDEX]]);
//        colorB = pgm_read_byte(&gamma8[colorRangeTable[colorIndex][B_INDEX]]);
//      }
    }else{   
      if((switchState && !isBank)){
        encFbData[currentBank][indexChanged].encRingState |= (newOrientation ? ENCODER_SWITCH_V_ON : ENCODER_SWITCH_H_ON);
        colorR = pgm_read_byte(&gamma8[encoder[indexChanged].switchFeedback.color[R_INDEX]]);
        colorG = pgm_read_byte(&gamma8[encoder[indexChanged].switchFeedback.color[G_INDEX]]);
        colorB = pgm_read_byte(&gamma8[encoder[indexChanged].switchFeedback.color[B_INDEX]]);    
      }else if(switchState && isBank){ // if it is a bank selector, color is gray
        encFbData[currentBank][indexChanged].encRingState |= (newOrientation ? ENCODER_SWITCH_V_ON : ENCODER_SWITCH_H_ON);    
        colorR = pgm_read_byte(&gamma8[encoder[indexChanged].switchFeedback.color[R_INDEX]]);
        colorG = pgm_read_byte(&gamma8[encoder[indexChanged].switchFeedback.color[G_INDEX]]);
        colorB = pgm_read_byte(&gamma8[encoder[indexChanged].switchFeedback.color[B_INDEX]]);  
      }else if(!switchState && isBank){
        encFbData[currentBank][indexChanged].encRingState |= (newOrientation ? ENCODER_SWITCH_V_ON : ENCODER_SWITCH_H_ON);    // If it's a bank shifter, switch LED's are on
        colorR = pgm_read_byte(&gamma8[encoder[indexChanged].switchFeedback.color[R_INDEX]*BANK_OFF_BRIGHTNESS_FACTOR]);      // and dimmer
        colorG = pgm_read_byte(&gamma8[encoder[indexChanged].switchFeedback.color[G_INDEX]*BANK_OFF_BRIGHTNESS_FACTOR]);
        colorB = pgm_read_byte(&gamma8[encoder[indexChanged].switchFeedback.color[B_INDEX]*BANK_OFF_BRIGHTNESS_FACTOR]); 
      }else{
        encFbData[currentBank][indexChanged].encRingState &= ~(newOrientation ? ENCODER_SWITCH_V_ON : ENCODER_SWITCH_H_ON);
        colorR = 0;
        colorG = 0;
        colorB = 0;
      } 
      encoderSwitchChanged = true;
    }
  }

  if (fbUpdateType == FB_ENCODER || fbUpdateType == FB_2CC
        || updatingBankFeedback 
        || colorIndexChanged
        || encoderSwitchChanged) {
//    SerialUSB.print("PREV STATE: ");SerialUSB.println(encFbData[currentBank][indexChanged].encRingStatePrev,BIN);
    
    encFbData[currentBank][indexChanged].encRingStatePrev = encFbData[currentBank][indexChanged].encRingState;

//    SerialUSB.print("CurrentBank: ");SerialUSB.println(currentBank);
//    SerialUSB.print("IndexChanged: ");SerialUSB.println(indexChanged);
//    SerialUSB.print("CURR STATE: ");SerialUSB.println(encFbData[currentBank][indexChanged].encRingState,BIN);
//    SerialUSB.print("PREV STATE: ");SerialUSB.println(encFbData[currentBank][indexChanged].encRingStatePrev,BIN);
//    SerialUSB.println();

    //sendSerialBufferDec[msgLength] = TX_BYTES;   // INIT SERIAL FRAME WITH CONSTANT DATA
    sendSerialBufferDec[d_frameType] =  fbUpdateType == FB_ENCODER        ? ENCODER_CHANGE_FRAME  :
                                        fbUpdateType == FB_2CC            ? ENCODER_DOUBLE_FRAME  : 
                                        fbUpdateType == FB_ENCODER_SWITCH ? ENCODER_SWITCH_CHANGE_FRAME : 255;   
    sendSerialBufferDec[d_nRing] = indexChanged;
    sendSerialBufferDec[d_orientation] = newOrientation;
    sendSerialBufferDec[d_ringStateH] = encFbData[currentBank][indexChanged].encRingState >> 8;
    sendSerialBufferDec[d_ringStateL] = encFbData[currentBank][indexChanged].encRingState & 0xff;
    sendSerialBufferDec[d_currentValue] = newValue;
    sendSerialBufferDec[d_fbMin] = minValue;
    sendSerialBufferDec[d_fbMax] = maxValue;
    sendSerialBufferDec[d_R] = colorR;
    sendSerialBufferDec[d_G] = colorG;
    sendSerialBufferDec[d_B] = colorB;
    sendSerialBufferDec[d_ENDOFFRAME] = END_OF_FRAME_BYTE;
    feedbackDataToSend = true;
  }
}

void FeedbackClass::FillFrameWithDigitalData(byte updateIndex){
  uint8_t colorR = 0, colorG = 0, colorB = 0;
  bool colorIndexChanged = false;
  uint8_t colorIndex = 0; 
  
  uint8_t indexChanged = feedbackUpdateBuffer[updateIndex].indexChanged;
  uint16_t newValue = feedbackUpdateBuffer[updateIndex].newValue;
  uint8_t fbUpdateType = feedbackUpdateBuffer[updateIndex].type;
  bool isBank = feedbackUpdateBuffer[updateIndex].isBank;
  bool newState = feedbackUpdateBuffer[updateIndex].newOrientation;
  
  if(digital[indexChanged].feedback.colorRangeEnable && !isBank){
    if      (!newValue)                                              colorIndex = digital[indexChanged].feedback.colorRange0;   // VALUE: 0
    else if (newValue > COLOR_RANGE_0 && newValue <= COLOR_RANGE_1)  colorIndex = digital[indexChanged].feedback.colorRange1;   // VALUE: 1-3
    else if (newValue > COLOR_RANGE_1 && newValue <= COLOR_RANGE_2)  colorIndex = digital[indexChanged].feedback.colorRange2;   // VALUE: 4-7
    else if (newValue > COLOR_RANGE_2 && newValue <= COLOR_RANGE_3)  colorIndex = digital[indexChanged].feedback.colorRange3;   // VALUE: 8-15
    else if (newValue > COLOR_RANGE_3 && newValue <= COLOR_RANGE_4)  colorIndex = digital[indexChanged].feedback.colorRange4;   // VALUE: 16-31
    else if (newValue > COLOR_RANGE_4 && newValue <= COLOR_RANGE_5)  colorIndex = digital[indexChanged].feedback.colorRange5;   // VALUE: 32-63
    else if (newValue > COLOR_RANGE_5 && newValue <= COLOR_RANGE_6)  colorIndex = digital[indexChanged].feedback.colorRange6;   // VALUE: 64-126
    else if (newValue == COLOR_RANGE_7)                              colorIndex = digital[indexChanged].feedback.colorRange7;   // VALUE: 127
  
    colorR = pgm_read_byte(&gamma8[colorRangeTable[colorIndex][R_INDEX]]);
    colorG = pgm_read_byte(&gamma8[colorRangeTable[colorIndex][G_INDEX]]);
    colorB = pgm_read_byte(&gamma8[colorRangeTable[colorIndex][B_INDEX]]);
  }else{     
    if((newState && !isBank)){
      colorR = pgm_read_byte(&gamma8[digital[indexChanged].feedback.color[R_INDEX]]);
      colorG = pgm_read_byte(&gamma8[digital[indexChanged].feedback.color[G_INDEX]]);
      colorB = pgm_read_byte(&gamma8[digital[indexChanged].feedback.color[B_INDEX]]);   
    }else if(!newState && !isBank){
      colorR = 0;
      colorG = 0;
      colorB = 0;
    }else if(newState && isBank){
      colorR = pgm_read_byte(&gamma8[digital[indexChanged].feedback.color[R_INDEX]]);
      colorG = pgm_read_byte(&gamma8[digital[indexChanged].feedback.color[G_INDEX]]);
      colorB = pgm_read_byte(&gamma8[digital[indexChanged].feedback.color[B_INDEX]]);
    }else if(!newState && isBank){
      colorR = pgm_read_byte(&gamma8[digital[indexChanged].feedback.color[R_INDEX]*BANK_OFF_BRIGHTNESS_FACTOR]);
      colorG = pgm_read_byte(&gamma8[digital[indexChanged].feedback.color[G_INDEX]*BANK_OFF_BRIGHTNESS_FACTOR]);
      colorB = pgm_read_byte(&gamma8[digital[indexChanged].feedback.color[B_INDEX]*BANK_OFF_BRIGHTNESS_FACTOR]);
    }
  }
  
  //sendSerialBufferDec[msgLength] = TX_BYTES;   // INIT SERIAL FRAME WITH CONSTANT DATA
  sendSerialBufferDec[d_frameType] = (indexChanged < amountOfDigitalInConfig[0]) ?  DIGITAL1_CHANGE_FRAME : 
                                                                                    DIGITAL2_CHANGE_FRAME;   
  sendSerialBufferDec[d_nDig] = indexChanged;
  sendSerialBufferDec[d_orientation] = 0;
  sendSerialBufferDec[d_digitalState] = isBank ? 1 : digFbData[currentBank][indexChanged].digitalFbValue;
  sendSerialBufferDec[d_ringStateL] = 0;
  sendSerialBufferDec[d_R] = colorR;
  sendSerialBufferDec[d_G] = colorG;
  sendSerialBufferDec[d_B] = colorB;
  sendSerialBufferDec[d_ENDOFFRAME] = END_OF_FRAME_BYTE;
  feedbackDataToSend = true;
}


void FeedbackClass::SetChangeEncoderFeedback(uint8_t type, uint8_t encIndex, uint16_t val, uint8_t encoderOrientation, bool isBank) {
  feedbackUpdateBuffer[feedbackUpdateWriteIdx].type = type;
  feedbackUpdateBuffer[feedbackUpdateWriteIdx].indexChanged = encIndex;
  feedbackUpdateBuffer[feedbackUpdateWriteIdx].newValue = val;
  feedbackUpdateBuffer[feedbackUpdateWriteIdx].newOrientation = encoderOrientation;
  feedbackUpdateBuffer[feedbackUpdateWriteIdx].isBank = isBank;

//  SerialUSB.print("type: "); SerialUSB.print(type);
//  SerialUSB.print("\t encIndex: "); SerialUSB.print(encIndex);
//  SerialUSB.print("\t val: "); SerialUSB.print(val);
//  SerialUSB.print("\t encoderOrientation: "); SerialUSB.print(encoderOrientation);
//  SerialUSB.print("\t isBank: "); SerialUSB.println(isBank);
  
  if(++feedbackUpdateWriteIdx >= FEEDBACK_UPDATE_BUFFER_SIZE)  
      feedbackUpdateWriteIdx = 0;
  
  if(updatingBankFeedback) Update();

//  SerialUSB.print("Write index: "); SerialUSB.println(feedbackUpdateWriteIdx);
}

void FeedbackClass::SetChangeDigitalFeedback(uint16_t digitalIndex, uint16_t updateValue, bool hwState, bool isBank){
    feedbackUpdateBuffer[feedbackUpdateWriteIdx].type = FB_DIGITAL;
    feedbackUpdateBuffer[feedbackUpdateWriteIdx].indexChanged = digitalIndex;
    feedbackUpdateBuffer[feedbackUpdateWriteIdx].newValue = updateValue;
    feedbackUpdateBuffer[feedbackUpdateWriteIdx].newOrientation = hwState;
    feedbackUpdateBuffer[feedbackUpdateWriteIdx].isBank = isBank;
    
    if(++feedbackUpdateWriteIdx >= FEEDBACK_UPDATE_BUFFER_SIZE)  
      feedbackUpdateWriteIdx = 0;
//    SerialUSB.print("write idx: ");
//    SerialUSB.println(feedbackUpdateWriteIdx);
    digFbData[currentBank][digitalIndex].digitalFbValue = updateValue;

    if(updatingBankFeedback) Update();

}

void FeedbackClass::SetChangeIndependentFeedback(uint8_t type, uint16_t fbIndex, uint16_t val){
  feedbackUpdateBuffer[feedbackUpdateWriteIdx].type = type;
  feedbackUpdateBuffer[feedbackUpdateWriteIdx].indexChanged = fbIndex;
  feedbackUpdateBuffer[feedbackUpdateWriteIdx].newValue = val;

  if(++feedbackUpdateWriteIdx >= FEEDBACK_UPDATE_BUFFER_SIZE)  
      feedbackUpdateWriteIdx = 0;

  if(updatingBankFeedback) Update();
}

void FeedbackClass::SetBankChangeFeedback(){
  feedbackUpdateBuffer[feedbackUpdateWriteIdx].type = FB_BANK_CHANGED;
  updatingBankFeedback = true;
  
  if(++feedbackUpdateWriteIdx >= FEEDBACK_UPDATE_BUFFER_SIZE)  
      feedbackUpdateWriteIdx = 0;

}

void FeedbackClass::SendDataIfReady(){
  if(feedbackDataToSend){
    feedbackDataToSend = false;
    //AddCheckSum(); 
    SendFeedbackData(); 
  }
}

void FeedbackClass::AddCheckSum(){ 
  uint16_t sum = 2019 - checkSum(sendSerialBufferEnc, e_B+1);

  sum &= 0x3FFF;    // 14 bit checksum
  
  sendSerialBufferEnc[e_checkSum_MSB] = (sum >> 7) & 0x7F;
  sendSerialBufferEnc[e_checkSum_LSB] = sum & 0x7F;
//      sendSerialBufferDec[CRC] = 127 - CRC8(sendSerialBufferDec, B+1);
//      SerialUSB.println(sendSerialBufferDec[CRC], DEC);
}

//#define DEBUG_FB_FRAME
void FeedbackClass::SendFeedbackData(){
  unsigned long serialTimeout = millis();
  byte tries = 0;
  bool okToContinue = false;
  byte ack = 0;

  byte encodedFrameSize = encodeSysEx(sendSerialBufferDec, sendSerialBufferEnc, d_ENDOFFRAME);
  
  // Adds checksum bytes to encoded frame
  AddCheckSum();

//  if(sendSerialBufferDec[d_frameType] == ENCODER_DOUBLE_FRAME){
//    SerialUSB.print("FRAME WITHOUT ENCODING:\n");
//    for(int i = 0; i <= d_B; i++){
//      SerialUSB.print(i); SerialUSB.print(": ");SerialUSB.println(sendSerialBufferDec[i]);
//    }  
//  }
  #ifdef DEBUG_FB_FRAME
  SerialUSB.print("FRAME WITHOUT ENCODING:\n");
  for(int i = 0; i <= d_B; i++){
    SerialUSB.print(i); SerialUSB.print(": ");SerialUSB.println(sendSerialBufferDec[i]);
  }
  SerialUSB.print("ENCODED FRAME:\n");
  for(int i = 0; i < encodedFrameSize; i++){
    SerialUSB.print(i); SerialUSB.print(": ");SerialUSB.println(sendSerialBufferEnc[i]);
  }
  SerialUSB.println("******************************************");
  SerialUSB.println("Serial DATA: ");
  #endif
  do{
    ack = 0;
    if(burst) Serial.write(BANK_INIT);   // SEND BANK INIT if burst mode is enabled
    Serial.write(NEW_FRAME_BYTE);   // SEND FRAME HEADER
    Serial.write(e_ENDOFFRAME+1); // NEW FRAME SIZE - SIZE FOR ENCODED FRAME
    #ifdef DEBUG_FB_FRAME
    SerialUSB.println(NEW_FRAME_BYTE);
    SerialUSB.println(e_ENDOFFRAME+1);
    #endif
    for (int i = 0; i < e_ENDOFFRAME; i++) {
      Serial.write(sendSerialBufferEnc[i]);
      #ifdef DEBUG_FB_FRAME
//      if(i == ringStateH || i == ringStateL)
//        SerialUSB.println(sendSerialBufferEnc[i],BIN);
//      else
        SerialUSB.println(sendSerialBufferEnc[i]);
      #endif
//      delayMicroseconds(5);
    }
//    Serial.write(sendSerialBufferEnc[e_checkSum_MSB]);     // Checksum is calculated on decoded frame
//    Serial.write(sendSerialBufferEnc[e_checkSum_LSB]);     
    Serial.write(END_OF_FRAME_BYTE);                         // SEND END OF FRAME BYTE
    if((feedbackUpdateWriteIdx - feedbackUpdateReadIdx) <= 1 && burst){
      burst = false; 
      Serial.write(BANK_END);               // SEND BANK END if burst mode was enabled
//      SerialUSB.println("BURST MODE OFF");
    }
    Serial.flush();    
    #ifdef DEBUG_FB_FRAME
//    SerialUSB.println(sendSerialBufferEnc[e_checkSum_MSB]);     // Checksum is calculated on decoded frame
//    SerialUSB.println(sendSerialBufferEnc[e_checkSum_LSB]);     
    SerialUSB.println(END_OF_FRAME_BYTE);
    SerialUSB.println("******************************************");
    #endif
    uint32_t antMicrosAck = micros();

    while(!Serial.available() && ((micros() - antMicrosAck) < 1000));
    //while(!Serial.available());
    if(Serial.available()){
      ack = Serial.read();
//      if(ack == sendSerialBufferEnc[e_checkSum_LSB]){
      if(ack == 0xAA){
        okToContinue = true;
      }else{
        tries++;
      }
    }else{
      tries++;
    }
    #ifdef DEBUG_FB_FRAME
    SerialUSB.print("ACK: "); SerialUSB.print(ack);
//    if(!ack){
//      SerialUSB.print("\tINDEX: "); SerialUSB.print(sendSerialBuffer[nRing]);
//      SerialUSB.print("\tTIME: "); SerialUSB.print(micros() - antMicrosAck);
//      SerialUSB.print("\tT: "); SerialUSB.println(tries);
//    }else{
//      SerialUSB.println();
//    }
    SerialUSB.println("******************************************");
    #endif
    
  //}while(!okToContinue && (millis() - serialTimeout < 5));
  }while(!okToContinue && tries < 20);
  
  
  //}while(!okToContinue);
}

void FeedbackClass::SendCommand(uint8_t cmd){
//  SerialUSB.print("Command sent: ");
//  SerialUSB.println(cmd, HEX);
  Serial.write(cmd);
  Serial.flush();
}
void FeedbackClass::SendResetToBootloader(){
  
  Serial.flush();
}

void * FeedbackClass::GetEncoderFBPtr(){
  return (void*) encFbData[currentBank];
}
      
