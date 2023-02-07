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

int SPIinfinitePot::readEncoder(uint32_t n) {
    if(state&(1<<(4+n))){
      state &= ~(1<<(4+n));

      if(state&(1<<n))
        return 1;
      else
        return -1;
    }
    else
      return 0;
}

uint16_t SPIinfinitePot::readModule() {
    uint16_t data = 0;
    uint8_t cmd = OPCODER_YTX | ((_addr & 0b11111) << 1);
  _spi->beginTransaction(_configSPISettings);
    ::digitalWrite(_cs, LOW);
    _spi->transfer(cmd);
    _spi->transfer(REGISTRER_OFFSET);//index of first data register in module
    _spi->transfer(0xFF);//dummy 
    data = (uint16_t)(_spi->transfer(0xFF));
    data |= (uint16_t)(_spi->transfer(0xFF)<<8);
    ::digitalWrite(_cs, HIGH);
  _spi->endTransaction();

  return data;
}

void SPIinfinitePot::setNextAddress(uint8_t nextAddress) {
  if (nextAddress >= 8) {
    return;
  }
    uint8_t cmd = OPCODEW_YTX | ((_addr & 0b11111) << 1);
  _spi->beginTransaction(_configSPISettings);
    ::digitalWrite(_cs, LOW);
    _spi->transfer(cmd);
    _spi->transfer(0); //index of first control register in module
    _spi->transfer(nextAddress);
    _spi->transfer(0x01); //configure flag
    ::digitalWrite(_cs, HIGH);
  _spi->endTransaction();
}


