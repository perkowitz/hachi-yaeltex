//----------------------------------------------------------------------------------------------------
// SETUP
//----------------------------------------------------------------------------------------------------

void setup() {

  SPI.begin();
  SPI.setClockDivider(SPI_CLOCK_DIV2);
 
  SerialUSB.begin(250000);  // TO PC
  Serial.begin(1000000); // FEEDBACK -> SAMD11
  
  while(!SerialUSB);
  
  // EEPROM INITIALIZATION
    uint8_t eepStatus = eep.begin(extEEPROM::twiClock400kHz); //go fast!
  if (eepStatus) {
      SerialUSB.print("extEEPROM.begin() failed, status = ");SerialUSB.println(eepStatus);
      delay(1000);
      while (1);
  }
  //read fw signature from eeprom
//  if(signature == valid){        // SIGNATURE CHECK SUCCES
  if(true){        // SIGNATURE CHECK SUCCESS
    memHost = new memoryHost(&eep, ytxIOBLOCK::BLOCKS_COUNT);
    memHost->ConfigureBlock(ytxIOBLOCK::Configuration, 1, sizeof(ytxConfigurationType),true);
    config = (ytxConfigurationType*) memHost->Block(ytxIOBLOCK::Configuration);
    
    memHost->ConfigureBlock(ytxIOBLOCK::Button, config->inputs.digitalCount, sizeof(ytxDigitaltype),false);
    memHost->ConfigureBlock(ytxIOBLOCK::Encoder, config->inputs.encoderCount, sizeof(ytxEncoderType),false);
    memHost->ConfigureBlock(ytxIOBLOCK::Analog, config->inputs.analogCount, sizeof(ytxAnalogType),false);
    memHost->ConfigureBlock(ytxIOBLOCK::Feedback, config->inputs.feedbackCount, sizeof(ytxFeedbackType),false);
    memHost->LayoutBanks(); 
    
    digital = (ytxDigitaltype*) memHost->Block(ytxIOBLOCK::Button);
    encoder = (ytxEncoderType*) memHost->Block(ytxIOBLOCK::Encoder);
    analog = (ytxAnalogType*) memHost->Block(ytxIOBLOCK::Analog);
    feedback = (ytxFeedbackType*) memHost->Block(ytxIOBLOCK::Feedback);
    
    // SET NUMBER OF INPUTS OF EACH TYPE
    config->banks.count = 8;
    config->inputs.analogCount = 64;
    config->inputs.encoderCount = 8;
    config->inputs.digitalCount = 64;
    analogHw.Init(config->banks.count, config->inputs.analogCount);
//    analogHw.Init(8, 64);
    encoderHw.Init(config->banks.count, config->inputs.encoderCount, &SPI);
//    encoderHw.Init(8, 32, &SPI);
    digitalHw.Init(config->banks.count, 16, config->inputs.digitalCount, &SPI);
//    digitalHw.Init(8, 16, 64, &SPI);
    feedbackHw.Init(config->banks.count, config->inputs.encoderCount, config->inputs.digitalCount, 0);
//    feedbackHw.Init(8, 32, 64, 0);
    
    currentBank = memHost->LoadBank(0);  
  }else {           // SIGNATURE CHECK FAILED
    
  }
  
  
  MIDI.begin(MIDI_CHANNEL_OMNI); // Se inicializa la comunicación MIDI por USB.
  MIDI.setHandleSystemExclusive(handleSystemExclusive);
  MIDI.turnThruOff();            // Por default, la librería de Arduino MIDI tiene el THRU en ON, y NO QUEREMOS ESO!
  
  MIDIHW.begin(MIDI_CHANNEL_OMNI); // Se inicializa la comunicación MIDI por puerto serie(DIN5).
  MIDIHW.turnThruOff();            // Por default, la librería de Arduino MIDI tiene el THRU en ON, y NO QUEREMOS ESO!

  Keyboard.begin();

  // POWER MANAGEMENT - READ FROM POWER PIN, IF POWER SUPPLY IS PRESENT AND SET LED BRIGHTNESS ACCORDINGLY
  // SEND LED BRIGHTNESS AND INIT MESSAGE TO SAMD11
  feedbackHw.SendCommand(CMD_ALL_LEDS_OFF);
  
  antMicros = micros();

  SerialUSB.println(FreeMemory());

//  while(1){
////    Rainbow(&buttonLEDs1, 10);
//    for(int e = 0; e < 4; e++){
//      for(int i = 0; i<128; i++){
//        statusLED.setPixelColor(NUM_STATUS_LED, statusLED.Color(127,0,0)); // Moderately bright green color.
//        statusLED.show();
//        UpdateLeds(e, i);
//        delay(40);
//        statusLED.setPixelColor(NUM_STATUS_LED, statusLED.Color(0,0,0)); // Moderately bright green color.
//        statusLED.show();
//        delay(40);
//      }
//      UpdateLeds(e, 0);
//    }
//  }
}
