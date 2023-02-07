#include <Arduino.h>
#include "pins_arduino.h"
#include "wiring_private.h"
#include <SPI.h>


//#define DEBUG // comment this line out to not print debug data on the serial bus

// Constant value definitions
#define REGISTRER_OFFSET 0x10
#define REGISTRER_COUNT  16
#define POT_COUNT        4

#define INPUT_ADDRESS_PIN_A0  7
#define INPUT_ADDRESS_PIN_A1  SDA
#define INPUT_ADDRESS_PIN_A2  SCL

#define OUTPUT_ADDRESS_PIN_A0  27
#define OUTPUT_ADDRESS_PIN_A1  14
#define OUTPUT_ADDRESS_PIN_A2  42

int inputPin[] = {INPUT_ADDRESS_PIN_A0,INPUT_ADDRESS_PIN_A1,INPUT_ADDRESS_PIN_A2};
int outputPin[] = {OUTPUT_ADDRESS_PIN_A0,OUTPUT_ADDRESS_PIN_A1,OUTPUT_ADDRESS_PIN_A2};

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
  Serial.begin (250000);   // debugging

  analogReference(AR_INTERNAL);

  myAddress = 8;
  addressEnable = false;

  pinMode(INPUT_ADDRESS_PIN_A0,INPUT);
  pinMode(INPUT_ADDRESS_PIN_A1,INPUT);
  pinMode(INPUT_ADDRESS_PIN_A2,INPUT);

  for(uint8_t i=0;i<3;i++){
    // uint32_t temp ;
    // if ( g_APinDescription[outputPin[i]].ulPin & 1 ){ // is pin odd?
    //   temp = (PORT->Group[g_APinDescription[outputPin[i]].ulPort].PMUX[g_APinDescription[outputPin[i]].ulPin >> 1].reg) & 0xF0;
    // }else{
    //   temp = (PORT->Group[g_APinDescription[outputPin[i]].ulPort].PMUX[g_APinDescription[outputPin[i]].ulPin >> 1].reg) & 0x0F;
    // }
    // PORT->Group[g_APinDescription[outputPin[i]].ulPort].PMUX[g_APinDescription[outputPin[i]].ulPin >> 1].reg = temp;
    // PORT->Group[g_APinDescription[outputPin[i]].ulPort].PINCFG[g_APinDescription[outputPin[i]].ulPin].bit.PMUXEN = 0;
    // PORT->Group[g_APinDescription[outputPin[i]].ulPort].PINCFG[g_APinDescription[outputPin[i]].ulPin].bit.PULLEN = 0;
    pinMode(outputPin[i],OUTPUT);
    digitalWrite(outputPin[i],LOW);
  }
  
  // PORT->Group[PORTA].PINCFG[2].bit.PMUXEN = 0;
  // PORT->Group[PORTA].PINCFG[3].bit.PMUXEN = 0;
  // PORT->Group[PORTA].PINCFG[28].bit.PMUXEN = 0;
  // PORT->Group[PORTA].DIRSET.reg  =  OUTPUT_ADDRESS_PIN_A0;
  // PORT->Group[PORTA].DIRSET.reg  =  OUTPUT_ADDRESS_PIN_A1;
  // PORT->Group[PORTA].DIRSET.reg  =  OUTPUT_ADDRESS_PIN_A2;


  for(uint8_t i=0;i<sizeof(registerValues);i++)
    registerValues[i] = 0;
  
  for(uint8_t i=0;i<POT_COUNT;i++)
  {
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
float expAvgConstant = 0.25;
uint8_t aInputs[POT_COUNT][2] = {{18,19},{7,16},{4,5},{10,11}};

void loop()
{
  if(millis()-antMillisDebug>1000)
  {
    antMillisDebug = millis();
    Serial.println("LOOP");

    int address = 0;
    
    for(int i=0;i<3;i++){
      if(digitalRead(inputPin[i])==HIGH)
        address += 1<<i;
    }
    Serial.print("Input address: ");Serial.println(address);

    if(myAddress!=address)
      myAddress = address;
  }
  // Update ADC readings
  for(int i=0;i<POT_COUNT;i++){
    int direction = decodeInfinitePot(i);

    if(direction){
      writeOnRegisters(i, direction);
    }
  }
  delay(5);

  if(controlRegister[MAPPING::CONFIGURE_FLAG]){
    controlRegister[MAPPING::CONFIGURE_FLAG] = 0;

    nextAddress = controlRegister[MAPPING::NEXT_ADDRESS];
    Serial.print("Output address: ");Serial.println(nextAddress);

    
    for(int i=0;i<3;i++){
      if(nextAddress & (1<<i)){
        digitalWrite(outputPin[i],HIGH);
        // PORT->Group[PORTA].OUTSET.reg = outputPin[i];
      }
      else{
        digitalWrite(outputPin[i],LOW);
        // PORT->Group[PORTA].OUTCLR.reg = outputPin[i];
      }
    }
  }
}
