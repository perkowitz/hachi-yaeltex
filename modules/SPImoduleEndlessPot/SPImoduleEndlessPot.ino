#include <Arduino.h>
#include "pins_arduino.h"
#include "wiring_private.h"

#include <SPIEndlessPot.h>
#include <SPIAddressableSlave.h>

// #define SERIAL_DEBUG // comment this line out to not print debug data on the serial bus
// #define COMPUTE_POT_VALUE

#if defined(SERIAL_DEBUG)
  #define SERIALPRINT(a)        {Serial.print(a);     }
  #define SERIALPRINTLN(a)      {Serial.println(a);   }
  #define SERIALPRINTF(a, f)    {Serial.print(a,f);   }
  #define SERIALPRINTLNF(a, f)  {Serial.println(a,f); }
#else
  #define SERIALPRINT(a)        {}
  #define SERIALPRINTLN(a)      {}
  #define SERIALPRINTF(a, f)    {}
  #define SERIALPRINTLNF(a, f)  {}
#endif

// Constant value definitions
#define POT_COUNT     4

#define SLAVE_BASE_ADDRESS       INF_POT_BASE_ADDRESS_A

// Input/Output declarations
#define INPUT_ADDRESS_PIN_A0  7
#define INPUT_ADDRESS_PIN_A1  20
#define INPUT_ADDRESS_PIN_A2  21

#define OUTPUT_ADDRESS_PIN_A0  27
#define OUTPUT_ADDRESS_PIN_A1  14
#define OUTPUT_ADDRESS_PIN_A2  42

int inputAddressPin[] = {INPUT_ADDRESS_PIN_A0,INPUT_ADDRESS_PIN_A1,INPUT_ADDRESS_PIN_A2};
int outputAddressPin[] = {OUTPUT_ADDRESS_PIN_A0,OUTPUT_ADDRESS_PIN_A1,OUTPUT_ADDRESS_PIN_A2};

#if defined(SERIAL_DEBUG)
  int inputSwitchsPin[POT_COUNT] = {5,3,8,8};
#else
  int inputSwitchsPin[POT_COUNT] = {5,3,8,30};
#endif

//Control Register mapping (view SPIEndlessPotParameters)
#define CTRL_REG_COUNT sizeof(SPIEndlessPotParameters)

//UserData Register mapping
enum USER_MAPPING
{
  ROTARY_DATA=0,  
  SWITCH_DATA,
  USR_REG_COUNT     
};

typedef struct 
{
  int valueA;            //Pot1 tap A value
  int valueB;            //Pot1 tap B value
  int previousValueA;    //Used to remember last value to determine turn direction
  int previousValueB;    //Used to remember last value to determine turn direction
  int dirA;              //Direction for Pot 1 tap A
  int dirB;              //Direction for Pot1 tap B
  int Value;             //Final CALCULATED value
}endlessPot;

// Variables for potmeter
endlessPot rotary[POT_COUNT];

void cleanup(void)
{
  SPIAddressableSlaveModule.userDataRegister[ROTARY_DATA] = 0; //flush rotary data
}

SPIEndlessPotParameters *parameters;

void setup (void)
{
  #if defined(SERIAL_DEBUG)
    Serial.begin (250000);   // debugging
  #endif

  SPIAddressableSlaveModule.begin(SLAVE_BASE_ADDRESS,CTRL_REG_COUNT,USR_REG_COUNT);
  SPIAddressableSlaveModule.wiring(inputAddressPin,outputAddressPin);
  SPIAddressableSlaveModule.setTransmissionCompleteCallback(cleanup);

  parameters = (SPIEndlessPotParameters *)SPIAddressableSlaveModule.getControlRegistersPointer();

  parameters->sampleInterval = 1000;
  parameters->hysteresis = 50;
  parameters->expFilter = 0.25;

  FastADCsetup();

  for(uint8_t i=0;i<POT_COUNT;i++){
    pinMode(inputSwitchsPin[i],INPUT_PULLUP);
  }

  memset((void*)rotary,0,sizeof(rotary));
}// end of setup


uint32_t antMicrosSample = 0;

void loop()
{
  SPIAddressableSlaveModule.hook();

  // Decode rotarys
  if(micros()-antMicrosSample>parameters->sampleInterval){
    antMicrosSample = micros();

    uint8_t auxRotary = 0;
    
    for(int i=0;i<POT_COUNT;i++){
      int direction = decodeInfinitePot(i);
    
      if(direction!=0){ //has activity

        //write activity
        auxRotary |= (1<<(4+i)); 

        //write CW direction(otherwise CCW)
        if(direction>0)
          auxRotary |= 1<<i;
      }
    }
    //write complete register at once
    SPIAddressableSlaveModule.userDataRegister[ROTARY_DATA] = auxRotary;
  }

  // Poll switches
  for(int i=0;i<POT_COUNT;i++){
    
    int switchState = digitalRead(inputSwitchsPin[i]);

    if(switchState){
      SPIAddressableSlaveModule.userDataRegister[SWITCH_DATA] |= (1<<i);
    }else{
      SPIAddressableSlaveModule.userDataRegister[SWITCH_DATA] &= ~(1<<i);
    }
  }
}
