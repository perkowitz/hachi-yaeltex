/*
 * Copyright (c) 2023, YAELTEX Technologies
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
 *  3. Neither the name of YAELTEX Technologies nor the names of its contributors may be used
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

#include "SPIEndlessPot.h"

void SPIEndlessPot::begin(SPIAdressableBUS *_spiBUS, uint8_t _addr,uint8_t _next_addr) {
    addr = _addr;
    spiBUS = _spiBUS;

    if(addr<8){
        base = INF_POT_BASE_ADDRESS_A<<4;
    }else{
        base = INF_POT_BASE_ADDRESS_B<<4;
        addr -= 8;
    }

    enableAddressing(_next_addr);
    delay(5); // wait addresing ready
}
void SPIEndlessPot::configure(SPIEndlessPotParameters *parameters){
    writeChunk(0,parameters,sizeof(SPIEndlessPotParameters));
    delay(5); // wait config ready
}

uint16_t SPIEndlessPot::readModule() {
  uint16_t data;
  readChunk(REGISTER_OFFSET,&data,sizeof(data));
  return data;
}

