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
  feedbackHw.UpdateStatusLED();

  if(enableProcessing){
//    analogHw.Read();
  
    encoderHw.Read();
    
    digitalHw.Read();  

    feedbackHw.Update();
  }


  
  //SerialUSB.println(micros()-antMicros);
  
  
  //nLoops++;
  //if (micros()-antMicros > 1000000){
    //SerialUSB.println(nLoops);
    //nLoops = 0;
    //antMicros = micros();
  //}
  //SerialUSB.println(micros()-antMicros);

}
