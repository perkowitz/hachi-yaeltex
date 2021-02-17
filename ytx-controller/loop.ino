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

void loop() { 
  if(testMicrosLoop)       
    antMicrosLoop = micros();
    
  // Update status LED
  UpdateStatusLED();

  // Check for incoming Serial messages
  CheckSerialUSB();


  // if configuration is valid, and not in kwhat mode
  if(enableProcessing){
    // Read all inputs
    encoderHw.Read();       // 32 encoders -> ~560 microseconds
    
  
    analogHw.Read();        // 44 analogs -> ~1200 microseconds
    analogHw.SendNRPN();
    
    digitalHw.Read();       // 3 RB82 + 2 RB42 -> ~600 microseconds
       
    // and update feedback
    feedbackHw.Update();  
    
    // Release keys that 
    if(keyboardReleaseFlag && millis() > millisKeyboardPress){
      keyboardReleaseFlag = false;
      Keyboard.releaseAll();
    }

    if(millis()-antMillisSaveControllerState > SAVE_CONTROLLER_STATE_MS){   
      antMillisSaveControllerState = millis();         // Reset millis
      memHost->SaveControllerState();
      // SerialUSB.println(F("Backup"));
    } 
  }
  
  // if(countOn && millis()-antMicrosFirstMessage > 100){
  //   countOn = false;
  //   SerialUSB.print("Since first: "); SerialUSB.println(millis()-antMicrosFirstMessage);
  //   SerialUSB.print("Msg count: "); SerialUSB.println(msgCount);
  //   msgCount = 0;
  // }

  // If there was an interrupt because the power source changed, re-set brightness
  if(enableProcessing && powerChangeFlag && millis() - antMillisPowerChange > 50){
    powerChangeFlag = false;
    feedbackHw.SetBankChangeFeedback(FB_BANK_CHANGED);
  }
 
  if(millis()-antMillisWD > WATCHDOG_RESET_MS){   
    Watchdog.reset();               // Reset count for WD
    antMillisWD = millis();         // Reset millis
  } 

  

  if(testMicrosLoop) 
    SerialUSB.println(micros()-antMicrosLoop);    


}
