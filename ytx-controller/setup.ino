//----------------------------------------------------------------------------------------------------
// SETUP
//----------------------------------------------------------------------------------------------------

//#define PRINT_CONFIG
#define INIT_CONFIG
//#define ERASE_EEPROM

extern uint8_t STRING_PRODUCT[];
extern uint8_t STRING_MANUFACTURER[];
extern DeviceDescriptor USB_DeviceDescriptorB;
extern DeviceDescriptor USB_DeviceDescriptor;

void setup() {

  SPI.begin();              // TO ENCODERS AND DIGITAL
  SerialUSB.begin(250000);  // TO PC
  Serial.begin(1000000);    // FEEDBACK -> SAMD11

  // CAUSA DEL ULTIMO RESET
//  SerialUSB.println(PM->RCAUSE.reg);

  pinMode(pinExternalVoltage, INPUT);
  pinMode(pinResetSAMD11, OUTPUT);
  digitalWrite(pinResetSAMD11, HIGH);

  // RESET SAMD11
  ResetFBMicro();
  delay(50); // wait for samd11 reset

  // EEPROM INITIALIZATION
  uint8_t eepStatus = eep.begin(extEEPROM::twiClock600kHz,extEEPROM::twiClock400kHz); //go fast!
  if (eepStatus) {
    SerialUSB.print(F("extEEPROM.begin() failed, status = ")); SerialUSB.println(eepStatus);
    delay(1000);
//    while (1);
  }

  memHost = new memoryHost(&eep, ytxIOBLOCK::BLOCKS_COUNT);
  memHost->ConfigureBlock(ytxIOBLOCK::Configuration, 1, sizeof(ytxConfigurationType), true);
  config = (ytxConfigurationType*) memHost->Block(ytxIOBLOCK::Configuration);    
        
#ifdef INIT_CONFIG
   // DUMMY INIT - LATER TO BE REPLACED BY KILOWHAT
  initConfig();
#endif
    
#ifdef ERASE_EEPROM
  eeErase(128, 0, 65535);  
#endif
  
  // Read fw signature from eeprom. If there is, initialize RAM for all controls.
  if (config->board.signature == SIGNATURE_CHAR) {      // SIGNATURE CHECK SUCCESS
    SerialUSB.println(F("YTX VALID CONFIG FOUND"));
    
    // MODIFY DESCRIPTORS TO RENAME CONTROLLER
    strcpy((char*)STRING_PRODUCT, config->board.deviceName);
    strcpy((char*)STRING_MANUFACTURER, "Yaeltex");
    USB_DeviceDescriptor.idVendor = 0x2342;
    USB_DeviceDescriptorB.idVendor = 0x2342;
    USB_DeviceDescriptor.idProduct = config->board.pid;
    USB_DeviceDescriptorB.idProduct = config->board.pid;
  
    // INIT USB DEVICE (this was taken from Arduino zero's core main.cpp - It was done before setup())
  #if defined(USBCON)
    USBDevice.init();
    USBDevice.attach();
  #endif
  
    // Wait for serial monitor to open
    while (!SerialUSB);

    enableProcessing = true; // process inputs on loop
    
    // Create memory map for eeprom
    memHost->ConfigureBlock(ytxIOBLOCK::Encoder, config->inputs.encoderCount, sizeof(ytxEncoderType), false);
    memHost->ConfigureBlock(ytxIOBLOCK::Analog, config->inputs.analogCount, sizeof(ytxAnalogType), false);
    memHost->ConfigureBlock(ytxIOBLOCK::Digital, config->inputs.digitalCount, sizeof(ytxDigitaltype), false);
    memHost->ConfigureBlock(ytxIOBLOCK::Feedback, config->inputs.feedbackCount, sizeof(ytxFeedbackType), false);
    memHost->LayoutBanks();

    encoder = (ytxEncoderType*) memHost->Block(ytxIOBLOCK::Encoder);
    analog = (ytxAnalogType*) memHost->Block(ytxIOBLOCK::Analog);
    digital = (ytxDigitaltype*) memHost->Block(ytxIOBLOCK::Digital);
    feedback = (ytxFeedbackType*) memHost->Block(ytxIOBLOCK::Feedback);

#ifdef INIT_CONFIG
    for (int b = 0; b < config->banks.count; b++) {
      initInputsConfig(b);
      memHost->SaveBank(b);
    }
#endif    
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

    
#ifdef PRINT_CONFIG
  printConfig(ytxIOBLOCK::Configuration, 0);
  for(int e = 0; e < config->inputs.encoderCount; e++)
    printConfig(ytxIOBLOCK::Encoder, e);
  for(int d = 0; d < config->inputs.digitalCount; d++)
    printConfig(ytxIOBLOCK::Digital, d);
  for(int a = 0; a < config->inputs.analogCount; a++)
    printConfig(ytxIOBLOCK::Analog, a);
#endif
  } else {
    // SIGNATURE CHECK FAILED
    SerialUSB.println(F("YTX NOT VALID CONFIG FOUND"));
    SerialUSB.print(F("YTX SIGNATURE BYTE: ")); SerialUSB.println(config->board.signature);

    // MODIFY DESCRIPTORS TO RENAME CONTROLLER
    strcpy((char*)STRING_PRODUCT, "KilomuxV2");
    strcpy((char*)STRING_MANUFACTURER, "Yaeltex");
    USB_DeviceDescriptor.idVendor = 0x2342;
    USB_DeviceDescriptorB.idVendor = 0x2342;
    USB_DeviceDescriptor.idProduct = 0x2000;
    USB_DeviceDescriptorB.idProduct = 0x2000;
  
    // INIT USB DEVICE (this was taken from Arduino zero's core main.cpp - It was done before setup())
  #if defined(USBCON)
    USBDevice.init();
    USBDevice.attach();
  #endif

    enableProcessing = false;
  }

  // Begin MIDI USB port and set handler for Sysex Messages
  MIDI.begin(MIDI_CHANNEL_OMNI); // Se inicializa la comunicación MIDI por USB.
  MIDI.turnThruOff();            // Por default, la librería de Arduino MIDI tiene el THRU en ON, y NO QUEREMOS ESO!
  MIDI.setHandleSystemExclusive(handleSystemExclusive);
  // Begin MIDI HW (DIN5) port
  MIDIHW.begin(MIDI_CHANNEL_OMNI); // Se inicializa la comunicación MIDI por puerto serie(DIN5).
  MIDIHW.turnThruOff();            // Por default, la librería de Arduino MIDI tiene el THRU en ON, y NO QUEREMOS ESO!

  uint32_t sampleRate = 12; //sample rate in milliseconds, determines how often TC5_Handler is called
  tcConfigure(sampleRate); //configure the timer to run at <sampleRate>Hertz
  tcStartCounter(); //starts the timer
  
  if(enableProcessing){
    // Set handlers for each port and message
    MIDI.setHandleNoteOn(handleNoteOnUSB);
    MIDIHW.setHandleNoteOn(handleNoteOnHW);
    MIDI.setHandleNoteOff(handleNoteOffUSB);
    MIDIHW.setHandleNoteOff(handleNoteOffHW);
    MIDI.setHandleControlChange(handleControlChangeUSB);
    MIDIHW.setHandleControlChange(handleControlChangeHW);
    MIDI.setHandlePitchBend(handlePitchBendUSB);
    MIDIHW.setHandlePitchBend(handlePitchBendHW);
    
    // If this controller is routing messages, make nrpnStepInterval bigger (default is 5ms)
    if( config->midiConfig.midiMergeFlags & 0x01 || 
        config->midiConfig.midiMergeFlags & 0x02 ||
        config->midiConfig.midiMergeFlags & 0x04 || 
        config->midiConfig.midiMergeFlags & 0x08){
      nrpnIntervalStep = 10;
    }
  
    // Begin keyboard communication
    Keyboard.begin();
  
    // Initialize brigthness and power configuration
    feedbackHw.InitPower();
    
    // While rainbow is on, initialize MIDI buffer
    for (int b = 0; b < config->banks.count; b++) {
      currentBank = memHost->LoadBank(b);
      MidiSettingsInit();
    }
    
    SerialUSB.print(F("Channels to listen: ")); SerialUSB.println(midiRxSettings.listenToChannel, BIN);
    
    // Calculate and dinamically allocate entries for MIDI buffer
    if(midiRxSettings.midiBufferSize7 > MIDI_BUF_MAX_LEN) midiRxSettings.midiBufferSize7 = MIDI_BUF_MAX_LEN;
    
    midiMsgBuf7 = (midiMsgBuffer7*) memHost->AllocateRAM(midiRxSettings.midiBufferSize7*sizeof(midiMsgBuffer7));
    midiMsgBuf14 = (midiMsgBuffer14*) memHost->AllocateRAM(midiRxSettings.midiBufferSize14*sizeof(midiMsgBuffer14));
    SerialUSB.print(F("midi buffer size 7 bit: ")); SerialUSB.println(midiRxSettings.midiBufferSize7);
    SerialUSB.print(F("midi buffer size 14 bit: ")); SerialUSB.println(midiRxSettings.midiBufferSize14);

    MidiBufferInitClear();
    
    for (int b = 0; b < config->banks.count; b++) {
      currentBank = memHost->LoadBank(b);
      MidiBufferFill();
    }
    currentBank = memHost->LoadBank(0);

    printMidiBuffer(); 
    
    // Wait for rainbow animation to end
    while (!(Serial.read() == END_OF_RAINBOW));
    // Set all initial values for feedback to show
    feedbackHw.SetBankChangeFeedback();
  }
    
 // STATUS LED
  statusLED = Adafruit_NeoPixel(N_STATUS_PIXEL, STATUS_LED_PIN, NEO_GRB + NEO_KHZ800);
  statusLED.begin();
  statusLED.setBrightness(STATUS_LED_BRIGHTNESS);
  statusLED.show();
  
  SetStatusLED(STATUS_BLINK, 3, STATUS_FB_CONFIG);
  
  SerialUSB.print(F("Free RAM: ")); SerialUSB.println(FreeMemory()); 
}

#ifdef INIT_CONFIG
void initConfig() {
  // SET NUMBER OF INPUTS OF EACH TYPE
  config->banks.count = 2;
  config->inputs.encoderCount = 32;
  config->inputs.analogCount = 0;
  config->inputs.digitalCount = 32;
  config->inputs.feedbackCount = 0;

  config->midiMergeFlags = 0x00;

  config->board.signature = SIGNATURE_CHAR;
  strcpy(config->board.deviceName, "MiniblockV2");
  config->board.pid = 0x2231;
  strcpy(config->board.serialNumber, "ABCDEFGHI");

  for (int bank = 0; bank < MAX_BANKS; bank++) {
    config->banks.shifterId[bank] = config->inputs.encoderCount + bank;
//    config->banks.shifterId[bank] = bank;
  }
  config->banks.momToggFlags = 0b00001111;

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
//  config->hwMapping.digital[0][4] = DigitalModuleTypes::RB82;
//  config->hwMapping.digital[0][5] = DigitalModuleTypes::RB82;
//  config->hwMapping.digital[0][6] = DigitalModuleTypes::RB82;
//  config->hwMapping.digital[0][7] = DigitalModuleTypes::RB82;
//  config->hwMapping.digital[1][0] = DigitalModuleTypes::RB82;
//  config->hwMapping.digital[1][1] = DigitalModuleTypes::RB82;
//  config->hwMapping.digital[1][2] = DigitalModuleTypes::RB82;
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

#define INTENSIDAD_NP 255
void initInputsConfig(uint8_t b) {
  int i = 0;

  for (i = 0; i < config->inputs.encoderCount; i++) {
    encoder[i].mode.hwMode = 0;
    encoder[i].mode.speed = 0;
    encoder[i].mode.unused = 0;
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
    strcpy(encoder[i].rotaryConfig.comment, "");
    
    encoder[i].rotaryFeedback.mode = encoderRotaryFeedbackMode::fb_walk;
//    encoder[i].rotaryFeedback.mode = i % 4;
    encoder[i].rotaryFeedback.source = feedbackSource::fb_src_local;
    encoder[i].rotaryFeedback.channel = b;
//    encoder[i].rotaryFeedback.message = (i) % (rotary_msg_rpn + 1) + 1;
    encoder[i].rotaryFeedback.message = rotaryMessageTypes::rotary_msg_cc;
    encoder[i].rotaryFeedback.parameterLSB = i;
    encoder[i].rotaryFeedback.parameterMSB = 0;
    //    encoder[i].rotaryFeedback.color[R-R] = (i%10)*7+20;
    //    encoder[i].rotaryFeedback.color[R-R] = (i*8)+20*(b+1);
    //    encoder[i].rotaryFeedback.color[G-R] = (i*4)+40*b;
    //    encoder[i].rotaryFeedback.color[B-R] = (i*2)+20;
    encoder[i].rotaryFeedback.color[R_INDEX] = (b == 0) ? 0x9A : 0;
    encoder[i].rotaryFeedback.color[G_INDEX] = (b == 0) ? 0xCD : INTENSIDAD_NP;
    encoder[i].rotaryFeedback.color[B_INDEX] = (b == 0) ? 0x32 : INTENSIDAD_NP;
    

    encoder[i].switchConfig.mode = switchModes::switch_mode_message;
    //encoder[i].switchConfig.message = (i) % (switch_msg_rpn + 1) + 1;
    encoder[i].switchConfig.message = switch_msg_note;
//    encoder[i].switchConfig.action = (i % 2) * switchActions::switch_toggle;
    encoder[i].switchConfig.action = switchActions::switch_toggle;
    encoder[i].switchConfig.channel = b;
    encoder[i].switchConfig.midiPort = midiPortsType::midi_usb;
    //    SerialUSB.println(encoder[i].rotaryConfig.midiPort);
    encoder[i].switchConfig.parameter[switch_parameter_LSB] = i + 32;
    encoder[i].switchConfig.parameter[switch_parameter_MSB] = 0;
    encoder[i].switchConfig.parameter[switch_minValue_LSB] = 0;
    encoder[i].switchConfig.parameter[switch_minValue_MSB] = 0;
    encoder[i].switchConfig.parameter[switch_maxValue_LSB] = 127;
    encoder[i].switchConfig.parameter[switch_maxValue_MSB] = 0;

    encoder[i].switchFeedback.source = feedbackSource::fb_src_local;
    encoder[i].switchFeedback.localBehaviour = fb_lb_on_with_press;
    encoder[i].switchFeedback.channel = b;
//    encoder[i].switchFeedback.message = (i) % (switch_msg_rpn + 1) + 1;
    encoder[i].switchFeedback.message = switch_msg_note;
    encoder[i].switchFeedback.parameterLSB = i + 32;
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

  for (i = 0; i < config->inputs.digitalCount; i++) {
    //    digital[i].actionConfig.action = (i % 2) * switchActions::switch_toggle;
    digital[i].actionConfig.action = switchActions::switch_toggle;
    //    digital[i].actionConfig.message = (i) % (digital_rpn + 1) + 1;
    digital[i].actionConfig.message = digital_msg_note;

    digital[i].actionConfig.channel = b;
    digital[i].actionConfig.midiPort = midiPortsType::midi_usb;
    //    SerialUSB.println(digital[i].actionConfig.midiPort);
    //    if(!i%digital_ks){
    //      digital[i].actionConfig.parameter[digital_key] = '+';
    //      digital[i].actionConfig.parameter[digital_modifier] = KEY_LEFT_CTRL;
    //    }else{
    digital[i].actionConfig.parameter[digital_LSB] = i + 64;
    digital[i].actionConfig.parameter[digital_MSB] = 0;
    uint16_t minVal = 0;
    uint16_t maxVal = 127;
    digital[i].actionConfig.parameter[digital_minLSB] = minVal & 0xFF;
    digital[i].actionConfig.parameter[digital_minMSB] = (minVal >> 7) & 0xFF;
    digital[i].actionConfig.parameter[digital_maxLSB] = maxVal & 0xFF;
    digital[i].actionConfig.parameter[digital_maxMSB] = (maxVal >> 7) & 0xFF;
    strcpy(digital[i].actionConfig.comment, "");

    //    digital[6].actionConfig.message = digital_msg_key;
    //    digital[6].actionConfig.parameter[digital_LSB] = KEY_UP_ARROW;
    //    digital[13].actionConfig.message = digital_msg_key;
    //    digital[13].actionConfig.parameter[digital_LSB] = KEY_LEFT_ARROW;
    //    digital[14].actionConfig.message = digital_msg_key;
    //    digital[14].actionConfig.parameter[digital_LSB] = KEY_DOWN_ARROW;
    //    digital[15].actionConfig.message = digital_msg_key;
    //    digital[15].actionConfig.parameter[digital_LSB] = KEY_RIGHT_ARROW;

    digital[i].feedback.source = feedbackSource::fb_src_local;
    digital[i].feedback.localBehaviour = fb_lb_on_with_press;
    digital[i].feedback.channel = b;
    digital[i].feedback.message = digital_msg_note;
    digital[i].feedback.parameterLSB = i + 64;
    digital[i].feedback.parameterMSB = 0;
    digital[i].feedback.colorRangeEnable = false;
    digital[i].feedback.colorRange0 = 0;
    digital[i].feedback.colorRange1 = 2;
    digital[i].feedback.colorRange2 = 3;
    digital[i].feedback.colorRange3 = 4;
    digital[i].feedback.colorRange4 = 5;
    digital[i].feedback.colorRange5 = 12;
    digital[i].feedback.colorRange6 = 13;
    digital[i].feedback.colorRange7 = 14;
    digital[i].feedback.color[R_INDEX] = (b == 0) ? 0xFF : 0;
    digital[i].feedback.color[G_INDEX] = (b == 0) ? 0x69   : INTENSIDAD_NP;
    digital[i].feedback.color[B_INDEX] = (b == 0) ? 0xB4 : INTENSIDAD_NP;;
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
    
    analog[i].feedback.message = i%2 ? analogMessageTypes::analog_msg_cc : analogMessageTypes::analog_msg_nrpn;
//    analog[i].feedback.message = analog_msg_nrpn;
//    analog[i].message = i%2 ? analogMessageTypes::analog_msg_cc : analogMessageTypes::analog_msg_nrpn;
    analog[i].feedback.channel = b;
    analog[i].feedback.source = midiPortsType::midi_hw_usb;
    analog[i].feedback.parameterLSB = 32+i;
    analog[i].feedback.parameterMSB = 0;
    analog[i].feedback.colorRangeEnable = false;
    analog[i].feedback.color[R_INDEX] = 0xFF;
    analog[i].feedback.color[G_INDEX] = 0x00;
    analog[i].feedback.color[B_INDEX] = 0x00;
  }
}
#endif


void printConfig(uint8_t block, uint8_t i){
  
  if(block == ytxIOBLOCK::Configuration){
    SerialUSB.println(F("--------------------------------------------------------"));
    SerialUSB.println(F("GENERAL CONFIGURATION"));
    
    SerialUSB.print(F("Encoder count: ")); SerialUSB.println(config->inputs.encoderCount);
    SerialUSB.print(F("Analog count: ")); SerialUSB.println(config->inputs.analogCount);
    SerialUSB.print(F("Digital count: ")); SerialUSB.println(config->inputs.digitalCount);
    SerialUSB.print(F("Feedback count: ")); SerialUSB.println(config->inputs.feedbackCount);
    
    SerialUSB.print(F("Device name: ")); SerialUSB.println((char*)config->board.deviceName);
    SerialUSB.print(F("USB-PID: ")); SerialUSB.println(config->board.pid,HEX);
    SerialUSB.print(F("Serial number: ")); SerialUSB.println((char*)config->board.serialNumber);
    
    for(int mE = 0; mE < 8; mE++){
      SerialUSB.print(F("Encoder module ")); SerialUSB.print(mE); SerialUSB.print(F(": ")); 
      SerialUSB.println(config->hwMapping.encoder[mE] == 0 ? F("NONE") :
                        config->hwMapping.encoder[mE] == 1 ? F("E41H") :
                        config->hwMapping.encoder[mE] == 2 ? F("E41V") :
                        config->hwMapping.encoder[mE] == 3 ? F("E41H_D") : 
                        config->hwMapping.encoder[mE] == 4 ? F("E41V_D") : F("NOT DEFINED"));
    }
    for(int aPort = 0; aPort < 4; aPort++){
      for(int mA = 0; mA < 8; mA++){
        SerialUSB.print(F("Analog port/module ")); SerialUSB.print(aPort); SerialUSB.print(F("/")); SerialUSB.print(mA); SerialUSB.print(F(": ")); 
        SerialUSB.println(config->hwMapping.analog[aPort][mA] == 0 ? F("NONE") :
                          config->hwMapping.analog[aPort][mA] == 1 ? F("P41") :
                          config->hwMapping.analog[aPort][mA] == 2 ? F("F41") :
                          config->hwMapping.analog[aPort][mA] == 3 ? F("JAF") : 
                          config->hwMapping.analog[aPort][mA] == 4 ? F("JAL") : F("NOT DEFINED"));
      }
    }
    for(int dPort = 0; dPort < 2; dPort++){
      for(int mD = 0; mD < 8; mD++){
        SerialUSB.print(F("Digital port/module ")); SerialUSB.print(dPort); SerialUSB.print(F("/")); SerialUSB.print(mD); SerialUSB.print(F(": ")); 
        SerialUSB.println(config->hwMapping.digital[dPort][mD] == 0 ? F("NONE") :
                          config->hwMapping.digital[dPort][mD] == 1 ? F("RB41") :
                          config->hwMapping.digital[dPort][mD] == 2 ? F("RB42") :
                          config->hwMapping.digital[dPort][mD] == 3 ? F("RB82") : 
                          config->hwMapping.digital[dPort][mD] == 4 ? F("ARC41") : F("NOT DEFINED"));
      }
    }
    
    SerialUSB.print(F("Midi merge routing:")); 
    if(config->midiConfig.midiMergeFlags & 0x01) SerialUSB.print(F(" HW -> HW /"));
    if(config->midiConfig.midiMergeFlags & 0x02) SerialUSB.print(F(" HW -> USB /"));
    if(config->midiConfig.midiMergeFlags & 0x04) SerialUSB.print(F(" USB -> HW /"));
    if(config->midiConfig.midiMergeFlags & 0x08) SerialUSB.print(F(" USB -> USB"));
    SerialUSB.println();

    SerialUSB.print(F("Number of banks: ")); SerialUSB.println(config->banks.count);
    for(int b = 0; b < MAX_BANKS; b++){
      SerialUSB.print(F("Bank ")); SerialUSB.print(b); SerialUSB.print(F(" shifter ID: ")); SerialUSB.print(config->banks.shifterId[b]); 
      SerialUSB.print(F(" Action for bank ")); SerialUSB.print(b); SerialUSB.print(F(" is ")); SerialUSB.println((config->banks.momToggFlags>>b) & 0x1 ? F("TOGGLE") : F("MOMENTARY")); 
    }
    
  }else if(block == ytxIOBLOCK::Encoder){
    SerialUSB.println(F("--------------------------------------------------------"));
    SerialUSB.print(F("Encoder ")); SerialUSB.print(i); SerialUSB.println(F(":"));
    SerialUSB.print(F("HW mode: ")); SerialUSB.println(encoder[i].mode.hwMode == 0 ? F("REL") : 
                                                    encoder[i].mode.hwMode == 1 ? F("ABS") : F("NOT DEFINED"));
    SerialUSB.print(F("Speed: ")); SerialUSB.println(encoder[i].mode.speed == 0 ? F("ACCEL") : 
                                                  encoder[i].mode.speed == 1 ? F("VEL 1") : 
                                                  encoder[i].mode.speed == 2 ? F("VEL 2") : 
                                                  encoder[i].mode.speed == 3 ? F("VEL 3") : F("NOT DEFINED"));
    
    SerialUSB.println(); 
    SerialUSB.print(F("Rotary Message: ")); 
    switch(encoder[i].rotaryConfig.message){
      case rotary_msg_none:
        SerialUSB.println(F("NONE"));
      break;
      case rotary_msg_note:
        SerialUSB.println(F("NOTE"));
      break;
      case rotary_msg_cc:
        SerialUSB.println(F("CC"));
      break;
      case rotary_msg_pc_rel:
        SerialUSB.println(F("PROGRAM CHANGE"));
      break;
      case rotary_msg_nrpn:
        SerialUSB.println(F("NRPN"));
      break;
      case rotary_msg_rpn:
        SerialUSB.println(F("RPN"));
      break;
      case rotary_msg_pb:
        SerialUSB.println(F("PITCH BEND"));
      break;
      case rotary_msg_key:
        SerialUSB.println(F("KEYSTROKE"));
      break;
      default:
        SerialUSB.println(F("NOT DEFINED"));
      break;
    }
    SerialUSB.print(F("Rotary MIDI Channel: ")); SerialUSB.println(encoder[i].rotaryConfig.channel+1);
    SerialUSB.print(F("Rotary MIDI Port: ")); SerialUSB.println( encoder[i].rotaryConfig.midiPort == 0 ? F("NONE") : 
                                                              encoder[i].rotaryConfig.midiPort == 1 ? F("USB") :
                                                              encoder[i].rotaryConfig.midiPort == 2 ? F("MIDI HW") : 
                                                              encoder[i].rotaryConfig.midiPort == 3 ? F("USB + MIDI HW") : F("NOT DEFINED"));
    SerialUSB.print(F("Rotary Parameter: ")); SerialUSB.println(encoder[i].rotaryConfig.parameter[rotary_MSB] << 7 | encoder[i].rotaryConfig.parameter[rotary_LSB]);
    SerialUSB.print(F("Rotary MIN value: ")); SerialUSB.println(encoder[i].rotaryConfig.parameter[rotary_minMSB] << 7 | encoder[i].rotaryConfig.parameter[rotary_minLSB]);
    SerialUSB.print(F("Rotary MAX value: ")); SerialUSB.println(encoder[i].rotaryConfig.parameter[rotary_maxMSB] << 7 | encoder[i].rotaryConfig.parameter[rotary_maxLSB]);
    SerialUSB.print(F("Rotary Comment: ")); SerialUSB.println((char*)encoder[i].rotaryConfig.comment);

    SerialUSB.println(); 
    SerialUSB.print(F("Rotary Feedback mode: ")); SerialUSB.println( encoder[i].rotaryFeedback.mode == 0 ? F("WALK") : 
                                                                  encoder[i].rotaryFeedback.mode == 1 ? F("SPREAD") :
                                                                  encoder[i].rotaryFeedback.mode == 2 ? F("FILL") : 
                                                                  encoder[i].rotaryFeedback.mode == 3 ? F("EQ") : F("NOT DEFINED"));
    SerialUSB.print(F("Rotary Feedback source: ")); SerialUSB.println( encoder[i].rotaryFeedback.source == 0 ? F("LOCAL") : 
                                                                    encoder[i].rotaryFeedback.source == 1 ? F("USB") :
                                                                    encoder[i].rotaryFeedback.source == 2 ? F("MIDI HW") : 
                                                                    encoder[i].rotaryFeedback.source == 3 ? F("USB + MIDI HW") : F("NOT DEFINED"));             
    SerialUSB.print(F("Rotary Feedback Message: "));
    switch(encoder[i].rotaryFeedback.message){
      case rotary_msg_none:
        SerialUSB.println(F("NONE"));
      break;
      case rotary_msg_note:
        SerialUSB.println(F("NOTE"));
      break;
      case rotary_msg_cc:
        SerialUSB.println(F("CC"));
      break;
      case rotary_msg_pc_rel:
        SerialUSB.println(F("PROGRAM CHANGE"));
      break;
      case rotary_msg_nrpn:
        SerialUSB.println(F("NRPN"));
      break;
      case rotary_msg_rpn:
        SerialUSB.println(F("RPN"));
      break;
      case rotary_msg_pb:
        SerialUSB.println(F("PITCH BEND"));
      break;
      default:
        SerialUSB.println(F("NOT DEFINED"));
      break;
    }                                                                                                                         
    SerialUSB.print(F("Rotary Feedback MIDI Channel: ")); SerialUSB.println(encoder[i].rotaryFeedback.channel+1);
    SerialUSB.print(F("Rotary Feedback Parameter: ")); SerialUSB.println(encoder[i].rotaryFeedback.parameterMSB << 7 | encoder[i].rotaryFeedback.parameterLSB);
    SerialUSB.print(F("Rotary Feedback Color: ")); SerialUSB.print(encoder[i].rotaryFeedback.color[0],HEX); 
                                                SerialUSB.print(encoder[i].rotaryFeedback.color[1],HEX);
                                                SerialUSB.println(encoder[i].rotaryFeedback.color[2],HEX);
        
                                                              
    SerialUSB.println(); 
    SerialUSB.print(F("Switch action: ")); SerialUSB.println(encoder[i].switchConfig.action == 0 ? F("MOMENTARY") : F("TOGGLE"));
    SerialUSB.print(F("Switch double click config: ")); SerialUSB.println(encoder[i].switchConfig.doubleClick == 0 ? F("NONE") :
                                                                       encoder[i].switchConfig.doubleClick == 1 ? F("JUMP TO MIN") :
                                                                       encoder[i].switchConfig.doubleClick == 2 ? F("JUMP TO CENTER") : 
                                                                       encoder[i].switchConfig.doubleClick == 3 ? F("JUMP TO MAX") : F("NOT DEFINED"));
    SerialUSB.print(F("Switch Mode: ")); 
    switch(encoder[i].switchConfig.mode){
      case switch_mode_none:
        SerialUSB.println(F("NONE"));
      break;
      case switch_mode_message:
        SerialUSB.println(F("MIDI MSG"));
      break;
      case switch_mode_shift_rot:
        SerialUSB.println(F("SHIFT ROTARY ACTION"));
      break;
      case switch_mode_fine:
        SerialUSB.println(F("FINE ADJUST"));
      break;
      case switch_mode_2cc:
        SerialUSB.println(F("DOUBLE CC"));
      break;
      case switch_mode_quick_shift:
        SerialUSB.println(F("QUICK SHIFT TO BANK"));
      break;
      case switch_mode_quick_shift_note:
        SerialUSB.println(F("QUICK SHIFT TO BANK + NOTE"));
      break;
      default:
        SerialUSB.println(F("NOT DEFINED"));
      break;
    }                                                                         
    SerialUSB.print(F("Switch Message: ")); 
    switch(encoder[i].switchConfig.message){
      case switch_msg_note:
        SerialUSB.println(F("NOTE"));
      break;
      case switch_msg_cc:
        SerialUSB.println(F("CC"));
      break;
      case switch_msg_pc:
        SerialUSB.println(F("PROGRAM CHANGE #"));
      break;
      case switch_msg_pc_m:
        SerialUSB.println(F("PROGRAM CHANGE -"));
      break;
      case switch_msg_pc_p:
        SerialUSB.println(F("PROGRAM CHANGE +"));
      break;
      case switch_msg_nrpn:
        SerialUSB.println(F("NRPN"));
      break;
      case switch_msg_rpn:
        SerialUSB.println(F("RPN"));
      break;
      case switch_msg_pb:
        SerialUSB.println(F("PITCH BEND"));
      break;
      case switch_msg_key:
        SerialUSB.println(F("KEYSTROKE"));
      break;
      default:
        SerialUSB.println(F("NOT DEFINED"));
      break;
    }
    SerialUSB.print(F("Switch MIDI Channel: ")); SerialUSB.println(encoder[i].switchConfig.channel+1);
    SerialUSB.print(F("Switch MIDI Port: ")); SerialUSB.println(encoder[i].switchConfig.midiPort == 0 ? F("NONE") : 
                                                      encoder[i].switchConfig.midiPort == 1 ? F("USB") :
                                                      encoder[i].switchConfig.midiPort == 2 ? F("MIDI HW") : 
                                                      encoder[i].switchConfig.midiPort == 3 ? F("USB + MIDI HW") : F("NOT DEFINED"));
    SerialUSB.print(F("Switch Parameter: ")); SerialUSB.println(encoder[i].switchConfig.parameter[switch_parameter_MSB] << 7 | encoder[i].switchConfig.parameter[switch_parameter_LSB]);
    SerialUSB.print(F("Switch MIN value: ")); SerialUSB.println(encoder[i].switchConfig.parameter[switch_minValue_MSB] << 7 | encoder[i].switchConfig.parameter[switch_minValue_LSB]);
    SerialUSB.print(F("Switch MAX value: ")); SerialUSB.println(encoder[i].switchConfig.parameter[switch_maxValue_MSB] << 7 | encoder[i].switchConfig.parameter[switch_maxValue_LSB]);

    SerialUSB.println(); 
    SerialUSB.print(F("Switch Feedback source: ")); SerialUSB.println( encoder[i].switchFeedback.source == 0 ? F("LOCAL") : 
                                                                    encoder[i].switchFeedback.source == 1 ? F("USB") :
                                                                    encoder[i].switchFeedback.source == 2 ? F("MIDI HW") : 
                                                                    encoder[i].switchFeedback.source == 3 ? F("USB + MIDI HW") : F("NOT DEFINED"));   
    SerialUSB.print(F("Switch Feedback Local behaviour: ")); SerialUSB.println(encoder[i].switchFeedback.localBehaviour == 0 ? F("ON WITH PRESS") :
                                                                               encoder[i].switchFeedback.localBehaviour == 1 ? F("ALWAYS ON") : F("NOT DEFINED"));
    SerialUSB.print(F("Switch Feedback Message: "));                                                                      
    switch(encoder[i].switchFeedback.message){
      case switch_msg_note:
        SerialUSB.println(F("NOTE"));
      break;
      case switch_msg_cc:
        SerialUSB.println(F("CC"));
      break;
      case switch_msg_pc:
        SerialUSB.println(F("PROGRAM CHANGE #"));
      break;
      case switch_msg_pc_m:
        SerialUSB.println(F("PROGRAM CHANGE -"));
      break;
      case switch_msg_pc_p:
        SerialUSB.println(F("PROGRAM CHANGE +"));
      break;
      case switch_msg_nrpn:
        SerialUSB.println(F("NRPN"));
      break;
      case switch_msg_rpn:
        SerialUSB.println(F("RPN"));
      break;
      case switch_msg_pb:
        SerialUSB.println(F("PITCH BEND"));
      break;
      default:
        SerialUSB.println(F("NOT DEFINED"));
      break;
    }
    SerialUSB.print(F("Switch Feedback MIDI Channel: ")); SerialUSB.println(encoder[i].switchFeedback.channel+1);
    SerialUSB.print(F("Switch Feedback Parameter: ")); SerialUSB.println(encoder[i].switchFeedback.parameterMSB << 7 | encoder[i].rotaryFeedback.parameterLSB);
    SerialUSB.print(F("Switch Feedback Color: ")); SerialUSB.print(encoder[i].switchFeedback.color[0],HEX); 
                                                SerialUSB.print(encoder[i].switchFeedback.color[1],HEX);
                                                SerialUSB.println(encoder[i].switchFeedback.color[2],HEX);
  }else if(block == ytxIOBLOCK::Digital){
    SerialUSB.println(F("--------------------------------------------------------"));
    SerialUSB.print(F("Digital ")); SerialUSB.print(i); SerialUSB.println(F(":"));
    SerialUSB.print(F("Digital action: ")); SerialUSB.println(digital[i].actionConfig.action == 0 ? F("MOMENTARY") : F("TOGGLE"));
    
    SerialUSB.print(F("Digital Message: ")); 
    switch(digital[i].actionConfig.message){
      case digital_msg_none:
        SerialUSB.println(F("NONE"));
      break;
      case digital_msg_note:
        SerialUSB.println(F("NOTE"));
      break;
      case digital_msg_cc:
        SerialUSB.println(F("CC"));
      break;
      case digital_msg_pc:
        SerialUSB.println(F("PROGRAM CHANGE #"));
      break;
      case digital_msg_pc_m:
        SerialUSB.println(F("PROGRAM CHANGE -"));
      break;
      case digital_msg_pc_p:
        SerialUSB.println(F("PROGRAM CHANGE +"));
      break;
      case digital_msg_nrpn:
        SerialUSB.println(F("NRPN"));
      break;
      case digital_msg_rpn:
        SerialUSB.println(F("RPN"));
      break;
      case digital_msg_pb:
        SerialUSB.println(F("PITCH BEND"));
      break;
      case digital_msg_key:
        SerialUSB.println(F("KEYSTROKE"));
      break;
      default:
        SerialUSB.println(F("NOT DEFINED"));
      break;
    }
    SerialUSB.print(F("Digital MIDI Channel: ")); SerialUSB.println(digital[i].actionConfig.channel+1);
    SerialUSB.print(F("Digital MIDI Port: ")); SerialUSB.println(digital[i].actionConfig.midiPort == 0 ? F("NONE") : 
                                                                digital[i].actionConfig.midiPort == 1 ? F("USB") :
                                                                digital[i].actionConfig.midiPort == 2 ? F("MIDI HW") : 
                                                                digital[i].actionConfig.midiPort == 3 ? F("USB + MIDI HW") : F("NOT DEFINED"));
    SerialUSB.print(F("Digital Parameter: ")); SerialUSB.println(digital[i].actionConfig.parameter[digital_MSB] << 7 | digital[i].actionConfig.parameter[digital_LSB]);
    SerialUSB.print(F("Digital MIN value: ")); SerialUSB.println(digital[i].actionConfig.parameter[digital_minMSB] << 7 | digital[i].actionConfig.parameter[digital_minLSB]);
    SerialUSB.print(F("Digital MAX value: ")); SerialUSB.println(digital[i].actionConfig.parameter[digital_maxMSB] << 7 | digital[i].actionConfig.parameter[digital_maxLSB]);

    SerialUSB.println(); 
    SerialUSB.print(F("Digital Feedback source: ")); SerialUSB.println(digital[i].feedback.source == 0 ? F("LOCAL") : 
                                                                      digital[i].feedback.source == 1 ? F("USB") :
                                                                      digital[i].feedback.source == 2 ? F("MIDI HW") : 
                                                                      digital[i].feedback.source == 3 ? F("USB + MIDI HW") : F("NOT DEFINED"));   
    SerialUSB.print(F("Digital Feedback Local behaviour: ")); SerialUSB.println(digital[i].feedback.localBehaviour == 0 ? F("ON WITH PRESS") :
                                                                                digital[i].feedback.localBehaviour == 1 ? F("ALWAYS ON") : F("NOT DEFINED"));
    SerialUSB.print(F("Digital Feedback Message: ")); 
    switch(digital[i].feedback.message){
      case digital_msg_none:
        SerialUSB.println(F("NONE"));
      break;
      case digital_msg_note:
        SerialUSB.println(F("NOTE"));
      break;
      case digital_msg_cc:
        SerialUSB.println(F("CC"));
      break;
      case digital_msg_pc:
        SerialUSB.println(F("PROGRAM CHANGE #"));
      break;
      case digital_msg_pc_m:
        SerialUSB.println(F("PROGRAM CHANGE -"));
      break;
      case digital_msg_pc_p:
        SerialUSB.println(F("PROGRAM CHANGE +"));
      break;
      case digital_msg_nrpn:
        SerialUSB.println(F("NRPN"));
      break;
      case digital_msg_rpn:
        SerialUSB.println(F("RPN"));
      break;
      case digital_msg_pb:
        SerialUSB.println(F("PITCH BEND"));
      break;
      case digital_msg_key:
        SerialUSB.println(F("KEYSTROKE"));
      break;
      default:
        SerialUSB.println(F("NOT DEFINED"));
      break;
    }
    SerialUSB.print(F("Digital Feedback MIDI Channel: ")); SerialUSB.println(digital[i].feedback.channel+1);
    SerialUSB.print(F("Digital Feedback Parameter: ")); SerialUSB.println(digital[i].feedback.parameterMSB << 7 | encoder[i].rotaryFeedback.parameterLSB);
    SerialUSB.print(F("Digital Feedback Color: ")); SerialUSB.print(digital[i].feedback.color[0],HEX); 
                                                SerialUSB.print(digital[i].feedback.color[1],HEX);
                                                SerialUSB.println(digital[i].feedback.color[2],HEX);
  }else if(block == ytxIOBLOCK::Analog){
    
  }
}
