 #include "headers/AnalogInputs.h"

//----------------------------------------------------------------------------------------------------
// ANALOG METHODS
//----------------------------------------------------------------------------------------------------

void AnalogInputs::Init(byte maxBanks, byte maxAnalog){
  nBanks = maxBanks;
  nAnalog = maxAnalog;

  if(!nBanks || !nAnalog) return;  // If number of analog is zero, return;
   
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
  
//  SerialUSB.print(" N ANALOGS: "); SerialUSB.println(nAnalog);
  for (int aInput = 0; aInput < nAnalog; aInput++){      
    if(analog[aInput].message == analogMessageTypes::analog_none) continue;
    
    byte mux = aInput < 16 ? MUX_A :  (aInput < 32 ? MUX_B : ( aInput < 48 ? MUX_C : MUX_D)) ;           // MUX A
    byte muxChannel = aInput % NUM_MUX_CHANNELS;        
    
    if(aInput != 48) continue;
//    SerialUSB.print("MUX: "); SerialUSB.print(mux);
//    SerialUSB.print(" CHANNEL: "); SerialUSB.println(muxChannel);

    aHwData[aInput].analogRawValue = MuxAnalogRead(mux, muxChannel);         // Read analog value from MUX_A and channel 'aInput'
//    SerialUSB.println(aHwData[aInput].analogRawValue);
    if( aHwData[aInput].analogRawValue == aHwData[aInput].analogRawValuePrev ) continue;
//    SerialUSB.print(aHwData[aInput].analogRawValue);SerialUSB.print(",");
//    delay(200);
    if(IsNoise( aHwData[aInput].analogRawValue, 
                aHwData[aInput].analogRawValuePrev, 
                aInput,
                9,
                true))  continue;  
    SerialUSB.println(aHwData[aInput].analogRawValue);
    aHwData[aInput].analogRawValuePrev = aHwData[aInput].analogRawValue;
//    SerialUSB.print(aHwData[aInput].analogRawValue);SerialUSB.print(",");
    FilterGetNewAverage(aInput, aHwData[aInput].analogRawValue);
//    SerialUSB.println(aHwData[aInput].analogRawValue);                
//    SerialUSB.print(aInput); SerialUSB.print(": "); 
    
//    SerialUSB.println("");                
//    
////    SerialUSB.print("Input: "); SerialUSB.print(aInput); SerialUSB.print(" Value: "); SerialUSB.println(aHwData[aInput].analogRawValue);
    
//    
//    aBankData[currentBank][aInput].analogValue = aHwData[aInput].analogRawValue >> 5;
//    
//    aBankData[currentBank][aInput].analogValuePrev = aBankData[currentBank][aInput].analogValue;
//    
//    
//      if(!IsNoise(aHwData[aInput].analogRawValue, 
//                aHwData[aInput].analogRawValuePrev, 
//                aInput,
//                1,
//                false))
        //      #if defined(SERIAL_COMMS)
//        SerialUSB.print("ANALOG IN "); SerialUSB.print(aInput);
//        SerialUSB.print(": ");
//        SerialUSB.print(aBankData[currentBank][aInput].analogValue);
//        SerialUSB.println("");                                             // New Line
  //      #elif defined(MIDI_COMMS)
  //      MIDI.controlChange(MIDI_CHANNEL, CCmap[aInput], analogValue[currentBank][aInput]);   // Channel 0, middle C, normal velocity
  //      #endif 
    
  }
//  SerialUSB.println();
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
  
  if (aHwData[input].filterIndex == FILTER_SIZE) aHwData[input].filterIndex = 0;  // faster than %
  // update count as last otherwise if( _cnt == 0) above will fail
  if (aHwData[input].filterCount < FILTER_SIZE)
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
  for (uint8_t i = 0; i < FILTER_SIZE; i++) {
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
    if (chan >= 0 && chan <= 15){     
      chan = MuxMapping[chan];      // Re-map hardware channels to have them read in the header order
//      SerialUSB.print(" Mux Channel "); SerialUSB.println(chan);
      if (mux == MUX_A){
        pinMode(InMuxA, INPUT);
      }
      else if (mux == MUX_B){
        pinMode(InMuxB, INPUT);
        //chan = MuxBMapping[chan];
      }
      else if (mux == MUX_C){
        pinMode(InMuxC, INPUT);
        //chan = MuxBMapping[chan];
      }
      else if (mux == MUX_D){
        pinMode(InMuxD, INPUT);
        //chan = MuxBMapping[chan];
      }
      else return -1;     // Return ERROR
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
    switch (mux) {
        case MUX_A:{
//           analogADCdata = analogRead(InMuxA);
//           analogADCdata = analogRead(InMuxA);
//           analogADCdata = AnalogReadFast(InMuxA);
           
           analogADCdata = AnalogReadFast(InMuxA);
        }break;
        case MUX_B:{
          // analogADCdata = analogRead(InMuxB);
          // analogADCdata = analogRead(InMuxB);
            analogADCdata = AnalogReadFast(InMuxB);
          //  analogADCdata = analogReadFast(InMuxB);
        }break;
        case MUX_C:{
           //analogValue = analogRead(InMuxC);
           //analogValue = analogRead(InMuxC);
           analogADCdata = AnalogReadFast(InMuxC);
          // analogValue = analogReadFast(InMuxC);
        }break;
        case MUX_D:{
          // analogValue = analogRead(InMuxD);
          // analogValue = analogRead(InMuxD);
            analogADCdata = AnalogReadFast(InMuxD);
          //  analogValue = analogReadFast(InMuxD);
        }break;
        default:
            break;
    }
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
    if (chan >= 0 && chan <= 15){
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

    switch (mux) {
        case MUX_A:
            digitalState = digitalRead(InMuxA);
            break;
        case MUX_B:
            digitalState = digitalRead(InMuxB);
            break;

        default:
            break;
    }
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
    if (chan >= 0 && chan <= 15){
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


    switch (mux) {
        case MUX_A:
            pinMode(MUX_A_PIN, INPUT);                // These two lines set the analog input pullup resistor
            digitalWrite(MUX_A_PIN, HIGH);
            digitalState = digitalRead(InMuxA);     // Read mux pin
            break;
        case MUX_B:
            pinMode(MUX_B_PIN, INPUT);                // These two lines set the analog input pullup resistor
            digitalWrite(MUX_B_PIN, HIGH);
            digitalState = digitalRead(InMuxB);     // Read mux pin
            break;

        default:
            break;
    }
    return digitalState;
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
}
//##############################################################################



void AnalogInputs::FastADCsetup() {
  const byte gClk = 3; //used to define which generic clock we will use for ADC
  const int cDiv = 1; //divide factor for generic clock
  
  //Input control register
  ADCsync();
  ADC->INPUTCTRL.bit.GAIN = ADC_INPUTCTRL_GAIN_1X_Val;      // Gain select as 1X
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
