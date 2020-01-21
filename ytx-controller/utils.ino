
// UTILITIES

bool CheckIfBankShifter(uint16_t index, bool switchState) {
  static bool bankShifterPressed = false;
  static uint8_t prevBank = 0;

  if (config->banks.count > 1) {  // If there is more than one bank
    for (int bank = 0; bank < config->banks.count; bank++) { // Cycle all banks
      if (index == config->banks.shifterId[bank]) {           // If index matches to this bank's shifter
        bool toggleBank = ((config->banks.momToggFlags) >> bank) & 1;
        
        if (switchState && currentBank != bank && !bankShifterPressed) {
          SerialUSB.print("Bank pressed. "); SerialUSB.println(toggleBank ? "Toggle." : "Mommentary."); 
          prevBank = currentBank;
          currentBank = memHost->LoadBank(bank);
          bankShifterPressed = true;
          SetBankForAll(currentBank);
          feedbackHw.SetBankChangeFeedback();
        } else if (!switchState && currentBank == bank && !toggleBank && bankShifterPressed) {
          SerialUSB.print("Bank released. "); SerialUSB.println("Mommentary."); 
          bankShifterPressed = false;
          currentBank = memHost->LoadBank(prevBank);
          SetBankForAll(currentBank);
          feedbackHw.SetBankChangeFeedback();
//          SerialUSB.print("Returned to bank: "); SerialUSB.println(currentBank);
        } else if (!switchState && currentBank == bank && toggleBank && bankShifterPressed) {
          SerialUSB.print("Bank released. "); SerialUSB.println("Toggle."); 
          bankShifterPressed = false;
        }
        //          feedbackHw.SetChangeEncoderFeedback(FB_ENCODER_SWITCH, encNo, currentBank == bank, encMData[encNo/4].moduleOrientation);
        //          SerialUSB.println(bankShifterPressed ? "BANK PRESSED" : "BANK RELEASED");
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
  SerialUSB.print("HELP");
  feedbackHw.SendCommand(CMD_ALL_LEDS_OFF);
  uint8_t powerAdapterConnected = !digitalRead(pinExternalVoltage);
  static int sumBright = 0;

  antMillisPowerChange = millis();
  powerChangeFlag = true;

  if (powerAdapterConnected) {
    SerialUSB.println("Power connected");
    feedbackHw.SendCommand(CHANGE_BRIGHTNESS);
    feedbackHw.SendCommand(BRIGHTNESS_WITH_POWER);
    SetStatusLED(STATUS_BLINK, 3, STATUS_FB_CONFIG);
  } else {
    SerialUSB.println("Power disconnected");
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
   El intervalo es fijo y dado por la etiqueta 'STATUS_BLINK_INTERVAL'
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

        colorR = pgm_read_byte(&gamma8[statusLEDColor[statusLEDfbType] & 0xFF]);
        colorG = pgm_read_byte(&gamma8[(statusLEDColor[statusLEDfbType] >> 8) & 0xFF]);
        colorB = pgm_read_byte(&gamma8[(statusLEDColor[statusLEDfbType] >> 16) & 0xFF]);

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
    }
    else if (flagBlinkStatusLED == STATUS_ON) {
      statusLED.setPixelColor(0, statusLEDColor[statusLEDfbType]);
    } else if (flagBlinkStatusLED == STATUS_OFF) {
      statusLED.setPixelColor(0, statusLEDColor[statusLEDtypes::STATUS_FB_NONE]);
    }
  }
  return;
}
