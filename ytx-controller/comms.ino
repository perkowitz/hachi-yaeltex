// 14 bit message data types and declarations
typedef struct{
  uint16_t parameter;  
  uint16_t value;
  uint8_t parameterMSB;  
  uint8_t parameterLSB;  
  uint8_t valueMSB;
  uint8_t valueLSB;
} ytx14bitMessage;

ytx14bitMessage nrpnMessage;
ytx14bitMessage rpnMessage;

uint8_t rcvdEncoderMsgType = 0;
uint8_t rcvdEncoderSwitchMsgType = 0;
uint8_t rcvdDigitalMsgType = 0;
uint8_t rcvdAnalogMsgType = 0;  

bool msg14bitComplete = false;

//----------------------------------------------------------------------------------------------------
// COMMS MIDI - SERIAL
//----------------------------------------------------------------------------------------------------
/*
 * Handler for Note On messages received from USB port
 */
void handleNoteOnUSB(byte channel, byte note, byte velocity){
  uint8_t msgType = MIDI.getType();
  rcvdEncoderMsgType = rotaryMessageTypes::rotary_msg_note;
  rcvdEncoderSwitchMsgType = switchMessageTypes::switch_msg_note;
  rcvdDigitalMsgType = digitalMessageTypes::digital_msg_note;
  rcvdAnalogMsgType = analogMessageTypes::analog_msg_note;
//  SerialUSB.println("LLEGO NOTE ON POR USB");
  ProcessMidi(msgType, channel, note, velocity, MIDI_USB);
//      SerialUSB.println("I AM NOTE!");
}
/*
 * Handler for Note Off messages received from USB port
 */
void handleNoteOffUSB(byte channel, byte note, byte velocity){
  uint8_t msgType = MIDI.getType();
  rcvdEncoderMsgType = rotaryMessageTypes::rotary_msg_note;
  rcvdEncoderSwitchMsgType = switchMessageTypes::switch_msg_note;
  rcvdDigitalMsgType = digitalMessageTypes::digital_msg_note;
  rcvdAnalogMsgType = analogMessageTypes::analog_msg_note;
//  SerialUSB.println("LLEGO NOTE OFF POR USB");
  ProcessMidi(msgType, channel, note, velocity, MIDI_USB);
}
/*
 * Handler for CC messages received from USB port
 */
void handleControlChangeUSB(byte channel, byte number, byte value){
  uint8_t msgType = MIDI.getType();
  uint16_t fullParam = 0, fullValue = 0;
//  SerialUSB.println("LLEGO CC POR USB");
  msg14bitParser(channel, number, value);

  if (msg14bitComplete){
     msg14bitComplete = false;
    if( rcvdEncoderMsgType == rotaryMessageTypes::rotary_msg_nrpn || 
        rcvdEncoderSwitchMsgType == switchMessageTypes::switch_msg_nrpn || 
        rcvdDigitalMsgType == digitalMessageTypes::digital_msg_nrpn || 
        rcvdAnalogMsgType == analogMessageTypes::analog_msg_nrpn){
          
      fullParam = nrpnMessage.parameter;
      fullValue = nrpnMessage.value;
//      SerialUSB.print("NRPN MESSAGE COMPLETE -> ");
//      SerialUSB.print("\tPARAM: "); SerialUSB.print(fullParam);
//      SerialUSB.print("\tVALUE: "); SerialUSB.println(fullValue);
    }else if( rcvdEncoderMsgType == rotaryMessageTypes::rotary_msg_rpn || 
              rcvdEncoderSwitchMsgType == switchMessageTypes::switch_msg_rpn || 
              rcvdDigitalMsgType == digitalMessageTypes::digital_msg_rpn || 
              rcvdAnalogMsgType == analogMessageTypes::analog_msg_rpn){
                
      fullParam = rpnMessage.parameter;
      fullValue = rpnMessage.value;
//      SerialUSB.print("RPN MESSAGE COMPLETE -> ");
//      SerialUSB.print("\tPARAM: "); SerialUSB.print(fullParam);
//      SerialUSB.print("\tVALUE: "); SerialUSB.println(fullValue);
    }   
    ProcessMidi(msgType, channel, fullParam, fullValue, MIDI_USB);
  }else{
    ProcessMidi(msgType, channel, number, value, MIDI_USB); 
  }  
}

/*
 * Handler for Pitch Bend messages received from USB port
 */
void handlePitchBendUSB(byte channel, int bend){
  uint8_t msgType = MIDI.getType();
//  SerialUSB.println("LLEGO PITCH BEND POR USB");
  rcvdEncoderMsgType = rotaryMessageTypes::rotary_msg_pb;
  rcvdEncoderSwitchMsgType = switchMessageTypes::switch_msg_pb;
  rcvdDigitalMsgType = digitalMessageTypes::digital_msg_pb;
  rcvdAnalogMsgType = analogMessageTypes::analog_msg_pb;

  ProcessMidi(msgType, channel, 0, bend, MIDI_USB);
}
/*
 * Handler for Note On messages received from DIN5 port
 */
void handleNoteOnHW(byte channel, byte note, byte velocity){
  uint8_t msgType = MIDIHW.getType();
//  SerialUSB.println("LLEGO NOTE ON POR HW");
  rcvdEncoderMsgType = rotaryMessageTypes::rotary_msg_note;
  rcvdEncoderSwitchMsgType = switchMessageTypes::switch_msg_note;
  rcvdDigitalMsgType = digitalMessageTypes::digital_msg_note;
  rcvdAnalogMsgType = analogMessageTypes::analog_msg_note;

  ProcessMidi(msgType, channel, note, velocity, MIDI_HW);
//      SerialUSB.println("I AM NOTE!");
}

/*
 * Handler for Note Off messages received from DIN5 port
 */
void handleNoteOffHW(byte channel, byte note, byte velocity){
  uint8_t msgType = MIDIHW.getType();
//  SerialUSB.println("LLEGO NOTE OFF POR HW");
  rcvdEncoderMsgType = rotaryMessageTypes::rotary_msg_note;
  rcvdEncoderSwitchMsgType = switchMessageTypes::switch_msg_note;
  rcvdDigitalMsgType = digitalMessageTypes::digital_msg_note;
  rcvdAnalogMsgType = analogMessageTypes::analog_msg_note;

  ProcessMidi(msgType, channel, note, velocity, MIDI_HW);
}

/*
 * Handler for CC messages received from DIN5 port
 */
void handleControlChangeHW(byte channel, byte number, byte value){
  uint8_t msgType = MIDIHW.getType();
  uint16_t fullParam = 0, fullValue = 0;
  //SerialUSB.println("LLEGO CC POR HW");
  msg14bitParser(channel, number, value);

  if (msg14bitComplete){
     msg14bitComplete = false;
    if( rcvdEncoderMsgType == rotaryMessageTypes::rotary_msg_nrpn || 
        rcvdEncoderSwitchMsgType == switchMessageTypes::switch_msg_nrpn || 
        rcvdDigitalMsgType == digitalMessageTypes::digital_msg_nrpn || 
        rcvdAnalogMsgType == analogMessageTypes::analog_msg_nrpn){
          
      fullParam = nrpnMessage.parameter;
      fullValue = nrpnMessage.value;
//      SerialUSB.print("NRPN MESSAGE COMPLETE -> ");
//      SerialUSB.print("\tPARAM: "); SerialUSB.print(fullParam);
//      SerialUSB.print("\tVALUE: "); SerialUSB.println(fullValue);
    }else if( rcvdEncoderMsgType == rotaryMessageTypes::rotary_msg_rpn || 
              rcvdEncoderSwitchMsgType == switchMessageTypes::switch_msg_rpn || 
              rcvdDigitalMsgType == digitalMessageTypes::digital_msg_rpn || 
              rcvdAnalogMsgType == analogMessageTypes::analog_msg_rpn){
                
      fullParam = rpnMessage.parameter;
      fullValue = rpnMessage.value;
//      SerialUSB.print("RPN MESSAGE COMPLETE -> ");
//      SerialUSB.print("\tPARAM: "); SerialUSB.print(fullParam);
//      SerialUSB.print("\tVALUE: "); SerialUSB.println(fullValue);
    }   
    ProcessMidi(msgType, channel, fullParam, fullValue, MIDI_HW);
  }else{
    ProcessMidi(msgType, channel, number, value, MIDI_HW); 
  }  
}

/*
 * Handler for Pitch Bend messages received from DIN5 port
 */
void handlePitchBendHW(byte channel, int bend){
  uint8_t msgType = MIDIHW.getType();
//  SerialUSB.println("LLEGO PITCH BEND POR HW");
  rcvdEncoderMsgType = rotaryMessageTypes::rotary_msg_pb;
  rcvdEncoderSwitchMsgType = switchMessageTypes::switch_msg_pb;
  rcvdDigitalMsgType = digitalMessageTypes::digital_msg_pb;
  rcvdAnalogMsgType = analogMessageTypes::analog_msg_pb;

  ProcessMidi(msgType, channel, 0, bend, MIDI_HW);
}

/*
 * NRPN/RPN parser
 * Take current CC message and decide if it is part of a NRPN or RPN message
 * Return 14 bit message struct properly filled with corresponding flag if it's been detected
 */
void msg14bitParser(byte channel, byte param, byte value){
  static uint8_t prevParam = 0;
  static uint32_t prevMillisNRPN = 0;
  static bool nrpnOnGoing = false;
  static bool rpnOnGoing = false;
  
  // NRPN Message parser
  if (param == midi::NRPNMSB && prevParam != midi::NRPNMSB){    // If new CC arrives 
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
}

void ProcessMidi(byte msgType, byte channel, uint16_t param, uint16_t value, bool midiSrc) {
 //uint32_t antMicrosComms = micros(); 
  // MIDI THRU
  if(midiSrc){  // IN FROM MIDI HW
    if(config->midiMergeFlags & 0x01){    // Send to MIDI DIN port
      MIDIHW.send( (midi::MidiType) msgType, param, value, channel);
    }
    if(config->midiMergeFlags & 0x02){    // Send to MIDI USB port
      MIDI.send(  (midi::MidiType) msgType, param, value, channel);
    }
  }else{        // IN FROM MIDI USB
    if(config->midiMergeFlags & 0x04){    // Send to MIDI DIN port
      MIDIHW.send( (midi::MidiType) msgType, param, value, channel);
    }
    if(config->midiMergeFlags & 0x08){    // Send to MIDI USB port
      SerialUSB.println("WTF");
      MIDI.send( (midi::MidiType) msgType, param, value, channel);
    }
  }
  
  channel--; // GO from 1-16 to 0-15
   
//  SerialUSB.print(midiSrc ? "MIDI_HW: " : "MIDI_USB: ");
//  SerialUSB.print(msgType); SerialUSB.print("\t");
//  SerialUSB.print(channel); SerialUSB.print("\t");
//  SerialUSB.print(param); SerialUSB.print("\t");
//  SerialUSB.println(value);
  
  // Blink status LED when receiving MIDI message
  SetStatusLED(STATUS_BLINK, 1, statusLEDtypes::STATUS_FB_MIDI_IN);

  // TEST SELF RESET WITH MIDI MESSAGE
//  if(msgType == midi::NoteOn && param == 127 && value == 127){
//    char bootSign = 'Y';
//    eep.write(BOOT_SIGN_ADDR, (byte*)(&bootSign), sizeof(bootSign));
//    SelfReset();
//  }
   
 // If it is a regular message, check if it matches the feedback configuration for all the inputs (only the current bank)
 // SWEEP ALL ENCODERS - // FIX FOR SHIFT ROTARY ACTION AND CHANGE ROTARY CONFIG FOR ROTARY FEEDBACK IN ALL CASES
  for(uint8_t encNo = 0; encNo < config->inputs.encoderCount; encNo++){
    if((encoder[encNo].rotaryFeedback.source&0x01) && !midiSrc || (encoder[encNo].rotaryFeedback.source&0x02 && midiSrc)){
      if(encoder[encNo].rotaryFeedback.channel == channel){
        if(encoder[encNo].rotaryFeedback.message == rcvdEncoderMsgType){
          if( encoder[encNo].rotaryFeedback.parameterLSB == param || 
              rcvdEncoderMsgType == rotaryMessageTypes::rotary_msg_pb){
            // If there's a match, set encoder value and feedback
            encoderHw.SetEncoderValue(currentBank, encNo, value);
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
            if(msgType == midi::NoteOff) value = 0;
            encoderHw.SetEncoderSwitchValue(currentBank, encNo, value);
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
            if(msgType == midi::NoteOff) value = 0;
            digitalHw.SetDigitalValue(currentBank, digNo, value);
          }
        }
      }
    }
  }
  // SWEEP ALL ANALOG
//  for(uint8_t analogNo = 0; analogNo < config->inputs.analogCount; analogNo++){
//     // analog has no feedback yet
//  }
}
