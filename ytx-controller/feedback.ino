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
  waitingBulk = false;
  antMillisWaitBulk = 0;

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

void FeedbackClass::InitFbPower(){
  // POWER MANAGEMENT - READ FROM POWER PIN, IF POWER SUPPLY IS PRESENT AND SET LED BRIGHTNESS ACCORDINGLY
  feedbackHw.SendCommand(CMD_ALL_LEDS_OFF);
  delay(10);
  
  bool okToContinue = false;
  byte initFrameIndex = 0;
  
  if(digitalRead(externalVoltagePin)){
    if(nEncoders >= 28)  currentBrightness = BRIGHTNESS_WOP_32_ENC;
    else                 currentBrightness = BRIGHTNESS_WOP;
  }else{
    currentBrightness = BRIGHTNESS_WITH_POWER;
  }
  // Set External ISR for the power adapter detector pin
  attachInterrupt(digitalPinToInterrupt(externalVoltagePin), ChangeBrigthnessISR, CHANGE);
  
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
    SendCommand(initFrameArray[initFrameIndex++]); 

    if(initFrameIndex == INIT_FRAME_SIZE) okToContinue = true;

  }while(!okToContinue);

  begun = true;
}

void FeedbackClass::Update() {
 uint32_t antMicrosFbUpdate = micros();
  if(!begun) return;    // If didn't go through INIT, return;
  
  if(millis()-antMillisWaitBulk > MAX_WAIT_BULK_MS){
    waitingBulk = false;
  }

  if(waitingBulk || fbShowInProgress) return;

  
  while (feedbackUpdateReadIdx != feedbackUpdateWriteIdx) {  
    if((feedbackUpdateWriteIdx - feedbackUpdateReadIdx) > 1 && !fbMsgBurstModeOn){
      fbMsgBurstModeOn = true;
      // SerialUSB.println("BURST MODE ON");
    }
      
    byte fbUpdateType = feedbackUpdateBuffer[feedbackUpdateReadIdx].type;
    byte fbUpdateQueueIndex = feedbackUpdateReadIdx;
    
    if(++feedbackUpdateReadIdx >= FEEDBACK_UPDATE_BUFFER_SIZE)  
      feedbackUpdateReadIdx = 0;


    switch(fbUpdateType){
      case FB_ENCODER:
      case FB_2CC:
      case FB_ENCODER_SWITCH:
      case FB_ENC_VUMETER:{
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
        updatingBankFeedback = true;
        
        feedbackHw.SendCommand(BANK_INIT);
        for(uint8_t n = 0; n < nEncoders; n++){
        
          if(encoder[n].rotaryFeedback.message == rotaryMessageTypes::rotary_msg_vu_cc){
            SetChangeEncoderFeedback(FB_ENC_VUMETER, n, encFbData[currentBank][n].vumeterValue, 
                                                        encoderHw.GetModuleOrientation(n/4), NO_SHIFTER, BANK_UPDATE);   // HARDCODE: N° of encoders in module / is   
            SetChangeEncoderFeedback(FB_2CC, n, encoderHw.GetEncoderValue(n), 
                                                encoderHw.GetModuleOrientation(n/4), NO_SHIFTER, BANK_UPDATE);   // HARDCODE: N° of encoders in module / is 
          }else{
            SetChangeEncoderFeedback(FB_ENCODER, n, encoderHw.GetEncoderValue(n), 
                                                    encoderHw.GetModuleOrientation(n/4), NO_SHIFTER, BANK_UPDATE);   // HARDCODE: N° of encoders in module / is 
            if(encoder[n].switchConfig.mode == switchModes::switch_mode_2cc){
              SetChangeEncoderFeedback(FB_2CC, n, encoderHw.GetEncoderValue2(n), 
                                                  encoderHw.GetModuleOrientation(n/4), NO_SHIFTER, BANK_UPDATE);   // HARDCODE: N° of encoders in module / is                                                   
            }            
          }
        }
        for(uint8_t n = 0; n < nEncoders; n++){
          bool isShifter = false;
          for(int bank = 0; bank < config->banks.count; bank++){
            byte bankShifterIndex = config->banks.shifterId[bank];
            if(GetHardwareID(ytxIOBLOCK::Encoder, n) == bankShifterIndex){
              isShifter = true;
            }
          }
          if(!isShifter)
            SetChangeEncoderFeedback(FB_ENCODER_SWITCH, n, encoderHw.GetEncoderSwitchValue(n), 
                                                           encoderHw.GetModuleOrientation(n/4), NO_SHIFTER, BANK_UPDATE);  // HARDCODE: N° of encoders in module                                                
        }
        
        for(uint8_t n = 0; n < nDigitals; n++){
          bool isShifter = false;
          for(int bank = 0; bank < config->banks.count; bank++){
            byte bankShifterIndex = config->banks.shifterId[bank];
            if(GetHardwareID(ytxIOBLOCK::Digital, n) == bankShifterIndex){
              isShifter = true;
            }
          }

          if(!isShifter) 
            SetChangeDigitalFeedback(n, digitalHw.GetDigitalValue(n), digitalHw.GetDigitalState(n), NO_SHIFTER, BANK_UPDATE);
        }

        // for(uint8_t n = 0; n < nIndependent; n++){
          
        // }

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
  //              SerialUSB.println("FB SWITCH BANK OFF");
                SetChangeEncoderFeedback(FB_ENCODER_SWITCH, bankShifterIndex, false, encoderHw.GetModuleOrientation(bankShifterIndex/4), IS_SHIFTER, NO_BANK_UPDATE);  // HARDCODE: N° of encoders in module     
              }else{
                // DIGITAL
                SetChangeDigitalFeedback(bankShifterIndex - config->inputs.encoderCount, false, false, IS_SHIFTER, NO_BANK_UPDATE);
              }
            }
          }
        }

        feedbackHw.SendCommand(BANK_END);

        if(bankUpdateFirstTime){
          SetBankChangeFeedback();
          bankUpdateFirstTime = false;
          // SerialUSB.println("One Time");
        }
        updatingBankFeedback = false;
      }break;
      default: break;
    }
    
  }
  
  // if(micros()-antMicrosFbUpdate > 10000) SerialUSB.println(micros()-antMicrosFbUpdate);
}

void FeedbackClass::FillFrameWithEncoderData(byte updateIndex){
  // FIX FOR SHIFT ROTARY ACTION
  uint8_t colorR = 0, colorG = 0, colorB = 0;
  uint8_t colorIndex = 0;
  bool colorIndexChanged = false;
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
  if(isRotaryShifted || isFb2cc){ // If encoder is shifted
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
  uint16_t vuRingState = 0;
  
  if(fbUpdateType == FB_ENC_VUMETER && encoder[indexChanged].rotaryFeedback.message == rotaryMessageTypes::rotary_msg_vu_cc){
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
    bool switchState = (newValue == minValue) ? 0 : 1 ;   // if new value is minValue, state of LED is OFF, otherwise is ON
    bool is2cc          = (encoder[indexChanged].switchConfig.mode == switchModes::switch_mode_2cc);
    bool isFineAdj      = (encoder[indexChanged].switchConfig.mode == switchModes::switch_mode_fine);
    bool isQSTB         = (encoder[indexChanged].switchConfig.mode == switchModes::switch_mode_quick_shift) || 
                          (encoder[indexChanged].switchConfig.mode == switchModes::switch_mode_quick_shift_note);
    bool isShiftRotary  = (encoder[indexChanged].switchConfig.mode == switchModes::switch_mode_shift_rot);
    
    bool encoderSwitchState = encoderHw.GetEncoderSwitchState(indexChanged);

    if((is2cc || isQSTB || isFineAdj || isShiftRotary) && !isShifter){    // Any encoder special function has a white ON state
      if(encoderSwitchState)  encFbData[currentBank][indexChanged].encRingState |=  (newOrientation ? ENCODER_SWITCH_V_ON : ENCODER_SWITCH_H_ON);
      else                    encFbData[currentBank][indexChanged].encRingState &= ~(newOrientation ? ENCODER_SWITCH_V_ON : ENCODER_SWITCH_H_ON);

      // SerialUSB.print("ENCODER SWITCH "); SerialUSB.print(indexChanged); SerialUSB.print(" - STATE: "); SerialUSB.println(encoderSwitchState);

      colorR = pgm_read_byte(&gamma8[encoderSwitchState ? 220 : 0]);
      colorG = pgm_read_byte(&gamma8[encoderSwitchState ? 220 : 0]);
      colorB = pgm_read_byte(&gamma8[encoderSwitchState ? 220 : 0]);
      encoderSwitchChanged = true;
    }else if(encoder[indexChanged].switchFeedback.colorRangeEnable && !isShifter ){     // If color range is configured, get color from value
      encFbData[currentBank][indexChanged].encRingState |= (newOrientation ? ENCODER_SWITCH_V_ON : ENCODER_SWITCH_H_ON);
      colorIndex = newValue;

      if(newValue <= 127)
        colorIndex = newValue;

      if(colorIndex != encFbData[currentBank][indexChanged].colorIndexPrev || bankUpdate){
        colorIndexChanged = true;
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
        || colorIndexChanged
        || encoderSwitchChanged) {
    encFbData[currentBank][indexChanged].encRingStatePrev = encFbData[currentBank][indexChanged].encRingState;
    
    sendSerialBufferDec[d_frameType] = (fbUpdateType == FB_ENCODER        ? ENCODER_CHANGE_FRAME  :
                                        fbUpdateType == FB_2CC            ? ENCODER_DOUBLE_FRAME  : 
                                        fbUpdateType == FB_ENC_VUMETER    ? ENCODER_VUMETER_FRAME  : 
                                        fbUpdateType == FB_ENCODER_SWITCH ? ENCODER_SWITCH_CHANGE_FRAME : 255);   
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
  bool isShifter = feedbackUpdateBuffer[updateIndex].isShifter;
  bool newState = feedbackUpdateBuffer[updateIndex].newOrientation;
  
  if(digital[indexChanged].feedback.colorRangeEnable && !isShifter){
    colorIndex = newValue;
    // if      (!newValue)                                              colorIndex = digital[indexChanged].feedback.colorRange0;   // VALUE: 0
    // else if (newValue > COLOR_RANGE_0 && newValue <= COLOR_RANGE_1)  colorIndex = digital[indexChanged].feedback.colorRange1;   // VALUE: 1-3
    // else if (newValue > COLOR_RANGE_1 && newValue <= COLOR_RANGE_2)  colorIndex = digital[indexChanged].feedback.colorRange2;   // VALUE: 4-7
    // else if (newValue > COLOR_RANGE_2 && newValue <= COLOR_RANGE_3)  colorIndex = digital[indexChanged].feedback.colorRange3;   // VALUE: 8-15
    // else if (newValue > COLOR_RANGE_3 && newValue <= COLOR_RANGE_4)  colorIndex = digital[indexChanged].feedback.colorRange4;   // VALUE: 16-31
    // else if (newValue > COLOR_RANGE_4 && newValue <= COLOR_RANGE_5)  colorIndex = digital[indexChanged].feedback.colorRange5;   // VALUE: 32-63
    // else if (newValue > COLOR_RANGE_5 && newValue <= COLOR_RANGE_6)  colorIndex = digital[indexChanged].feedback.colorRange6;   // VALUE: 64-126
    // else if (newValue == COLOR_RANGE_7)                              colorIndex = digital[indexChanged].feedback.colorRange7;   // VALUE: 127
  
    colorR = pgm_read_byte(&gamma8[pgm_read_byte(&colorRangeTable[colorIndex][R_INDEX])]);
    colorG = pgm_read_byte(&gamma8[pgm_read_byte(&colorRangeTable[colorIndex][G_INDEX])]);
    colorB = pgm_read_byte(&gamma8[pgm_read_byte(&colorRangeTable[colorIndex][B_INDEX])]);
  }else{     

    if((newState)){
      colorR = pgm_read_byte(&gamma8[digital[indexChanged].feedback.color[R_INDEX]]);
      colorG = pgm_read_byte(&gamma8[digital[indexChanged].feedback.color[G_INDEX]]);
      colorB = pgm_read_byte(&gamma8[digital[indexChanged].feedback.color[B_INDEX]]);   
    }else if(!newState && !isShifter){
      colorR = 0;
      colorG = 0;
      colorB = 0;
    }else if(!newState && isShifter){
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
  sendSerialBufferDec[d_digitalState] = isShifter ? 1 : 
                                                    (digFbData[currentBank][indexChanged].digitalFbValue ? 1 : 0);
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

  waitingBulk = true;
  antMillisWaitBulk = millis();
  
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

    waitingBulk = true;
    antMillisWaitBulk = millis();

    if(++feedbackUpdateWriteIdx >= FEEDBACK_UPDATE_BUFFER_SIZE)  
      feedbackUpdateWriteIdx = 0;

    digFbData[currentBank][digitalIndex].digitalFbValue = updateValue;

}

void FeedbackClass::SetChangeIndependentFeedback(uint8_t type, uint16_t fbIndex, uint16_t val, bool bankUpdate){
  feedbackUpdateBuffer[feedbackUpdateWriteIdx].type = type;
  feedbackUpdateBuffer[feedbackUpdateWriteIdx].indexChanged = fbIndex;
  feedbackUpdateBuffer[feedbackUpdateWriteIdx].newValue = val;
  feedbackUpdateBuffer[feedbackUpdateWriteIdx].updatingBank = bankUpdate;

  if(++feedbackUpdateWriteIdx >= FEEDBACK_UPDATE_BUFFER_SIZE)  
      feedbackUpdateWriteIdx = 0;
}

void FeedbackClass::SetBankChangeFeedback(){
  feedbackUpdateBuffer[feedbackUpdateWriteIdx].type = FB_BANK_CHANGED;
  
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
}

// #define DEBUG_FB_FRAME
void FeedbackClass::SendFeedbackData(){
  unsigned long serialTimeout = millis();
  byte tries = 0;
  bool okToContinue = false;
  byte ack = 0;

  byte encodedFrameSize = encodeSysEx(sendSerialBufferDec, sendSerialBufferEnc, d_ENDOFFRAME);
  
  // Adds checksum bytes to encoded frame
  AddCheckSum();

 // SerialUSB.print("FRAME WITHOUT ENCODING:\n");
 // for(int i = 0; i <= d_B; i++){
 //   SerialUSB.print(i); SerialUSB.print(": ");SerialUSB.println(sendSerialBufferDec[i]);
 // }
#ifdef DEBUG_FB_FRAME
 SerialUSB.print("FRAME WITHOUT ENCODING:\n");
 for(int i = 0; i <= d_B; i++){
   SerialUSB.print(i); SerialUSB.print(": ");SerialUSB.println(sendSerialBufferDec[i]);
 }
 // SerialUSB.print("ENCODED FRAME:\n");
 // for(int i = 0; i < encodedFrameSize; i++){
 //   SerialUSB.print(i); SerialUSB.print(": ");SerialUSB.println(sendSerialBufferEnc[i]);
 // }
 SerialUSB.println("******************************************");
 SerialUSB.println("Serial DATA: ");
  #endif
  do{
    ack = 0;
    if(fbMsgBurstModeOn) Serial.write(BANK_INIT);   // SEND BANK INIT if burst mode is enabled
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
    if((feedbackUpdateWriteIdx - feedbackUpdateReadIdx) <= 1 && fbMsgBurstModeOn){
      fbMsgBurstModeOn = false; 
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

    if(Serial.available()){
      ack = Serial.read();

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
    SerialUSB.println("******************************************");
    #endif    
  }while(!okToContinue && tries < 20);
  // SerialUSB.println(tries);
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
      
