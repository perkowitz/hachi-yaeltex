
//----------------------------------------------------------------------------------------------------
// DIGITAL INPUTS FUNCTIONS
//----------------------------------------------------------------------------------------------------

void InitDigital(){
  for (int n = 0; n < N_DIGITAL_MODULES; n++){
//    digitalMCP[n].begin(&SPI, n < 4 ? digitalMCPChipSelect1 : digitalMCPChipSelect2, n);
//    for(int i=0; i<16; i++){
//      if(i != a0pin && i != a1pin && i != a2pin && i != (a2pin+1)){
//        digitalMCP[n].pullUp(i, HIGH);
//      }
//    }
  }
//  for (int n = 0; n < NUM_DIGITAL_INPUTS; n++) {
//    digitalInputState[n] = false;  // default state
//  }
//  
//  for (int n = 0; n < N_DIGITAL_MODULES; n++){
//    setNextAddress(digitalMCP[n], n+1);
//  }
}


void ReadButtons(void){
  unsigned int butMCPState[N_DIGITAL_MODULES];
  static unsigned int butMCPPrevState[N_DIGITAL_MODULES];
  
  for(byte mcpNo = 0; mcpNo < N_DIGITAL_MODULES; mcpNo++){
    butMCPState[mcpNo] = digitalMCP[mcpNo].digitalRead();
    //SerialUSB.print(butMCPState[0],BIN); SerialUSB.println(" ");
    
    if( butMCPState[mcpNo] != butMCPPrevState[mcpNo]){
      butMCPPrevState[mcpNo] = butMCPState[mcpNo];     
  
      for(int nBut = 0; nBut < N_DIGITAL_INPUTS_X_MOD; nBut++){
        digitalInputState[nBut] = !((butMCPState[mcpNo]>>digitalInputPins[nBut]) & 0x0001);
        if( digitalInputState[nBut] !=  digitalInputPrevState[nBut]){
          digitalInputPrevState[nBut] = digitalInputState[nBut];
          if(digitalInputState[nBut]){
            indexRgbList = nBut;
            buttonLEDs1.setPixelColor(nBut*mcpNo+nBut, buttonLEDs1.Color(32*nBut,0,32*(3-nBut))); // Moderately bright green color.
          }else{
            buttonLEDs1.setPixelColor(nBut*mcpNo+nBut, buttonLEDs1.Color(0,0,0)); // Moderately bright green color.
          }
          buttonLEDs1.show();
          SerialUSB.print("Button "); SerialUSB.print(nBut);
          SerialUSB.print(" : "); SerialUSB.println(digitalInputState[nBut]);  
        }
      }
    }
  }
}
