//----------------------------------------------------------------------------------------------------
// MAIN LOOP
//----------------------------------------------------------------------------------------------------

void loop() {       // Loop time = aprox 190 us / 2 encoders
   antMicros = micros();
   MIDI.read();
   MIDIHW.read();
//  if (MIDI.read()){
//    ReadMidi(MIDI_USB);
//  }
//  if (MIDIHW.read()){
//    ReadMidi(MIDI_HW);
//  }
  
  if(enableProcessing){
    encoderHw.Read();
    
    analogHw.Read();
    
    digitalHw.Read();

    feedbackHw.Update();
    
    if(keyboardReleaseFlag && millis()- millisKeyboardPress > KEYBOARD_MILLIS){
      keyboardReleaseFlag = false;
      Keyboard.releaseAll();
    }
  }
  if(powerChangeFlag && millis() - antMillisPowerChange > 50){
    powerChangeFlag = false;
    feedbackHw.SetBankChangeFeedback();
  }
  UpdateStatusLED();
//  SerialUSB.print("LOOP: ");SerialUSB.println(micros()-antMicros);
}
