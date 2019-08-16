#include "headers/FeedbackClass.h"

//----------------------------------------------------------------------------------------------------
// FEEDBACK FUNCTIONS
//----------------------------------------------------------------------------------------------------
void FeedbackClass::Init(uint8_t maxBanks, uint8_t maxEncoders, uint8_t maxDigital, uint8_t maxIndependent) {
  nBanks = maxBanks;
  nEncoders = maxEncoders;
  nDigital = maxDigital;
  nIndependent = maxIndependent;

  if(!nBanks) return;    // If number of encoders is zero, return;
  
  feedbackUpdateFlag = NONE;

  // STATUS LED
  statusLED =  Adafruit_NeoPixel(N_STATUS_PIXEL, STATUS_LED_PIN, NEO_GRB + NEO_KHZ800);

  statusLED.begin();
  statusLED.setBrightness(STATUS_LED_BRIGHTNESS);

  flagBlinkStatusLED = 0;
  blinkCountStatusLED = 0;
  statusLEDfbType = 0;
  lastStatusLEDState = LOW;
  millisStatusPrev = 0;
    
  // First dimension is an array of pointers, each pointing to a column - https://www.eskimo.com/~scs/cclass/int/sx9b.html

  // ESTO ADELANTE DEL MALLOC DE LOS ENCODERS HACE QUE FUNCIONEN LOS ENCODERS PARA 1 BANCO CONFIGURADO
  // EL PRIMER PUNTERO QUE LLAMA A MALLOC, CAMBIA LA DIRECCIÓN A LA QUE APUNTA DESPUÉS DE LA INICIALIZACIÓN,
  // EN LA SEGUNDA VUELTA DE LOOP, SOLO CUANDO HAY 1 BANCO CONFIGURADO
  if(nDigital){
    digitalFbState = (uint8_t**) memHost->AllocateRAM(nBanks*sizeof(uint8_t*));
//    SerialUSB.print("Digital FB: "); printPointer(digitalFbState);
  }
  if(nEncoders){
    encFbData = (encFeedbackData**) memHost->AllocateRAM(nBanks*sizeof(encFeedbackData*));
  }
  
  for (int b = 0; b < nBanks; b++) {
    encFbData[b] = (encFeedbackData*) memHost->AllocateRAM(nEncoders*sizeof(encFeedbackData));
    digitalFbState[b] = (uint8_t*) memHost->AllocateRAM(nDigital*sizeof(uint8_t));
//    SerialUSB.print("Digital FB[0]: "); printPointer(digitalFbState[b]);
//    SerialUSB.print("Encoder FB[0]: "); printPointer(encFbData[b]);
    for (int e = 0; e < nEncoders; e++) {
      encFbData[b][e].encRingState = 0;
      encFbData[b][e].encRingStatePrev = 0;
      encFbData[b][e].ringStateIndex = 0;
    }
    for (int d = 0; d < nDigital; d++) {
      digitalFbState[b][d] = 0;
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
    currentBrightness = BRIGHNESS_WO_POWER;
  }else{
    currentBrightness = BRIGHNESS_WITH_POWER;
  }

  attachInterrupt(digitalPinToInterrupt(pinExternalVoltage), ChangeBrigthnessISR, CHANGE);
  // SEND INITIAL VALUES AND LED BRIGHTNESS TO SAMD11
  #define INIT_FRAME_SIZE 6
  byte initFrameArray[INIT_FRAME_SIZE] = {INIT_VALUES, 
                                          config->inputs.encoderCount,
                                          config->inputs.analogCount,   // CHANGE TO AMOUNT OF ANALOG WITH FEEDBACK
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
  
}

void FeedbackClass::Update() {
//  SerialUSB.print("Digital FB[0]: "); printPointer(digitalFbState[currentBank]);
//  SerialUSB.print("Encoder FB[0]: "); printPointer(encFbData[currentBank]);
  if (feedbackUpdateFlag) {
    switch(feedbackUpdateFlag){
      case FB_ENCODER:
      case FB_ENCODER_SWITCH:{
        FillFrameWithEncoderData();
        SendDataIfChanged();
      }break;
      case FB_DIGITAL:{
        FillFrameWithDigitalData();
        SendDataIfChanged();
      }break;
      case FB_ANALOG:{
        
      }break;
      case FB_INDEPENDENT:{
        
      }break;
      case FB_BANK_CHANGED:{
        // 9ms para cambiar el banco - 32 encoders, 0 dig, 0 analog - 16/7/2009
        unsigned long antMicrosBank = micros();
        feedbackHw.SendCommand(CMD_ALL_LEDS_OFF);
        for(int n = 0; n < nEncoders; n++){
          SetChangeEncoderFeedback(FB_ENCODER, n, encoderHw.GetEncoderValue(n), 
                                                  encoderHw.GetModuleOrientation(n/4));   // HARDCODE: N° of encoders in module
          
          SetChangeEncoderFeedback(FB_ENCODER_SWITCH, n, encoderHw.GetEncoderSwitchValue(n), 
                                                         encoderHw.GetModuleOrientation(n/4));  // HARDCODE: N° of encoders in module
          FillFrameWithEncoderData();
          SendDataIfChanged();
        }
        updatingBankFeedback = false;
        SerialUSB.print("F - ");
        SerialUSB.println(micros()-antMicrosBank);
      }break;
      default: break;
    }
    feedbackUpdateFlag = NONE;
  }
  
}

void FeedbackClass::FillFrameWithEncoderData(){
//  SerialUSB.println(indexChanged);
  if(feedbackUpdateFlag == FB_ENCODER){
    switch(encoder[indexChanged].rotaryFeedback.mode){
      case encoderRotaryFeedbackMode::fb_walk: {
        encFbData[currentBank][indexChanged].ringStateIndex = map(newValue, 
                                                                  encoder[indexChanged].rotaryConfig.parameter[rotary_minMSB]<<8 | 
                                                                  encoder[indexChanged].rotaryConfig.parameter[rotary_minLSB], 
                                                                  encoder[indexChanged].rotaryConfig.parameter[rotary_maxMSB]<<8 | 
                                                                  encoder[indexChanged].rotaryConfig.parameter[rotary_maxLSB], 
                                                                  0, 
                                                                  WALK_SIZE - 1);
        encFbData[currentBank][indexChanged].encRingState &= orientation ? ENCODER_SWITCH_V_ON : ENCODER_SWITCH_H_ON;
        encFbData[currentBank][indexChanged].encRingState |= walk[orientation][encFbData[currentBank][indexChanged].ringStateIndex];
      }
      break;
      case encoderRotaryFeedbackMode::fb_fill: {
        encFbData[currentBank][indexChanged].ringStateIndex = map(newValue, 
                                                                  encoder[indexChanged].rotaryConfig.parameter[rotary_minMSB]<<8 | 
                                                                  encoder[indexChanged].rotaryConfig.parameter[rotary_minLSB], 
                                                                  encoder[indexChanged].rotaryConfig.parameter[rotary_maxMSB]<<8 | 
                                                                  encoder[indexChanged].rotaryConfig.parameter[rotary_maxLSB], 
                                                                  0, 
                                                                  FILL_SIZE - 1);
        encFbData[currentBank][indexChanged].encRingState &= orientation ? ENCODER_SWITCH_V_ON : ENCODER_SWITCH_H_ON;
        encFbData[currentBank][indexChanged].encRingState |= fill[orientation][encFbData[currentBank][indexChanged].ringStateIndex];
      }
      break;
      case encoderRotaryFeedbackMode::fb_eq: {
        encFbData[currentBank][indexChanged].ringStateIndex = map(newValue, 
                                                                  encoder[indexChanged].rotaryConfig.parameter[rotary_minMSB]<<8 | 
                                                                  encoder[indexChanged].rotaryConfig.parameter[rotary_minLSB], 
                                                                  encoder[indexChanged].rotaryConfig.parameter[rotary_maxMSB]<<8 | 
                                                                  encoder[indexChanged].rotaryConfig.parameter[rotary_maxLSB], 
                                                                  0, 
                                                                  EQ_SIZE - 1);
        encFbData[currentBank][indexChanged].encRingState &= orientation ? ENCODER_SWITCH_V_ON : ENCODER_SWITCH_H_ON;
        encFbData[currentBank][indexChanged].encRingState |= eq[orientation][encFbData[currentBank][indexChanged].ringStateIndex];
      }
      break;
      case encoderRotaryFeedbackMode::fb_spread: {
        encFbData[currentBank][indexChanged].ringStateIndex = map(newValue, 
                                                                  encoder[indexChanged].rotaryConfig.parameter[rotary_minMSB]<<8 | 
                                                                  encoder[indexChanged].rotaryConfig.parameter[rotary_minLSB], 
                                                                  encoder[indexChanged].rotaryConfig.parameter[rotary_maxMSB]<<8 | 
                                                                  encoder[indexChanged].rotaryConfig.parameter[rotary_maxLSB], 
                                                                  0, 
                                                                  SPREAD_SIZE - 1);
        encFbData[currentBank][indexChanged].encRingState &= orientation ? ENCODER_SWITCH_V_ON : ENCODER_SWITCH_H_ON;
        encFbData[currentBank][indexChanged].encRingState |= spread[orientation][encFbData[currentBank][indexChanged].ringStateIndex];
      }
      break;
      default: break;
    }
  }else{  // Feedback for encoder switch
    if (newValue) encFbData[currentBank][indexChanged].encRingState |= (orientation ? ENCODER_SWITCH_V_ON : ENCODER_SWITCH_H_ON);
    else          encFbData[currentBank][indexChanged].encRingState &= ~(orientation ? ENCODER_SWITCH_V_ON : ENCODER_SWITCH_H_ON);
  }

  if ((encFbData[currentBank][indexChanged].encRingState != encFbData[currentBank][indexChanged].encRingStatePrev) || updatingBankFeedback) {
//    SerialUSB.print("PREV STATE: ");SerialUSB.println(encFbData[currentBank][indexChanged].encRingStatePrev,BIN);
    
    encFbData[currentBank][indexChanged].encRingStatePrev = encFbData[currentBank][indexChanged].encRingState;

//    SerialUSB.print("CurrentBank: ");SerialUSB.println(currentBank);
//    SerialUSB.print("IndexChanged: ");SerialUSB.println(indexChanged);
//    SerialUSB.print("CURR STATE: ");SerialUSB.println(encFbData[currentBank][indexChanged].encRingState,BIN);
//    SerialUSB.print("PREV STATE: ");SerialUSB.println(encFbData[currentBank][indexChanged].encRingStatePrev,BIN);
//    SerialUSB.println();

    sendSerialBuffer[msgLength] = TX_BYTES;   // INIT SERIAL FRAME WITH CONSTANT DATA
    sendSerialBuffer[frameType] = ENCODER_CHANGE_FRAME;   
    sendSerialBuffer[nRing] = indexChanged;
    sendSerialBuffer[ringStateH] = encFbData[currentBank][indexChanged].encRingState >> 8;
    sendSerialBuffer[ringStateL] = encFbData[currentBank][indexChanged].encRingState & 0xff;
    sendSerialBuffer[R] = encoder[indexChanged].rotaryFeedback.color[R_INDEX];    
    sendSerialBuffer[G] = encoder[indexChanged].rotaryFeedback.color[G_INDEX];
    sendSerialBuffer[B] = encoder[indexChanged].rotaryFeedback.color[B_INDEX];
    sendSerialBuffer[ENDOFFRAME] = 255;
    feedbackDataToSend = true;
  }
}

void FeedbackClass::FillFrameWithDigitalData(){
  sendSerialBuffer[msgLength] = TX_BYTES;   // INIT SERIAL FRAME WITH CONSTANT DATA
  sendSerialBuffer[frameType] = (indexChanged < amountOfDigitalInConfig[0]) ? DIGITAL1_CHANGE_FRAME : 
                                                                              DIGITAL2_CHANGE_FRAME;   
  sendSerialBuffer[nRing] = indexChanged;
  sendSerialBuffer[ringStateH] = newValue;
  sendSerialBuffer[ringStateL] = 0;
  if(!digital[indexChanged].feedback.colorRangeEnable){
    sendSerialBuffer[R] = digital[indexChanged].feedback.color[R_INDEX];    
    sendSerialBuffer[G] = digital[indexChanged].feedback.color[G_INDEX];
    sendSerialBuffer[B] = digital[indexChanged].feedback.color[B_INDEX];
  }
  sendSerialBuffer[ENDOFFRAME] = 255;
}


void FeedbackClass::SetChangeEncoderFeedback(uint8_t type, uint8_t encIndex, uint8_t val, uint8_t encoderOrientation) {
  feedbackUpdateFlag = type;
  indexChanged = encIndex;
  newValue = val;
  orientation = encoderOrientation;
//  Update();
}

void FeedbackClass::SetChangeDigitalFeedback(uint8_t digitalIndex, uint8_t val){
  feedbackUpdateFlag = FB_DIGITAL;
  indexChanged = digitalIndex;
  newValue = val;
//  Update();
}

void FeedbackClass::SetChangeIndependentFeedback(uint8_t type, uint8_t fbIndex, uint8_t val){
  feedbackUpdateFlag = type;
  indexChanged = fbIndex;
  newValue = val;
  Update();
}

void FeedbackClass::SetBankChangeFeedback(){
  feedbackUpdateFlag = FB_BANK_CHANGED;
  updatingBankFeedback = true;
  Update();
}
/*
   Esta función es llamada por el loop principal, cuando las variables flagBlinkStatusLED y blinkCountStatusLED son distintas de cero.
   blinkCountStatusLED tiene la cantidad de veces que debe titilar el LED.
   El intervalo es fijo y dado por la etiqueta 'STATUS_BLINK_INTERVAL'
*/

void FeedbackClass::UpdateStatusLED() {
  
  if (flagBlinkStatusLED && blinkCountStatusLED) {
    if (firstTime) {
      firstTime = false;
      millisStatusPrev = millis();
    }
    
    if(flagBlinkStatusLED == STATUS_BLINK){
      if (millis() - millisStatusPrev > blinkInterval) {
        millisStatusPrev = millis();
        lastStatusLEDState = !lastStatusLEDState;
        if (lastStatusLEDState) {
          statusLED.setPixelColor(0, statusLEDColor[statusLEDfbType]); // Moderately bright green color.
        } else {
          statusLED.setPixelColor(0, statusLEDColor[STATUS_FB_NONE]); // Moderately bright green color.
          blinkCountStatusLED--;
        }
        statusLED.show(); // This sends the updated pixel color to the hardware.
  
        if (!blinkCountStatusLED) {
          flagBlinkStatusLED = STATUS_OFF;
          statusLEDfbType = 0;
          firstTime = true;
        }
      }
    }
//    else if (flagBlinkStatusLED == STATUS_ON){
//      statusLED.setPixelColor(0, statusLEDColor[statusLEDfbType]); // Moderately bright green color.
//    }else if (flagBlinkStatusLED == STATUS_OFF){
//      statusLED.setPixelColor(0, statusLEDColor[STATUS_FB_NONE]); // Moderately bright green color.
//    }      
  }
  return;
}

void FeedbackClass::SetStatusLED(uint8_t onOrBlinkOrOff, uint8_t nTimes, uint8_t status_type) {
  if (!flagBlinkStatusLED) {
    flagBlinkStatusLED = onOrBlinkOrOff;
    statusLEDfbType = status_type;
    blinkCountStatusLED = nTimes;
    
    switch(statusLEDfbType){
      case STATUS_FB_NONE:{
        blinkInterval = 0;
      }break;
      case STATUS_FB_CONFIG:{
        blinkInterval = STATUS_CONFIG_BLINK_INTERVAL;
      }break;
      case STATUS_FB_INPUT_CHANGED:{
        blinkInterval = STATUS_MIDI_BLINK_INTERVAL;
      }break;
      case STATUS_FB_ERROR:{
        blinkInterval = STATUS_ERROR_BLINK_INTERVAL;
      }break;
      default:
        blinkInterval = 0; break;
    }
  }
}

void FeedbackClass::SendDataIfChanged(){
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

void FeedbackClass::SendFeedbackData(){
unsigned long serialTimeout = millis();
  bool okToContinue = false;
  byte ack = 0;
//  SerialUSB.println("******************************************");
//  SerialUSB.println("Serial DATA: ");
  do{
    Serial.write(NEW_FRAME_BYTE);
    for (int i = msgLength; i <= ENDOFFRAME; i++) {
      Serial.write(sendSerialBuffer[i]);
//      if(i == ringStateH || i == ringStateL)
//        SerialUSB.println(sendSerialBuffer[i],BIN);
//      else
//        SerialUSB.println(sendSerialBuffer[i]);
    }
    Serial.flush();    
//    SerialUSB.println("******************************************");
    if(Serial.available()){
      ack = Serial.read();
      if(ack == sendSerialBuffer[nRing])
        okToContinue = true;
    }
  }while(!okToContinue && (millis() - serialTimeout < 4));
  
//  SerialUSB.print("ACK: "); SerialUSB.println(ack);
//  SerialUSB.println("******************************************");
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
      
