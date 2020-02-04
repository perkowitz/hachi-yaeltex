
// UTILITIES

bool CheckIfBankShifter(uint16_t index, bool switchState) {
  static bool bankShifterPressed = false;
  static uint8_t prevBank = 0;

  if (config->banks.count > 1) {  // If there is more than one bank
    for (int bank = 0; bank < config->banks.count; bank++) { // Cycle all banks
      if (index == config->banks.shifterId[bank]) {           // If index matches to this bank's shifter
        bool toggleBank = ((config->banks.momToggFlags) >> bank) & 1;
        
        if (switchState && currentBank != bank && !bankShifterPressed) {
          prevBank = currentBank;                   // save previous bank for momentary bank shifters
          currentBank = memHost->LoadBank(bank);    // Load new bank in RAM
          bankShifterPressed = true;                // Set flag to indicate there is a bank shifter pressed
          SetBankForAll(currentBank);               // Set new bank for components
          for(int idx = 0; idx < midiRxSettings.lastMidiBufferIndex7; idx++){
            CheckAllAndUpdate(midiMsgBuf7[idx].message,
                              midiMsgBuf7[idx].channel,
                              midiMsgBuf7[idx].parameter,
                              midiMsgBuf7[idx].value,
                              midiMsgBuf7[idx].port);
          }
          for(int idx = 0; idx < midiRxSettings.lastMidiBufferIndex14; idx++){
            CheckAllAndUpdate(midiMsgBuf14[idx].message,
                              midiMsgBuf14[idx].channel,
                              midiMsgBuf14[idx].parameter,
                              midiMsgBuf14[idx].value,
                              midiMsgBuf14[idx].port);
          }
          
          feedbackHw.SetBankChangeFeedback();
        } else if (!switchState && currentBank == bank && !toggleBank && bankShifterPressed) {
          //          Bank released. "); SerialUSB.println(F("Momentary."));
          bankShifterPressed = false;
          currentBank = memHost->LoadBank(prevBank);
          for(int idx = 0; idx < midiRxSettings.lastMidiBufferIndex7; idx++){
            CheckAllAndUpdate(midiMsgBuf7[idx].message,
                              midiMsgBuf7[idx].channel,
                              midiMsgBuf7[idx].parameter,
                              midiMsgBuf7[idx].value,
                              midiMsgBuf7[idx].port);
          }
          for(int idx = 0; idx < midiRxSettings.lastMidiBufferIndex14; idx++){
            CheckAllAndUpdate(midiMsgBuf14[idx].message,
                              midiMsgBuf14[idx].channel,
                              midiMsgBuf14[idx].parameter,
                              midiMsgBuf14[idx].value,
                              midiMsgBuf14[idx].port);
          }
          SetBankForAll(currentBank);
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

void ChangeBrigthnessISR(void) {
  SerialUSB.print(F("HELP"));
  feedbackHw.SendCommand(CMD_ALL_LEDS_OFF);
  uint8_t powerAdapterConnected = !digitalRead(pinExternalVoltage);
  static int sumBright = 0;

  antMillisPowerChange = millis();
  powerChangeFlag = true;

  if (powerAdapterConnected) {
    SerialUSB.println(F("Power connected"));
    feedbackHw.SendCommand(CHANGE_BRIGHTNESS);
    feedbackHw.SendCommand(BRIGHTNESS_WITH_POWER);
    SetStatusLED(STATUS_BLINK, 3, STATUS_FB_CONFIG);
  } else {
    SerialUSB.println(F("Power disconnected"));
    feedbackHw.SendCommand(CHANGE_BRIGHTNESS);
    feedbackHw.SendCommand(BRIGHTNESS_WO_POWER);
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
    }else if (flagBlinkStatusLED == STATUS_ON) {
      statusLED.setPixelColor(0, statusLEDColor[statusLEDfbType]);
    }else if (flagBlinkStatusLED == STATUS_OFF) {
      statusLED.setPixelColor(0, statusLEDColor[statusLEDtypes::STATUS_FB_NONE]);
    }
  }
  return;
}

void MidiSettingsInit(){
  // If it is a regular message, check if it matches the feedback configuration for all the inputs (only the current bank)
  // SWEEP ALL ENCODERS
  
  for(uint8_t encNo = 0; encNo < config->inputs.encoderCount; encNo++){
    // SWEEP ALL ENCODERS
    for(uint8_t channel = 0; channel <= 15; channel++){
      if(encoder[encNo].rotaryFeedback.channel == channel){
        // If there's a match, set channel flag
        midiRxSettings.listenToChannel |= (1 << channel);
      }
      
      // SWEEP ALL ENCODER SWITCHES
      if(encoder[encNo].switchFeedback.channel == channel){
        midiRxSettings.listenToChannel |= (1 << channel);  
      }
    }
    if( encoder[encNo].rotaryFeedback.message == rotary_msg_nrpn || 
        encoder[encNo].rotaryFeedback.message == rotary_msg_rpn ||
        encoder[encNo].rotaryFeedback.message == rotary_msg_pb){
      midiRxSettings.midiBufferSize14++;
    }else if( encoder[encNo].rotaryFeedback.message == rotary_msg_note || 
              encoder[encNo].rotaryFeedback.message == rotary_msg_cc ||
              encoder[encNo].rotaryFeedback.message == rotary_msg_pc_rel){
      midiRxSettings.midiBufferSize7++;                  
    }
    if( encoder[encNo].switchFeedback.message == switch_msg_nrpn || 
        encoder[encNo].switchFeedback.message == switch_msg_rpn ||
        encoder[encNo].switchFeedback.message == switch_msg_pb){
      midiRxSettings.midiBufferSize14++;
    }else if( encoder[encNo].switchFeedback.message == switch_msg_note || 
              encoder[encNo].switchFeedback.message == switch_msg_cc ||
              encoder[encNo].switchFeedback.message == switch_msg_pc ||
              encoder[encNo].switchFeedback.message == switch_msg_pc_m ||
              encoder[encNo].switchFeedback.message == switch_msg_pc_p){
      midiRxSettings.midiBufferSize7++;                  
    }
    
  }
  // SWEEP ALL DIGITAL
  for(uint16_t digNo = 0; digNo < config->inputs.digitalCount; digNo++){
    for(uint8_t channel = 0; channel <= 15; channel++){
      // SWEEP ALL DIGITAL
      if(digital[digNo].feedback.channel == channel){
        midiRxSettings.listenToChannel |= (1 << channel);  
      }
    }
    // Add 14 bit messages
    if( digital[digNo].feedback.message == digital_msg_nrpn || 
        digital[digNo].feedback.message == digital_msg_rpn ||
        digital[digNo].feedback.message == digital_msg_pb){
      midiRxSettings.midiBufferSize14++;
    }else if( digital[digNo].feedback.message == digital_msg_note || 
              digital[digNo].feedback.message == digital_msg_cc ||
              digital[digNo].feedback.message == digital_msg_pc ||
              digital[digNo].feedback.message == digital_msg_pc_m ||
              digital[digNo].feedback.message == digital_msg_pc_p){
      midiRxSettings.midiBufferSize7++;                  
    }
  }
  // SWEEP ALL FEEDBACK
  for(uint8_t analogNo = 0; analogNo < config->inputs.analogCount; analogNo++){
    for(uint8_t channel = 0; channel <= 15; channel++){
      if(analog[analogNo].feedback.channel == channel){
        midiRxSettings.listenToChannel |= (1 << channel); 
      }
    }
    if( analog[analogNo].feedback.message == analog_msg_nrpn || 
        analog[analogNo].feedback.message == analog_msg_rpn ||
        analog[analogNo].feedback.message == analog_msg_pb){
      midiRxSettings.midiBufferSize14++;
    } else if ( analog[analogNo].feedback.message == analog_msg_note ||
                analog[analogNo].feedback.message == analog_msg_cc ||
                analog[analogNo].feedback.message == analog_msg_pc ||
                analog[analogNo].feedback.message == analog_msg_pc_m ||
                analog[analogNo].feedback.message == analog_msg_pc_p) {
      midiRxSettings.midiBufferSize7++;
    }
  }
}

void MidiBufferFill() {
  bool thereIsAMatch = false;

  for (uint8_t encNo = 0; encNo < config->inputs.encoderCount; encNo++) {
    thereIsAMatch = false;
    // SWEEP ALL ENCODERS
    if ( encoder[encNo].rotaryFeedback.message == rotaryMessageTypes::rotary_msg_nrpn ||
         encoder[encNo].rotaryFeedback.message == rotaryMessageTypes::rotary_msg_rpn ||
         encoder[encNo].rotaryFeedback.message == rotaryMessageTypes::rotary_msg_pb) {
      for (uint32_t idx = 0; idx < midiRxSettings.lastMidiBufferIndex14; idx++) {
        if (midiMsgBuf14[idx].port == encoder[encNo].rotaryFeedback.source) {
          if (midiMsgBuf14[idx].channel == encoder[encNo].rotaryFeedback.channel) {
            if (midiMsgBuf14[idx].message == encoder[encNo].rotaryFeedback.message) {
              if (midiMsgBuf14[idx].parameter == ((encoder[encNo].rotaryFeedback.parameterMSB << 7) |
                                                  (encoder[encNo].rotaryFeedback.parameterLSB))) {
//                SerialUSB.print(idx); SerialUSB.print(F(" - ENC: ")); SerialUSB.print(encNo);
//                SerialUSB.print(F(" - PARAM: ")); SerialUSB.print((encoder[encNo].rotaryFeedback.parameterMSB << 7) | (encoder[encNo].rotaryFeedback.parameterLSB));
//                SerialUSB.println(F(" - MIDI MESSAGE ALREADY IN 14 BIT BUFFER"));
                thereIsAMatch = true;
                break;
              }
            }
          }
        }
      }
      if (midiRxSettings.lastMidiBufferIndex14 < midiRxSettings.midiBufferSize14 && !thereIsAMatch) {
//        SerialUSB.print(midiRxSettings.lastMidiBufferIndex14); SerialUSB.println(F(": NEW MIDI MESSAGE ADDED TO 14 BIT BUFFER"));

        midiMsgBuf14[midiRxSettings.lastMidiBufferIndex14].type = FeebackTypes::FB_ENCODER;
        midiMsgBuf14[midiRxSettings.lastMidiBufferIndex14].port = encoder[encNo].rotaryFeedback.source;
        midiMsgBuf14[midiRxSettings.lastMidiBufferIndex14].channel = encoder[encNo].rotaryFeedback.channel;
        midiMsgBuf14[midiRxSettings.lastMidiBufferIndex14].message = encoder[encNo].rotaryFeedback.message;
        midiMsgBuf14[midiRxSettings.lastMidiBufferIndex14].parameter =  (encoder[encNo].rotaryFeedback.parameterMSB << 7) |
            (encoder[encNo].rotaryFeedback.parameterLSB);
        midiMsgBuf14[midiRxSettings.lastMidiBufferIndex14].value = 0;
        //        SerialUSB.print(midiMsgBuf14[midiRxSettings.lastMidiBufferIndex14].port ? "MIDI_HW: " : "MIDI_USB: ");
        //        SerialUSB.print(midiMsgBuf14[midiRxSettings.lastMidiBufferIndex14].message, HEX); SerialUSB.print("\t");
        //        SerialUSB.print(midiMsgBuf14[midiRxSettings.lastMidiBufferIndex14].channel); SerialUSB.print("\t");
        //        SerialUSB.print(midiMsgBuf14[midiRxSettings.lastMidiBufferIndex14].parameter); SerialUSB.print("\t");
        //        SerialUSB.println(midiMsgBuf14[midiRxSettings.lastMidiBufferIndex14].value);
        midiRxSettings.lastMidiBufferIndex14++;
      }
    } else {
      for (uint32_t idx = 0; idx < midiRxSettings.lastMidiBufferIndex7; idx++) {
        if (midiMsgBuf7[idx].port == encoder[encNo].rotaryFeedback.source) {
          if (midiMsgBuf7[idx].channel == encoder[encNo].rotaryFeedback.channel) {
            if (midiMsgBuf7[idx].message == encoder[encNo].rotaryFeedback.message) {
              if (midiMsgBuf7[idx].parameter == encoder[encNo].rotaryFeedback.parameterLSB) {
                //              SerialUSB.println(F("MIDI MESSAGE ALREADY IN 7 BIT BUFFER"));
                thereIsAMatch = true;
                break;
              }
            }
          }
        }
      }
      if (midiRxSettings.lastMidiBufferIndex7 < midiRxSettings.midiBufferSize7 && !thereIsAMatch) {
//        SerialUSB.print(midiRxSettings.lastMidiBufferIndex7); SerialUSB.println(F(": NEW MIDI MESSAGE ADDED TO 7 BIT BUFFER"));

        midiMsgBuf7[midiRxSettings.lastMidiBufferIndex7].type = FeebackTypes::FB_ENCODER;
        midiMsgBuf7[midiRxSettings.lastMidiBufferIndex7].port = encoder[encNo].rotaryFeedback.source;
        midiMsgBuf7[midiRxSettings.lastMidiBufferIndex7].channel = encoder[encNo].rotaryFeedback.channel;
        midiMsgBuf7[midiRxSettings.lastMidiBufferIndex7].message = encoder[encNo].rotaryFeedback.message;
        midiMsgBuf7[midiRxSettings.lastMidiBufferIndex7].parameter = encoder[encNo].rotaryFeedback.parameterLSB;
        midiMsgBuf7[midiRxSettings.lastMidiBufferIndex7].value = 0;
        //      SerialUSB.print(midiMsgBuf7[midiRxSettings.lastMidiBufferIndex7].port ? "MIDI_HW: " : "MIDI_USB: ");
        //      SerialUSB.print(midiMsgBuf7[midiRxSettings.lastMidiBufferIndex7].message, HEX); SerialUSB.print("\t");
        //      SerialUSB.print(midiMsgBuf7[midiRxSettings.lastMidiBufferIndex7].channel); SerialUSB.print("\t");
        //      SerialUSB.print(midiMsgBuf7[midiRxSettings.lastMidiBufferIndex7].parameter); SerialUSB.print("\t");
        //      SerialUSB.println(midiMsgBuf7[midiRxSettings.lastMidiBufferIndex7].value);
        midiRxSettings.lastMidiBufferIndex7++;
      }
    }
    // Reset flag to scan encoder switch message
    thereIsAMatch = false;
    if ( encoder[encNo].switchFeedback.message == switchMessageTypes::switch_msg_nrpn ||
         encoder[encNo].switchFeedback.message == switchMessageTypes::switch_msg_rpn ||
         encoder[encNo].switchFeedback.message == switchMessageTypes::switch_msg_pb) {
      for (uint32_t idx = 0; idx < midiRxSettings.lastMidiBufferIndex14; idx++) {
        if (midiMsgBuf14[idx].port == encoder[encNo].switchFeedback.source) {
          if (midiMsgBuf14[idx].channel == encoder[encNo].switchFeedback.channel) {
            if (midiMsgBuf14[idx].message == encoder[encNo].switchFeedback.message) {
              if (midiMsgBuf14[idx].parameter == ((encoder[encNo].switchFeedback.parameterMSB << 7) |
                                                  (encoder[encNo].switchFeedback.parameterLSB))) {
//                SerialUSB.print(idx); SerialUSB.println(F(" - MIDI MESSAGE ALREADY IN 14 BIT BUFFER"));
                thereIsAMatch = true;
                break;
              }
            }
          }
        }
      }
      if (midiRxSettings.lastMidiBufferIndex14 < midiRxSettings.midiBufferSize14 && !thereIsAMatch) {
//        SerialUSB.print(midiRxSettings.lastMidiBufferIndex14); SerialUSB.println(F(": NEW MIDI MESSAGE ADDED TO 14 BIT BUFFER"));

        midiMsgBuf14[midiRxSettings.lastMidiBufferIndex14].type = FeebackTypes::FB_ENCODER_SWITCH;
        midiMsgBuf14[midiRxSettings.lastMidiBufferIndex14].port = encoder[encNo].switchFeedback.source;
        midiMsgBuf14[midiRxSettings.lastMidiBufferIndex14].channel = encoder[encNo].switchFeedback.channel;
        midiMsgBuf14[midiRxSettings.lastMidiBufferIndex14].message = encoder[encNo].switchFeedback.message;
        midiMsgBuf14[midiRxSettings.lastMidiBufferIndex14].parameter =  (encoder[encNo].switchFeedback.parameterMSB << 7) |
            (encoder[encNo].switchFeedback.parameterLSB);
        midiMsgBuf14[midiRxSettings.lastMidiBufferIndex14].value = 0;
        //      SerialUSB.print(midiMsgBuf14[midiRxSettings.lastMidiBufferIndex14].port ? "MIDI_HW: " : "MIDI_USB: ");
        //      SerialUSB.print(midiMsgBuf14[midiRxSettings.lastMidiBufferIndex14].message, HEX); SerialUSB.print("\t");
        //      SerialUSB.print(midiMsgBuf14[midiRxSettings.lastMidiBufferIndex14].channel); SerialUSB.print("\t");
        //      SerialUSB.print(midiMsgBuf14[midiRxSettings.lastMidiBufferIndex14].parameter); SerialUSB.print("\t");
        //      SerialUSB.println(midiMsgBuf14[midiRxSettings.lastMidiBufferIndex14].value);
        midiRxSettings.lastMidiBufferIndex14++;
      }
    } else {
      for (uint32_t idx = 0; idx < midiRxSettings.lastMidiBufferIndex7; idx++) {
        if (midiMsgBuf7[idx].port == encoder[encNo].switchFeedback.source) {
          if (midiMsgBuf7[idx].channel == encoder[encNo].switchFeedback.channel) {
            if (midiMsgBuf7[idx].message == encoder[encNo].switchFeedback.message) {
              if (midiMsgBuf7[idx].parameter == encoder[encNo].switchFeedback.parameterLSB) {
                //              SerialUSB.println(F("MIDI MESSAGE ALREADY IN 7 BIT BUFFER"));
                thereIsAMatch = true;
                break;
              }
            }
          }
        }
      }
      if (midiRxSettings.lastMidiBufferIndex7 < midiRxSettings.midiBufferSize7 && !thereIsAMatch) {
//        SerialUSB.print(midiRxSettings.lastMidiBufferIndex7); SerialUSB.println(F(": NEW MIDI MESSAGE ADDED TO 7 BIT BUFFER"));

        midiMsgBuf7[midiRxSettings.lastMidiBufferIndex7].type = FeebackTypes::FB_ENCODER_SWITCH;
        midiMsgBuf7[midiRxSettings.lastMidiBufferIndex7].port = encoder[encNo].switchFeedback.source;
        midiMsgBuf7[midiRxSettings.lastMidiBufferIndex7].channel = encoder[encNo].switchFeedback.channel;
        midiMsgBuf7[midiRxSettings.lastMidiBufferIndex7].message = encoder[encNo].switchFeedback.message;
        midiMsgBuf7[midiRxSettings.lastMidiBufferIndex7].parameter = encoder[encNo].switchFeedback.parameterLSB;
        midiMsgBuf7[midiRxSettings.lastMidiBufferIndex7].value = 0;
        //      SerialUSB.print(midiMsgBuf7[midiRxSettings.lastMidiBufferIndex7].port ? "MIDI_HW: " : "MIDI_USB: ");
        //      SerialUSB.print(midiMsgBuf7[midiRxSettings.lastMidiBufferIndex7].message, HEX); SerialUSB.print("\t");
        //      SerialUSB.print(midiMsgBuf7[midiRxSettings.lastMidiBufferIndex7].channel); SerialUSB.print("\t");
        //      SerialUSB.print(midiMsgBuf7[midiRxSettings.lastMidiBufferIndex7].parameter); SerialUSB.print("\t");
        //      SerialUSB.println(midiMsgBuf7[midiRxSettings.lastMidiBufferIndex7].value);
        midiRxSettings.lastMidiBufferIndex7++;
      }
    }
  }

  // SWEEP ALL DIGITAL
  for (uint16_t digNo = 0; digNo < config->inputs.digitalCount; digNo++) {
    // Reset flag to scan digital messages
    thereIsAMatch = false;
    if ( digital[digNo].feedback.message == digitalMessageTypes::digital_msg_nrpn ||
         digital[digNo].feedback.message == digitalMessageTypes::digital_msg_rpn ||
         digital[digNo].feedback.message == digitalMessageTypes::digital_msg_pb) {
      for (uint32_t idx = 0; idx < midiRxSettings.lastMidiBufferIndex14; idx++) {
        if (midiMsgBuf14[idx].port == digital[digNo].feedback.source) {
          if (midiMsgBuf14[idx].channel == digital[digNo].feedback.channel) {
            if (midiMsgBuf14[idx].message == digital[digNo].feedback.message) {
              if (midiMsgBuf14[idx].parameter == ((digital[digNo].feedback.parameterMSB << 7) |
                                                  (digital[digNo].feedback.parameterLSB))) {
//                SerialUSB.print(idx); SerialUSB.println(F(" - MIDI MESSAGE ALREADY IN 14 BIT BUFFER"));
                thereIsAMatch = true;
                break;
              }
            }
          }
        }
      }
      if (midiRxSettings.lastMidiBufferIndex14 < midiRxSettings.midiBufferSize14 && !thereIsAMatch) {
//        SerialUSB.print(midiRxSettings.lastMidiBufferIndex14); SerialUSB.println(F(": NEW MIDI MESSAGE ADDED TO 14 BIT BUFFER"));

        midiMsgBuf14[midiRxSettings.lastMidiBufferIndex14].type = FeebackTypes::FB_DIGITAL;
        midiMsgBuf14[midiRxSettings.lastMidiBufferIndex14].port = digital[digNo].feedback.source;
        midiMsgBuf14[midiRxSettings.lastMidiBufferIndex14].channel = digital[digNo].feedback.channel;
        midiMsgBuf14[midiRxSettings.lastMidiBufferIndex14].message = digital[digNo].feedback.message;
        midiMsgBuf14[midiRxSettings.lastMidiBufferIndex14].parameter = (digital[digNo].feedback.parameterMSB << 7) |
            (digital[digNo].feedback.parameterLSB);
        midiMsgBuf14[midiRxSettings.lastMidiBufferIndex14].value = 0;
        //      SerialUSB.print(midiMsgBuf14[midiRxSettings.lastMidiBufferIndex14].port ? "MIDI_HW: " : "MIDI_USB: ");
        //      SerialUSB.print(midiMsgBuf14[midiRxSettings.lastMidiBufferIndex14].message, HEX); SerialUSB.print("\t");
        //      SerialUSB.print(midiMsgBuf14[midiRxSettings.lastMidiBufferIndex14].channel); SerialUSB.print("\t");
        //      SerialUSB.print(midiMsgBuf14[midiRxSettings.lastMidiBufferIndex14].parameter); SerialUSB.print("\t");
        //      SerialUSB.println(midiMsgBuf14[midiRxSettings.lastMidiBufferIndex14].value);
        midiRxSettings.lastMidiBufferIndex14++;
      }
    } else {
      for (uint32_t idx = 0; idx < midiRxSettings.lastMidiBufferIndex7; idx++) {
        if (midiMsgBuf7[idx].port == digital[digNo].feedback.source) {
          if (midiMsgBuf7[idx].channel == digital[digNo].feedback.channel) {
            if (midiMsgBuf7[idx].message == digital[digNo].feedback.message) {
              if (midiMsgBuf7[idx].parameter == digital[digNo].feedback.parameterLSB) {
                //              SerialUSB.println(F("MIDI MESSAGE ALREADY IN 7 BIT BUFFER"));
                thereIsAMatch = true;
                break;
              }
            }
          }
        }
      }
      if (midiRxSettings.lastMidiBufferIndex7 < midiRxSettings.midiBufferSize7 && !thereIsAMatch) {
//        SerialUSB.print(midiRxSettings.lastMidiBufferIndex7); SerialUSB.println(F(": NEW MIDI MESSAGE ADDED TO 7 BIT BUFFER"));

        midiMsgBuf7[midiRxSettings.lastMidiBufferIndex7].type = FeebackTypes::FB_DIGITAL;
        midiMsgBuf7[midiRxSettings.lastMidiBufferIndex7].port = digital[digNo].feedback.source;
        midiMsgBuf7[midiRxSettings.lastMidiBufferIndex7].channel = digital[digNo].feedback.channel;
        midiMsgBuf7[midiRxSettings.lastMidiBufferIndex7].message = digital[digNo].feedback.message;
        midiMsgBuf7[midiRxSettings.lastMidiBufferIndex7].parameter = digital[digNo].feedback.parameterLSB;
        midiMsgBuf7[midiRxSettings.lastMidiBufferIndex7].value = 0;
        //      SerialUSB.print(midiMsgBuf7[midiRxSettings.lastMidiBufferIndex7].port ? "MIDI_HW: " : "MIDI_USB: ");
        //      SerialUSB.print(midiMsgBuf7[midiRxSettings.lastMidiBufferIndex7].message, HEX); SerialUSB.print("\t");
        //      SerialUSB.print(midiMsgBuf7[midiRxSettings.lastMidiBufferIndex7].channel); SerialUSB.print("\t");
        //      SerialUSB.print(midiMsgBuf7[midiRxSettings.lastMidiBufferIndex7].parameter); SerialUSB.print("\t");
        //      SerialUSB.println(midiMsgBuf7[midiRxSettings.lastMidiBufferIndex7].value);
        midiRxSettings.lastMidiBufferIndex7++;
      }
    }
  }
  // SWEEP ALL FEEDBACK
  for (uint8_t analogNo = 0; analogNo < config->inputs.analogCount; analogNo++) {
    // Reset flag to scan digital messages
    thereIsAMatch = false;
    if ( analog[analogNo].feedback.message == analogMessageTypes::analog_msg_nrpn ||
         analog[analogNo].feedback.message == analogMessageTypes::analog_msg_rpn ||
         analog[analogNo].feedback.message == analogMessageTypes::analog_msg_pb) {
      for (uint32_t idx = 0; idx < midiRxSettings.lastMidiBufferIndex14; idx++) {
        if (midiMsgBuf14[idx].port == analog[analogNo].feedback.source) {
          if (midiMsgBuf14[idx].channel == analog[analogNo].feedback.channel) {
            if (midiMsgBuf14[idx].message == analog[analogNo].feedback.message) {
              if (midiMsgBuf14[idx].parameter == ((analog[analogNo].feedback.parameterMSB << 7) |
                                                  (analog[analogNo].feedback.parameterLSB))) {
//                SerialUSB.print(idx); SerialUSB.println(F(" - MIDI MESSAGE ALREADY IN 14 BIT BUFFER"));
                thereIsAMatch = true;
                break;
              }
            }
          }
        }
      }
      if (midiRxSettings.lastMidiBufferIndex14 < midiRxSettings.midiBufferSize14 && !thereIsAMatch) {
//        SerialUSB.print(midiRxSettings.lastMidiBufferIndex14); SerialUSB.println(F(": NEW MIDI MESSAGE ADDED TO 14 BIT BUFFER"));

        midiMsgBuf14[midiRxSettings.lastMidiBufferIndex14].type = FeebackTypes::FB_ANALOG;
        midiMsgBuf14[midiRxSettings.lastMidiBufferIndex14].port = analog[analogNo].feedback.source;
        midiMsgBuf14[midiRxSettings.lastMidiBufferIndex14].channel = analog[analogNo].feedback.channel;
        midiMsgBuf14[midiRxSettings.lastMidiBufferIndex14].message = analog[analogNo].feedback.message;
        midiMsgBuf14[midiRxSettings.lastMidiBufferIndex14].parameter = (analog[analogNo].feedback.parameterMSB << 7) |
            (analog[analogNo].feedback.parameterLSB);
        midiMsgBuf14[midiRxSettings.lastMidiBufferIndex14].value = 0;
        //      SerialUSB.print(midiMsgBuf14[midiRxSettings.lastMidiBufferIndex14].port ? "MIDI_HW: " : "MIDI_USB: ");
        //      SerialUSB.print(midiMsgBuf14[midiRxSettings.lastMidiBufferIndex14].message, HEX); SerialUSB.print("\t");
        //      SerialUSB.print(midiMsgBuf14[midiRxSettings.lastMidiBufferIndex14].channel); SerialUSB.print("\t");
        //      SerialUSB.print(midiMsgBuf14[midiRxSettings.lastMidiBufferIndex14].parameter); SerialUSB.print("\t");
        //      SerialUSB.println(midiMsgBuf14[midiRxSettings.lastMidiBufferIndex14].value);
        midiRxSettings.lastMidiBufferIndex14++;
      }
    } else {
      for (uint32_t idx = 0; idx < midiRxSettings.lastMidiBufferIndex7; idx++) {
        if (midiMsgBuf7[idx].port == analog[analogNo].feedback.source) {
          if (midiMsgBuf7[idx].channel == analog[analogNo].feedback.channel) {
            if (midiMsgBuf7[idx].message == analog[analogNo].feedback.message) {
              if (midiMsgBuf7[idx].parameter == analog[analogNo].feedback.parameterLSB) {
                //              SerialUSB.println(F("MIDI MESSAGE ALREADY IN 7 BIT BUFFER"));
                thereIsAMatch = true;
                break;
              }
            }
          }
        }
      }
      if (midiRxSettings.lastMidiBufferIndex7 < midiRxSettings.midiBufferSize7 && !thereIsAMatch) {
//        SerialUSB.print(midiRxSettings.lastMidiBufferIndex7); SerialUSB.println(F(": NEW MIDI MESSAGE ADDED TO 7 BIT BUFFER"));

        midiMsgBuf7[midiRxSettings.lastMidiBufferIndex7].type = FeebackTypes::FB_ANALOG;
        midiMsgBuf7[midiRxSettings.lastMidiBufferIndex7].port = analog[analogNo].feedback.source;
        midiMsgBuf7[midiRxSettings.lastMidiBufferIndex7].channel = analog[analogNo].feedback.channel;
        midiMsgBuf7[midiRxSettings.lastMidiBufferIndex7].message = analog[analogNo].feedback.message;
        midiMsgBuf7[midiRxSettings.lastMidiBufferIndex7].parameter = analog[analogNo].feedback.parameterLSB;
        midiMsgBuf7[midiRxSettings.lastMidiBufferIndex7].value = 0;
        //      SerialUSB.print(midiMsgBuf7[midiRxSettings.lastMidiBufferIndex7].port ? "MIDI_HW: " : "MIDI_USB: ");
        //      SerialUSB.print(midiMsgBuf7[midiRxSettings.lastMidiBufferIndex7].message, HEX); SerialUSB.print("\t");
        //      SerialUSB.print(midiMsgBuf7[midiRxSettings.lastMidiBufferIndex7].channel); SerialUSB.print("\t");
        //      SerialUSB.print(midiMsgBuf7[midiRxSettings.lastMidiBufferIndex7].parameter); SerialUSB.print("\t");
        //      SerialUSB.println(midiMsgBuf7[midiRxSettings.lastMidiBufferIndex7].value);
        midiRxSettings.lastMidiBufferIndex7++;
      }
    }
  }
}

void printMidiBuffer() {
  SerialUSB.print(F("7 BIT MIDI BUFFER - TOTAL LENGTH: ")); SerialUSB.print(midiRxSettings.midiBufferSize7); SerialUSB.println(F(" MESSAGES"));
  SerialUSB.print(F("7 BIT MIDI BUFFER - FILL LENGTH: ")); SerialUSB.print(midiRxSettings.lastMidiBufferIndex7); SerialUSB.println(F(" MESSAGES"));
  for (uint32_t idx = 0; idx < midiRxSettings.lastMidiBufferIndex7; idx++) {
    SerialUSB.print(F("7 BIT - Type: ")); SerialUSB.print( midiMsgBuf7[idx].type == 1 ? "ENC. ROTARY" :
        midiMsgBuf7[idx].type == 2 ? "ENC. SWITCH" :
        midiMsgBuf7[idx].type == 3 ? "DIGITAL" :
        midiMsgBuf7[idx].type == 4 ? "ANALOG" : "UNDEFINED");
    SerialUSB.print(F("\tPort: ")); SerialUSB.print( midiMsgBuf7[idx].port == 0 ? "LOCAL" :
        midiMsgBuf7[idx].port == 1 ? "USB" :
        midiMsgBuf7[idx].port == 2 ? "MIDI" :
        midiMsgBuf7[idx].port == 3 ? "U+M" : "NOT DEFINED");
    SerialUSB.print(F("\tChannel: ")); SerialUSB.print(midiMsgBuf7[idx].channel + 1);
    SerialUSB.print(F("\tMessage: ")); SerialUSB.print(midiMsgBuf7[idx].message);
    SerialUSB.print(F("\tParameter: ")); SerialUSB.println(midiMsgBuf7[idx].parameter); SerialUSB.println();

  }

  SerialUSB.print(F("14 BIT MIDI BUFFER - TOTAL LENGTH: ")); SerialUSB.print(midiRxSettings.midiBufferSize14); SerialUSB.println(F(" MESSAGES"));
  SerialUSB.print(F("14 BIT MIDI BUFFER - FILL LENGTH: ")); SerialUSB.print(midiRxSettings.lastMidiBufferIndex14); SerialUSB.println(F(" MESSAGES"));
  for (uint32_t idx = 0; idx < midiRxSettings.lastMidiBufferIndex14; idx++) {
    SerialUSB.print(F("14 BIT - Type: ")); SerialUSB.print(  midiMsgBuf14[idx].type == 1 ? "ENC. ROTARY" :
        midiMsgBuf14[idx].type == 2 ? "ENC. SWITCH" :
        midiMsgBuf14[idx].type == 3 ? "DIGITAL" :
        midiMsgBuf14[idx].type == 4 ? "ANALOG" : "UNDEFINED");
    SerialUSB.print(F("\tPort: ")); SerialUSB.print( midiMsgBuf14[idx].port == 0 ? "LOCAL" :
        midiMsgBuf14[idx].port == 1 ? "USB" :
        midiMsgBuf14[idx].port == 2 ? "MIDI" :
        midiMsgBuf14[idx].port == 3 ? "U+M" : "NOT DEFINED");
    SerialUSB.print(F("\tChannel: ")); SerialUSB.print(midiMsgBuf14[idx].channel + 1);
    SerialUSB.print(F("\tMessage: ")); SerialUSB.print(midiMsgBuf14[idx].message);
    SerialUSB.print(F("\tParameter: ")); SerialUSB.println(midiMsgBuf14[idx].parameter); SerialUSB.println();

  }
}
