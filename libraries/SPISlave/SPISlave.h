#ifndef SPISlave_h
#define SPISlave_h

#include <Arduino.h>
#include "pins_arduino.h"
#include "wiring_private.h"

#define FRAMEWORK // comment this line to use direct registrer manipulation

#define CONCAT_(a,b) a ## b
#define CONCAT(a,b) CONCAT_(a,b)

#define SERCOM SERCOM4
#define IRQ(sercom_n) CONCAT(sercom_n,_IRQn)

#define ADDR_ENABLE   (0b00001000)  // Configuration register for MCP23S17, the only thing we change is enabling hardware addressing
#define ADDR_DISABLE  (0b00000000)  // Configuration register for MCP23S17, the only thing we change is disabling hardware addressing

#define CONTROL_REGISTERS 16  // Legacy Configuration register for MCP23S17

typedef void (*voidFuncPtr)(void);

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

class SPISlave {
  public:
    // Constructors //
    SPISlave();

    // Public methods //
    void begin(int,int,int);
    void hook();
    void wiring(int*,int*);
    void takeMISObus();
    void leaveMISObus();
    void resetInternalState();
    void setTransmissionCompleteCallback(voidFuncPtr);

    uint8_t  opcode;
    uint8_t  configureRegister;
    uint8_t  registersCount;
    uint32_t myAddress;
    uint32_t nextAddress;

    volatile uint32_t state;

    volatile uint32_t registerIndex;
    volatile uint32_t receivedBytes;
    volatile uint8_t* registers;
    volatile uint8_t* controlRegister;
    volatile uint8_t* userDataRegister;

    volatile bool isTransmissionComplete;
    volatile bool isAddressEnable;
  private:
    void SercomInit();

    int inputAddressPin[3];
    int outputAddressPin[3];

    void (*transmissionCompleteCallback)(void);
};

extern SPISlave SPISlaveModule;

#endif