#ifndef ENCODER_INPUTS_H
#define ENCODER_INPUTS_H

#include <SPI.h>
#include "MCP23S17.h"  // Majenko library
#include "modules.h"

//----------------------------------------------------------------------------------------------------
// CLASS DEFINITION
//----------------------------------------------------------------------------------------------------

#define ENCODER_MASK_H		0x3FFE
#define ENCODER_MASK_V		0xC7FF
#define ENCODER_SWITCH_H_ON		0xC001
#define ENCODER_SWITCH_V_ON		0x1C00
#define BOUNCE_MILLIS		40 


class EncoderInputs{
private:
	uint8_t nBanks;
	uint8_t nEncoders;
	uint8_t nModules;
	
	// setup the port expander
	MCP23S17 encodersMCP[8];
	SPIClass *spi;
	const uint8_t encodersMCPChipSelect = 2;
	byte *moduleOrientation;

	byte priorityCount = 0;
  	byte priorityList[2] = {};
  	unsigned long priorityTime = 0;
	
	uint16_t *mcpState;
  	uint16_t *mcpStatePrev;

	uint16_t **encoderValue;
	uint8_t **encoderState;
	uint16_t **encoderValuePrev;
	uint16_t **pulseCounter;
	int16_t *encoderPosition;
	uint32_t *millisUpdatePrev;
	uint8_t *encoderChange;        // goes true when a change in the encoder state is detected

	uint8_t **switchState;
	uint8_t **switchStatePrev;
	bool **digitalInputState;
	uint32_t *swBounceMillisPrev;


	void SetNextAddress(MCP23S17, byte);
	void SwitchCheck(byte, byte);
	void EncoderCheck(byte, byte);

public:
	void Init(uint8_t,uint8_t, SPIClass*);
	void Read();
};


#endif