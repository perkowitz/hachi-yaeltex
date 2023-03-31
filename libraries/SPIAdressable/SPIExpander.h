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


#ifndef _SPIExpander_H
#define _SPIExpander_H

#if (ARDUINO >= 100) 
# include <Arduino.h>
#else
# include <WProgram.h>
#endif

#include <SPI.h>

#include "SPIAddressable.h"



// registers
#define MCP23017_IODIRA     0x00
#define MCP23017_IPOLA      0x02
#define MCP23017_GPINTENA   0x04
#define MCP23017_DEFVALA    0x06
#define MCP23017_INTCONA    0x08
#define MCP23017_IOCONA     0x0A
#define MCP23017_GPPUA      0x0C
#define MCP23017_INTFA      0x0E
#define MCP23017_INTCAPA    0x10
#define MCP23017_GPIOA      0x12
#define MCP23017_OLATA      0x14


#define MCP23017_IODIRB     0x01
#define MCP23017_IPOLB      0x03
#define MCP23017_GPINTENB   0x05
#define MCP23017_DEFVALB    0x07
#define MCP23017_INTCONB    0x09
#define MCP23017_IOCONB     0x0B
#define MCP23017_GPPUB      0x0D
#define MCP23017_INTFB      0x0F
#define MCP23017_INTCAPB    0x11
#define MCP23017_GPIOB      0x13
#define MCP23017_OLATB      0x15

#define MCP23017_INT_ERR 255




#endif
