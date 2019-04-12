//----------------------------------------------------------------------------------------------------
// FEEDBACK FUNCTIONS
//----------------------------------------------------------------------------------------------------
 
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                             void UpdateLeds(byte encNo, byte val){
   //Scale the counter value for referencing the LED sequence
    //***NOTE: Change the map() function to suit the limits of your rotary encoder application.
    //         The first two numbers are the lower, upper limit of the rotary encoder, the
    //         second two numbers 0 and 14 are limits of LED sequence arrays.  This is done
    //         so that the LEDs can use a different scaling than the encoder value.

    ringState[encNo] = map(val, 0, MAX_ENC_VAL, 0, 31);
//    ringState[encNo] = map(val, 0, MAX_ENC_VAL, 0, 15);
   
    if(ringState[encNo] != ringPrevState[encNo]){
      ringPrevState[encNo] = ringState[encNo];
       
      #define TX_BYTES 11
      
      uint8_t sendSerialBuffer[TX_BYTES] = {};
      sendSerialBuffer[msgLength] = TX_BYTES;
      sendSerialBuffer[nRing] = encNo;
      sendSerialBuffer[ringStateH] = walk[ringState[encNo]]>>8;
      sendSerialBuffer[ringStateL] = walk[ringState[encNo]]&0xff;
//      sendSerialBuffer[ringStateH] = fill[ringState[encNo]]>>8;
//      sendSerialBuffer[ringStateL] = fill[ringState[encNo]]&0xff;
      sendSerialBuffer[R] = rgbList[indexRgbList][0];
      sendSerialBuffer[G] = rgbList[indexRgbList][1];
      sendSerialBuffer[B] = rgbList[indexRgbList][2];
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
      
      
//      SerialUSB.println("Trama recibida: ");
//      for(int i = 0; i <= ringStateL; i++){
//        SerialUSB.println(spiAck[i],DEC);
//      }
//      SerialUSB.println("-----------------------------------------");
    }
 }
 /*
    Esta función es llamada por el loop principal, cuando las variables flagBlinkStatusLED y blinkCountStatusLED son distintas de cero.
    blinkCountStatusLED tiene la cantidad de veces que debe titilar el LED.
    El intervalo es fijo y dado por la etiqueta 'STATUS_BLINK_INTERVAL'
*/

void blinkStatusLED() {
  static bool lastLEDState = LOW;
  static unsigned long millisPrev = 0;
  static bool firstTime = true;

  if (firstTime) {
    firstTime = false;
    millisPrev = millis();
  }
  uint16_t blinkInterval = midiStatusLED ? STATUS_MIDI_BLINK_INTERVAL : STATUS_CONFIG_BLINK_INTERVAL;
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
      midiStatusLED = 0;
      firstTime = true;
    }
  }

  return;
}
