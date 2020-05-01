/*

Author/s: Franco Grassano - Franco Zaccra

MIT License

Copyright (c) 2020 - Yaeltex

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

*/

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

  antMicrosCC = micros();

  
  uint8_t msgType = MIDI.getType();
  uint16_t fullParam = 0, fullValue = 0;
  msg14bitParser(channel, number, value);
  
  if (msg14bitComplete){
    if( rcvdEncoderMsgType == rotaryMessageTypes::rotary_msg_nrpn || 
        rcvdEncoderSwitchMsgType == switchMessageTypes::switch_msg_nrpn || 
        rcvdDigitalMsgType == digitalMessageTypes::digital_msg_nrpn || 
        rcvdAnalogMsgType == analogMessageTypes::analog_msg_nrpn){
          
      fullParam = nrpnMessage.parameter;
      fullValue = nrpnMessage.value;
//      SerialUSB.println();
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
 * Handler for Program Change messages received from USB port
 */
void handleProgramChangeUSB(byte channel, byte number){
  uint8_t msgType = MIDI.getType();
  // SerialUSB.println("LLEGO PC POR USB");
  rcvdEncoderMsgType = rotaryMessageTypes::rotary_msg_pc_rel;
  rcvdEncoderSwitchMsgType = switchMessageTypes::switch_msg_pc;
  rcvdDigitalMsgType = digitalMessageTypes::digital_msg_pc;
  rcvdAnalogMsgType = analogMessageTypes::analog_msg_pc;

  ProcessMidi(msgType, channel, 0, number, MIDI_USB);
}

/*
 * Handler for Pitch Bend messages received from USB port
 */
void handlePitchBendUSB(byte channel, int bend){
  uint8_t msgType = MIDI.getType();
  rcvdEncoderMsgType = rotaryMessageTypes::rotary_msg_pb;
  rcvdEncoderSwitchMsgType = switchMessageTypes::switch_msg_pb;
  rcvdDigitalMsgType = digitalMessageTypes::digital_msg_pb;
  rcvdAnalogMsgType = analogMessageTypes::analog_msg_pb;
  // SerialUSB.println("PB ARRIVED!");
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
 * Handler for Program Change messages received from DIN5 port
 */
void handleProgramChangeHW(byte channel, byte number){
  uint8_t msgType = MIDIHW.getType();
//  SerialUSB.println("LLEGO NOTE ON POR HW");
  rcvdEncoderMsgType = rotaryMessageTypes::rotary_msg_pc_rel;
  rcvdEncoderSwitchMsgType = switchMessageTypes::switch_msg_pc;
  rcvdDigitalMsgType = digitalMessageTypes::digital_msg_pc;
  rcvdAnalogMsgType = analogMessageTypes::analog_msg_pc;

  ProcessMidi(msgType, channel, 0, number, MIDI_HW);
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
//      SerialUSB.println("1� BYTE");
      return;
  }
  else if (param == midi::NRPNLSB && prevParam == midi::NRPNMSB && nrpnOnGoing){
      prevParam = midi::NRPNLSB;
      nrpnMessage.parameterLSB = value;
      nrpnMessage.parameter = nrpnMessage.parameterMSB<<7 | nrpnMessage.parameterLSB;
//      SerialUSB.println("2� BYTE");
      return;
  }
  else if (param == midi::DataEntryMSB && prevParam == midi::NRPNLSB && nrpnOnGoing){
      prevParam = midi::DataEntryMSB;
      nrpnMessage.valueMSB = value;
//      SerialUSB.println("3� BYTE");
      return;
  }
  else if (param == midi::DataEntryLSB && prevParam == midi::DataEntryMSB && nrpnOnGoing){
      prevParam = 0;
      nrpnMessage.valueLSB = value;
      nrpnMessage.value = nrpnMessage.valueMSB<<7 | nrpnMessage.valueLSB;
      msg14bitComplete = true;
      nrpnOnGoing = false;
      decoding14bit = false;
//      SerialUSB.println("4� BYTE");
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


void ProcessMidi(byte msgType, byte channel, uint16_t param, int16_t value, bool midiSrc) {
 //uint32_t antMicrosComms = micros(); 
  // MIDI THRU
  if(midiSrc){  // IN FROM MIDI HW
    if(config->midiConfig.midiMergeFlags & 0x01){    // Send to MIDI DIN port
      MIDI.send( (midi::MidiType) msgType, param, value, channel);
    }
    if(config->midiConfig.midiMergeFlags & 0x02){    // Send to MIDI USB port
      MIDIHW.send( (midi::MidiType) msgType, param, value, channel);
    }
  }else{        // IN FROM MIDI USB
    if(config->midiConfig.midiMergeFlags & 0x04){    // Send to MIDI DIN port
      MIDI.send(  (midi::MidiType) msgType, param, value, channel);
    }
    if(config->midiConfig.midiMergeFlags & 0x08){    // Send to MIDI USB port
      MIDIHW.send( (midi::MidiType) msgType, param, value, channel);
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
  SetStatusLED(STATUS_BLINK, 1, statusLEDtypes::STATUS_FB_MSG_IN);

  // // TEST SELF RESET WITH MIDI MESSAGE
  // if(msgType == midi::NoteOn && param == 127 && value == 127){
  //   config->board.bootFlag = 1;                                            
  //   byte bootFlagState = 0;
  //   eep.read(BOOT_SIGN_ADDR, (byte *) &bootFlagState, sizeof(bootFlagState));
  //   bootFlagState |= 1;
  //   eep.write(BOOT_SIGN_ADDR, (byte *) &bootFlagState, sizeof(bootFlagState));

  //   SelfReset();
  // }
  
  // Set NOTES OFF as NOTES ON for the next section to treat them as the same
  if( msgType == midi::NoteOff){
    msgType = midi::NoteOn;      
    value = 0;
  }

  // Convert signed PB value (-8192..8191) to unsigned (0..16383)
  uint16_t unsignedValue = 0;
  if(msg14bitComplete){
    if(rcvdEncoderMsgType == rotaryMessageTypes::rotary_msg_nrpn ||
       rcvdEncoderSwitchMsgType == switchMessageTypes::switch_msg_nrpn ||
       rcvdDigitalMsgType == digitalMessageTypes::digital_msg_nrpn ||
       rcvdAnalogMsgType == analogMessageTypes::analog_msg_nrpn){
      msgType = MidiTypeYTX::NRPN;     // USE CUSTOM TYPE FOR NRPN     
    }else if(rcvdEncoderMsgType == rotaryMessageTypes::rotary_msg_rpn ||
             rcvdEncoderSwitchMsgType == switchMessageTypes::switch_msg_rpn ||
             rcvdDigitalMsgType == digitalMessageTypes::digital_msg_rpn ||
             rcvdAnalogMsgType == analogMessageTypes::analog_msg_rpn){
      msgType = MidiTypeYTX::RPN;     // USE CUSTOM TYPE FOR RPN 
    }else if(rcvdEncoderMsgType == rotaryMessageTypes::rotary_msg_pb ||
             rcvdEncoderSwitchMsgType == switchMessageTypes::switch_msg_pb ||
             rcvdDigitalMsgType == digitalMessageTypes::digital_msg_pb ||
             rcvdAnalogMsgType == analogMessageTypes::analog_msg_pb){
      msgType = MidiTypeYTX::PitchBend; 
      unsignedValue = (uint16_t) value + 8192; // Correct value
      param = 0;
    }
  }else{
    msgType = (msgType >> 4) & 0x0F;    // Convert to YTX midi type
    unsignedValue = (uint16_t) value;
  }  
  
  SerialUSB.print(midiSrc ? "MIDI_HW: " : "MIDI_USB: ");
  SerialUSB.print(msgType, HEX); SerialUSB.print("\t");
  SerialUSB.print(channel); SerialUSB.print("\t");
  SerialUSB.print(param); SerialUSB.print("\t");
  SerialUSB.println(unsignedValue);
  
  UpdateMidiBuffer(FB_ENCODER, msgType, channel, param, unsignedValue, midiSrc);
  UpdateMidiBuffer(FB_ENC_VUMETER, msgType, channel, param, unsignedValue, midiSrc);
  UpdateMidiBuffer(FB_ENCODER_SWITCH, msgType, channel, param, unsignedValue, midiSrc);
  UpdateMidiBuffer(FB_DIGITAL, msgType, channel, param, unsignedValue, midiSrc);
  UpdateMidiBuffer(FB_ANALOG, msgType, channel, param, unsignedValue, midiSrc);
  UpdateMidiBuffer(FB_2CC, msgType, channel, param, unsignedValue, midiSrc);
  UpdateMidiBuffer(FB_SHIFT, msgType, channel, param, unsignedValue, midiSrc);
  
  // SerialUSB.println(micros()-antMicrosCC);
  // RESET VALUES
  rcvdEncoderMsgType = 0;
  rcvdEncoderSwitchMsgType = 0;
  rcvdDigitalMsgType = 0;
  rcvdAnalogMsgType = 0;  
  
  
}

void UpdateMidiBuffer(byte fbType, byte msgType, byte channel, uint16_t param, uint16_t value, bool midiSrc){
  bool thereIsAMatch = false;
  
  if(!msg14bitComplete){  // IF IT'S A 7 BIT MESSAGE
    // BOUNDARY BINARY SEARCH - Search lower bound and upper bound for a single parameter - Buffer is sorted based on parameter numbers
    int lowerB = lower_bound_search7(midiMsgBuf7, param, 0, midiRxSettings.lastMidiBufferIndex7);
    int upperB = upper_bound_search7(midiMsgBuf7, param, 0, midiRxSettings.lastMidiBufferIndex7);

    if((lowerB < 0) || (upperB < 0)) return;  // if any of the boundaries are negative, return, since parameter wasn't found
    
    for(uint32_t idx = lowerB; idx < upperB; idx++){
        if(midiMsgBuf7[idx].channel == channel){
          if(midiMsgBuf7[idx].message == msgType){
            if(midiMsgBuf7[idx].type == fbType){
              if(midiMsgBuf7[idx].port & (1 << midiSrc)){
                midiMsgBuf7[idx].value = value;
                midiMsgBuf7[idx].banksToUpdate = midiMsgBuf7[idx].banksPresent;
                if((midiMsgBuf7[idx].banksToUpdate >> currentBank) & 0x1){
                  // SerialUSB.print(F("7- Message type in buffer: ")); SerialUSB.println(midiMsgBuf7[idx].message, HEX);
                  // SerialUSB.print(F("7- Message type arrived: ")); SerialUSB.println(msgType, HEX);
                  // Reset bank flag
                  midiMsgBuf7[idx].banksToUpdate &= ~(1 << currentBank);
                  SearchMsgInConfigAndUpdate( midiMsgBuf7[idx].type,
                                              midiMsgBuf7[idx].message,
                                              midiMsgBuf7[idx].channel,
                                              midiMsgBuf7[idx].parameter,
                                              midiMsgBuf7[idx].value,
                                              midiMsgBuf7[idx].port,
                                              false);
                }   
              }
            }
          }
        }
    }
  }else{
    int lowerB = lower_bound_search14(midiMsgBuf14, param, 0, midiRxSettings.lastMidiBufferIndex14);
    int upperB = upper_bound_search14(midiMsgBuf14, param, 0, midiRxSettings.lastMidiBufferIndex14);
  
    if((lowerB < 0) || (upperB < 0)) return;  // if any of the boundaries are negative, return, since parameter wasn't found
    
    // SerialUSB.print(lowerB); SerialUSB.print(" - "); SerialUSB.println(upperB);

    for(uint32_t idx = lowerB; idx < upperB; idx++){
      if(midiMsgBuf14[idx].channel == channel){
       // SerialUSB.print(F("14- Message type in buffer: "));SerialUSB.println(midiMsgBuf14[idx].message, HEX);
       // SerialUSB.print(F("14- Message type arrived: "));SerialUSB.println(msgType, HEX);
        if(midiMsgBuf14[idx].message == msgType){
          if(midiMsgBuf14[idx].type == fbType){
            if(midiMsgBuf14[idx].port & (1 << midiSrc)){
//              SerialUSB.println(F("MATCH"));
              midiMsgBuf14[idx].value = value;      // Update value in MIDI RX buffer
              midiMsgBuf14[idx].banksToUpdate = midiMsgBuf14[idx].banksPresent;
//                SerialUSB.println("MIDI MESSAGE ALREADY IN 14 BIT BUFFER, UPDATED");
              if((midiMsgBuf14[idx].banksToUpdate >> currentBank) & 0x1){
                // Reset bank flag
                // SerialUSB.println(F("14 bit MATCH"));
                midiMsgBuf14[idx].banksToUpdate &= ~(1 << currentBank);
                SearchMsgInConfigAndUpdate( midiMsgBuf14[idx].type,
                                            midiMsgBuf14[idx].message,
                                            midiMsgBuf14[idx].channel,
                                            midiMsgBuf14[idx].parameter,
                                            midiMsgBuf14[idx].value,
                                            midiMsgBuf14[idx].port,
                                            false);
              }
              thereIsAMatch = true;
            }
          }
        }
      }
    }
  }   
}

void SearchMsgInConfigAndUpdate(byte fbType, byte msgType, byte channel, uint16_t param, uint16_t value, bool midiSrc, bool bankChange){
  // If it is a regular message, check if it matches the feedback configuration for all the inputs (only the current bank)
  byte messageToCompare = 0;
  byte portToCompare = 0;

  switch(fbType){
    case FB_ENCODER:{
      // SWEEP ALL ENCODERS - // FIX FOR SHIFT ROTARY ACTION AND CHANGE ROTARY CONFIG FOR ROTARY FEEDBACK IN ALL CASES
      for(uint8_t encNo = 0; encNo < config->inputs.encoderCount; encNo++){
        switch(msgType){
          case MidiTypeYTX::NoteOn:{
            messageToCompare = rotaryMessageTypes::rotary_msg_note;
          }break;
          case MidiTypeYTX::ControlChange:{
            messageToCompare = rotaryMessageTypes::rotary_msg_cc; 
          }break;
          case MidiTypeYTX::ProgramChange:{
            messageToCompare = rotaryMessageTypes::rotary_msg_pc_rel;
          }break;
          case MidiTypeYTX::NRPN:{
            messageToCompare = rotaryMessageTypes::rotary_msg_nrpn;
          }break;
          case MidiTypeYTX::RPN:{
            messageToCompare = rotaryMessageTypes::rotary_msg_rpn;
          }break;
          case MidiTypeYTX::PitchBend:{
            messageToCompare = rotaryMessageTypes::rotary_msg_pb;
          }break;
        }
        if( encoder[encNo].rotaryFeedback.parameterLSB == param  ||
                messageToCompare == rotaryMessageTypes::rotary_msg_pb ||
                messageToCompare == rotaryMessageTypes::rotary_msg_pc_rel){
          if(encoder[encNo].rotaryFeedback.channel == channel){
            if(encoder[encNo].rotaryFeedback.message == messageToCompare){
              if(encoder[encNo].rotaryFeedback.source & midiSrc){      
                // If there's a match, set encoder value and feedback
                encoderHw.SetEncoderValue(currentBank, encNo, value);
              }
            }
          }
        }
      }
    }break;
    case FB_2CC:{
      // SWEEP ALL ENCODERS - // FIX FOR SHIFT ROTARY ACTION AND CHANGE ROTARY CONFIG FOR ROTARY FEEDBACK IN ALL CASES
      for(uint8_t encNo = 0; encNo < config->inputs.encoderCount; encNo++){   
        // SWEEP ALL ENCODERS SWITCHES
        if(encoder[encNo].switchFeedback.parameterLSB == param){ 
          if(encoder[encNo].switchFeedback.channel == channel){
            if(encoder[encNo].switchFeedback.source & (1 << midiSrc)){    
              // If there's a match, set encoder value and feedback
              if(encoder[encNo].switchConfig.mode == switchModes::switch_mode_2cc)
                encoderHw.SetEncoder2cc(currentBank, encNo, value);                
            }
          }
        }
      }
    }break;
    case FB_ENC_VUMETER:{
      // SWEEP ALL ENCODERS - // FIX FOR SHIFT ROTARY ACTION AND CHANGE ROTARY CONFIG FOR ROTARY FEEDBACK IN ALL CASES
      for(uint8_t encNo = 0; encNo < config->inputs.encoderCount; encNo++){
        if( encoder[encNo].rotaryFeedback.parameterLSB == param){
          if(channel == VUMETER_CHANNEL){
            if(encoder[encNo].rotaryFeedback.message == rotary_msg_vu_cc){
              if(encoder[encNo].rotaryFeedback.source & midiSrc){      
                // If there's a match, set encoder value and feedback
                feedbackHw.SetChangeEncoderFeedback(FB_ENC_VUMETER, encNo, value, 
                                                    encoderHw.GetModuleOrientation(encNo/4), false);  // HARDCODE: N° of encoders in module / is
                feedbackHw.SetChangeEncoderFeedback(FB_2CC, encNo, encoderHw.GetEncoderValue(encNo), 
                                                    encoderHw.GetModuleOrientation(encNo/4), false);   // HARDCODE: N° of encoders in module / is 
              }
            }
          }
        }
      }
    }break;
    case FB_SHIFT:{
      // SWEEP ALL ENCODERS SWITCHES - // FIX FOR SHIFT ROTARY ACTION AND CHANGE ROTARY CONFIG FOR ROTARY FEEDBACK IN ALL CASES
      for(uint8_t encNo = 0; encNo < config->inputs.encoderCount; encNo++){   
          switch(msgType){
            case MidiTypeYTX::NoteOn:{
              messageToCompare = rotaryMessageTypes::rotary_msg_note;
            }break;
            case MidiTypeYTX::ControlChange:{
              messageToCompare = rotaryMessageTypes::rotary_msg_cc; 
            }break;
            case MidiTypeYTX::ProgramChange:{
              messageToCompare = rotaryMessageTypes::rotary_msg_pc_rel;
            }break;
            case MidiTypeYTX::NRPN:{
              messageToCompare = rotaryMessageTypes::rotary_msg_nrpn;
            }break;
            case MidiTypeYTX::RPN:{
              messageToCompare = rotaryMessageTypes::rotary_msg_rpn;
            }break;
            case MidiTypeYTX::PitchBend:{
              messageToCompare = rotaryMessageTypes::rotary_msg_pb;
            }break;
          }

        // SWEEP ALL ENCODERS SWITCHES
        if(encoder[encNo].switchFeedback.parameterLSB == param || 
                  messageToCompare == rotaryMessageTypes::rotary_msg_pb ||
                  messageToCompare == rotaryMessageTypes::rotary_msg_pc_rel){ 
          if(encoder[encNo].switchFeedback.channel == channel){
            if(encoder[encNo].switchFeedback.message == messageToCompare){
              if(encoder[encNo].switchFeedback.source & midiSrc){    
              // If there's a match, set encoder value and feedback
                encoderHw.SetEncoderShiftValue(currentBank, encNo, value);  
              }
            }
          }
        }
      }
    }break;
    case FB_ENCODER_SWITCH:{
      // SWEEP ALL ENCODERS SWITCHES - // FIX FOR SHIFT ROTARY ACTION AND CHANGE ROTARY CONFIG FOR ROTARY FEEDBACK IN ALL CASES
      for(uint8_t encNo = 0; encNo < config->inputs.encoderCount; encNo++){   
          switch(msgType){
            case MidiTypeYTX::NoteOn:{
              messageToCompare = switchMessageTypes::switch_msg_note;
            }break;
            case MidiTypeYTX::ControlChange:{
              messageToCompare = switchMessageTypes::switch_msg_cc; 
            }break;
            case MidiTypeYTX::ProgramChange:{
              messageToCompare = switchMessageTypes::switch_msg_pc;
            }break;
            case MidiTypeYTX::NRPN:{
              messageToCompare = switchMessageTypes::switch_msg_nrpn;
            }break;
            case MidiTypeYTX::RPN:{
              messageToCompare = switchMessageTypes::switch_msg_rpn;
            }break;
            case MidiTypeYTX::PitchBend:{
              messageToCompare = switchMessageTypes::switch_msg_pb;
            }break;
          }

        // SWEEP ALL ENCODERS SWITCHES
        if(encoder[encNo].switchFeedback.parameterLSB == param || 
                  messageToCompare == switchMessageTypes::switch_msg_pb  ||
                  messageToCompare == switchMessageTypes::switch_msg_pc){ 
          if(encoder[encNo].switchFeedback.channel == channel){
            if(encoder[encNo].switchFeedback.message == messageToCompare){
              if(encoder[encNo].switchFeedback.source & midiSrc){  
                // If there's a match, set encoder value and feedback
                encoderHw.SetEncoderSwitchValue(currentBank, encNo, value);  
              }
            }
          }
        }
      }
    }break;
    case FB_DIGITAL:{
      // SWEEP ALL DIGITAL
      for(uint16_t digNo = 0; digNo < config->inputs.digitalCount; digNo++){

        switch(msgType){
          case MidiTypeYTX::NoteOn:{
            messageToCompare = digitalMessageTypes::digital_msg_note;
          }break;
          case MidiTypeYTX::ControlChange:{
            messageToCompare = digitalMessageTypes::digital_msg_cc; 
          }break;
          case MidiTypeYTX::ProgramChange:{
            messageToCompare = digitalMessageTypes::digital_msg_pc;
          }break;
          case MidiTypeYTX::NRPN:{
            messageToCompare = digitalMessageTypes::digital_msg_nrpn;
          }break;
          case MidiTypeYTX::RPN:{
            messageToCompare = digitalMessageTypes::digital_msg_rpn;
          }break;
          case MidiTypeYTX::PitchBend:{
            messageToCompare = digitalMessageTypes::digital_msg_pb;
          }break;
        } 
        
        if(digital[digNo].feedback.parameterLSB == param || 
                  messageToCompare == digitalMessageTypes::digital_msg_pb  ||
                  messageToCompare == digitalMessageTypes::digital_msg_pc){
          if(digital[digNo].feedback.channel == channel){
            if(digital[digNo].feedback.message == messageToCompare){
              if(digital[digNo].feedback.source & midiSrc){
                // If there's a match, set encoder value and feedback
                digitalHw.SetDigitalValue(currentBank, digNo, value);
              }
            }
          }
        }
      }
    }break;
    case FB_ANALOG:{
      // SWEEP ALL ANALOG
      for(uint8_t analogNo = 0; analogNo < config->inputs.analogCount; analogNo++){
        // analog has no feedback yet
        switch(msgType){
          case MidiTypeYTX::NoteOn:{
           messageToCompare = analogMessageTypes::analog_msg_note;
          }break;
          case MidiTypeYTX::ControlChange:{
            messageToCompare = analogMessageTypes::analog_msg_cc; 
          }break;
          case MidiTypeYTX::ProgramChange:{
            messageToCompare = analogMessageTypes::analog_msg_pc;
          }break;
          case MidiTypeYTX::NRPN:{
            messageToCompare = analogMessageTypes::analog_msg_nrpn;
          }break;
          case MidiTypeYTX::RPN:{
            messageToCompare = analogMessageTypes::analog_msg_rpn;
          }break;
          case MidiTypeYTX::PitchBend:{
            messageToCompare = analogMessageTypes::analog_msg_pb;
          }break;
        }
      
        if(analog[analogNo].feedback.parameterLSB == param || 
                  messageToCompare == analogMessageTypes::analog_msg_pb  ||
                  messageToCompare == analogMessageTypes::analog_msg_pc){
          if(analog[analogNo].feedback.channel == channel){
            if(analog[analogNo].feedback.message == messageToCompare){
              if(analog[analogNo].feedback.source & midiSrc){
                // If there's a match, set encoder value and feedback
                // SerialUSB.println("ANALOG MATCH");
                analogHw.SetAnalogValue(currentBank, analogNo, value);
              }
            }
          }
        }
      }
    }break;
    default: break;
  }
}
