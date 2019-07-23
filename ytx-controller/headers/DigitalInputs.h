#ifndef DIGITAL_INPUTS_H
#define DIGITAL_INPUTS_H

#include <SPI.h>
#include "MCP23S17.h"  // Majenko library
#include "modules.h"
#include "FeedbackClass.h"
//----------------------------------------------------------------------------------------------------
// CLASS DEFINITION
//----------------------------------------------------------------------------------------------------

#define BOUNCE_MILLIS		40 

class DigitalInputs{
private:
	uint8_t nBanks;
	uint8_t nDigital;
	uint8_t nModules;
	
	// setup the port expander
	SPIClass *spi;
	const uint8_t digitalMCPChipSelect1 = 7;
	const uint8_t digitalMCPChipSelect2 = 10;

  	typedef struct{
  		MCP23S17 digitalMCP;
		uint16_t mcpState;
	  	uint16_t mcpStatePrev;
	  	uint8_t moduleType;
  	}moduleData;
	moduleData *mData;
	
  	typedef struct{
  		uint8_t digitalInputState;
		uint8_t digitalInputStatePrev;
  	}digitalBankData;
  	digitalBankData **dBankData;

	typedef struct{
  		uint8_t digitalHWState;
		uint8_t digitalHWStatePrev;
		uint32_t swBounceMillisPrev;
  	}digitalHwData;  	
	digitalHwData *dHwData;

	void SetNextAddress(MCP23S17, byte);

public:
	void Init(uint8_t,uint8_t,uint8_t,SPIClass*);
	void Read();

	
};


#endif