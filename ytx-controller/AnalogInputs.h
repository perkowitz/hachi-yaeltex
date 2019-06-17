//----------------------------------------------------------------------------------------------------
// CLASS DEFINITION
//----------------------------------------------------------------------------------------------------

class AnalogInputs{
private:
  byte maxAnalog;
  
  uint16_t *analogData;         // Variable to store analog values
  uint16_t *analogDataPrev;     // Variable to store previous analog values
  uint8_t *analogDirection;            // Variable to store current direction of change

  bool IsNoise(unsigned int input);

public:
  void SetAnalogQty(byte maxNumberOfAnalog);
  void Update();
};
