//----------------------------------------------------------------------------------------------------
// SETUP
//----------------------------------------------------------------------------------------------------

void setup() {

  SPI.begin();
  SPI.setClockDivider(SPI_CLOCK_DIV4);
  pinMode(encodersMCPChipSelect, OUTPUT);
  pinMode(digitalMCPChipSelect1, OUTPUT);
  pinMode(digitalMCPChipSelect2, OUTPUT);

  SerialUSB.begin(250000);  // TO PC
  Serial.begin(250000); // FEEDBACK -> SAMD11
  
  while(!SerialUSB);
  
  // EEPROM INITIALIZATION
    uint8_t eepStatus = eep.begin(extEEPROM::twiClock400kHz); //go fast!
  if (eepStatus) {
      SerialUSB.print("extEEPROM.begin() failed, status = ");SerialUSB.println(eepStatus);
      delay(1000);
      while (1);
  }
  
  if(true){        // SIGNATURE CHECK SUCCESS
    memHost = new memoryHost(&eep, ytxIOBLOCK::BLOCKS_COUNT);
    memHost->configureBlock(ytxIOBLOCK::Configuration, 1, sizeof(ytxConfigurationType),true);
    config = (ytxConfigurationType*) memHost->block(ytxIOBLOCK::Configuration);
    
    memHost->configureBlock(ytxIOBLOCK::Button, config->inputs.digitalsCount, sizeof(ytxDigitaltype),false);
    memHost->configureBlock(ytxIOBLOCK::Encoder, config->inputs.encodersCount, sizeof(ytxEncoderType),false);
    memHost->configureBlock(ytxIOBLOCK::Analog, config->inputs.analogsCount, sizeof(ytxAnalogType),false);
    memHost->configureBlock(ytxIOBLOCK::Feedback, config->inputs.feedbacksCount, sizeof(ytxFeedbackType),false);
    memHost->layoutBanks(); 
    
    digital = (ytxDigitaltype*) memHost->block(ytxIOBLOCK::Button);
    encoder = (ytxEncoderType*) memHost->block(ytxIOBLOCK::Encoder);
    analog = (ytxAnalogType*) memHost->block(ytxIOBLOCK::Analog);
    feedback = (ytxFeedbackType*) memHost->block(ytxIOBLOCK::Feedback);
    
    // SET NUMBER OF INPUTS OF EACH TYPE
    SerialUSB.println(config->inputs.analogsCount);
    SerialUSB.println();
    analogHw.SetAnalogQty(config->inputs.analogsCount);

    memHost->loadBank(0);  
  }else {           // SIGNATURE CHECK FAILED
    
  }

  
  
  
  MIDI.begin(MIDI_CHANNEL_OMNI); // Se inicializa la comunicación MIDI por USB.
  MIDI.setHandleSystemExclusive(handleSystemExclusive);
  MIDI.turnThruOff();            // Por default, la librería de Arduino MIDI tiene el THRU en ON, y NO QUEREMOS ESO!
  
  MIDIHW.begin(MIDI_CHANNEL_OMNI); // Se inicializa la comunicación MIDI por puerto serie(DIN5).
  MIDIHW.turnThruOff();            // Por default, la librería de Arduino MIDI tiene el THRU en ON, y NO QUEREMOS ESO!

//  sysEx.init();
  
  InitEncoders();
  InitDigital();
  
  // Analog setup
  KmBoard.init();                                    // Initialize Kilomux shield hardware

  statusLED.begin();
  statusLED.setBrightness(50);
//  buttonLEDs1.begin();
//  buttonLEDs1.setBrightness(50);
//  buttonLEDs2.begin();
//  buttonLEDs1.setBrightness(50);
//  
  
  
  
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
