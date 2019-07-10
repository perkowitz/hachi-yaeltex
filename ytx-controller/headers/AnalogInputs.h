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
  bool IsNoise(uint16_t);
  void FastADCsetup();
  void SelAnalog(uint32_t);
  uint32_t AnalogReadFast(byte);
  int16_t MuxAnalogRead(int16_t , int16_t);
  int16_t MuxDigitalRead(int16_t, int16_t, int16_t);
  int16_t MuxDigitalRead(int16_t, int16_t);

  // Variables

  uint8_t nBanks;
  uint8_t nAnalog;
  
  uint16_t **analogValuePrev;     // Variable to store previous analog values
  uint8_t **analogDirection;            // Variable to store current direction of change

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
  byte MuxAMapping[NUM_MUX_CHANNELS] =  {0,        // INPUT 0   - Mux channel 0
                                         7,        // INPUT 1   - Mux channel 7
                                         1,        // INPUT 2   - Mux channel 1
                                         6,        // INPUT 3   - Mux channel 6
                                         2,        // INPUT 4   - Mux channel 2
                                         5,        // INPUT 5   - Mux channel 5
                                         3,        // INPUT 6   - Mux channel 3
                                         4,        // INPUT 7   - Mux channel 4
                                         15,       // INPUT 8   - Mux channel 15
                                         11,       // INPUT 9   - Mux channel 11
                                         14,       // INPUT 10  - Mux channel 14
                                         10,       // INPUT 11  - Mux channel 10
                                         13,       // INPUT 12  - Mux channel 13
                                         9,        // INPUT 13  - Mux channel 9
                                         12,       // INPUT 14  - Mux channel 12
                                         8};       // INPUT 15  - Mux channel 8

  byte MuxBMapping[NUM_MUX_CHANNELS] =   {0,       // INPUT 0   - Mux channel 0
                                          7,       // INPUT 1   - Mux channel 7
                                          1,       // INPUT 2   - Mux channel 1
                                          6,       // INPUT 3   - Mux channel 6
                                          2,       // INPUT 4   - Mux channel 2
                                          5,       // INPUT 5   - Mux channel 5
                                          3,       // INPUT 6   - Mux channel 3
                                          4,       // INPUT 7   - Mux channel 4
                                          15,      // INPUT 8   - Mux channel 15
                                          11,      // INPUT 9   - Mux channel 11
                                          14,      // INPUT 10  - Mux channel 14
                                          10,      // INPUT 11  - Mux channel 10
                                          13,      // INPUT 12  - Mux channel 13
                                          9,       // INPUT 13  - Mux channel 9
                                          12,      // INPUT 14  - Mux channel 12
                                          8};      // INPUT 15  - Mux channel 8

public:
  void Init(uint8_t,uint8_t);
  void Read();

  uint16_t **analogValue;         // Variable to store analog values
};

#endif