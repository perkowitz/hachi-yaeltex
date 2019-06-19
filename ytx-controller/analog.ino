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
    analogData[memHost->currBank][input] = KmBoard.analogReadKm(mux, muxChannel)>>2;         // Read analog value from MUX_A and channel 'input'
    if(!IsNoise(input)){

//        SerialUSB.print(input);SerialUSB.print(" - ");
//        SerialUSB.println(analogData[memHost->currBank][input]);


//      #if defined(SERIAL_COMMS)
//      SerialUSB.print("ANALOG IN "); SerialUSB.print(CCmap[input]);
//      SerialUSB.print(": ");
//      SerialUSB.print(analogData[memHost->currBank][input]);
//      SerialUSB.print("\t");
//      SerialUSB.println("");                                             // New Line
//      #elif defined(MIDI_COMMS)
//      MIDI.controlChange(MIDI_CHANNEL, CCmap[input], analogData[memHost->currBank][input]);   // Channel 0, middle C, normal velocity
//      #endif
    }
  }
}


// Thanks to Pablo Fullana for the help with this function!
// It's just a threshold filter. If the new value stays within the previous value + - the noise threshold set, then it's considered noise
bool AnalogInputs::IsNoise(unsigned int input) {
  if (analogDirection[memHost->currBank][input] == ANALOG_INCREASING){   // CASE 1: If signal is increasing,
    if(analogData[memHost->currBank][input] > analogDataPrev[memHost->currBank][input]){      // and the new value is greater than the previous,
       analogDataPrev[memHost->currBank][input] = analogData[memHost->currBank][input];       // store new value as previous and return
       return 0;                                        // NOT NOISE!
    }
    else if(analogData[memHost->currBank][input] < analogDataPrev[memHost->currBank][input] - NOISE_THRESHOLD){  // If, otherwise, it's lower than the previous value and the noise threshold together,
      analogDirection[memHost->currBank][input] = ANALOG_DECREASING;                           // means it started to decrease,
      analogDataPrev[memHost->currBank][input] = analogData[memHost->currBank][input];                            // so store new value as previous and return
      return 0;                                                             // NOT NOISE!
    }
  }
  if (analogDirection[memHost->currBank][input] == ANALOG_DECREASING){   // CASE 2: If signal is increasing,
    if(analogData[memHost->currBank][input] < analogDataPrev[memHost->currBank][input]){      // and the new value is lower than the previous,
       analogDataPrev[memHost->currBank][input] = analogData[memHost->currBank][input];       // store new value as previous and return
       return 0;                                        // NOT NOISE!
    }
    else if(analogData[memHost->currBank][input] > analogDataPrev[memHost->currBank][input] + NOISE_THRESHOLD){  // If, otherwise, it's greater than the previous value and the noise threshold together,
      analogDirection[memHost->currBank][input] = ANALOG_INCREASING;                            // means it started to increase,
      analogDataPrev[memHost->currBank][input] = analogData[memHost->currBank][input];                             // so store new value as previous and return
      return 0;                                                              // NOT NOISE!
    }
  }
  return 1;                                           // If everyting above was not true, then IT'S NOISE! Pot is trying to fool us. But we owned you pot ;)
}
