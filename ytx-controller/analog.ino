#include "headers/AnalogInputs.h"

//----------------------------------------------------------------------------------------------------
// ANALOG METHODS
//----------------------------------------------------------------------------------------------------

void AnalogInputs::Init(byte maxBanks, byte maxAnalog){
  nBanks = maxBanks;
  nAnalog = maxAnalog;

  // First dimension is an array of pointers, each pointing to a column - https://www.eskimo.com/~scs/cclass/int/sx9b.html
  analogData = (uint16_t**) memHost->allocateRAM(nBanks*sizeof(uint16_t*));
  analogDataPrev = (uint16_t**) memHost->allocateRAM(nBanks*sizeof(uint16_t*));
  analogDirection = (uint8_t**) memHost->allocateRAM(nBanks*sizeof(uint8_t*));
  for (int b = 0; b < nBanks; b++){
    analogData[b] = (uint16_t*) memHost->allocateRAM(nAnalog * sizeof(uint16_t));
    analogDataPrev[b] = (uint16_t*) memHost->allocateRAM(nAnalog * sizeof(uint16_t));
    analogDirection[b] = (uint8_t*) memHost->allocateRAM(nAnalog * sizeof(uint8_t));
  }
  
  // Set all elements in arrays to 0
  for(int b = 0; b < nBanks; b++){
    for(int i = 0; i < nAnalog; i++){
       analogData[b][i] = i*b+i;
       analogDataPrev[b][i] = i*b+2*i;
       analogDirection[b][i] = b;
       SerialUSB.print(analogData[b][i]); SerialUSB.print("\t");
       SerialUSB.print(analogDataPrev[b][i]); SerialUSB.print("\t");
       SerialUSB.print(analogDirection[b][i]); SerialUSB.print("\n");
    }
    SerialUSB.println();
  }
}


void AnalogInputs::Read(){

  for (int input = 0; input < nAnalog; input++){      // Sweeps all 8 multiplexer inputs of Mux A1 header
    byte mux = input < 16 ? MUX_A : MUX_B;           // MUX 0 or 1
    byte muxChannel = input % NUM_MUX_CHANNELS;         // CHANNEL 0-15
    analogData[0][input] = KmBoard.analogReadKm(mux, muxChannel)>>2;         // Read analog value from MUX_A and channel 'input'
    if(!IsNoise(input)){

//        SerialUSB.print(input);SerialUSB.print(" - ");
//        SerialUSB.println(analogData[0][input]);


//      #if defined(SERIAL_COMMS)
//      SerialUSB.print("ANALOG IN "); SerialUSB.print(CCmap[input]);
//      SerialUSB.print(": ");
//      SerialUSB.print(analogData[0][input]);
//      SerialUSB.print("\t");
//      SerialUSB.println("");                                             // New Line
//      #elif defined(MIDI_COMMS)
//      MIDI.controlChange(MIDI_CHANNEL, CCmap[input], analogData[0][input]);   // Channel 0, middle C, normal velocity
//      #endif
    }
  }
}


// Thanks to Pablo Fullana for the help with this function!
// It's just a threshold filter. If the new value stays within the previous value + - the noise threshold set, then it's considered noise
bool AnalogInputs::IsNoise(unsigned int input) {
  if (analogDirection[0][input] == ANALOG_INCREASING){   // CASE 1: If signal is increasing,
    if(analogData[0][input] > analogDataPrev[0][input]){      // and the new value is greater than the previous,
       analogDataPrev[0][input] = analogData[0][input];       // store new value as previous and return
       return 0;                                        // NOT NOISE!
    }
    else if(analogData[0][input] < analogDataPrev[0][input] - NOISE_THRESHOLD){  // If, otherwise, it's lower than the previous value and the noise threshold together,
      analogDirection[0][input] = ANALOG_DECREASING;                           // means it started to decrease,
      analogDataPrev[0][input] = analogData[0][input];                            // so store new value as previous and return
      return 0;                                                             // NOT NOISE!
    }
  }
  if (analogDirection[0][input] == ANALOG_DECREASING){   // CASE 2: If signal is increasing,
    if(analogData[0][input] < analogDataPrev[0][input]){      // and the new value is lower than the previous,
       analogDataPrev[0][input] = analogData[0][input];       // store new value as previous and return
       return 0;                                        // NOT NOISE!
    }
    else if(analogData[0][input] > analogDataPrev[0][input] + NOISE_THRESHOLD){  // If, otherwise, it's greater than the previous value and the noise threshold together,
      analogDirection[0][input] = ANALOG_INCREASING;                            // means it started to increase,
      analogDataPrev[0][input] = analogData[0][input];                             // so store new value as previous and return
      return 0;                                                              // NOT NOISE!
    }
  }
  return 1;                                           // If everyting above was not true, then IT'S NOISE! Pot is trying to fool us. But we owned you pot ;)
}
