#include "headers/DigitalInputs.h"

//----------------------------------------------------------------------------------------------------
// DIGITAL INPUTS FUNCTIONS
//----------------------------------------------------------------------------------------------------

void DigitalInputs::Init(uint8_t maxBanks, uint8_t numberOfModules, uint8_t numberOfDigital, SPIClass *spiPort){
  nBanks = maxBanks;
  nDigital = numberOfDigital;
  nModules = numberOfModules;

  // First dimension is an array of pointers, each pointing to a column - https://www.eskimo.com/~scs/cclass/int/sx9b.html
  digitalInputState = (bool**) memHost->AllocateRAM(nBanks*sizeof(bool*));
  digitalInputStatePrev = (bool**) memHost->AllocateRAM(nBanks*sizeof(bool*));

  digitalMCP = (MCP23S17*) memHost->AllocateRAM(nModules*sizeof(MCP23S17));
  mcpState = (uint16_t*) memHost->AllocateRAM(nModules*sizeof(uint16_t));
  mcpStatePrev = (uint16_t*) memHost->AllocateRAM(nModules*sizeof(uint16_t));
  moduleType = (byte*) memHost->AllocateRAM(nModules*sizeof(byte));

  digitalHWState = (uint8_t*) memHost->AllocateRAM(nDigital*sizeof(uint8_t));
  digitalHWStatePrev = (uint8_t*) memHost->AllocateRAM(nDigital*sizeof(uint8_t));
  swBounceMillisPrev = (uint32_t*) memHost->AllocateRAM(nDigital*sizeof(uint32_t));

   for (int b = 0; b < nBanks; b++){
    digitalInputState[b] = (bool*) memHost->AllocateRAM(nDigital * sizeof(bool));
    digitalInputStatePrev[b] = (bool*) memHost->AllocateRAM(nDigital * sizeof(bool));
  }
  
  for(int e = 0; e < nDigital; e++){
    digitalHWState[e] = 0;
    digitalHWStatePrev[e] = 0;
    swBounceMillisPrev[e] = 0;
  }
  // Set all elements in arrays to 0<
  for(int b = 0; b < nBanks; b++){
    for(int e = 0; e < nDigital; e++){
       digitalInputState[b][e] = 0;
       digitalInputStatePrev[b][e] = 0;
    }
  }
  pinMode(digitalMCPChipSelect1, OUTPUT);
  pinMode(digitalMCPChipSelect2, OUTPUT);

  moduleType[0] = RB41; moduleType[0] = RB41; moduleType[0] = RB41; moduleType[0] = RB41; 
  moduleType[0] = RB42; moduleType[0] = RB42; moduleType[0] = RB42; moduleType[0] = RB42; 
  moduleType[0] = RB82; moduleType[0] = RB82; moduleType[0] = RB82; moduleType[0] = RB82; 
  moduleType[0] = ARC41; moduleType[0] = ARC41; moduleType[0] = ARC41; moduleType[0] = ARC41; 
  for (int n = 0; n < nModules; n++){
//    moduleType[n] = config->hwMapping.digital[n/8][n%8]-1; 
    
    
    digitalMCP[n].begin(spiPort, n < 8 ? digitalMCPChipSelect1 : digitalMCPChipSelect2, n);
    printPointer(&digitalMCP[n]);
    
    mcpState[n] = 0;
    mcpStatePrev[n] = 0;
    
    for(int i=0; i<16; i++){
      if( i != defRB41module.nextAddressPin[0] && i != defRB41module.nextAddressPin[1] && 
          i != defRB41module.nextAddressPin[2] && i != (defRB41module.nextAddressPin[2]+1)){
       digitalMCP[n].pullUp(i, HIGH);
      }
    }
  }
  // AFTER INITIALIZATION SET NEXT ADDRESS ON EACH MODULE (EXCEPT 3 and 7, cause they don't have a next on the chain)
  for (int n = 0; n < nModules; n++){
    if(nModules > 1 && n != 7 && n != 15){
      SetNextAddress(digitalMCP[n], n+1);
    }
  }
}


void DigitalInputs::Read(void){
  byte indexDigital = 0;
  
  for(byte mcpNo = 0; mcpNo < nModules; mcpNo++){
    mcpState[mcpNo] = digitalMCP[mcpNo].digitalRead();
    byte nButtonsInModule = 0;
    
    if( mcpState[mcpNo] != mcpStatePrev[mcpNo]){
      mcpStatePrev[mcpNo] = mcpState[mcpNo];     
      
      // FOR EACH MODULE IN CONFIG, READ DIFFERENTLY
//      moduleType[mcpNo] = ARC41;
      switch(moduleType[mcpNo]){
        case DigitalModuleTypes::ARC41:
        case DigitalModuleTypes::RB41:
        case DigitalModuleTypes::RB42:{
          nButtonsInModule = moduleType[mcpNo] == RB42 ?  defRB42module.components.nDigital : 
                                                          defRB41module.components.nDigital;
           
          for(int nBut = 0; nBut < nButtonsInModule; nBut++){
            byte pin = moduleType[mcpNo] == DigitalModuleTypes::RB42 ?  defRB42module.rb42pins[nBut] : 
                                                                        defRB41module.rb41pins[nBut];
//            SerialUSB.print("Button "); SerialUSB.println(pin);
            digitalHWState[indexDigital] = !((mcpState[mcpNo] >> pin) & 0x0001);

            if( digitalHWState[indexDigital] !=  digitalHWStatePrev[indexDigital] && 
              (swBounceMillisPrev[indexDigital+nBut] - millis() > BOUNCE_MILLIS)){
              
              swBounceMillisPrev[indexDigital] = millis();
              digitalHWStatePrev[indexDigital] = digitalHWState[indexDigital];
              
              if(digitalHWState[indexDigital]){
                digitalInputState[currentBank][indexDigital] = !digitalInputState[currentBank][indexDigital];
//                SEND MIDI/KB MESSAGE
//                Keyboard.press(KEY_LEFT_ALT);
//                Keyboard.press(KEY_TAB);
//                delay(500);
                
//                SerialUSB.print("Button "); SerialUSB.print(nBut);
//                SerialUSB.print(" : "); SerialUSB.println(digitalInputState[currentBank][indexDigital]);

//                SELECT MIDI PORT
//                MIDI.sendNoteOn(0, 127, 1);
//                MIDIHW.sendNoteOn(0, 127, 1);

                
              }else if (!digitalHWState[indexDigital] && 
                        digital[indexDigital].actionConfig.action != switchActions::switch_toggle){
                digitalInputState[currentBank][indexDigital] = 0;
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
            SerialUSB.println(indexDigital);
          }
        }break;
        case DigitalModuleTypes::RB82:{
          nButtonsInModule = defRB82module.components.nDigital;
          for(int nBut = 0; nBut < nButtonsInModule; nBut++){
            byte pin = moduleType[mcpNo] == DigitalModuleTypes::RB42 ?  defRB42module.rb42pins[nBut] : 
                                                                        defRB41module.rb41pins[nBut];
             indexDigital++;      
             SerialUSB.println(indexDigital);                                                                  
          }
        }break;
        default:break;
      } 
    }
  }
}

void DigitalInputs::SetNextAddress(MCP23S17 mcpX, byte addr){
  mcpX.pinMode(defRB41module.nextAddressPin[0],OUTPUT);
  mcpX.pinMode(defRB41module.nextAddressPin[1],OUTPUT);
  mcpX.pinMode(defRB41module.nextAddressPin[2],OUTPUT);
    
  mcpX.digitalWrite(defRB41module.nextAddressPin[0], addr&1);
  mcpX.digitalWrite(defRB41module.nextAddressPin[1],(addr>>1)&1);
  mcpX.digitalWrite(defRB41module.nextAddressPin[2],(addr>>2)&1);
//  SerialUSB.print("Address: "); SerialUSB.println(addr);
//  SerialUSB.print("Pin 0: "); SerialUSB.println(addr&1);
//  SerialUSB.print("Pin 1: "); SerialUSB.println((addr>>1)&1);
//  SerialUSB.print("Pin 2: "); SerialUSB.println((addr>>2)&1);
  return;
}
