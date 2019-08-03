
//----------------------------------------------------------------------------------------------------
// COMMS MIDI - SERIAL
//----------------------------------------------------------------------------------------------------
void ReadMidi(bool midiSrc) {
  uint8_t msgType = midiSrc ? MIDIHW.getType() : MIDI.getType();
//  SerialUSB.print(midiSrc ? "MIDI_HW: " : "MIDI_USB: ");
//  SerialUSB.print(msgType); SerialUSB.print("\t");
//  SerialUSB.print(MIDIHW.getData1()); SerialUSB.print("\t");
//  SerialUSB.print(MIDIHW.getData2()); SerialUSB.print("\t");
//  SerialUSB.print(MIDIHW.getChannel()); SerialUSB.println("\t");
  switch (msgType) {
      case midi::NoteOn:
        if(midiSrc == MIDI_USB){
           MIDI.sendNoteOn(MIDI.getData1(), MIDI.getData2(), MIDI.getChannel());
        }
        else{
           MIDIHW.sendNoteOn(MIDIHW.getData1(), MIDIHW.getData2(), MIDIHW.getChannel());
        }
      break;

      case midi::NoteOff:
        if(midiSrc == MIDI_USB){
           MIDI.sendNoteOff(MIDI.getData1(), MIDI.getData2(), MIDI.getChannel());
        }
        else{
           MIDIHW.sendNoteOff(MIDIHW.getData1(), MIDIHW.getData2(), MIDIHW.getChannel());
        }
      break;
      case midi::ControlChange:
        if(midiSrc == MIDI_USB){
           feedbackHw.SetChangeEncoderFeedback(FB_ENCODER, MIDI.getData1(), MIDI.getData2(), false);           // aprox 90 us / 4 rings de 16 leds   // 120 us / 8 enc // 200 us / 16 enc
        }
        else{
          
        }
      break;

//    case midi::SystemExclusive:
//      int sysexLength = 0;
//      const byte *pMsg;
//
//      sysexLength = (int) MIDI.getSysExArrayLength();
//      pMsg = MIDI.getSysExArray();
//
//      // Write to Serial the sysex received
//      SerialUSB.println("SYSEX DATA RECEIVED");
//      for (int s = 0; s<sysexLength; s++){
//        SerialUSB.print(pMsg[s],HEX); SerialUSB.print(" ");
//      }
//      SerialUSB.println("\n");
//
//      char sysexID[3];
//      sysexID[0] = (char) pMsg[1];
//      sysexID[1] = (char) pMsg[2];
//      sysexID[2] = (char) pMsg[3];
//
//      char command = pMsg[4];
//
//      if (sysexID[0] == 'Y' && sysexID[1] == 'T' && sysexID[2] == 'X') {
//
//        if (command == CONFIG_MODE) {           // Enter config mode
//          //MIDI.turnThruOff();
//          flagBlinkStatusLED = 1;
//          blinkCountStatusLED = 1;
//          const byte configAckSysExMsg[5] = {'Y', 'T', 'X', CONFIG_ACK, 0};
//          MIDI.sendSysEx(5, configAckSysExMsg, false);
//          //ResetConfig(CONFIG_ON);
//        }
//        else if (command == EXIT_CONFIG) {       // Enter config mode
//          flagBlinkStatusLED = 1;
//          blinkCountStatusLED = 2;
//          const byte configAckSysExMsg[5] = {'Y', 'T', 'X', EXIT_CONFIG_ACK, 0};
//          MIDI.sendSysEx(5, configAckSysExMsg, false);
//          //MIDI.turnThruOn();
//          //ResetConfig(CONFIG_OFF);
//        }
//        else if (command == DUMP_TO_HW) {           // Save dump data
//          if (!receivingSysEx) {
//            receivingSysEx = 1;
//            MIDI.turnThruOff();
//          }
//
//          byte txStatus = KMS::io.write(dataPacketSize * pMsg[5], pMsg + 6, sysexLength - 7); // pMsg has index in byte 6, total sysex packet has max.
//          if(txStatus != 0){
//            SerialUSB.print("ERROR WRITING EEPROM. ERROR CODE: ");
//            SerialUSB.println(txStatus);
//          }
//          // |F0, 'Y' , 'T' , 'X', command, index, F7|
//          flagBlinkStatusLED = 1;
//          blinkCountStatusLED = 1;
//
//          if (sysexLength < dataPacketSize + 7) { // Last message?
//            receivingSysEx = 0;
//            flagBlinkStatusLED = 1;
//            blinkCountStatusLED = 3;
//            const byte dumpOkMsg[5] = {'Y', 'T', 'X', DUMP_OK, 0};
//            MIDI.sendSysEx(5, dumpOkMsg, false);
//            const byte configAckSysExMsg[5] = {'Y', 'T', 'X', EXIT_CONFIG_ACK, 0};
//            MIDI.sendSysEx(5, configAckSysExMsg, false);
//            //MIDI.turnThruOn();
//            //ResetConfig(CONFIG_OFF);
//
//            // Read config
//            for(int i = 0; i < 20; i++){
//              KMS::io.read(64*i, data, 64);
//              SerialUSB.println("Sysex config in EEPROM: ");
//              for(int i = 0; i<64; i++){
//                SerialUSB.print(data[i]);
//                SerialUSB.print((i+1)%32 ? ", " : "\n");
//              }
//              SerialUSB.println();
//            }
//           // initiateReset(50);
//          }
//        }
//      }
//      break;
  }
}
