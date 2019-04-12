//----------------------------------------------------------------------------------------------------
// SETUP
//----------------------------------------------------------------------------------------------------

void setup() {
  pinMode(slavePinAtmega, OUTPUT); // LED string latch
  digitalWrite(slavePinAtmega, HIGH);

  SPI.begin();
  SPI.setClockDivider(SPI_CLOCK_DIV4);
  pinMode(encodersMCPChipSelect, OUTPUT);
  pinMode(digitalMCPChipSelect1, OUTPUT);
  pinMode(digitalMCPChipSelect2, OUTPUT);

  SerialUSB.begin(250000);
  Serial.begin(250000);
  
  MIDI.begin(MIDI_CHANNEL_OMNI); // Se inicializa la comunicación MIDI.
  MIDI.turnThruOff();            // Por default, la librería de Arduino MIDI tiene el THRU en ON, y NO QUEREMOS ESO!
  MIDIHW.begin(MIDI_CHANNEL_OMNI); // Se inicializa la comunicación MIDI.
  MIDIHW.turnThruOff();            // Por default, la librería de Arduino MIDI tiene el THRU en ON, y NO QUEREMOS ESO!

//  sysEx.init();
  
  InitEncoders();
  InitDigital();
  
  // Analog setup
  KmBoard.init();                                    // Initialize Kilomux shield hardware

  statusLED.begin();
  statusLED.setBrightness(50);
  buttonLEDs1.begin();
  buttonLEDs1.setBrightness(50);
  buttonLEDs2.begin();
  buttonLEDs1.setBrightness(50);
  
  // Set all elements in arrays to 0
  for(int i = 0; i < NUM_ANALOG; i++){
     analogData[i] = 0;
     analogDataPrev[i] = 0;
     analogDirection[i] = 0;
  }
  
  antMicros = micros();
//  while(1){
//    statusLED.setPixelColor(NUM_STATUS_LED, statusLED.Color(0,0,0)); // Moderately bright green color.
//    statusLED.show();
//    delay(500);
//    statusLED.setPixelColor(NUM_STATUS_LED, statusLED.Color(127,0,0)); // Moderately bright green color.
//    statusLED.show();
//    delay(500);
//    statusLED.setPixelColor(NUM_STATUS_LED, statusLED.Color(0,127,0)); // Moderately bright green color.
//    statusLED.show();
//    delay(500);
//    statusLED.setPixelColor(NUM_STATUS_LED, statusLED.Color(0,0,127)); // Moderately bright green color.
//    statusLED.show();
//    delay(500);
//    statusLED.setPixelColor(NUM_STATUS_LED, statusLED.Color(0,127,127)); // Moderately bright green color.
//    statusLED.show();
//    delay(500);
//    statusLED.setPixelColor(NUM_STATUS_LED, statusLED.Color(127,127,0)); // Moderately bright green color.
//    statusLED.show();
//    delay(500);      
//    statusLED.setPixelColor(NUM_STATUS_LED, statusLED.Color(127,0,127)); // Moderately bright green color.
//    statusLED.show();
//    delay(500);
//  }
  
//  while(1){
//    Rainbow(&buttonLEDs1, 10);
////    for(int e = 0; e < 4; e++){
////      for(int i = 0; i<128; i+=4){
////        statusLED.setPixelColor(NUM_STATUS_LED, statusLED.Color(127,0,0)); // Moderately bright green color.
////        statusLED.show();
////        UpdateLeds(e, i);
////        delay(100);
////        statusLED.setPixelColor(NUM_STATUS_LED, statusLED.Color(0,0,0)); // Moderately bright green color.
////        statusLED.show();
////        delay(100);
////      }
////      UpdateLeds(e, 0);
////    }
//  }
}
