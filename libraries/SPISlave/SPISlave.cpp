#include "SPISlave.h"

#define SERIALPRINT(a)        {Serial.print(a);     }
#define SERIALPRINTLN(a)      {Serial.println(a);   }
#define SERIALPRINTF(a, f)    {Serial.print(a,f);   }
#define SERIALPRINTLNF(a, f)  {Serial.println(a,f); }

SPISlave::SPISlave(){};

void SPISlave::begin(int _opcode,int _ctrlRegisters,int _usrRegisters)
{
  myAddress = 0;
  isAddressEnable = false;
  isTransmissionComplete = 0;

  opcode = (uint8_t)_opcode;
  configureRegister = (uint8_t)_ctrlRegisters;

  registersCount = CONTROL_REGISTERS + _usrRegisters;
  registers = (uint8_t*)malloc(registersCount);
  memset((void*)registers,0,sizeof(registersCount));

  controlRegister = &registers[1];
  userDataRegister = &registers[CONTROL_REGISTERS];

  //turn on SPI in slave mode
  SercomInit();
  resetInternalState();
}

void SPISlave::hook()
{
  static uint32_t antMillisAddress = 0;

  if(millis()-antMillisAddress>1000)
  {
    antMillisAddress = millis();

    int address = 0;
    
    for(int i=0;i<3;i++){
      if(digitalRead(inputAddressPin[i])==HIGH)
        address += 1<<i;
    }

    if(myAddress!=address)
      myAddress = address;
  }

  if(isTransmissionComplete){
    registerIndex = 0;
    isTransmissionComplete = 0;

    transmissionCompleteCallback();

    if(controlRegister[configureRegister]){
      controlRegister[configureRegister] = 0;

      //MAPPING::NEXT_ADDRESS
      nextAddress = registers[0];
      
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
}

void SPISlave::wiring(int* inputWiring,int* outputWiring)
{
  //init addressing i/o pins
  for(int i=0;i<3;i++){
    inputAddressPin[i] = inputWiring[i];
    outputAddressPin[i] = outputWiring[i];
    pinMode(inputAddressPin[i],INPUT);
    pinMode(outputAddressPin[i],OUTPUT);
    digitalWrite(outputAddressPin[i],LOW);
  }
}

void SPISlave::setTransmissionCompleteCallback(voidFuncPtr callback)
{
  transmissionCompleteCallback = callback;
}


void SPISlave::SercomInit()
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

inline void OnTransmissionStart()
{
  SPISlaveModule.resetInternalState();
}

inline void OnTransmissionStop()
{
  
}

inline void SPISlave::takeMISObus()
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

inline void SPISlave::leaveMISObus()
{
  #ifdef FRAMEWORK
    pinMode(MISO, INPUT);
  #else
    PORT->Group[MISO_PORT].PINCFG[MISO_PIN].bit.PMUXEN = 0;
    PORT->Group[MISO_PORT].PINCFG[MISO_PIN].bit.INEN = 1;
    PORT->Group[MISO_PORT].DIRCLR.reg = (1ul << MISO_PIN);
  #endif
}

inline void SPISlave::resetInternalState(void)
{
  leaveMISObus();
  state = GET_OPCODE;
  receivedBytes = 0;
}

void SERCOM4_Handler(void)
{
  volatile uint8_t interrupts = SERCOM->SPI.INTFLAG.reg; //Read SPI interrupt register

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
    volatile uint8_t data = (uint8_t)SERCOM->SPI.DATA.reg;
    static volatile uint8_t opcode;
    
    SPISlaveModule.receivedBytes++;

    if(SPISlaveModule.receivedBytes==1)
    {
      volatile uint8_t requestedAddress;

      opcode = data&0b11000001;
      requestedAddress = (data&0b00111110)>>1;

      if(SPISlaveModule.isAddressEnable)
      {
        if(requestedAddress==SPISlaveModule.myAddress)
        {
          SPISlaveModule.state = GET_REG_INDEX;
          SPISlaveModule.takeMISObus();
        }
        else
          SPISlaveModule.leaveMISObus();
      }
      else
      {
        SPISlaveModule.state = GET_REG_INDEX;
        SPISlaveModule.takeMISObus();
      }
    }  
    else if(SPISlaveModule.receivedBytes == 2)
    {
      if(SPISlaveModule.state==GET_REG_INDEX)
      {
        SPISlaveModule.registerIndex = constrain(data,0,SPISlaveModule.registersCount-1);
        SPISlaveModule.state = GET_TRANSFER;
        if(opcode==SPISlaveModule.opcode) //Write opcode
        {
          SERCOM->SPI.INTFLAG.bit.RXC = 1;
          return;
        }
      }
    }

    if(SPISlaveModule.state == GET_TRANSFER)
    {
      if(opcode==(SPISlaveModule.opcode+1)) //Read opcode
      {
        SERCOM->SPI.DATA.reg = SPISlaveModule.registers[SPISlaveModule.registerIndex];
      }
      else if(opcode==SPISlaveModule.opcode) //Write opcode
      {
        if(SPISlaveModule.registerIndex==0x05 || SPISlaveModule.registerIndex==0x0A || 
          SPISlaveModule.registerIndex==0x0B || SPISlaveModule.registerIndex==0x0F)
        {
          if(data==ADDR_ENABLE)
            SPISlaveModule.isAddressEnable = true;
          else if(data==ADDR_DISABLE)
            SPISlaveModule.isAddressEnable = false;
        }
        else
        {
          SPISlaveModule.registers[SPISlaveModule.registerIndex] = data;
        }
      }

      if(++SPISlaveModule.registerIndex == SPISlaveModule.registersCount)
      {
        SPISlaveModule.isTransmissionComplete = 1;
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

SPISlave SPISlaveModule;

