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
void FeedbackClass::Init(uint8_t maxBanks, uint8_t maxEncoders, uint16_t maxDigital, uint16_t maxIndependent) {
  nBanks = maxBanks;
  nEncoders = maxEncoders;
  nDigitals = maxDigital;
  nIndependent = maxIndependent;

  if(!nBanks) return;    // If number of encoders is zero, return;
  
  feedbackUpdateWriteIdx = 0;
  feedbackUpdateReadIdx = 0;
  waitingMoreData = false;
  antMillisWaitMoreData = 0;

  for(int f = 0; f < FEEDBACK_UPDATE_BUFFER_SIZE; f++){
    feedbackUpdateBuffer[f].type = 0;
    feedbackUpdateBuffer[f].indexChanged = 0;
    feedbackUpdateBuffer[f].newValue = 0;
    feedbackUpdateBuffer[f].newOrientation = 0;
    feedbackUpdateBuffer[f].isShifter = 0;
    feedbackUpdateBuffer[f].updatingBank = false;
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
  
  // Reset to bootloader if there isn't enough RAM
  if(FreeMemory() < nBanks*nEncoders*sizeof(encFeedbackData) + nBanks*nDigitals*sizeof(digFeedbackData) + 800){
    SerialUSB.println("NOT ENOUGH RAM / FEEDBACK -> REBOOTING TO BOOTLOADER...");
    delay(500);
    config->board.bootFlag = 1;                                            
    byte bootFlagState = 0;
    eep.read(BOOT_FLAGS_ADDR, (byte *) &bootFlagState, sizeof(bootFlagState));
    bootFlagState |= 1;
    eep.write(BOOT_FLAGS_ADDR, (byte *) &bootFlagState, sizeof(bootFlagState));

    SelfReset();
  }

  for (int b = 0; b < nBanks; b++) {
    if(nEncoders){
      encFbData[b] = (encFeedbackData*) memHost->AllocateRAM(nEncoders*sizeof(encFeedbackData));
      for (int e = 0; e < nEncoders; e++) {
        encFbData[b][e].encRingState = 0;
        encFbData[b][e].encRingStatePrev = 0;
        // encFbData[b][e].nextStateOn = 0;       // FEATURE NEXT STATE SHOW ON EACH ENCODER CHANGE
        // encFbData[b][e].millisStateUpdate = 0; // FEATURE NEXT STATE SHOW ON EACH ENCODER CHANGE
        if(!(e%16))  encFbData[b][e].vumeterValue = 127;
        else  encFbData[b][e].vumeterValue = random(127);
  //      encFbData[b][e].ringStateIndex = 0;
        encFbData[b][e].colorIndexPrev = 0;
      }
    }
    if(nDigitals){
      digFbData[b] = (digFeedbackData*) memHost->AllocateRAM(nDigitals*sizeof(digFeedbackData));

      for (uint16_t d = 0; d < nDigitals; d++) {
        // digFbData[b][d].digitalFbValue = 0;
        digFbData[b][d].colorIndexPrev = 0;
      }
    }
  } 
}

void FeedbackClass::InitFb(){
  // POWER MANAGEMENT - READ FROM POWER PIN, IF POWER SUPPLY IS PRESENT AND SET LED BRIGHTNESS ACCORDINGLY
  feedbackHw.SendCommand(CMD_ALL_LEDS_OFF);
  delay(10);
    
  if(digitalRead(externalVoltagePin)){
    if(nEncoders >= 28)  currentBrightness = BRIGHTNESS_WOP_32_ENC;
    else                 currentBrightness = BRIGHTNESS_WOP;
  }else{
    currentBrightness = BRIGHTNESS_WITH_POWER;
  }
  // Set External ISR for the power adapter detector pin
  attachInterrupt(digitalPinToInterrupt(externalVoltagePin), ChangeBrigthnessISR, CHANGE);
                            
  InitAuxController(false);

  begun = true;
}

void FeedbackClass::InitAuxController(bool resetHappened){
  bool okToContinue = false;
  byte initFrameIndex = 0;

  // SEND INITIAL VALUES AND LED BRIGHTNESS TO SAMD11
  #define INIT_FRAME_SIZE 7
  byte initFrameArray[INIT_FRAME_SIZE] = {INIT_VALUES, 
                                          nEncoders,
                                          nIndependent,   // CHANGE TO AMOUNT OF ANALOG WITH FEEDBACK
                                          amountOfDigitalInConfig[0],
                                          amountOfDigitalInConfig[1],
                                          currentBrightness,
                                          resetHappened ? 0 : config->board.rainbowOn};
  do{
    SendCommand(initFrameArray[initFrameIndex++]); 

    if(initFrameIndex == INIT_FRAME_SIZE) okToContinue = true;

  }while(!okToContinue);
}

void FeedbackClass::Update() {
  uint32_t antMicrosFbUpdate = micros();
  static unsigned long antMicrosBank = 0;

  if(!begun) return;    // If didn't go through INIT, return;
  
  if(millis()-antMillisWaitMoreData > MAX_WAIT_MORE_DATA_MS){
    waitingMoreData = false;
  }

  if(waitingMoreData || fbShowInProgress) return;

  while (feedbackUpdateReadIdx != feedbackUpdateWriteIdx) {  
    
    // if(((feedbackUpdateWriteIdx - feedbackUpdateReadIdx) > 1 || (feedbackUpdateReadIdx > feedbackUpdateWriteIdx))
    //    && !fbMsgBurstModeOn){
    //   fbMsgBurstModeOn = true;
    //   SendCommand(BURST_INIT);
    //   // SerialUSB.println(F("BURST MODE ON"));
    // }
      
    byte fbUpdateType = feedbackUpdateBuffer[feedbackUpdateReadIdx].type;
    byte fbUpdateQueueIndex = feedbackUpdateReadIdx;
    
    if(++feedbackUpdateReadIdx >= FEEDBACK_UPDATE_BUFFER_SIZE)  
      feedbackUpdateReadIdx = 0;

    SendCommand(BURST_INIT);
    fbMsgBurstModeOn = true;

    switch(fbUpdateType){
      case FB_ENCODER:
      case FB_2CC:
      case FB_ENCODER_SWITCH:
      case FB_ENC_VUMETER:{
        FillFrameWithEncoderData(fbUpdateQueueIndex);
        SendDataIfReady();
        // SerialUSB.println("Encoder feedback update");
      }break;
      case FB_DIGITAL:{
        FillFrameWithDigitalData(fbUpdateQueueIndex);
        SendDataIfReady();
        // SerialUSB.println("Digital feedback update");
      }break;
      case FB_ANALOG:{
        
      }break;
      case FB_INDEPENDENT:{
        
      }break;
      case FB_BANK_CHANGED:{
        // A bank change consists of several states:
        // First we update the encoders
        // Then the DIGITAL 1 port
        // Then, if necessary, the DIGITAL 2 port
        // 9ms para cambiar el banco - 32 encoders, 0 dig, 0 analog - 16/7/2009
        antMicrosBank = micros();
        updatingBankFeedback = true;

        feedbackHw.SendCommand(BURST_INIT); // Sending this 2 times

        // Update all rotary encoders
        for(uint8_t n = 0; n < nEncoders; n++){
          // If it's configured as vumeter encoder, send vumeter feedback first and then value indicator
          if(encoder[n].rotaryFeedback.message == rotaryMessageTypes::rotary_msg_vu_cc){
            SetChangeEncoderFeedback(FB_ENC_VUMETER, n, encFbData[currentBank][n].vumeterValue, 
                                                        encoderHw.GetModuleOrientation(n/4), NO_SHIFTER, BANK_UPDATE);   // HARDCODE: N° of encoders in module / is   
            SetChangeEncoderFeedback(FB_2CC, n, encoderHw.GetEncoderValue(n), 
                                                encoderHw.GetModuleOrientation(n/4), NO_SHIFTER, BANK_UPDATE);   // HARDCODE: N° of encoders in module / is 
          }else{
            SetChangeEncoderFeedback(FB_ENCODER, n, encoderHw.GetEncoderValue(n), 
                                                    encoderHw.GetModuleOrientation(n/4), NO_SHIFTER, BANK_UPDATE);   // HARDCODE: N° of encoders in module / is 
            // If it's a double CC encoder, update second CC after first
            if(encoder[n].switchConfig.mode == switchModes::switch_mode_2cc){
              SetChangeEncoderFeedback(FB_2CC, n, encoderHw.GetEncoderValue2(n), 
                                                  encoderHw.GetModuleOrientation(n/4), NO_SHIFTER, BANK_UPDATE);   // HARDCODE: N° of encoders in module / is                                                   
            }            
          }
        }
        // Update all encoder switches that aren't shifters
        for(uint8_t n = 0; n < nEncoders; n++){
          bool isShifter = false;
          // Is it a shifter?
          if(config->banks.count > 1){
            for(int bank = 0; bank < config->banks.count; bank++){
              byte bankShifterIndex = config->banks.shifterId[bank];
              if(GetHardwareID(ytxIOBLOCK::Encoder, n) == bankShifterIndex){
                isShifter = true;
              }
            }
          }
            
          if(!isShifter)
            SetChangeEncoderFeedback(FB_ENCODER_SWITCH, n, encoderHw.GetEncoderSwitchValue(n), 
                                                           encoderHw.GetModuleOrientation(n/4), NO_SHIFTER, BANK_UPDATE);  // HARDCODE: N° of encoders in module                                                
        }
        SetBankChangeFeedback(FB_BANK_DIGITAL1);
        feedbackHw.SendCommand(BURST_END);
      }break;  
      case FB_BANK_DIGITAL1:{
        feedbackHw.SendCommand(BURST_INIT);
        // Update all digitales that aren't shifters
        if(nDigitals > 128){
          for(uint16_t n = 0; n < 128; n++){
            bool isShifter = false;
            // Is it a shifter?
            if(config->banks.count > 1){
              for(int bank = 0; bank < config->banks.count; bank++){
                byte bankShifterIndex = config->banks.shifterId[bank];
                if(GetHardwareID(ytxIOBLOCK::Digital, n) == bankShifterIndex){
                  isShifter = true;
                }
              }
            }

            if(!isShifter) {
              SetChangeDigitalFeedback(n, digitalHw.GetDigitalValue(n), digitalHw.GetDigitalState(n), NO_SHIFTER, BANK_UPDATE);
            }
          }
          SetBankChangeFeedback(FB_BANK_DIGITAL2);
        }else{
          for(uint16_t n = 0; n < nDigitals; n++){
            bool isShifter = false;

            // Is it a shifter?
            if(config->banks.count > 1){
              for(int bank = 0; bank < config->banks.count; bank++){
                byte bankShifterIndex = config->banks.shifterId[bank];
                if(GetHardwareID(ytxIOBLOCK::Digital, n) == bankShifterIndex){
                  isShifter = true;
                }
              }
            }

            if(!isShifter) {
              SetChangeDigitalFeedback(n, digitalHw.GetDigitalValue(n), digitalHw.GetDigitalState(n), NO_SHIFTER, BANK_UPDATE);
            }
          }

          // Set shifters feedback
          SetShifterFeedback();
        }
        if(bankUpdateFirstTime){
          SetBankChangeFeedback(FB_BANK_CHANGED);        // Double update banks
          bankUpdateFirstTime = false;
          // SerialUSB.println(micros()-antMicrosBank);
          // SerialUSB.println(F("One Time"));
        }
        updatingBankFeedback = false;
        feedbackHw.SendCommand(BURST_END);
      }break;
      case FB_BANK_DIGITAL2:{  
        feedbackHw.SendCommand(BURST_INIT);
        for(uint16_t n = 128; n < nDigitals; n++){
          bool isShifter = false;
          if(config->banks.count > 1){
            for(int bank = 0; bank < config->banks.count; bank++){
              byte bankShifterIndex = config->banks.shifterId[bank];
              if(GetHardwareID(ytxIOBLOCK::Digital, n) == bankShifterIndex){
                isShifter = true;
              }
            }
          }

          if(!isShifter) {
            SetChangeDigitalFeedback(n, digitalHw.GetDigitalValue(n), digitalHw.GetDigitalState(n), NO_SHIFTER, BANK_UPDATE);
          }
        }
        
        SetShifterFeedback();

        feedbackHw.SendCommand(BURST_END);  
        if(bankUpdateFirstTime){
          SerialUSB.println(micros()-antMicrosBank);
          SetBankChangeFeedback(FB_BANK_CHANGED);        // Double update banks
          bankUpdateFirstTime = false;
          // SerialUSB.println(F("One Time"));
        }
        updatingBankFeedback = false;
      }break;
      default: break;
    }
    
  }
  
  // if(micros()-antMicrosFbUpdate > 10000) SerialUSB.println(micros()-antMicrosFbUpdate);
}

void FeedbackClass::SetShifterFeedback(){
  if(config->banks.count > 1){    // If there is more than one bank
    for(int bank = 0; bank < config->banks.count; bank++){
      byte bankShifterIndex = config->banks.shifterId[bank];

      if(currentBank == bank){
        if(bankShifterIndex < config->inputs.encoderCount){              
          SetChangeEncoderFeedback(FB_ENCODER_SWITCH, bankShifterIndex, true, encoderHw.GetModuleOrientation(bankShifterIndex/4), IS_SHIFTER, NO_BANK_UPDATE);  // HARDCODE: N° of encoders in module     
        }else{
          // DIGITAL
          SetChangeDigitalFeedback(bankShifterIndex - (config->inputs.encoderCount), true, true, IS_SHIFTER, NO_BANK_UPDATE);
        }
      }else{
        if(bankShifterIndex < config->inputs.encoderCount){
//              SerialUSB.println(F("FB SWITCH BANK OFF"));
          SetChangeEncoderFeedback(FB_ENCODER_SWITCH, bankShifterIndex, false, encoderHw.GetModuleOrientation(bankShifterIndex/4), IS_SHIFTER, NO_BANK_UPDATE);  // HARDCODE: N° of encoders in module     
        }else{
          // DIGITAL
          SetChangeDigitalFeedback(bankShifterIndex - (config->inputs.encoderCount), false, false, IS_SHIFTER, NO_BANK_UPDATE);
        }
      }
    }
  }
}
void FeedbackClass::FillFrameWithEncoderData(byte updateIndex){
  // FIX FOR SHIFT ROTARY ACTION
  uint8_t colorR = 0, colorG = 0, colorB = 0;
  uint8_t colorIndex = 0;
  uint8_t ringStateIndex= false;
  bool encoderSwitchChanged = false;
  bool isRotaryShifted = false;
  bool isFb2cc = false;
  uint16_t minValue = 0, maxValue = 0;
  uint8_t msgType = 0;
  bool is14bits = false;
  bool onCenterValue = false;
  
  uint8_t indexChanged = feedbackUpdateBuffer[updateIndex].indexChanged;
  uint8_t newOrientation = feedbackUpdateBuffer[updateIndex].newOrientation;
  uint16_t newValue = feedbackUpdateBuffer[updateIndex].newValue;
  uint8_t fbUpdateType = feedbackUpdateBuffer[updateIndex].type;
  bool isShifter = feedbackUpdateBuffer[updateIndex].isShifter;
  bool bankUpdate = feedbackUpdateBuffer[updateIndex].updatingBank;

  // Get state for alternate switch functions
  isRotaryShifted = encoderHw.IsShiftActionOn(indexChanged);
  isFb2cc = (fbUpdateType == FB_2CC);
  
  // Get config info for this encoder
  if(fbUpdateType == FB_ENCODER_SWITCH || isRotaryShifted || isFb2cc){ // If encoder is shifted
    minValue = encoder[indexChanged].switchConfig.parameter[switch_minValue_MSB]<<7 | encoder[indexChanged].switchConfig.parameter[switch_minValue_LSB];
    maxValue = encoder[indexChanged].switchConfig.parameter[switch_maxValue_MSB]<<7 | encoder[indexChanged].switchConfig.parameter[switch_maxValue_LSB];
    msgType = encoder[indexChanged].switchConfig.message;
  }else{
    minValue = encoder[indexChanged].rotaryConfig.parameter[rotary_minMSB]<<7 | encoder[indexChanged].rotaryConfig.parameter[rotary_minLSB];
    maxValue = encoder[indexChanged].rotaryConfig.parameter[rotary_maxMSB]<<7 | encoder[indexChanged].rotaryConfig.parameter[rotary_maxLSB];
    msgType = encoder[indexChanged].rotaryConfig.message;
  }
  
  // IF NOT 14 BITS, USE LOWER PART FOR MIN AND MAX
  if( msgType == rotary_msg_nrpn || msgType == rotary_msg_rpn || msgType == rotary_msg_pb || 
      msgType == switch_msg_nrpn || msgType == switch_msg_rpn || msgType == switch_msg_pb){
    is14bits = true;      
  }else if(fbUpdateType == FB_ENCODER && msgType == rotary_msg_key){
    minValue = 0;
    maxValue = S_SPOT_SIZE;
  }else{
    minValue = minValue & 0x7F;
    maxValue = maxValue & 0x7F;
  }

  bool invert = false;
  uint16_t lowerValue = minValue;
  uint16_t higherValue = maxValue;
  if(minValue > maxValue){
    invert = true;
    lowerValue = maxValue;
    higherValue = minValue;
  }

  uint8_t rotaryMode = encoder[indexChanged].rotaryFeedback.mode;
  
  if((fbUpdateType == FB_ENC_VUMETER && encoder[indexChanged].rotaryFeedback.message == rotaryMessageTypes::rotary_msg_vu_cc) ||
     (fbUpdateType == FB_ENCODER && msgType == rotaryMessageTypes::rotary_msg_key)){
    rotaryMode = encoderRotaryFeedbackMode::fb_spot;
  }

  if(fbUpdateType == FB_ENCODER){
    switch(rotaryMode){
      case encoderRotaryFeedbackMode::fb_spot: {
        uint16_t fbStep = abs(maxValue-minValue);
        fbStep =  fbStep/S_SPOT_SIZE;
        if(fbStep){
          for(int step = 0; step < S_SPOT_SIZE-1; step++){
            if((newValue >= lowerValue + step*fbStep) && (newValue <= lowerValue + (step+1)*fbStep)){
              ringStateIndex = invert ? (S_SPOT_SIZE-1 - step) : step;
            }else if(newValue > lowerValue + (step+1)*fbStep){
              ringStateIndex = invert ? 0 : S_SPOT_SIZE-1;
            }
          }
        }else{
          if(!invert)
            ringStateIndex = mapl(newValue, lowerValue, higherValue, 0, S_SPOT_SIZE-1);
          else
            ringStateIndex = mapl(newValue, lowerValue, higherValue, S_SPOT_SIZE-1, 0);
        }  
                                                                         
        encFbData[currentBank][indexChanged].encRingState &= newOrientation ? ENCODER_SWITCH_V_ON : ENCODER_SWITCH_H_ON;
        encFbData[currentBank][indexChanged].encRingState |= pgm_read_word(&simpleSpot[newOrientation][ringStateIndex]);
      }
      break;
      case encoderRotaryFeedbackMode::fb_fill: {
        uint16_t fbStep = abs(maxValue-minValue);
        fbStep =  fbStep/FILL_SIZE;
        if(fbStep){
          for(int step = 0; step < FILL_SIZE-1; step++){
            if((newValue >= lowerValue + step*fbStep) && (newValue <= lowerValue + (step+1)*fbStep)){
              ringStateIndex = invert ? (FILL_SIZE-1 - step) : step;
            }else if(newValue > lowerValue + (step+1)*fbStep){
              ringStateIndex = invert ? 0 : FILL_SIZE-1;
            }
          }
        }else{
          if(!invert)
            ringStateIndex = mapl(newValue, lowerValue, higherValue, 0, FILL_SIZE-1);
          else
            ringStateIndex = mapl(newValue, lowerValue, higherValue, FILL_SIZE-1, 0);
        }

        encFbData[currentBank][indexChanged].encRingState &= newOrientation ? ENCODER_SWITCH_V_ON : ENCODER_SWITCH_H_ON;
        encFbData[currentBank][indexChanged].encRingState |= pgm_read_word(&fill[newOrientation][ringStateIndex]);
      }
      break;
      case encoderRotaryFeedbackMode::fb_pivot: {
        uint16_t fbStep = abs(maxValue-minValue);
        fbStep =  fbStep/PIVOT_SIZE;
        if(fbStep){
          for(int step = 0; step < PIVOT_SIZE-1; step++){
            if((newValue >= lowerValue + step*fbStep) && (newValue <= lowerValue + (step+1)*fbStep)){
              ringStateIndex = invert ? (PIVOT_SIZE-1 - step) : step;
            }else if(newValue > lowerValue + (step+1)*fbStep){
              ringStateIndex = invert ? 0 : PIVOT_SIZE-1;
            }
          }
        }else{
          if(!invert)
            ringStateIndex = mapl(newValue, lowerValue, higherValue, 0, PIVOT_SIZE-1);
          else
            ringStateIndex = mapl(newValue, lowerValue, higherValue, PIVOT_SIZE-1, 0);
        }  
        encFbData[currentBank][indexChanged].encRingState &= newOrientation ? ENCODER_SWITCH_V_ON : ENCODER_SWITCH_H_ON;
        encFbData[currentBank][indexChanged].encRingState |= pgm_read_word(&pivot[newOrientation][ringStateIndex]);

        uint16_t centerValue = 0;
        if((minValue+maxValue)%2)   centerValue = (minValue+maxValue+1)/2;
        else                        centerValue = (minValue+maxValue)/2;
        if(newValue == centerValue) onCenterValue = true;    // Flag center value to change color
      }
      break;
      case encoderRotaryFeedbackMode::fb_mirror: {
        uint16_t fbStep = abs(maxValue-minValue);
        fbStep =  fbStep/MIRROR_SIZE;
        if(fbStep){
          for(int step = 0; step < MIRROR_SIZE-1; step++){
            if((newValue >= lowerValue + step*fbStep) && (newValue <= lowerValue + (step+1)*fbStep)){
              ringStateIndex = invert ? (MIRROR_SIZE-1 - step) : step;
            }else if(newValue > lowerValue + (step+1)*fbStep){
              ringStateIndex = invert ? 0 : MIRROR_SIZE-1;
            }
          }
        }else{
          if(!invert)
            ringStateIndex = mapl(newValue, lowerValue, higherValue, 0, MIRROR_SIZE-1);
          else
            ringStateIndex = mapl(newValue, lowerValue, higherValue, MIRROR_SIZE-1, 0);
        }
        encFbData[currentBank][indexChanged].encRingState &= newOrientation ? ENCODER_SWITCH_V_ON : ENCODER_SWITCH_H_ON;
        encFbData[currentBank][indexChanged].encRingState |= pgm_read_word(&spread[newOrientation][ringStateIndex]);
      }
      break;
      default: break;
    }

    // FEATURE NEXT STATE SHOW ON EACH ENCODER CHANGE
    // if(encFbData[currentBank][indexChanged].encRingStatePrev == encFbData[currentBank][indexChanged].encRingState){
    //   encFbData[currentBank][indexChanged].nextStateOn = true;
    //   encFbData[currentBank][indexChanged].millisStateUpdate = millis();
    // }

    // If encoder isn't shifted, use rotary feedback data to get color, otherwise use switch feedback data
    if(!isRotaryShifted){ 
      if(onCenterValue){
        colorR = pgm_read_byte(&gamma8[255-encoder[indexChanged].rotaryFeedback.color[R_INDEX]]);
        colorG = pgm_read_byte(&gamma8[255-encoder[indexChanged].rotaryFeedback.color[G_INDEX]]);
        colorB = pgm_read_byte(&gamma8[255-encoder[indexChanged].rotaryFeedback.color[B_INDEX]]);
      }else{
        colorR = pgm_read_byte(&gamma8[encoder[indexChanged].rotaryFeedback.color[R_INDEX]]);
        colorG = pgm_read_byte(&gamma8[encoder[indexChanged].rotaryFeedback.color[G_INDEX]]);
        colorB = pgm_read_byte(&gamma8[encoder[indexChanged].rotaryFeedback.color[B_INDEX]]);
      }
    }else{
      if(onCenterValue){
        colorR = pgm_read_byte(&gamma8[255-encoder[indexChanged].switchFeedback.color[R_INDEX]]);
        colorG = pgm_read_byte(&gamma8[255-encoder[indexChanged].switchFeedback.color[G_INDEX]]);
        colorB = pgm_read_byte(&gamma8[255-encoder[indexChanged].switchFeedback.color[B_INDEX]]);
      }else{
        colorR = pgm_read_byte(&gamma8[encoder[indexChanged].switchFeedback.color[R_INDEX]]);
        colorG = pgm_read_byte(&gamma8[encoder[indexChanged].switchFeedback.color[G_INDEX]]);
        colorB = pgm_read_byte(&gamma8[encoder[indexChanged].switchFeedback.color[B_INDEX]]);
      }
    }
      
  }else if (fbUpdateType == FB_2CC){  // Feedback for double CC and for vumeter indicator
    uint16_t fbStep = abs(maxValue-minValue);
    fbStep =  fbStep/S_SPOT_SIZE;
    for(int step = 0; step < S_SPOT_SIZE-1; step++){
      if((newValue >= lowerValue + step*fbStep) && (newValue <= lowerValue + (step+1)*fbStep)){
        ringStateIndex = invert ? (FILL_SIZE-1 - step) : step;
      }else if(newValue > lowerValue + (step+1)*fbStep){
        ringStateIndex = invert ? 0 : FILL_SIZE-1;
      }
    }           
    encFbData[currentBank][indexChanged].encRingState &= newOrientation ? ENCODER_SWITCH_V_ON : ENCODER_SWITCH_H_ON;
    if(invert)  encFbData[currentBank][indexChanged].encRingState |= pgm_read_word(&simpleSpotInv[newOrientation][ringStateIndex]);
    else        encFbData[currentBank][indexChanged].encRingState |= pgm_read_word(&simpleSpot[newOrientation][ringStateIndex]);
    
    if(encoder[indexChanged].rotaryFeedback.message == rotaryMessageTypes::rotary_msg_vu_cc){
      colorR = pgm_read_byte(&gamma8[pgm_read_byte(&colorRangeTable[127][R_INDEX])]);  // whte for vumeter overlay indicator
      colorG = pgm_read_byte(&gamma8[pgm_read_byte(&colorRangeTable[127][G_INDEX])]);
      colorB = pgm_read_byte(&gamma8[pgm_read_byte(&colorRangeTable[127][B_INDEX])]);
    }else if(newValue == feedbackUpdateBuffer[(updateIndex != 0) ? (updateIndex-1) : (FEEDBACK_UPDATE_BUFFER_SIZE-1)].newValue){    // Ring buffer, catch the event where the two vumeter messages are first and last
      colorR = pgm_read_byte(&gamma8[255-encoder[indexChanged].rotaryFeedback.color[B_INDEX]]);
      colorG = pgm_read_byte(&gamma8[255-encoder[indexChanged].rotaryFeedback.color[R_INDEX]]);
      colorB = pgm_read_byte(&gamma8[255-encoder[indexChanged].rotaryFeedback.color[G_INDEX]]);
    }else{
      colorR = pgm_read_byte(&gamma8[255-encoder[indexChanged].rotaryFeedback.color[R_INDEX]]);
      colorG = pgm_read_byte(&gamma8[255-encoder[indexChanged].rotaryFeedback.color[G_INDEX]]);
      colorB = pgm_read_byte(&gamma8[255-encoder[indexChanged].rotaryFeedback.color[B_INDEX]]);
    }
    
  }else if (fbUpdateType == FB_ENC_VUMETER){  // Feedback type for vumeter visualization on encoder ring
    ringStateIndex = mapl(newValue, 
                          minValue,           // might need to be hardcoded 0 
                          maxValue,           // might need to be hardcoded 127
                          0, 
                          FILL_SIZE - 1);
                                                    
    encFbData[currentBank][indexChanged].encRingState &= newOrientation ? ENCODER_SWITCH_V_ON : ENCODER_SWITCH_H_ON;
    encFbData[currentBank][indexChanged].encRingState |= pgm_read_word(&fill[newOrientation][ringStateIndex]);
    
    colorR = 0;  // Color for this feedback type is on SAMD11
    colorG = 0;
    colorB = 0;
    
  }else if (fbUpdateType == FB_ENCODER_SWITCH) {  // Feedback for encoder switch  
    bool switchState    = (newValue == minValue) ? 0 : 1 ;   // if new value is minValue, state of LED is OFF, otherwise is ON
    bool is2cc          = (encoder[indexChanged].switchConfig.mode == switchModes::switch_mode_2cc);
    bool isFineAdj      = (encoder[indexChanged].switchConfig.mode == switchModes::switch_mode_fine);
    bool isQSTB         = (encoder[indexChanged].switchConfig.mode == switchModes::switch_mode_quick_shift) || 
                          (encoder[indexChanged].switchConfig.mode == switchModes::switch_mode_quick_shift_note);
    bool isShiftRotary  = (encoder[indexChanged].switchConfig.mode == switchModes::switch_mode_shift_rot);
    bool encoderSwitchState = encoderHw.GetEncoderSwitchState(indexChanged);

    if((is2cc || isQSTB || isFineAdj || isShiftRotary) && !isShifter){    // Any encoder special function has a white ON state
      if(encoderSwitchState)  encFbData[currentBank][indexChanged].encRingState |=  (newOrientation ? ENCODER_SWITCH_V_ON : ENCODER_SWITCH_H_ON);
      else                    encFbData[currentBank][indexChanged].encRingState &= ~(newOrientation ? ENCODER_SWITCH_V_ON : ENCODER_SWITCH_H_ON);

      // SerialUSB.print(F("ENCODER SWITCH ")); SerialUSB.print(indexChanged); SerialUSB.print(F(" - STATE: ")); SerialUSB.println(encoderSwitchState);
      // White colour for switch special functions
      colorR = pgm_read_byte(&gamma8[encoderSwitchState ? 220 : 0]);
      colorG = pgm_read_byte(&gamma8[encoderSwitchState ? 220 : 0]);
      colorB = pgm_read_byte(&gamma8[encoderSwitchState ? 220 : 0]);
      encoderSwitchChanged = true;
    }else if(encoder[indexChanged].switchFeedback.colorRangeEnable && !isShifter ){     // If color range is configured, get color from value
      encFbData[currentBank][indexChanged].encRingState |= (newOrientation ? ENCODER_SWITCH_V_ON : ENCODER_SWITCH_H_ON);
      colorIndex = newValue;

      if(newValue <= 127)       // Safe guard
        colorIndex = newValue;

      if(colorIndex != encFbData[currentBank][indexChanged].colorIndexPrev || bankUpdate){
        encoderSwitchChanged = true;
        // SerialUSB.print(indexChanged); SerialUSB.print(" color changed. Color index: "); SerialUSB.println(colorIndex);
        encFbData[currentBank][indexChanged].colorIndexPrev = colorIndex;
        colorR = pgm_read_byte(&gamma8[pgm_read_byte(&colorRangeTable[colorIndex][R_INDEX])]);
        colorG = pgm_read_byte(&gamma8[pgm_read_byte(&colorRangeTable[colorIndex][G_INDEX])]);
        colorB = pgm_read_byte(&gamma8[pgm_read_byte(&colorRangeTable[colorIndex][B_INDEX])]);
      }
    }else{   // No color range, no special function, might be normal encoder switch or shifter button
      if(switchState){      // ON
        encFbData[currentBank][indexChanged].encRingState |= (newOrientation ? ENCODER_SWITCH_V_ON : ENCODER_SWITCH_H_ON);
        colorR = pgm_read_byte(&gamma8[encoder[indexChanged].switchFeedback.color[R_INDEX]]);
        colorG = pgm_read_byte(&gamma8[encoder[indexChanged].switchFeedback.color[G_INDEX]]);
        colorB = pgm_read_byte(&gamma8[encoder[indexChanged].switchFeedback.color[B_INDEX]]);    
      }else if(!switchState && isShifter){    // SHIFTER, OFF
        encFbData[currentBank][indexChanged].encRingState |= (newOrientation ? ENCODER_SWITCH_V_ON : ENCODER_SWITCH_H_ON);    // If it's a bank shifter, switch LED's are on
        if(IsPowerConnected()){    // POWER SUPPLY CONNECTED
          colorR = pgm_read_byte(&gamma8[encoder[indexChanged].switchFeedback.color[R_INDEX]*BANK_OFF_BRIGHTNESS_FACTOR_WP]);
          colorG = pgm_read_byte(&gamma8[encoder[indexChanged].switchFeedback.color[G_INDEX]*BANK_OFF_BRIGHTNESS_FACTOR_WP]);
          colorB = pgm_read_byte(&gamma8[encoder[indexChanged].switchFeedback.color[B_INDEX]*BANK_OFF_BRIGHTNESS_FACTOR_WP]);  
        }else{                     // NO POWER SUPPLY 
          colorR = pgm_read_byte(&gamma8[encoder[indexChanged].switchFeedback.color[R_INDEX]*BANK_OFF_BRIGHTNESS_FACTOR_WOP]);
          colorG = pgm_read_byte(&gamma8[encoder[indexChanged].switchFeedback.color[G_INDEX]*BANK_OFF_BRIGHTNESS_FACTOR_WOP]);
          colorB = pgm_read_byte(&gamma8[encoder[indexChanged].switchFeedback.color[B_INDEX]*BANK_OFF_BRIGHTNESS_FACTOR_WOP]);
        }
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
        || fbUpdateType == FB_2CC
        || fbUpdateType == FB_ENC_VUMETER
        || updatingBankFeedback 
        || encoderSwitchChanged) {
    encFbData[currentBank][indexChanged].encRingStatePrev = encFbData[currentBank][indexChanged].encRingState;    // not being used
    
    sendSerialBufferDec[d_frameType] = (fbUpdateType == FB_ENCODER        ? ENCODER_CHANGE_FRAME  :
                                        fbUpdateType == FB_2CC            ? ENCODER_DOUBLE_FRAME  : 
                                        fbUpdateType == FB_ENC_VUMETER    ? ENCODER_VUMETER_FRAME  : 
                                        fbUpdateType == FB_ENCODER_SWITCH ? ENCODER_SWITCH_CHANGE_FRAME : 255);   
    
    sendSerialBufferDec[d_nRing] = indexChanged;
    sendSerialBufferDec[d_orientation] = newOrientation;
    sendSerialBufferDec[d_ringStateH] = encFbData[currentBank][indexChanged].encRingState >> 8;
    sendSerialBufferDec[d_ringStateL] = encFbData[currentBank][indexChanged].encRingState & 0xff;
    // sendSerialBufferDec[d_currentValue] = newValue;
    // sendSerialBufferDec[d_fbMin] = minValue;
    // sendSerialBufferDec[d_fbMax] = maxValue;
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
  bool isShifter = feedbackUpdateBuffer[updateIndex].isShifter;
  bool newState = feedbackUpdateBuffer[updateIndex].newOrientation;
  bool bankUpdate = feedbackUpdateBuffer[updateIndex].updatingBank;
  
  if(digital[indexChanged].feedback.colorRangeEnable && !isShifter){
    colorIndex = newValue;
    
    colorR = pgm_read_byte(&gamma8[pgm_read_byte(&colorRangeTable[colorIndex][R_INDEX])]);
    colorG = pgm_read_byte(&gamma8[pgm_read_byte(&colorRangeTable[colorIndex][G_INDEX])]);
    colorB = pgm_read_byte(&gamma8[pgm_read_byte(&colorRangeTable[colorIndex][B_INDEX])]);
  }else{     
    if(newValue){
      colorR = pgm_read_byte(&gamma8[digital[indexChanged].feedback.color[R_INDEX]]);
      colorG = pgm_read_byte(&gamma8[digital[indexChanged].feedback.color[G_INDEX]]);
      colorB = pgm_read_byte(&gamma8[digital[indexChanged].feedback.color[B_INDEX]]);   
    }else if(!newValue && !isShifter){
      colorR = 0;
      colorG = 0;
      colorB = 0;
    }else if(!newValue && isShifter){
      if(IsPowerConnected()){
        colorR = pgm_read_byte(&gamma8[digital[indexChanged].feedback.color[R_INDEX]*BANK_OFF_BRIGHTNESS_FACTOR_WP]);
        colorG = pgm_read_byte(&gamma8[digital[indexChanged].feedback.color[G_INDEX]*BANK_OFF_BRIGHTNESS_FACTOR_WP]);
        colorB = pgm_read_byte(&gamma8[digital[indexChanged].feedback.color[B_INDEX]*BANK_OFF_BRIGHTNESS_FACTOR_WP]);  
      }else{
        colorR = pgm_read_byte(&gamma8[digital[indexChanged].feedback.color[R_INDEX]*BANK_OFF_BRIGHTNESS_FACTOR_WOP]);
        colorG = pgm_read_byte(&gamma8[digital[indexChanged].feedback.color[G_INDEX]*BANK_OFF_BRIGHTNESS_FACTOR_WOP]);
        colorB = pgm_read_byte(&gamma8[digital[indexChanged].feedback.color[B_INDEX]*BANK_OFF_BRIGHTNESS_FACTOR_WOP]);
      }    
    }
  }
  
  //sendSerialBufferDec[msgLength] = TX_BYTES;   // INIT SERIAL FRAME WITH CONSTANT DATA
  sendSerialBufferDec[d_frameType] = (indexChanged < amountOfDigitalInConfig[0]) ?  DIGITAL1_CHANGE_FRAME : 
                                                                                    DIGITAL2_CHANGE_FRAME;   
  sendSerialBufferDec[d_nDig] = indexChanged;
  sendSerialBufferDec[d_orientation] = 0;
  sendSerialBufferDec[d_digitalState] = isShifter ? 1 : (newValue ? 1 : 0);
  sendSerialBufferDec[d_ringStateL] = 0;
  sendSerialBufferDec[d_R] = colorR;
  sendSerialBufferDec[d_G] = colorG;
  sendSerialBufferDec[d_B] = colorB;
  sendSerialBufferDec[d_ENDOFFRAME] = END_OF_FRAME_BYTE;
  feedbackDataToSend = true;
}

uint8_t FeedbackClass::GetVumeterValue(uint8_t encNo){
  if(encNo < nEncoders)
    return encFbData[currentBank][encNo].vumeterValue;
}

void FeedbackClass::SetChangeEncoderFeedback(uint8_t type, uint8_t encIndex, uint16_t val, uint8_t encoderOrientation, bool isShifter, bool bankUpdate) {
  feedbackUpdateBuffer[feedbackUpdateWriteIdx].type = type;
  feedbackUpdateBuffer[feedbackUpdateWriteIdx].indexChanged = encIndex;
  feedbackUpdateBuffer[feedbackUpdateWriteIdx].newValue = val;
  if(type == FB_ENC_VUMETER)  encFbData[currentBank][encIndex].vumeterValue = val;
  feedbackUpdateBuffer[feedbackUpdateWriteIdx].newOrientation = encoderOrientation;
  feedbackUpdateBuffer[feedbackUpdateWriteIdx].isShifter = isShifter;
  feedbackUpdateBuffer[feedbackUpdateWriteIdx].updatingBank = bankUpdate;

  waitingMoreData = true;
  antMillisWaitMoreData = millis();
  
  if(++feedbackUpdateWriteIdx >= FEEDBACK_UPDATE_BUFFER_SIZE)  
    feedbackUpdateWriteIdx = 0;
}

void FeedbackClass::SetChangeDigitalFeedback(uint16_t digitalIndex, uint16_t updateValue, bool hwState, bool isShifter, bool bankUpdate){
    feedbackUpdateBuffer[feedbackUpdateWriteIdx].type = FB_DIGITAL;
    feedbackUpdateBuffer[feedbackUpdateWriteIdx].indexChanged = digitalIndex;
    feedbackUpdateBuffer[feedbackUpdateWriteIdx].newValue = updateValue;
    feedbackUpdateBuffer[feedbackUpdateWriteIdx].newOrientation = hwState;
    feedbackUpdateBuffer[feedbackUpdateWriteIdx].isShifter = isShifter;
    feedbackUpdateBuffer[feedbackUpdateWriteIdx].updatingBank = bankUpdate;
    
    waitingMoreData = true;
    antMillisWaitMoreData = millis();

    if(++feedbackUpdateWriteIdx >= FEEDBACK_UPDATE_BUFFER_SIZE){
      feedbackUpdateWriteIdx = 0;
    }
}

void FeedbackClass::SetChangeIndependentFeedback(uint8_t type, uint16_t fbIndex, uint16_t val, bool bankUpdate){
  feedbackUpdateBuffer[feedbackUpdateWriteIdx].type = type;
  feedbackUpdateBuffer[feedbackUpdateWriteIdx].indexChanged = fbIndex;
  feedbackUpdateBuffer[feedbackUpdateWriteIdx].newValue = val;
  feedbackUpdateBuffer[feedbackUpdateWriteIdx].updatingBank = bankUpdate;

  if(++feedbackUpdateWriteIdx >= FEEDBACK_UPDATE_BUFFER_SIZE)  
      feedbackUpdateWriteIdx = 0;
}

void FeedbackClass::SetBankChangeFeedback(uint8_t type){
  feedbackUpdateBuffer[feedbackUpdateWriteIdx].type = type;
  
  if(++feedbackUpdateWriteIdx >= FEEDBACK_UPDATE_BUFFER_SIZE)  
      feedbackUpdateWriteIdx = 0;

}

void FeedbackClass::SendDataIfReady(){
  if(feedbackDataToSend){
    SendFeedbackData(); 
    feedbackDataToSend = false;
  }
  if((feedbackUpdateWriteIdx == feedbackUpdateReadIdx) && fbMsgBurstModeOn){
    fbMsgBurstModeOn = false; 
    Serial.write(BURST_END);               // SEND BANK END if burst mode was enabled
  }
}

void FeedbackClass::AddCheckSum(){ 
  uint16_t sum = 2019 - checkSum(sendSerialBufferEnc, e_B+1);
  
  sum &= 0x3FFF;    // 14 bit checksum
  
  sendSerialBufferEnc[e_checkSum_MSB] = (sum >> 7) & 0x7F;
  sendSerialBufferEnc[e_checkSum_LSB] = sum & 0x7F;
}

// #define DEBUG_FB_FRAME
void FeedbackClass::SendFeedbackData(){
  unsigned long serialTimeout = millis();
  byte tries = 0;
  bool okToContinue = false;
  byte cmd = 0;
  static uint32_t ackNotReceivedCount = 0;

  byte encodedFrameSize = encodeSysEx(sendSerialBufferDec, sendSerialBufferEnc, d_ENDOFFRAME);
  
  // Adds checksum bytes to encoded frame
  AddCheckSum();

  // SerialUSB.print(F("FRAME WITHOUT ENCODING:\n"));
  // for(int i = 0; i <= d_B; i++){
  //   SerialUSB.print(i); SerialUSB.print(F(": "));SerialUSB.println(sendSerialBufferDec[i]);
  // }  
  
  #ifdef DEBUG_FB_FRAME
  SerialUSB.print(F("FRAME WITHOUT ENCODING:\n"));
  for(int i = 0; i <= d_B; i++){
    SerialUSB.print(i); SerialUSB.print(F(": "));SerialUSB.println(sendSerialBufferDec[i]);
  }
  // SerialUSB.print(F("ENCODED FRAME:\n"));
  // for(int i = 0; i < encodedFrameSize; i++){
  //   SerialUSB.print(i); SerialUSB.print(F(": "));SerialUSB.println(sendSerialBufferEnc[i]);
  // }
  SerialUSB.println(F("******************************************"));
  SerialUSB.println(F("Serial DATA: "));
  #endif
  
  do{
    cmd = 0;
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
    }
    Serial.write(END_OF_FRAME_BYTE);                         // SEND END OF FRAME BYTE
    
    Serial.flush();    

    // if(!fbMsgBurstModeOn){
    //   SendCommand(SHOW_IN_PROGRESS);
    // }

    #ifdef DEBUG_FB_FRAME
    SerialUSB.println(END_OF_FRAME_BYTE);
    SerialUSB.println(F("******************************************"));
    #endif
    
    // Wait 1ms for fb microcontroller to acknowledge message reception, or try again
    uint32_t antMicrosAck = micros();
    while(!Serial.available() && ((micros() - antMicrosAck) < 400));

    if(Serial.available()){
      cmd = Serial.read();

      if(cmd == ACK_CMD){
        okToContinue = true;
      }else{
        ackNotReceivedCount++;
        tries++;
        SerialUSB.print("total ack not received: ");
        SerialUSB.print(ackNotReceivedCount);
        SerialUSB.print(" times\t\tNACK: ");
        SerialUSB.print(cmd);
        SerialUSB.print("\t");
        SerialUSB.print(micros() - antMicrosAck);
        SerialUSB.print("\t");
        SerialUSB.print(sendSerialBufferDec[d_frameType]);
        SerialUSB.print(",");
        SerialUSB.print(sendSerialBufferDec[d_nRing]);
        SerialUSB.print("\t");
        SerialUSB.print(feedbackUpdateReadIdx);
        SerialUSB.print("\t");
        SerialUSB.println(feedbackUpdateWriteIdx);
      }
    }else{
      ackNotReceivedCount++;
      tries++;
      SerialUSB.print("total ack not received: ");
      SerialUSB.print(ackNotReceivedCount);
      SerialUSB.print("\t");
      SerialUSB.print(micros() - antMicrosAck);
      SerialUSB.print("\t");
      SerialUSB.print(sendSerialBufferDec[d_frameType]);
      SerialUSB.print(",");
      SerialUSB.print(sendSerialBufferDec[d_nRing]);
      SerialUSB.print("\t");
      SerialUSB.print(feedbackUpdateReadIdx);
      SerialUSB.print("\t");
      SerialUSB.println(feedbackUpdateWriteIdx);
    }
    #ifdef DEBUG_FB_FRAME
    SerialUSB.print(F("ACK: ")); SerialUSB.print(cmd);
    SerialUSB.println(F("******************************************"));
    #endif    
  }while(!okToContinue && tries < 20 && !fbShowInProgress);

}

void FeedbackClass::SendCommand(uint8_t cmd){
//  SerialUSB.print(F("Command sent: "));
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
      
