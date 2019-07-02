#ifndef DIGITAL_INPUTS_H
#define DIGITAL_INPUTS_H

#include <SPI.h>
#include "MCP23S17.h"  // Majenko library
#include "modules.h"

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
	MCP23S17 *digitalMCP;
	SPIClass *spi;
	const uint8_t digitalMCPChipSelect1 = 7;
	const uint8_t digitalMCPChipSelect2 = 10;
	uint8_t *moduleType;

	uint16_t *mcpState;
  	uint16_t *mcpStatePrev;

	bool **digitalInputState;
	bool **digitalInputStatePrev;

	uint8_t *digitalHWState;
	uint8_t *digitalHWStatePrev;
	uint32_t *swBounceMillisPrev;

	void SetNextAddress(MCP23S17, byte);

public:
	void Init(uint8_t,uint8_t,uint8_t,SPIClass*);
	void Read();
};


#endif