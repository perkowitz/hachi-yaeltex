#include "headers/DigitalInputs.h"

//----------------------------------------------------------------------------------------------------
// DIGITAL INPUTS FUNCTIONS
//----------------------------------------------------------------------------------------------------

void DigitalInputs::Init(uint8_t maxBanks, uint16_t numberOfDigital, SPIClass *spiPort) {
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
  
  // If amount of digitals based on module count and amount on config match, continue
  if ((amountOfDigitalInConfig[0] + amountOfDigitalInConfig[1]) != numberOfDigital) {
    SerialUSB.println("Error in config: Number of digitales does not match modules in config");
    SerialUSB.print("nDigitals: "); SerialUSB.println(numberOfDigital);
    SerialUSB.print("Modules: "); SerialUSB.println(amountOfDigitalInConfig[0]+amountOfDigitalInConfig[1]);
    return;
  } 
  SerialUSB.println("nDigitals and module config match");
    
  // Set class parameters
  nBanks = maxBanks;
  nDigitals = numberOfDigital;
  nModules = modulesInConfig.digital[0] + modulesInConfig.digital[1];
  
  // First dimension is an array of pointers, each pointing to a column - https://www.eskimo.com/~scs/cclass/int/sx9b.html
  dBankData = (digitalBankData**) memHost->AllocateRAM(nBanks * sizeof(digitalBankData*));
  dHwData = (digitalHwData*) memHost->AllocateRAM(nDigitals * sizeof(digitalHwData));
  digMData = (moduleData*) memHost->AllocateRAM(nModules * sizeof(moduleData));
  // Second dimension is an array for each bank
  for (int b = 0; b < nBanks; b++) {
    dBankData[b] = (digitalBankData*) memHost->AllocateRAM(nDigitals * sizeof(digitalBankData));
  }
  // Init all the data
  for (int d = 0; d < nDigitals; d++) {
    dHwData[d].digitalHWState = 0;
    dHwData[d].digitalHWStatePrev = 0;
    dHwData[d].swBounceMillisPrev = 0;
    dHwData[d].bounceOn = 0;
  }
  // Set all elements in arrays to 0
  for (int b = 0; b < nBanks; b++) {
    for (int d = 0; d < nDigitals; d++) {
      dBankData[b][d].digitalInputValue = 0;
      dBankData[b][d].digitalInputValuePrev = 0;
    }
  }
  // SET PROGRAM CHANGE TO 0 FOR ALL CHANNELS
  for (int c = 0; c < 16; c++) {
    currentProgram[midi_usb - 1][c] = 0;
    currentProgram[midi_hw - 1][c] = 0;
  }
  // CS pins for both SPI chains
  pinMode(digitalMCPChipSelect1, OUTPUT);
  pinMode(digitalMCPChipSelect2, OUTPUT);

  // DISABLE HARDWARE ADDRESSING FOR ALL CHIPS - ONLY NEEDED FOR RESET
  DisableHWAddress();
  // Set pullups on all pins
  SetPullUps();
  EnableHWAddress();
  
  //  SerialUSB.println("DIGITAL");
  for (int mcpNo = 0; mcpNo < nModules; mcpNo++) {
    byte chipSelect = 0;
    byte mcpAddress = mcpNo % 8;
    if (mcpNo < 8) chipSelect = digitalMCPChipSelect1;
    else    chipSelect = digitalMCPChipSelect2;
    // Begin and initialize each SPI IC
    digitalMCP[mcpNo].begin(spiPort, chipSelect, mcpAddress);
    if (nModules > 1)
      SetNextAddress(mcpNo, mcpAddress + 1);
  }
  
  // First module starts at index 0
  digMData[0].digitalIndexStart = 0;
  
  // Scan interval to spread the modules along the total scan time
  individualScanInterval = DIGITAL_SCAN_INTERVAL/nModules;
  generalMillis = millis();
  
  // Init all modules data
  for (int mcpNo = 0; mcpNo < nModules; mcpNo++) {
    digMData[mcpNo].moduleType = config->hwMapping.digital[mcpNo / 8][mcpNo % 8];
    //    SerialUSB.println(digMData[mcpNo].moduleType);

    digMData[mcpNo].mcpState = 0;
    digMData[mcpNo].mcpStatePrev = 0;
    digMData[mcpNo].antMillisScan = millis();
    
    if (nModules > 1) {
      if (mcpNo < nModules - 1) {
        // GET START INDEX FOR EACH MODULE
        switch (digMData[mcpNo].moduleType) {
          case DigitalModuleTypes::ARC41:
          case DigitalModuleTypes::RB41: {
              digMData[mcpNo + 1].digitalIndexStart = digMData[mcpNo].digitalIndexStart + defRB41module.components.nDigital;
            } break;
          case DigitalModuleTypes::RB42: {
              digMData[mcpNo + 1].digitalIndexStart = digMData[mcpNo].digitalIndexStart + defRB42module.components.nDigital;
            } break;
          case DigitalModuleTypes::RB82: {
              digMData[mcpNo + 1].digitalIndexStart = digMData[mcpNo].digitalIndexStart + defRB82module.components.nDigital;
            } break;
          default: break;
        }
      }
    }
    // Initialize all pins, based on which module is in each position    
    for (int i = 0; i < 16; i++) {
      if (digMData[mcpNo].moduleType == DigitalModuleTypes::RB41 || digMData[mcpNo].moduleType == DigitalModuleTypes::ARC41) {
        for ( int j = 0; j < defRB41module.components.nDigital; j++) {
          if (i == defRB41module.rb41pins[j])
            digitalMCP[mcpNo].pullUp(i, HIGH);
        }
      } else if (digMData[mcpNo].moduleType == DigitalModuleTypes::RB42) {
        for ( int j = 0; j < defRB42module.components.nDigital; j++) {
          if (i == defRB42module.rb42pins[j])
            digitalMCP[mcpNo].pullUp(i, HIGH);
        }
      } else if (digMData[mcpNo].moduleType == DigitalModuleTypes::RB82) {
        for (int rowIndex = 0; rowIndex < RB82_ROWS; rowIndex++) {
          digitalMCP[mcpNo].pinMode(defRB82module.rb82pins[ROWS][rowIndex], INPUT);
        }
        for (int colIndex = 0; colIndex < RB82_COLS; colIndex++) {
          digitalMCP[mcpNo].pinMode(defRB82module.rb82pins[COLS][colIndex], OUTPUT);
        }
      }
    }
    digMData[mcpNo].mcpState = digitalMCP[mcpNo].digitalRead();
//        if(mcpNo == 0){
//      SerialUSB.print("MODULE ");SerialUSB.print(mcpNo);SerialUSB.print(": ");
//      for (int i = 0; i < 16; i++) {
//        SerialUSB.print( (digMData[mcpNo].mcpState >> (15 - i)) & 0x01, BIN);
//        if (i == 9 || i == 6) SerialUSB.print(" ");
//      }
//      SerialUSB.print("\n");
//        }
  }
}


void DigitalInputs::Read(void) {
  if (!nBanks || !nDigitals || !nModules) return;  // if no banks, no digital inputs or no modules are configured, exit here
  
  byte nButtonsInModule = 0;
  static byte mcpNo = 0;

  // DIGITAL MODULES ARE SCANNED EVERY "DIGITAL_SCAN_INTERVAL" EACH 
  // BUT EVERY "individualScanInterval" THE PROGRAM READS ONE MODULE
  // IF THERE ARE N MODULES,  individualScanInterval = DIGITAL_SCAN_INTERVAL/N MILLIS
  if (millis() - generalMillis > individualScanInterval) {
    generalMillis = millis();
//    SerialUSB.print("Reading module: "); SerialUSB.print(mcpNo);  SerialUSB.print(" at millis(): "); SerialUSB.print(millis()); 
//    SerialUSB.print("\tElapsed time since last read: "); SerialUSB.println(millis() - digMData[mcpNo].antMillisScan);
    digMData[mcpNo].antMillisScan = millis();
    
    // FOR EACH MODULE IN CONFIG, READ DIFFERENTLY
    if (digMData[mcpNo].moduleType != DigitalModuleTypes::RB82) {   // NOT RB82
      digMData[mcpNo].mcpState = digitalMCP[mcpNo].digitalRead();  // READ ENTIRE MODULE
      
      if ( digMData[mcpNo].mcpState != digMData[mcpNo].mcpStatePrev) {  // if module state changed
        digMData[mcpNo].mcpStatePrev = digMData[mcpNo].mcpState;
        
        // Get number of digital inputs in module
        if (digMData[mcpNo].moduleType == DigitalModuleTypes::RB42) {
          nButtonsInModule = defRB42module.components.nDigital;
        } else {
          nButtonsInModule = defRB41module.components.nDigital;
        }

        for (int nBut = 0; nBut < nButtonsInModule; nBut++) { // check each digital input
          byte indexDigital = digMData[mcpNo].digitalIndexStart + nBut;
          
          byte pin = (digMData[mcpNo].moduleType == DigitalModuleTypes::RB42) ? defRB42module.rb42pins[nBut] :  // get pin for each individual input
                     defRB41module.rb41pins[nBut];

          dHwData[indexDigital].digitalHWState = !((digMData[mcpNo].mcpState >> pin) & 0x0001);  // read the state of each input

          if(digital[indexDigital].actionConfig.message == digitalMessageTypes::digital_msg_none) return;
          
          CheckIfChanged(indexDigital);
        }
      }
    } else {
//      unsigned long antMicrosMatrix = micros();
      // MATRIX MODULES
     // iterate the columns
      for (int colIndex = 0; colIndex < RB82_COLS; colIndex++) {
        // col: set to output to low
        byte colPin = defRB82module.rb82pins[COLS][colIndex];

        digitalMCP[mcpNo].digitalWrite(colPin, LOW);
        
        uint16_t pullUpState = 0;
        // row: interate through the rows
        
        // 1- SETEAR PULLUPS EN LAS FILAS
        for (int rowIndex = 0; rowIndex < RB82_ROWS; rowIndex++) {
          byte rowPin = defRB82module.rb82pins[ROWS][rowIndex];
          pullUpState |= 1<<rowPin;
        }
        digitalMCP[mcpNo].writeWord(GPPUA, pullUpState);   
        
        // 2- Leer estado de la fila completa y almacenar nuevo estado
        digMData[mcpNo].mcpState = digitalMCP[mcpNo].digitalRead();  // READ ENTIRE MODULE
        // row: interate through the rows
        for (int rowIndex = 0; rowIndex < RB82_ROWS; rowIndex++) {
          uint8_t rowPin = defRB82module.rb82pins[ROWS][rowIndex];
          uint8_t mapIndex = defRB82module.buttonMapping[rowIndex][colIndex];
          dHwData[digMData[mcpNo].digitalIndexStart + mapIndex].digitalHWState = !((digMData[mcpNo].mcpState>>rowPin)&0x1);

          // 3- Chequear cambios          
          CheckIfChanged(digMData[mcpNo].digitalIndexStart + mapIndex);
        }
        
        // disable the column
        digitalMCP[mcpNo].digitalWrite(colPin, HIGH);
      }
    }
    mcpNo++;
    if (mcpNo >= nModules)  mcpNo = 0;    // If last module was scanned, start over
  }
}


void DigitalInputs::CheckIfChanged(uint8_t indexDigital) {

  if ( dHwData[indexDigital].digitalHWState != dHwData[indexDigital].digitalHWStatePrev) {             // and bounce time elapsed   // REVIEW BOUNCE ROUTINE
    dHwData[indexDigital].digitalHWStatePrev = dHwData[indexDigital].digitalHWState;
    //                dHwData[indexDigital].bounceOn = true;
    dHwData[indexDigital].swBounceMillisPrev = millis();

    // HW-ID for digital inputs starts after encoders
    if (CheckIfBankShifter(indexDigital+config->inputs.encoderCount, dHwData[indexDigital].digitalHWState)){
      // IF IT IS BANK SHIFTER, RETURN, DON'T DO ACTION FOR THIS SWITCH
      SerialUSB.println("IS SHIFTER");
      return;
    }  
    
//    SerialUSB.print(indexDigital);SerialUSB.print(": ");
//    SerialUSB.print(dHwData[indexDigital].digitalHWState);SerialUSB.println();
    
    if (dHwData[indexDigital].digitalHWState) {
      dBankData[currentBank][indexDigital].digitalInputValue = !dBankData[currentBank][indexDigital].digitalInputValue;

      //      if (indexDigital < nBanks && currentBank != indexDigital ) { // ADD BANK CONDITION
      //        currentBank = memHost->LoadBank(indexDigital);
      //        //                  SerialUSB.println("___________________________");
      //        //                  SerialUSB.print("Loaded Bank: "); SerialUSB.println(currentBank);
      //        //                  SerialUSB.println("___________________________");
      //        feedbackHw.SetBankChangeFeedback();
      //      }else{
      DigitalAction(indexDigital, dBankData[currentBank][indexDigital].digitalInputValue);
      //      }
    } else if (!dHwData[indexDigital].digitalHWState &&
               digital[indexDigital].actionConfig.action != switchActions::switch_toggle) {
      dBankData[currentBank][indexDigital].digitalInputValue = 0;
      DigitalAction(indexDigital, dBankData[currentBank][indexDigital].digitalInputValue);
//      SerialUSB.print("Button "); SerialUSB.print(indexDigital);
//      SerialUSB.print(" : "); SerialUSB.println(dBankData[currentBank][indexDigital].digitalInputValue);
    }
  }
}

void DigitalInputs::SetNextAddress(uint8_t mcpNo, uint8_t addr) {
  for (int i = 0; i < 3; i++) {
    digitalMCP[mcpNo].pinMode(defRB41module.nextAddressPin[i], OUTPUT); 
  }
  for (int i = 0; i < 3; i++) {
    digitalMCP[mcpNo].digitalWrite(defRB41module.nextAddressPin[i], (addr >> i) & 1);
  }
}

void DigitalInputs::DigitalAction(uint16_t index, uint16_t value) {

  for(int bank = 0; bank < config->banks.count; bank++){
    if(currentBank != bank){
      if((32+index) == config->banks.shifterId[bank]){
        currentBank = memHost->LoadBank(bank);
      }
    } 
  }
  
  uint16_t paramToSend = digital[index].actionConfig.parameter[digital_MSB] << 7 |
                         digital[index].actionConfig.parameter[digital_LSB];
  byte channelToSend = digital[index].actionConfig.channel + 1;
  uint16_t minValue = digital[index].actionConfig.parameter[digital_minMSB] << 7 |
                      digital[index].actionConfig.parameter[digital_minLSB];
  uint16_t maxValue = digital[index].actionConfig.parameter[digital_maxMSB] << 7 |
                      digital[index].actionConfig.parameter[digital_maxLSB];

  uint16_t valueToSend = 0;

  if (value) {
    valueToSend = maxValue;
  } else {
    valueToSend = minValue;
  }
//    SerialUSB.print(index);SerialUSB.print(" -> ");
//    SerialUSB.print("Param: ");SerialUSB.print(paramToSend);
//    SerialUSB.print(" Valor: ");SerialUSB.print(valueToSend);
//    SerialUSB.print(" Canal: ");SerialUSB.print(channelToSend);
//    SerialUSB.print(" Min: ");SerialUSB.print(minValue);
//    SerialUSB.print(" Max: ");SerialUSB.println(maxValue);
  switch (digital[index].actionConfig.message) {
    case digitalMessageTypes::digital_msg_note: {
        if (digital[index].actionConfig.midiPort & 0x01)
          MIDI.sendNoteOn( paramToSend & 0x7f, valueToSend & 0x7f, channelToSend);
        if (digital[index].actionConfig.midiPort & 0x02)
          MIDIHW.sendNoteOn( paramToSend & 0x7f, valueToSend & 0x7f, channelToSend);
      } break;
    case digitalMessageTypes::digital_msg_cc: {
        if (digital[index].actionConfig.midiPort & 0x01)
          MIDI.sendControlChange( paramToSend & 0x7f, valueToSend & 0x7f, channelToSend);
        if (digital[index].actionConfig.midiPort & 0x02)
          MIDIHW.sendControlChange( paramToSend & 0x7f, valueToSend & 0x7f, channelToSend);
      } break;
    case digitalMessageTypes::digital_msg_pc: {
        if (digital[index].actionConfig.midiPort & 0x01 && valueToSend != minValue)
          MIDI.sendProgramChange( paramToSend & 0x7f, channelToSend);
        if (digital[index].actionConfig.midiPort & 0x02 && valueToSend)
          MIDIHW.sendProgramChange( paramToSend & 0x7f, channelToSend);
      } break;
    case digitalMessageTypes::digital_msg_pc_m: {
        if (digital[index].actionConfig.midiPort & 0x01) {
          if (currentProgram[midi_usb - 1][channelToSend - 1] > 0 && value) {
            currentProgram[midi_usb - 1][channelToSend - 1]--;
            MIDI.sendProgramChange(currentProgram[midi_usb - 1][channelToSend - 1], channelToSend);
          }
        }
        if (digital[index].actionConfig.midiPort & 0x02) {
          if (currentProgram[midi_hw - 1][channelToSend - 1] > 0 && value) {
            currentProgram[midi_hw - 1][channelToSend - 1]--;
            MIDIHW.sendProgramChange(currentProgram[midi_hw - 1][channelToSend - 1], channelToSend);
          }
        }
      } break;
    case digitalMessageTypes::digital_msg_pc_p: {
        if (digital[index].actionConfig.midiPort & 0x01) {
          if (currentProgram[midi_usb - 1][channelToSend - 1] < 127 && value) {
            currentProgram[midi_usb - 1][channelToSend - 1]++;
            MIDI.sendProgramChange(currentProgram[midi_usb - 1][channelToSend - 1], channelToSend);
          }
        }
        if (digital[index].actionConfig.midiPort & 0x02) {
          if (currentProgram[midi_hw - 1][channelToSend - 1] < 127 && value) {
            currentProgram[midi_hw - 1][channelToSend - 1]++;
            MIDIHW.sendProgramChange(currentProgram[midi_hw - 1][channelToSend - 1], channelToSend);
          }
        }
      } break;
    case digitalMessageTypes::digital_msg_nrpn: {
        if (digital[index].actionConfig.midiPort & 0x01) {
          MIDI.sendControlChange( 99, (paramToSend >> 7) & 0x7F, channelToSend);
          MIDI.sendControlChange( 98, (paramToSend & 0x7F), channelToSend);
          MIDI.sendControlChange( 6, (valueToSend >> 7) & 0x7F, channelToSend);
          MIDI.sendControlChange( 38, (valueToSend & 0x7F), channelToSend);
        }
        if (digital[index].actionConfig.midiPort & 0x02) {
          MIDIHW.sendControlChange( 99, (paramToSend >> 7) & 0x7F, channelToSend);
          MIDIHW.sendControlChange( 98, (paramToSend & 0x7F), channelToSend);
          MIDIHW.sendControlChange( 6, (valueToSend >> 7) & 0x7F, channelToSend);
          MIDIHW.sendControlChange( 38, (valueToSend & 0x7F), channelToSend);
        }
      } break;
    case digitalMessageTypes::digital_msg_rpn: {
        if (digital[index].actionConfig.midiPort & 0x01) {
          MIDI.sendControlChange( 101, (paramToSend >> 7) & 0x7F, channelToSend);
          MIDI.sendControlChange( 100, (paramToSend & 0x7F), channelToSend);
          MIDI.sendControlChange( 6, (valueToSend >> 7) & 0x7F, channelToSend);
          MIDI.sendControlChange( 38, (valueToSend & 0x7F), channelToSend);
        }
        if (digital[index].actionConfig.midiPort & 0x02) {
          MIDIHW.sendControlChange( 101, (paramToSend >> 7) & 0x7F, channelToSend);
          MIDIHW.sendControlChange( 100, (paramToSend & 0x7F), channelToSend);
          MIDIHW.sendControlChange( 6, (valueToSend >> 7) & 0x7F, channelToSend);
          MIDIHW.sendControlChange( 38, (valueToSend & 0x7F), channelToSend);
        }
      } break;
    case digitalMessageTypes::digital_msg_pb: {
        valueToSend = map(valueToSend, minValue, maxValue, -8192, 8191);
        if (digital[index].actionConfig.midiPort & 0x01)
          MIDI.sendPitchBend( valueToSend, channelToSend);
        if (digital[index].actionConfig.midiPort & 0x02)
          MIDIHW.sendPitchBend( valueToSend, channelToSend);
      } break;
    case digitalMessageTypes::digital_msg_key: {
        if (digital[index].actionConfig.parameter[digital_modifier] && value)
          Keyboard.press(digital[index].actionConfig.parameter[digital_modifier]);
        if (digital[index].actionConfig.parameter[digital_key] && value)
          Keyboard.press(digital[index].actionConfig.parameter[digital_key]);
        millisKeyboardPress = millis();
        keyboardReleaseFlag = true;
      } break;
  }
  // STATUS LED SET BLINK
   SetStatusLED(STATUS_BLINK, 1, statusLEDtypes::STATUS_FB_MIDI_OUT);
   
   if ( (digital[index].feedback.source == fb_src_local || digital[index].actionConfig.message == digital_msg_key) && 
        dBankData[currentBank][index].digitalInputValue != dBankData[currentBank][index].digitalInputValuePrev) {
      dBankData[currentBank][index].digitalInputValuePrev = dBankData[currentBank][index].digitalInputValue;
    
     // SET INPUT FEEDBACK
     uint16_t fbValue = 0;
     if(digital[index].feedback.source == fb_src_local && digital[index].feedback.localBehaviour == fb_lb_always_on){
       fbValue = true;
     }else{
       fbValue = dBankData[currentBank][index].digitalInputValue;
     }
     feedbackHw.SetChangeDigitalFeedback(index, fbValue, false);
   }
}

uint16_t DigitalInputs::GetDigitalValue(uint16_t digNo){
  uint16_t fbValue = 0;
  if(digNo < nDigitals){
    if(digital[digNo].feedback.source == fb_src_local && digital[digNo].feedback.localBehaviour == fb_lb_always_on){
      fbValue = digital[digNo].actionConfig.parameter[digital_maxMSB] << 7 |
                digital[digNo].actionConfig.parameter[digital_maxLSB];
    }else{
      fbValue = dBankData[currentBank][digNo].digitalInputValue;
    }   
    return fbValue;
  }   
}

void DigitalInputs::SetDigitalValue(uint8_t bank, uint16_t digNo, uint16_t value){
  uint16_t minValue = 0, maxValue = 0;
    
  minValue = digital[digNo].actionConfig.parameter[digital_minMSB]<<7 | digital[digNo].actionConfig.parameter[digital_minLSB];
  maxValue = digital[digNo].actionConfig.parameter[digital_maxMSB]<<7 | digital[digNo].actionConfig.parameter[digital_maxLSB];
  
  // Don't update value if switch is momentary
  if(digital[digNo].actionConfig.action = switchActions::switch_momentary) 
    dBankData[bank][digNo].digitalInputValue = value;  
  
  //SerialUSB.println("Set Digital Value");
  if (bank == currentBank){
    feedbackHw.SetChangeDigitalFeedback(digNo, value, false);
  }
}

void DigitalInputs::SetPullUps(){
  byte cmd = OPCODEW;
  SPI.beginTransaction(SPISettings(1000000,MSBFIRST,SPI_MODE0));
    digitalWrite(digitalMCPChipSelect1, LOW);
    digitalWrite(digitalMCPChipSelect2, LOW);
    SPI.transfer(cmd);
    SPI.transfer(GPPUA);
    SPI.transfer(0xFF);
    digitalWrite(digitalMCPChipSelect1, HIGH);
    digitalWrite(digitalMCPChipSelect2, HIGH);
  SPI.endTransaction();
  delayMicroseconds(5);
  SPI.beginTransaction(SPISettings(1000000,MSBFIRST,SPI_MODE0));
    digitalWrite(digitalMCPChipSelect1, LOW);
    digitalWrite(digitalMCPChipSelect2, LOW);
    SPI.transfer(cmd);
    SPI.transfer(GPPUB);
    SPI.transfer(0xFF);
    digitalWrite(digitalMCPChipSelect1, HIGH);
    digitalWrite(digitalMCPChipSelect2, HIGH);
  SPI.endTransaction();
}
void DigitalInputs::EnableHWAddress(){
  // ENABLE HARDWARE ADDRESSING MODE FOR ALL CHIPS
  digitalWrite(digitalMCPChipSelect1, HIGH);
  digitalWrite(digitalMCPChipSelect2, HIGH);
  byte cmd = OPCODEW;
  digitalWrite(digitalMCPChipSelect1, LOW);
  digitalWrite(digitalMCPChipSelect2, LOW);
  SPI.transfer(cmd);
  SPI.transfer(IOCONA);
  SPI.transfer(ADDR_ENABLE);
  digitalWrite(digitalMCPChipSelect1, HIGH);
  digitalWrite(digitalMCPChipSelect2, HIGH);
}

void DigitalInputs::DisableHWAddress(){
  byte cmd = 0;
  // DISABLE HARDWARE ADDRESSING FOR ALL CHIPS - ONLY NEEDED FOR RESET
  for (int n = 0; n < nModules; n++) {
    byte chipSelect = 0;
    byte digPort = n / 8;
    byte mcpAddress = n % 8;
    
    if(n<8) chipSelect = digitalMCPChipSelect1;
    else    chipSelect = digitalMCPChipSelect2;
    
    digitalWrite(chipSelect, HIGH);
    cmd = OPCODEW | ((mcpAddress & 0b111) << 1);
    digitalWrite(chipSelect, LOW);
    SPI.transfer(cmd);
    SPI.transfer(IOCONA);
    SPI.transfer(ADDR_DISABLE);
    digitalWrite(chipSelect, HIGH);
  }
}
