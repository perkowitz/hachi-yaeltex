/*
 * Copyright (c) 2014, Majenko Technologies
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

#include "headers/SPIaddressableModule.h"

SPIaddressableModule::SPIaddressableModule() {}

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


void SPIaddressableModule::begin(SPIClass *spi, uint8_t cs, uint8_t addr) { 
    _spi = spi;
    _cs = cs;
    _addr = addr;
    _configSPISettings = SPISettings(500000,MSBFIRST,SPI_MODE0);

    uint8_t cmd = OPCODEW_YTX | ((_addr & 0b11111) << 1);
  _spi->beginTransaction(_configSPISettings);
    ::digitalWrite(_cs, LOW);
    _spi->transfer(cmd);
    _spi->transfer(0x0F);
    _spi->transfer(ADDR_ENABLE);
    ::digitalWrite(_cs, HIGH);
  _spi->endTransaction();
}

/*! This private function performs a bulk read on all the registers in the chip to
 *  ensure the _reg array contains all the correct current values.
 */
void SPIaddressableModule::readAll() {
    uint8_t cmd = OPCODER_YTX | ((_addr & 0b11111) << 1);
  _spi->beginTransaction(_configSPISettings);
    ::digitalWrite(_cs, LOW);
    _spi->transfer(cmd);
    _spi->transfer(REGISTRER_OFFSET);//index of first valid register in module
    _spi->transfer(0xFF);//dummy 
    for (uint8_t i = 0; i < REGISTRER_COUNT; i++) {
        _reg[i] = _spi->transfer(0xFF);
    }
    ::digitalWrite(_cs, HIGH);
  _spi->endTransaction();
}

/*! This private function performs a bulk write of all the data in the _reg array
 *  out to all the registers on the chip.  It is mainly used during the initialisation
 *  of the chip.
 */
void SPIaddressableModule::writeAll() {
    uint8_t cmd = OPCODEW_YTX | ((_addr & 0b11111) << 1);
  _spi->beginTransaction(_configSPISettings);
    ::digitalWrite(_cs, LOW);
    _spi->transfer(cmd);
    _spi->transfer(REGISTRER_OFFSET); //FIRST
    for (uint8_t i = 0; i < REGISTRER_COUNT; i++) {
        _spi->transfer(_reg[i]);
    }
    ::digitalWrite(_cs, HIGH);
  _spi->endTransaction();
}
