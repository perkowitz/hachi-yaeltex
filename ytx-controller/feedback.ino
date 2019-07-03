#include "headers/Feedback.h"

//----------------------------------------------------------------------------------------------------
// FEEDBACK FUNCTIONS
//----------------------------------------------------------------------------------------------------
void Feedback::Init(){
  statusLED.begin();
  statusLED.setBrightness(50);
}

                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                             void Feedback::Update(uint8_t type, uint8_t encNo, uint8_t val, uint8_t encoderOrientation){
   //Scale the counter value for referencing the LED sequence
    //***NOTE: Change the map() function to suit the limits of your rotary encoder application.
    //         The first two numbers are the lower, upper limit of the rotary encoder, the
    //         second two numbers 0 and 14 are limits of LED sequence arrays.  This is done
    //         so that the LEDs can use a different scaling than the encoder value.

  if(type == FB_ENCODER){
//    switch(encoder[encNo].rotaryFeedback.mode){
    switch(0){    
      case encoderRotaryFeedbackMode::fb_walk:{
        ringStateIndex = map(val, 0, MAX_ENC_VAL, 0, WALK_SIZE-1);
        allRingState[encNo] &= encoderOrientation ? ENCODER_SWITCH_V_ON : ENCODER_SWITCH_H_ON;
        allRingState[encNo] |= walk[encoderOrientation][ringStateIndex];
      }
      break;
      case encoderRotaryFeedbackMode::fb_fill:{
        ringStateIndex = map(val, 0, MAX_ENC_VAL, 0, FILL_SIZE-1);
        allRingState[encNo] &= encoderOrientation ? ENCODER_SWITCH_V_ON : ENCODER_SWITCH_H_ON;
        allRingState[encNo] |= fill[encoderOrientation][ringStateIndex];
      }
      break;
      case encoderRotaryFeedbackMode::fb_eq:{
        ringStateIndex = map(val, 0, MAX_ENC_VAL, 0, EQ_SIZE-1);
        allRingState[encNo] &= encoderOrientation ? ENCODER_SWITCH_V_ON : ENCODER_SWITCH_H_ON;
        allRingState[encNo] |= eq[encoderOrientation][ringStateIndex];
      }
      break;
      case encoderRotaryFeedbackMode::fb_spread:{
        ringStateIndex = map(val, 0, MAX_ENC_VAL, 0, SPREAD_SIZE-1);
        allRingState[encNo] &= encoderOrientation ? ENCODER_SWITCH_V_ON : ENCODER_SWITCH_H_ON;
        allRingState[encNo] |= spread[encoderOrientation][ringStateIndex];
      }
      break;
    }
  }else if(type == FB_ENCODER_SWITCH){
    if(val) allRingState[encNo] |= (encoderOrientation ? ENCODER_SWITCH_V_ON : ENCODER_SWITCH_H_ON);
    else    allRingState[encNo] &= ~(encoderOrientation ? ENCODER_SWITCH_V_ON : ENCODER_SWITCH_H_ON);
  }else if(type == FB_DIGITAL){
    // DIGITAL FEEDBACK
  }else if(type == FB_INDEPENDENT){
    // INDEPENDENT / ANALOG FEEDBACK
  }
  
 // AFTER FILLING THE RING STATE, SEND MESSAGE THROUGH USART
  if(allRingState[encNo] != allRingStatePrev[encNo]){
    allRingStatePrev[encNo] = allRingState[encNo];
     
    #define TX_BYTES 11
    
    uint8_t sendSerialBuffer[TX_BYTES] = {};
    sendSerialBuffer[msgLength] = TX_BYTES;
    sendSerialBuffer[nRing] = encNo;
    sendSerialBuffer[ringStateH] = allRingState[encNo]>>8;
    sendSerialBuffer[ringStateL] = allRingState[encNo]&0xff;      
    sendSerialBuffer[R] = rgbList[2][0];
    sendSerialBuffer[G] = rgbList[2][1];
    sendSerialBuffer[B] = rgbList[2][2];
    sendSerialBuffer[ENDOFFRAME] = 255;
    
    uint16_t sum = 2019 - checkSum(sendSerialBuffer, B+1);
    
    sendSerialBuffer[checkSum_MSB] = (sum>>8)&0xFF;
    sendSerialBuffer[checkSum_LSB] = sum&0xff;

//      sendSerialBuffer[CRC] = 127 - CRC8(sendSerialBuffer, B+1);
//      SerialUSB.println(sendSerialBuffer[CRC], DEC);
    
    #define ACK_BYTES 8
    byte spiAck[ACK_BYTES];

    for(int i = msgLength; i <= ENDOFFRAME; i++){
      Serial.write(sendSerialBuffer[i]);
    }
    Serial.flush();
    
//      SerialUSB.println("Trama enviada:");
//      for(int i = msgLength; i <= ENDOFFRAME; i++){
//        SerialUSB.print("BYTE "); SerialUSB.println(i,DEC);
//        SerialUSB.println(sendSerialBuffer[i],DEC);
//      }
//      SerialUSB.println("----------- Final Trama");
//    SerialUSB.println(sendSerialBuffer[CS_MSB],DEC);
//    SerialUSB.println(sendSerialBuffer[CS_LSB],DEC);
//    SerialUSB.println(sum,DEC);
//    SerialUSB.println(sendSerialBuffer[R],DEC);
//    SerialUSB.println(sendSerialBuffer[G],DEC);
//    SerialUSB.println(sendSerialBuffer[B],DEC);
//    SerialUSB.println();
//      int recvIndex = 0;
//      while(Serial.available()){
//        spiAck[recvIndex] = (char)Serial.read();
//        
//        SerialUSB.print("Byte "); SerialUSB.print(recvIndex); SerialUSB.print(": "); SerialUSB.println(spiAck[recvIndex++], DEC);
//      }
  }
 }
 /*
    Esta función es llamada por el loop principal, cuando las variables flagBlinkStatusLED y blinkCountStatusLED son distintas de cero.
    blinkCountStatusLED tiene la cantidad de veces que debe titilar el LED.
    El intervalo es fijo y dado por la etiqueta 'STATUS_BLINK_INTERVAL'
*/

void Feedback::UpdateStatusLED() {
  static bool lastLEDState = LOW;
  static unsigned long millisPrev = 0;
  static bool firstTime = true;

  if (flagBlinkStatusLED && blinkCountStatusLED){
    if (firstTime) {
      firstTime = false;
      millisPrev = millis();
    }
    uint16_t blinkInterval = statusLEDfbType ? STATUS_MIDI_BLINK_INTERVAL : STATUS_CONFIG_BLINK_INTERVAL;
    if (millis() - millisPrev > blinkInterval) {
      millisPrev = millis();
      lastLEDState = !lastLEDState;
      if(lastLEDState){
        statusLED.setPixelColor(0, statusLED.Color(64,0,0)); // Moderately bright green color.
      }else{
        statusLED.setPixelColor(0, statusLED.Color(0,0,0)); // Moderately bright green color.
        blinkCountStatusLED--;
      }
      statusLED.show(); // This sends the updated pixel color to the hardware.
  
      if (!blinkCountStatusLED) {
        flagBlinkStatusLED = 0;
        statusLEDfbType = 0;
        firstTime = true;
      }
    }
  }
  return;
}

void Feedback::SetStatusLED(uint8_t nTimes, bool status_type){
  if(!flagBlinkStatusLED){
    flagBlinkStatusLED = 1;
    statusLEDfbType = status_type;
    blinkCountStatusLED = nTimes;
  }
}
