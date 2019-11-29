//----------------------------------------------------------------------------------------------------
// SETUP
//----------------------------------------------------------------------------------------------------

void setup() {

  SPI.begin();
  //SPI.setClockDivider(SPI_CLOCK_DIV);

  SerialUSB.begin(250000);  // TO PC
  Serial.begin(1000000); // FEEDBACK -> SAMD11

  // CAUSA DEL ULTIMO RESET
  SerialUSB.println(PM->RCAUSE.reg);

  pinMode(pinExternalVoltage, INPUT);
  pinMode(pinResetSAMD11, OUTPUT);
  digitalWrite(pinResetSAMD11, HIGH);

  // RESET SAMD11
  ResetFBMicro();

//  while (!SerialUSB);
  
  delay(50); // wait for samd11 reset
  
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
    config->inputs.encoderCount = 32;
    config->inputs.analogCount = 0;
    config->inputs.digitalCount = 32;
    config->inputs.feedbackCount = 0;
    
    memHost->ConfigureBlock(ytxIOBLOCK::Encoder, config->inputs.encoderCount, sizeof(ytxEncoderType), false);
    memHost->ConfigureBlock(ytxIOBLOCK::Analog, config->inputs.analogCount, sizeof(ytxAnalogType), false);
    memHost->ConfigureBlock(ytxIOBLOCK::Digital, config->inputs.digitalCount, sizeof(ytxDigitaltype), false);
    memHost->ConfigureBlock(ytxIOBLOCK::Feedback, config->inputs.feedbackCount, sizeof(ytxFeedbackType), false);
    memHost->LayoutBanks();
    
    encoder = (ytxEncoderType*) memHost->Block(ytxIOBLOCK::Encoder);
    analog = (ytxAnalogType*) memHost->Block(ytxIOBLOCK::Analog);
    digital = (ytxDigitaltype*) memHost->Block(ytxIOBLOCK::Digital);
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

  MIDI.setHandleNoteOn(handleNoteOnUSB);
  MIDIHW.setHandleNoteOn(handleNoteOnHW);
  MIDI.setHandleNoteOff(handleNoteOffUSB);
  MIDIHW.setHandleNoteOff(handleNoteOffHW);
  MIDI.setHandleControlChange(handleControlChangeUSB);
  MIDIHW.setHandleControlChange(handleControlChangeHW);
  MIDI.setHandlePitchBend(handlePitchBendUSB);
  MIDIHW.setHandlePitchBend(handlePitchBendHW);
  
  Keyboard.begin();
  
//  SerialUSB.println(FreeMemory());
//  SerialUSB.println(sizeof(midiMsgBuffer));
//
//  while(1);
  
  //  delay(20);
  // Initialize brigthness and power configuration
  feedbackHw.InitPower();
  while(!(Serial.read() == 24));
  
  feedbackHw.SetBankChangeFeedback();
  
  //  SerialUSB.print("Free RAM: "); SerialUSB.println(FreeMemory());
//  SerialUSB.println("Hola");
  // STATUS LED
  statusLED =  Adafruit_NeoPixel(N_STATUS_PIXEL, STATUS_LED_PIN, NEO_GRB + NEO_KHZ800);

  statusLED.begin();
  statusLED.setBrightness(STATUS_LED_BRIGHTNESS);

}

void initConfig() {

  config->midiMergeFlags = 0x00;

  for (int bank = 0; bank < config->banks.count; bank++) {
    config->banks.shifterId[bank] = bank+32;
    config->banks.momToggFlags = 0b0000000000000011;
    
//    config->banks.shifterId[bank] = bank;
    SerialUSB.print("BANK SHIFTER "); SerialUSB.print(bank); SerialUSB.print(": "); SerialUSB.println(config->banks.shifterId[bank]); 
    SerialUSB.println((config->banks.momToggFlags>>bank)&1 ? "TOGGLE" : "MOMENTARY");
  }

//  for(int i = 15; i>=0; i--){
//    SerialUSB.print(((config->banks.momToggFlags)>>i)&1,BIN);
//  }
//  SerialUSB.println();
  
  config->hwMapping.encoder[0] = EncoderModuleTypes::E41H_D;
  config->hwMapping.encoder[1] = EncoderModuleTypes::E41H_D;
  config->hwMapping.encoder[2] = EncoderModuleTypes::E41H_D;
  config->hwMapping.encoder[3] = EncoderModuleTypes::E41H_D;
  config->hwMapping.encoder[4] = EncoderModuleTypes::E41H_D;
  config->hwMapping.encoder[5] = EncoderModuleTypes::E41H_D;
  config->hwMapping.encoder[6] = EncoderModuleTypes::E41H_D;
  config->hwMapping.encoder[7] = EncoderModuleTypes::E41H_D;

  config->hwMapping.digital[0][0] = DigitalModuleTypes::RB82;
  config->hwMapping.digital[0][1] = DigitalModuleTypes::RB42;
  config->hwMapping.digital[0][2] = DigitalModuleTypes::RB42;
//  config->hwMapping.digital[0][3] = DigitalModuleTypes::RB42;
//  config->hwMapping.digital[0][4] = DigitalModuleTypes::RB42;
//  config->hwMapping.digital[0][5] = DigitalModuleTypes::RB42;
//  config->hwMapping.digital[0][6] = DigitalModuleTypes::RB41;
//  config->hwMapping.digital[0][7] = DigitalModuleTypes::RB41;
//  config->hwMapping.digital[1][0] = DigitalModuleTypes::RB41;
//  config->hwMapping.digital[1][1] = DigitalModuleTypes::RB41;
//  config->hwMapping.digital[1][2] = DigitalModuleTypes::RB41;
//  config->hwMapping.digital[1][3] = DigitalModuleTypes::RB82;
//  config->hwMapping.digital[1][4] = DigitalModuleTypes::RB82;
//  config->hwMapping.digital[1][5] = DigitalModuleTypes::RB82;
//  config->hwMapping.digital[1][6] = DigitalModuleTypes::RB82;
//  config->hwMapping.digital[1][7] = DigitalModuleTypes::RB82;
//  config->hwMapping.digital[0][1] = DigitalModuleTypes::DIGITAL_NONE;
//  config->hwMapping.digital[0][2] = DigitalModuleTypes::DIGITAL_NONE;
  config->hwMapping.digital[0][3] = DigitalModuleTypes::DIGITAL_NONE;
  config->hwMapping.digital[0][4] = DigitalModuleTypes::DIGITAL_NONE;
  config->hwMapping.digital[0][5] = DigitalModuleTypes::DIGITAL_NONE;
  config->hwMapping.digital[0][6] = DigitalModuleTypes::DIGITAL_NONE;
  config->hwMapping.digital[0][7] = DigitalModuleTypes::DIGITAL_NONE;
  config->hwMapping.digital[1][0] = DigitalModuleTypes::DIGITAL_NONE;
  config->hwMapping.digital[1][1] = DigitalModuleTypes::DIGITAL_NONE;
  config->hwMapping.digital[1][2] = DigitalModuleTypes::DIGITAL_NONE;
  config->hwMapping.digital[1][3] = DigitalModuleTypes::DIGITAL_NONE;
  config->hwMapping.digital[1][4] = DigitalModuleTypes::DIGITAL_NONE;
  config->hwMapping.digital[1][5] = DigitalModuleTypes::DIGITAL_NONE;
  config->hwMapping.digital[1][6] = DigitalModuleTypes::DIGITAL_NONE;
  config->hwMapping.digital[1][7] = DigitalModuleTypes::DIGITAL_NONE;

  config->hwMapping.analog[0][0] = AnalogModuleTypes::ANALOG_NONE;
  config->hwMapping.analog[0][1] = AnalogModuleTypes::ANALOG_NONE;
  config->hwMapping.analog[0][2] = AnalogModuleTypes::ANALOG_NONE;
  config->hwMapping.analog[0][3] = AnalogModuleTypes::ANALOG_NONE;
  config->hwMapping.analog[0][4] = AnalogModuleTypes::ANALOG_NONE;
  config->hwMapping.analog[0][5] = AnalogModuleTypes::ANALOG_NONE;
  config->hwMapping.analog[0][6] = AnalogModuleTypes::ANALOG_NONE;
  config->hwMapping.analog[0][7] = AnalogModuleTypes::ANALOG_NONE;
  config->hwMapping.analog[1][0] = AnalogModuleTypes::ANALOG_NONE;
  config->hwMapping.analog[1][1] = AnalogModuleTypes::ANALOG_NONE;
  config->hwMapping.analog[1][2] = AnalogModuleTypes::ANALOG_NONE;
  config->hwMapping.analog[1][3] = AnalogModuleTypes::ANALOG_NONE;
  config->hwMapping.analog[1][4] = AnalogModuleTypes::ANALOG_NONE;
  config->hwMapping.analog[1][5] = AnalogModuleTypes::ANALOG_NONE;
  config->hwMapping.analog[1][6] = AnalogModuleTypes::ANALOG_NONE;
  config->hwMapping.analog[1][7] = AnalogModuleTypes::ANALOG_NONE;
  
  config->hwMapping.analog[2][0] = AnalogModuleTypes::ANALOG_NONE;
  config->hwMapping.analog[2][1] = AnalogModuleTypes::ANALOG_NONE;
  config->hwMapping.analog[2][2] = AnalogModuleTypes::ANALOG_NONE;
  config->hwMapping.analog[2][3] = AnalogModuleTypes::ANALOG_NONE;
  config->hwMapping.analog[2][4] = AnalogModuleTypes::ANALOG_NONE;
  config->hwMapping.analog[2][5] = AnalogModuleTypes::ANALOG_NONE;
  config->hwMapping.analog[2][6] = AnalogModuleTypes::ANALOG_NONE;
  config->hwMapping.analog[3][7] = AnalogModuleTypes::ANALOG_NONE;
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

#define INTENSIDAD_NP 255
void initInputsConfig(uint8_t b) {
  int i = 0;
  for (i = 0; i < config->inputs.digitalCount; i++) {
//    digital[i].actionConfig.action = (i % 2) * switchActions::switch_toggle;
    digital[i].actionConfig.action = switchActions::switch_momentary;
//    digital[i].actionConfig.message = (i) % (digital_rpn + 1) + 1;
    digital[i].actionConfig.message = digital_msg_note;
    digital[i].actionConfig.channel = b;
    digital[i].actionConfig.midiPort = midiPortsType::midi_usb;
    digital[i].actionConfig.parameter[digital_LSB] = i+64;
    digital[i].actionConfig.parameter[digital_MSB] = 0;
    uint16_t minVal = 0;
    uint16_t maxVal = 127;
    digital[i].actionConfig.parameter[digital_minLSB] = minVal & 0xFF;
    digital[i].actionConfig.parameter[digital_minMSB] = (minVal >> 7) & 0xFF;
    digital[i].actionConfig.parameter[digital_maxLSB] = maxVal & 0xFF;
    digital[i].actionConfig.parameter[digital_maxMSB] = (maxVal >> 7) & 0xFF;

//    digital[6].actionConfig.message = digital_msg_key;
//    digital[6].actionConfig.parameter[digital_LSB] = KEY_UP_ARROW;
//    digital[13].actionConfig.message = digital_msg_key;
//    digital[13].actionConfig.parameter[digital_LSB] = KEY_LEFT_ARROW;
//    digital[14].actionConfig.message = digital_msg_key;
//    digital[14].actionConfig.parameter[digital_LSB] = KEY_DOWN_ARROW;
//    digital[15].actionConfig.message = digital_msg_key;
//    digital[15].actionConfig.parameter[digital_LSB] = KEY_RIGHT_ARROW;

    digital[i].feedback.source = feedbackSource::fb_src_usb;
    //if(i > 15) digital[i].feedback.localBehaviour = fb_lb_always_on;
    digital[i].feedback.channel = b;
    digital[i].feedback.message = digital_msg_note;
    digital[i].feedback.parameterLSB = i+64;
    digital[i].feedback.parameterMSB = 0;
    digital[i].feedback.colorRangeEnable = true;
    digital[i].feedback.colorRange0 = 0;
    digital[i].feedback.colorRange1 = 0;
    digital[i].feedback.colorRange2 = 0;
    digital[i].feedback.colorRange3 = 0;
    digital[i].feedback.colorRange4 = 13;
    digital[i].feedback.colorRange5 = 11;
    digital[i].feedback.colorRange6 = 12;
    digital[i].feedback.colorRange7 = 15;
    digital[i].feedback.color[R_INDEX] = (b == 0) ? 0xFF : 0;
    digital[i].feedback.color[G_INDEX] = (b == 0) ? 0x69   : INTENSIDAD_NP;
    digital[i].feedback.color[B_INDEX] = (b == 0) ? 0xB4 : INTENSIDAD_NP;;
  }

  for (i = 0; i < config->inputs.encoderCount; i++) {
    encoder[i].mode.speed = 0;
//    encoder[i].rotaryConfig.message = (i) % (rotary_msg_rpn + 1) + 1;
    encoder[i].rotaryConfig.message = rotary_msg_cc;
    encoder[i].rotaryConfig.channel = b;
    encoder[i].rotaryConfig.midiPort = midiPortsType::midi_usb;
    //    SerialUSB.println(encoder[i].rotaryConfig.midiPort);
    encoder[i].rotaryConfig.parameter[rotary_LSB] = i;
    encoder[i].rotaryConfig.parameter[rotary_MSB] = 0;
    encoder[i].rotaryConfig.parameter[rotary_minLSB] = 0;
    encoder[i].rotaryConfig.parameter[rotary_minMSB] = 0;
    encoder[i].rotaryConfig.parameter[rotary_maxLSB] = 127;
    encoder[i].rotaryConfig.parameter[rotary_maxMSB] = 0;
  
//    encoder[i].rotaryFeedback.mode = i % 4;
    encoder[i].rotaryFeedback.mode = fb_fill;
    encoder[i].rotaryFeedback.source = feedbackSource::fb_src_usb;
    encoder[i].rotaryFeedback.channel = b;
    encoder[i].rotaryFeedback.message = rotaryMessageTypes::rotary_msg_cc;
    encoder[i].rotaryFeedback.parameterLSB = i;
    encoder[i].rotaryFeedback.parameterMSB = 0;
    //    encoder[i].rotaryFeedback.color[R-R] = (i%10)*7+20;
    //    encoder[i].rotaryFeedback.color[R-R] = (i*8)+20*(b+1);
    //    encoder[i].rotaryFeedback.color[G-R] = (i*4)+40*b;
    //    encoder[i].rotaryFeedback.color[B-R] = (i*2)+20;
    encoder[i].rotaryFeedback.color[R_INDEX] = (b == 0) ? 0x9A : 0;
    encoder[i].rotaryFeedback.color[G_INDEX] = (b == 0) ? 0xCD   : INTENSIDAD_NP;
    encoder[i].rotaryFeedback.color[B_INDEX] = (b == 0) ? 0x32 : INTENSIDAD_NP;


    encoder[i].switchConfig.mode = switchModes::switch_mode_message;
//    encoder[i].switchConfig.message = (i) % (digital_rpn + 1) + 1;
    encoder[i].switchConfig.message = switch_msg_cc;
    encoder[i].switchConfig.action = (i % 2) * switchActions::switch_toggle;
    encoder[i].switchConfig.channel = b;
    encoder[i].switchConfig.midiPort = midiPortsType::midi_usb;
    //    SerialUSB.println(encoder[i].rotaryConfig.midiPort);
    encoder[i].switchConfig.parameter[switch_parameter_LSB] = i+32;
    encoder[i].switchConfig.parameter[switch_parameter_MSB] = 0;
    encoder[i].switchConfig.parameter[switch_minValue_LSB] = 0;
    encoder[i].switchConfig.parameter[switch_minValue_MSB] = 0;
    encoder[i].switchConfig.parameter[switch_maxValue_LSB] = 127;
    encoder[i].switchConfig.parameter[switch_maxValue_MSB] = 0;
  
    encoder[i].switchFeedback.source = feedbackSource::fb_src_local;
    encoder[i].switchFeedback.channel = b;
    encoder[i].switchFeedback.message = digital_msg_note;
    encoder[i].switchFeedback.parameterLSB = i+32;
    encoder[i].switchFeedback.parameterMSB = 0;
    encoder[i].switchFeedback.colorRangeEnable = false;
    encoder[i].switchFeedback.colorRange0 = 0;
    encoder[i].switchFeedback.colorRange1 = 2;
    encoder[i].switchFeedback.colorRange2 = 3;
    encoder[i].switchFeedback.colorRange3 = 4;
    encoder[i].switchFeedback.colorRange4 = 5;
    encoder[i].switchFeedback.colorRange5 = 12;
    encoder[i].switchFeedback.colorRange6 = 13;
    encoder[i].switchFeedback.colorRange7 = 14;
    encoder[i].switchFeedback.color[R_INDEX] = (b == 0) ? 0xFF  : INTENSIDAD_NP;
    encoder[i].switchFeedback.color[G_INDEX] = (b == 0) ? 0xD7  : 0;
    encoder[i].switchFeedback.color[B_INDEX] = (b == 0) ? 0x00  : 0;
  }

    
    
  for (i = 0; i < config->inputs.analogCount; i++) {
//    if (i < 16) analog[i].message = i % (analog_rpn + 1) + 1;
//    if (i >= 8 && i < 13){
//      analog[i].message = analogMessageTypes::analog_msg_cc;
//      analog[i].parameter[rotary_LSB] = i-8;
//    }
//    if (i >= 24 && i < 31){
//      analog[i].message = analogMessageTypes::analog_msg_cc;
//      analog[i].parameter[rotary_LSB] = i-20;
//    }
//    else                    analog[i].message = analogMessageTypes::analog_msg_none;  

//    if (i >= 0 && i < 12)  analog[i].message = analogMessageTypes::analog_msg_cc;
//    //else if (i >= 16 && i < 32)  analog[i].message = analogMessageTypes::analog_msg_cc;
//    else        analog[i].message = analogMessageTypes::analog_msg_none;  
    
//    if (i < 16) analog[i].message = analogMessageTypes::analog_msg_cc;
//    if (i >= 16 && i < 32) analog[i].message = analogMessageTypes::analog_msg_cc;
//    if (i >= 32 && i < 48) analog[i].message = analogMessageTypes::analog_msg_cc;
//    if (i >= 48 && i < 64) analog[i].message = analogMessageTypes::analog_msg_cc;
//    if (i >= 48 && i < 64) analog[i].message = i % (analog_rpn + 1) + 1;
    analog[i].message = analogMessageTypes::analog_msg_none;  
    analog[i].channel = b;
    analog[i].midiPort = midiPortsType::midi_usb;
    //    SerialUSB.println(encoder[i].rotaryConfig.midiPort);
    analog[i].parameter[rotary_LSB] = i;
    analog[i].parameter[rotary_MSB] = 0;
    analog[i].parameter[rotary_minLSB] = 0;
    analog[i].parameter[rotary_minMSB] = 0;
    analog[i].parameter[rotary_maxLSB] = 127;
    analog[i].parameter[rotary_maxMSB] = ((analog[i].message == analogMessageTypes::analog_msg_nrpn) || 
                                          (analog[i].message == analogMessageTypes::analog_msg_rpn) || 
                                          (analog[i].message == analogMessageTypes::analog_msg_pb)) ? 127 : 0;
  }
}
