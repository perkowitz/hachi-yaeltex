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

uint32_t antMillisRate = 0;
uint32_t antMeasure;
uint32_t rate = 0;

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
    encoderHw.Read();

    analogHw.Read();        
    analogHw.SendNRPN();
    
    digitalHw.Read();       
       
    // and update feedback
    feedbackHw.Update();  
    
    // Release keys that 
    if(keyboardReleaseFlag && millis() > millisKeyboardPress){
      keyboardReleaseFlag = false;
      Keyboard.releaseAll();
    }

    // TO DO: Add feature "SAVE CONTROLLER STATE" enabled check
    if(config->board.saveControllerState && (millis()-antMillisSaveControllerState > SAVE_CONTROLLER_STATE_MS)){   
      antMillisSaveControllerState = millis();         // Reset millis
      SetStatusLED(STATUS_BLINK, 1, statusLEDtypes::STATUS_FB_EEPROM);
      memHost->SaveControllerState();
      // SerialUSB.println(millis()-antMillisSaveControllerState);
      // SerialUSB.println(F("Backup"));
    } 
  }
  
  // If there was an interrupt because the power source changed, re-set brightness
  if(enableProcessing && powerChangeFlag && millis() - antMillisPowerChange > 50){
    powerChangeFlag = false;
    feedbackHw.SetBankChangeFeedback(FB_BANK_CHANGED);
  }
 
  if(millis()-antMillisWD > WATCHDOG_CHECK_MS){   
    Watchdog.reset();               // Reset count for WD
    antMillisWD = millis();         // Reset millis
  } 

  if(receivingConfig){
    if(millis()-antMicrosSysex > 5000){
      receivingConfig = false;
      // Set watchdog time to normal and reset it
      Watchdog.disable();
      Watchdog.enable(WATCHDOG_RESET_NORMAL);
      Watchdog.reset();
    }
  }

  if(testMicrosLoop) 
    SerialUSB.println(micros()-antMicrosLoop);    

  // antMicrosLoop = micros();
  // uint32_t measure = sensor.readRangeContinuousMillimeters()-50;
  // if(measure!=antMeasure)
  // {
  //   rate++;
  //   antMeasure = measure;
  // }
  // //SerialUSB.println(micros()-antMicrosLoop);

  // if(millis()-antMillisRate > 1000)
  // {
  //   antMillisRate = millis();
  //   SerialUSB.print("Rate: ");
  //   SerialUSB.println(rate);
  //   rate = 0;
  // }  
}
