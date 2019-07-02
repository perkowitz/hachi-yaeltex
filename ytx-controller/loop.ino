//----------------------------------------------------------------------------------------------------
// MAIN LOOP
//----------------------------------------------------------------------------------------------------

void loop() {       // Loop time = aprox 190 us / 2 encoders
  antMicros = micros();
  //  if (MIDI.read()){
//    ReadMidi(MIDI_USB);
//  }
//  if (MIDIHW.read()){
//    ReadMidi(MIDI_HW);
//  }
//
  if (flagBlinkStatusLED && blinkCountStatusLED) blinkStatusLED();

  if(enableProcessing){
//    analogHw.Read();
  
    encoderHw.Read();
    
//    digitalHw.Read();  
  }
  
  SerialUSB.println(micros()-antMicros);
  //feedbackHw.Update();
  
  //nLoops++;
  //if (micros()-antMicros > 1000000){
    //SerialUSB.println(nLoops);
    //nLoops = 0;
    //antMicros = micros();
  //}
  //SerialUSB.println(micros()-antMicros);

}
