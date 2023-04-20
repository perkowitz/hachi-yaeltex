#include <Arduino.h>
#include "pins_arduino.h"
#include "wiring_private.h"
#include <SPI.h>
#include <SPIAddressable.h>

#define CONCAT_(a,b) a ## b
#define CONCAT(a,b) CONCAT_(a,b)

#define SERCOM SERCOM4
#define IRQ(sercom_n) CONCAT(sercom_n,_IRQn)

#define FRAMEWORK // comment this line to use direct registrer manipulation
// #define SERIAL_DEBUG // comment this line out to not print SERIAL_DEBUG data on the serial bus

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

#define SLAVE_BASE_ADDRESS       (0b00000000)


//MCP23S17 legacy opcodes 
#define    OPCODEW_YTX       (0b00000000)  // Opcode for MCP23S17 with LSB (bit0) set to write (0), address OR'd in later, bits 1-3
#define    OPCODER_YTX       (0b00000001)  // Opcode for MCP23S17 with LSB (bit0) set to read (1), address OR'd in later, bits 1-3
//MCP23S17 legacy commands 
#define    ADDR_ENABLE   (0b00001000)  // Configuration register for MCP23S17, the only thing we change is enabling hardware addressing
#define    ADDR_DISABLE  (0b00000000)  // Configuration register for MCP23S17, the only thing we change is disabling hardware addressing

#define    REGISTER_OFFSET     0x10
#define    ANALOG_INPUTS_COUNT  96

#define ANALOG_INCREASING       0
#define ANALOG_DECREASING       1

typedef struct analogType{
  uint16_t rawValue;
  uint16_t rawValuePrev;
  bool direction;
};

volatile boolean addressEnable;

volatile uint32_t state;
volatile uint8_t opcode;

volatile uint32_t myAddress;
volatile uint32_t requestedAddress;

volatile uint32_t receivedBytes;
volatile uint32_t registerIndex;
volatile uint8_t  registerValues[REGISTER_OFFSET+ANALOG_INPUTS_COUNT*2+ANALOG_INPUTS_COUNT/8];

uint8_t *controlRegister = (uint8_t *)registerValues;
uint16_t *analogRegister = (uint16_t *)(&registerValues[REGISTER_OFFSET]);
uint8_t  *activeChannels = (uint8_t *)(&registerValues[REGISTER_OFFSET+ANALOG_INPUTS_COUNT*2]);

float expAvgConstant = 1.0;

analogType analog[ANALOG_INPUTS_COUNT];

//Control Register mapping
enum CTRL_MAPPING
{
  INPUTS_CNT = 0,
  AVG_FILTER,
  NOISE_TH,
  CTRL_REG_COUNT
};

//UserData Register mapping
enum USER_MAPPING
{
  ANALOG_INPUTS_DATA = 0,  
  ACTIVE_CHANNELS_DATA = ANALOG_INPUTS_COUNT*2,
  USR_REG_COUNT     
};


void cleanup(void)
{
  memset((void*)activeChannels,0,ANALOG_INPUTS_COUNT/8);
}

// Constant value definitions
#define ADC_MAX_VALUE   1023

#define MUX_A                0            // Mux A identifier
#define MUX_B                1            // Mux B identifier
#define MUX_C                2            // Mux C identifier
#define MUX_D                3            // Mux D identifier
#define MUX_E                4            // Mux E identifier
#define MUX_F                5            // Mux F identifier

#define MUX_A_PIN            11           // Mux A pin
#define MUX_B_PIN            10           // Mux B pin
#define MUX_C_PIN            0            // Mux C pin
#define MUX_D_PIN            2            // Mux D pin
#define MUX_E_PIN            5            // Mux E pin
#define MUX_F_PIN            4            // Mux F pin

#define NUM_MUX              6            // Number of multiplexers to address
#define NUM_MUX_CHANNELS     16           // Number of multiplexing channels
#define MAX_NUMBER_ANALOG    NUM_MUX*NUM_MUX_CHANNELS  

// Address lines for multiplexer
const int _S0 = (4u);
const int _S1 = (3u);
const int _S2 = (8u);
const int _S3 = (9u);
// Input signal of multiplexers
byte muxPin[NUM_MUX] = {MUX_A_PIN, MUX_B_PIN, MUX_C_PIN, MUX_D_PIN, MUX_E_PIN, MUX_F_PIN};

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
                                       10,       // INPUT 12  - Mux channel 9
                                       9,        // INPUT 13  - Mux channel 10
                                       8,        // INPUT 14  - Mux channel 8
                                       11};      // INPUT 15  - Mux channel 11

void setup (void)
{
  Serial.begin(250000);   // debugging

  SPIAddressableSlaveModule.begin(SLAVE_BASE_ADDRESS,CTRL_REG_COUNT,USR_REG_COUNT);
  SPIAddressableSlaveModule.setTransmissionCompleteCallback(cleanup);

  // Set output pins for multiplexers
  pinMode(_S0, OUTPUT);
  pinMode(_S1, OUTPUT);
  pinMode(_S2, OUTPUT);
  pinMode(_S3, OUTPUT);

  // init ADC peripheral
  analogReference(AR_EXTERNAL);
  FastADCsetup();

  memset((void*)registerValues,0,sizeof(registerValues));
  memset((void*)analog,0,sizeof(analog));
  
  myAddress = 4;
  addressEnable = true;
  
  // turn on SPI in slave mode  
  spiSlave_init();
  resetInternalState();
}// end of setup

void loop()
{
  #if defined(SERIAL_DEBUG)
    Serial.println("Loop");
  #endif

  // Update ADC readings
  // Scan analog inputs
  for(int aInput = 0; aInput < registerValues[MAPPING::INPUTS_CNT]; aInput++){
    byte mux = aInput < 16 ? MUX_A : (aInput < 32 ? MUX_B : ( aInput < 48 ? MUX_C : ( aInput < 64 ? MUX_D : ( aInput < 80 ? MUX_E : MUX_F))));    // Select correct multiplexer for this input
    
    byte muxChannel = aInput % NUM_MUX_CHANNELS;        
    
    uint16_t newread = MuxAnalogRead(mux, muxChannel)&0x0FFF;
    analog[aInput].rawValue = expAvgConstant*newread + (1-expAvgConstant)*analog[aInput].rawValue;

    if(analog[aInput].rawValue != analog[aInput].rawValuePrev){
        
      if(!isNoise(&analog[aInput],controlRegister[MAPPING::NOISE_TH])){
        #if defined(SERIAL_DEBUG)
          Serial.print("Channel ");Serial.print(aInput);
          Serial.print(" -> ");Serial.println(analog[aInput].rawValue);
        #endif
        analogRegister[aInput] = analog[aInput].rawValue;

        analog[aInput].rawValuePrev = analog[aInput].rawValue;

        activeChannels[aInput/8] |= (1<<aInput%8);
      }
    }    
  }

  if(controlRegister[MAPPING::CONFIGURE_FLAG]){
    controlRegister[MAPPING::CONFIGURE_FLAG] = 0;
    expAvgConstant = (float)(controlRegister[MAPPING::AVG_FILTER])/255.0;
  }
}
