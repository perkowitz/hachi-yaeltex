/*

Author/s: Franco Grassano - Franco Zaccra

MIT License

Copyright (c) 2020 - Yaeltex

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

*/

#ifndef DIGITAL_INPUTS_H
#define DIGITAL_INPUTS_H

#include <SPI.h>
#include "../lib/SPIExpander/SPIExpander.h"  // Majenko library
#include "modules.h"
#include "FeedbackClass.h"
//----------------------------------------------------------------------------------------------------
// CLASS DEFINITION
//----------------------------------------------------------------------------------------------------

#define BOUNCE_MILLIS		50 

class DigitalInputs{
private:
	uint8_t nBanks;
	uint16_t nDigitals;
	uint8_t nModules;
	bool begun;
	
	// setup the port expander
	SPIClass *spi;
	const uint8_t digitalMCPChipSelect1 = 7;
	const uint8_t digitalMCPChipSelect2 = 10;
	SPIExpander digitalMCP[MAX_DIGITAL_MODULES];

	uint8_t individualScanInterval;
	uint32_t generalMillis;	

  	typedef struct{
		uint16_t 	mcpState;
	  	uint16_t 	mcpStatePrev;
	  	uint8_t 	moduleType;
	  	uint16_t	digitalIndexStart;
	  	unsigned long antMillisScan;
  	}moduleData;
	moduleData *digMData;
	
  	typedef struct __attribute__((packed)){
  		uint8_t digitalInputState : 1;
		uint8_t digitalInputStatePrev : 1;
		uint16_t lastValue : 14;
  	}digitalBankData;
  	digitalBankData **dBankData;

	typedef struct __attribute__((packed)){
  		uint8_t digitalHWState : 1;
		uint8_t digitalHWStatePrev : 1;
  	}digitalHwData;  	
	digitalHwData *dHwData;

	uint8_t currentProgram[2][16]; 	// Program change # for each port (USB and HW) and channel

	void SetNextAddress(uint8_t, uint8_t);
	void DigitalAction(uint16_t,uint16_t);
	void CheckIfChanged(uint8_t);
	void EnableHWAddress();
	void DisableHWAddress();
	void SetPullUps();
public:
	void Init(uint8_t,uint16_t,SPIClass*);
	void Read();
	void SetDigitalValue(uint8_t,uint16_t,uint16_t);
	uint16_t GetDigitalValue(uint16_t);
	bool GetDigitalState(uint16_t);
};


#endif