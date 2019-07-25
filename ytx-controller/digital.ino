#include "headers/DigitalInputs.h"

//----------------------------------------------------------------------------------------------------
// DIGITAL INPUTS FUNCTIONS
//----------------------------------------------------------------------------------------------------

void DigitalInputs::Init(uint8_t maxBanks, uint8_t numberOfModules, uint8_t numberOfDigital, SPIClass *spiPort){
  nBanks = maxBanks;
  nDigital = numberOfDigital;
  nModules = numberOfModules;
  
  if(!nBanks || !nDigital || !nModules) return; // If number of digitals is zero, return;
  
  // First dimension is an array of pointers, each pointing to a column - https://www.eskimo.com/~scs/cclass/int/sx9b.html
  dBankData = (digitalBankData**) memHost->AllocateRAM(nBanks*sizeof(digitalBankData*));
  dHwData = (digitalHwData*) memHost->AllocateRAM(nDigital*sizeof(digitalHwData));
  mData = (moduleData*) memHost->AllocateRAM(nModules*sizeof(moduleData));

   for (int b = 0; b < nBanks; b++){
    dBankData[b] = (digitalBankData*) memHost->AllocateRAM(nDigital*sizeof(digitalBankData));
  }
  
  for(int d = 0; d < nDigital; d++){
    dHwData[d].digitalHWState = 0;
    dHwData[d].digitalHWStatePrev = 0;
    dHwData[d].swBounceMillisPrev = 0;
  }
  // Set all elements in arrays to 0
  for(int b = 0; b < nBanks; b++){
    for(int d = 0; d < nDigital; d++){
       dBankData[b][d].digitalInputState = 0;
       dBankData[b][d].digitalInputStatePrev = 0;
    }
  }
  pinMode(digitalMCPChipSelect1, OUTPUT);
  pinMode(digitalMCPChipSelect2, OUTPUT);

   
  for (int n = 0; n < nModules; n++){
    mData[n].moduleType = config->hwMapping.digital[n/8][n%8]; 
//    SerialUSB.println(mData[n].moduleType);
    
    mData[n].digitalMCP.begin(spiPort, 
                              n < 8 ? digitalMCPChipSelect1 : digitalMCPChipSelect2,
                              n);
//    printPointer(&mData[n].digitalMCP);
    
    mData[n].mcpState = 0;
    mData[n].mcpStatePrev = 0;
    
    for(int i=0; i < 16; i++){
      if(mData[n].moduleType == DigitalModuleTypes::RB41 || mData[n].moduleType == DigitalModuleTypes::ARC41){
        for ( int j = 0; j < defRB41module.components.nDigital; j++){
          if(i == defRB41module.rb41pins[j]){
            mData[n].digitalMCP.pinMode(i, INPUT);
            mData[n].digitalMCP.pullUp(i, HIGH);
          }else{
            mData[n].digitalMCP.pinMode(i, OUTPUT);
            mData[n].digitalMCP.digitalWrite(i, HIGH);
          }
        }
      }else if(mData[n].moduleType == DigitalModuleTypes::RB42){
        for ( int j = 0; j < defRB42module.components.nDigital; j++){
          if(i == defRB42module.rb42pins[j]){
            mData[n].digitalMCP.pinMode(i, INPUT);
            mData[n].digitalMCP.pullUp(i, HIGH);
          }else{
            mData[n].digitalMCP.pinMode(i, OUTPUT);
            mData[n].digitalMCP.digitalWrite(i, HIGH);
          }
        }
      }else if(mData[n].moduleType == DigitalModuleTypes::RB82){
        
      }
      
//      if(i != defRB41module.nextAddressPin[0] && i != defRB41module.nextAddressPin[1] && 
//        i != defRB41module.nextAddressPin[2] && i != (defRB41module.nextAddressPin[2]+1)){
//        mData[n].digitalMCP.pullUp(i, HIGH);
//      }
    } 
  }
  // AFTER INITIALIZATION SET NEXT ADDRESS ON EACH MODULE (EXCEPT 7 and 15, cause they don't have a module next on the chain)
//  if(nModules > 1){
//    for (int n = 0; n < nModules-1; n++){  
//      if(n < 7){
//        SetNextAddress(&mData[n].digitalMCP, n+1);  
//      }else if (n > 7){
//        SetNextAddress(&mData[n].digitalMCP, (n-8)+1);  
//      }
//    }  
//  }
  if(nModules > 1){
    for (int n = 0; n < nModules-1; n++){
      if(n != 7){
        SetNextAddress(&mData[n].digitalMCP, n+1);
      }
    }  
  }
}


void DigitalInputs::Read(void){
  byte indexDigital = 0;
  byte nButtonsInModule = 0;
  
  if(!nBanks || !nDigital || !nModules) return;   // if no banks, no digital inputs or no modules are configured, exit here
  
  for(byte mcpNo = 0; mcpNo < nModules; mcpNo++){
    // FOR EACH MODULE IN CONFIG, READ DIFFERENTLY
    switch(mData[mcpNo].moduleType){
      case DigitalModuleTypes::ARC41:
      case DigitalModuleTypes::RB42:
      case DigitalModuleTypes::RB41:{
        mData[mcpNo].mcpState = mData[mcpNo].digitalMCP.digitalRead();  // READ ENTIRE MODULE
        for(int i=0; i<16; i++){
          SerialUSB.print( (mData[mcpNo].mcpState >> (15-i)) & 0x01, BIN);  
        }
        SerialUSB.print("\t");
//        mData[mcpNo].mcpState = 0xFFFF;

        // Get number of digital inputs in module        
        if (mData[mcpNo].moduleType == DigitalModuleTypes::RB42){
          nButtonsInModule = defRB42module.components.nDigital;
        }else {
          nButtonsInModule = defRB41module.components.nDigital;
        }
                                                   
        if( mData[mcpNo].mcpState != mData[mcpNo].mcpStatePrev){    // if module state changed
          mData[mcpNo].mcpStatePrev = mData[mcpNo].mcpState;     
      
          for(int nBut = 0; nBut < nButtonsInModule; nBut++){   // check each digital input
            byte pin = (mData[mcpNo].moduleType == DigitalModuleTypes::RB42) ?  defRB42module.rb42pins[nBut] :  // get pin for each individual input
                                                                                defRB41module.rb41pins[nBut];
                                                                        
            dHwData[indexDigital].digitalHWState = !((mData[mcpNo].mcpState >> pin) & 0x0001);  // read the state of each input
//            
            if( dHwData[indexDigital].digitalHWState != dHwData[indexDigital].digitalHWStatePrev &&  // if input changed
              ( dHwData[indexDigital].swBounceMillisPrev - millis() > BOUNCE_MILLIS) ){              // and bounce time elapsed   // REVIEW BOUNCE ROUTINE
              
              dHwData[indexDigital].swBounceMillisPrev = millis();
              dHwData[indexDigital].digitalHWStatePrev = dHwData[indexDigital].digitalHWState;
//              
              if(dHwData[indexDigital].digitalHWState){
                dBankData[currentBank][indexDigital].digitalInputState = !dBankData[currentBank][indexDigital].digitalInputState;
//                SEND MIDI/KB MESSAGE
//                Keyboard.press(KEY_LEFT_ALT);
//                Keyboard.press(KEY_TAB);
//                delay(500);
                
//                SerialUSB.print("Button "); SerialUSB.print(indexDigital);
//                SerialUSB.print(" : "); SerialUSB.println(dBankData[currentBank][indexDigital].digitalInputState);

//                SELECT MIDI PORT
//                MIDI.sendNoteOn(0, 127, 1);
//                MIDIHW.sendNoteOn(0, 127, 1);
                if(indexDigital < nBanks && currentBank != indexDigital ){ // ADD BANK CONDITION
                  currentBank = memHost->LoadBank(indexDigital);                   
//                  SerialUSB.println("___________________________");
//                  SerialUSB.print("Loaded Bank: "); SerialUSB.println(currentBank);
//                  SerialUSB.println("___________________________");
                  feedbackHw.SetBankChangeFeedback();
                }
                
              }else if (!dHwData[indexDigital].digitalHWState && 
                         digital[indexDigital].actionConfig.action != switchActions::switch_toggle){
                dBankData[currentBank][indexDigital].digitalInputState = 0;
//                SEND MIDI/KB MESSAGE
//                Keyboard.releaseAll();

//                SerialUSB.print("Button "); SerialUSB.print(nBut);
//                SerialUSB.print(" : "); SerialUSB.println(digitalInputState[currentBank][indexDigital]);

//                SELECT MIDI PORT                
//                MIDI.sendNoteOff(0, 0, 1);
//                MIDIHW.sendNoteOff(0, 0, 1);
              }
            }   
            indexDigital++;
          }
        }else{
          indexDigital += nButtonsInModule;
        }
      }break;
      // MATRIX MODULES
      case DigitalModuleTypes::RB82:{
        nButtonsInModule = defRB82module.components.nDigital;
        
        for(int nBut = 0; nBut < nButtonsInModule; nBut++){
//          byte pin = defRB82module.rb82pins[0][nBut];
  
  //            SerialUSB.println(indexDigital);
          indexDigital++;
        }
      }break;
      default:break;
    } 
    
//    SerialUSB.println(indexDigital);
  }
  SerialUSB.println();
}

void DigitalInputs::SetNextAddress(MCP23S17 *mcpX, byte addr){
  for (int i = 0; i<3; i++){
    mcpX->pinMode(defRB41module.nextAddressPin[i],OUTPUT);
    mcpX->digitalWrite(defRB41module.nextAddressPin[i],(addr>>i)&1);
  }
//  mcpX.pinMode(defRB41module.nextAddressPin[0],OUTPUT);
//  mcpX.pinMode(defRB41module.nextAddressPin[1],OUTPUT);
//  mcpX.pinMode(defRB41module.nextAddressPin[2],OUTPUT);
//  
//  mcpX.digitalWrite(defRB41module.nextAddressPin[0], addr&1);
//  mcpX.digitalWrite(defRB41module.nextAddressPin[1],(addr>>1)&1);
//  mcpX.digitalWrite(defRB41module.nextAddressPin[2],(addr>>2)&1);
  
  SerialUSB.print("Next: ");  SerialUSB.print(addr); SerialUSB.print(": ");
  SerialUSB.print((addr>>2)&1, BIN);
  SerialUSB.print((addr>>1)&1, BIN);
  SerialUSB.println(addr&1, BIN);
  return;
}
