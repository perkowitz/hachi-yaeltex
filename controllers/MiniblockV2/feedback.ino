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
        digFbData[b][d].digitalFbState = 0;
        digFbData[b][d].colorIndexPrev = 0;
      }
    }
//    SerialUSB.println("**************************************************");
//    SerialUSB.print("Digital FB[0]: "); printPointer(digitalFbState[b]);
//    SerialUSB.print("Encoder FB[0]: "); printPointer(encFbData[b]);
//    SerialUSB.println("**************************************************");
  } 
}

void FeedbackClass::InitPower(){
  // POWER MANAGEMENT - READ FROM POWER PIN, IF POWER SUPPLY IS PRESENT AND SET LED BRIGHTNESS ACCORDINGLY
  feedbackHw.SendCommand(CMD_ALL_LEDS_OFF);
  delay(10);
  
  bool okToContinue = false;
  byte initFrameIndex = 0;
  
  if(digitalRead(pinExternalVoltage)){
    currentBrightness = BRIGHTNESS_WO_POWER;
  }else{
    currentBrightness = BRIGHTNESS_WITH_POWER;
  }
  // Set External ISR for the power adapter detector pin
  attachInterrupt(digitalPinToInterrupt(pinExternalVoltage), ChangeBrigthnessISR, CHANGE);
  
  // SEND INITIAL VALUES AND LED BRIGHTNESS TO SAMD11
  #define INIT_FRAME_SIZE 6
  byte initFrameArray[INIT_FRAME_SIZE] = {INIT_VALUES, 
                                          nEncoders,
                                          nIndependent,   // CHANGE TO AMOUNT OF ANALOG WITH FEEDBACK
                                          amountOfDigitalInConfig[0],
                                          amountOfDigitalInConfig[1],
                                          currentBrightness };
                            
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
}

void FeedbackClass::Update() {
//  SerialUSB.print("Digital FB[0]: "); printPointer(digitalFbState[currentBank]);
//  SerialUSB.print("Encoder FB[0]: "); printPointer(encFbData[currentBank]);
    
  if (feedbackUpdateReadIdx != feedbackUpdateWriteIdx) {
    switch(feedbackUpdateBuffer[feedbackUpdateReadIdx].type){
      case FB_ENCODER:
      case FB_ENCODER_SWITCH:{
        FillFrameWithEncoderData();
        SendDataIfReady();
      }break;
      case FB_DIGITAL:{
        FillFrameWithDigitalData();
        SendDataIfReady();
      }break;
      case FB_ANALOG:{
        
      }break;
      case FB_INDEPENDENT:{
        
      }break;
      case FB_BANK_CHANGED:{
        // 9ms para cambiar el banco - 32 encoders, 0 dig, 0 analog - 16/7/2009
        unsigned long antMicrosBank = micros();
        feedbackHw.SendCommand(CMD_ALL_LEDS_OFF);
        for(uint8_t n = 0; n < nEncoders; n++){
          SetChangeEncoderFeedback(FB_ENCODER, n, encoderHw.GetEncoderValue(n), 
                                                  encoderHw.GetModuleOrientation(n/4));   // HARDCODE: N° of encoders in module
          FillFrameWithEncoderData();
          SendDataIfReady();
        }
        for(uint8_t n = 0; n < nEncoders; n++){
          SetChangeEncoderFeedback(FB_ENCODER_SWITCH, n, encoderHw.GetEncoderSwitchValue(n), 
                                                         encoderHw.GetModuleOrientation(n/4));  // HARDCODE: N° of encoders in module
          FillFrameWithEncoderData();
          SendDataIfReady();
        }
        
        for(uint8_t n = 0; n < nDigitals; n++){
          SetChangeDigitalFeedback(n, digitalHw.GetDigitalValue(n), false);
          
          FillFrameWithDigitalData();
          SendDataIfReady();
        }

        if(config->banks.count > 1){    // If there is more than one bank
          for(int bank = 0; bank < config->banks.count; bank++){
            byte bankShifterIndex = config->banks.shifterId[bank];
            if(currentBank == bank){
              if(bankShifterIndex < 31){
  //              SerialUSB.print("FB SWITCH "); 
  //              SerialUSB.print(bankShifterIndex); 
  //              SerialUSB.print(" BANK ON: ");
                
                SetChangeEncoderFeedback(FB_ENCODER_SWITCH, bankShifterIndex, true, encoderHw.GetModuleOrientation(bankShifterIndex/4));  // HARDCODE: N° of encoders in module     
              }else{
                // DIGITAL
                SetChangeDigitalFeedback(bankShifterIndex-32, true, true);
              }
            }else{
              if(bankShifterIndex < 31){
  //              SerialUSB.println("FB SWITCH BANK OFF");
                SetChangeEncoderFeedback(FB_ENCODER_SWITCH, bankShifterIndex, false, encoderHw.GetModuleOrientation(bankShifterIndex/4));  // HARDCODE: N° of encoders in module     
              }else{
                // DIGITAL
                SetChangeDigitalFeedback(bankShifterIndex-32, false, true);
              }
            }
          }
        }
        updatingBankFeedback = false;
        SerialUSB.print(feedbackUpdateReadIdx);
        SerialUSB.print(" F - ");
        SerialUSB.println(micros()-antMicrosBank);
      }break;
      default: break;
    }
    
    feedbackUpdateBuffer[feedbackUpdateReadIdx].type = FeebackTypes::NONE;
    
    if(++feedbackUpdateReadIdx >= FEEDBACK_UPDATE_BUFFER_SIZE)  
      feedbackUpdateReadIdx = 0;

//    SerialUSB.print("Read index: "); SerialUSB.println(feedbackUpdateReadIdx);
    
  }
  
  
}

void FeedbackClass::FillFrameWithEncoderData(){
//  SerialUSB.println(indexChanged);
  // FIX FOR SHIFT ROTARY ACTION
  uint8_t colorR = 0, colorG = 0, colorB = 0;
  uint8_t colorIndex = 0;
  bool colorIndexChanged = false;
  uint8_t ringStateIndex= false;
  bool encoderSwitchChanged = false;
  
  uint16_t minValue = 0, maxValue = 0;
  uint8_t msgType = 0;
  bool is14bits = false;
  
  uint8_t indexChanged = feedbackUpdateBuffer[feedbackUpdateReadIdx].indexChanged;
  uint8_t newOrientation = feedbackUpdateBuffer[feedbackUpdateReadIdx].newOrientation;
  uint16_t newValue = feedbackUpdateBuffer[feedbackUpdateReadIdx].newValue;
  uint8_t fbUpdateType = feedbackUpdateBuffer[feedbackUpdateReadIdx].type;
  
  // Get config info for this encoder
  if(!encoderHw.GetEncoderRotaryActionState(indexChanged)){
    minValue = encoder[indexChanged].rotaryConfig.parameter[rotary_minMSB]<<7 | encoder[indexChanged].rotaryConfig.parameter[rotary_minLSB];
    maxValue = encoder[indexChanged].rotaryConfig.parameter[rotary_maxMSB]<<7 | encoder[indexChanged].rotaryConfig.parameter[rotary_maxLSB];
    msgType = encoder[indexChanged].rotaryConfig.message;
  }else{
    minValue = encoder[indexChanged].switchConfig.parameter[switch_minValue_MSB]<<7 | encoder[indexChanged].switchConfig.parameter[switch_minValue_LSB];
    maxValue = encoder[indexChanged].switchConfig.parameter[switch_maxValue_MSB]<<7 | encoder[indexChanged].switchConfig.parameter[switch_maxValue_LSB];
    msgType = encoder[indexChanged].switchConfig.message;
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
  if(minValue > maxValue){
    invert = true;
  }
//  SerialUSB.print("ENCODER: ");SerialUSB.print(indexChanged);
//  SerialUSB.print("\tVALUE: ");SerialUSB.print(newValue);
//  SerialUSB.print("\tMIN: ");SerialUSB.print(minValue);
//  SerialUSB.print("\tMAX: ");SerialUSB.print(maxValue);
  
  if(fbUpdateType == FB_ENCODER){
    switch(encoder[indexChanged].rotaryFeedback.mode){
      case encoderRotaryFeedbackMode::fb_walk: {
        ringStateIndex = map( newValue, 
                              minValue, 
                              maxValue, 
                              invert ? WALK_SIZE - 1 : 0, 
                              invert ? 0 : WALK_SIZE - 1);
        if (abs(maxValue - minValue) <= WALK_SIZE/2){
          if((ringStateIndex%2)) ringStateIndex--;
        }                              
//        SerialUSB.print("\tRS INDEX: ");SerialUSB.println(ringStateIndex);                                                                  
        encFbData[currentBank][indexChanged].encRingState &= newOrientation ? ENCODER_SWITCH_V_ON : ENCODER_SWITCH_H_ON;
        encFbData[currentBank][indexChanged].encRingState |= walk[newOrientation][ringStateIndex];
      }
      break;
      case encoderRotaryFeedbackMode::fb_fill: {
        ringStateIndex = map( newValue, 
                              minValue, 
                              maxValue, 
                              invert ? FILL_SIZE - 1 : 0, 
                              invert ? 0 : FILL_SIZE - 1);
        encFbData[currentBank][indexChanged].encRingState &= newOrientation ? ENCODER_SWITCH_V_ON : ENCODER_SWITCH_H_ON;
        encFbData[currentBank][indexChanged].encRingState |= fill[newOrientation][ringStateIndex];
      }
      break;
      case encoderRotaryFeedbackMode::fb_eq: {
        ringStateIndex = map( newValue, 
                              minValue, 
                              maxValue, 
                              invert ? EQ_SIZE - 1 : 0, 
                              invert ? 0 : EQ_SIZE - 1);
        encFbData[currentBank][indexChanged].encRingState &= newOrientation ? ENCODER_SWITCH_V_ON : ENCODER_SWITCH_H_ON;
        encFbData[currentBank][indexChanged].encRingState |= eq[newOrientation][ringStateIndex];
      }
      break;
      case encoderRotaryFeedbackMode::fb_spread: {
        ringStateIndex = map( newValue, 
                              minValue, 
                              maxValue, 
                              invert ? SPREAD_SIZE - 1 : 0, 
                              invert ? 0 : SPREAD_SIZE - 1);
        encFbData[currentBank][indexChanged].encRingState &= newOrientation ? ENCODER_SWITCH_V_ON : ENCODER_SWITCH_H_ON;
        encFbData[currentBank][indexChanged].encRingState |= spread[newOrientation][ringStateIndex];
      }
      break;
      default: break;
    }
    colorR = pgm_read_byte(&gamma8[encoder[indexChanged].rotaryFeedback.color[R_INDEX]]);
    colorG = pgm_read_byte(&gamma8[encoder[indexChanged].rotaryFeedback.color[G_INDEX]]);
    colorB = pgm_read_byte(&gamma8[encoder[indexChanged].rotaryFeedback.color[B_INDEX]]);
  }else{  // Feedback for encoder switch  
    if(encoder[indexChanged].switchFeedback.colorRangeEnable){
      encFbData[currentBank][indexChanged].encRingState |= (newOrientation ? ENCODER_SWITCH_V_ON : ENCODER_SWITCH_H_ON);
      
      if(!newValue)                                                   colorIndex = encoder[indexChanged].switchFeedback.colorRange0;
      else if(newValue > COLOR_RANGE_0 && newValue <= COLOR_RANGE_1)  colorIndex = encoder[indexChanged].switchFeedback.colorRange1;
      else if(newValue > COLOR_RANGE_1 && newValue <= COLOR_RANGE_2)  colorIndex = encoder[indexChanged].switchFeedback.colorRange2;
      else if(newValue > COLOR_RANGE_2 && newValue <= COLOR_RANGE_3)  colorIndex = encoder[indexChanged].switchFeedback.colorRange3;
      else if(newValue > COLOR_RANGE_3 && newValue <= COLOR_RANGE_4)  colorIndex = encoder[indexChanged].switchFeedback.colorRange4;
      else if(newValue > COLOR_RANGE_4 && newValue <= COLOR_RANGE_5)  colorIndex = encoder[indexChanged].switchFeedback.colorRange5;
      else if(newValue > COLOR_RANGE_5 && newValue <= COLOR_RANGE_6)  colorIndex = encoder[indexChanged].switchFeedback.colorRange6;
      else if(newValue == COLOR_RANGE_7)                              colorIndex = encoder[indexChanged].switchFeedback.colorRange7;

      if(colorIndex != encFbData[currentBank][indexChanged].colorIndexPrev){
        colorIndexChanged = true;
        encFbData[currentBank][indexChanged].colorIndexPrev = colorIndex;
      }else
      colorR = pgm_read_byte(&gamma8[colorRangeTable[colorIndex][R_INDEX]]);
      colorG = pgm_read_byte(&gamma8[colorRangeTable[colorIndex][G_INDEX]]);
      colorB = pgm_read_byte(&gamma8[colorRangeTable[colorIndex][B_INDEX]]);
//      SerialUSB.print("IDX: ");SerialUSB.print(colorIndex); 
//      SerialUSB.print("\tR: ");SerialUSB.print(colorR); 
//      SerialUSB.print("\tG: ");SerialUSB.print(colorG); 
//      SerialUSB.print("\tB: ");SerialUSB.println(colorB); 
    }else{    
      if(newValue){
        encFbData[currentBank][indexChanged].encRingState |= (newOrientation ? ENCODER_SWITCH_V_ON : ENCODER_SWITCH_H_ON);
        colorR = pgm_read_byte(&gamma8[encoder[indexChanged].switchFeedback.color[R_INDEX]]);
        colorG = pgm_read_byte(&gamma8[encoder[indexChanged].switchFeedback.color[G_INDEX]]);
        colorB = pgm_read_byte(&gamma8[encoder[indexChanged].switchFeedback.color[B_INDEX]]);   
      }else{
        encFbData[currentBank][indexChanged].encRingState &= ~(newOrientation ? ENCODER_SWITCH_V_ON : ENCODER_SWITCH_H_ON);
        colorR = 0;
        colorG = 0;
        colorB = 0;
      }
      encoderSwitchChanged = true;
    }
  }

  if (fbUpdateType == FB_ENCODER
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

    sendSerialBuffer[msgLength] = TX_BYTES;   // INIT SERIAL FRAME WITH CONSTANT DATA
    sendSerialBuffer[frameType] = fbUpdateType == FB_ENCODER ? 
                                  ENCODER_CHANGE_FRAME : 
                                  ENCODER_SWITCH_CHANGE_FRAME;   
    sendSerialBuffer[nRing] = indexChanged;
    sendSerialBuffer[orientation] = newOrientation;
    sendSerialBuffer[ringStateH] = encFbData[currentBank][indexChanged].encRingState >> 8;
    sendSerialBuffer[ringStateL] = encFbData[currentBank][indexChanged].encRingState & 0xff;
    sendSerialBuffer[currentValue] = newValue;
    sendSerialBuffer[fbMin] = minValue;
    sendSerialBuffer[fbMax] = maxValue;
    sendSerialBuffer[R] = colorR;
    sendSerialBuffer[G] = colorG;
    sendSerialBuffer[B] = colorB;
    sendSerialBuffer[ENDOFFRAME] = 255;
    feedbackDataToSend = true;
  }
}

void FeedbackClass::FillFrameWithDigitalData(){
  uint8_t colorR = 0, colorG = 0, colorB = 0;
  bool colorIndexChanged = false;
  uint8_t colorIndex = 0; 
  
  uint8_t indexChanged = feedbackUpdateBuffer[feedbackUpdateReadIdx].indexChanged;
  uint16_t newValue = feedbackUpdateBuffer[feedbackUpdateReadIdx].newValue;
  uint8_t fbUpdateType = feedbackUpdateBuffer[feedbackUpdateReadIdx].type;
  bool isBank = feedbackUpdateBuffer[feedbackUpdateReadIdx].newOrientation;
  
  if(digital[indexChanged].feedback.colorRangeEnable){
      if(!newValue)                                                   colorIndex = digital[indexChanged].feedback.colorRange0;
      else if(newValue > COLOR_RANGE_0 && newValue <= COLOR_RANGE_1)  colorIndex = digital[indexChanged].feedback.colorRange1;
      else if(newValue > COLOR_RANGE_1 && newValue <= COLOR_RANGE_2)  colorIndex = digital[indexChanged].feedback.colorRange2;
      else if(newValue > COLOR_RANGE_2 && newValue <= COLOR_RANGE_3)  colorIndex = digital[indexChanged].feedback.colorRange3;
      else if(newValue > COLOR_RANGE_3 && newValue <= COLOR_RANGE_4)  colorIndex = digital[indexChanged].feedback.colorRange4;
      else if(newValue > COLOR_RANGE_4 && newValue <= COLOR_RANGE_5)  colorIndex = digital[indexChanged].feedback.colorRange5;
      else if(newValue > COLOR_RANGE_5 && newValue <= COLOR_RANGE_6)  colorIndex = digital[indexChanged].feedback.colorRange6;
      else if(newValue == COLOR_RANGE_7)                              colorIndex = digital[indexChanged].feedback.colorRange7;
    
      colorR = pgm_read_byte(&gamma8[colorRangeTable[colorIndex][R_INDEX]]);
      colorG = pgm_read_byte(&gamma8[colorRangeTable[colorIndex][G_INDEX]]);
      colorB = pgm_read_byte(&gamma8[colorRangeTable[colorIndex][B_INDEX]]);
    }else{    
//      SerialUSB.print(indexChanged);
//      SerialUSB.print(isBank ? ": BANK " : ": NOT BANK ");
//      SerialUSB.println(newValue ? "PRESSED" : "RELEASED");
      
      if((newValue && !isBank) || (!newValue && isBank)){
        colorR = pgm_read_byte(&gamma8[digital[indexChanged].feedback.color[R_INDEX]]);
        colorG = pgm_read_byte(&gamma8[digital[indexChanged].feedback.color[G_INDEX]]);
        colorB = pgm_read_byte(&gamma8[digital[indexChanged].feedback.color[B_INDEX]]);   
      }else if(newValue && isBank){
        colorR = pgm_read_byte(&gamma8[127]);
        colorG = pgm_read_byte(&gamma8[127]);
        colorB = pgm_read_byte(&gamma8[127]);
      }else{
        colorR = 0;
        colorG = 0;
        colorB = 0;
      }
    }
//      SerialUSB.print("IDX: ");SerialUSB.print(indexChanged); 
//      SerialUSB.print("\tR: ");SerialUSB.print(colorR); 
//      SerialUSB.print("\tG: ");SerialUSB.print(colorG); 
//      SerialUSB.print("\tB: ");SerialUSB.println(colorB); 
//      SerialUSB.println(); 
  
  sendSerialBuffer[msgLength] = TX_BYTES;   // INIT SERIAL FRAME WITH CONSTANT DATA
  sendSerialBuffer[frameType] = (indexChanged < amountOfDigitalInConfig[0]) ? DIGITAL1_CHANGE_FRAME : 
                                                                              DIGITAL2_CHANGE_FRAME;   
  sendSerialBuffer[nDig] = indexChanged;
  sendSerialBuffer[orientation] = 0;
  sendSerialBuffer[ringStateH] = digFbData[currentBank][indexChanged].digitalFbState;
  sendSerialBuffer[ringStateL] = 0;
  sendSerialBuffer[R] = colorR;
  sendSerialBuffer[G] = colorG;
  sendSerialBuffer[B] = colorB;
  sendSerialBuffer[ENDOFFRAME] = 255;
  feedbackDataToSend = true;
}


void FeedbackClass::SetChangeEncoderFeedback(uint8_t type, uint8_t encIndex, uint16_t val, uint8_t encoderOrientation) {
  feedbackUpdateBuffer[feedbackUpdateWriteIdx].type = type;
  feedbackUpdateBuffer[feedbackUpdateWriteIdx].indexChanged = encIndex;
  feedbackUpdateBuffer[feedbackUpdateWriteIdx].newValue = val;
  feedbackUpdateBuffer[feedbackUpdateWriteIdx].newOrientation = encoderOrientation;
  
  if(++feedbackUpdateWriteIdx >= FEEDBACK_UPDATE_BUFFER_SIZE)  
      feedbackUpdateWriteIdx = 0;
      
  if(updatingBankFeedback) Update();

//  SerialUSB.print("Write index: "); SerialUSB.println(feedbackUpdateWriteIdx);
}

void FeedbackClass::SetChangeDigitalFeedback(uint16_t digitalIndex, uint16_t val, bool isBank){
  //if(digFbData[currentBank][digitalIndex].digitalFbState != val){
    feedbackUpdateBuffer[feedbackUpdateWriteIdx].type = FB_DIGITAL;
    feedbackUpdateBuffer[feedbackUpdateWriteIdx].indexChanged = digitalIndex;
    feedbackUpdateBuffer[feedbackUpdateWriteIdx].newValue = val;
    feedbackUpdateBuffer[feedbackUpdateWriteIdx].newOrientation = isBank;
    
    if(++feedbackUpdateWriteIdx >= FEEDBACK_UPDATE_BUFFER_SIZE)  
      feedbackUpdateWriteIdx = 0;
      
    digFbData[currentBank][digitalIndex].digitalFbState = val;
    if(updatingBankFeedback) Update();
 // }
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
      
  Update();
  
//  feedbackUpdateFlag = FB_BANK_CHANGED;
//  updatingBankFeedback = true;
//  Update();
}

void FeedbackClass::SendDataIfReady(){
  if(feedbackDataToSend){
    feedbackDataToSend = false;
    AddCheckSum(); 
    SendFeedbackData(); 
  }
}

void FeedbackClass::AddCheckSum(){ 
  uint16_t sum = 2019 - checkSum(sendSerialBuffer, B + 1);

  sendSerialBuffer[checkSum_MSB] = (sum >> 8) & 0xFF;
  sendSerialBuffer[checkSum_LSB] = sum & 0xff;
//      sendSerialBuffer[CRC] = 127 - CRC8(sendSerialBuffer, B+1);
//      SerialUSB.println(sendSerialBuffer[CRC], DEC);
}

//#define DEBUG_FB_FRAME
void FeedbackClass::SendFeedbackData(){
  unsigned long serialTimeout = millis();
  bool okToContinue = false;
  byte ack = 0;
  #ifdef DEBUG_FB_FRAME
  SerialUSB.println("******************************************");
  SerialUSB.println("Serial DATA: ");
  #endif
  do{
    Serial.write(NEW_FRAME_BYTE);
    for (int i = msgLength; i <= ENDOFFRAME; i++) {
      Serial.write(sendSerialBuffer[i]);
      #ifdef DEBUG_FB_FRAME
      if(i == ringStateH || i == ringStateL)
        SerialUSB.println(sendSerialBuffer[i],BIN);
      else
        SerialUSB.println(sendSerialBuffer[i]);
      #endif
    }
    Serial.flush();    
    #ifdef DEBUG_FB_FRAME
    SerialUSB.println("******************************************");
    #endif
    if(Serial.available()){
      ack = Serial.read();
      if(ack == sendSerialBuffer[checkSum_LSB])
        okToContinue = true;
    }
  }while(!okToContinue && (millis() - serialTimeout < 5));
  
  #ifdef DEBUG_FB_FRAME
  SerialUSB.print("ACK: "); SerialUSB.println(ack);
  SerialUSB.println("******************************************");
  #endif
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
      
