 #include "headers/AnalogInputs.h"

//----------------------------------------------------------------------------------------------------
// ANALOG METHODS
//----------------------------------------------------------------------------------------------------

void AnalogInputs::Init(byte maxBanks, byte maxAnalog){

  if(!maxBanks || !maxAnalog) return;  // If number of analog is zero, return;
  
  byte analogInConfig = 0;
  
  // CHECK WHETHER AMOUNT OF DIGITAL INPUTS IN MODULES COMBINED MATCH THE AMOUNT OF DIGITAL INPUTS IN CONFIG
  // AMOUNT OF DIGITAL MODULES
  for (int nPort = 0; nPort < ANALOG_PORTS; nPort++) {
    for (int nMod = 0; nMod < ANALOG_MODULES_PER_PORT; nMod++) {
      //        SerialUSB.println(config->hwMapping.digital[nPort][nMod]);
      if (config->hwMapping.analog[nPort][nMod]) {
        modulesInConfig.analog++;
      }
      switch (config->hwMapping.analog[nPort][nMod]) {
        case AnalogModuleTypes::P41:
        case AnalogModuleTypes::F41: {
            analogInConfig += defP41module.nAnalog;
            nMod++;                                     // A module of 4 components takes two spaces, so jump one
          } break;
        case AnalogModuleTypes::JAL:
        case AnalogModuleTypes::JAF: {
            analogInConfig += defJAFmodule.nAnalog;
          } break;
        default: break;
      }
    }
  }
  if (analogInConfig != maxAnalog) {
    SerialUSB.println("Error in config: Number of analog does not match modules in config");
    SerialUSB.print("nAnalog: "); SerialUSB.println(maxAnalog);
    SerialUSB.print("Modules: "); SerialUSB.println(analogInConfig);
    return;
  } else {
    SerialUSB.println("nAnalog and module config match");
  }

  nBanks = maxBanks;
  nAnalog = maxAnalog;

  // First dimension is an array of pointers, each pointing to a column - https://www.eskimo.com/~scs/cclass/int/sx9b.html
  aBankData = (analogBankData**) memHost->AllocateRAM(nBanks*sizeof(analogBankData*));
  aHwData = (analogHwData*) memHost->AllocateRAM(nAnalog*sizeof(analogHwData));
  
  for (int b = 0; b < nBanks; b++){
    aBankData[b] = (analogBankData*) memHost->AllocateRAM(nAnalog*sizeof(analogBankData));
  }
  
  // Set all elements in arrays to 0
  for(int b = 0; b < nBanks; b++){
    for(int i = 0; i < nAnalog; i++){
       aBankData[b][i].analogValue = 0;
       aBankData[b][i].analogValuePrev = 0;
    }
  }
  for(int i = 0; i < nAnalog; i++){
     aHwData[i].analogRawValue = 0;
     aHwData[i].analogRawValuePrev = 0;
     aHwData[i].analogDirection = ANALOG_INCREASING;
     aHwData[i].analogDirectionRaw = ANALOG_INCREASING;
     FilterClear(i);
  }
    
  // Set output pins for multiplexers
  pinMode(_S0, OUTPUT);
  pinMode(_S1, OUTPUT);
  pinMode(_S2, OUTPUT);
  pinMode(_S3, OUTPUT);
  
  FastADCsetup();
}


void AnalogInputs::Read(){
  if(!nBanks || !nAnalog) return;  // If number of analog is zero, return;

  if(priorityCount && (millis()-priorityTime > 500)){
    priorityCount = 0;
//    SerialUSB.print("Priority list emptied");
  }
  
  int aInput = 0;
  int nAnalogInMod = 0;
  for (int nPort = 0; nPort < ANALOG_PORTS; nPort++) {
    for (int nMod = 0; nMod < ANALOG_MODULES_PER_PORT; nMod++) {
      
      switch(config->hwMapping.analog[nPort][nMod]){
        case AnalogModuleTypes::P41:
        case AnalogModuleTypes::F41: {
          nAnalogInMod = defP41module.nAnalog;    // both have 4 components
        } break;
        case AnalogModuleTypes::JAL:
        case AnalogModuleTypes::JAF: {
          nAnalogInMod = defJAFmodule.nAnalog;    // both have 2 components
        }break;
        default:
          continue;
        break;
      }
      
      for(int a = 0; a < nAnalogInMod; a++){
        aInput = nPort*ANALOGS_PER_PORT + nMod*ANALOG_MODULES_PER_MOD + a;
    
        if(analog[aInput].message == analogMessageTypes::analog_none) continue;
         
//        if(priorityCount && !IsInPriority(aInput))  continue;
            
        byte mux = aInput < 16 ? MUX_A :  (aInput < 32 ? MUX_B : ( aInput < 48 ? MUX_C : MUX_D)) ;           // MUX A
        byte muxChannel = aInput % NUM_MUX_CHANNELS;        
    
    //    if(aInput != 48) continue;
        unsigned long antMicrosAvg = micros(); 
        aHwData[aInput].analogRawValue = MuxAnalogRead(mux, muxChannel);         // Read analog value from MUX_A and channel 'aInput'
//        SerialUSB.println(micros()-antMicrosAvg);
        
        if( aHwData[aInput].analogRawValue == aHwData[aInput].analogRawValuePrev ) continue;
    
        if(IsNoise( aHwData[aInput].analogRawValue, 
                    aHwData[aInput].analogRawValuePrev, 
                    aInput,
                    NOISE_THRESHOLD_RAW,
                    true))  continue;
    
        aHwData[aInput].analogRawValuePrev = aHwData[aInput].analogRawValue;
      
        aHwData[aInput].analogRawValue = FilterGetNewAverage(aInput, aHwData[aInput].analogRawValue);   
//        SerialUSB.print(aInput); SerialUSB.print(": "); 
//        SerialUSB.print(aHwData[aInput].analogRawValue);SerialUSB.println("");          
            
    //    SerialUSB.print(aInput); SerialUSB.print(": "); 
    //    SerialUSB.print(aHwData[aInput].analogRawValue);SerialUSB.println("");  
        
        uint16_t paramToSend = analog[aInput].parameter[analog_MSB]<<7 | analog[aInput].parameter[analog_LSB];
        byte channelToSend = analog[aInput].channel + 1;
        uint16_t minValue = analog[aInput].parameter[analog_minMSB]<<7 | analog[aInput].parameter[analog_minLSB];
        uint16_t maxValue = analog[aInput].parameter[analog_maxMSB]<<7 | analog[aInput].parameter[analog_maxLSB];
    
        #define RAW_LIMIT_LOW   20
        #define RAW_LIMIT_HIGH  4080
        
        uint16_t constrainedValue = constrain(aHwData[aInput].analogRawValue, RAW_LIMIT_LOW, RAW_LIMIT_HIGH);
        
        aBankData[currentBank][aInput].analogValue = mapl(constrainedValue,
                                                          RAW_LIMIT_LOW,
                                                          RAW_LIMIT_HIGH,
                                                          minValue,
                                                          maxValue);            
        
    //    if(aInput != 48) return;
    //    SerialUSB.print("PREV: "); SerialUSB.print(aBankData[currentBank][aInput].analogValuePrev);
    //    SerialUSB.print(" CURR: "); SerialUSB.println(aBankData[currentBank][aInput].analogValue);
        
        if(analog[aInput].message == analog_nrpn || analog[aInput].message == analog_rpn || analog[aInput].message == analog_pb){
          int maxMinDiff = maxValue - minValue;
          byte noiseTh14 = abs(maxMinDiff) >> 8;          // divide range to get noise threshold. Max th is 127/4 = 64 : Min th is 0.     
          if(IsNoise( aBankData[currentBank][aInput].analogValue, 
                      aBankData[currentBank][aInput].analogValuePrev, 
                      aInput,
                      noiseTh14,
                      false))  continue;
        }
        
        if(aBankData[currentBank][aInput].analogValue != aBankData[currentBank][aInput].analogValuePrev){
          AddToPriority(aInput);
          
          aBankData[currentBank][aInput].analogValuePrev = aBankData[currentBank][aInput].analogValue;
          
          uint16_t valueToSend = aBankData[currentBank][aInput].analogValue;
          
//          SerialUSB.print(aInput); SerialUSB.print(": "); 
//          SerialUSB.print(aBankData[currentBank][aInput].analogValue);SerialUSB.println("");          


//          SerialUSB.println(analog[aInput].midiPort);
          
//          switch(analog[aInput].message){
//            case analogMessageTypes::analog_note:{
//              if(analog[aInput].midiPort & 0x01)
//                MIDI.sendNoteOn( paramToSend&0x7f, valueToSend&0x7f, channelToSend);
//              if(analog[aInput].midiPort & 0x02)
//                MIDIHW.sendNoteOn( paramToSend&0x7f, valueToSend&0x7f, channelToSend);
//            }break;
//            case analogMessageTypes::analog_cc:{
//              if(analog[aInput].midiPort & 0x01)
//                MIDI.sendControlChange( paramToSend&0x7f, valueToSend&0x7f, channelToSend);
//              if(analog[aInput].midiPort & 0x02)
//                MIDIHW.sendControlChange( paramToSend&0x7f, valueToSend&0x7f, channelToSend);
//            }break;
//            case analogMessageTypes::analog_pc:{
//              if(analog[aInput].midiPort & 0x01)
//                MIDI.sendProgramChange( valueToSend&0x7f, channelToSend);
//              if(analog[aInput].midiPort & 0x02)
//                MIDIHW.sendProgramChange( valueToSend&0x7f, channelToSend);
//            }break;
//            case analogMessageTypes::analog_nrpn:{
//              if(analog[aInput].midiPort & 0x01){
//                MIDI.sendControlChange( 99, (paramToSend >> 7) & 0x7F, channelToSend);
//                MIDI.sendControlChange( 98, (paramToSend & 0x7F), channelToSend);
//                MIDI.sendControlChange( 6, (valueToSend >> 7) & 0x7F, channelToSend);
//                MIDI.sendControlChange( 38, (valueToSend & 0x7F), channelToSend);       
//              }
//              if(analog[aInput].midiPort & 0x02){
//                MIDIHW.sendControlChange( 99, (paramToSend >> 7) & 0x7F, channelToSend);
//                MIDIHW.sendControlChange( 98, (paramToSend & 0x7F), channelToSend);
//                MIDIHW.sendControlChange( 6, (valueToSend >> 7) & 0x7F, channelToSend);
//                MIDIHW.sendControlChange( 38, (valueToSend & 0x7F), channelToSend);    
//              }
//            }break;
//            case analogMessageTypes::analog_rpn:{
//              if(analog[aInput].midiPort & 0x01){
//                MIDI.sendControlChange( 101, (paramToSend >> 7) & 0x7F, channelToSend);
//                MIDI.sendControlChange( 100, (paramToSend & 0x7F), channelToSend);
//                MIDI.sendControlChange( 6, (valueToSend >> 7) & 0x7F, channelToSend);
//                MIDI.sendControlChange( 38, (valueToSend & 0x7F), channelToSend);       
//              }
//              if(analog[aInput].midiPort & 0x02){
//                MIDIHW.sendControlChange( 101, (paramToSend >> 7) & 0x7F, channelToSend);
//                MIDIHW.sendControlChange( 100, (paramToSend & 0x7F), channelToSend);
//                MIDIHW.sendControlChange( 6, (valueToSend >> 7) & 0x7F, channelToSend);
//                MIDIHW.sendControlChange( 38, (valueToSend & 0x7F), channelToSend);    
//              }
//            }break;
//            case analogMessageTypes::analog_pb:{
//              valueToSend = map(valueToSend, minValue, maxValue, -8192, 8191);
//              if(analog[aInput].midiPort & 0x01)
//                MIDI.sendPitchBend( valueToSend, channelToSend);    
//              if(analog[aInput].midiPort & 0x02)
//                MIDIHW.sendPitchBend( valueToSend, channelToSend);    
//            }break;
//            case analogMessageTypes::analog_ks:{
//              if(analog[aInput].parameter[analog_modifier])
//                Keyboard.press(analog[aInput].parameter[analog_modifier]);
//              if(analog[aInput].parameter[analog_key])
//                Keyboard.press(analog[aInput].parameter[analog_key]);
//              
//              millisKeyboardPress = millis();
//              keyboardReleaseFlag = true; 
//            }break;
//          }
          SetStatusLED(STATUS_BLINK, 1, statusLEDtypes::STATUS_FB_INPUT_CHANGED);
    //      SerialUSB.println(micros()-antMicrosAvg);
        }
        
      }
      if (config->hwMapping.analog[nPort][nMod] == AnalogModuleTypes::P41 ||
          config->hwMapping.analog[nPort][nMod] == AnalogModuleTypes::F41){
        nMod++;     
      }      
    }
  }
//  SerialUSB.print(" N ANALOGS: "); SerialUSB.println(nAnalog);
  for (int aInput = 0; aInput < nAnalog; aInput++){      
        
    
  }
}

// Thanks to Pablo Fullana for the help with this function!
// It's a threshold filter. If the new value stays within the previous value + - the noise threshold set, then it's considered noise
bool AnalogInputs::IsNoise(uint16_t currentValue, uint16_t prevValue, uint16_t input, byte noiseTh, bool raw) {
  bool directionOfChange = (raw  ?  aHwData[input].analogDirectionRaw : 
                                    aHwData[input].analogDirection);
  
//  SerialUSB.print(input); SerialUSB.print(": "); SerialUSB.println(directionOfChange);
//  SerialUSB.print(input); SerialUSB.print(": "); SerialUSB.println(aHwData[input].analogDirectionRaw);
//  SerialUSB.println();
  
  if (directionOfChange == ANALOG_INCREASING){   // CASE 1: If signal is increasing,
    if(currentValue >  prevValue){      // and the new value is greater than the previous,
       return 0;                                        // NOT NOISE!
    }
    else if(currentValue < (prevValue - noiseTh)){  // If, otherwise, it's lower than the previous value and the noise threshold together,
      if(raw)  aHwData[input].analogDirectionRaw = ANALOG_DECREASING;    // means it started to decrease,
      else     aHwData[input].analogDirection    = ANALOG_DECREASING;
      return 0;                                                             // NOT NOISE!
    }
  }
  if (directionOfChange == ANALOG_DECREASING){   // CASE 2: If signal is increasing,
    if(currentValue < prevValue){      // and the new value is lower than the previous,
       return 0;                                        // NOT NOISE!
    }
    else if(currentValue > (prevValue + noiseTh)){  // If, otherwise, it's greater than the previous value and the noise threshold together,
      if(raw)  aHwData[input].analogDirectionRaw = ANALOG_INCREASING; // means it started to increase,
      else     aHwData[input].analogDirection    = ANALOG_INCREASING;
      return 0;                                                              // NOT NOISE!
    }
  }
  return 1;                                           // If everyting above was not true, then IT'S NOISE! Pot is trying to fool us. But we owned you pot ;)
}

/*
    Filtro de media móvil para el sensor de ultrasonido (librería RunningAverage integrada) (http://playground.arduino.cc/Main/RunningAverage)
*/
uint16_t AnalogInputs::FilterGetNewAverage(uint8_t input, uint16_t newVal) {
  aHwData[input].filterSum -= aHwData[input].filterSamples[aHwData[input].filterIndex];
  aHwData[input].filterSamples[aHwData[input].filterIndex] = newVal;
  aHwData[input].filterSum += aHwData[input].filterSamples[aHwData[input].filterIndex];
  
  aHwData[input].filterIndex++;
  
  if (aHwData[input].filterIndex == FILTER_SIZE_ANALOG) aHwData[input].filterIndex = 0;  // faster than %
  // update count as last otherwise if( _cnt == 0) above will fail
  if (aHwData[input].filterCount < FILTER_SIZE_ANALOG)
    aHwData[input].filterCount++;
    
  if (aHwData[input].filterCount == 0)
    return NAN;
    
  return aHwData[input].filterSum / aHwData[input].filterCount;
}

/*
   Limpia los valores del filtro de media móvil para un nuevo uso.
*/
void AnalogInputs::FilterClear(uint8_t input) {
  aHwData[input].filterCount = 0;
  aHwData[input].filterIndex = 0;
  aHwData[input].filterSum = 0;
  for (uint8_t i = 0; i < FILTER_SIZE_ANALOG; i++) {
    aHwData[input].filterSamples[i] = 0; // keeps addValue simpler
  }
}

/*
  Method:         MuxAnalogRead
  Description:    Read the analog value of a multiplexer channel as an analog input.
  Parameters:
                  mux    - Identifier of the multiplexer desired. Must be MUX_A or MUX_B (0 or 1)
                  chan   - Number of the channel desired. Must be within 1 and 16
  Returns:        uint16_t  - 0..1023: analog value read
                            -1: mux or chan is not valid
*/
int16_t AnalogInputs::MuxAnalogRead(uint8_t mux, uint8_t chan){
    static unsigned int analogADCdata;
    if (chan >= 0 && chan <= 15 && mux < NUM_MUX){     
      chan = MuxMapping[chan];      // Re-map hardware channels to have them read in the header order
//      SerialUSB.print(" Mux Channel "); SerialUSB.println(chan);
      pinMode(muxPin[mux], INPUT);
    }
    else return -1;       // Return ERROR

  // Select channel
  bitRead(chan, 0) ?  PORT->Group[g_APinDescription[_S0].ulPort].OUTSET.reg = (1ul << g_APinDescription[_S0].ulPin) :
            PORT->Group[g_APinDescription[_S0].ulPort].OUTCLR.reg = (1ul << g_APinDescription[_S0].ulPin) ;
  bitRead(chan, 1) ?  PORT->Group[g_APinDescription[_S1].ulPort].OUTSET.reg = (1ul << g_APinDescription[_S1].ulPin) :
            PORT->Group[g_APinDescription[_S1].ulPort].OUTCLR.reg = (1ul << g_APinDescription[_S1].ulPin) ;
  bitRead(chan, 2) ?  PORT->Group[g_APinDescription[_S2].ulPort].OUTSET.reg = (1ul << g_APinDescription[_S2].ulPin) :
            PORT->Group[g_APinDescription[_S2].ulPort].OUTCLR.reg = (1ul << g_APinDescription[_S2].ulPin) ;
  bitRead(chan, 3) ?  PORT->Group[g_APinDescription[_S3].ulPort].OUTSET.reg = (1ul << g_APinDescription[_S3].ulPin) :
            PORT->Group[g_APinDescription[_S3].ulPort].OUTCLR.reg = (1ul << g_APinDescription[_S3].ulPin) ;

//    unsigned long antMicrosAread = micros();
    analogADCdata = AnalogReadFast(muxPin[mux]);
//    SerialUSB.println(micros()-antMicrosAread);
  return analogADCdata;
}

/*
  Method:         digitalReadKm
  Description:    Read the state of a multiplexer channel as a digital input.
  Parameters:
                  mux    - Identifier of the multiplexer desired. Must be MUX_A or MUX_B (0 or 1)
                  chan   - Number of the channel desired. Must be within 1 and 16
  Returns:        int    - 0: input is LOW
                           1: input is HIGH
                          -1: mux or chan is not valid
*/
int16_t AnalogInputs::MuxDigitalRead(uint8_t mux, uint8_t chan){
    int16_t digitalState;
    if (chan >= 0 && chan <= 15 && mux < NUM_MUX){
      chan = MuxMapping[chan];
    }
    else return -1;     // Return ERROR

  // Select channel
  bitRead(chan, 0) ?  PORT->Group[g_APinDescription[_S0].ulPort].OUTSET.reg = (1ul << g_APinDescription[_S0].ulPin) :
            PORT->Group[g_APinDescription[_S0].ulPort].OUTCLR.reg = (1ul << g_APinDescription[_S0].ulPin) ;
  bitRead(chan, 1) ?  PORT->Group[g_APinDescription[_S1].ulPort].OUTSET.reg = (1ul << g_APinDescription[_S1].ulPin) :
            PORT->Group[g_APinDescription[_S1].ulPort].OUTCLR.reg = (1ul << g_APinDescription[_S1].ulPin) ;
  bitRead(chan, 2) ?  PORT->Group[g_APinDescription[_S2].ulPort].OUTSET.reg = (1ul << g_APinDescription[_S2].ulPin) :
            PORT->Group[g_APinDescription[_S2].ulPort].OUTCLR.reg = (1ul << g_APinDescription[_S2].ulPin) ;
  bitRead(chan, 3) ?  PORT->Group[g_APinDescription[_S3].ulPort].OUTSET.reg = (1ul << g_APinDescription[_S3].ulPin) :
            PORT->Group[g_APinDescription[_S3].ulPort].OUTCLR.reg = (1ul << g_APinDescription[_S3].ulPin) ;

    digitalState = digitalRead(muxPin[mux]);
    
    return digitalState;
}

/*
  Method:         digitalReadKm
  Description:    Read the state of a multiplexer channel as a digital input, setting the analog input of the Arduino with a pullup resistor.
  Parameters:
                  mux    - Identifier for the desired multiplexer. Must be MUX_A or MUX_B (0 or 1)
                  chan   - Number for the desired channel. Must be within 1 and 16
  Returns:        int    - 0: input is LOW
                           1: input is HIGH
                          -1: mux or chan is not valid
*/
int16_t AnalogInputs::MuxDigitalRead(uint8_t mux, uint8_t chan, uint8_t pullup){
    int16_t digitalState;
    if (chan >= 0 && chan <= 15 && mux < NUM_MUX){
      chan = MuxMapping[chan];
    }
    else return -1;     // Return ERROR

  // SET PIN HIGH
  // PORT->Group[g_APinDescription[YOUR_PIN].ulPort].OUTSET.reg = (1ul << g_APinDescription[YOUR_PIN].ulPin) ;
  // SET PIN LOW
  // PORT->Group[g_APinDescription[YOUR_PIN].ulPort].OUTCLR.reg = (1ul << g_APinDescription[YOUR_PIN].ulPin) ;

  // Select channel
  bitRead(chan, 0) ?  PORT->Group[g_APinDescription[_S0].ulPort].OUTSET.reg = (1ul << g_APinDescription[_S0].ulPin) :
            PORT->Group[g_APinDescription[_S0].ulPort].OUTCLR.reg = (1ul << g_APinDescription[_S0].ulPin) ;
  bitRead(chan, 1) ?  PORT->Group[g_APinDescription[_S1].ulPort].OUTSET.reg = (1ul << g_APinDescription[_S1].ulPin) :
            PORT->Group[g_APinDescription[_S1].ulPort].OUTCLR.reg = (1ul << g_APinDescription[_S1].ulPin) ;
  bitRead(chan, 2) ?  PORT->Group[g_APinDescription[_S2].ulPort].OUTSET.reg = (1ul << g_APinDescription[_S2].ulPin) :
            PORT->Group[g_APinDescription[_S2].ulPort].OUTCLR.reg = (1ul << g_APinDescription[_S2].ulPin) ;
  bitRead(chan, 3) ?  PORT->Group[g_APinDescription[_S3].ulPort].OUTSET.reg = (1ul << g_APinDescription[_S3].ulPin) :
            PORT->Group[g_APinDescription[_S3].ulPort].OUTCLR.reg = (1ul << g_APinDescription[_S3].ulPin) ;


    pinMode(muxPin[mux], INPUT);                // These two lines set the analog input pullup resistor
    digitalWrite(muxPin[mux], HIGH);
    digitalState = digitalRead(muxPin[mux]);     // Read mux pin

    return digitalState;
}

bool AnalogInputs::IsInPriority(byte analogN){
  for(int p = 0; p < priorityCount; p++){
    if (analogN == priorityList[p]) return true;
  }
  return false;
}

void AnalogInputs::AddToPriority(byte analogN){
  if (!priorityCount){
    priorityCount++;
    priorityList[0] = analogN;
    priorityTime = millis();
  }
  else if(priorityCount == 1 && analogN != priorityList[0]){
    priorityCount++;
    priorityList[1] = analogN;
    priorityTime = millis();
  }
}

#if defined(__arm__)
//##############################################################################
// Fast analogue read analogReadFast()
// This is a stripped down version of analogRead() where the set-up commands
// are executed during setup()
// ulPin is the analog input pin number to be read.
//  Mk. 2 - has some more bits removed for speed up
//##############################################################################
uint32_t AnalogInputs::AnalogReadFast(byte ADCpin) {
  SelAnalog(ADCpin);
  ADC->INTFLAG.bit.RESRDY = 1;              // Data ready flag cleared
  ADCsync();
  ADC->SWTRIG.bit.START = 1;                // Start ADC conversion
  while ( ADC->INTFLAG.bit.RESRDY == 0 );   // Wait till conversion done
  ADCsync();
  uint32_t valueRead = ADC->RESULT.reg;     // read the result
  ADCsync();
  ADC->SWTRIG.reg = 0x01;                    //  and flush for good measure
  return valueRead;
  
//  REG_ADC_CTRLA = 2;
//  //REG_ADC_INPUTCTRL = 0x0F001800;
//  //REG_ADC_INPUTCTRL = 0x00001900;
////  ADC->INPUTCTRL.bit.MUXPOS = g_APinDescription[ADCpin].ulADCChannelNumber; // Selection for the positive ADC input
//  REG_ADC_SWTRIG = 2;
//  while (!(REG_ADC_INTFLAG & 1));
//  ADCsync();
//  uint32_t valueRead = ADC->RESULT.reg;     // read the result
//  ADCsync();
//  ADC->SWTRIG.reg = 0x01;                    //  and flush for good measure
//  
////  uint32_t valueRead = ADC->RESULT.reg;     // read the result
////  float v = 3.3*((float)valueRead)/4096;
////  SerialUSB.print(valueRead);
////  SerialUSB.print(" ");
////  SerialUSB.print(v,3);
////  SerialUSB.println();
// 
//  return REG_ADC_RESULT;
}
//##############################################################################



void AnalogInputs::FastADCsetup() {
  const byte gClk = 3; //used to define which generic clock we will use for ADC
  const int cDiv = 1; //divide factor for generic clock
  
  //Input control register
  ADCsync();
  ADC->INPUTCTRL.bit.GAIN = ADC_INPUTCTRL_GAIN_1X_Val;      // Gain select as 1X
//  ADC->INPUTCTRL.bit.GAIN = ADC_INPUTCTRL_GAIN_DIV2_Val;  // Gain select as 1/2X - When AREF is VCCINT (default)
  //Set ADC reference source
  ADCsync();
  ADC->REFCTRL.bit.REFSEL = ADC_REFCTRL_REFSEL_AREFA_Val;
  // Set sample length and averaging
  ADCsync();
  ADC->AVGCTRL.reg = 0x00 ;       //Single conversion no averaging
  ADCsync();
  ADC->SAMPCTRL.reg = 0x00;       //Minimal sample length is 1/2 CLK_ADC cycle
  //Control B register
  ADCsync();
  ADC->CTRLB.reg =  PRESCALER_32 | RESOL_12BIT; // Prescale 64, 12 bit resolution, single conversion
  /* ADC->CTRLB.reg &= 0b1111100011111111;          // mask PRESCALER bits
  ADC->CTRLB.reg |= ADC_CTRLB_PRESCALER_DIV16;   // divide Clock by 64 */
  // Enable ADC in control B register
  ADCsync();
  ADC->CTRLA.bit.ENABLE = 0x01;

 //Enable interrupts
  // ADC->INTENSET.reg |= ADC_INTENSET_RESRDY; // enable Result Ready ADC interrupts
  // ADCsync();

   // NVIC_EnableIRQ(ADC_IRQn); // enable ADC interrupts
   // NVIC_SetPriority(ADC_IRQn, 1); //set priority of the interrupt - higher number -> lower priority
}


//###################################################################################
// ADC select here
//
//###################################################################################

void AnalogInputs::SelAnalog(uint32_t ulPin){      // Selects the analog input channel in INPCTRL
  ADCsync();
  ADC->INPUTCTRL.bit.MUXPOS = g_APinDescription[ulPin].ulADCChannelNumber; // Selection for the positive ADC input
}

static void ADCsync() {
  while (ADC->STATUS.bit.SYNCBUSY == 1); //Just wait till the ADC is free
}
#endif
