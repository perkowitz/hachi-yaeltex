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

//----------------------------------------------------------------------------------------------------
// MAIN LOOP
//----------------------------------------------------------------------------------------------------

void loop() {       // Loop time = aprox 190 us / 2 encoders
   // antMicrosLoop = micros();

  if(Serial.available()){
    byte cmd = Serial.read();
//    SerialUSB.print("Serial received: 0x");SerialUSB.println(cmd, HEX);
    if(cmd == SHOW_IN_PROGRESS){
      showInProgress = true;
//      SerialUSB.println("SHOW");
    }else if(cmd == SHOW_END){
      showInProgress = false;
//      SerialUSB.println("END");
    }
  }
  // if configuration is valid, and not in kwhat mode
  if(enableProcessing){
    // Read all inputs
    encoderHw.Read();
    
    analogHw.Read();
    analogHw.SendNRPN();
    
    digitalHw.Read();

    // and update feedback
    feedbackHw.Update();  

    // if(micros()-antMicrosLoop > 10000)
    
  }
  
  // If there was an interrupt because the power source changed, re-set brightness
  if(powerChangeFlag && millis() - antMillisPowerChange > 50){
    powerChangeFlag = false;
    feedbackHw.SetBankChangeFeedback();
  }
  // Update status LED if needed
  UpdateStatusLED();
    // SerialUSB.println(micros()-antMicrosLoop);  
}
