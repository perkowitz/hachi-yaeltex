#ifndef ENCODER_INPUTS_H
#define ENCODER_INPUTS_H

#include <SPI.h>
#include "MCP23S17.h"  // Majenko library

//----------------------------------------------------------------------------------------------------
// CLASS DEFINITION
//----------------------------------------------------------------------------------------------------

class EncoderInputs{
private:
	uint8_t nBanks;
	uint8_t nEncoders;
	uint8_t nModules;
	

	// setup the port expander
	MCP23S17 *encodersMCP;
	SPIClass *spi;
	const uint8_t encodersMCPChipSelect = 2;
	
	uint16_t *mcpState;
  	uint16_t *mcpStatePrev;

	ytxE41Module module;

	uint16_t **encoderValue;
	uint8_t **encoderState;
	uint16_t **encoderValuePrev;
	int16_t **pulseCounter;
	int16_t *encoderPosition;
	uint32_t *antMillisEncoderUpdate;
	uint8_t *change;        // goes true when a change in the encoder state is detected


public:
	void Init(uint8_t,uint8_t, SPIClass*);
	void Read();
};


#endif