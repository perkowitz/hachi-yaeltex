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
#include "headers/Defines.h"

bool CheckIfBankShifter(uint16_t index, bool switchState) {
  static bool bankShifterPressed = false;
  static uint8_t prevBank = 0;
  static unsigned long antMicrosBank = 0;
  
  if (config->banks.count > 1) {  // If there is more than one bank
    for (int bank = 0; bank < config->banks.count; bank++) { // Cycle all banks
      if (index == config->banks.shifterId[bank]) {           // If index matches to this bank's shifter
             
        bool toggleBank = ((config->banks.momToggFlags) >> bank) & 1;

        if (switchState && !bankShifterPressed) {
          prevBank = currentBank;                   // save previous bank for momentary bank shifters
          
          currentBank = memHost->LoadBank(bank);    // Load new bank in RAM
          
          bankShifterPressed = true;                // Set flag to indicate there is a bank shifter pressed
          
          // send update to the rest of the set
          if(config->board.remoteBanks){
            MIDI.sendProgramChange(currentBank, BANK_CHANGE_CHANNEL);
            MIDIHW.sendProgramChange(currentBank, BANK_CHANGE_CHANNEL);
          }
            
          // Send component info if enabled
          byte sectionIndex = 0;
          if(componentInfoEnabled){
            if(index < config->inputs.encoderCount){
              sectionIndex = index;
              SendComponentInfo(ytxIOBLOCK::Encoder, sectionIndex);
            }else if(index < (config->inputs.encoderCount + config->inputs.digitalCount)){
              sectionIndex = index - config->inputs.encoderCount;  
              SendComponentInfo(ytxIOBLOCK::Digital, sectionIndex);
            }else if(index < (config->inputs.encoderCount + config->inputs.digitalCount + config->inputs.analogCount)){
              sectionIndex = index - config->inputs.encoderCount - config->inputs.digitalCount;  
              SendComponentInfo(ytxIOBLOCK::Analog, sectionIndex);
            }
          }

          SetStatusLED(STATUS_BLINK, 1, statusLEDtypes::STATUS_FB_MSG_OUT);

          ScanMidiBufferAndUpdate(currentBank, NO_QSTB_LOAD, 0);                
 
          SetBankForAll(currentBank);               // Set new bank for components that need it

          feedbackHw.SetBankChangeFeedback();  
          bankUpdateFirstTime = true;
          // feedbackHw.SetBankChangeFeedback();  

        } else if (!switchState && currentBank == bank && !toggleBank && bankShifterPressed) {
          currentBank = memHost->LoadBank(prevBank);
          bankShifterPressed = false;
          
          // send update to the rest of the set
          if(config->board.remoteBanks){
            MIDI.sendProgramChange(currentBank, BANK_CHANGE_CHANNEL);
            MIDIHW.sendProgramChange(currentBank, BANK_CHANGE_CHANNEL);
          }
          
          SetStatusLED(STATUS_BLINK, 1, statusLEDtypes::STATUS_FB_MSG_OUT);

          ScanMidiBufferAndUpdate(currentBank, NO_QSTB_LOAD, 0);
          
          SetBankForAll(currentBank);
          
          feedbackHw.SetBankChangeFeedback();
          bankUpdateFirstTime = true;
          // feedbackHw.SetBankChangeFeedback();  
        } else if (!switchState && currentBank == bank && toggleBank && bankShifterPressed) {
          bankShifterPressed = false;
        }
        
        return true;
      }
    }
   
  }
  return false;
}

bool MidiBankChange(uint16_t newBank){
  if(newBank != currentBank){
    currentBank = memHost->LoadBank(newBank);    // Load new bank in RAM
  
    ScanMidiBufferAndUpdate(newBank, false, 0);                

    SetBankForAll(currentBank);               // Set new bank for components that need it

    feedbackHw.SetBankChangeFeedback();
  }        
}

bool IsShifter(uint16_t index) {
  if (config->banks.count > 1) {  // If there is more than one bank
    for (int bank = 0; bank < config->banks.count; bank++) { // Cycle all banks
      if (index == config->banks.shifterId[bank]) {           // If index matches to this bank's shifter
        return true;      
      }
    }
  }
  return false;
}

void ScanMidiBufferAndUpdate(uint8_t newBank, bool qstb, uint8_t encNo){
  for (int idx = 0; idx < midiRxSettings.lastMidiBufferIndex7; idx++) {
    if((midiMsgBuf7[idx].banksToUpdate >> newBank) & 0x1){
      if(!qstb){
        midiMsgBuf7[idx].banksToUpdate &= ~(1 << newBank);  // Reset bank flag
        SearchMsgInConfigAndUpdate( midiMsgBuf7[idx].type,      // Check for configuration match for this message, and update all that match
                                    midiMsgBuf7[idx].message,
                                    midiMsgBuf7[idx].channel,
                                    midiMsgBuf7[idx].parameter,
                                    midiMsgBuf7[idx].value,
                                    midiMsgBuf7[idx].port);
      }else if(qstb && midiMsgBuf7[idx].type == FB_ENCODER){    // Special case for QSTB, update only the encoder that matches.
        bool valueUpdated = QSTBUpdateValue(newBank, encNo,
                                            midiMsgBuf7[idx].message, 
                                            midiMsgBuf7[idx].channel, 
                                            midiMsgBuf7[idx].parameter, 
                                            midiMsgBuf7[idx].value, 
                                            midiMsgBuf7[idx].port);
        if(valueUpdated) midiMsgBuf7[idx].banksToUpdate &= ~(1 << newBank);
      }   
    }
  } 
  for (int idx = 0; idx < midiRxSettings.lastMidiBufferIndex14; idx++) {
    if((midiMsgBuf14[idx].banksToUpdate >> newBank) & 0x1){
      if(!qstb){
        // Reset bank flag
        midiMsgBuf14[idx].banksToUpdate &= ~(1 << newBank);
        SearchMsgInConfigAndUpdate( midiMsgBuf14[idx].type,
                                    midiMsgBuf14[idx].message,
                                    midiMsgBuf14[idx].channel,
                                    midiMsgBuf14[idx].parameter,
                                    midiMsgBuf14[idx].value,
                                    midiMsgBuf14[idx].port);
      }else if(qstb && midiMsgBuf14[idx].type == FB_ENCODER){    // Special case for QSTB, update only the encoder that matches.
        bool valueUpdated = QSTBUpdateValue(newBank, encNo,
                                            midiMsgBuf14[idx].message, 
                                            midiMsgBuf14[idx].channel, 
                                            midiMsgBuf14[idx].parameter, 
                                            midiMsgBuf14[idx].value, 
                                            midiMsgBuf14[idx].port);
        if(valueUpdated) midiMsgBuf14[idx].banksToUpdate &= ~(1 << newBank);
      }
    }
  }
}

bool QSTBUpdateValue(byte newBank, byte encNo, byte msgType, byte channel, uint16_t param, uint16_t value, uint8_t midiSrc){
  // If it is a regular message, check if it matches the feedback configuration for all the inputs (only the current bank)
  byte messageToCompare = 0;
  uint16_t paramToCompare = 0;
  byte portToCompare = 0;
  switch(msgType){
    case MidiTypeYTX::NoteOn:         { messageToCompare = rotaryMessageTypes::rotary_msg_note;   
                                        paramToCompare = encoder[encNo].rotaryFeedback.parameterLSB;  } break;
    case MidiTypeYTX::ControlChange:  { messageToCompare = 
                                        (encoder[encNo].rotaryFeedback.message == rotaryMessageTypes::rotary_msg_cc) ?
                                                rotaryMessageTypes::rotary_msg_cc :
                                                rotaryMessageTypes::rotary_msg_vu_cc;             
                                        paramToCompare = encoder[encNo].rotaryFeedback.parameterLSB;  } break;
    case MidiTypeYTX::ProgramChange:  { messageToCompare = rotaryMessageTypes::rotary_msg_pc_rel;     } break;    
    case MidiTypeYTX::NRPN:           { messageToCompare = rotaryMessageTypes::rotary_msg_nrpn;      
                                        paramToCompare =  encoder[encNo].rotaryFeedback.parameterMSB<<7 | 
                                                          encoder[encNo].rotaryFeedback.parameterLSB; } break;
    case MidiTypeYTX::RPN:            { messageToCompare = rotaryMessageTypes::rotary_msg_rpn;    
                                        paramToCompare =  encoder[encNo].rotaryFeedback.parameterMSB<<7 | 
                                                          encoder[encNo].rotaryFeedback.parameterLSB; } break;
    case MidiTypeYTX::PitchBend:      { messageToCompare = rotaryMessageTypes::rotary_msg_pb;         } break;
  }

  if( paramToCompare == param  || 
      messageToCompare == rotaryMessageTypes::rotary_msg_pb ||
      messageToCompare == rotaryMessageTypes::rotary_msg_pc_rel){

    if(encoder[encNo].rotaryFeedback.channel == channel){
      if(encoder[encNo].rotaryFeedback.message == messageToCompare){
        // SerialUSB.println("ENCODER MSG FOUND");
        if(encoder[encNo].rotaryFeedback.source & midiSrc){    
          // If there's a match, set encoder value and feedback
          if(encoderHw.GetEncoderValue(encNo) != value || encoder[encNo].rotBehaviour.hwMode != rotaryModes::rot_absolute){
            encoderHw.SetEncoderValue(newBank, encNo, value);
            return true;
          }
        }
      }
    }
  }
  return false;
}


void SetBankForAll(uint8_t newBank) {
  encoderHw.SetBankForEncoders(newBank);
  analogHw.SetBankForAnalog(newBank);
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
}

void SelfReset() {
  SerialUSB.println("Rebooting...");
  
  SPI.end();
  SerialUSB.end();
  Serial.end();
  // USBDevice.detach();
  // USBDevice.end();
  delay(200);

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
    //SetStatusLED(STATUS_BLINK, 3, STATUS_FB_INIT);
  } else {
    // SerialUSB.println(F("Power disconnected"));
    feedbackHw.SendCommand(CHANGE_BRIGHTNESS);
    feedbackHw.SendCommand(currentBrightness);
    //    feedbackHw.SendCommand(BRIGHNESS_WO_POWER+sumBright);
    //SerialUSB.println(BRIGHNESS_WO_POWER+sumBright);
    //SetStatusLED(STATUS_BLINK, 1, STATUS_FB_INIT);
  }
}

long mapl(long x, long in_min, long in_max, long out_min, long out_max)
{
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}



void SetStatusLED(uint8_t onOrBlinkOrOff, uint8_t nTimes, uint8_t status_type) {
  
  // SerialUSB.print("BLINK FLAG: "); SerialUSB.print(onOrBlinkOrOff);
  // SerialUSB.print("\tN TIMES: "); SerialUSB.print(nTimes);
  // SerialUSB.print("\tSTATUS FB TYPE: "); SerialUSB.println(status_type);

 if (!flagBlinkStatusLED) {
    flagBlinkStatusLED = onOrBlinkOrOff;
    statusLEDfbType = status_type;
    blinkCountStatusLED = nTimes;
  
    switch (statusLEDfbType) {
      case STATUS_FB_NONE: {
          blinkInterval = 0;
        } break;
      case STATUS_FB_INIT:
      case STATUS_FB_CONFIG_IN: 
      case STATUS_FB_CONFIG_OUT: {
          blinkInterval = STATUS_CONFIG_BLINK_INTERVAL;
        } break;
      case STATUS_FB_MSG_OUT:
      case STATUS_FB_MSG_IN: {
          blinkInterval = STATUS_MIDI_BLINK_INTERVAL;
        } break;
      case STATUS_FB_ERROR: {
          blinkInterval = STATUS_ERROR_BLINK_INTERVAL;
        } break;
      case STATUS_FB_NO_CONFIG: {
          blinkInterval = STATUS_CONFIG_ERROR_BLINK_INTERVAL;
        } break;
      default:
        blinkInterval = 0; break;
    }
  }
}

/*
   Esta función es llamada por el loop principal, cuando las variables flagBlinkStatusLED y blinkCountStatusLED son distintas de cero.
   blinkCountStatusLED tiene la cantidad de veces que debe titilar el LED.
   El intervalo de parpadeo depende del tipo de status que se quiere representar
*/
void UpdateStatusLED() {
  uint8_t colorR = 0, colorG = 0, colorB = 0;
  if (flagBlinkStatusLED != STATUS_NONE && blinkCountStatusLED) {

    if (firstTime) {
      firstTime = false;
      millisStatusPrev = millis();
    }

    colorR = pgm_read_byte(&gamma8[(statusLEDColor[statusLEDfbType] >> 16) & 0xFF]);
    colorG = pgm_read_byte(&gamma8[(statusLEDColor[statusLEDfbType] >> 8) & 0xFF]);
    colorB = pgm_read_byte(&gamma8[statusLEDColor[statusLEDfbType] & 0xFF]);
    
    if (flagBlinkStatusLED == STATUS_BLINK) {
      if (millis() - millisStatusPrev > blinkInterval) {
        statusLED->clear();

        millisStatusPrev = millis();
        lastStatusLEDState = !lastStatusLEDState;
        
        if (lastStatusLEDState) {
          statusLED->setPixelColor(0, colorR, colorG, colorB); // Moderately bright green color.
        } else {
          statusLED->setPixelColor(0, 0, 0, 0); // Moderately bright green color.
          statusLED->show();                    // This sends the updated pixel color to the hardware.

          blinkCountStatusLED--;
        }
        
        statusLED->show(); // This sends the updated pixel color to the hardware.
       
        if (!blinkCountStatusLED) {
          flagBlinkStatusLED = STATUS_NONE;
          statusLEDfbType = 0;
          firstTime = true;
        }
      }
    } else if (flagBlinkStatusLED == STATUS_ON) {
      // statusLED->SetPixelColor(0, colorR, colorG, colorB); // Moderately bright green color.
      // statusLED->Show(); // This sends the updated pixel color to the hardware.
      flagBlinkStatusLED = STATUS_NONE;
    } else if (flagBlinkStatusLED == STATUS_OFF) {
      // statusLED.SetPixelColor(0, 0, 0, 0);
      // statusLED.Show(); // This sends the updated pixel color to the hardware.
      flagBlinkStatusLED = STATUS_NONE;
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


void MidiBufferInit() {
#if defined(USE_KWHAT_COUNT_BUFFER)
  // If there is a valid configuration, set number of messages
  midiRxSettings.midiBufferSize7  = config->board.qtyMessages7bit;
  midiRxSettings.midiBufferSize14 = config->board.qtyMessages14bit;  
#else
  // While rainbow is on, initialize MIDI buffer
    for (int b = 0; b < config->banks.count; b++) {
      currentBank = memHost->LoadBank(b);
      for (uint8_t encNo = 0; encNo < config->inputs.encoderCount; encNo++) {
        // SWEEP ALL ENCODERS
        if(encoder[encNo].rotaryFeedback.source != feedbackSource::fb_src_local){
          // Set channel flags to filter channels of incoming messages quickly
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
        // Add 14 bit messages
        if      ( IS_DIGITAL_FB_14_BIT(digNo) ) { midiRxSettings.midiBufferSize14++; } 
        else if ( IS_DIGITAL_FB_7_BIT(digNo)  ) { midiRxSettings.midiBufferSize7++;  }
      }
      // SWEEP ALL ANALOG
      for (uint8_t analogNo = 0; analogNo < config->inputs.analogCount; analogNo++) {
        if(analog[analogNo].feedback.source == feedbackSource::fb_src_local) continue; // If feedback source is local, don't count

        if      ( IS_ANALOG_FB_14_BIT(analogNo) ) { midiRxSettings.midiBufferSize14++;  } 
        else if ( IS_ANALOG_FB_7_BIT(analogNo)  ) { midiRxSettings.midiBufferSize7++;   }
      }
    }
  #endif  
    // Calculate and dinamically allocate entries for MIDI buffer
    if(midiRxSettings.midiBufferSize7 > MIDI_BUF_MAX_LEN) midiRxSettings.midiBufferSize7 = MIDI_BUF_MAX_LEN;
    
    midiMsgBuf7 = (midiMsgBuffer7*) memHost->AllocateRAM(midiRxSettings.midiBufferSize7*sizeof(midiMsgBuffer7));
    midiMsgBuf14 = (midiMsgBuffer14*) memHost->AllocateRAM(midiRxSettings.midiBufferSize14*sizeof(midiMsgBuffer14));
    SerialUSB.print(F("midi buffer size 7 bit: ")); SerialUSB.println(midiRxSettings.midiBufferSize7);
    SerialUSB.print(F("midi buffer size 14 bit: ")); SerialUSB.println(midiRxSettings.midiBufferSize14);

    MidiBufferInitClear();
    
    for (int b = 0; b < config->banks.count; b++) {
      currentBank = memHost->LoadBank(b);
      MidiBufferFill();
    }
    
    if(midiRxSettings.lastMidiBufferIndex7)   bubbleSort7(midiMsgBuf7, midiRxSettings.lastMidiBufferIndex7);
    if(midiRxSettings.lastMidiBufferIndex14)  bubbleSort14(midiMsgBuf14, midiRxSettings.lastMidiBufferIndex14);
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
    if(encoder[encNo].rotaryConfig.message  == rotaryMessageTypes::rotary_msg_key ||
      (encoder[encNo].switchConfig.message  == switchMessageTypes::switch_msg_key &&
       encoder[encNo].switchConfig.mode     == switchModes::switch_mode_message   )) {
      keyboardInit = true;
    }

    thereIsAMatch = false;                                                    // Set flag to signal msg match false for new check
    
    if(encoder[encNo].rotaryFeedback.source != feedbackSource::fb_src_local){ // Don't save in buffer if feedback source is local
      // Get MIDI type from config type
      switch(encoder[encNo].rotaryFeedback.message){
        case rotaryMessageTypes::rotary_msg_note:   { messageConfigType  =   MidiTypeYTX  ::  NoteOn;         } break;
        case rotaryMessageTypes::rotary_msg_cc:
        case rotaryMessageTypes::rotary_msg_vu_cc:  { messageConfigType  =   MidiTypeYTX  ::  ControlChange;  } break;
        case rotaryMessageTypes::rotary_msg_pc_rel: { messageConfigType  =   MidiTypeYTX  ::  ProgramChange;  } break;
        case rotaryMessageTypes::rotary_msg_nrpn:   { messageConfigType  =   MidiTypeYTX  ::  NRPN;           } break;
        case rotaryMessageTypes::rotary_msg_rpn:    { messageConfigType  =   MidiTypeYTX  ::  RPN;            } break;
        case rotaryMessageTypes::rotary_msg_pb:     { messageConfigType  =   MidiTypeYTX  ::  PitchBend;      } break;
        default:                                    { messageConfigType  =   MidiTypeYTX  ::  InvalidType;    } break;
      }

      uint16_t takeoverIndex = 0;
      // If current encoder rotary config message is 7 bit
      if ( IS_ENCODER_ROT_FB_14_BIT(encNo) ) {
        for (uint32_t idx = 0; idx < midiRxSettings.lastMidiBufferIndex14; idx++) {               // Search every message already saved in 14 bit buffer
          if (midiMsgBuf14[idx].parameter == ((encoder[encNo].rotaryFeedback.parameterMSB << 7) | 
                                              (encoder[encNo].rotaryFeedback.parameterLSB)) 
             || messageConfigType == MidiTypeYTX::PitchBend) {                                // Check full 14 bit parameter. If pitch bend, don't check parameter
            if (midiMsgBuf14[idx].port & encoder[encNo].rotaryFeedback.source) {                 // Check message source
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
            if (midiMsgBuf7[idx].port & encoder[encNo].rotaryFeedback.source) {                // Check source
              if (midiMsgBuf7[idx].channel == encoder[encNo].rotaryFeedback.channel) {          // Check channel
                if (midiMsgBuf7[idx].message == messageConfigType) {                            // Check message
                  if (midiMsgBuf7[idx].type == FB_ENCODER || 
                      midiMsgBuf7[idx].type == FB_ENC_VUMETER) {                                    // Check fb type
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
          midiMsgBuf7[midiRxSettings.lastMidiBufferIndex7].message        =   messageConfigType;                            // Save message type in buffer
          midiMsgBuf7[midiRxSettings.lastMidiBufferIndex7].type           =   FeebackTypes::FB_ENCODER;                     // Save component type in buffer  
          midiMsgBuf7[midiRxSettings.lastMidiBufferIndex7].port           =   encoder[encNo].rotaryFeedback.source;         // Save feedback source in buffer
          midiMsgBuf7[midiRxSettings.lastMidiBufferIndex7].channel        =   encoder[encNo].rotaryFeedback.channel;        // Save feedback channel in buffer
          if(messageConfigType == MidiTypeYTX::ProgramChange){
            midiMsgBuf7[midiRxSettings.lastMidiBufferIndex7].parameter    =   0;                                            // Save feedback param in buffer
          }else{
            midiMsgBuf7[midiRxSettings.lastMidiBufferIndex7].parameter    =   encoder[encNo].rotaryFeedback.parameterLSB;   // Save feedback param in buffer
          }
          midiMsgBuf7[midiRxSettings.lastMidiBufferIndex7].banksPresent   |=  (1<<currentBank);                             // Flag that this message is present in current bank
          midiMsgBuf7[midiRxSettings.lastMidiBufferIndex7++].value        =   0;                                            // Initialize value to 0
          // IF FEEDBACK IS CONFIGURED AS VUMETER CC, ADD A NEW ENTRY TO THE MIDI RX BUFFER
          if(encoder[encNo].rotaryFeedback.message == rotaryMessageTypes::rotary_msg_vu_cc){
            midiMsgBuf7[midiRxSettings.lastMidiBufferIndex7].message        =   messageConfigType;                            // Save message type in buffer
            midiMsgBuf7[midiRxSettings.lastMidiBufferIndex7].type           =   FeebackTypes::FB_ENC_VUMETER;                 // Save component type in buffer
            midiMsgBuf7[midiRxSettings.lastMidiBufferIndex7].port           =   encoder[encNo].rotaryFeedback.source;         // Save feedback source in buffer
            midiMsgBuf7[midiRxSettings.lastMidiBufferIndex7].channel        =   VUMETER_CHANNEL;                              // Save vumeter channel
            if(messageConfigType == MidiTypeYTX::ProgramChange){
              midiMsgBuf7[midiRxSettings.lastMidiBufferIndex7].parameter    =   0;                                            // Save feedback param in buffer
            }else{
              midiMsgBuf7[midiRxSettings.lastMidiBufferIndex7].parameter    =   encoder[encNo].rotaryFeedback.parameterLSB;   // Save feedback param in buffer
            }
            midiMsgBuf7[midiRxSettings.lastMidiBufferIndex7].banksPresent   |=  (1<<currentBank);                             // Flag that this message is present in current bank
            midiMsgBuf7[midiRxSettings.lastMidiBufferIndex7++].value        =   0;                                            // Initialize value to 0
          }     
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
            if (midiMsgBuf14[idx].port & encoder[encNo].switchFeedback.source) {                       // Check message source
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
            midiMsgBuf7[midiRxSettings.lastMidiBufferIndex7].message    =   MidiTypeYTX ::ControlChange;                // Save CC message type in buffer 
            midiMsgBuf7[midiRxSettings.lastMidiBufferIndex7].type       =   FeebackTypes::FB_2CC;                         // Save component type in buffer
          }else if(encoder[encNo].switchConfig.mode == switchModes::switch_mode_shift_rot){
            midiMsgBuf7[midiRxSettings.lastMidiBufferIndex7].type       =   FeebackTypes::FB_SHIFT;                       // Save component type in buffer 
          }else{
            midiMsgBuf7[midiRxSettings.lastMidiBufferIndex7].type       =   FeebackTypes::FB_ENCODER_SWITCH;              // Save component type in buffer 
          }
          
          midiMsgBuf7[midiRxSettings.lastMidiBufferIndex7].port         =   encoder[encNo].switchFeedback.source;         // Save feedback source in buffer
          midiMsgBuf7[midiRxSettings.lastMidiBufferIndex7].channel      =   encoder[encNo].switchFeedback.channel;        // Save feedback channel in buffer
          if(midiMsgBuf7[midiRxSettings.lastMidiBufferIndex7].message == MidiTypeYTX::ProgramChange){
            midiMsgBuf7[midiRxSettings.lastMidiBufferIndex7].parameter  =   0;                                            // Save feedback param in buffer
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
    
    if( digital[digNo].actionConfig.message == digitalMessageTypes::digital_msg_key){
      keyboardInit = true;
    }

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
    
    if( analog[analogNo].message == analogMessageTypes::analog_msg_key){
      keyboardInit = true;
    }

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
                                                            midiMsgBuf7[idx].type == FB_ENC_VUMETER ? "ENC. VUMETER" :
                                                            midiMsgBuf7[idx].type == FB_ENCODER_SWITCH ? "ENC. SWITCH" :
                                                            midiMsgBuf7[idx].type == FB_DIGITAL ? "DIGITAL" :
                                                            midiMsgBuf7[idx].type == FB_ANALOG ? "ANALOG" : 
                                                            midiMsgBuf7[idx].type == FB_2CC ? "2CC\t" : 
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
