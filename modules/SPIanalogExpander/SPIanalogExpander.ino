#include <Arduino.h>
#include "pins_arduino.h"
#include "wiring_private.h"
#include <SPI.h>

#define CONCAT_(a,b) a ## b
#define CONCAT(a,b) CONCAT_(a,b)

#define SERCOM SERCOM4
#define IRQ(sercom_n) CONCAT(sercom_n,_IRQn)

#define FRAMEWORK // comment this line to use direct registrer manipulation
//#define DEBUG // comment this line out to not print debug data on the serial bus

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
enum MAPPING
{
  INPUTS_CNT = 0,
  AVG_FILTER,
  NOISE_TH,
  CONFIGURE_FLAG
};

enum machineStates
{
  GET_OPCODE = 0,
  GET_REG_INDEX,
  GET_TRANSFER
};

enum transactionDirection
{
  TRANSFER_WRITE = 0,
  TRANSFER_READ,
};

#ifdef FRAMEWORK
  enum spi_transfer_mode
  {
    SERCOM_SPI_TRANSFER_MODE_0 = 0,
    SERCOM_SPI_TRANSFER_MODE_1 = SERCOM_SPI_CTRLA_CPHA,
    SERCOM_SPI_TRANSFER_MODE_2 = SERCOM_SPI_CTRLA_CPOL,
    SERCOM_SPI_TRANSFER_MODE_3 = SERCOM_SPI_CTRLA_CPHA | SERCOM_SPI_CTRLA_CPOL,
  };

  enum spi_frame_format
  {
    SERCOM_SPI_FRAME_FORMAT_SPI_FRAME      = SERCOM_SPI_CTRLA_FORM(0),
    SERCOM_SPI_FRAME_FORMAT_SPI_FRAME_ADDR = SERCOM_SPI_CTRLA_FORM(2),
  };

  enum spi_character_size
  {
    SERCOM_SPI_CHARACTER_SIZE_8BIT = SERCOM_SPI_CTRLB_CHSIZE(0),
    SERCOM_SPI_CHARACTER_SIZE_9BIT = SERCOM_SPI_CTRLB_CHSIZE(1),
  };

  enum spi_data_order
  {
    SERCOM_SPI_DATA_ORDER_LSB = SERCOM_SPI_CTRLA_DORD,
    SERCOM_SPI_DATA_ORDER_MSB   = 0,
  };
#else
  #define MISO_PORT PORTA
  #define MISO_PIN  12
#endif//#ifdef FRAMEWORK

void spiSlave_init()
{
  //Configure SERCOM4 SPI PINS  
  //PA12
  //PB09 (initily not configured)
  //PB10
  //PB11

  #ifdef FRAMEWORK
    //pinPeripheral(MISO, PIO_SERCOM_ALT);
    pinPeripheral(MOSI, PIO_SERCOM_ALT);
    pinPeripheral(SCK,  PIO_SERCOM_ALT);
    pinPeripheral(A2,   PIO_SERCOM_ALT);
    
    sercom4.disableSPI();
    sercom4.resetSPI();
  #else
    //PORT->Group[PORTA].PINCFG[12].bit.PMUXEN = 0x1; //Enable Peripheral Multiplexing for SERCOM4 SPI PA12 FRAMEWORK PIN22
    //PORT->Group[PORTA].PMUX[6].bit.PMUXE = 0x3; //SERCOM 4 is selected for peripheral use of this pad (0x3 selects peripheral function D: SERCOM-ALT)
    
    PORT->Group[PORTB].PINCFG[9].reg = PORT_PINCFG_INEN | PORT_PINCFG_PULLEN;
    PORT->Group[PORTB].OUTSET.reg = (1u << 9);

    PORT->Group[PORTB].PINCFG[9].bit.PMUXEN = 0x1; //Enable Peripheral Multiplexing for SERCOM4 SPI PB09 FRAMEWORK PIN16
    PORT->Group[PORTB].PMUX[4].bit.PMUXO = 0x3; //SERCOM 4 is selected for peripheral use of this pad (0x3 selects peripheral function D: SERCOM-ALT)
    

    PORT->Group[PORTB].PINCFG[10].bit.PMUXEN = 0x1; //Enable Peripheral Multiplexing for SERCOM4 SPI PB10 FRAMEWORK PIN23
    PORT->Group[PORTB].PMUX[5].bit.PMUXE = 0x3; //SERCOM 4 is selected for peripheral use of this pad (0x3 selects peripheral function D: SERCOM-ALT)
    PORT->Group[PORTB].PINCFG[11].bit.PMUXEN = 0x1; //Enable Peripheral Multiplexing for SERCOM4 SPI PB11 FRAMEWORK PIN24
    PORT->Group[PORTB].PMUX[5].bit.PMUXO = 0x3; //SERCOM 4 is selected for peripheral use of this pad (0x3 selects peripheral function D: SERCOM-ALT)

    /*
    Explanation:
    PMUXEN stands for Peripheral Multiplexing Enable
    PMUXE stands for Even bits in the Peripheral Multiplexing register
    PMUXO stands for Odd bits in the Peripheral Multiplexing register
    The selection of peripheral function A to H is done by writing to the Peripheral Multiplexing Odd and Even bits in the Peripheral Multiplexing register (PMUXn.PMUXE/O) in the PORT.
    Reference: Atmel-42181G-SAM-D21_Datasheet section 6.1 on page 21
    PA12 corresponds to: PORTA, PMUX[6] Even
    PB09 corresponds to: PORTB, PMUX[4] Odd
    PB10 corresponds to: PORTB, PMUX[5] Even
    PB11 corresponds to: PORTB, PMUX[5] Odd
    In general:
    Px(2n+0/1) corresponds to Portx, PMUX[n] Even=0/Odd=1
    */

    //Disable SPI 1
    SERCOM->SPI.CTRLA.bit.ENABLE =0;
    while(SERCOM->SPI.SYNCBUSY.bit.ENABLE);
    
    //Reset SPI 1
    SERCOM->SPI.CTRLA.bit.SWRST = 1;
    while(SERCOM->SPI.CTRLA.bit.SWRST || SERCOM->SPI.SYNCBUSY.bit.SWRST);
  #endif//#ifdef FRAMEWORK

  //Setting up NVIC
  NVIC_EnableIRQ(SERCOM4_IRQn);
  NVIC_SetPriority(SERCOM4_IRQn,2);
  
  //Setting Generic Clock Controller!!!!
  GCLK->CLKCTRL.reg = GCLK_CLKCTRL_ID(GCM_SERCOM4_CORE) | //Generic Clock 0
            GCLK_CLKCTRL_GEN_GCLK0 | // Generic Clock Generator 0 is the source
            GCLK_CLKCTRL_CLKEN; // Enable Generic Clock Generator
  
  while(GCLK->STATUS.reg & GCLK_STATUS_SYNCBUSY); //Wait for synchronisation
  
  //Set up SPI Control A Register
  //  - MISO: PAD[0], PA12, D22
  //  - MOSI: PAD[2], PB10, D23
  //  - SCK:  PAD[3], PB11, D24
  //  - SS:   PAD[1], PB09, A2
  SERCOM->SPI.CTRLA.bit.DORD = 0; //MSB first
  SERCOM->SPI.CTRLA.bit.CPOL = 0; //SCK is low when idle, leading edge is rising edge
  SERCOM->SPI.CTRLA.bit.CPHA = 0; //data sampled on leading sck edge and changed on a trailing sck edge
  SERCOM->SPI.CTRLA.bit.FORM = 0x0; //Frame format = SPI
  SERCOM->SPI.CTRLA.bit.DIPO = SERCOM_RX_PAD_2; //DATA PAD2 MOSI is used as slave input (slave mode) // page 492
  SERCOM->SPI.CTRLA.bit.DOPO = SPI_PAD_0_SCK_3; //DATA PAD0 MISO is used as slave output
  
  SERCOM->SPI.CTRLA.bit.MODE = 0x2; //SPI in Slave mode
  SERCOM->SPI.CTRLA.bit.IBON = 0x1; //Buffer Overflow notification
  SERCOM->SPI.CTRLA.bit.RUNSTDBY = 1; //wake on receiver complete
  
  //Set up SPI control B register
  SERCOM->SPI.CTRLB.bit.SSDE = 0x1; //Slave Selecte Detection Enabled
  SERCOM->SPI.CTRLB.bit.CHSIZE = 0; //character size 8 Bit
  
  //Set up SPI interrupts
  SERCOM->SPI.INTENSET.bit.SSL = 0x1; //Enable Slave Select low interrupt        
  SERCOM->SPI.INTENSET.bit.RXC = 0x1; //Receive complete interrupt
  SERCOM->SPI.INTENSET.bit.TXC = 0x1; //Receive complete interrupt
  SERCOM->SPI.INTENSET.bit.ERROR = 0x1; //Receive complete interrupt
  //SERCOM->SPI.INTENSET.bit.DRE = 0x1; //Data Register Empty interrupt
  
  //Enable SPI
  #ifdef FRAMEWORK
    sercom4.enableSPI();
  #else
    SERCOM->SPI.CTRLA.bit.ENABLE = 1;
    while(SERCOM->SPI.SYNCBUSY.bit.ENABLE);
  #endif//#ifdef FRAMEWORK

  SERCOM->SPI.CTRLB.bit.RXEN = 0x1; //Enable Receiver, this is done here due to errate issue
  while(SERCOM->SPI.SYNCBUSY.bit.CTRLB); //wait until receiver is enabled
}

inline void takeMISObus()
{
  #ifdef FRAMEWORK
    pinPeripheral(MISO, PIO_SERCOM_ALT);
  #else
    PORT->Group[MISO_PORT].PINCFG[MISO_PIN].bit.INEN = 0;
    PORT->Group[MISO_PORT].DIRCLR.reg &= ~(1ul << MISO_PIN) ;

    if(MISO_PIN&0x01)
      PORT->Group[MISO_PORT].PMUX[MISO_PIN>>1].bit.PMUXO = 0x3; //SERCOM 4 is selected for peripheral use of this pad (0x3 selects peripheral function D: SERCOM-ALT)
    else
      PORT->Group[MISO_PORT].PMUX[MISO_PIN>>1].bit.PMUXE = 0x3; //SERCOM 4 is selected for peripheral use of this pad (0x3 selects peripheral function D: SERCOM-ALT)
    
    PORT->Group[MISO_PORT].PINCFG[MISO_PIN].bit.PMUXEN = 0x1; //Enable Peripheral Multiplexing for SERCOM4 SPI PB11 FRAMEWORK PIN24
  #endif
}

inline void leaveMISObus()
{
  #ifdef FRAMEWORK
    pinMode(MISO, INPUT);
  #else
    PORT->Group[MISO_PORT].PINCFG[MISO_PIN].bit.PMUXEN = 0;
    PORT->Group[MISO_PORT].PINCFG[MISO_PIN].bit.INEN = 1;
    //Antes era --> PORT->Group[MISO_PORT].DIRCLR.reg |= (1ul << MISO_PIN); 
    PORT->Group[MISO_PORT].DIRCLR.reg = (1ul << MISO_PIN);
  #endif
}

inline void resetInternalState(void)
{
  leaveMISObus();
  state = GET_OPCODE;
  receivedBytes = 0;
}

void SERCOM4_Handler(void)
{
  uint8_t data = 0;
  uint8_t interrupts = SERCOM->SPI.INTFLAG.reg; //Read SPI interrupt register

  /*
  *  1. ERROR
  *   Occurs when the SPI receiver has one or more errors.
  */
  if (SERCOM->SPI.INTFLAG.bit.ERROR)
  {
    SERCOM->SPI.INTFLAG.bit.ERROR = 1;
  }
  /*
  *  2. SSL: Slave Select Low
  *   Occurs when SS goes low
  */
  if(interrupts & (1<<3)) // 8 = 1000 = SSL
  {
    OnTransmissionStart();
    SERCOM->SPI.INTFLAG.bit.SSL = 1;
  }
  /*
  *  3. TXC: Transmission Complete
  *   Occurs when SS goes high. The transmission is complete.
  */
  if(interrupts & (1<<1)) // 2 = 0010 = TXC
  {
    OnTransmissionStop();
    SERCOM->SPI.INTFLAG.bit.TXC = 1;
  }
  /*
  *  4. RXC: Receive Complete
  *   Occurs after a character has been full stored in the data buffer. It can now be retrieved from DATA.
  */
  if(interrupts & (1<<2)) // 4 = 0100 = RXC
  {
    uint8_t data = (uint8_t)SERCOM->SPI.DATA.reg;

    receivedBytes++;

    if(receivedBytes==1)
    {
      opcode = data&0b11000001;
      requestedAddress = (data&0b00111110)>>1;

      if(addressEnable)
      {
        if(requestedAddress==myAddress)
        {
          state = GET_REG_INDEX;
          takeMISObus();
        }
        else
          leaveMISObus();
      }
      else
      {
        state = GET_REG_INDEX;
        takeMISObus();
      }
    }  
    else if(receivedBytes == 2)
    {
      if(state==GET_REG_INDEX)
      {
        registerIndex = constrain(data,0,sizeof(registerValues)-1);
        state = GET_TRANSFER;
        if(opcode==OPCODEW_YTX)
        {
          SERCOM->SPI.INTFLAG.bit.RXC = 1;
          return;
        }
      }
    }

    if(state == GET_TRANSFER)
    {
      if(opcode==OPCODER_YTX)
      {
        SERCOM->SPI.DATA.reg = registerValues[registerIndex];
      }
      else if(opcode==OPCODEW_YTX)
      {
        if(registerIndex==0x05 || registerIndex==0x0A || 
          registerIndex==0x0B || registerIndex==0x0F)
        {
          if(data==ADDR_ENABLE)
            addressEnable = true;
          else if(data==ADDR_DISABLE)
            addressEnable = false;
        }
        else
        {
          registerValues[registerIndex] = data;
        }
      }

      if(++registerIndex == sizeof(registerValues))
      {
        registerIndex=0;
      }
    }

    SERCOM->SPI.INTFLAG.bit.RXC = 1;
  } 
  /*
  *  5. DRE: Data Register Empty
  *   Occurs when data register is empty
  */
  if(interrupts & (1<<0)) // 1 = 0001 = DRE
  {
    SERCOM->SPI.INTFLAG.bit.DRE = 1;
  }
}

inline void OnTransmissionStart()
{
  resetInternalState();
}

inline void OnTransmissionStop()
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




static inline void ADCsync() {
  while (ADC->STATUS.bit.SYNCBUSY == 1); //Just wait till the ADC is free
}

inline void SelAnalog(uint32_t ulPin){      // Selects the analog input channel in INPCTRL
  ADCsync();
  //ADC->INPUTCTRL.bit.MUXPOS = g_APinDescription[ulPin].ulADCChannelNumber; // Selection for the positive ADC input
  ADC->INPUTCTRL.bit.MUXPOS = ulPin;
}

void FastADCsetup() {
  const byte gClk = 3; //used to define which generic clock we will use for ADC
  const int cDiv = 1; //divide factor for generic clock
  
  ADC->CTRLA.bit.ENABLE = 0;                     // Disable ADC
  while( ADC->STATUS.bit.SYNCBUSY == 1 );        // Wait for synchronization
  ADC->CTRLB.reg = ADC_CTRLB_PRESCALER_DIV32 |   // Divide Clock by 64.
                   ADC_CTRLB_RESSEL_12BIT;       // Result on 12 bits
  ADC->AVGCTRL.reg = ADC_AVGCTRL_SAMPLENUM_1 |   // 1 sample
                     ADC_AVGCTRL_ADJRES(0x00ul); // Adjusting result by 0
  ADC->SAMPCTRL.reg = 0x00;                      // Sampling Time Length = 0
  ADC->CTRLA.bit.ENABLE = 1;                     // Enable ADC
  while( ADC->STATUS.bit.SYNCBUSY == 1 );        // Wait for synchronization
  
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

int16_t MuxAnalogRead(uint8_t mux, uint8_t chan){
    if (chan >= 0 && chan <= 15 && mux < NUM_MUX){     
      chan = MuxMapping[chan];      // Re-map hardware channels to have them read in the header order
    }
    else return -1;       // Return ERROR

  //Select channel
  bitRead(chan, 0) ?  PORT->Group[g_APinDescription[_S0].ulPort].OUTSET.reg = (1ul << g_APinDescription[_S0].ulPin) :
            PORT->Group[g_APinDescription[_S0].ulPort].OUTCLR.reg = (1ul << g_APinDescription[_S0].ulPin) ;
  bitRead(chan, 1) ?  PORT->Group[g_APinDescription[_S1].ulPort].OUTSET.reg = (1ul << g_APinDescription[_S1].ulPin) :
            PORT->Group[g_APinDescription[_S1].ulPort].OUTCLR.reg = (1ul << g_APinDescription[_S1].ulPin) ;
  bitRead(chan, 2) ?  PORT->Group[g_APinDescription[_S2].ulPort].OUTSET.reg = (1ul << g_APinDescription[_S2].ulPin) :
            PORT->Group[g_APinDescription[_S2].ulPort].OUTCLR.reg = (1ul << g_APinDescription[_S2].ulPin) ;
  bitRead(chan, 3) ?  PORT->Group[g_APinDescription[_S3].ulPort].OUTSET.reg = (1ul << g_APinDescription[_S3].ulPin) :
            PORT->Group[g_APinDescription[_S3].ulPort].OUTCLR.reg = (1ul << g_APinDescription[_S3].ulPin) ;

  return AnalogReadFast(muxPin[mux]);
}

bool isNoise(analogType *input, uint8_t threshold) {
  if (input->direction == ANALOG_INCREASING){   // CASE 1: If signal is increasing,
    if(input->rawValue > input->rawValuePrev){  // and the new value is greater than the previous,
       return 0;                                // NOT NOISE!
    }
    else if(input->rawValue < (input->rawValuePrev - threshold)){  
      // If, otherwise, it's lower than the previous value and the noise threshold together,
      // means it started to decrease,
      input->direction = ANALOG_DECREASING;
      return 0;                                 // NOT NOISE!
    }
  }
  if (input->direction == ANALOG_DECREASING){   // CASE 2: If signal is increasing,
    if(input->rawValue < input->rawValuePrev){  // and the new value is lower than the previous,
       return 0;                                // NOT NOISE!
    }
    else if(input->rawValue > (input->rawValuePrev + threshold)){  
      // If, otherwise, it's greater than the previous value and the noise threshold together,
      // means it started to increase,
      input->direction = ANALOG_INCREASING;

      return 0;                                 // NOT NOISE!
    }
  }
  return 1; // If everyting above was not true, then IT'S NOISE! Pot is trying to fool us. But we owned you pot ;)
}


void setup (void)
{
  Serial.begin(250000);   // debugging

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
  #if defined(DEBUG)
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
        #if defined(DEBUG)
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
