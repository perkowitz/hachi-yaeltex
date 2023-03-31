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
 *  3. Neither the name of YAELTEX nor the names of its contributors may be used
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

#include "SPIAddressable.h"


SPIAdressableBUS::SPIAdressableBUS() {}

void SPIAdressableBUS::begin(SPIClass *_spi, SPISettings _spiSettings, uint8_t _cs) { 
    port = _spi;
    cs = _cs;
    settings = _spiSettings;

    pinMode(cs, OUTPUT);
}

void SPIAdressableBUS::DisableHWAddress(uint8_t base){
  byte cmd = 0;
  // DISABLE HARDWARE ADDRESSING FOR ALL CHIPS - ONLY NEEDED FOR RESET
  for (int n = 0; n < 8; n++) {
    cmd = OPCODEW | ((base | (n & 0b111)) << 1);
    port->beginTransaction(settings);
    ::digitalWrite(cs, LOW);
    port->transfer(cmd);
    port->transfer(IOCONA);                     // ADDRESS FOR IOCONA, for IOCON.BANK = 0
    port->transfer(ADDR_DISABLE);
    ::digitalWrite(cs, HIGH);
    port->endTransaction();
    
    port->beginTransaction(settings);
    ::digitalWrite(cs, LOW);
    port->transfer(cmd);
    port->transfer(IOCONB);                     // ADDRESS FOR IOCONB, for IOCON.BANK = 0
    port->transfer(ADDR_DISABLE);
    ::digitalWrite(cs, HIGH);
    port->endTransaction();

    port->beginTransaction(settings);
    ::digitalWrite(cs, LOW);
    port->transfer(cmd);
    port->transfer(5);                          // ADDRESS FOR IOCONA, for IOCON.BANK = 1 
    port->transfer(ADDR_DISABLE); 
    ::digitalWrite(cs, HIGH);
    port->endTransaction();

    port->beginTransaction(settings);
    ::digitalWrite(cs, LOW);
    port->transfer(cmd);
    port->transfer(15);                          // ADDRESS FOR IOCONB, for IOCON.BANK = 1 
    port->transfer(ADDR_DISABLE); 
    ::digitalWrite(cs, HIGH);
    port->endTransaction();
  }
}


SPIAddressableElement::SPIAddressableElement() {}

/*! The begin function takes three parameters.  The first is an SPI class
 *  pointer.  This is the address of an SPI object (either the default
 *  SPI object on the Arduino, or an object made using the DSPIx classes
 *  on the chipKIT).  The second parameter is the chip select pin number
 *  to use when communicating with the chip.  The third is the internal
 *  address number of the chip.  This is controlled by the three Ax pins
 *  on the chip.
 *  It also performs the initial configuration of the IO expander chip.
 *  Not only does it set up the SPI communications, but it also configures the chip
 *  for address-based communication and sets the default parameters and registers
 *  to sensible values.
 *
 *  Example:
 *
 *      myModule.begin(&SPI, 10, 0);
 *
 */

void SPIAddressableElement::begin(SPIAdressableBUS *_spiBUS, uint8_t _addr) {}

void SPIAddressableElement::begin(SPIAdressableBUS *_spiBUS, uint8_t _base, uint8_t _addr) {
    spiBUS = _spiBUS;
    addr = _addr;
    base = _base;

    uint8_t cmd = OPCODEW | ((base | (addr & 0b111)) << 1);
  spiBUS->port->beginTransaction(spiBUS->settings);
    ::digitalWrite(spiBUS->cs, LOW);
    spiBUS->port->transfer(cmd);
    spiBUS->port->transfer(0x0F);
    spiBUS->port->transfer(ADDR_ENABLE);
    ::digitalWrite(spiBUS->cs, HIGH);
  spiBUS->port->endTransaction();
}
