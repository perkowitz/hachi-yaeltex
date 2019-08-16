#include "headers/DigitalInputs.h"

//----------------------------------------------------------------------------------------------------
// DIGITAL INPUTS FUNCTIONS
//----------------------------------------------------------------------------------------------------

void DigitalInputs::Init(uint8_t maxBanks, uint8_t numberOfDigital, SPIClass *spiPort) {
  if (!maxBanks || !numberOfDigital) return; // If number of digitals is zero, return;
  
  // CHECK WHETHER AMOUNT OF DIGITAL INPUTS IN MODULES COMBINED MATCH THE AMOUNT OF DIGITAL INPUTS IN CONFIG
  // AMOUNT OF DIGITAL MODULES
  for (int nPort = 0; nPort < DIGITAL_PORTS; nPort++) {
    for (int nMod = 0; nMod < MODULES_PER_PORT; nMod++) {
      //        SerialUSB.println(config->hwMapping.digital[nPort][nMod]);
      if (config->hwMapping.digital[nPort][nMod]) {
        modulesInConfig.digital[nPort]++;
      }
      switch (config->hwMapping.digital[nPort][nMod]) {
        case DigitalModuleTypes::ARC41:
        case DigitalModuleTypes::RB41: {
            amountOfDigitalInConfig[nPort] += defRB41module.components.nDigital;
          } break;
        case DigitalModuleTypes::RB42: {
            amountOfDigitalInConfig[nPort] += defRB42module.components.nDigital;
          } break;
        case DigitalModuleTypes::RB82: {
            amountOfDigitalInConfig[nPort] += defRB82module.components.nDigital;
          } break;
        default: break;
      }
    }
  }
  if ((amountOfDigitalInConfig[0]+amountOfDigitalInConfig[1]) != numberOfDigital) {
//    SerialUSB.println("Error in config: Number of digitales does not match modules in config");
//    SerialUSB.print("nDigitals: "); SerialUSB.println(numberOfDigital);
//    SerialUSB.print("Modules: "); SerialUSB.println(amountOfDigitalInConfig[0]+amountOfDigitalInConfig[1]);
    return;
  } else {
//    SerialUSB.println("nDigitals and module config match");
  }

  nBanks = maxBanks;
  nDigital = numberOfDigital;
  nModules = modulesInConfig.digital[0]+modulesInConfig.digital[1];

  // First dimension is an array of pointers, each pointing to a column - https://www.eskimo.com/~scs/cclass/int/sx9b.html
  dBankData = (digitalBankData**) memHost->AllocateRAM(nBanks * sizeof(digitalBankData*));
  dHwData = (digitalHwData*) memHost->AllocateRAM(nDigital * sizeof(digitalHwData));
  digMData = (moduleData*) memHost->AllocateRAM(nModules * sizeof(moduleData));

  for (int b = 0; b < nBanks; b++) {
    dBankData[b] = (digitalBankData*) memHost->AllocateRAM(nDigital * sizeof(digitalBankData));
  }

  for (int d = 0; d < nDigital; d++) {
    dHwData[d].digitalHWState = 0;
    dHwData[d].digitalHWStatePrev = 0;
    dHwData[d].swBounceMillisPrev = 0;
    dHwData[d].bounceOn = 0;
  }
  // Set all elements in arrays to 0
  for (int b = 0; b < nBanks; b++) {
    for (int d = 0; d < nDigital; d++) {
      dBankData[b][d].digitalInputState = 0;
      dBankData[b][d].digitalInputStatePrev = 0;
    }
  }

  for (int c = 0; c < 16; c++) {
    currentProgram[0][c] = 0;
    currentProgram[1][c] = 0;
  }

  pinMode(digitalMCPChipSelect1, OUTPUT);
  pinMode(digitalMCPChipSelect2, OUTPUT);

//  SerialUSB.println("DIGITAL");
  for (int n = 0; n < nModules; n++) {
    byte chipSelect = 0;
    byte mcpAddress = n % 8;
    if(n<8) chipSelect = digitalMCPChipSelect1;
    else    chipSelect = digitalMCPChipSelect2;
    
    digitalMCP[n].begin(spiPort, chipSelect, mcpAddress);
//    printPointer(&digitalMCP[n]);
//    SerialUSB.print("Module ");SerialUSB.print(n);
//    SerialUSB.print(" chip select ");SerialUSB.print(chipSelect);
//    SerialUSB.print(" mcpAddress ");SerialUSB.println(mcpAddress);
  }
  
  for (int n = 0; n < nModules; n++) {
    digMData[n].moduleType = config->hwMapping.digital[n/8][n%8];
    //    SerialUSB.println(digMData[n].moduleType);
   
    digMData[n].mcpState = 0;
    digMData[n].mcpStatePrev = 0;

    // AFTER INITIALIZATION SET NEXT ADDRESS ON EACH MODULE (EXCEPT 7 and 15, cause they don't have a module next on the chain)
    byte mcpAddress = n % 8;
    if (nModules > 1) {
      SetNextAddress(&digitalMCP[n], mcpAddress + 1);
    }
    
    for (int i = 0; i < 16; i++) {
      if (digMData[n].moduleType == DigitalModuleTypes::RB41 || digMData[n].moduleType == DigitalModuleTypes::ARC41) {
        for ( int j = 0; j < defRB41module.components.nDigital; j++) {
          if (i == defRB41module.rb41pins[j]) 
            digitalMCP[n].pullUp(i, HIGH);
        }                                                                                                                                                                                                                                                                                
      } else if (digMData[n].moduleType == DigitalModuleTypes::RB42) {
        for ( int j = 0; j < defRB42module.components.nDigital; j++) {
          if (i == defRB42module.rb42pins[j])
            digitalMCP[n].pullUp(i, HIGH);
        }
      } else if (digMData[n].moduleType == DigitalModuleTypes::RB82) {

      }
    }
  }
}


void DigitalInputs::Read(void) {
  byte indexDigital = 0;
  byte nButtonsInModule = 0;

  if (!nBanks || !nDigital || !nModules) return;  // if no banks, no digital inputs or no modules are configured, exit here

  for (byte mcpNo = 0; mcpNo < nModules; mcpNo++) {
    // FOR EACH MODULE IN CONFIG, READ DIFFERENTLY
    digMData[mcpNo].mcpState = digitalMCP[mcpNo].digitalRead();  // READ ENTIRE MODULE
  
    if(mcpNo>7){
//      SerialUSB.print(mcpNo); SerialUSB.print(": ");
//      for(int i=0; i<16; i++){
//        SerialUSB.print( (digMData[mcpNo].mcpState >> (15-i)) & 0x01, BIN);
//        if(i == 9 || i == 6) SerialUSB.print(" ");
//      }
//      SerialUSB.print("\t");
    }
    
    if ( digMData[mcpNo].mcpState != digMData[mcpNo].mcpStatePrev) {  // if module state changed
      digMData[mcpNo].mcpStatePrev = digMData[mcpNo].mcpState;
//      SerialUSB.print(mcpNo); SerialUSB.print(": ");
//      for(int i=0; i<16; i++){
//        SerialUSB.print( (digMData[mcpNo].mcpState >> (15-i)) & 0x01, BIN);
//        if(i == 9 || i == 6) SerialUSB.print(" ");
//      }
//      SerialUSB.println("");
      
      switch (digMData[mcpNo].moduleType) {
        case DigitalModuleTypes::ARC41:
        case DigitalModuleTypes::RB42:
        case DigitalModuleTypes::RB41: {  
            // Get number of digital inputs in module        
            if (digMData[mcpNo].moduleType == DigitalModuleTypes::RB42){
              nButtonsInModule = defRB42module.components.nDigital;
            }else {
              nButtonsInModule = defRB41module.components.nDigital;
            }

            for (int nBut = 0; nBut < nButtonsInModule; nBut++) { // check each digital input
              byte pin = (digMData[mcpNo].moduleType == DigitalModuleTypes::RB42) ? defRB42module.rb42pins[nBut] :  // get pin for each individual input
                                                                                    defRB41module.rb41pins[nBut];
  
              dHwData[indexDigital].digitalHWState = !((digMData[mcpNo].mcpState >> pin) & 0x0001);  // read the state of each input
              //
              if ( dHwData[indexDigital].digitalHWState != dHwData[indexDigital].digitalHWStatePrev &&
                   millis()-dHwData[indexDigital].swBounceMillisPrev > BOUNCE_MILLIS) {             // and bounce time elapsed   // REVIEW BOUNCE ROUTINE
                dHwData[indexDigital].digitalHWStatePrev = dHwData[indexDigital].digitalHWState;
//                dHwData[indexDigital].bounceOn = true;
                dHwData[indexDigital].swBounceMillisPrev = millis();
                
                // STATUS LED SET BLINK
                feedbackHw.SetStatusLED(STATUS_BLINK, 1, STATUS_FB_INPUT_CHANGED);
                
                if (dHwData[indexDigital].digitalHWState) {
                  dBankData[currentBank][indexDigital].digitalInputState = !dBankData[currentBank][indexDigital].digitalInputState;

//                  if (indexDigital < nBanks && currentBank != indexDigital ) { // ADD BANK CONDITION
//                    currentBank = memHost->LoadBank(indexDigital);
//                    //                  SerialUSB.println("___________________________");
//                    //                  SerialUSB.print("Loaded Bank: "); SerialUSB.println(currentBank);
//                    //                  SerialUSB.println("___________________________");
//                    feedbackHw.SetBankChangeFeedback();
//                  }else{
                    SendActionMessage(indexDigital, dBankData[currentBank][indexDigital].digitalInputState); 
//                  } 
                } else if (!dHwData[indexDigital].digitalHWState &&
                           digital[indexDigital].actionConfig.action != switchActions::switch_toggle) {
                  dBankData[currentBank][indexDigital].digitalInputState = 0;
                  
                  SendActionMessage(indexDigital, dBankData[currentBank][indexDigital].digitalInputState); 
//                  SerialUSB.print("Button "); SerialUSB.print(indexDigital);
//                  SerialUSB.print(" : "); SerialUSB.println(dBankData[currentBank][indexDigital].digitalInputState);
                }
                if(digital[indexDigital].feedback.source == fb_src_local){
                  feedbackHw.SetChangeDigitalFeedback(indexDigital, dBankData[currentBank][indexDigital].digitalInputState);
                }
              }
              indexDigital++;
            }
          } break;
        // MATRIX MODULES
        case DigitalModuleTypes::RB82: {
            nButtonsInModule = defRB82module.components.nDigital;
  
            for (int nBut = 0; nBut < nButtonsInModule; nBut++) {
              //          byte pin = defRB82module.rb82pins[0][nBut];
  
              //            SerialUSB.println(indexDigital);
              indexDigital++;
            }
          } break;
        default: break;
      }
    } else {
      // Get number of digital inputs in module
      if (digMData[mcpNo].moduleType == DigitalModuleTypes::RB42) {
        indexDigital += defRB42module.components.nDigital;
      } else if (digMData[mcpNo].moduleType == DigitalModuleTypes::RB82){
        indexDigital += defRB82module.components.nDigital;
      } else {
        indexDigital += defRB41module.components.nDigital;
      }
    }
    //    SerialUSB.println(indexDigital);
  }
//    SerialUSB.println();
//    while(1);
}

void DigitalInputs::SetNextAddress(MCP23S17 *mcpX, byte addr) {
  for (int i = 0; i < 3; i++) {
    mcpX->pinMode(defRB41module.nextAddressPin[i], OUTPUT);
  }
  for (int i = 0; i < 3; i++) {
    mcpX->digitalWrite(defRB41module.nextAddressPin[i], (addr >> i) & 1);
  }
  //  mcpX.pinMode(defRB41module.nextAddressPin[0],OUTPUT);
  //  mcpX.pinMode(defRB41module.nextAddressPin[1],OUTPUT);
  //  mcpX.pinMode(defRB41module.nextAddressPin[2],OUTPUT);
  //
  //  mcpX.digitalWrite(defRB41module.nextAddressPin[0], addr&1);
  //  mcpX.digitalWrite(defRB41module.nextAddressPin[1],(addr>>1)&1);
  //  mcpX.digitalWrite(defRB41module.nextAddressPin[2],(addr>>2)&1);

  //  SerialUSB.print("Next: ");  SerialUSB.print(addr); SerialUSB.print(": ");
  //  SerialUSB.print((addr>>2)&1, BIN);
  //  SerialUSB.print((addr>>1)&1, BIN);
  //  SerialUSB.println(addr&1, BIN);
  return;
}

void DigitalInputs::SendActionMessage(uint16_t index, uint16_t value){
  uint16_t paramToSend = digital[index].actionConfig.parameter[digital_MSB]<<7 | 
                         digital[index].actionConfig.parameter[digital_LSB];
  byte channelToSend = digital[index].actionConfig.channel+1;
  uint16_t minValue = digital[index].actionConfig.parameter[digital_minMSB]<<7 | 
                      digital[index].actionConfig.parameter[digital_minLSB];
  uint16_t maxValue = digital[index].actionConfig.parameter[digital_maxMSB]<<7 | 
                      digital[index].actionConfig.parameter[digital_maxLSB];

   uint16_t valueToSend = 0;
   
  if(value){
    valueToSend = maxValue;
  }else{
    valueToSend = minValue;
  }
//  SerialUSB.print(index);SerialUSB.print(" -> ");
//  SerialUSB.print("Param: ");SerialUSB.print(paramToSend);
//  SerialUSB.print(" Valor: ");SerialUSB.print(valueToSend);
//  SerialUSB.print(" Canal: ");SerialUSB.print(channelToSend);
//  SerialUSB.print(" Min: ");SerialUSB.print(minValue);
//  SerialUSB.print(" Max: ");SerialUSB.println(maxValue);
  switch(digital[index].actionConfig.message){
    case digitalMessageTypes::digital_note:{
      if(digital[index].actionConfig.midiPort & 0x01)
        MIDI.sendNoteOn( paramToSend&0x7f, valueToSend&0x7f, channelToSend);
      if(digital[index].actionConfig.midiPort & 0x02)
        MIDIHW.sendNoteOn( paramToSend&0x7f, valueToSend&0x7f, channelToSend);
    }break;
    case digitalMessageTypes::digital_cc:{
      if(digital[index].actionConfig.midiPort & 0x01)
        MIDI.sendControlChange( paramToSend&0x7f, valueToSend&0x7f, channelToSend);
      if(digital[index].actionConfig.midiPort & 0x02)
        MIDIHW.sendControlChange( paramToSend&0x7f, valueToSend&0x7f, channelToSend);
    }break;
    case digitalMessageTypes::digital_pc:{
      if(digital[index].actionConfig.midiPort & 0x01 && valueToSend != minValue)
        MIDI.sendProgramChange( paramToSend&0x7f, channelToSend);
      if(digital[index].actionConfig.midiPort & 0x02 && valueToSend)
        MIDIHW.sendProgramChange( paramToSend&0x7f, channelToSend);
    }break;
    case digitalMessageTypes::digital_pc_m:{
      if(digital[index].actionConfig.midiPort & 0x01){
        if (currentProgram[0][channelToSend - 1] > 0 && value) {
          currentProgram[0][channelToSend - 1]--;
          MIDI.sendProgramChange(currentProgram[0][channelToSend - 1], channelToSend);
        }
      }
      if(digital[index].actionConfig.midiPort & 0x02){
        if (currentProgram[1][channelToSend - 1] > 0 && value) {
          currentProgram[1][channelToSend - 1]--;
          MIDIHW.sendProgramChange(currentProgram[1][channelToSend - 1], channelToSend);
        }
      }
    }break;
    case digitalMessageTypes::digital_pc_p:{
      if(digital[index].actionConfig.midiPort & 0x01){
        if (currentProgram[0][channelToSend - 1] < 127 && value) {
          currentProgram[0][channelToSend - 1]++;
          MIDI.sendProgramChange(currentProgram[0][channelToSend - 1], channelToSend);
        }
      }
      if(digital[index].actionConfig.midiPort & 0x02){
        if (currentProgram[1][channelToSend - 1] < 127 && value) {
          currentProgram[1][channelToSend - 1]++;
          MIDIHW.sendProgramChange(currentProgram[1][channelToSend - 1], channelToSend);
        }
      }
    }break;
    case digitalMessageTypes::digital_nrpn:{
      if(digital[index].actionConfig.midiPort & 0x01){
        MIDI.sendControlChange( 99, (paramToSend >> 7) & 0x7F, channelToSend);
        MIDI.sendControlChange( 98, (paramToSend & 0x7F), channelToSend);
        MIDI.sendControlChange( 6, (valueToSend >> 7) & 0x7F, channelToSend);
        MIDI.sendControlChange( 38, (valueToSend & 0x7F), channelToSend);       
      }
      if(digital[index].actionConfig.midiPort & 0x02){
        MIDIHW.sendControlChange( 99, (paramToSend >> 7) & 0x7F, channelToSend);
        MIDIHW.sendControlChange( 98, (paramToSend & 0x7F), channelToSend);
        MIDIHW.sendControlChange( 6, (valueToSend >> 7) & 0x7F, channelToSend);
        MIDIHW.sendControlChange( 38, (valueToSend & 0x7F), channelToSend);    
      }
    }break;
    case digitalMessageTypes::digital_rpn:{
      if(digital[index].actionConfig.midiPort & 0x01){
        MIDI.sendControlChange( 101, (paramToSend >> 7) & 0x7F, channelToSend);
        MIDI.sendControlChange( 100, (paramToSend & 0x7F), channelToSend);
        MIDI.sendControlChange( 6, (valueToSend >> 7) & 0x7F, channelToSend);
        MIDI.sendControlChange( 38, (valueToSend & 0x7F), channelToSend);       
      }
      if(digital[index].actionConfig.midiPort & 0x02){
        MIDIHW.sendControlChange( 101, (paramToSend >> 7) & 0x7F, channelToSend);
        MIDIHW.sendControlChange( 100, (paramToSend & 0x7F), channelToSend);
        MIDIHW.sendControlChange( 6, (valueToSend >> 7) & 0x7F, channelToSend);
        MIDIHW.sendControlChange( 38, (valueToSend & 0x7F), channelToSend);    
      }
    }break;
    case digitalMessageTypes::digital_pb:{
      valueToSend = map(valueToSend, minValue, maxValue, -8192, 8191);
      if(digital[index].actionConfig.midiPort & 0x01)
        MIDI.sendPitchBend( valueToSend, channelToSend);    
      if(digital[index].actionConfig.midiPort & 0x02)
        MIDIHW.sendPitchBend( valueToSend, channelToSend);    
    }break;
    case digitalMessageTypes::digital_ks:{  
      if(digital[index].actionConfig.parameter[digital_modifier] && value)
        Keyboard.press(digital[index].actionConfig.parameter[digital_modifier]);
      if(digital[index].actionConfig.parameter[digital_key] && value)  
        Keyboard.press(digital[index].actionConfig.parameter[digital_key]);
    
      millisKeyboardPress = millis();
      keyboardReleaseFlag = true; 
    }break;
  }
}
