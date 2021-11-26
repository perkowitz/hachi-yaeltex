#include <Arduino.h>
#include "pins_arduino.h"
#include "wiring_private.h"
#include <SPI.h>

#define CONCAT_(a,b) a ## b
#define CONCAT(a,b) CONCAT_(a,b)

#define SERCOM SERCOM4
#define IRQ(sercom_n) CONCAT(sercom_n,_IRQn)

//#define FRAMEWORK // comment this line to use direct registrer manipulation
//#define DEBUG // comment this line out to not print debug data on the serial bus

//MCP23S17 legacy opcodes 
#define    OPCODEW       (0b01000000)  // Opcode for MCP23S17 with LSB (bit0) set to write (0), address OR'd in later, bits 1-3
#define    OPCODER       (0b01000001)  // Opcode for MCP23S17 with LSB (bit0) set to read (1), address OR'd in later, bits 1-3
//MCP23S17 legacy commands 
#define    ADDR_ENABLE   (0b00001000)  // Configuration register for MCP23S17, the only thing we change is enabling hardware addressing
#define    ADDR_DISABLE  (0b00000000)  // Configuration register for MCP23S17, the only thing we change is disabling hardware addressing

#define    REGISTRER_OFFSET 0x10
#define    REGISTRER_COUNT  16

volatile boolean debugFlag;
volatile boolean addressEnable;

volatile uint32_t state;
volatile uint8_t opcode;

volatile uint32_t myAddress;
volatile uint32_t requestedAddress;

volatile uint32_t registerIndex;
volatile uint8_t  registerValues[REGISTRER_OFFSET+REGISTRER_COUNT];
volatile uint32_t receivedBytes;

enum machineSates
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
    PORT->Group[MISO_PORT].DIRCLR.reg |= (1ul << MISO_PIN) ;
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
      opcode = data&0b11110001;
      requestedAddress = (data&0b00001110)>>1;

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
        if(opcode==OPCODEW)
        {
          SERCOM->SPI.INTFLAG.bit.RXC = 1;
          return;
        }
      }
    }

    if(state == GET_TRANSFER)
    {
      if(opcode==OPCODER)
      {
        SERCOM->SPI.DATA.reg = registerValues[registerIndex];
        registerValues[registerIndex] = 128;
      }
      else if(opcode==OPCODEW)
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

}


// Input/Output declarations
#define POT_A_INPUT A0           //Analog input 1
#define POT_B_INPUT A1           //Analog input 2

// Constant value definitions

#define ADC_HYSTERESIS  8        //Must be 1 or higher. Noise filter, determines how big ADC change needed
#define MAX_POT_VALUE   127
#define POT_SENSITIVITY 8        //Higher number = more turns needed to reach max value

#define ADC_MAX_VALUE   1023

#define POT_COUNT       2


// Variables for potmeter
int ValuePotA[POT_COUNT];            //Pot1 tap A value
int ValuePotB[POT_COUNT];           //Pot1 tap B value
int PreviousValuePotA[POT_COUNT];    //Used to remember last value to determine turn direction
int PreviousValuePotB[POT_COUNT];    //Used to remember last value to determine turn direction
int DirPotA[POT_COUNT];              //Direction for Pot 1 tap A
int DirPotB[POT_COUNT];              //Direction for Pot1 tap B

int Direction[POT_COUNT];         //Final CALCULATED direction
int Value[POT_COUNT];              //Final CALCULATED value

int analogInputs[POT_COUNT][2]={{A0,A1},{A4,A5}};

void setup (void)
{
  resetInternalState();

  myAddress = 4;
  addressEnable = true;

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
  //SerialUSB.begin (250000);   // debugging

  // turn on SPI in slave mode  
  spiSlave_init();
}// end of setup

void loop()
{
  // Update ADC readings
  for(int i=0;i<POT_COUNT;i++)
  {
    ValuePotA[i] = analogRead(analogInputs[i][0]);
    ValuePotB[i] = analogRead(analogInputs[i][1]);
    
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
      
      // SerialUSB.print("Pot ");
      // SerialUSB.print(i);
      // SerialUSB.print(": ");
      // SerialUSB.println(Direction[i]);

      registerValues[REGISTRER_OFFSET+i] = Direction[i]+128;
    }
  }

  delay(5);
}