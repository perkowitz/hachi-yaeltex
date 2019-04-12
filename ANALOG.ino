
//----------------------------------------------------------------------------------------------------
// ANALOG FUNCTIONS
//----------------------------------------------------------------------------------------------------
void ReadAnalog(){
  for (int input = 0; input < NUM_ANALOG; input++){      // Sweeps all 8 multiplexer inputs of Mux A1 header
    byte mux = input < 16 ? MUX_A : MUX_B;           // MUX 0 or 1
    byte muxChannel = input % NUM_MUX_CHANNELS;         // CHANNEL 0-15
    analogData[input] = KmBoard.analogReadKm(mux, muxChannel)>>2;         // Read analog value from MUX_A and channel 'input'
    if(!IsNoise(input)){

//        SerialUSB.print(input);SerialUSB.print(" - ");
//        SerialUSB.println(analogData[input]);


//      #if defined(SERIAL_COMMS)
//      SerialUSB.print("ANALOG IN "); SerialUSB.print(CCmap[input]);
//      SerialUSB.print(": ");
//      SerialUSB.print(analogData[input]);
//      SerialUSB.print("\t");
//      SerialUSB.println("");                                             // New Line
//      #elif defined(MIDI_COMMS)
//      MidiUSB.controlChange(MIDI_CHANNEL, CCmap[input], analogData[input]);   // Channel 0, middle C, normal velocity
//      #endif
    }
  }
}


// Thanks to Pablo Fullana for the help with this function!
// It's just a threshold filter. If the new value stays within the previous value + - the noise threshold set, then it's considered noise
unsigned int IsNoise(unsigned int input) {
  if (analogDirection[input] == ANALOG_INCREASING){   // CASE 1: If signal is increasing,
    if(analogData[input] > analogDataPrev[input]){      // and the new value is greater than the previous,
       analogDataPrev[input] = analogData[input];       // store new value as previous and return
       return 0;                                        // NOT NOISE!
    }
    else if(analogData[input] < analogDataPrev[input] - NOISE_THRESHOLD){  // If, otherwise, it's lower than the previous value and the noise threshold together,
      analogDirection[input] = ANALOG_DECREASING;                           // means it started to decrease,
      analogDataPrev[input] = analogData[input];                            // so store new value as previous and return
      return 0;                                                             // NOT NOISE!
    }
  }
  if (analogDirection[input] == ANALOG_DECREASING){   // CASE 2: If signal is increasing,
    if(analogData[input] < analogDataPrev[input]){      // and the new value is lower than the previous,
       analogDataPrev[input] = analogData[input];       // store new value as previous and return
       return 0;                                        // NOT NOISE!
    }
    else if(analogData[input] > analogDataPrev[input] + NOISE_THRESHOLD){  // If, otherwise, it's greater than the previous value and the noise threshold together,
      analogDirection[input] = ANALOG_INCREASING;                            // means it started to increase,
      analogDataPrev[input] = analogData[input];                             // so store new value as previous and return
      return 0;                                                              // NOT NOISE!
    }
  }
  return 1;                                           // If everyting above was not true, then IT'S NOISE! Pot is trying to fool us. But we owned you pot ;)
}
