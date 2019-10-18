typedef struct{
  uint16_t parameter;  
  uint16_t value;
  uint8_t parameterMSB;  
  uint8_t valueMSB;
  uint8_t parameterLSB;  
  uint8_t valueLSB;
} ytx14bitMessage;

ytx14bitMessage nrpnMessage;
ytx14bitMessage rpnMessage;

//----------------------------------------------------------------------------------------------------
// COMMS MIDI - SERIAL
//----------------------------------------------------------------------------------------------------
void ReadMidi(bool midiSrc) {
  static uint8_t prevParam = 0;
  static uint32_t prevMillisNRPN = 0;
  bool msg14bitComplete = false;
  static bool nrpnOnGoing = false;
  static bool rpnOnGoing = false;
  
  uint8_t msgType = midiSrc ? MIDIHW.getType() : MIDI.getType();
  uint8_t channel = midiSrc ? MIDIHW.getChannel() : MIDI.getChannel();
  uint16_t param = midiSrc ? MIDIHW.getData1() : MIDI.getData1();
  uint16_t value = midiSrc ? MIDIHW.getData2() : MIDI.getData2();

 uint32_t antMicrosComms = micros(); 
  // MIDI THRU
  if(midiSrc){
    if(config->midiMergeFlags & 0x01){
      MIDIHW.send( (midi::MidiType) msgType, param, value, channel);
    }
    if(config->midiMergeFlags & 0x02){
      MIDI.send(  (midi::MidiType) msgType, param, value, channel);
    }
  }else{
    if(config->midiMergeFlags & 0x04){
      MIDIHW.send( (midi::MidiType) msgType, param, value, channel);
    }
    if(config->midiMergeFlags & 0x08){
      SerialUSB.println("WTF");
      MIDI.send( (midi::MidiType) msgType, param, value, channel);
    }
  }
  
  channel--; // GO from 1-16 to 0-15
   
  uint8_t rcvdEncoderMsgType = 0;
  uint8_t rcvdEncoderSwitchMsgType = 0;
  uint8_t rcvdDigitalMsgType = 0;
  uint8_t rcvdAnalogMsgType = 0;

//  SerialUSB.print(midiSrc ? "MIDI_HW: " : "MIDI_USB: ");
//  SerialUSB.print(msgType); SerialUSB.print("\t");
//  SerialUSB.print(channel); SerialUSB.print("\t");
//  SerialUSB.print(param); SerialUSB.print("\t");
//  SerialUSB.println(value);
  
  switch (msgType) {
    case midi::NoteOn:
    case midi::NoteOff:
      rcvdEncoderMsgType = rotaryMessageTypes::rotary_msg_note;
      rcvdEncoderSwitchMsgType = switchMessageTypes::switch_msg_note;
      rcvdDigitalMsgType = digitalMessageTypes::digital_msg_note;
      rcvdAnalogMsgType = analogMessageTypes::analog_msg_note;
//      SerialUSB.println("I AM NOTE!");
    break;
    case midi::ControlChange:
      // NRPN Message parser
      if (param == midi::NRPNMSB && prevParam != midi::NRPNMSB){
          prevParam = midi::NRPNMSB;
          nrpnMessage.parameterMSB = value;
          nrpnOnGoing = true;
          return;
      }
      else if (param == midi::NRPNLSB && prevParam == midi::NRPNMSB && nrpnOnGoing){
          prevParam = midi::NRPNLSB;
          nrpnMessage.parameterLSB = value;
          nrpnMessage.parameter = nrpnMessage.parameterMSB<<7 | nrpnMessage.parameterLSB;
          return;
      }
      else if (param == midi::DataEntryMSB && prevParam == midi::NRPNLSB && nrpnOnGoing){
          prevParam = midi::DataEntryMSB;
          nrpnMessage.valueMSB = value;
          return;
      }
      else if (param == midi::DataEntryLSB && prevParam == midi::DataEntryMSB && nrpnOnGoing){
          prevParam = 0;
          nrpnMessage.valueLSB = value;
          nrpnMessage.value = nrpnMessage.valueMSB<<7 | nrpnMessage.valueLSB;
          msg14bitComplete = true;
          nrpnOnGoing = false;
          
          rcvdEncoderMsgType = rotaryMessageTypes::rotary_msg_nrpn;
          rcvdEncoderSwitchMsgType = switchMessageTypes::switch_msg_nrpn;
          rcvdDigitalMsgType = digitalMessageTypes::digital_msg_nrpn;
          rcvdAnalogMsgType = analogMessageTypes::analog_msg_nrpn;
//          SerialUSB.print("NRPN MESSAGE COMPLETE -> ");
//          SerialUSB.print("\tPARAM MSB: "); SerialUSB.print(nrpnMessage.parameterMSB);
//          SerialUSB.print("\tPARAM LSB: "); SerialUSB.print(nrpnMessage.parameterLSB);
//          SerialUSB.print("\tPARAM: "); SerialUSB.print(nrpnMessage.parameter);
//          SerialUSB.print("\tVALUE MSB: "); SerialUSB.print(nrpnMessage.valueMSB);
//          SerialUSB.print("\tVALUE LSB: "); SerialUSB.print(nrpnMessage.valueLSB);
//          SerialUSB.print("\tVALUE: "); SerialUSB.println(nrpnMessage.value);
      }
      ////////////////////////////////////////////////////////////////////////////////////
      // RPN Message parser //////////////////////////////////////////////////////////////
      else if (param == midi::RPNMSB && prevParam != midi::RPNMSB){
          prevParam = midi::RPNMSB;
          rpnMessage.parameterMSB = value;
          rpnOnGoing = true;
          return;
      }
      else if (param == midi::RPNLSB && prevParam == midi::RPNMSB && rpnOnGoing){
          prevParam = midi::RPNLSB;
          rpnMessage.parameterLSB = value;
          rpnMessage.parameter = rpnMessage.parameterMSB<<7 | rpnMessage.parameterLSB;
          return;
      }
      else if (param == midi::DataEntryMSB && prevParam == midi::RPNLSB && rpnOnGoing){
          prevParam = midi::DataEntryMSB;
          rpnMessage.valueMSB = value;
          return;
      }
      else if (param == midi::DataEntryLSB && prevParam == midi::DataEntryMSB && rpnOnGoing){
          prevParam = 0;
          rpnMessage.valueLSB = value;
          rpnMessage.value = rpnMessage.valueMSB<<7 | rpnMessage.valueLSB;
          msg14bitComplete = true;
          rpnOnGoing = false;
          
          rcvdEncoderMsgType = rotaryMessageTypes::rotary_msg_rpn;
          rcvdEncoderSwitchMsgType = switchMessageTypes::switch_msg_rpn;
          rcvdDigitalMsgType = digitalMessageTypes::digital_msg_rpn;
          rcvdAnalogMsgType = analogMessageTypes::analog_msg_rpn;
//          SerialUSB.print("RPN MESSAGE COMPLETE -> ");
//          SerialUSB.print("\tPARAM MSB: "); SerialUSB.print(rpnMessage.parameterMSB);
//          SerialUSB.print("\tPARAM LSB: "); SerialUSB.print(rpnMessage.parameterLSB);
//          SerialUSB.print("\tPARAM: "); SerialUSB.print(rpnMessage.parameter);
//          SerialUSB.print("\tVALUE MSB: "); SerialUSB.print(rpnMessage.valueMSB);
//          SerialUSB.print("\tVALUE LSB: "); SerialUSB.print(rpnMessage.valueLSB);
//          SerialUSB.print("\tVALUE: "); SerialUSB.println(rpnMessage.value);
      ////////////////////////////////////////////////////////////////////////////////////
      // Just CC /////////////////////////////////////////////////////////////////////////
      }else{
        rcvdEncoderMsgType = rotaryMessageTypes::rotary_msg_cc;
        rcvdEncoderSwitchMsgType = switchMessageTypes::switch_msg_cc;
        rcvdDigitalMsgType = digitalMessageTypes::digital_msg_cc;
        rcvdAnalogMsgType = analogMessageTypes::analog_msg_cc;
        nrpnOnGoing = false;
        rpnOnGoing = false;
        msg14bitComplete = false;
        prevParam = 0;
      }
    break;
    case midi::PitchBend:{
      rcvdEncoderMsgType = rotaryMessageTypes::rotary_msg_pb;
      rcvdEncoderSwitchMsgType = switchMessageTypes::switch_msg_pb;
      rcvdDigitalMsgType = digitalMessageTypes::digital_msg_pb;
      rcvdAnalogMsgType = analogMessageTypes::analog_msg_pb;
    }
    break;
  }
  
//  
  
  if (msg14bitComplete){
     msg14bitComplete = false;
    if( rcvdEncoderMsgType == rotaryMessageTypes::rotary_msg_nrpn || 
        rcvdEncoderSwitchMsgType == switchMessageTypes::switch_msg_nrpn || 
        rcvdDigitalMsgType == digitalMessageTypes::digital_msg_nrpn || 
        rcvdAnalogMsgType == analogMessageTypes::analog_msg_nrpn){
          
      param = nrpnMessage.parameter;
      value = nrpnMessage.value;
//      SerialUSB.print("NRPN MESSAGE COMPLETE -> ");
//      SerialUSB.print("\tPARAM: "); SerialUSB.print(param);
//      SerialUSB.print("\tVALUE: "); SerialUSB.println(value);
    }else if( rcvdEncoderMsgType == rotaryMessageTypes::rotary_msg_rpn || 
              rcvdEncoderSwitchMsgType == switchMessageTypes::switch_msg_rpn || 
              rcvdDigitalMsgType == digitalMessageTypes::digital_msg_rpn || 
              rcvdAnalogMsgType == analogMessageTypes::analog_msg_rpn){
                
      param = rpnMessage.parameter;
      value = rpnMessage.value;
//      SerialUSB.print("RPN MESSAGE COMPLETE -> ");
//      SerialUSB.print("\tPARAM: "); SerialUSB.print(param);
//      SerialUSB.print("\tVALUE: "); SerialUSB.println(value);
    }
      
  }
//      SerialUSB.print("\tPARAM: "); SerialUSB.print(param);
//      SerialUSB.print("\tVALUE: "); SerialUSB.println(value);
   
  for(uint8_t bank = 0; bank < config->banks.count; bank++){
   // SWEEP ALL ENCODERS - // FIX FOR SHIFT ROTARY ACTION AND CHANGE ROTARY CONFIG FOR ROTARY FEEDBACK IN ALL CASES
    for(uint8_t encNo = 0; encNo < config->inputs.encoderCount; encNo++){
      
      //if(bank != currentBank) memHost->LoadBankSingleSection(bank, ytxIOBLOCK::Encoder, encNo);
      
//      if((encoder[encNo].rotaryFeedback.midiPort&0x01) && !midiSrc || (encoder[encNo].rotaryConfig.midiPort&0x02 && midiSrc)){
      if((encoder[encNo].rotaryFeedback.source&0x01) && !midiSrc || (encoder[encNo].rotaryFeedback.source&0x02 && midiSrc)){
        if(encoder[encNo].rotaryFeedback.channel == channel){
          if(encoder[encNo].rotaryFeedback.message == rcvdEncoderMsgType){
            if( encoder[encNo].rotaryFeedback.parameterLSB == param || 
                rcvdEncoderMsgType == rotaryMessageTypes::rotary_msg_pb){
              // If there's a match, set encoder value and feedback
              
//              encoderHw.SetEncoderValue(bank, encNo, value);
              
            }
          }
        }
      }
    
    // SWEEP ALL ENCODERS SWITCHES
    
      if((encoder[encNo].switchFeedback.source&0x01) && !midiSrc || (encoder[encNo].switchFeedback.source&0x02 && midiSrc)){
        if(encoder[encNo].switchFeedback.channel == channel){
          if(encoder[encNo].switchFeedback.message == rcvdEncoderSwitchMsgType){
            if(encoder[encNo].switchFeedback.parameterLSB == param){
              // If there's a match, set encoder value and feedback
//              if(msgType == midi::NoteOff) value = 0;
//              encoderHw.SetEncoderSwitchValue(bank, encNo, value);
            }
          }
        }
      }
    }
    // SWEEP ALL DIGITAL
    for(uint16_t digNo = 0; digNo < config->inputs.digitalCount; digNo++){
      if((digital[digNo].feedback.source&0x01) && !midiSrc || (digital[digNo].feedback.source&0x02 && midiSrc)){
        if(digital[digNo].feedback.channel == channel){
          if(digital[digNo].feedback.message == rcvdDigitalMsgType){
            if(digital[digNo].feedback.parameterLSB == param){
              // If there's a match, set encoder value and feedback
              //if(msgType == midi::NoteOff) value = 0;
//              digitalHw.SetDigitalValue(bank, digNo, value);
            }
          }
        }
      }
    }
    // SWEEP ALL ANALOG
    for(uint8_t analogNo = 0; analogNo < config->inputs.analogCount; analogNo++){
       
    }
  }
}
