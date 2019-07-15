#include "headers/FeedbackClass.h"

//----------------------------------------------------------------------------------------------------
// FEEDBACK FUNCTIONS
//----------------------------------------------------------------------------------------------------
void FeedbackClass::Init(uint8_t maxBanks, uint8_t maxEncoders, uint8_t maxDigital, uint8_t maxIndependent) {
  nBanks = maxBanks;
  nEncoders = maxEncoders;
  nDigital = maxDigital;
  nIndependent = maxIndependent;

  feedbackUpdateFlag = NONE;

  // STATUS LED
  statusLED =  Adafruit_NeoPixel(N_STATUS_PIXEL, STATUS_LED_PIN, NEO_GRB + NEO_KHZ800);

  statusLED.begin();
  statusLED.setBrightness(90);

  flagBlinkStatusLED = 0;
  blinkCountStatusLED = 0;
  statusLEDfbType = 0;
  lastStatusLEDState = LOW;
  millisStatusPrev = 0;
  ringStateIndex = 0;

  // First dimension is an array of pointers, each pointing to a column - https://www.eskimo.com/~scs/cclass/int/sx9b.html
  allRingState = (uint16_t**) memHost->AllocateRAM(nBanks * sizeof(uint16_t*));
  allRingStatePrev = (uint16_t**) memHost->AllocateRAM(nBanks * sizeof(uint16_t*));

  for (int b = 0; b < nBanks; b++) {
    allRingState[b] = (uint16_t*) memHost->AllocateRAM(nEncoders * sizeof(uint16_t));
    allRingStatePrev[b] = (uint16_t*) memHost->AllocateRAM(nEncoders * sizeof(uint16_t));
    for (int e = 0; e < nEncoders; e++) {
      allRingState[b][e] = 0;
      allRingStatePrev[b][e] = 0;
    }
  }
  // Set all elements in arrays to 0
  //  for (int b = 0; b < nBanks; b++) {
  //
  //  }
}

                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                             void FeedbackClass::Update() {
  if (feedbackUpdateFlag) {
    switch (feedbackUpdateFlag) {
      case FB_ENCODER:
      case FB_ENCODER_SWITCH: {
          FillFrameWithEncoderData();
          AddCheckSum();
          SendFeedbackData();
        } break;
      case FB_DIGITAL: {

        } break;
      case FB_ANALOG: {

        } break;
      case FB_INDEPENDENT: {

        } break;
      case FB_BANK_CHANGED: {
          unsigned long antMicrosBank = micros();
          feedbackHw.SendCommand(CMD_ALL_LEDS_OFF);
          delay(3);
          updatingBankFeedback = true;
          for (int n = 0; n < nEncoders; n++) {
            SetChangeEncoderFeedback(FB_ENCODER, n, encoderHw.GetEncoderValue(n),
                                     encoderHw.GetModuleOrientation(n / 4));

            SetChangeEncoderFeedback(FB_ENCODER_SWITCH, n, encoderHw.GetEncoderSwitchValue(n),
                                     encoderHw.GetModuleOrientation(n / 4));
            //          SerialUSB.print("FB update type: ");SerialUSB.println(feedbackUpdateFlag);
            //          SerialUSB.print("Index: ");SerialUSB.println(indexChanged);
            //          SerialUSB.print("New Data: "); SerialUSB.println(newValue);
            //          SerialUSB.print("orientation: "); SerialUSB.println(orientation);
            //          SerialUSB.println();
            FillFrameWithEncoderData();
            //          SerialUSB.print("Resulting ring state: "); SerialUSB.println(allRingState[currentBank][n], BIN);
            //          SerialUSB.println("------------------------");
            AddCheckSum();
            SendFeedbackData();
            //          delay(3);
          }
          updatingBankFeedback = false;
          SerialUSB.print("F - ");
          SerialUSB.println(micros() - antMicrosBank);
        }
      default: break;
    }
    feedbackUpdateFlag = NONE;
  }


}

void FeedbackClass::FillFrameWithEncoderData() {
  if (feedbackUpdateFlag == FB_ENCODER) {
    switch (encoder[indexChanged].rotaryFeedback.mode) {
      //    switch (1) {
      case encoderRotaryFeedbackMode::fb_walk: {
          ringStateIndex = map(newValue, 0, MAX_ENC_VAL, 0, WALK_SIZE - 1);
          allRingState[currentBank][indexChanged] &= orientation ? ENCODER_SWITCH_V_ON : ENCODER_SWITCH_H_ON;
          allRingState[currentBank][indexChanged] |= walk[orientation][ringStateIndex];
        }
        break;
      case encoderRotaryFeedbackMode::fb_fill: {
          ringStateIndex = map(newValue, 0, MAX_ENC_VAL, 0, FILL_SIZE - 1);
          allRingState[currentBank][indexChanged] &= orientation ? ENCODER_SWITCH_V_ON : ENCODER_SWITCH_H_ON;
          allRingState[currentBank][indexChanged] |= fill[orientation][ringStateIndex];
        }
        break;
      case encoderRotaryFeedbackMode::fb_eq: {
          ringStateIndex = map(newValue, 0, MAX_ENC_VAL, 0, EQ_SIZE - 1);
          allRingState[currentBank][indexChanged] &= orientation ? ENCODER_SWITCH_V_ON : ENCODER_SWITCH_H_ON;
          allRingState[currentBank][indexChanged] |= eq[orientation][ringStateIndex];
        }
        break;
      case encoderRotaryFeedbackMode::fb_spread: {
          ringStateIndex = map(newValue, 0, MAX_ENC_VAL, 0, SPREAD_SIZE - 1);
          allRingState[currentBank][indexChanged] &= orientation ? ENCODER_SWITCH_V_ON : ENCODER_SWITCH_H_ON;
          allRingState[currentBank][indexChanged] |= spread[orientation][ringStateIndex];
        }
        break;
    }
  } else { // F
    if (newValue) allRingState[currentBank][indexChanged] |= (orientation ? ENCODER_SWITCH_V_ON : ENCODER_SWITCH_H_ON);
    else          allRingState[currentBank][indexChanged] &= ~(orientation ? ENCODER_SWITCH_V_ON : ENCODER_SWITCH_H_ON);
  }
  if ((allRingState[currentBank][indexChanged] != allRingStatePrev[currentBank][indexChanged]) || updatingBankFeedback) {
    allRingStatePrev[currentBank][indexChanged] = allRingState[currentBank][indexChanged];

    sendSerialBuffer[msgLength] = TX_BYTES;   // INIT SERIAL FRAME WITH CONSTANT DATA
    sendSerialBuffer[frameType] = ENCODER_CHANGE_FRAME;
    sendSerialBuffer[nRing] = indexChanged;
    sendSerialBuffer[ringStateH] = allRingState[currentBank][indexChanged] >> 8;
    sendSerialBuffer[ringStateL] = allRingState[currentBank][indexChanged] & 0xff;
    sendSerialBuffer[R] = encoder[indexChanged].rotaryFeedback.color[R - R];
    sendSerialBuffer[G] = encoder[indexChanged].rotaryFeedback.color[G - R];
    sendSerialBuffer[B] = encoder[indexChanged].rotaryFeedback.color[B - R];
    sendSerialBuffer[ENDOFFRAME] = 255;

  }
}


void FeedbackClass::SetChangeEncoderFeedback(uint8_t type, uint8_t encIndex, uint8_t val, uint8_t encoderOrientation) {
  feedbackUpdateFlag = type;
  indexChanged = encIndex;
  newValue = val;
  orientation = encoderOrientation;
}

void FeedbackClass::SetChangeDigitalFeedback(uint8_t digitalIndex, uint8_t val) {
  feedbackUpdateFlag = FB_DIGITAL;
  indexChanged = digitalIndex;
  newValue = val;
}

void FeedbackClass::SetChangeIndependentFeedback(uint8_t type, uint8_t fbIndex, uint8_t val) {
  feedbackUpdateFlag = type;
  indexChanged = fbIndex;
  newValue = val;
}

void FeedbackClass::SetBankChangeFeedback() {
  feedbackUpdateFlag = FB_BANK_CHANGED;
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

    if (flagBlinkStatusLED == STATUS_BLINK) {
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

    switch (statusLEDfbType) {
      case STATUS_FB_NONE: {
          blinkInterval = 0;
        } break;
      case STATUS_FB_CONFIG: {
          blinkInterval = STATUS_CONFIG_BLINK_INTERVAL;
        } break;
      case STATUS_FB_INPUT_CHANGED: {
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

void FeedbackClass::AddCheckSum() {
  uint16_t sum = 2019 - checkSum(sendSerialBuffer, B + 1);

  sendSerialBuffer[checkSum_MSB] = (sum >> 8) & 0xFF;
  sendSerialBuffer[checkSum_LSB] = sum & 0xff;
  //      sendSerialBuffer[CRC] = 127 - CRC8(sendSerialBuffer, B+1);
  //      SerialUSB.println(sendSerialBuffer[CRC], DEC);
}

void FeedbackClass::SendFeedbackData() {
    SerialUSB.println("******************************************");
    SerialUSB.println("Serial DATA: ");
  for (int i = msgLength; i <= ENDOFFRAME; i++) {
    Serial.write(sendSerialBuffer[i]);
        if(i == ringStateH || i == ringStateL)
          SerialUSB.println(sendSerialBuffer[i],BIN);
        else
          SerialUSB.println(sendSerialBuffer[i]);
  }
  Serial.flush();
    SerialUSB.println("******************************************");
}

void FeedbackClass::SendCommand(uint8_t cmd) {
  Serial.write(cmd);
  Serial.flush();
}
void FeedbackClass::SendResetToBootloader() {

  Serial.flush();
}
