// Constant value definitions
#define ADC_HYSTERESIS  50        //Must be 1 or higher. Noise filter, determines how big ADC change needed
#define MAX_POT_VALUE   255
#define POT_SENSITIVITY 50        //Higher number = more turns needed to reach max value

#define ADC_MAX_VALUE   4095

uint8_t arduinoWiring[POT_COUNT][2] = {{0,1},{9,4},{17,18},{19,25}};
uint8_t aInputs[POT_COUNT][2] = {{18,19},{7,16},{4,5},{10,11}};

static inline void ADCsync() {
  while (ADC->STATUS.bit.SYNCBUSY == 1); //Just wait till the ADC is free
}

inline void SelAnalog(uint32_t ulPin){      // Selects the analog input channel in INPCTRL
  ADCsync();
  ADC->INPUTCTRL.bit.MUXPOS = ulPin;
}

void FastADCsetup() {
  const byte gClk = 3; //used to define which generic clock we will use for ADC
  const int cDiv = 1; //divide factor for generic clock
  
  ADC->CTRLA.bit.ENABLE = 0;                     // Disable ADC
  while( ADC->STATUS.bit.SYNCBUSY == 1 );        // Wait for synchronization
  ADC->REFCTRL.bit.REFSEL = ADC_REFCTRL_REFSEL_INTVCC1_Val; //set internal reference
  ADC->CTRLB.reg = ADC_CTRLB_PRESCALER_DIV32 |   // Divide Clock by 64.
                   ADC_CTRLB_RESSEL_12BIT;       // Result on 12 bits
  ADC->AVGCTRL.reg = ADC_AVGCTRL_SAMPLENUM_1 |   // 1 sample
                     ADC_AVGCTRL_ADJRES(0x00ul); // Adjusting result by 0
  ADC->SAMPCTRL.reg = 0x00;                      // Sampling Time Length = 0
  ADC->CTRLA.bit.ENABLE = 1;                     // Enable ADC
  while( ADC->STATUS.bit.SYNCBUSY == 1 );        // Wait for synchronization
  
  //configure inputs pins
  for(uint8_t i=0;i<POT_COUNT;i++){
    pinPeripheral(arduinoWiring[i][0],PIO_ANALOG);
    pinPeripheral(arduinoWiring[i][1],PIO_ANALOG);
  }
}

uint32_t AnalogReadFast(byte ADCpin) {
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

int decodeInfinitePot(uint8_t i)
{
  ValuePotA[i] = expAvgConstant*AnalogReadFast(aInputs[i][0]) + (1-expAvgConstant)*ValuePotA[i];
  ValuePotB[i] = expAvgConstant*AnalogReadFast(aInputs[i][1]) + (1-expAvgConstant)*ValuePotB[i];
  
  /****************************************************************************
  * Step 1 decode each  individual pot tap's direction
  ****************************************************************************/
  // First check direction for Tap A
  if (ValuePotA[i] > (PreviousValuePotA[i] + ADC_HYSTERESIS))       // check if new reading is higher (by <debounce value>), if so...
  {
    DirPotA[i] = 1;                                              // ...direction of tap A is up
  }
  else if (ValuePotA[i] < (PreviousValuePotA[i] - ADC_HYSTERESIS))  // check if new reading is lower (by <debounce value>), if so...
  {
    DirPotA[i] = -1;                                             // ...direction of tap A is down
  }
  else
  {
    DirPotA[i] = 0;                                              // No change
  }
  // then check direction for tap B
  if (ValuePotB[i] > (PreviousValuePotB[i] + ADC_HYSTERESIS))       // check if new reading is higher (by <debounce value>), if so...
  {
    DirPotB[i] = 1;                                              // ...direction of tap B is up
  }
  else if (ValuePotB[i] < (PreviousValuePotB[i] - ADC_HYSTERESIS))  // check if new reading is lower (by <debounce value>), if so...
  {
    DirPotB[i] = -1;                                             // ...direction of tap B is down
  }
  else
  {
    DirPotB[i] = 0;                                              // No change
  }

  /****************************************************************************
  * Step 2: Determine actual direction of ENCODER based on each individual
  * potentiometer tap´s direction and the phase
  ****************************************************************************/
  if (DirPotA[i] == -1 and DirPotB[i] == -1)       //If direction of both taps is down
  {
    if (ValuePotA[i] > ValuePotB[i])               // If value A above value B...
    {
      Direction[i] = 1;                         // ...direction is up
    }
    else
    {
      Direction[i] = -1;                        // otherwise direction is down
    }
  }
  else if (DirPotA[i] == 1 and DirPotB[i] == 1)    //If direction of both taps is up
  {
    if (ValuePotA[i] < ValuePotB[i])               // If value A below value B...
    {
      Direction[i] = 1;                         // ...direction is up
    }
    else
    {
      Direction[i] = -1;                        // otherwise direction is down
    }
  }
  else if (DirPotA[i] == 1 and DirPotB[i] == -1)   // If A is up and B is down
  {
    if ( (ValuePotA[i] > (ADC_MAX_VALUE/2)) || (ValuePotB[i] > (ADC_MAX_VALUE/2)) )  //If either pot at upper range A/B = up/down means direction is up
    {
      Direction[i] = 1;
    }
    else                                     //otherwise if both pots at lower range A/B = up/down means direction is down
    {
      Direction[i] = -1;
    }
  }
  else if (DirPotA[i] == -1 and DirPotB[i] == 1)
  {
    if ( (ValuePotA[i] < (ADC_MAX_VALUE/2)) || (ValuePotB[i] < (ADC_MAX_VALUE/2)))   //If either pot  at lower range, A/B = down/up means direction is down
    {
      Direction[i] = 1;
    }
    else                                     //otherwise if bnoth pots at higher range A/B = up/down means direction is down
    {
      Direction[i] = -1;
    }
  }
  else
  {
    Direction[i] = 0;                           // if any of tap A or B has status unchanged (0), indicate unchanged
  }

  /****************************************************************************
  * Step 3: Calculate value based on direction, how big change in ADC value,
  * and sensitivity. Avoid values around zero and max  as value has flat region
  ****************************************************************************/
  if (DirPotA[i] != 0 && DirPotB[i] != 0)         //If both taps indicate movement
  {
    if ((ValuePotA[i] < ADC_MAX_VALUE*0.8) && (ValuePotA[i] > ADC_MAX_VALUE*0.2))         // if tap A is not at endpoints
    {
      Value[i] = Value[i] + Direction[i]*abs(ValuePotA[i] - PreviousValuePotA[i])/POT_SENSITIVITY; //increment value
    }
    else                                    // If tap A is close to end points, use tap B to calculate value
    {
      Value[i] = Value[i] + Direction[i]*abs(ValuePotB[i] - PreviousValuePotB[i])/POT_SENSITIVITY;  //Make sure to add/subtract at least 1, and then additionally the jump in voltage
    }
    // Finally apply output value limit control
    if (Value[i] <= 0)
    {
      Value[i] = 0;
    }
    if (Value[i] >= MAX_POT_VALUE)
    {
      Value[i] = MAX_POT_VALUE;
    }                                           // Update prev value storage
    PreviousValuePotA[i] = ValuePotA[i];          // Update previous value variable
    PreviousValuePotB[i] = ValuePotB[i];          // Update previous value variable
    
    SERIALPRINT("Pot ");
    SERIALPRINT(i);
    SERIALPRINT(": ");
    SERIALPRINTLN(Direction[i]);

    return Direction[i];
  }
}