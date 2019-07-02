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
    
    memHost->ConfigureBlock(ytxIOBLOCK::Button, config->inputs.digitalsCount, sizeof(ytxDigitaltype),false);
    memHost->ConfigureBlock(ytxIOBLOCK::Encoder, config->inputs.encodersCount, sizeof(ytxEncoderType),false);
    memHost->ConfigureBlock(ytxIOBLOCK::Analog, config->inputs.analogsCount, sizeof(ytxAnalogType),false);
    memHost->ConfigureBlock(ytxIOBLOCK::Feedback, config->inputs.feedbacksCount, sizeof(ytxFeedbackType),false);
    memHost->LayoutBanks(); 
    
    digital = (ytxDigitaltype*) memHost->Block(ytxIOBLOCK::Button);
    encoder = (ytxEncoderType*) memHost->Block(ytxIOBLOCK::Encoder);
    analog = (ytxAnalogType*) memHost->Block(ytxIOBLOCK::Analog);
    feedback = (ytxFeedbackType*) memHost->Block(ytxIOBLOCK::Feedback);
    
    // SET NUMBER OF INPUTS OF EACH TYPE
    //analogHw.Init(config->inputs.analogsCount);
    analogHw.Init(4, 64);
    //encoderHw.Init(config->banks.count, config->inputs.encodersCount, &SPI);
    encoderHw.Init(4, 32, &SPI);
    //digitalHw.Init(config->banks.count, nModules, config->inputs.digitalCount, &SPI);
    digitalHw.Init(4, 16, 32, &SPI);
    
    currentBank = memHost->LoadBank(0);  
  }else {           // SIGNATURE CHECK FAILED
    
  }
  SerialUSB.println(FreeMemory());
  
  MIDI.begin(MIDI_CHANNEL_OMNI); // Se inicializa la comunicación MIDI por USB.
  MIDI.setHandleSystemExclusive(handleSystemExclusive);
  MIDI.turnThruOff();            // Por default, la librería de Arduino MIDI tiene el THRU en ON, y NO QUEREMOS ESO!
  
  MIDIHW.begin(MIDI_CHANNEL_OMNI); // Se inicializa la comunicación MIDI por puerto serie(DIN5).
  MIDIHW.turnThruOff();            // Por default, la librería de Arduino MIDI tiene el THRU en ON, y NO QUEREMOS ESO!

  Keyboard.begin();
//  sysEx.init();

//  KmBoard.init();                                    // Initialize Kilomux shield hardware

  // POWER MANAGEMENT - READ FROM POWER PIN, IF POWER SUPPLY IS PRESENT AND SET LED BRIGHTNESS ACCORDINGLY
  // SEND LED BRIGHTNESS AND INIT MESSAGE TO SAMD11
 
  statusLED.begin();
  statusLED.setBrightness(50);
    
  antMicros = micros();


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
