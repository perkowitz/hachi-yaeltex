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


// UTILITIES

bool CheckIfBankShifter(uint16_t index, bool switchState) {
  static bool bankShifterPressed = false;
  static uint8_t prevBank = 0;
  static unsigned long antMicrosBank = 0;
  
  if (config->banks.count > 1) {  // If there is more than one bank
    for (int bank = 0; bank < config->banks.count; bank++) { // Cycle all banks
      if (index == config->banks.shifterId[bank]) {           // If index matches to this bank's shifter
             
        bool toggleBank = ((config->banks.momToggFlags) >> bank) & 1;

        if (switchState && currentBank != bank && !bankShifterPressed) {
//          antMicrosBank = micros(); 
          prevBank = currentBank;                   // save previous bank for momentary bank shifters
          
          currentBank = memHost->LoadBank(bank);    // Load new bank in RAM
          
          bankShifterPressed = true;                // Set flag to indicate there is a bank shifter pressed
          
          SetBankForAll(currentBank);               // Set new bank for components that need it
          
          ScanMidiBufferAndUpdate();                
 
          feedbackHw.SetBankChangeFeedback();
//          SerialUSB.println(micros()-antMicrosBank); 
                   
        } else if (!switchState && currentBank == bank && !toggleBank && bankShifterPressed) {
          //          Bank released. "); SerialUSB.println(F("Momentary."));
          currentBank = memHost->LoadBank(prevBank);
          bankShifterPressed = false;
          
          SetBankForAll(currentBank);
          
          ScanMidiBufferAndUpdate();
          
          feedbackHw.SetBankChangeFeedback();
          //          SerialUSB.print(F("Returned to bank: ")); SerialUSB.println(currentBank);
        } else if (!switchState && currentBank == bank && toggleBank && bankShifterPressed) {
          //          SerialUSB.print("Bank released. "); SerialUSB.println(F("Toggle."));
          bankShifterPressed = false;
        }
        
        return true;
      }
    }
   
  }
  return false;
}


void ScanMidiBufferAndUpdate(){
  for (int idx = 0; idx < midiRxSettings.lastMidiBufferIndex7; idx++) {
    if((midiMsgBuf7[idx].banksToUpdate >> currentBank) & 0x1){
      // Reset bank flag
      midiMsgBuf7[idx].banksToUpdate &= ~(1 << currentBank);
//      SerialUSB.print("PORT: "); SerialUSB.print(midiMsgBuf7[idx].port); 
//      SerialUSB.print("\tCH: "); SerialUSB.print(midiMsgBuf7[idx].channel); 
//      SerialUSB.print("\tMSG: "); SerialUSB.print(midiMsgBuf7[idx].message, HEX); 
//      SerialUSB.print("\tP: "); SerialUSB.print(midiMsgBuf7[idx].parameter); 
//      SerialUSB.print("\tVAL: "); SerialUSB.println(midiMsgBuf7[idx].value); 
      SearchMsgInConfigAndUpdate( midiMsgBuf7[idx].type,
                                  midiMsgBuf7[idx].message,
                                  midiMsgBuf7[idx].channel,
                                  midiMsgBuf7[idx].parameter,
                                  midiMsgBuf7[idx].value,
                                  midiMsgBuf7[idx].port,
                                  true);
    }   
  } 
  for (int idx = 0; idx < midiRxSettings.lastMidiBufferIndex14; idx++) {
    if((midiMsgBuf14[idx].banksToUpdate >> currentBank) & 0x1){
      // Reset bank flag
      midiMsgBuf14[idx].banksToUpdate &= ~(1 << currentBank);
//      SerialUSB.print("PORT: "); SerialUSB.print(midiMsgBuf14[idx].port); 
//      SerialUSB.print("\tCH: "); SerialUSB.print(midiMsgBuf14[idx].channel); 
//      SerialUSB.print("\tMSG: "); SerialUSB.print(midiMsgBuf14[idx].message, HEX); 
//      SerialUSB.print("\tP: "); SerialUSB.print(midiMsgBuf14[idx].parameter); 
//      SerialUSB.print("\tVAL: "); SerialUSB.println(midiMsgBuf14[idx].value); 
      SearchMsgInConfigAndUpdate( midiMsgBuf14[idx].type,
                                  midiMsgBuf14[idx].message,
                                  midiMsgBuf14[idx].channel,
                                  midiMsgBuf14[idx].parameter,
                                  midiMsgBuf14[idx].value,
                                  midiMsgBuf14[idx].port,
                                  true);
    }
  }
}

void SetBankForAll(uint8_t newBank) {
  encoderHw.SetBankForEncoders(newBank);
  //SetBankForDigitals(newBank);
}

void printPointer(void* pointer) {
  char buffer[30];
  sprintf(buffer, "%p", pointer);
  SerialUSB.println(buffer);
  sprintf(buffer, "");
}
uint16_t checkSum(const uint8_t *data, uint8_t len)
{
  uint16_t sum = 0x00;
  for (uint8_t i = 0; i < len; i++)
    sum ^= data[i];

  //  SerialUSB.print("\n\nTotal checksum: "); SerialUSB.print(2019-sum);
  //  SerialUSB.print("\tMSB: "); SerialUSB.print(((2019-sum)>>7)&0x7F);
  //  SerialUSB.print("\tLSB: "); SerialUSB.println((2019-sum)&0x7F);

  return sum;
}

//CRC-8 - algoritmo basato sulle formule di CRC-8 di Dallas/Maxim
//codice pubblicato sotto licenza GNU GPL 3.0
uint8_t CRC8(const uint8_t *data, uint8_t len)
{
  uint8_t crc = 0x00;
  while (len--) {
    uint8_t extract = *data++;
    for (uint8_t tempI = 8; tempI; tempI--)
    {
      uint8_t sum = (crc ^ extract) & 0x01;
      crc >>= 1;
      if (sum) {
        crc ^= 0x8C;
      }
      extract >>= 1;
    }
  }
  return crc & 0x7F;
}

void ResetFBMicro() {
  digitalWrite(pinResetSAMD11, LOW);
  delay(5);
  digitalWrite(pinResetSAMD11, HIGH);
  delay(5);
}

void SelfReset() {
  SPI.end();
  SerialUSB.end();
  Serial.end();

  NVIC_SystemReset();      // processor software reset
}


//write 0xFF to eeprom, "chunk" bytes at a time
void eeErase(uint8_t chunk, uint32_t startAddr, uint32_t endAddr) {
  chunk &= 0xFC;                //force chunk to be a multiple of 4
  uint8_t data[chunk];
  SerialUSB.println(F("Erasing..."));
  for (int i = 0; i < chunk; i++) data[i] = 0xFF;
  uint32_t msStart = millis();

  for (uint32_t a = startAddr; a <= endAddr; a += chunk) {
    if ( (a & 0xFFF) == 0 ) SerialUSB.println(a);
    eep.write(a, data, chunk);
  }
  uint32_t msLapse = millis() - msStart;
  SerialUSB.print(F("Erase lapse: "));
  SerialUSB.print(msLapse);
  SerialUSB.println(F(" ms"));
}

bool IsPowerConnected(){
  return !digitalRead(externalVoltagePin);
}

void ChangeBrigthnessISR(void) {    // External interrupt on "externalVoltagePin"
  // SerialUSB.print(F("HELP"));
  feedbackHw.SendCommand(CMD_ALL_LEDS_OFF);
  uint8_t powerAdapterConnected = !digitalRead(externalVoltagePin);
  static int sumBright = 0;

  antMillisPowerChange = millis();
  powerChangeFlag = true;

  if(!powerAdapterConnected){
    if(config->inputs.encoderCount >= 28)  currentBrightness = BRIGHTNESS_WOP_32_ENC;
    else                                   currentBrightness = BRIGHTNESS_WOP;
  }else{
    currentBrightness = BRIGHTNESS_WITH_POWER;
  }

  if (powerAdapterConnected) {
    // SerialUSB.println(F("Power connected"));
    feedbackHw.SendCommand(CHANGE_BRIGHTNESS);
    feedbackHw.SendCommand(currentBrightness);
    SetStatusLED(STATUS_BLINK, 3, STATUS_FB_CONFIG);
  } else {
    // SerialUSB.println(F("Power disconnected"));
    feedbackHw.SendCommand(CHANGE_BRIGHTNESS);
    feedbackHw.SendCommand(currentBrightness);
    //    feedbackHw.SendCommand(BRIGHNESS_WO_POWER+sumBright);
    //SerialUSB.println(BRIGHNESS_WO_POWER+sumBright);
    SetStatusLED(STATUS_BLINK, 1, STATUS_FB_CONFIG);
  }
}

long mapl(long x, long in_min, long in_max, long out_min, long out_max)
{
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

/*
   Esta función es llamada por el loop principal, cuando las variables flagBlinkStatusLED y blinkCountStatusLED son distintas de cero.
   blinkCountStatusLED tiene la cantidad de veces que debe titilar el LED.
   El intervalo de parpadeo depende del tipo de status que se quiere representar
*/

void SetStatusLED(uint8_t onOrBlinkOrOff, uint8_t nTimes, uint8_t status_type) {
  if (!flagBlinkStatusLED) {
    flagBlinkStatusLED = onOrBlinkOrOff;
    statusLEDfbType = status_type;
    blinkCountStatusLED = nTimes;

    switch (statusLEDfbType) {
      case STATUS_FB_NONE: {
          blinkInterval = 0;
        } break;
      case STATUS_FB_CONFIG: {
          blinkInterval = STATUS_CONFIG_BLINK_INTERVAL;
        } break;
      case STATUS_FB_MIDI_OUT:
      case STATUS_FB_MIDI_IN: {
          blinkInterval = STATUS_MIDI_BLINK_INTERVAL;
        } break;
      case STATUS_FB_ERROR: {
          blinkInterval = STATUS_ERROR_BLINK_INTERVAL;
        } break;
      default:
        blinkInterval = 0; break;
    }
  }
}

void UpdateStatusLED() {
  uint8_t colorR = 0, colorG = 0, colorB = 0;
  if (flagBlinkStatusLED && blinkCountStatusLED) {

    if (firstTime) {
      firstTime = false;
      millisStatusPrev = millis();
    }

    if (flagBlinkStatusLED == STATUS_BLINK) {
      if (millis() - millisStatusPrev > blinkInterval) {
        millisStatusPrev = millis();
        lastStatusLEDState = !lastStatusLEDState;

        colorR = pgm_read_byte(&gamma8[(statusLEDColor[statusLEDfbType] >> 16) & 0xFF]);
        colorG = pgm_read_byte(&gamma8[(statusLEDColor[statusLEDfbType] >> 8) & 0xFF]);
        colorB = pgm_read_byte(&gamma8[statusLEDColor[statusLEDfbType] & 0xFF]);

        if (lastStatusLEDState) {
          statusLED.setPixelColor(0, colorR, colorG, colorB); // Moderately bright green color.
        } else {
          statusLED.setPixelColor(0, 0, 0, 0); // Moderately bright green color.
          blinkCountStatusLED--;
        }
        statusLED.show(); // This sends the updated pixel color to the hardware.

        if (!blinkCountStatusLED) {
          flagBlinkStatusLED = STATUS_OFF;
          statusLEDfbType = 0;
          firstTime = true;
        }
      }
    } else if (flagBlinkStatusLED == STATUS_ON) {
      statusLED.setPixelColor(0, statusLEDColor[statusLEDfbType]);
    } else if (flagBlinkStatusLED == STATUS_OFF) {
      statusLED.setPixelColor(0, statusLEDColor[statusLEDtypes::STATUS_FB_NONE]);
    }
  }
  return;
}

/* 
 * Functions to sort midi buffers by parameter
 * https://www.geeksforgeeks.org/bubble-sort/
 */
void swap7(midiMsgBuffer7 *xp, midiMsgBuffer7 *yp)  
{  
    midiMsgBuffer7 temp = *xp;  
    *xp = *yp;  
    *yp = temp;  
}  
  
// A function to implement bubble sort  
void bubbleSort7(midiMsgBuffer7 buf[], int n)  
{  
    int i, j;  
    for (i = 0; i < n-1; i++)      
      
    // Last i elements are already in place  
    for (j = 0; j < n-i-1; j++)  
        if (buf[j].parameter > buf[j+1].parameter)  
            swap7(&buf[j], &buf[j+1]);  
}

void swap14(midiMsgBuffer14 *xp, midiMsgBuffer14 *yp)  
{  
    midiMsgBuffer14 temp = *xp;  
    *xp = *yp;  
    *yp = temp;  
}  
  
// A function to implement bubble sort  
void bubbleSort14(midiMsgBuffer14 buf[], int n)  
{  
    int i, j;  
    for (i = 0; i < n-1; i++)      
      
    // Last i elements are already in place  
    for (j = 0; j < n-i-1; j++)  
        if (buf[j].parameter > buf[j+1].parameter)  
            swap14(&buf[j], &buf[j+1]);  
}

/* 
 * Functions to binary search lower and upper limits for a parameter in midi buffers
 * https://stackoverflow.com/questions/12144802/finding-multiple-entries-with-binary-search 
 */
// 7 bit buffer
int lower_bound_search7(midiMsgBuffer7 buf[], int key, int low, int high)
{
    if (low > high)
        //return -1;
        return low;

    int mid = low + ((high - low) >> 1);
    //if (arr[mid] == key) return mid;

    //Attention here, we go left for lower_bound when meeting equal values
    if (buf[mid].parameter >= key) 
        return lower_bound_search7(buf, key, low, mid - 1);
    else
        return lower_bound_search7(buf, key, mid + 1, high);
}

int upper_bound_search7(midiMsgBuffer7 buf[], int key, int low, int high)
{
    if (low > high)
        //return -1;
        return low;

    int mid = low + ((high - low) >> 1);
    //if (arr[mid] == key) return mid;

    //Attention here, we go right for upper_bound when meeting equal values
    if (buf[mid].parameter > key) 
        return upper_bound_search7(buf, key, low, mid - 1);
    else
        return upper_bound_search7(buf, key, mid + 1, high);
}
// 14 bit buffer
int lower_bound_search14(midiMsgBuffer14 buf[], int key, int low, int high)
{
    if (low > high)
        //return -1;
        return low;

    int mid = low + ((high - low) >> 1);
    //if (arr[mid] == key) return mid;

    //Attention here, we go left for lower_bound when meeting equal values
    if (buf[mid].parameter >= key) 
        return lower_bound_search14(buf, key, low, mid - 1);
    else
        return lower_bound_search14(buf, key, mid + 1, high);
}

int upper_bound_search14(midiMsgBuffer14 buf[], int key, int low, int high)
{
    if (low > high)
        //return -1;
        return low;

    int mid = low + ((high - low) >> 1);
    //if (arr[mid] == key) return mid;

    //Attention here, we go right for upper_bound when meeting equal values
    if (buf[mid].parameter > key) 
        return upper_bound_search14(buf, key, low, mid - 1);
    else
        return upper_bound_search14(buf, key, mid + 1, high);
}

bool CheckConfigIfMatch(uint8_t type, uint8_t index, uint8_t src, uint8_t msg, uint8_t channel, uint16_t param){
  for (uint8_t encNo = 0; encNo < config->inputs.encoderCount; encNo++) {
    if((type == FB_ENCODER || type == FB_ENCODER_SWITCH) && index == encNo) continue;
    
    if(((encoder[encNo].rotaryFeedback.parameterMSB<<7) | encoder[encNo].rotaryFeedback.parameterLSB) == param){
      if(encoder[encNo].rotaryFeedback.channel == channel){
        if(encoder[encNo].rotaryFeedback.message == msg){
          if(encoder[encNo].rotaryFeedback.source == src){
            return true; // there's a match
          }
        }
      }
    }
    if(((encoder[encNo].switchFeedback.parameterMSB<<7) | encoder[encNo].switchFeedback.parameterLSB) == param){
      if(encoder[encNo].switchFeedback.channel == channel){
        if(encoder[encNo].switchFeedback.message == msg){
          if(encoder[encNo].switchFeedback.source == src){
            return true; // there's a match
          }
        }
      }
    }
  }
  for (uint8_t digNo = 0; digNo < config->inputs.digitalCount; digNo++) {
    if(type == FB_DIGITAL && index == digNo) continue;
    
    if(((digital[digNo].feedback.parameterMSB<<7) | digital[digNo].feedback.parameterLSB) == param){
      if(digital[digNo].feedback.channel == channel){
        if(digital[digNo].feedback.message == msg){
          if(digital[digNo].feedback.source == src){  
            return true; // there's a match
          }
        }
      }
    }
  }
  for (uint8_t analogNo = 0; analogNo < config->inputs.analogCount; analogNo++) {
    if(type == FB_ANALOG && index == analogNo) continue;
    
    if(((analog[analogNo].feedback.parameterMSB<<7) | analog[analogNo].feedback.parameterLSB) == param){
      if(analog[analogNo].feedback.channel == channel){
        if(analog[analogNo].feedback.message == msg){
          if(analog[analogNo].feedback.source == src){   
            return true; // there's a match
          }
        }
      }
    }
  }
  return false; // if arrived here, there's no match
}

//void MidiSettingsInit() {
//
//  // If it is a regular message, check if it matches the feedback configuration for all the inputs (only the current bank)
//  // SWEEP ALL ENCODERS
//
//  for (uint8_t encNo = 0; encNo < config->inputs.encoderCount; encNo++) {
//    // SWEEP ALL ENCODERS
//    if(encoder[encNo].rotaryFeedback.source != feedbackSource::fb_src_local){
//      uint8_t srcToCompare = encoder[encNo].rotaryFeedback.source;
//      uint8_t messageToCompare = encoder[encNo].rotaryFeedback.message;
//      uint8_t channelToCompare = encoder[encNo].rotaryFeedback.channel;
//      uint16_t paramToCompare = (encoder[encNo].rotaryFeedback.parameterMSB<<7 | encoder[encNo].rotaryFeedback.parameterLSB);
//  
//      if(!CheckConfigIfMatch(FB_ENCODER, encNo, srcToCompare, messageToCompare, channelToCompare, paramToCompare)){
//        SerialUSB.println("-----------------------------------------------");
//        SerialUSB.print(encNo); SerialUSB.println(": Encoder switch has no match in config");
//        if      ( IS_ENCODER_ROT_FB_14_BIT(encNo) ) { midiRxSettings.midiBufferSize14++; SerialUSB.print("MIDI BUFFER 14 bit: ");SerialUSB.println(midiRxSettings.midiBufferSize14); }   // If 
//        else if ( IS_ENCODER_ROT_FB_7_BIT(encNo)  ) { midiRxSettings.midiBufferSize7++; SerialUSB.print("MIDI BUFFER 7 bit: ");SerialUSB.println(midiRxSettings.midiBufferSize7); }
//        SerialUSB.println("-----------------------------------------------");
//      }else{
//        SerialUSB.println("/////////////////////////////////////////////////////////////////////");
//        SerialUSB.print(encNo); SerialUSB.println(": Encoder has a match in config");
//        SerialUSB.println("/////////////////////////////////////////////////////////////////////");
//      }
//      for (uint8_t channel = 0; channel <= 15; channel++) {
//        if (encoder[encNo].rotaryFeedback.channel == channel) { midiRxSettings.listenToChannel |= (1 << channel); } // If there's a match, set channel flag
//      }
//    }
//
//    if(encoder[encNo].switchFeedback.source != feedbackSource::fb_src_local){
//      uint8_t srcToCompare = encoder[encNo].switchFeedback.source;
//      uint8_t messageToCompare = encoder[encNo].switchFeedback.message;
//      uint8_t channelToCompare = encoder[encNo].switchFeedback.channel;
//      uint16_t paramToCompare = (encoder[encNo].switchFeedback.parameterMSB<<7 | encoder[encNo].switchFeedback.parameterLSB);
//  
//      if(!CheckConfigIfMatch(FB_ENCODER_SWITCH, encNo, srcToCompare, messageToCompare, channelToCompare, paramToCompare)){
//        SerialUSB.println("-----------------------------------------------");
//        SerialUSB.print(encNo); SerialUSB.println(": Encoder switch has no match in config");
//        if      ( IS_ENCODER_SW_FB_14_BIT(encNo) ) { midiRxSettings.midiBufferSize14++; SerialUSB.print("MIDI BUFFER 14 bit: ");SerialUSB.println(midiRxSettings.midiBufferSize14);  }   // If 
//        else if ( IS_ENCODER_SW_FB_7_BIT(encNo)  ) { midiRxSettings.midiBufferSize7++; SerialUSB.print("MIDI BUFFER 7 bit: ");SerialUSB.println(midiRxSettings.midiBufferSize7); }
//        SerialUSB.println("-----------------------------------------------");
//      }else{
//        SerialUSB.println("/////////////////////////////////////////////////////////////////////");
//        SerialUSB.print(encNo); SerialUSB.print(": Encoder switch has a match in config");
//        SerialUSB.println("/////////////////////////////////////////////////////////////////////");
//      }
//      for (uint8_t channel = 0; channel <= 15; channel++) {
//        // SWEEP ALL ENCODER SWITCHES
//        if (encoder[encNo].switchFeedback.channel == channel) { midiRxSettings.listenToChannel |= (1 << channel); }
//      }
//    }
//  }
//  
//  // SWEEP ALL DIGITAL
//  for (uint16_t digNo = 0; digNo < config->inputs.digitalCount; digNo++) {
//    if(digital[digNo].feedback.source == feedbackSource::fb_src_local) continue; // If feedback source is local, don't count
//
//    uint8_t srcToCompare = digital[digNo].feedback.source;
//    uint8_t messageToCompare = digital[digNo].feedback.message;
//    uint8_t channelToCompare = digital[digNo].feedback.channel;
//    uint16_t paramToCompare = (digital[digNo].feedback.parameterMSB<<7 | digital[digNo].feedback.parameterLSB);
//
//    if(!CheckConfigIfMatch(FB_DIGITAL, digNo, srcToCompare, messageToCompare, channelToCompare, paramToCompare)){
//      SerialUSB.println("-----------------------------------------------");
//      SerialUSB.print(digNo); SerialUSB.println(": digital has no match in config");        
//      if      ( IS_DIGITAL_FB_14_BIT(digNo) ) { midiRxSettings.midiBufferSize14++;  SerialUSB.print("MIDI BUFFER 14 bit: ");SerialUSB.println(midiRxSettings.midiBufferSize14); }   // If 
//      else if ( IS_DIGITAL_FB_7_BIT(digNo)  ) { midiRxSettings.midiBufferSize7++;   SerialUSB.print("MIDI BUFFER 7 bit: ");SerialUSB.println(midiRxSettings.midiBufferSize7); }
//      SerialUSB.println("-----------------------------------------------");
//    }else{
//      SerialUSB.println("/////////////////////////////////////////////////////////////////////");
//      SerialUSB.print(digNo); SerialUSB.println(": Digital has a match in config");
//      SerialUSB.println("/////////////////////////////////////////////////////////////////////");
//    }
//
//    // SWEEP ALL CHANNELS
//    for (uint8_t channel = 0; channel <= 15; channel++) {   
//      if (digital[digNo].feedback.channel == channel) { midiRxSettings.listenToChannel |= (1 << channel); }
//    }
//  }
//  // SWEEP ALL ANALOG
//  for (uint8_t analogNo = 0; analogNo < config->inputs.analogCount; analogNo++) {
//    if(analog[analogNo].feedback.source == feedbackSource::fb_src_local) continue; // If feedback source is local, don't count
//
//    uint8_t srcToCompare = analog[analogNo].feedback.source;
//    uint8_t messageToCompare = analog[analogNo].feedback.message;
//    uint8_t channelToCompare = analog[analogNo].feedback.channel;
//    uint16_t paramToCompare = (analog[analogNo].feedback.parameterMSB<<7 | analog[analogNo].feedback.parameterLSB);
//
//    if(!CheckConfigIfMatch(FB_ANALOG, analogNo, srcToCompare, messageToCompare, channelToCompare, paramToCompare)){
//      SerialUSB.println("-----------------------------------------------");
//      SerialUSB.print(analogNo); SerialUSB.println(": Analog has no match in config");
//      if      ( IS_ANALOG_FB_14_BIT(analogNo) ) { midiRxSettings.midiBufferSize14++; SerialUSB.print("MIDI BUFFER 14 bit: ");SerialUSB.println(midiRxSettings.midiBufferSize14); }   // If 
//      else if ( IS_ANALOG_FB_7_BIT(analogNo)  ) { midiRxSettings.midiBufferSize7++; SerialUSB.print("MIDI BUFFER 7 bit: ");SerialUSB.println(midiRxSettings.midiBufferSize7);  }
//      SerialUSB.println("-----------------------------------------------");
//    }else{
//      SerialUSB.println("/////////////////////////////////////////////////////////////////////");
//      SerialUSB.print(analogNo); SerialUSB.print(": Analog has a match in config");
//      SerialUSB.println("/////////////////////////////////////////////////////////////////////");
//    }
//    
//    for (uint8_t channel = 0; channel <= 15; channel++) {
//      if (analog[analogNo].feedback.channel == channel) { midiRxSettings.listenToChannel |= (1 << channel); }
//    }
//  }
//}

void MidiSettingsInit() {
  // If it is a regular message, check if it matches the feedback configuration for all the inputs (only the current bank)
  // SWEEP ALL ENCODERS

  for (uint8_t encNo = 0; encNo < config->inputs.encoderCount; encNo++) {
    // SWEEP ALL ENCODERS
    if(encoder[encNo].rotaryFeedback.source != feedbackSource::fb_src_local){
      for (uint8_t channel = 0; channel <= 15; channel++) {
        if (encoder[encNo].rotaryFeedback.channel == channel) { midiRxSettings.listenToChannel |= (1 << channel); } // If there's a match, set channel flag
  
        // SWEEP ALL ENCODER SWITCHES
        if (encoder[encNo].switchFeedback.channel == channel) { midiRxSettings.listenToChannel |= (1 << channel); } 
      }
      if      ( IS_ENCODER_ROT_FB_14_BIT(encNo) ) { midiRxSettings.midiBufferSize14++;  } 
      else if ( IS_ENCODER_ROT_FB_7_BIT(encNo)  ) { midiRxSettings.midiBufferSize7++;   }
    }
    
    if(encoder[encNo].switchFeedback.source != feedbackSource::fb_src_local){
      if      ( IS_ENCODER_SW_FB_14_BIT(encNo)) { midiRxSettings.midiBufferSize14++;  } 
      else if ( IS_ENCODER_SW_FB_7_BIT(encNo) ) { midiRxSettings.midiBufferSize7++;   } 
    }
  }
  // SWEEP ALL DIGITAL
  for (uint16_t digNo = 0; digNo < config->inputs.digitalCount; digNo++) {
    if( digital[digNo].feedback.source == feedbackSource::fb_src_local ) continue; // If feedback source is local, don't count
    
    for (uint8_t channel = 0; channel <= 15; channel++) {
      // SWEEP ALL DIGITAL
      if (digital[digNo].feedback.channel == channel) { midiRxSettings.listenToChannel |= (1 << channel); }
    }
    // Add 14 bit messages
    if      ( IS_DIGITAL_FB_14_BIT(digNo) ) { midiRxSettings.midiBufferSize14++; } 
    else if ( IS_DIGITAL_FB_7_BIT(digNo)  ) { midiRxSettings.midiBufferSize7++;  }
  }
  // SWEEP ALL ANALOG
  for (uint8_t analogNo = 0; analogNo < config->inputs.analogCount; analogNo++) {
    if(analog[analogNo].feedback.source == feedbackSource::fb_src_local) continue; // If feedback source is local, don't count
    
    for (uint8_t channel = 0; channel <= 15; channel++) {
      if (analog[analogNo].feedback.channel == channel) { midiRxSettings.listenToChannel |= (1 << channel); }
    }
    if      ( IS_ANALOG_FB_14_BIT(analogNo) ) { midiRxSettings.midiBufferSize14++;  } 
    else if ( IS_ANALOG_FB_7_BIT(analogNo)  ) { midiRxSettings.midiBufferSize7++;   }
  }

  // SWEEP ALL FEEDBACK _ TO IMPLEMENT
}

void MidiBufferInitClear(){
  for (uint32_t idx = 0; idx < midiRxSettings.midiBufferSize14; idx++) {
    midiMsgBuf14[idx].banksPresent = 0;
    midiMsgBuf14[idx].banksToUpdate = 0;
  }
  for (uint32_t idx = 0; idx < midiRxSettings.midiBufferSize7; idx++) {
    midiMsgBuf7[idx].banksPresent = 0;
    midiMsgBuf7[idx].banksToUpdate = 0;
  }
}
/*
 * MidiBufferFill()
 * 
 * Description: This function fills the midi rx watch buffer with all the different midi messages in configuration 
 */
void MidiBufferFill() {
  EncoderScanAndFill();  
  DigitalScanAndFill();
  AnalogScanAndFill();
//  FeedbackScanAndFill();  
}

void EncoderScanAndFill(){
  bool thereIsAMatch = false;
  byte messageConfigType = 0;
  
  for (uint8_t encNo = 0; encNo < config->inputs.encoderCount; encNo++) {     // SWEEP ALL ENCODERS
    thereIsAMatch = false;                                                    // Set flag to signal msg match false for new check
    
    if(encoder[encNo].rotaryFeedback.source != feedbackSource::fb_src_local){ // Don't save in buffer if feedback source is local
      // Get MIDI type from config type
      switch(encoder[encNo].rotaryFeedback.message){
        case rotaryMessageTypes::rotary_msg_note:   { messageConfigType  =   MidiTypeYTX  ::  NoteOn;         } break;
        case rotaryMessageTypes::rotary_msg_cc:     { messageConfigType  =   MidiTypeYTX  ::  ControlChange;  } break;
        case rotaryMessageTypes::rotary_msg_pc_rel: { messageConfigType  =   MidiTypeYTX  ::  ProgramChange;  } break;
        case rotaryMessageTypes::rotary_msg_nrpn:   { messageConfigType  =   MidiTypeYTX  ::  NRPN;           } break;
        case rotaryMessageTypes::rotary_msg_rpn:    { messageConfigType  =   MidiTypeYTX  ::  RPN;            } break;
        case rotaryMessageTypes::rotary_msg_pb:     { messageConfigType  =   MidiTypeYTX  ::  PitchBend;      } break;
        default:                                    { messageConfigType  =   MidiTypeYTX  ::  InvalidType;    } break;
      }

      // If current encoder rotary config message is 7 bit
      if ( IS_ENCODER_ROT_FB_14_BIT(encNo) ) {
        for (uint32_t idx = 0; idx < midiRxSettings.lastMidiBufferIndex14; idx++) {               // Search every message already saved in 14 bit buffer
          if (midiMsgBuf14[idx].parameter == ((encoder[encNo].rotaryFeedback.parameterMSB << 7) | 
                                              (encoder[encNo].rotaryFeedback.parameterLSB)) 
             || messageConfigType == MidiTypeYTX::PitchBend) {                                // Check full 14 bit parameter. If pitch bend, don't check parameter
            if (midiMsgBuf14[idx].port == encoder[encNo].rotaryFeedback.source) {                 // Check message source
              if (midiMsgBuf14[idx].channel == encoder[encNo].rotaryFeedback.channel) {           // Check channel
                if (midiMsgBuf14[idx].message == messageConfigType) {                             // Check message type
                  if (midiMsgBuf14[idx].type == FB_ENCODER) {                                     // Check fb type
                
                    thereIsAMatch                   = true;                                       // If there's a match, signal it,
                    midiMsgBuf14[idx].banksPresent |= (1<<currentBank);                           // flag that this message is present in current bank,
                    continue;                                                                     // and check next message
                  }
                }
              }
            }
          }
        }

        // If buffer hasn't been filled, and there wasn't a match
        if (midiRxSettings.lastMidiBufferIndex14 < midiRxSettings.midiBufferSize14 && !thereIsAMatch) {
  //        SerialUSB.print(midiRxSettings.lastMidiBufferIndex14); SerialUSB.println(F(": NEW MIDI MESSAGE ADDED TO 14 BIT BUFFER"));
          midiMsgBuf14[midiRxSettings.lastMidiBufferIndex14].message      =   messageConfigType;                                  // Save message type in buffer
          midiMsgBuf14[midiRxSettings.lastMidiBufferIndex14].type         =   FeebackTypes::FB_ENCODER;                           // Save component type in buffer
          midiMsgBuf14[midiRxSettings.lastMidiBufferIndex14].port         =   encoder[encNo].rotaryFeedback.source;               // Save feedback source in buffer
          midiMsgBuf14[midiRxSettings.lastMidiBufferIndex14].channel      =   encoder[encNo].rotaryFeedback.channel;              // Save feedback channel in buffer
          if(messageConfigType == MidiTypeYTX::PitchBend){
            midiMsgBuf14[midiRxSettings.lastMidiBufferIndex14].parameter    =   0;                                          // Save feedback param in buffer
          }else{
            midiMsgBuf14[midiRxSettings.lastMidiBufferIndex14].parameter  =   (encoder[encNo].rotaryFeedback.parameterMSB << 7) | 
                                                                              (encoder[encNo].rotaryFeedback.parameterLSB);       // Save feedback full 14 bit param in buffer
          }
          midiMsgBuf14[midiRxSettings.lastMidiBufferIndex14].banksPresent |=  (1<<currentBank);                                   // Flag that this message is present in current bank                                            
          midiMsgBuf14[midiRxSettings.lastMidiBufferIndex14++].value      =   0;                                                  // Initialize value to 0
        }
      // If current encoder rotary config message is 7 bit
      } else if ( IS_ENCODER_ROT_FB_7_BIT(encNo) ){ 
        for (uint32_t idx = 0; idx < midiRxSettings.lastMidiBufferIndex7; idx++) {              // Search every message already saved in 7 bit buffer
          if (midiMsgBuf7[idx].parameter == encoder[encNo].rotaryFeedback.parameterLSB
              || messageConfigType == MidiTypeYTX::ProgramChange) {                         // check parameter
            if (midiMsgBuf7[idx].port == encoder[encNo].rotaryFeedback.source) {                // Check source
              if (midiMsgBuf7[idx].channel == encoder[encNo].rotaryFeedback.channel) {          // Check channel
                if (midiMsgBuf7[idx].message == messageConfigType) {                            // Check message
                  if (midiMsgBuf7[idx].type == FB_ENCODER) {                                    // Check fb type
                    thereIsAMatch                 = true;                                       // If there's a match, signal it,
                    midiMsgBuf7[idx].banksPresent |= (1<<currentBank);                          // flag that this message is present in current bank,
                    continue;                                                                   // and check next message
                  }
                }
              }
            }
          }
        }
        // If 7 bit buffer isn't full and and there wasn't a match already saved, save new message
        if (midiRxSettings.lastMidiBufferIndex7 < midiRxSettings.midiBufferSize7 && !thereIsAMatch) {
          midiMsgBuf7[midiRxSettings.lastMidiBufferIndex7].message      =   messageConfigType;                            // Save message type in buffer
          midiMsgBuf7[midiRxSettings.lastMidiBufferIndex7].type         =   FeebackTypes::FB_ENCODER;                     // Save component type in buffer
          midiMsgBuf7[midiRxSettings.lastMidiBufferIndex7].port         =   encoder[encNo].rotaryFeedback.source;         // Save feedback source in buffer
          midiMsgBuf7[midiRxSettings.lastMidiBufferIndex7].channel      =   encoder[encNo].rotaryFeedback.channel;        // Save feedback channel in buffer
          if(messageConfigType == MidiTypeYTX::ProgramChange){
            midiMsgBuf7[midiRxSettings.lastMidiBufferIndex7].parameter    =   0;                                          // Save feedback param in buffer
          }else{
            midiMsgBuf7[midiRxSettings.lastMidiBufferIndex7].parameter    =   encoder[encNo].rotaryFeedback.parameterLSB;   // Save feedback param in buffer
          }
          midiMsgBuf7[midiRxSettings.lastMidiBufferIndex7].banksPresent |=  (1<<currentBank);                             // Flag that this message is present in current bank
          midiMsgBuf7[midiRxSettings.lastMidiBufferIndex7++].value      =   0;                                            // Initialize value to 0
        }
      }
    }
    
    thereIsAMatch = false;                                                    // Set flag to signal msg match false for new check
    if(encoder[encNo].switchFeedback.source != feedbackSource::fb_src_local){ // Don't save in buffer if feedback source is local
      // Get MIDI type from config type
      if(encoder[encNo].switchConfig.mode != switchModes::switch_mode_shift_rot){
        switch(encoder[encNo].switchFeedback.message){
          case switchMessageTypes::switch_msg_note: { messageConfigType = MidiTypeYTX::NoteOn;         } break;
          case switchMessageTypes::switch_msg_cc:   { messageConfigType = MidiTypeYTX::ControlChange;  } break;
          case switchMessageTypes::switch_msg_pc:   
          case switchMessageTypes::switch_msg_pc_m:   
          case switchMessageTypes::switch_msg_pc_p: { messageConfigType = MidiTypeYTX::ProgramChange;  } break;
          case switchMessageTypes::switch_msg_nrpn: { messageConfigType = MidiTypeYTX::NRPN;           } break;
          case switchMessageTypes::switch_msg_rpn:  { messageConfigType = MidiTypeYTX::RPN;            } break;
          case switchMessageTypes::switch_msg_pb:   { messageConfigType = MidiTypeYTX::PitchBend;      } break;
          default:                                  { messageConfigType = MidiTypeYTX::InvalidType;    } break;
        }
      }else{
        switch(encoder[encNo].switchFeedback.message){
          case rotaryMessageTypes::rotary_msg_note:   { messageConfigType = MidiTypeYTX ::  NoteOn;         } break;
          case rotaryMessageTypes::rotary_msg_cc:     { messageConfigType = MidiTypeYTX ::  ControlChange;  } break;
          case rotaryMessageTypes::rotary_msg_pc_rel: { messageConfigType = MidiTypeYTX ::  ProgramChange;  } break;
          case rotaryMessageTypes::rotary_msg_nrpn:   { messageConfigType = MidiTypeYTX ::  NRPN;           } break;
          case rotaryMessageTypes::rotary_msg_rpn:    { messageConfigType = MidiTypeYTX ::  RPN;            } break;
          case rotaryMessageTypes::rotary_msg_pb:     { messageConfigType = MidiTypeYTX ::  PitchBend;      } break;
          default:                                    { messageConfigType = MidiTypeYTX ::  InvalidType;    } break;
        }
      }

      
      if ( IS_ENCODER_SW_FB_14_BIT(encNo) ) {
        for (uint32_t idx = 0; idx < midiRxSettings.lastMidiBufferIndex14; idx++) {                     // Search every message already saved in 14 bit buffer
          if (midiMsgBuf14[idx].parameter == ((encoder[encNo].switchFeedback.parameterMSB << 7) | 
                                              (encoder[encNo].switchFeedback.parameterLSB))
             || messageConfigType == MidiTypeYTX::PitchBend) {                                      // Check full 14 bit parameter. If pitch bend, don't check parameter
            if (midiMsgBuf14[idx].port == encoder[encNo].switchFeedback.source) {                       // Check message source
              if (midiMsgBuf14[idx].channel == encoder[encNo].switchFeedback.channel) {                 // Check channel
                if (midiMsgBuf14[idx].message == messageConfigType) {                                   // Check message type
                  if (midiMsgBuf14[idx].type == FB_ENCODER_SWITCH
                      || midiMsgBuf14[idx].type == FB_SHIFT) {                                    // Check fb type                                                      
                    thereIsAMatch                   = true;             // If there's a match, signal it,
                    midiMsgBuf14[idx].banksPresent |= (1<<currentBank); // flag that this message is present in current bank,
                    continue;                                           // and check next message
                  }
                }
              }
            }
          }
        }
        // If buffer isn't full and there wasn't a match in it
        if (midiRxSettings.lastMidiBufferIndex14 < midiRxSettings.midiBufferSize14 && !thereIsAMatch) {
  //        SerialUSB.print(midiRxSettings.lastMidiBufferIndex14); SerialUSB.println(F(": NEW MIDI MESSAGE ADDED TO 14 BIT BUFFER"));
          midiMsgBuf14[midiRxSettings.lastMidiBufferIndex14].message    = messageConfigType;                                  // Save message type in buffer
          if(encoder[encNo].switchConfig.mode != switchModes::switch_mode_shift_rot){
            midiMsgBuf14[midiRxSettings.lastMidiBufferIndex14].type       = FeebackTypes::FB_ENCODER_SWITCH;                  // Save component type in buffer   
          }else{
            midiMsgBuf14[midiRxSettings.lastMidiBufferIndex14].type       = FeebackTypes::FB_SHIFT;                           // Save component type in buffer 
          }
          midiMsgBuf14[midiRxSettings.lastMidiBufferIndex14].port         = encoder[encNo].switchFeedback.source;               // Save feedback source in buffer
          midiMsgBuf14[midiRxSettings.lastMidiBufferIndex14].channel      = encoder[encNo].switchFeedback.channel;              // Save feedback channel in buffer
          if(messageConfigType == MidiTypeYTX::PitchBend){
            midiMsgBuf14[midiRxSettings.lastMidiBufferIndex14].parameter    =   0;                                                // Save feedback param in buffer
          }else{
            midiMsgBuf14[midiRxSettings.lastMidiBufferIndex14].parameter  = (encoder[encNo].switchFeedback.parameterMSB << 7) | 
                                                                            (encoder[encNo].switchFeedback.parameterLSB);       // Save feedback full 14 bit param in buffer
          }        
          midiMsgBuf14[midiRxSettings.lastMidiBufferIndex14].banksPresent |=  (1<<currentBank);                               // Flag that this message is present in current bank                                           
          midiMsgBuf14[midiRxSettings.lastMidiBufferIndex14++].value    = 0;                                                  // Initialize value to 0
        }
      // If current encoder switch config message is 7 bit
      } else if( IS_ENCODER_SW_FB_7_BIT(encNo) ){
        for (uint32_t idx = 0; idx < midiRxSettings.lastMidiBufferIndex7; idx++) {              // Search every message already saved in 7 bit buffer
          if (midiMsgBuf7[idx].parameter == encoder[encNo].switchFeedback.parameterLSB
              || messageConfigType == MidiTypeYTX::ProgramChange) {                           // check parameter
            if (midiMsgBuf7[idx].port == encoder[encNo].switchFeedback.source) {                  // Check source
              if (midiMsgBuf7[idx].channel == encoder[encNo].switchFeedback.channel) {            // Check channel
                if (midiMsgBuf7[idx].message == messageConfigType) {                              // Check message
                  if (midiMsgBuf7[idx].type == FB_ENCODER_SWITCH 
                      || midiMsgBuf7[idx].type == FB_2CC
                      || midiMsgBuf7[idx].type == FB_SHIFT) {                                       // Check fb type
  //                  SerialUSB.println(F("MIDI MESSAGE ALREADY IN 7 BIT BUFFER"));
                    thereIsAMatch                 = true;                                         // If there's a match, signal it,
                    midiMsgBuf7[idx].banksPresent |= (1<<currentBank);                            // flag that this message is present in current bank,
                    continue;                                                                     // and check next message
                  }
                }
              }
            }
          }
        }
        // If 7 bit buffer isn't full and and there wasn't a match already saved, save new message
        if (midiRxSettings.lastMidiBufferIndex7 < midiRxSettings.midiBufferSize7 && !thereIsAMatch) {
  //        SerialUSB.print(midiRxSettings.lastMidiBufferIndex7); SerialUSB.println(F(": NEW MIDI MESSAGE ADDED TO 7 BIT BUFFER"));
          midiMsgBuf7[midiRxSettings.lastMidiBufferIndex7].message      =   messageConfigType;                            // Save message type in buffer
          
          if(encoder[encNo].switchConfig.mode == switchModes::switch_mode_2cc){
            midiMsgBuf7[midiRxSettings.lastMidiBufferIndex7].type       =   FeebackTypes::FB_2CC;                         // Save component type in buffer
          }else if(encoder[encNo].switchConfig.mode == switchModes::switch_mode_shift_rot){
            midiMsgBuf7[midiRxSettings.lastMidiBufferIndex7].type       =   FeebackTypes::FB_SHIFT;                       // Save component type in buffer 
          }else{
            midiMsgBuf7[midiRxSettings.lastMidiBufferIndex7].type       =   FeebackTypes::FB_ENCODER_SWITCH;              // Save component type in buffer 
          }
          
          midiMsgBuf7[midiRxSettings.lastMidiBufferIndex7].port         =   encoder[encNo].switchFeedback.source;         // Save feedback source in buffer
          midiMsgBuf7[midiRxSettings.lastMidiBufferIndex7].channel      =   encoder[encNo].switchFeedback.channel;        // Save feedback channel in buffer
          if(messageConfigType == MidiTypeYTX::ProgramChange){
            midiMsgBuf7[midiRxSettings.lastMidiBufferIndex7].parameter  =   0;                                          // Save feedback param in buffer
          }else{
            midiMsgBuf7[midiRxSettings.lastMidiBufferIndex7].parameter  =   encoder[encNo].switchFeedback.parameterLSB;   // Save feedback param in buffer
          }
          midiMsgBuf7[midiRxSettings.lastMidiBufferIndex7].banksPresent |=  (1<<currentBank);                             // Flag that this message is present in current bank
          midiMsgBuf7[midiRxSettings.lastMidiBufferIndex7++].value      =   0;                                            // Initialize value to 0
        }
      }
    }
  }
}

void DigitalScanAndFill(){
  bool thereIsAMatch = false;
  byte messageConfigType = 0;
  
  // SWEEP ALL DIGITAL
  for (uint16_t digNo = 0; digNo < config->inputs.digitalCount; digNo++) {
    thereIsAMatch = false;                                                    // Set flag to signal msg match false for new check
    
    if(digital[digNo].feedback.source == feedbackSource::fb_src_local) continue; // If feedback source is local, don't save in buffer
    
    // Get MIDI type from config type
    switch(digital[digNo].feedback.message){
      case digitalMessageTypes::digital_msg_note: { messageConfigType   =   MidiTypeYTX ::  NoteOn;         } break;
      case digitalMessageTypes::digital_msg_cc:   { messageConfigType   =   MidiTypeYTX ::  ControlChange;  } break;
      case digitalMessageTypes::digital_msg_pc:   
      case digitalMessageTypes::digital_msg_pc_m:   
      case digitalMessageTypes::digital_msg_pc_p: { messageConfigType   =   MidiTypeYTX ::  ProgramChange;  } break;
      case digitalMessageTypes::digital_msg_nrpn: { messageConfigType   =   MidiTypeYTX ::  NRPN;           } break;
      case digitalMessageTypes::digital_msg_rpn:  { messageConfigType   =   MidiTypeYTX ::  RPN;            } break;
      case digitalMessageTypes::digital_msg_pb:   { messageConfigType   =   MidiTypeYTX ::  PitchBend;      } break;
      default:                                    { messageConfigType   =   0;                              } break;
    }

    // If current digital config message is 14 bit
    if ( IS_DIGITAL_FB_14_BIT(digNo) ) {
      for (uint32_t idx = 0; idx < midiRxSettings.lastMidiBufferIndex14; idx++) {                 // Search every message already saved in 14 bit buffer
        if (midiMsgBuf14[idx].parameter == ((digital[digNo].feedback.parameterMSB << 7) | 
                                                  (digital[digNo].feedback.parameterLSB)) 
             || messageConfigType == MidiTypeYTX::PitchBend) {                                // Check full 14 bit parameter. If pitch bend, don't check parameter
          if (midiMsgBuf14[idx].port == digital[digNo].feedback.source) {                         // Check message source
            if (midiMsgBuf14[idx].channel == digital[digNo].feedback.channel) {                   // Check channel
              if (midiMsgBuf14[idx].message == messageConfigType) {                               // Check message type
                if (midiMsgBuf14[idx].type == FB_DIGITAL) {                                       // Check fb type
                  thereIsAMatch                   = true;                                         // If there's a match, signal it,
                  midiMsgBuf14[idx].banksPresent |= (1<<currentBank);                             // flag that this message is present in current bank,
                  continue;                                                                       // and check next message                                                                     
                }
              }
            }
          }
        }
      }
      // If 14 bit buffer isn't full and and there wasn't a match already saved, save new message
      if (midiRxSettings.lastMidiBufferIndex14 < midiRxSettings.midiBufferSize14 && !thereIsAMatch) {
//        SerialUSB.print(midiRxSettings.lastMidiBufferIndex14); SerialUSB.println(F(": NEW MIDI MESSAGE ADDED TO 14 BIT BUFFER"));
        midiMsgBuf14[midiRxSettings.lastMidiBufferIndex14].message      = messageConfigType;                            // Save message type in buffer
        midiMsgBuf14[midiRxSettings.lastMidiBufferIndex14].type         = FeebackTypes::FB_DIGITAL;              // Save component type in buffer
        midiMsgBuf14[midiRxSettings.lastMidiBufferIndex14].port         = digital[digNo].feedback.source;               // Save feedback source in buffer
        midiMsgBuf14[midiRxSettings.lastMidiBufferIndex14].channel      = digital[digNo].feedback.channel;              // Save feedback channel in buffer
        if(messageConfigType == MidiTypeYTX::PitchBend){
          midiMsgBuf14[midiRxSettings.lastMidiBufferIndex14].parameter    =   0;                                          // Save feedback param in buffer
        }else{
          midiMsgBuf14[midiRxSettings.lastMidiBufferIndex14].parameter  = (digital[digNo].feedback.parameterMSB << 7) |
                                                                          (digital[digNo].feedback.parameterLSB);       // Save feedback full 14 bit param in buffer
        }        
        midiMsgBuf14[midiRxSettings.lastMidiBufferIndex14].banksPresent |=  (1<<currentBank);                         // Flag that this message is present in current bank                                           
        midiMsgBuf14[midiRxSettings.lastMidiBufferIndex14++].value      = 0;                                            // Initialize value to 0
      }
    // If current encoder switch config message is 7 bit
    } else if( IS_DIGITAL_FB_7_BIT(digNo) ){
      for (uint32_t idx = 0; idx < midiRxSettings.lastMidiBufferIndex7; idx++) {        // Search every message already saved in 7 bit buffer
        if (midiMsgBuf7[idx].parameter == digital[digNo].feedback.parameterLSB
            || messageConfigType == MidiTypeYTX::ProgramChange) {                     // check parameter
          if (midiMsgBuf7[idx].port == digital[digNo].feedback.source) {                  // Check source
            if (midiMsgBuf7[idx].channel == digital[digNo].feedback.channel) {            // Check channel
              if (midiMsgBuf7[idx].message == messageConfigType) {                        // Check message
                if (midiMsgBuf7[idx].type == FB_DIGITAL) {                                       // Check fb type
                  //                  SerialUSB.println(F("MIDI MESSAGE ALREADY IN 7 BIT BUFFER"));
                  thereIsAMatch                 = true;                                   // If there's a match, signal it,
                  midiMsgBuf7[idx].banksPresent |= (1<<currentBank);                      // flag that this message is present in current bank,
                  continue;                                                               // and check next message 
                }
              }
            }
          }
        }
      }
      // If 7 bit buffer isn't full and and there wasn't a match already saved, save new message
      if (midiRxSettings.lastMidiBufferIndex7 < midiRxSettings.midiBufferSize7 && !thereIsAMatch) {
//        SerialUSB.print(midiRxSettings.lastMidiBufferIndex7); SerialUSB.println(F(": NEW MIDI MESSAGE ADDED TO 7 BIT BUFFER"));
        midiMsgBuf7[midiRxSettings.lastMidiBufferIndex7].message      =   messageConfigType;                      // Save message type in buffer
        midiMsgBuf7[midiRxSettings.lastMidiBufferIndex7].type         =   FeebackTypes::FB_DIGITAL;        // Save component type in buffer
        midiMsgBuf7[midiRxSettings.lastMidiBufferIndex7].port         =   digital[digNo].feedback.source;         // Save feedback source in buffer
        midiMsgBuf7[midiRxSettings.lastMidiBufferIndex7].channel      =   digital[digNo].feedback.channel;        // Save feedback channel in buffer
        if(messageConfigType == MidiTypeYTX::ProgramChange){
          midiMsgBuf7[midiRxSettings.lastMidiBufferIndex7].parameter  =   0;                                          // Save feedback param in buffer
        }else{
          midiMsgBuf7[midiRxSettings.lastMidiBufferIndex7].parameter  =   digital[digNo].feedback.parameterLSB;   // Save feedback param in buffer
        }
        midiMsgBuf7[midiRxSettings.lastMidiBufferIndex7].banksPresent |=  (1<<currentBank);                       // Flag that this message is present in current bank
        midiMsgBuf7[midiRxSettings.lastMidiBufferIndex7++].value      =   0;                                      // Initialize value to 0
      }
    }
  }
  
}

void AnalogScanAndFill(){
  bool thereIsAMatch = false;
  byte messageConfigType = 0;
  
  // SWEEP ALL FEEDBACK
  for (uint8_t analogNo = 0; analogNo < config->inputs.analogCount; analogNo++) {
    thereIsAMatch = false;                                                    // Set flag to signal msg match false for new check
    
    if(analog[analogNo].feedback.source == feedbackSource::fb_src_local) continue; // If feedback source is local, don't count

    // Get MIDI Type from config type
    switch(analog[analogNo].feedback.message){
      case analogMessageTypes::analog_msg_note: { messageConfigType  =  MidiTypeYTX  ::  NoteOn;         } break;
      case analogMessageTypes::analog_msg_cc:   { messageConfigType  =  MidiTypeYTX  ::  ControlChange;  } break;
      case analogMessageTypes::analog_msg_pc:      
      case analogMessageTypes::analog_msg_pc_m:   
      case analogMessageTypes::analog_msg_pc_p: { messageConfigType  =  MidiTypeYTX  ::  ProgramChange;  } break;
      case analogMessageTypes::analog_msg_nrpn: { messageConfigType  =  MidiTypeYTX  ::  NRPN;           } break;
      case analogMessageTypes::analog_msg_rpn:  { messageConfigType  =  MidiTypeYTX  ::  RPN;            } break;
      case analogMessageTypes::analog_msg_pb:   { messageConfigType  =  MidiTypeYTX  ::  PitchBend;      } break;
      default:                                  { messageConfigType  =  0;                               } break;
    }

    // If current analog config message is 14 bit
    if ( IS_ANALOG_FB_14_BIT(analogNo) ) {
      for (uint32_t idx = 0; idx < midiRxSettings.lastMidiBufferIndex14; idx++) {                   // Search every message already saved in 14 bit buffer
        if (midiMsgBuf14[idx].parameter == ((analog[analogNo].feedback.parameterMSB << 7) | 
                                                  (analog[analogNo].feedback.parameterLSB)) 
             || messageConfigType == MidiTypeYTX::PitchBend) {                                  // Check full 14 bit parameter. If pitch bend, don't check parameter
          if (midiMsgBuf14[idx].port == analog[analogNo].feedback.source) {                         // Check message source
            if (midiMsgBuf14[idx].channel == analog[analogNo].feedback.channel) {                   // Check channel
              if (midiMsgBuf14[idx].message == messageConfigType) {                                 // Check message type
                if (midiMsgBuf14[idx].type == FB_ANALOG) {                                          // Check fb type
                  thereIsAMatch                   = true;                                           // If there's a match, signal it,
                  midiMsgBuf14[idx].banksPresent |= (1<<currentBank);                               // flag that this message is present in current bank,
                  continue;                                                                         // and check next message                                                     
                }
              }
            }
          }
        }
      }
      // If 14 bit buffer isn't full and and there wasn't a match already saved, save new message
      if (midiRxSettings.lastMidiBufferIndex14 < midiRxSettings.midiBufferSize14 && !thereIsAMatch) {
//        SerialUSB.print(midiRxSettings.lastMidiBufferIndex14); SerialUSB.println(F(": NEW MIDI MESSAGE ADDED TO 14 BIT BUFFER"));
        midiMsgBuf14[midiRxSettings.lastMidiBufferIndex14].message      = messageConfigType;                            // Save message type in buffer
        midiMsgBuf14[midiRxSettings.lastMidiBufferIndex14].type         = FeebackTypes::FB_ANALOG;              // Save component type in buffer
        midiMsgBuf14[midiRxSettings.lastMidiBufferIndex14].port         = analog[analogNo].feedback.source;               // Save feedback source in buffer
        midiMsgBuf14[midiRxSettings.lastMidiBufferIndex14].channel      = analog[analogNo].feedback.channel;              // Save feedback channel in buffer
        if(messageConfigType == MidiTypeYTX::PitchBend){
          midiMsgBuf14[midiRxSettings.lastMidiBufferIndex14].parameter    =   0;                                          // Save feedback param in buffer
        }else{
          midiMsgBuf14[midiRxSettings.lastMidiBufferIndex14].parameter  = (analog[analogNo].feedback.parameterMSB << 7) |
                                                                          (analog[analogNo].feedback.parameterLSB);       // Save feedback full 14 bit param in buffer
        }

        midiMsgBuf14[midiRxSettings.lastMidiBufferIndex14].banksPresent |=  (1<<currentBank);                         // Flag that this message is present in current bank                                           
        midiMsgBuf14[midiRxSettings.lastMidiBufferIndex14++].value    = 0;                                            // Initialize value to 0
      }
    // If current encoder switch config message is 7 bit
    } else if( IS_ANALOG_FB_7_BIT(analogNo) ){
      for (uint32_t idx = 0; idx < midiRxSettings.lastMidiBufferIndex7; idx++) {        // Search every message already saved in 7 bit buffer
        if (midiMsgBuf7[idx].parameter == analog[analogNo].feedback.parameterLSB
            || messageConfigType == MidiTypeYTX::ProgramChange) {                       // check parameter  
          if (midiMsgBuf7[idx].port == analog[analogNo].feedback.source) {                  // Check source
            if (midiMsgBuf7[idx].channel == analog[analogNo].feedback.channel) {            // Check channel
              if (midiMsgBuf7[idx].message == messageConfigType) {                        // Check message
                if (midiMsgBuf7[idx].type == FB_ANALOG) {                                          // Check fb type
                  //                  SerialUSB.println(F("MIDI MESSAGE ALREADY IN 7 BIT BUFFER"));
                  thereIsAMatch                 = true;                                   // If there's a match, signal it,
                  midiMsgBuf7[idx].banksPresent |= (1<<currentBank);                      // flag that this message is present in current bank,
                  continue;                                                               // and check next message                  
                }
              }
            }
          }
        }
      }

      // If 7 bit buffer isn't full and and there wasn't a match already saved, save new message
      if (midiRxSettings.lastMidiBufferIndex7 < midiRxSettings.midiBufferSize7 && !thereIsAMatch) {
//        SerialUSB.print(midiRxSettings.lastMidiBufferIndex7); SerialUSB.println(F(": NEW MIDI MESSAGE ADDED TO 7 BIT BUFFER"));
        midiMsgBuf7[midiRxSettings.lastMidiBufferIndex7].message      =   messageConfigType;                      // Save message type in buffer
        midiMsgBuf7[midiRxSettings.lastMidiBufferIndex7].type         =   FeebackTypes::FB_ANALOG;                // Save component type in buffer
        midiMsgBuf7[midiRxSettings.lastMidiBufferIndex7].port         =   analog[analogNo].feedback.source;       // Save feedback source in buffer
        midiMsgBuf7[midiRxSettings.lastMidiBufferIndex7].channel      =   analog[analogNo].feedback.channel;      // Save feedback channel in buffer
        if(messageConfigType == MidiTypeYTX::ProgramChange){
          midiMsgBuf7[midiRxSettings.lastMidiBufferIndex7].parameter  =   0;                                          // Save feedback param in buffer
        }else{
          midiMsgBuf7[midiRxSettings.lastMidiBufferIndex7].parameter  =   analog[analogNo].feedback.parameterLSB; // Save feedback param in buffer
        }
        midiMsgBuf7[midiRxSettings.lastMidiBufferIndex7].banksPresent |=  (1<<currentBank);                       // Flag that this message is present in current bank
        midiMsgBuf7[midiRxSettings.lastMidiBufferIndex7++].value      =   0;                                      // Initialize value to 0
      }
    }
  }
}

void printMidiBuffer() {
  SerialUSB.print(F("7 BIT MIDI BUFFER - TOTAL LENGTH: ")); SerialUSB.print(midiRxSettings.midiBufferSize7); SerialUSB.println(F(" MESSAGES"));
  SerialUSB.print(F("7 BIT MIDI BUFFER - FILL LENGTH: ")); SerialUSB.print(midiRxSettings.lastMidiBufferIndex7); SerialUSB.println(F(" MESSAGES"));
  for (uint32_t idx = 0; idx < midiRxSettings.lastMidiBufferIndex7; idx++) {
    SerialUSB.print(idx);SerialUSB.print(F(" - "));
    SerialUSB.print(F("7 BIT - Type: ")); SerialUSB.print(  midiMsgBuf7[idx].type == FB_ENCODER ? "ENC. ROTARY" :
                                                            midiMsgBuf7[idx].type == FB_ENCODER_SWITCH ? "ENC. SWITCH" :
                                                            midiMsgBuf7[idx].type == FB_DIGITAL ? "DIGITAL" :
                                                            midiMsgBuf7[idx].type == FB_ANALOG ? "ANALOG" : 
                                                            midiMsgBuf7[idx].type == FB_2CC ? "2CC" : 
                                                            midiMsgBuf7[idx].type == FB_SHIFT ? "SHIFT ROT" : "UNDEFINED");
    SerialUSB.print(F("\tPort: ")); SerialUSB.print(  midiMsgBuf7[idx].port == 0 ? "LOCAL" :
                                                      midiMsgBuf7[idx].port == 1 ? "USB" :
                                                      midiMsgBuf7[idx].port == 2 ? "MIDI" :
                                                      midiMsgBuf7[idx].port == 3 ? "U+M" : "NOT DEFINED");
    SerialUSB.print(F("\tChannel: ")); SerialUSB.print(midiMsgBuf7[idx].channel);
    SerialUSB.print(F("\tMessage: ")); SerialUSB.print(midiMsgBuf7[idx].message, HEX);
    SerialUSB.print(F("\tBanks: ")); SerialUSB.print(midiMsgBuf7[idx].banksPresent, HEX);
    SerialUSB.print(F("\tParameter: ")); SerialUSB.println(midiMsgBuf7[idx].parameter); SerialUSB.println();

  }

  SerialUSB.print(F("14 BIT MIDI BUFFER - TOTAL LENGTH: ")); SerialUSB.print(midiRxSettings.midiBufferSize14); SerialUSB.println(F(" MESSAGES"));
  SerialUSB.print(F("14 BIT MIDI BUFFER - FILL LENGTH: ")); SerialUSB.print(midiRxSettings.lastMidiBufferIndex14); SerialUSB.println(F(" MESSAGES"));
  for (uint32_t idx = 0; idx < midiRxSettings.lastMidiBufferIndex14; idx++) {
    SerialUSB.print(F("14 BIT - Type: ")); SerialUSB.print( midiMsgBuf14[idx].type == FB_ENCODER ? "ENC. ROTARY" :
                                                            midiMsgBuf14[idx].type == FB_ENCODER_SWITCH ? "ENC. SWITCH" :
                                                            midiMsgBuf14[idx].type == FB_DIGITAL ? "DIGITAL" :
                                                            midiMsgBuf14[idx].type == FB_ANALOG ? "ANALOG" : 
                                                            midiMsgBuf14[idx].type == FB_2CC ? "2CC" : 
                                                            midiMsgBuf14[idx].type == FB_SHIFT ? "SHIFT ROT" : "UNDEFINED");
    SerialUSB.print(F("\tPort: ")); SerialUSB.print(  midiMsgBuf14[idx].port == 0 ? "LOCAL" :
                                                      midiMsgBuf14[idx].port == 1 ? "USB" :
                                                      midiMsgBuf14[idx].port == 2 ? "MIDI" :
                                                      midiMsgBuf14[idx].port == 3 ? "U+M" : "NOT DEFINED");
    SerialUSB.print(F("\tChannel: ")); SerialUSB.print(midiMsgBuf14[idx].channel);
    SerialUSB.print(F("\tMessage: ")); SerialUSB.print(midiMsgBuf14[idx].message, HEX);
    SerialUSB.print(F("\tBanks: ")); SerialUSB.print(midiMsgBuf14[idx].banksPresent, HEX);
    SerialUSB.print(F("\tParameter: ")); SerialUSB.println(midiMsgBuf14[idx].parameter); SerialUSB.println();

  }
}
