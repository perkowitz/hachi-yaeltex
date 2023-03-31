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

// LEGACY MCP23S17 opcodes and commands
#define    OPCODEW       (0b00000000)  // Opcode for MCP23S17 with LSB (bit0) set to write (0)
#define    OPCODER       (0b00000001)  // Opcode for MCP23S17 with LSB (bit0) set to read (1)
#define    ADDR_ENABLE   (0b00001000)  // Configuration register for MCP23S17, the only thing we change is enabling hardware addressing
#define    ADDR_DISABLE  (0b00000000)  // Configuration register for MCP23S17, the only thing we change is disabling hardware addressing
#define    SEQOP_ENABLE  (0b00100000)  // Configuration register for MCP23S17, the only thing we change is enabling sequential operation

class SPIAdressableBUS {     
    public:
        SPIAdressableBUS();
        void begin(SPIClass *, SPISettings, uint8_t);
        void DisableHWAddress(uint8_t);

        SPIClass *port; /*! This points to a valid SPI object created from the Arduino SPI library. */
        SPISettings settings;
        uint8_t cs;    /*! Chip select pin */
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
};

class SPIinfinitePot : public SPIAddressableElement {     
    public:
        void begin(SPIAdressableBUS *bus, uint8_t address) override;
        void configure(uint8_t,uint8_t,uint8_t);
        uint16_t readModule();
};
#endif
