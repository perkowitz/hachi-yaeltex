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

void SPIAnalogExpander::configure(uint8_t inputs,float avgFilter, uint8_t noiseTh){
    uint8_t cmd = OPCODEW_YTX | ((_addr & 0b11111) << 1);
    uint8_t filter = (uint8_t)(avgFilter*255);
  _spi->beginTransaction(_configSPISettings);
    ::digitalWrite(_cs, LOW);
    _spi->transfer(cmd);
    _spi->transfer(0x00); //index of first valid register in module
    _spi->transfer(inputs);
    _spi->transfer(filter);
    _spi->transfer(noiseTh);
    _spi->transfer(0x01); //configure flag
    ::digitalWrite(_cs, HIGH);
  _spi->endTransaction();
}

void SPIAnalogExpander::getActiveChannels(void) {
  uint8_t cmd = OPCODER_YTX | ((_addr & 0b11111) << 1);
  _spi->beginTransaction(_configSPISettings);
  ::digitalWrite(_cs, LOW);
  _spi->transfer(cmd);
  _spi->transfer(REGISTRER_OFFSET+96*2);//index of first active channel register
  _spi->transfer(0xFF);//dummy 
  for(int i=0;i<96/8;i++)
  {
    activeChannels[i] = (uint8_t)(_spi->transfer(0xFF));
  }
  ::digitalWrite(_cs, HIGH);
  _spi->endTransaction();
}

bool SPIAnalogExpander::isActiveChannel(uint32_t n) {
  return (bool)(activeChannels[n/8] & (1<<(n%8)));
}

uint16_t SPIAnalogExpander::analogRead(uint32_t n) {
    uint16_t value;
    uint8_t cmd = OPCODER_YTX | ((_addr & 0b11111) << 1);
  _spi->beginTransaction(_configSPISettings);
    ::digitalWrite(_cs, LOW);
    _spi->transfer(cmd);
    _spi->transfer(REGISTRER_OFFSET+n*2);//index of analog value register
    _spi->transfer(0xFF);//dummy 
    value = (uint16_t)(_spi->transfer(0xFF));
    value += (uint16_t)(_spi->transfer(0xFF))<<8;
    ::digitalWrite(_cs, HIGH);
  _spi->endTransaction();
  return value;
}