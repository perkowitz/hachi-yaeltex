//----------------------------------------------------------------------------------------------------
// SETUP
//----------------------------------------------------------------------------------------------------

void setup() {

  SPI.begin();
  //SPI.setClockDivider(SPI_CLOCK_DIV);

  SerialUSB.begin(250000);  // TO PC
  Serial.begin(1000000); // FEEDBACK -> SAMD11

  // CAUSA DEL ULTIMO RESET
  //  SerialUSB.println(PM->RCAUSE.reg);

  pinMode(pinExternalVoltage, INPUT);
  pinMode(pinResetSAMD11, OUTPUT);
  digitalWrite(pinResetSAMD11, HIGH);

  // RESET SAMD11
  ResetFBMicro();

  while (!SerialUSB);

  // EEPROM INITIALIZATION
  uint8_t eepStatus = eep.begin(extEEPROM::twiClock400kHz); //go fast!
  if (eepStatus) {
    SerialUSB.print("extEEPROM.begin() failed, status = "); SerialUSB.println(eepStatus);
    delay(1000);
    while (1);
  }
  //read fw signature from eeprom
  //  if(signature == valid){        // SIGNATURE CHECK SUCCES
  if (true) {      // SIGNATURE CHECK SUCCESS
    memHost = new memoryHost(&eep, ytxIOBLOCK::BLOCKS_COUNT);
    memHost->ConfigureBlock(ytxIOBLOCK::Configuration, 1, sizeof(ytxConfigurationType), true);
    config = (ytxConfigurationType*) memHost->Block(ytxIOBLOCK::Configuration);

    // SET NUMBER OF INPUTS OF EACH TYPE
    config->banks.count = 1;          
    config->inputs.encoderCount = 28;
    config->inputs.analogCount = 32;
    config->inputs.digitalCount = 24;

    memHost->ConfigureBlock(ytxIOBLOCK::Button, config->inputs.digitalCount, sizeof(ytxDigitaltype), false);
    memHost->ConfigureBlock(ytxIOBLOCK::Encoder, config->inputs.encoderCount, sizeof(ytxEncoderType), false);
    memHost->ConfigureBlock(ytxIOBLOCK::Analog, config->inputs.analogCount, sizeof(ytxAnalogType), false);
    memHost->ConfigureBlock(ytxIOBLOCK::Feedback, config->inputs.feedbackCount, sizeof(ytxFeedbackType), false);
    memHost->LayoutBanks();

    digital = (ytxDigitaltype*) memHost->Block(ytxIOBLOCK::Button);
    encoder = (ytxEncoderType*) memHost->Block(ytxIOBLOCK::Encoder);
    analog = (ytxAnalogType*) memHost->Block(ytxIOBLOCK::Analog);
    feedback = (ytxFeedbackType*) memHost->Block(ytxIOBLOCK::Feedback);

    initConfig();
    for (int b = 0; b < config->banks.count; b++) {
      initInputsConfig(b);
      memHost->SaveBank(b);
    }
    currentBank = memHost->LoadBank(0);

    encoderHw.Init(config->banks.count,           // N BANKS
                   config->inputs.encoderCount,   // N INPUTS
                   &SPI);                         // SPI INTERFACE
    analogHw.Init(config->banks.count,            // N BANKS
                  config->inputs.analogCount);    // N INPUTS
    digitalHw.Init(config->banks.count,           // N BANKS
                   config->inputs.digitalCount,   // N INPUTS
                   &SPI);                         // SPI  INTERFACE
    feedbackHw.Init(config->banks.count,          // N BANKS
                    config->inputs.encoderCount,  // N ENCODER INPUTS
                    config->inputs.digitalCount,  // N DIGITAL INPUTS
                    0);                           // N INDEPENDENT LEDs
  } else {
    // SIGNATURE CHECK FAILED
  }

  MIDI.begin(MIDI_CHANNEL_OMNI); // Se inicializa la comunicación MIDI por USB.
  MIDI.setHandleSystemExclusive(handleSystemExclusive);
  MIDI.turnThruOff();            // Por default, la librería de Arduino MIDI tiene el THRU en ON, y NO QUEREMOS ESO!

  MIDIHW.begin(MIDI_CHANNEL_OMNI); // Se inicializa la comunicación MIDI por puerto serie(DIN5).
  MIDIHW.turnThruOff();            // Por default, la librería de Arduino MIDI tiene el THRU en ON, y NO QUEREMOS ESO!

  Keyboard.begin();

  //  delay(20);
  // Initialize brigthness and power configuration
  feedbackHw.InitPower();
  feedbackHw.SetBankChangeFeedback();
  //  SerialUSB.print("Free RAM: "); SerialUSB.println(FreeMemory());

  // STATUS LED
  statusLED =  Adafruit_NeoPixel(N_STATUS_PIXEL, STATUS_LED_PIN, NEO_GRB + NEO_KHZ800);

  statusLED.begin();
  statusLED.setBrightness(STATUS_LED_BRIGHTNESS);
  
//  while(1){
//    analogHw.AnalogReadFast(A4);
//    delay(100);
//  }
}

void initConfig() {

  config->hwMapping.encoder[0] = EncoderModuleTypes::E41H;
  config->hwMapping.encoder[1] = EncoderModuleTypes::E41H;
  config->hwMapping.encoder[2] = EncoderModuleTypes::E41H;
  config->hwMapping.encoder[3] = EncoderModuleTypes::E41H;
  config->hwMapping.encoder[4] = EncoderModuleTypes::E41H;
  config->hwMapping.encoder[5] = EncoderModuleTypes::E41H;
  config->hwMapping.encoder[6] = EncoderModuleTypes::E41H;
  config->hwMapping.encoder[7] = EncoderModuleTypes::ENCODER_NONE;

  config->hwMapping.digital[0][0] = DigitalModuleTypes::RB41;
  config->hwMapping.digital[0][1] = DigitalModuleTypes::RB41;
  config->hwMapping.digital[0][2] = DigitalModuleTypes::RB42;
  config->hwMapping.digital[0][3] = DigitalModuleTypes::RB42;
  config->hwMapping.digital[0][4] = DigitalModuleTypes::DIGITAL_NONE;
  config->hwMapping.digital[0][5] = DigitalModuleTypes::DIGITAL_NONE;
  config->hwMapping.digital[0][6] = DigitalModuleTypes::DIGITAL_NONE;
  config->hwMapping.digital[0][7] = DigitalModuleTypes::DIGITAL_NONE;
//
//  config->hwMapping.digital[0][0] = DigitalModuleTypes::RB41;
//  config->hwMapping.digital[0][1] = DigitalModuleTypes::RB42;
//  config->hwMapping.digital[0][2] = DigitalModuleTypes::DIGITAL_NONE;
//  config->hwMapping.digital[0][3] = DigitalModuleTypes::DIGITAL_NONE;
//  config->hwMapping.digital[0][4] = DigitalModuleTypes::DIGITAL_NONE;
//  config->hwMapping.digital[0][5] = DigitalModuleTypes::DIGITAL_NONE;
//  config->hwMapping.digital[0][6] = DigitalModuleTypes::DIGITAL_NONE;
//  config->hwMapping.digital[0][7] = DigitalModuleTypes::DIGITAL_NONE;
  config->hwMapping.digital[1][0] = DigitalModuleTypes::DIGITAL_NONE;
  config->hwMapping.digital[1][1] = DigitalModuleTypes::DIGITAL_NONE;
  config->hwMapping.digital[1][2] = DigitalModuleTypes::DIGITAL_NONE;
  config->hwMapping.digital[1][3] = DigitalModuleTypes::DIGITAL_NONE;
  config->hwMapping.digital[1][4] = DigitalModuleTypes::DIGITAL_NONE;
  config->hwMapping.digital[1][5] = DigitalModuleTypes::DIGITAL_NONE;
  config->hwMapping.digital[1][6] = DigitalModuleTypes::DIGITAL_NONE;
  config->hwMapping.digital[1][7] = DigitalModuleTypes::DIGITAL_NONE;
  //  config->hwMapping.digital[1][0] = DigitalModuleTypes::RB41;
  //  config->hwMapping.digital[1][1] = DigitalModuleTypes::RB42;
  //  config->hwMapping.digital[1][2] = DigitalModuleTypes::RB41;
  //  config->hwMapping.digital[1][3] = DigitalModuleTypes::RB41;
  //  config->hwMapping.digital[1][4] = DigitalModuleTypes::RB42;
  //  config->hwMapping.digital[1][5] = DigitalModuleTypes::RB41;
  //  config->hwMapping.digital[1][6] = DigitalModuleTypes::RB41;
  //  config->hwMapping.digital[1][7] = DigitalModuleTypes::RB42;

  config->hwMapping.analog[0][0] = AnalogModuleTypes::P41;
  config->hwMapping.analog[0][1] = AnalogModuleTypes::ANALOG_NONE;
  config->hwMapping.analog[0][2] = AnalogModuleTypes::JAL;
  config->hwMapping.analog[0][3] = AnalogModuleTypes::JAF;
  config->hwMapping.analog[0][4] = AnalogModuleTypes::F41;
  config->hwMapping.analog[0][5] = AnalogModuleTypes::ANALOG_NONE;
  config->hwMapping.analog[0][6] = AnalogModuleTypes::P41;
  config->hwMapping.analog[0][7] = AnalogModuleTypes::ANALOG_NONE;
  config->hwMapping.analog[1][0] = AnalogModuleTypes::JAL;
  config->hwMapping.analog[1][1] = AnalogModuleTypes::JAF;
  config->hwMapping.analog[1][2] = AnalogModuleTypes::F41;
  config->hwMapping.analog[1][3] = AnalogModuleTypes::ANALOG_NONE;
  config->hwMapping.analog[1][4] = AnalogModuleTypes::F41;
  config->hwMapping.analog[1][5] = AnalogModuleTypes::ANALOG_NONE;
  config->hwMapping.analog[1][6] = AnalogModuleTypes::P41;
  config->hwMapping.analog[1][7] = AnalogModuleTypes::ANALOG_NONE;
  config->hwMapping.analog[2][0] = AnalogModuleTypes::ANALOG_NONE;
  config->hwMapping.analog[2][1] = AnalogModuleTypes::ANALOG_NONE;
  config->hwMapping.analog[2][2] = AnalogModuleTypes::ANALOG_NONE;
  config->hwMapping.analog[2][3] = AnalogModuleTypes::ANALOG_NONE;
  config->hwMapping.analog[2][4] = AnalogModuleTypes::ANALOG_NONE;
  config->hwMapping.analog[2][5] = AnalogModuleTypes::ANALOG_NONE;
  config->hwMapping.analog[2][6] = AnalogModuleTypes::ANALOG_NONE;
  config->hwMapping.analog[2][7] = AnalogModuleTypes::ANALOG_NONE;
  config->hwMapping.analog[3][0] = AnalogModuleTypes::ANALOG_NONE;
  config->hwMapping.analog[3][1] = AnalogModuleTypes::ANALOG_NONE;
  config->hwMapping.analog[3][2] = AnalogModuleTypes::ANALOG_NONE;
  config->hwMapping.analog[3][3] = AnalogModuleTypes::ANALOG_NONE;
  config->hwMapping.analog[3][4] = AnalogModuleTypes::ANALOG_NONE;
  config->hwMapping.analog[3][5] = AnalogModuleTypes::ANALOG_NONE;
  config->hwMapping.analog[3][6] = AnalogModuleTypes::ANALOG_NONE;
  config->hwMapping.analog[3][7] = AnalogModuleTypes::ANALOG_NONE;
  
//  config->hwMapping.analog[0][1] = AnalogModuleTypes::ANALOG_NONE;
//  config->hwMapping.analog[0][2] = AnalogModuleTypes::ANALOG_NONE;
//  config->hwMapping.analog[0][3] = AnalogModuleTypes::ANALOG_NONE;
//  config->hwMapping.analog[0][4] = AnalogModuleTypes::ANALOG_NONE;
//  config->hwMapping.analog[0][5] = AnalogModuleTypes::ANALOG_NONE;
//  config->hwMapping.analog[0][6] = AnalogModuleTypes::ANALOG_NONE;
//  config->hwMapping.analog[0][7] = AnalogModuleTypes::ANALOG_NONE;
//  config->hwMapping.analog[1][0] = AnalogModuleTypes::ANALOG_NONE;
//  config->hwMapping.analog[1][1] = AnalogModuleTypes::ANALOG_NONE;
//  config->hwMapping.analog[1][2] = AnalogModuleTypes::ANALOG_NONE;
//  config->hwMapping.analog[1][3] = AnalogModuleTypes::ANALOG_NONE;
//  config->hwMapping.analog[1][4] = AnalogModuleTypes::ANALOG_NONE;
//  config->hwMapping.analog[1][5] = AnalogModuleTypes::ANALOG_NONE;
//  config->hwMapping.analog[1][6] = AnalogModuleTypes::ANALOG_NONE;
//  config->hwMapping.analog[1][7] = AnalogModuleTypes::ANALOG_NONE;
//  config->hwMapping.analog[2][0] = AnalogModuleTypes::ANALOG_NONE;
//  config->hwMapping.analog[2][1] = AnalogModuleTypes::ANALOG_NONE;
//  config->hwMapping.analog[2][2] = AnalogModuleTypes::ANALOG_NONE;
//  config->hwMapping.analog[2][3] = AnalogModuleTypes::ANALOG_NONE;
//  config->hwMapping.analog[2][4] = AnalogModuleTypes::ANALOG_NONE;
//  config->hwMapping.analog[2][5] = AnalogModuleTypes::ANALOG_NONE;
//  config->hwMapping.analog[2][6] = AnalogModuleTypes::ANALOG_NONE;
//  config->hwMapping.analog[2][7] = AnalogModuleTypes::ANALOG_NONE;
//  config->hwMapping.analog[3][0] = AnalogModuleTypes::ANALOG_NONE;
//  config->hwMapping.analog[3][1] = AnalogModuleTypes::ANALOG_NONE;
//  config->hwMapping.analog[3][2] = AnalogModuleTypes::ANALOG_NONE;
//  config->hwMapping.analog[3][3] = AnalogModuleTypes::ANALOG_NONE;
//  config->hwMapping.analog[3][4] = AnalogModuleTypes::ANALOG_NONE;
//  config->hwMapping.analog[3][5] = AnalogModuleTypes::ANALOG_NONE;
//  config->hwMapping.analog[3][6] = AnalogModuleTypes::ANALOG_NONE;
//  config->hwMapping.analog[3][7] = AnalogModuleTypes::ANALOG_NONE;
}

void initInputsConfig(uint8_t b) {
  int i = 0;
  for (i = 0; i < config->inputs.digitalCount; i++) {
    digital[i].actionConfig.action = (i % 2) * switchActions::switch_toggle;
    digital[i].actionConfig.message = (i) % (digital_rpn + 1) + 1;

    digital[i].actionConfig.channel = b;
    digital[i].actionConfig.midiPort = midiPortsType::midi_hw_usb;
    //    SerialUSB.println(digital[i].actionConfig.midiPort);
    //    if(!i%digital_ks){
    //      digital[i].actionConfig.parameter[digital_key] = '+';
    //      digital[i].actionConfig.parameter[digital_modifier] = KEY_LEFT_CTRL;
    //    }else{
    digital[i].actionConfig.parameter[digital_LSB] = i;
    digital[i].actionConfig.parameter[digital_MSB] = 0;
    uint16_t minVal = 0;
    uint16_t maxVal = 127;
    digital[i].actionConfig.parameter[digital_minLSB] = minVal & 0xFF;
    digital[i].actionConfig.parameter[digital_minMSB] = (minVal >> 7) & 0xFF;
    digital[i].actionConfig.parameter[digital_maxLSB] = maxVal & 0xFF;
    digital[i].actionConfig.parameter[digital_maxMSB] = (maxVal >> 7) & 0xFF;
    //    }
    digital[i].feedback.colorRangeEnable = false;
    digital[i].feedback.source = fb_src_local;
//    SerialUSB.print(i); SerialUSB.print(": ");SerialUSB.println(digital[i].feedback.source);
    digital[i].feedback.color[R_INDEX] = 127;
    digital[i].feedback.color[G_INDEX] = 0;
    digital[i].feedback.color[B_INDEX] = 127;
  }

  for (i = 0; i < config->inputs.encoderCount; i++) {
    encoder[i].mode.speed = i % 4;
//    encoder[i].rotaryConfig.message = (i) % (rotary_enc_rpn + 1) + 1;
    encoder[i].rotaryConfig.message = rotary_enc_cc;
    encoder[i].rotaryConfig.channel = b;
    encoder[i].rotaryConfig.midiPort = midiPortsType::midi_hw_usb;
    //    SerialUSB.println(encoder[i].rotaryConfig.midiPort);
    encoder[i].rotaryConfig.parameter[rotary_LSB] = i;
    encoder[i].rotaryConfig.parameter[rotary_MSB] = 0;
    encoder[i].rotaryConfig.parameter[rotary_minLSB] = 0;
    encoder[i].rotaryConfig.parameter[rotary_minMSB] = 0;
    encoder[i].rotaryConfig.parameter[rotary_maxLSB] = 127;
    encoder[i].rotaryConfig.parameter[rotary_maxMSB] = 0;

//    encoder[i].switchConfig.message = (i) % (digital_rpn + 1) + 1;
    encoder[i].switchConfig.message = digital_cc;
    encoder[i].switchConfig.action = (i % 2) * switchActions::switch_toggle;
    
//    encoder[i].rotaryFeedback.mode = i % 4;
    encoder[i].rotaryFeedback.mode = 0;
    encoder[i].rotaryFeedback.source = fb_src_local;
    //    encoder[i].rotaryFeedback.color[R-R] = (i%10)*7+20;
    //    encoder[i].rotaryFeedback.color[R-R] = (i*8)+20*(b+1);
    //    encoder[i].rotaryFeedback.color[G-R] = (i*4)+40*b;
    //    encoder[i].rotaryFeedback.color[B-R] = (i*2)+20;
    encoder[i].rotaryFeedback.color[R_INDEX] = 127;
    encoder[i].rotaryFeedback.color[G_INDEX] = 0;
    encoder[i].rotaryFeedback.color[B_INDEX] = 127;
  }
  for (i = 0; i < config->inputs.analogCount; i++) {
//    if (i < 16) analog[i].message = i % (analog_rpn + 1) + 1;
    if (i < 16) analog[i].message = analog_cc;
    if (i >= 16 && i < 32) analog[i].message = analogMessageTypes::analog_none;
    if (i >= 32 && i < 48) analog[i].message = analogMessageTypes::analog_none;
    if (i >= 48 && i < 64) analog[i].message = analogMessageTypes::analog_none;
//    if (i >= 48 && i < 64) analog[i].message = i % (analog_rpn + 1) + 1;

    analog[i].channel = b;
    analog[i].midiPort = midiPortsType::midi_usb;
    //    SerialUSB.println(encoder[i].rotaryConfig.midiPort);
    analog[i].parameter[rotary_LSB] = i;
    analog[i].parameter[rotary_MSB] = 0;
    analog[i].parameter[rotary_minLSB] = 0;
    analog[i].parameter[rotary_minMSB] = 0;
    analog[i].parameter[rotary_maxLSB] = 127;
    analog[i].parameter[rotary_maxMSB] = ((analog[i].message == analog_nrpn) || 
                                          (analog[i].message == analog_rpn) || 
                                          (analog[i].message == analog_pb)) ? 127 : 0;
  }
}
