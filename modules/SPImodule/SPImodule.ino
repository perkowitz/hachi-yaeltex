#include <Arduino.h>
#include "pins_arduino.h"
#include "wiring_private.h"
#include <SPI.h>

#define CONCAT_(a,b) a ## b
#define CONCAT(a,b) CONCAT_(a,b)

char buf [100];
volatile byte pos;
volatile byte cnt;
volatile boolean process_it;
volatile boolean response_it;
volatile boolean get_address;
volatile boolean interruptsResponseFlag;

uint8_t myAddress = 0;
volatile uint8_t requestedAddress = 0;

#define SERCOM SERCOM4
#define IRQ(sercom_n) CONCAT(sercom_n,_IRQn)

#define SYNC_BYTE '\n'

#define FRAMEWORK // comment this line to use direct registrer manipulation
//#define DEBUG // comment this line out to not print debug data on the serial bus

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
  SERCOM->SPI.INTENSET.bit.DRE = 0x1; //Data Register Empty interrupt
  
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

void takeMISObus()
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

void giveMISObus()
{
  #ifdef FRAMEWORK
    pinMode(MISO, INPUT);
  #else
    PORT->Group[MISO_PORT].PINCFG[MISO_PIN].bit.PMUXEN = 0;
    PORT->Group[MISO_PORT].PINCFG[MISO_PIN].bit.INEN = 1;
    PORT->Group[MISO_PORT].DIRCLR.reg |= (1ul << MISO_PIN) ;
  #endif
}

void setup (void)
{
  SerialUSB.begin (250000);   // debugging

  pos = 0;
  cnt = 0;
  process_it = false;
  response_it = false;
  get_address = false;
  interruptsResponseFlag = false;

  giveMISObus();
  // turn on SPI in slave mode  
  spiSlave_init();
}  // end of setup


// main loop - wait for flag set in interrupt routine

int antMillis = 0;
void loop (void)
{
  if (process_it)
  {
    process_it = false;
    buf [pos] = 0;
    pos = 0;
    SerialUSB.println (buf);
  }  // end of flag set

  if(interruptsResponseFlag)
  {
    interruptsResponseFlag = false;
    SerialUSB.println ("Interrupt response..");
  }
}  // end of loop

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

    if (data == SYNC_BYTE)
    {
      if(requestedAddress==myAddress)
      {
        process_it = true; 
        response_it = true; 
      }
    }
    else
    {
      if(get_address)
      {
        get_address = false;

        requestedAddress = data;
        if(requestedAddress==myAddress)
        {
          takeMISObus();
        } 
      }
      else
      {
          if (get_address==false && requestedAddress==myAddress)
          {
            if(pos < sizeof(buf))
              buf [pos++] = data;
          }
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
    // Write some test data to buffer. Master will shift out 'dataCount' increments of 'data'
    if(get_address==false && requestedAddress==myAddress)
    {
      interruptsResponseFlag = true;
      if(response_it)
      {
        response_it = false;
        cnt++;
      }
    }
    SERCOM->SPI.DATA.reg = cnt;

    //SERCOM4->SPI.INTFLAG.bit.DRE = 1;
  }
}

void OnTransmissionStart()
{
  pos = 0;
  process_it = false;
  response_it = false;

  get_address = true;
}

void OnTransmissionStop()
{
  if(requestedAddress==myAddress)
    giveMISObus();
}