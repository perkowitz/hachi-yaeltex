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
  uint32_t AnalogReadFast(byte);
  int16_t MuxAnalogRead(uint8_t , uint8_t);
  int16_t MuxDigitalRead(uint8_t, uint8_t, uint8_t);
  int16_t MuxDigitalRead(uint8_t, uint8_t);
  void FilterClear(uint8_t);
  uint16_t FilterGetNewAverage(uint8_t, uint16_t);

  // Variables

  uint8_t nBanks;
  uint8_t nAnalog;
  
  typedef struct{
    uint16_t analogValue;         // Variable to store analog values
    uint16_t analogValuePrev;     // Variable to store previous analog values
  }analogBankData;
  analogBankData **aBankData;

  typedef struct{
    uint16_t analogRawValue;         // Variable to store analog values
    uint16_t analogRawValuePrev;     // Variable to store previous analog values
    uint8_t analogDirection;            // Variable to store current direction of change
    uint8_t analogDirectionRaw;            // Variable to store current direction of change
    // Running average filter variables
    uint8_t filterIndex;            // Indice que recorre los valores del filtro de suavizado
    uint8_t filterCount;
    uint16_t filterSum;
    uint16_t filterSamples[FILTER_SIZE];
  }analogHwData;
  analogHwData *aHwData;

  uint8_t priorityCount = 0;        // Amount of modules in priority list
  uint8_t priorityList[2] = {0};      // Priority list: 1 or 2 modules to be read at a time, when changing
  unsigned long priorityTime = 0;   // Timer for priority

  // Address lines for multiplexer
  const int _S0 = (4u);
  const int _S1 = (3u);
  const int _S2 = (8u);
  const int _S3 = (9u);
  // Input signal of multiplexers
  byte InMuxA = MUX_A_PIN;
  byte InMuxB = MUX_B_PIN;
  byte InMuxC = MUX_C_PIN;
  byte InMuxD = MUX_D_PIN;


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

  // byte MuxBMapping[NUM_MUX_CHANNELS] =   {0,       // INPUT 0   - Mux channel 0
  //                                         7,       // INPUT 1   - Mux channel 7
  //                                         1,       // INPUT 2   - Mux channel 1
  //                                         6,       // INPUT 3   - Mux channel 6
  //                                         2,       // INPUT 4   - Mux channel 2
  //                                         5,       // INPUT 5   - Mux channel 5
  //                                         3,       // INPUT 6   - Mux channel 3
  //                                         4,       // INPUT 7   - Mux channel 4
  //                                         15,      // INPUT 8   - Mux channel 15
  //                                         11,      // INPUT 9   - Mux channel 11
  //                                         14,      // INPUT 10  - Mux channel 14
  //                                         10,      // INPUT 11  - Mux channel 10
  //                                         13,      // INPUT 12  - Mux channel 13
  //                                         9,       // INPUT 13  - Mux channel 9
  //                                         12,      // INPUT 14  - Mux channel 12
  //                                         8};      // INPUT 15  - Mux channel 8

public:
  void Init(uint8_t,uint8_t);
  void Read();
};

#endif