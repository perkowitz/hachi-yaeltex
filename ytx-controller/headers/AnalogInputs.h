#ifndef ANALOG_INPUTS_H
#define ANALOG_INPUTS_H

//----------------------------------------------------------------------------------------------------
// CLASS DEFINITION
//----------------------------------------------------------------------------------------------------

class AnalogInputs{
private:
  uint8_t nBanks;
  uint8_t nAnalog;

  // 
  
  uint16_t **analogData;         // Variable to store analog values
  uint16_t **analogDataPrev;     // Variable to store previous analog values
  uint8_t **analogDirection;            // Variable to store current direction of change

  bool IsNoise(unsigned int);

public:
  void Init(uint8_t,uint8_t);
  void Read();
};

#endif