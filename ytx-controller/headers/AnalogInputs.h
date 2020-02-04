#ifndef ANALOG_INPUTS_H
#define ANALOG_INPUTS_H

#include "Arduino.h"
#include "Defines.h"
#include "FeedbackClass.h"

//----------------------------------------------------------------------------------------------------
// CLASS DEFINITION
//----------------------------------------------------------------------------------------------------

class AnalogInputs{
private:
  bool IsNoise(uint16_t, uint16_t, uint16_t , byte, bool);
  void FastADCsetup();
  void SelAnalog(uint32_t);
  
  int16_t MuxAnalogRead(uint8_t , uint8_t);
  int16_t MuxDigitalRead(uint8_t, uint8_t, uint8_t);
  int16_t MuxDigitalRead(uint8_t, uint8_t);
  void FilterClear(uint8_t);
  uint16_t FilterGetNewAverage(uint8_t, uint16_t);
  
  // Variables

  uint8_t nBanks;
  uint8_t nAnalog;
  bool begun;
  
  typedef struct{
    uint16_t analogValue;         // Variable to store analog values
    uint16_t analogValuePrev;     // Variable to store previous analog values
  }analogBankData;
  analogBankData **aBankData;

  typedef struct __attribute__((packed)){
    uint16_t analogRawValue : 12;         // Variable to store analog values
    uint8_t analogDirection : 4;            // Variable to store current direction of change
    uint16_t analogRawValuePrev : 12;     // Variable to store previous analog values
    uint8_t analogDirectionRaw : 4;            // Variable to store current direction of change

    // Running average filter variables
    uint8_t filterIndex;            // Indice que recorre los valores del filtro de suavizado
    uint8_t filterCount;
    uint16_t filterSum;
    uint16_t filterSamples[FILTER_SIZE_ANALOG];
  }analogHwData;
  analogHwData *aHwData;

  uint64_t updateValue;
  uint32_t antMillisAnalogUpdate;

  // Address lines for multiplexer
  const int _S0 = (4u);
  const int _S1 = (3u);
  const int _S2 = (8u);
  const int _S3 = (9u);
  // Input signal of multiplexers
  byte muxPin[NUM_MUX] = {MUX_A_PIN, MUX_B_PIN, MUX_C_PIN, MUX_D_PIN};


  // Do not change - These are used to have the inputs and outputs of the headers in order
  byte MuxMapping[NUM_MUX_CHANNELS] =   {1,        // INPUT 0   - Mux channel 2
                                         0,        // INPUT 1   - Mux channel 0
                                         3,        // INPUT 2   - Mux channel 3
                                         2,        // INPUT 3   - Mux channel 1
                                         12,       // INPUT 4   - Mux channel 12
                                         13,       // INPUT 5   - Mux channel 14
                                         14,       // INPUT 6   - Mux channel 13
                                         15,       // INPUT 7   - Mux channel 15
                                         7,        // INPUT 8   - Mux channel 7
                                         4,        // INPUT 9   - Mux channel 4
                                         5,        // INPUT 10  - Mux channel 6
                                         6,        // INPUT 11  - Mux channel 5
                                         10,        // INPUT 12  - Mux channel 9
                                         9,       // INPUT 13  - Mux channel 10
                                         8,        // INPUT 14  - Mux channel 8
                                         11};      // INPUT 15  - Mux channel 11


public:
  void Init(uint8_t,uint8_t);
  void Read();
  uint32_t AnalogReadFast(byte);
  void SendNRPN();
};

#endif