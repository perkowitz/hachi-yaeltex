//----------------------------------------------------------------------------------------------------
// MAIN LOOP
//----------------------------------------------------------------------------------------------------

void loop() {       // Loop time = aprox 190 us / 2 encoders
   
//  if (MIDI.read()){
//    ReadMidi(MIDI_USB);
//  }
//  if (MIDIHW.read()){
//    ReadMidi(MIDI_HW);
//  }
  
//
  UpdateStatusLED();
 
  if(enableProcessing){
    antMicros = micros();

    encoderHw.Read();
//    analogHw.Read();
    
    digitalHw.Read();
//    SerialUSB.println(micros()-antMicros);
    feedbackHw.Update();
//    
    
    if(keyboardReleaseFlag && millis()- millisKeyboardPress > KEYBOARD_MILLIS){
      keyboardReleaseFlag = false;
      Keyboard.releaseAll();
    }
 //   SerialUSB.print("LOOP: ");SerialUSB.println(micros()-antMicros);
  }
}
