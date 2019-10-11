
//----------------------------------------------------------------------------------------------------
// COMMS MIDI - SERIAL
//----------------------------------------------------------------------------------------------------
void ReadMidi(bool midiSrc) {
  uint8_t msgType = midiSrc ? MIDIHW.getType() : MIDI.getType();
  uint8_t channel = midiSrc ? MIDIHW.getChannel() : MIDI.getChannel();
  uint8_t param = midiSrc ? MIDIHW.getData1() : MIDI.getData1();
  uint8_t value = midiSrc ? MIDIHW.getData2() : MIDI.getData2();

  channel--; // GO from 1-16 to 0-15
  
  uint8_t encoderMsgType = 0;
  uint8_t encoderSwitchMsgType = 0;
  uint8_t digitalMsgType = 0;
  uint8_t analogMsgType = 0;
  
  switch (msgType) {
    case midi::NoteOn:
    case midi::NoteOff:
      encoderMsgType = encoderMessageTypes::rotary_enc_note;
      encoderSwitchMsgType = switchModes::switch_mode_note;
      digitalMsgType = digitalMessageTypes::digital_note;
      analogMsgType = analogMessageTypes::analog_note;
//      SerialUSB.println("I AM NOTE!");
    break;
    case midi::ControlChange:
      encoderMsgType = encoderMessageTypes::rotary_enc_cc;
      encoderSwitchMsgType = switchModes::switch_mode_cc;
      digitalMsgType = digitalMessageTypes::digital_cc;
      analogMsgType = analogMessageTypes::analog_cc;
//      SerialUSB.println("I AM CC!");
    break;
    case midi::PitchBend:
      encoderMsgType = encoderMessageTypes::rotary_enc_pb;
      digitalMsgType = digitalMessageTypes::digital_pb;
      analogMsgType = analogMessageTypes::analog_pb;
    break;
  }
  
  SerialUSB.print(midiSrc ? "MIDI_HW: " : "MIDI_USB: ");
  SerialUSB.print(msgType); SerialUSB.print("\t");
  SerialUSB.print(channel); SerialUSB.print("\t");
  SerialUSB.print(param); SerialUSB.print("\t");
  SerialUSB.println(value);
  
  for(uint8_t bank = 0; bank < config->banks.count; bank++){
    if(bank != currentBank) memHost->LoadBank(bank);
    // SWEEP ALL ENCODERS - // FIX FOR SHIFT ROTARY ACTION AND CHANGE ROTARY CONFIG FOR ROTARY FEEDBACK IN ALL CASES
    for(uint8_t encNo = 0; encNo < config->inputs.encoderCount; encNo++){
//      if((encoder[encNo].rotaryFeedback.midiPort&0x01) && !midiSrc || (encoder[encNo].rotaryConfig.midiPort&0x02 && midiSrc)){
      if((encoder[encNo].rotaryFeedback.source&0x01) && !midiSrc || (encoder[encNo].rotaryFeedback.source&0x02 && midiSrc)){
        if(encoder[encNo].rotaryFeedback.channel == channel){
          if(encoder[encNo].rotaryFeedback.message == encoderMsgType){
            if(encoder[encNo].rotaryFeedback.parameterLSB == param || encoderMsgType == encoderMessageTypes::rotary_enc_pb){
              // If there's a match, set encoder value and feedback
              encoderHw.SetEncoderValue(bank, encNo, value);
            }
          }
        }
      }
    }
    // SWEEP ALL ENCODERS SWITCHES
    for(uint8_t encNo = 0; encNo < config->inputs.encoderCount; encNo++){
      if((encoder[encNo].switchFeedback.source&0x01) && !midiSrc || (encoder[encNo].switchFeedback.source&0x02 && midiSrc)){
        if(encoder[encNo].switchFeedback.channel == channel){
          if(encoder[encNo].switchFeedback.message == encoderSwitchMsgType){
            if(encoder[encNo].switchFeedback.parameterLSB == param){
              // If there's a match, set encoder value and feedback
              if(msgType == midi::NoteOff) value = 0;
              encoderHw.SetEncoderSwitchValue(bank, encNo, value);
            }
          }
        }
      }
    }
    // SWEEP ALL DIGITAL
    for(uint16_t digNo = 0; digNo < config->inputs.digitalCount; digNo++){
      
    }
    // SWEEP ALL ANALOG
    for(uint8_t analogNo = 0; analogNo< config->inputs.analogCount; analogNo++){
      
    }
  }
    
}
