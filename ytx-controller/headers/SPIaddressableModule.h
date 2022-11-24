/*
 * Copyright (c) , Majenko Technologies
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


#ifndef _SPIaddressable_H
#define _SPIaddressable_H

#if (ARDUINO >= 100) 
# include <Arduino.h>
#else
# include <WProgram.h>
#endif

#include <SPI.h>

#define    OPCODE_SET_NA (0b01000000)

#define    REGISTRER_OFFSET 0x10
#define    REGISTRER_COUNT  16

class SPIaddressableModule {     
    public:
        /*! Local mirrors of the 22 internal registers of the MCP23S17 chip */  
        uint8_t _reg[REGISTRER_COUNT];   

        SPIaddressableModule();
		void begin(SPIClass *spi, uint8_t cs, uint8_t addr);
        void setNextAddress(uint8_t nextAddress);

        //void readRegister(uint8_t addr); 
        //void writeRegister(uint8_t addr);
        void readAll();
        void writeAll();

        SPISettings configSPISettings;
    protected:
        SPIClass *_spi; /*! This points to a valid SPI object created from the Arduino SPI library. */
        uint8_t _cs;    /*! Chip select pin */
        uint8_t _addr;  /*! 3-bit chip address */   
};

class SPIAnalogExpander : public SPIaddressableModule {     
    public:
        uint16_t analogRead(uint32_t n);
        void analogReadAll(void);
        void getDifferences(void);

        uint16_t rawValue[96];
        uint8_t differences[96/8];
};
#endif
