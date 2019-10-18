
void CheckIfBankShifter(uint16_t index, bool switchState){
  static bool bankShifterPressed = false;
  static uint8_t prevBank = 0;

  if(nBanks > 1){
    for(int bank = 0; bank < config->banks.count; bank++){
      if(encNo == config->banks.shifterId[bank]){
        for(int i = 15; i>=0; i--){
          SerialUSB.print(((config->banks.momToggFlags)>>i)&1,BIN);
        }
        SerialUSB.println();
        bool toggleBank = ((config->banks.momToggFlags)>>bank)&1;
        SerialUSB.print("THIS BUTTON IS A BANK SHIFTER WORKING IN ");
        SerialUSB.println(toggleBank ? "TOGGLE MODE" : "MOMENTARY MODE");
        if(switchState && currentBank != bank){
          prevBank = currentBank;
          currentBank = memHost->LoadBank(bank);
          bankShifterPressed = true;
          SetBankForAll(currentBank);
          SerialUSB.print("Loaded Bank: "); SerialUSB.println(currentBank);
          feedbackHw.SetBankChangeFeedback();
        }else if(switchState && currentBank == bank && !toggleBank && bankShifterPressed){
          bankShifterPressed = false;
          currentBank = memHost->LoadBank(prevBank);
          SetBankForAll(currentBank);
          feedbackHw.SetBankChangeFeedback();
          SerialUSB.print("Returned to bank: "); SerialUSB.println(currentBank);
        }else if(switchState && currentBank == bank && toggleBank && bankShifterPressed){
          bankShifterPressed = false;
        }
//          feedbackHw.SetChangeEncoderFeedback(FB_ENCODER_SWITCH, encNo, currentBank == bank, encMData[encNo/4].moduleOrientation);   
//          SerialUSB.println(bankShifterPressed ? "BANK PRESSED" : "BANK RELEASED");
        return;
      }    
    }  
  }
}

void SetBankForAll(uint8_t newBank){
  encoderHw.SetBankForEncoders(newBank);
  //SetBankForDigitals(newBank);
}

void printPointer(void* pointer){
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
  return crc&0x7F;
}

void ResetFBMicro(){
  digitalWrite(pinResetSAMD11, LOW);
  delay(5);
  digitalWrite(pinResetSAMD11, HIGH);
  delay(5);
}

void ChangeBrigthnessISR(void){
  uint8_t powerAdapterConnected = !digitalRead(pinExternalVoltage);
  static int sumBright = 0;
  if(powerAdapterConnected){
    //SerialUSB.println("Power connected");
    feedbackHw.SendCommand(CHANGE_BRIGHTNESS);
    feedbackHw.SendCommand(BRIGHNESS_WITH_POWER);
    //SetStatusLED(STATUS_BLINK, 3, STATUS_FB_CONFIG);
  }else{
    
    feedbackHw.SendCommand(CHANGE_BRIGHTNESS);
//    sumBright += 10;
    feedbackHw.SendCommand(BRIGHNESS_WO_POWER);
//    feedbackHw.SendCommand(BRIGHNESS_WO_POWER+sumBright);
    //SerialUSB.println(BRIGHNESS_WO_POWER+sumBright);
    //SetStatusLED(STATUS_BLINK, 3, STATUS_FB_CONFIG);
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

void UpdateStatusLED() {
  
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
    else if (flagBlinkStatusLED == STATUS_ON){
      statusLED.setPixelColor(0, statusLEDColor[statusLEDfbType]); 
    }else if (flagBlinkStatusLED == STATUS_OFF){
      statusLED.setPixelColor(0, statusLEDColor[statusLEDtypes::STATUS_FB_NONE]); 
    }      
  }
  return;
}
