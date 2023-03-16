#include <Arduino.h>
#include "pins_arduino.h"
#include "wiring_private.h"
#include <SPI.h>

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
#define REGISTRER_OFFSET 0x10
#define REGISTRER_COUNT  2

#define POT_COUNT        4

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
enum MAPPING
{
  NEXT_ADDRESS = 0,
  SAMPLE_INTERVAL,     //ms
  ANALOG_HYSTERESIS,   //adc counts
  CONFIGURE_FLAG
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


volatile uint8_t opcode;
volatile uint32_t state;
volatile uint32_t isTransmissionComplete;

volatile bool addressEnable;
volatile uint32_t myAddress;
volatile uint32_t nextAddress;
volatile uint32_t requestedAddress;

volatile uint32_t registerIndex;
volatile uint32_t receivedBytes;
volatile uint8_t  registerValues[REGISTRER_OFFSET+REGISTRER_COUNT];

uint8_t *controlRegister = (uint8_t *)registerValues;
uint8_t *dataRegister = (uint8_t *)&registerValues[REGISTRER_OFFSET];

// Variables for potmeter
endlessPot rotary[POT_COUNT];

void setup (void)
{
  #if defined(SERIAL_DEBUG)
    Serial.begin (250000);   // debugging
  #endif

  myAddress = 0;
  addressEnable = false;
  isTransmissionComplete = 0;

  //init addressing i/o pins
  for(uint8_t i=0;i<3;i++){
    pinMode(inputAddressPin[i],INPUT);
    pinMode(outputAddressPin[i],OUTPUT);
    digitalWrite(outputAddressPin[i],LOW);
  }

  for(uint8_t i=0;i<POT_COUNT;i++){
    pinMode(inputSwitchsPin[i],INPUT_PULLUP);
  }

  memset((void*)registerValues,0,sizeof(registerValues));

  memset((void*)rotary,0,sizeof(rotary));

  FastADCsetup();

  //turn on SPI in slave mode  
  spiSlave_init();
  resetInternalState();

  controlRegister[MAPPING::SAMPLE_INTERVAL] = 5;
  controlRegister[MAPPING::ANALOG_HYSTERESIS] = 50;
}// end of setup

uint32_t antMillisAddress = 0;
uint32_t antMillisSample = 0;
float expAvgConstant = 0.25;

void loop()
{
  if(millis()-antMillisAddress>1000)
  {
    antMillisAddress = millis();

    SERIALPRINTLN("LOOP");

    int address = 0;
    
    for(int i=0;i<3;i++){
      if(digitalRead(inputAddressPin[i])==HIGH)
        address += 1<<i;
    }

    if(myAddress!=address)
      myAddress = address;
  }
  
  // Decode rotarys
  if(millis()-antMillisSample>controlRegister[MAPPING::SAMPLE_INTERVAL]){
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
    dataRegister[0] = auxRotary;
  }

  // Poll switches
  for(int i=0;i<POT_COUNT;i++){
    
    int switchState = digitalRead(inputSwitchsPin[i]);

    if(switchState){
      dataRegister[1] |= (1<<i);
    }else{
      dataRegister[1] &= ~(1<<i);
    }
  }

  if(controlRegister[MAPPING::CONFIGURE_FLAG]){
    controlRegister[MAPPING::CONFIGURE_FLAG] = 0;

    nextAddress = controlRegister[MAPPING::NEXT_ADDRESS];
    SERIALPRINT("Output address: ");SERIALPRINTLN(nextAddress);
    
    for(int i=0;i<3;i++){
      if(nextAddress & (1<<i)){
        digitalWrite(outputAddressPin[i],HIGH);
      }
      else{
        digitalWrite(outputAddressPin[i],LOW);
      }
    }
  }

  if(isTransmissionComplete){
    registerIndex = 0;
    dataRegister[0] = 0; //flush rotary data
    isTransmissionComplete = 0;
  }
}
