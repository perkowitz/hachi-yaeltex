//----------------------------------------------------------------------------------------------------
// MAIN LOOP
//----------------------------------------------------------------------------------------------------

void loop() {       // Loop time = aprox 190 us / 2 encoders
   antMicrosLoop = micros();

   // Call MIDI read function and if message arrived, the callbacks get called
//   MIDI.read();
//   MIDIHW.read();
  if(irqCounter >= 100){
    SerialUSB.println(micros()-antMicrosCC);
    antMicrosCC = micros();
    irqCounter = 0;
  }
  // if configuration is valid, and not in kwhat mode
  if(enableProcessing){
    // Read all inputs
    encoderHw.Read();
    analogHw.Read();
    digitalHw.Read();
    // and update feedback
    feedbackHw.Update();
  }
  analogHw.SendNRPN();
  // If there was an interrupt because the power source changed, re-set brightness
  if(powerChangeFlag && millis() - antMillisPowerChange > 50){
    powerChangeFlag = false;
    feedbackHw.SetBankChangeFeedback();
  }
  // Update status LED if needed
  UpdateStatusLED();
//  SerialUSB.print("LOOP: ");SerialUSB.println(micros()-antMicrosLoop);
}
