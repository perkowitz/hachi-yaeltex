#ifndef ENCODER_INPUTS_H
#define ENCODER_INPUTS_H

#include <SPI.h>
#include "MCP23S17.h"  // Majenko library
#include "modules.h"
#include "FeedbackClass.h"

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
	uint8_t *moduleOrientation;

	uint8_t priorityCount = 0;				// Amount of modules in priority list
  	uint8_t priorityList[2] = {0};			// Priority list: 1 or 2 modules to be read at a time, when changing
  	unsigned long priorityTime = 0;		// Timer for priority
	
	typedef struct{
		uint16_t mcpState;				// 16 bits - each is the state of one of the MCP digital pins
  		uint16_t mcpStatePrev;			// 16 bits - each is the previous state of one of the MCP digital pins	
  		uint8_t moduleOrientation;
	}moduleData;
	moduleData* mData;

	typedef struct{
		uint16_t encoderValue;		// Encoder value 0-127 or 0-16383
		uint16_t encoderValuePrev;	// Previous encoder value
		uint8_t encoderState;			// Logic state of encoder inputs
		uint16_t pulseCounter;		// Amount of encoder state changes
		bool switchInputState;		// Logic state of the input (could match the HW state, or not)
	}encoderBankData;
	encoderBankData** eBankData;

	typedef struct {
		int16_t encoderPosition;		// +1 or -1
		uint32_t millisUpdatePrev;		// Millis of last encoder check
		uint8_t encoderChange;        	// Goes true when a change in the encoder state is detected

		uint8_t switchHWState;			// Logic state of the button
		uint8_t switchHWStatePrev;		// Previous logic state of the button
		
		uint32_t swBounceMillisPrev;	// Last debounce check
	}encoderData;
	encoderData* eData;

	void SetNextAddress(MCP23S17, byte);
	void SwitchCheck(byte, byte);
	void EncoderCheck(byte, byte);
	void IsInPriority(byte);
	void SetFeedback(uint8_t, uint8_t, uint8_t, uint8_t);

public:
	void Init(uint8_t,uint8_t, SPIClass*);
	void Read();
	uint16_t GetEncoderValue(uint8_t);
	bool GetEncoderSwitchValue(uint8_t);
	uint8_t GetModuleOrientation(uint8_t);

};


#endif
