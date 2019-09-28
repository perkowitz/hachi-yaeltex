//----------------------------------------------------------------------------------------------------
// MAIN LOOP
//----------------------------------------------------------------------------------------------------

void loop() {       // Loop time = aprox 190 us / 2 encoders
  
  if (MIDI.read()){
    ReadMidi(MIDI_USB);
  }
  if (MIDIHW.read()){
    ReadMidi(MIDI_HW);
  }
//
  UpdateStatusLED();
  antMicros = micros();
  if(enableProcessing){

    
    encoderHw.Read();
    
//    analogHw.Read();
    digitalHw.Read();
    feedbackHw.Update();
//    
    
    if(keyboardReleaseFlag && millis()- millisKeyboardPress > KEYBOARD_MILLIS){
      keyboardReleaseFlag = false;
      Keyboard.releaseAll();
    }
//    SerialUSB.println(micros()-antMicros);
  }
}
