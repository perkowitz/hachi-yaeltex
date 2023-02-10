#include <Arduino.h>
#include "pins_arduino.h"
#include "wiring_private.h"
#include <SPI.h>


#define SERIAL_DEBUG // comment this line out to not print debug data on the serial bus

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
#define REGISTRER_COUNT  16
#define POT_COUNT        4

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
  CONFIGURE_FLAG
};


volatile boolean debugFlag;
volatile boolean addressEnable;

volatile uint32_t state;
volatile uint8_t opcode;

volatile uint32_t myAddress;
volatile uint32_t nextAddress;
volatile uint32_t requestedAddress;


volatile uint32_t registerIndex;
volatile uint8_t  registerValues[REGISTRER_OFFSET+REGISTRER_COUNT];
volatile uint32_t receivedBytes;

uint8_t *controlRegister = (uint8_t *)registerValues;
uint8_t *dataRegister = (uint8_t *)&registerValues[REGISTRER_OFFSET];

// Input/Output declarations


// Variables for potmeter
uint16_t ValuePotA[POT_COUNT];            //Pot1 tap A value
uint16_t ValuePotB[POT_COUNT];           //Pot1 tap B value
uint16_t PreviousValuePotA[POT_COUNT];    //Used to remember last value to determine turn direction
uint16_t PreviousValuePotB[POT_COUNT];    //Used to remember last value to determine turn direction
int DirPotA[POT_COUNT];              //Direction for Pot 1 tap A
int DirPotB[POT_COUNT];              //Direction for Pot1 tap B

int Direction[POT_COUNT];         //Final CALCULATED direction
int Value[POT_COUNT];              //Final CALCULATED value

void setup (void)
{
  #if defined(SERIAL_DEBUG)
    Serial.begin (250000);   // debugging
  #endif

  myAddress = 8;
  addressEnable = false;

  //init addressing i/o pins
  for(uint8_t i=0;i<3;i++){
    pinMode(inputAddressPin[i],INPUT);
    pinMode(outputAddressPin[i],OUTPUT);
    digitalWrite(outputAddressPin[i],LOW);
  }

  for(uint8_t i=0;i<sizeof(registerValues);i++)
    registerValues[i] = 0;
  
  for(uint8_t i=0;i<POT_COUNT;i++)
  {
    pinMode(inputSwitchsPin[i],INPUT_PULLUP);
    ValuePotA[i] = 0;
    ValuePotB[i] = 0;
    PreviousValuePotA[i] = 0;
    PreviousValuePotB[i] = 0;
    DirPotA[i] = 1;
    DirPotB[i] = 1;
    Direction[i] = 1;
    Value[i] = 0;
  }

  FastADCsetup();

  //turn on SPI in slave mode  
  spiSlave_init();
  resetInternalState();

  controlRegister[MAPPING::CONFIGURE_FLAG]=1;
  controlRegister[MAPPING::NEXT_ADDRESS]=6;

}// end of setup

uint32_t antMillisDebug = 0;
uint32_t antMillisSample = 0;
float expAvgConstant = 0.25;


void loop()
{
  if(millis()-antMillisDebug>1000)
  {
    antMillisDebug = millis();
    SERIALPRINTLN("LOOP");

    int address = 0;
    
    for(int i=0;i<3;i++){
      if(digitalRead(inputAddressPin[i])==HIGH)
        address += 1<<i;
    }
    // SERIALPRINT("Input address: ");SERIALPRINTLN(address);
    // for(int i=0;i<POT_COUNT;i++){
    //   SERIALPRINT("Pot ");SERIALPRINT(i);
    //   SERIALPRINT(" value: ");SERIALPRINT(ValuePotA[i]);
    //   SERIALPRINT(", ");SERIALPRINTLN(ValuePotB[i]);
    // }
    // SERIALPRINT("Input address: ");SERIALPRINTLN(address);
    // for(int i=0;i<POT_COUNT;i++){
    //   SERIALPRINT("switch ");SERIALPRINT(i);
    //   SERIALPRINT(" value: ");SERIALPRINTLN(digitalRead(inputSwitchsPin[i]));
    // }
    if(myAddress!=address)
      myAddress = address;
  }
  
  // Update ADC reading
  for(int i=0;i<POT_COUNT;i++){
    int direction = decodeInfinitePot(i);
    int switchState = digitalRead(inputSwitchsPin[i]);

    if(millis()-antMillisSample>2){
      antMillisSample = millis();

      if(direction){
        dataRegister[0] |= (1<<(4+i));

        if(direction>0)
          dataRegister[0] |= 1<<i;
        else
          dataRegister[0] &= ~(1<<i);
        
      }else{
        dataRegister[0] &= ~(1<<(4+i));
      }
    }

    if(switchState){
      dataRegister[1] |= (1<<i);
    }else{
      dataRegister[1] &= ~(1<<i);
    }
  }
  delay(1);

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
}
