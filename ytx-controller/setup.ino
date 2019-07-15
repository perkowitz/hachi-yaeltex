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

    // SET NUMBER OF INPUTS OF EACH TYPE
    config->banks.count = 2;
    config->inputs.analogCount = 64;
    config->inputs.encoderCount = 32;
    config->inputs.digitalCount = 64;
    
    memHost->ConfigureBlock(ytxIOBLOCK::Button, config->inputs.digitalCount, sizeof(ytxDigitaltype),false);
    memHost->ConfigureBlock(ytxIOBLOCK::Encoder, config->inputs.encoderCount, sizeof(ytxEncoderType),false);
    memHost->ConfigureBlock(ytxIOBLOCK::Analog, config->inputs.analogCount, sizeof(ytxAnalogType),false);
    memHost->ConfigureBlock(ytxIOBLOCK::Feedback, config->inputs.feedbackCount, sizeof(ytxFeedbackType),false);
    memHost->LayoutBanks(); 
    
    digital = (ytxDigitaltype*) memHost->Block(ytxIOBLOCK::Button);
    encoder = (ytxEncoderType*) memHost->Block(ytxIOBLOCK::Encoder);
    analog = (ytxAnalogType*) memHost->Block(ytxIOBLOCK::Analog);
    feedback = (ytxFeedbackType*) memHost->Block(ytxIOBLOCK::Feedback);
    
     initConfig();
    for(int b = 0; b < config->banks.count; b++){ 
      initInputsConfig();
      memHost->SaveBank(b);
    }
    currentBank = memHost->LoadBank(0); 

    analogHw.Init(config->banks.count,            // N BANKS
                  config->inputs.analogCount);    // N INPUTS
    encoderHw.Init(config->banks.count,           // N BANKS
                   config->inputs.encoderCount,   // N INPUTS
                   &SPI);                         // SPI INTERFACE
    digitalHw.Init(config->banks.count,           // N BANKS
                   16,                            // N MODULES
                   config->inputs.digitalCount,   // N INPUTS
                   &SPI);                         // SPI INTERFACE
//    digitalHw.Init(8, 16, 64, &SPI);
    feedbackHw.Init(config->banks.count,          // N BANKS
                    config->inputs.encoderCount,  // N ENCODER INPUTS
                    config->inputs.digitalCount,  // N DIGITAL INPUTS
                    0);                           // N INDEPENDENT LEDs
  }else {           
    // SIGNATURE CHECK FAILED 
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

void initConfig(){
  
  config->hwMapping.encoder[0] = EncoderModuleTypes::E41H;
  config->hwMapping.encoder[1] = EncoderModuleTypes::E41V;
  config->hwMapping.encoder[2] = EncoderModuleTypes::E41H;
  config->hwMapping.encoder[3] = EncoderModuleTypes::E41H;
  config->hwMapping.encoder[4] = EncoderModuleTypes::E41H;
  config->hwMapping.encoder[5] = EncoderModuleTypes::E41H;
  config->hwMapping.encoder[6] = EncoderModuleTypes::E41H;
  config->hwMapping.encoder[7] = EncoderModuleTypes::E41H;

  config->hwMapping.digital[0][0] = DigitalModuleTypes::RB41; 
  config->hwMapping.digital[0][1] = DigitalModuleTypes::RB41; 
  config->hwMapping.digital[0][2] = DigitalModuleTypes::RB41; 
  config->hwMapping.digital[0][3] = DigitalModuleTypes::RB41; 
  config->hwMapping.digital[0][4] = DigitalModuleTypes::RB41; 
  config->hwMapping.digital[0][5] = DigitalModuleTypes::RB41; 
  config->hwMapping.digital[0][6] = DigitalModuleTypes::RB41; 
  config->hwMapping.digital[0][7] = DigitalModuleTypes::RB41; 
  config->hwMapping.digital[1][0] = DigitalModuleTypes::RB82; 
  config->hwMapping.digital[1][1] = DigitalModuleTypes::RB82; 
  config->hwMapping.digital[1][2] = DigitalModuleTypes::RB82; 
  config->hwMapping.digital[1][3] = DigitalModuleTypes::RB82; 
  config->hwMapping.digital[1][4] = DigitalModuleTypes::ARC41; 
  config->hwMapping.digital[1][5] = DigitalModuleTypes::ARC41; 
  config->hwMapping.digital[1][6] = DigitalModuleTypes::ARC41; 
  config->hwMapping.digital[1][7] = DigitalModuleTypes::ARC41;
}

void initInputsConfig(){
  int i = 0;
  for (i = 0; i < config->inputs.digitalCount; i++){

  }
  for (i = 0; i < config->inputs.encoderCount; i++){
    encoder[i].mode.speed = i%4;
    encoder[i].rotaryFeedback.mode = i%4;
    encoder[i].rotaryFeedback.color[R-R] = (i%10)*7+20;
    encoder[i].rotaryFeedback.color[G-R] = (i%2)*5+14;
    encoder[i].rotaryFeedback.color[B-R] = (i%4)*11;
  }
  for (i = 0; i < config->inputs.analogCount; i++){
    
  }
}
