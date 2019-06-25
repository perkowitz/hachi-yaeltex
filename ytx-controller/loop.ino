//----------------------------------------------------------------------------------------------------
// MAIN LOOP
//----------------------------------------------------------------------------------------------------

void loop() {       // Loop time = aprox 190 us / 2 encoders
  //antMicros = micros();
//  analogHw.Read();
  
//  if (MIDI.read()){
//    ReadMidi(MIDI_USB);
//  }
//  if (MIDIHW.read()){
//    ReadMidi(MIDI_HW);
//  }
//
  encoderHw.Read();
  
//  ReadButtons();
 //SerialUSB.println(micros()-antMicros);
  if (flagBlinkStatusLED && blinkCountStatusLED) blinkStatusLED();
  //nLoops++;
  //if (micros()-antMicros > 1000000){
    //SerialUSB.println(nLoops);
    //nLoops = 0;
    //antMicros = micros();
  //}
  //SerialUSB.println(micros()-antMicros);

}
