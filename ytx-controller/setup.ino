//----------------------------------------------------------------------------------------------------
// SETUP
//----------------------------------------------------------------------------------------------------

void setup() {

  SPI.begin();
  //SPI.setClockDivider(SPI_CLOCK_DIV);
 
  SerialUSB.begin(250000);  // TO PC
  Serial.begin(1000000); // FEEDBACK -> SAMD11

  pinMode(pinExternalVoltage, INPUT);
  pinMode(pinResetSAMD11, OUTPUT);
  digitalWrite(pinResetSAMD11, HIGH);

  analogReference(AR_EXTERNAL);
  analogReadResolution(12);
  
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
    config->banks.count = 4;
    config->inputs.analogCount = 8;
    config->inputs.encoderCount = 32;
    config->inputs.digitalCount = 32;
    
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
      initInputsConfig(b);
      memHost->SaveBank(b);
    }
    currentBank = memHost->LoadBank(0); 

    // AMOUNT OF DIGITAL MODULES
    
    for (int nPort = 0; nPort < DIGITAL_PORTS; nPort++){
      for (int nMod = 0; nMod < MODULES_PER_PORT; nMod++){
//        SerialUSB.println(config->hwMapping.digital[nPort][nMod]);
        if(config->hwMapping.digital[nPort][nMod]){
          modulesInConfig.digital++;
        }
      }
    }
//    SerialUSB.print("N DIGITAL MODS: ");SerialUSB.print(modulesInConfig.digital);
//    while(1);
    
    analogHw.Init(config->banks.count,            // N BANKS
                  config->inputs.analogCount);    // N INPUTS
    encoderHw.Init(config->banks.count,           // N BANKS
                   config->inputs.encoderCount,   // N INPUTS
                   &SPI);                         // SPI INTERFACE
    digitalHw.Init(config->banks.count,           // N BANKS
                   modulesInConfig.digital,       // N MODULES
                   config->inputs.digitalCount,   // N INPUTS
                   &SPI);                         // SPI INTERFACE
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
  
  // RESET SAMD11
  ResetFBMicro();
  
  // POWER MANAGEMENT - READ FROM POWER PIN, IF POWER SUPPLY IS PRESENT AND SET LED BRIGHTNESS ACCORDINGLY
  // SEND LED BRIGHTNESS AND INIT MESSAGE TO SAMD11
  feedbackHw.SendCommand(CMD_ALL_LEDS_OFF);
  delay(10);
  
  bool okToContinue = false;
  byte initFrameIndex = 0;
  byte initialBrightness = 20;
  byte initFrameArray[5] = {INIT_VALUES, 
                            config->inputs.encoderCount,
                            config->inputs.analogCount,
                            config->inputs.digitalCount,
                            initialBrightness};
                            
  do{
//    SerialUSB.println("INIT SAMD11");
//    SerialUSB.println(initFrameArray[initFrameIndex]);
    feedbackHw.SendCommand(initFrameArray[initFrameIndex++]); 
     
    if(initFrameIndex == 5) okToContinue = true;
//    if(Serial.available()){
//      SerialUSB.print("Index: ");SerialUSB.println(initFrameIndex);
//      byte ack = Serial.read();
//      SerialUSB.print("ACK: ");SerialUSB.println(ack);
//      if(ack == initFrameArray[initFrameIndex]){
//        if(initFrameIndex >= 4)  
//          okToContinue = true;
//        else
//          initFrameIndex++;
//      }
//      
//    }else{
//      SerialUSB.println("no serial data");
//      delay(3);
//    }
  }while(!okToContinue);

//  while(1) SerialUSB.println(digitalRead(pinExternalVoltage));
  
  SerialUSB.println(FreeMemory());
}

void initConfig(){
  
  config->hwMapping.encoder[0] = EncoderModuleTypes::E41H;
  config->hwMapping.encoder[1] = EncoderModuleTypes::E41H;
  config->hwMapping.encoder[2] = EncoderModuleTypes::E41H;
  config->hwMapping.encoder[3] = EncoderModuleTypes::E41H;
  config->hwMapping.encoder[4] = EncoderModuleTypes::E41H;
  config->hwMapping.encoder[5] = EncoderModuleTypes::E41H;
  config->hwMapping.encoder[6] = EncoderModuleTypes::E41H;
  config->hwMapping.encoder[7] = EncoderModuleTypes::E41H;

  config->hwMapping.digital[0][0] = DigitalModuleTypes::RB41; 
  config->hwMapping.digital[0][1] = DigitalModuleTypes::ARC41; 
  config->hwMapping.digital[0][2] = DigitalModuleTypes::RB42; 
  config->hwMapping.digital[0][3] = DigitalModuleTypes::RB41; 
  config->hwMapping.digital[0][4] = DigitalModuleTypes::RB41; 
  config->hwMapping.digital[0][5] = DigitalModuleTypes::RB41; 
  config->hwMapping.digital[0][6] = DigitalModuleTypes::RB41; 
  config->hwMapping.digital[0][7] = DigitalModuleTypes::RB41; 
  config->hwMapping.digital[1][0] = DigitalModuleTypes::DIGITAL_NONE; 
  config->hwMapping.digital[1][1] = DigitalModuleTypes::DIGITAL_NONE; 
  config->hwMapping.digital[1][2] = DigitalModuleTypes::DIGITAL_NONE; 
  config->hwMapping.digital[1][3] = DigitalModuleTypes::DIGITAL_NONE; 
  config->hwMapping.digital[1][4] = DigitalModuleTypes::DIGITAL_NONE; 
  config->hwMapping.digital[1][5] = DigitalModuleTypes::DIGITAL_NONE; 
  config->hwMapping.digital[1][6] = DigitalModuleTypes::DIGITAL_NONE; 
  config->hwMapping.digital[1][7] = DigitalModuleTypes::DIGITAL_NONE;
}

void initInputsConfig(uint8_t b){
  int i = 0;
  for (i = 0; i < config->inputs.digitalCount; i++){

  }
  for (i = 0; i < config->inputs.encoderCount; i++){
    encoder[i].mode.speed = i%4;
    encoder[i].rotaryConfig.message = i%(rotary_enc_pb+1);
    encoder[i].rotaryConfig.channel = b;
    encoder[i].rotaryConfig.midiPort = midiPortsType::midi_hw_usb;
//    SerialUSB.println(encoder[i].rotaryConfig.midiPort);
    encoder[i].rotaryConfig.parameter[rotary_LSB] = i;
    encoder[i].rotaryConfig.parameter[rotary_MSB] = 0;
    encoder[i].rotaryConfig.parameter[rotary_minLSB] = i*4;
    encoder[i].rotaryConfig.parameter[rotary_minMSB] = 0;
    encoder[i].rotaryConfig.parameter[rotary_maxLSB] = 127-i*4;
    encoder[i].rotaryConfig.parameter[rotary_maxMSB] = 0;

    encoder[i].switchConfig.action = (i%2)*switchActions::switch_toggle;
    
    encoder[i].rotaryFeedback.mode = i%4;
    encoder[i].rotaryFeedback.source = fb_src_local;
//    encoder[i].rotaryFeedback.color[R-R] = (i%10)*7+20;
//    encoder[i].rotaryFeedback.color[R-R] = (i*8)+20*(b+1);
//    encoder[i].rotaryFeedback.color[G-R] = (i*4)+40*b;
//    encoder[i].rotaryFeedback.color[B-R] = (i*2)+20;
    encoder[i].rotaryFeedback.color[R-R] = 127;
    encoder[i].rotaryFeedback.color[G-R] = 0;
    encoder[i].rotaryFeedback.color[B-R] = 127;
  }
  for (i = 0; i < config->inputs.analogCount; i++){
    
  }
}
