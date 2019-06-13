//----------------------------------------------------------------------------------------------------
// MAIN LOOP
//----------------------------------------------------------------------------------------------------

void loop() {       // Loop time = aprox 190 us / 2 encoders
//  antMicros = micros();
  ReadAnalog();
//  SerialUSB.println(micros()-antMicros);
  //SerialUSB.print(" - ");

  if (MIDI.read()){
    ReadMidi(MIDI_USB);
  }
//  if (MIDIHW.read()){
//    ReadMidi(MIDI_HW);
//  }
//
 // check the encoders and buttons every 5 millis
//  currentTime = micros();
//  if(currentTime - loopTime >= 20){
    ReadEncoders();
    ReadButtons();
//    loopTime = currentTime;  // Updates loopTime
//    SerialUSB.println(micros()-currentTime);
//  }
//
  if (flagBlinkStatusLED && blinkCountStatusLED) blinkStatusLED();
  //nLoops++;
  //if (micros()-antMicros > 1000000){
    //SerialUSB.println(nLoops);
    //nLoops = 0;
    //antMicros = micros();
  //}
  //SerialUSB.println(micros()-antMicros);

}
