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

uint16_t SPIAnalogExpander::analogRead(uint32_t n) {
    uint16_t value;
    uint8_t cmd = OPCODER | ((_addr & 0b111) << 1);
  _spi->beginTransaction(configSPISettings);
    ::digitalWrite(_cs, LOW);
    _spi->transfer(cmd);
    _spi->transfer(REGISTRER_OFFSET+n*2);//index of first valid register in module
    _spi->transfer(0xFF);//dummy 
    value = (uint16_t)(_spi->transfer(0xFF));
    value += (uint16_t)(_spi->transfer(0xFF))<<8;
    ::digitalWrite(_cs, HIGH);
  _spi->endTransaction();
  return value;
}

void SPIAnalogExpander::analogReadAll(void) {
  uint8_t cmd = OPCODER | ((_addr & 0b111) << 1);
  _spi->beginTransaction(configSPISettings);
  ::digitalWrite(_cs, LOW);
  _spi->transfer(cmd);
  _spi->transfer(REGISTRER_OFFSET);//index of first valid register in module
  _spi->transfer(0xFF);//dummy 
  for(int i=0;i<96;i++)
  {
    rawValue[i] = (int)(_spi->transfer(0xFF));
    rawValue[i] += (int)(_spi->transfer(0xFF)<<8);
  }
  ::digitalWrite(_cs, HIGH);
  _spi->endTransaction();
}

void SPIAnalogExpander::getDifferences(void) {
  uint8_t cmd = OPCODER | ((_addr & 0b111) << 1);
  _spi->beginTransaction(configSPISettings);
  ::digitalWrite(_cs, LOW);
  _spi->transfer(cmd);
  _spi->transfer(REGISTRER_OFFSET+96*2);//index of first valid register in module
  _spi->transfer(0xFF);//dummy 
  for(int i=0;i<96/8;i++)
  {
    differences[i] = (uint8_t)(_spi->transfer(0xFF));
  }
  ::digitalWrite(_cs, HIGH);
  _spi->endTransaction();
}