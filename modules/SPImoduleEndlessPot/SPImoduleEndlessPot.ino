#include <Arduino.h>
#include "pins_arduino.h"
#include "wiring_private.h"
#include <SPI.h>
#include "SPISlave.h"

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

#define OPCODE       (0b00000000)

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

//Control Register mapping
enum CTRL_MAPPING
{
  SAMPLE_INTERVAL=0,     //ms
  ANALOG_HYSTERESIS,     //adc counts
  CTRL_REG_COUNT
};

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
  SPISlaveModule.userDataRegister[ROTARY_DATA] = 0; //flush rotary data
}

void setup (void)
{
  #if defined(SERIAL_DEBUG)
    Serial.begin (250000);   // debugging
  #endif

  SPISlaveModule.begin(OPCODE,CTRL_REG_COUNT,USR_REG_COUNT);
  SPISlaveModule.wiring(inputAddressPin,outputAddressPin);
  SPISlaveModule.setTransmissionCompleteCallback(cleanup);
  SPISlaveModule.controlRegister[SAMPLE_INTERVAL] = 5;
  SPISlaveModule.controlRegister[ANALOG_HYSTERESIS] = 50;

  FastADCsetup();

  for(uint8_t i=0;i<POT_COUNT;i++){
    pinMode(inputSwitchsPin[i],INPUT_PULLUP);
  }

  memset((void*)rotary,0,sizeof(rotary));
}// end of setup


uint32_t antMillisSample = 0;
float expAvgConstant = 0.25;

void loop()
{
  SPISlaveModule.hook();

  // Decode rotarys
  if(millis()-antMillisSample>SPISlaveModule.controlRegister[SAMPLE_INTERVAL]){
    antMillisSample = millis();

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
    SPISlaveModule.userDataRegister[ROTARY_DATA] = auxRotary;
  }

  // Poll switches
  for(int i=0;i<POT_COUNT;i++){
    
    int switchState = digitalRead(inputSwitchsPin[i]);

    if(switchState){
      SPISlaveModule.userDataRegister[SWITCH_DATA] |= (1<<i);
    }else{
      SPISlaveModule.userDataRegister[SWITCH_DATA] &= ~(1<<i);
    }
  }
}
