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
    
    msg14bitComplete = false;
  }else{
    rcvdEncoderMsgType = rotaryMessageTypes::rotary_msg_cc;
    rcvdEncoderSwitchMsgType = switchMessageTypes::switch_msg_cc;
    rcvdDigitalMsgType = digitalMessageTypes::digital_msg_cc;
    rcvdAnalogMsgType = analogMessageTypes::analog_msg_cc;
    
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
  msg14bitComplete = true;
  ProcessMidi(msgType, channel, 0, bend, MIDI_USB);
  msg14bitComplete = false;
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
    msg14bitComplete = false;
  }else{
    rcvdEncoderMsgType = rotaryMessageTypes::rotary_msg_cc;
    rcvdEncoderSwitchMsgType = switchMessageTypes::switch_msg_cc;
    rcvdDigitalMsgType = digitalMessageTypes::digital_msg_cc;
    rcvdAnalogMsgType = analogMessageTypes::analog_msg_cc;
    
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
  
  msg14bitComplete = true;
  ProcessMidi(msgType, channel, 0, bend, MIDI_HW);
  msg14bitComplete = false;
}

/*
 * NRPN/RPN parser
 * Take current CC message and decide if it is part of a NRPN or RPN message
 * Fills 14 bit message struct properly with corresponding flag if it's been detected
 * Returns if ther is and on going NRPN or RPN message being decoded
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
      decoding14bit = true;
//      SerialUSB.println("1° BYTE");
      return;
  }
  else if (param == midi::NRPNLSB && prevParam == midi::NRPNMSB && nrpnOnGoing){
      prevParam = midi::NRPNLSB;
      nrpnMessage.parameterLSB = value;
      nrpnMessage.parameter = nrpnMessage.parameterMSB<<7 | nrpnMessage.parameterLSB;
//      SerialUSB.println("2° BYTE");
      return;
  }
  else if (param == midi::DataEntryMSB && prevParam == midi::NRPNLSB && nrpnOnGoing){
      prevParam = midi::DataEntryMSB;
      nrpnMessage.valueMSB = value;
//      SerialUSB.println("3° BYTE");
      return;
  }
  else if (param == midi::DataEntryLSB && prevParam == midi::DataEntryMSB && nrpnOnGoing){
      prevParam = 0;
      nrpnMessage.valueLSB = value;
      nrpnMessage.value = nrpnMessage.valueMSB<<7 | nrpnMessage.valueLSB;
      msg14bitComplete = true;
      nrpnOnGoing = false;
      decoding14bit = false;
//      SerialUSB.println("4° BYTE");
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
      decoding14bit = true;
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
      decoding14bit = false;
      
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
    decoding14bit = false;
    msg14bitComplete = false;
    prevParam = 0;
  }
}

void ProcessMidi(byte msgType, byte channel, uint16_t param, uint16_t value, bool midiSrc) {
 //uint32_t antMicrosComms = micros(); 
  // MIDI THRU
  if(midiSrc){  // IN FROM MIDI HW
    if(config->midiConfig.midiMergeFlags & 0x01){    // Send to MIDI DIN port
      MIDIHW.send( (midi::MidiType) msgType, param, value, channel);
    }
    if(config->midiConfig.midiMergeFlags & 0x02){    // Send to MIDI USB port
      MIDI.send(  (midi::MidiType) msgType, param, value, channel);
    }
  }else{        // IN FROM MIDI USB
    if(config->midiConfig.midiMergeFlags & 0x04){    // Send to MIDI DIN port
      MIDIHW.send( (midi::MidiType) msgType, param, value, channel);
    }
    if(config->midiConfig.midiMergeFlags & 0x08){    // Send to MIDI USB port
      MIDI.send( (midi::MidiType) msgType, param, value, channel);
    }
  }

  if(decoding14bit) return;
  
  channel--; // GO from 1-16 to 0-15
   
//  SerialUSB.print(midiSrc ? "MIDI_HW: " : "MIDI_USB: ");
//  SerialUSB.print(msgType, HEX); SerialUSB.print("\t");
//  SerialUSB.print(channel); SerialUSB.print("\t");
//  SerialUSB.print(param); SerialUSB.print("\t");
//  SerialUSB.println(value);
  
  // Blink status LED when receiving MIDI message
  SetStatusLED(STATUS_BLINK, 1, statusLEDtypes::STATUS_FB_MIDI_IN);

  // TEST SELF RESET WITH MIDI MESSAGE
  if(msgType == midi::NoteOn && param == 127 && value == 127){
    char bootSign = 'Y';
    eep.write(BOOT_SIGN_ADDR, (byte*)(&bootSign), sizeof(bootSign));
    SelfReset();
  }
  
  // Set NOTES OFF as NOTES ON for the next section to treat them as the same
  if(msgType == 0x90 || msgType == 0x80){
    msgType = 0x90;      
  }else if(msg14bitComplete){
    if(rcvdEncoderMsgType == rotaryMessageTypes::rotary_msg_nrpn ||
       rcvdEncoderSwitchMsgType == switchMessageTypes::switch_msg_nrpn ||
       rcvdDigitalMsgType == digitalMessageTypes::digital_msg_nrpn ||
       rcvdAnalogMsgType == analogMessageTypes::analog_msg_nrpn){
      msgType = 0x60;     // USE CUSTOM TYPE FOR NRPN     
    }else if(rcvdEncoderMsgType == rotaryMessageTypes::rotary_msg_rpn ||
             rcvdEncoderSwitchMsgType == switchMessageTypes::switch_msg_rpn ||
             rcvdDigitalMsgType == digitalMessageTypes::digital_msg_rpn ||
             rcvdAnalogMsgType == analogMessageTypes::analog_msg_rpn){
      msgType = 0x70;     // USE CUSTOM TYPE FOR RPN 
    }
  }

//  SerialUSB.print(midiSrc ? "MIDI_HW: " : "MIDI_USB: ");
//  SerialUSB.print(msgType, HEX); SerialUSB.print("\t");
//  SerialUSB.print(channel); SerialUSB.print("\t");
//  SerialUSB.print(param); SerialUSB.print("\t");
//  SerialUSB.println(value);
  
  CheckAllAndUpdate(msgType, channel, param, value, midiSrc);
  
  UpdateMidiBuffer(msgType, channel, param, value, midiSrc);
  
  // RESET VALUES
  rcvdEncoderMsgType = 0;
  rcvdEncoderSwitchMsgType = 0;
  rcvdDigitalMsgType = 0;
  rcvdAnalogMsgType = 0;  
}


void CheckAllAndUpdate(byte msgType, byte channel, uint16_t param, uint16_t value, bool midiSrc){
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
            //if(msgType == midi::NoteOff) value = 0;
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
            //if(msgType == midi::NoteOff) value = 0;
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


void UpdateMidiBuffer(byte msgType, byte channel, uint16_t param, uint16_t value, bool midiSrc){
  bool thereIsAMatch = false;

  if(!msg14bitComplete){  // IF IT'S A 7 BIT MESSAGE
    for(uint32_t idx = 0; idx < midiRxSettings.lastMidiBufferIndex7; idx++){
      if(midiMsgBuf7[idx].port == midiSrc){
        if(midiMsgBuf7[idx].channel == channel){
          if(midiMsgBuf7[idx].message == (msgType>>4)&0x0F){
            if(midiMsgBuf7[idx].parameter == param){
//              SerialUSB.println("MIDI MESSAGE ALREADY IN 7 BIT BUFFER, UPDATED");
              midiMsgBuf7[idx].value = value;
              thereIsAMatch = true;
            }
          }
        }
      }
    }
    if (midiRxSettings.lastMidiBufferIndex7 < midiRxSettings.midiBufferSize7 && !thereIsAMatch && (midiRxSettings.listenToChannel>>channel)&0x1){
//      SerialUSB.println("NEW MIDI MESSAGE ADDED TO 7 BIT BUFFER");
      
      midiMsgBuf7[midiRxSettings.lastMidiBufferIndex7].port = midiSrc;
      midiMsgBuf7[midiRxSettings.lastMidiBufferIndex7].channel = channel;
      midiMsgBuf7[midiRxSettings.lastMidiBufferIndex7].message = (msgType>>4)&0x0F;
      midiMsgBuf7[midiRxSettings.lastMidiBufferIndex7].parameter = param;
      midiMsgBuf7[midiRxSettings.lastMidiBufferIndex7].value = value;  
//      SerialUSB.print(midiMsgBuf7[midiRxSettings.lastMidiBufferIndex7].port ? "MIDI_HW: " : "MIDI_USB: ");
//      SerialUSB.print(midiMsgBuf7[midiRxSettings.lastMidiBufferIndex7].message, HEX); SerialUSB.print("\t");
//      SerialUSB.print(midiMsgBuf7[midiRxSettings.lastMidiBufferIndex7].channel); SerialUSB.print("\t");
//      SerialUSB.print(midiMsgBuf7[midiRxSettings.lastMidiBufferIndex7].parameter); SerialUSB.print("\t");
//      SerialUSB.println(value);
      midiRxSettings.lastMidiBufferIndex7++;
    }else if(midiRxSettings.lastMidiBufferIndex7 == midiRxSettings.midiBufferSize7){
//      SerialUSB.println("7BIT MIDI BUFFER FULL"); 
    }
  }else{
    for(uint32_t idx = 0; idx < midiRxSettings.lastMidiBufferIndex14; idx++){
      if(midiMsgBuf14[idx].port == midiSrc){
        if(midiMsgBuf14[idx].channel == channel){
          if(midiMsgBuf14[idx].message == (msgType>>4)&0x0F){
            if(midiMsgBuf14[idx].parameter == param){
              midiMsgBuf14[idx].value = value;
//              SerialUSB.println("MIDI MESSAGE ALREADY IN 14 BIT BUFFER, UPDATED");
              thereIsAMatch = true;
            }
          }
        }
      }
    }
    
    if (midiRxSettings.lastMidiBufferIndex14 < midiRxSettings.midiBufferSize14 && !thereIsAMatch && (midiRxSettings.listenToChannel>>channel)&0x1 ){
//      SerialUSB.println("NEW MIDI MESSAGE ADDED TO 14 BIT BUFFER");
      midiMsgBuf14[midiRxSettings.lastMidiBufferIndex14].port = midiSrc;
      midiMsgBuf14[midiRxSettings.lastMidiBufferIndex14].channel = channel;
      midiMsgBuf14[midiRxSettings.lastMidiBufferIndex14].message = (msgType>>4)&0x0F;
      midiMsgBuf14[midiRxSettings.lastMidiBufferIndex14].parameter = param;
      midiMsgBuf14[midiRxSettings.lastMidiBufferIndex14].value = value;  
      
      midiRxSettings.lastMidiBufferIndex14++;
    }else if (midiRxSettings.lastMidiBufferIndex14 == midiRxSettings.midiBufferSize14){
//      SerialUSB.println("14 BIT MIDI BUFFER FULL"); 
    }
  }   
}
