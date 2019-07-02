#include "headers/AnalogInputs.h"

//----------------------------------------------------------------------------------------------------
// ANALOG METHODS
//----------------------------------------------------------------------------------------------------

void AnalogInputs::Init(byte maxBanks, byte maxAnalog){
  nBanks = maxBanks;
  nAnalog = maxAnalog;

  // First dimension is an array of pointers, each pointing to a column - https://www.eskimo.com/~scs/cclass/int/sx9b.html
  analogData = (uint16_t**) memHost->AllocateRAM(nBanks*sizeof(uint16_t*));
  analogDataPrev = (uint16_t**) memHost->AllocateRAM(nBanks*sizeof(uint16_t*));
  analogDirection = (uint8_t**) memHost->AllocateRAM(nBanks*sizeof(uint8_t*));
  for (int b = 0; b < nBanks; b++){
    analogData[b] = (uint16_t*) memHost->AllocateRAM(nAnalog * sizeof(uint16_t));
    analogDataPrev[b] = (uint16_t*) memHost->AllocateRAM(nAnalog * sizeof(uint16_t));
    analogDirection[b] = (uint8_t*) memHost->AllocateRAM(nAnalog * sizeof(uint8_t));
  }
  
  // Set all elements in arrays to 0
  for(int b = 0; b < nBanks; b++){
    for(int i = 0; i < nAnalog; i++){
       analogData[b][i] = 0;
       analogDataPrev[b][i] = 0;
       analogDirection[b][i] = 0;
//       SerialUSB.print(analogData[b][i]); SerialUSB.print("\t");
//       SerialUSB.print(analogDataPrev[b][i]); SerialUSB.print("\t");
//       SerialUSB.print(analogDirection[b][i]); SerialUSB.print("\n");
    }
//    SerialUSB.println();
  }

  // Set output pins for multiplexers
  pinMode(_S0, OUTPUT);
  pinMode(_S1, OUTPUT);
  pinMode(_S2, OUTPUT);
  pinMode(_S3, OUTPUT);
  
  FastADCsetup();
}


void AnalogInputs::Read(){
  for (int input = 0; input < nAnalog; input++){      // Sweeps all 8 multiplexer inputs of Mux A1 header
    byte mux = input < 16 ? MUX_A : MUX_B;           // MUX 0 or 1
    byte muxChannel = input % NUM_MUX_CHANNELS;         // CHANNEL 0-15
    analogData[currentBank][input] = MuxAnalogRead(mux, muxChannel)>>2;         // Read analog value from MUX_A and channel 'input'
    if(!IsNoise(input)){

//        SerialUSB.print(input);SerialUSB.print(" - ");
//        SerialUSB.println(analogData[currentBank][input]);


//      #if defined(SERIAL_COMMS)
//      SerialUSB.print("ANALOG IN "); SerialUSB.print(CCmap[input]);
//      SerialUSB.print(": ");
//      SerialUSB.print(analogData[currentBank][input]);
//      SerialUSB.print("\t");
//      SerialUSB.println("");                                             // New Line
//      #elif defined(MIDI_COMMS)
//      MIDI.controlChange(MIDI_CHANNEL, CCmap[input], analogData[currentBank][input]);   // Channel 0, middle C, normal velocity
//      #endif
    }
  }
}

/*
  Method:         analogReadKm
  Description:    Read the analog value of a multiplexer channel as an analog input.
  Parameters:
                  mux    - Identifier of the multiplexer desired. Must be MUX_A or MUX_B (0 or 1)
                  chan   - Number of the channel desired. Must be within 1 and 16
  Returns:        uint16_t  - 0..1023: analog value read
                            -1: mux or chan is not valid
*/
int16_t AnalogInputs::MuxAnalogRead(int16_t mux, int16_t chan){
    static unsigned int analogData;
    if (chan >= 0 && chan <= 15){     // Re-map hardware channels to have them read in the header order
      if (mux == MUX_A){
        //chan = MuxAMapping[chan];
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


    switch (mux) {
        case MUX_A:
           //analogData = analogRead(InMuxA);
           //analogData = analogRead(InMuxA);
           analogData = AnalogReadFast(InMuxA);
          // analogData = analogReadFast(InMuxA);
            break;
        case MUX_B:
      // analogData = analogRead(InMuxB);
      // analogData = analogRead(InMuxB);
            analogData = AnalogReadFast(InMuxB);
          //  analogData = analogReadFast(InMuxB);
            break;
        case MUX_C:
           //analogData = analogRead(InMuxC);
           //analogData = analogRead(InMuxC);
           analogData = AnalogReadFast(InMuxC);
          // analogData = analogReadFast(InMuxC);
            break;
        case MUX_D:
      // analogData = analogRead(InMuxD);
      // analogData = analogRead(InMuxD);
            analogData = AnalogReadFast(InMuxD);
          //  analogData = analogReadFast(InMuxD);
              
        default:
            break;
    }

  return analogData;
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
int16_t AnalogInputs::MuxDigitalRead(int16_t mux, int16_t chan){
    int16_t digitalState;
    if (chan >= 0 && chan <= 15){
      if (mux == MUX_A)
        chan = MuxAMapping[chan];
      else if (mux == MUX_B)
        chan = MuxBMapping[chan];
      else return -1;   // Return ERROR
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
int16_t AnalogInputs::MuxDigitalRead(int16_t mux, int16_t chan, int16_t pullup){
    int16_t digitalState;
    if (chan >= 0 && chan <= 15){
      if (mux == MUX_A)
        chan = MuxAMapping[chan];
      else if (mux == MUX_B)
        chan = MuxBMapping[chan];
      else return -1;   // Return ERROR
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

// Thanks to Pablo Fullana for the help with this function!
// It's a threshold filter. If the new value stays within the previous value + - the noise threshold set, then it's considered noise
bool AnalogInputs::IsNoise(uint16_t input) {
  if (analogDirection[currentBank][input] == ANALOG_INCREASING){   // CASE 1: If signal is increasing,
    if(analogData[currentBank][input] > analogDataPrev[currentBank][input]){      // and the new value is greater than the previous,
       analogDataPrev[currentBank][input] = analogData[currentBank][input];       // store new value as previous and return
       return 0;                                        // NOT NOISE!
    }
    else if(analogData[currentBank][input] < analogDataPrev[currentBank][input] - NOISE_THRESHOLD){  // If, otherwise, it's lower than the previous value and the noise threshold together,
      analogDirection[currentBank][input] = ANALOG_DECREASING;                           // means it started to decrease,
      analogDataPrev[currentBank][input] = analogData[currentBank][input];                            // so store new value as previous and return
      return 0;                                                             // NOT NOISE!
    }
  }
  if (analogDirection[currentBank][input] == ANALOG_DECREASING){   // CASE 2: If signal is increasing,
    if(analogData[currentBank][input] < analogDataPrev[currentBank][input]){      // and the new value is lower than the previous,
       analogDataPrev[currentBank][input] = analogData[currentBank][input];       // store new value as previous and return
       return 0;                                        // NOT NOISE!
    }
    else if(analogData[currentBank][input] > analogDataPrev[currentBank][input] + NOISE_THRESHOLD){  // If, otherwise, it's greater than the previous value and the noise threshold together,
      analogDirection[currentBank][input] = ANALOG_INCREASING;                            // means it started to increase,
      analogDataPrev[currentBank][input] = analogData[currentBank][input];                             // so store new value as previous and return
      return 0;                                                              // NOT NOISE!
    }
  }
  return 1;                                           // If everyting above was not true, then IT'S NOISE! Pot is trying to fool us. But we owned you pot ;)
}

void AnalogInputs::FastADCsetup() {
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
