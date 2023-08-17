/*
 * Copyright (c) 2023, YAELTEX
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without modification, 
 * are permitted provided that the following conditions are met:
 * 
 *  1. Redistributions of source code must retain the above copyright notice, 
 *     this list of conditions and the following disclaimer.
 * 
 *  2. Redistributions in binary form must reproduce the above copyright notice,
 *     this list of conditions and the following disclaimer in the documentation
 *      and/or other materials provided with the distribution.
 * 
 *  3. Neither the name of Majenko Technologies nor the names of its contributors may be used
 *     to endorse or promote products derived from this software without 
 *     specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" 
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE 
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE 
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL 
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR 
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER 
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, 
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


#ifndef _SPIAddressable_H
#define _SPIAddressable_H

#if (ARDUINO >= 100) 
# include <Arduino.h>
#else
# include <WProgram.h>
#endif

#include <SPI.h>

#define SERIALPRINT(a)        {Serial.print(a);     }
#define SERIALPRINTLN(a)      {Serial.println(a);   }
#define SERIALPRINTF(a, f)    {Serial.print(a,f);   }
#define SERIALPRINTLNF(a, f)  {Serial.println(a,f); }

#define    OPCODE_SET_NA (0b01000000)

#define    REGISTRER_OFFSET 0x10
#define    REGISTRER_COUNT  16

#define    MCP23017_BASE_ADDRESS 0x20
#define    INF_POT_BASE_ADDRESS  0x00

// LEGACY MCP23S17 opcodes and commands
#define    OPCODEW       (0b00000000)  // Opcode for MCP23S17 with LSB (bit0) set to write (0)
#define    OPCODER       (0b00000001)  // Opcode for MCP23S17 with LSB (bit0) set to read (1)
#define    ADDR_ENABLE   (0b00001000)  // Configuration register for MCP23S17, the only thing we change is enabling hardware addressing
#define    ADDR_DISABLE  (0b00000000)  // Configuration register for MCP23S17, the only thing we change is disabling hardware addressing
#define    SEQOP_ENABLE  (0b00100000)  // Configuration register for MCP23S17, the only thing we change is enabling sequential operation

// LEGACY MCP23S17 registers
enum {
    IODIRA,     IODIRB,
    IPOLA,      IPOLB,
    GPINTENA,   GPINTENB,
    DEFVALA,    DEFVALB,
    INTCONA,    INTCONB,
    IOCONA,     IOCONB,
    GPPUA,      GPPUB,
    INTFA,      INTFB,
    INTCAPA,    INTCAPB,
    GPIOA,      GPIOB,
    OLATA,      OLATB,
    MCP23017_REGISTER_COUNT
};

class SPIAdressableBUS {     
    public:
        SPIAdressableBUS();
        void begin(SPIClass *, SPISettings, uint8_t);
        void DisableHWAddress(uint8_t);

        SPIClass *port; /*! This points to a valid SPI object created from the Arduino SPI library. */
        SPISettings settings;
        uint8_t cs;    /*! Chip select pin */
};

class SPIExpanderGroup : public SPIAdressableBUS{        
    public:
        SPIExpanderGroup();
        void SetAllAsOutput();
        void InitPinsGhostModules();
        void SetPullUps();
        void readAllRegs();
        void writeAllRegs(byte);
};

class SPIAddressableElement {     
    public:
        SPIAddressableElement();
        virtual void begin(SPIAdressableBUS *, uint8_t);
		virtual void begin(SPIAdressableBUS *, uint8_t, uint8_t);
    protected:
        SPIAdressableBUS *spiBUS; /*! This points to a valid SPI object created from the Arduino SPI library. */
        uint8_t base;  /*! base address */
        uint8_t addr;  /*! 3-bit chip address */   

        void enableAddressing();
        void readChunk(uint8_t index, void *data, uint8_t size);
        void writeChunk(uint8_t index, void *data, uint8_t size);
};

class SPIGPIOExpander : public SPIAddressableElement{
    private:
        uint8_t _reg[MCP23017_REGISTER_COUNT];   /*! Local mirrors of the 22 internal registers of the MCP23S17 chip */        

        void readRegister(uint8_t index); 
        void writeRegister(uint8_t index);
        void readAll();
        void writeAll();
        
    public:
        SPIGPIOExpander();
        void begin(SPIAdressableBUS *bus, uint8_t address) override;
        void pinMode(uint8_t pin, uint8_t mode);
        void digitalWrite(uint8_t pin, uint8_t value);
        uint8_t digitalRead(uint8_t pin);
        uint16_t digitalRead();
        
        void writeWord(uint8_t reg, uint16_t word);
        uint8_t readPort(uint8_t port);
        uint16_t readPort();
        void updateRegisterBit(uint8_t pin, uint8_t pValue, uint8_t portAaddr, uint8_t portBaddr);
        void writePort(uint8_t port, uint8_t val);
        void writePort(uint16_t val);
        uint8_t bitForPin(uint8_t pin);
        uint8_t regForPin(uint8_t pin, uint8_t portAaddr, uint8_t portBaddr);
        void pullUp(uint8_t p, uint8_t d);
        void enableInterrupt(uint8_t pin, uint8_t type);
        void disableInterrupt(uint8_t pin);
        void setMirror(boolean m);
        uint16_t getInterruptPins();
        uint16_t getInterruptValue();
        void setInterruptLevel(uint8_t level);
        void setInterruptOD(boolean openDrain);

};

class SPIEndlessPot : public SPIAddressableElement {     
    public:
        void begin(SPIAdressableBUS *bus, uint8_t address) override;
        void configure(uint8_t,uint8_t,uint8_t);
        uint16_t readModule();

        typedef struct __attribute__((packed)){
          uint8_t nextAddress;
          uint8_t sampleInterval;
          uint8_t hysteresis;
          uint8_t configFlag;
        }moduleConfig;
};

#ifdef SPI_SLAVE
class SPIAddressableSlave {
  public:
    // Constructors //
    SPIAddressableSlave();

    // Public methods //
    void begin(int,int,int);
    void hook();
    void wiring(int*,int*);
    void takeMISObus();
    void leaveMISObus();
    void resetInternalState();
    void getAddress();
    void setNextAddress(int);
    void setTransmissionCompleteCallback(voidFuncPtr);

    volatile uint8_t  base;
    volatile uint8_t  configureRegister;
    volatile uint8_t  registersCount;
    volatile uint32_t myAddress;
    volatile uint32_t nextAddress;

    volatile uint32_t state;

    volatile uint32_t registerIndex;
    volatile uint32_t receivedBytes;
    volatile uint8_t* registers;
    volatile uint8_t* controlRegister;
    volatile uint8_t* userDataRegister;

    volatile bool isAddressEnable;
    volatile bool isTransmissionComplete;
    volatile bool updateAddressingMode;
    
  private:
    void SercomInit();

    int inputAddressPin[3];
    int outputAddressPin[3];

    void (*transmissionCompleteCallback)(void);
};

extern SPIAddressableSlave SPIAddressableSlaveModule;
#endif // SPI_SLAVE
#endif //_SPIAddressable_H
