#ifndef ENCODER_INPUTS_H
#define ENCODER_INPUTS_H

#include <SPI.h>
#include "MCP23S17.h"  // Majenko library
#include "modules.h"
#include "FeedbackClass.h"

//----------------------------------------------------------------------------------------------------
// CLASS DEFINITION
//----------------------------------------------------------------------------------------------------

#define ENCODER_MASK_H			0x3FFE
#define ENCODER_MASK_V			0xC7FF
#define ENCODER_SWITCH_H_ON		0xC001
#define ENCODER_SWITCH_V_ON		0x1C00
#define BOUNCE_MILLIS			40 

// Values returned by 'process'
// No complete step yet.
#define DIR_NONE 0x0
// Clockwise step.
#define DIR_CW 0x10
// Anti-clockwise step.
#define DIR_CCW 0x20

// Use the half-step state table (emits a code at 00 and 11)
#define RFS_START          0x00     //!< Rotary full step start
#define RFS_CW_FINAL       0x01     //!< Rotary full step clock wise final
#define RFS_CW_BEGIN       0x02     //!< Rotary full step clock begin
#define RFS_CW_NEXT        0x03     //!< Rotary full step clock next
#define RFS_CCW_BEGIN      0x04     //!< Rotary full step counter clockwise begin
#define RFS_CCW_FINAL      0x05     //!< Rotary full step counter clockwise final
#define RFS_CCW_NEXT       0x06     //!< Rotary full step counter clockwise next

static const PROGMEM uint8_t fullStepTable[7][4] = {
    // RFS_START
    {RFS_START,             RFS_CCW_BEGIN | DIR_CCW, RFS_CW_BEGIN | DIR_CW,  RFS_START},   //  0 -1  1  0
    // RFS_CW_FINAL
    {RFS_START | DIR_CW,    RFS_CW_FINAL,  RFS_START,     RFS_CW_NEXT | DIR_CCW},          //  1  0  0 -1 
    // RFS_CW_BEGIN
    {RFS_START | DIR_CCW,   RFS_START,     RFS_CW_BEGIN,  RFS_CW_NEXT | DIR_CW},           // -1  0  0  1
    // RFS_CW_NEXT
    {RFS_START,             RFS_CW_FINAL | DIR_CW,  RFS_CW_BEGIN | DIR_CCW,  RFS_CW_NEXT}, //  0  1 -1  0
    // RFS_CCW_BEGIN
    {RFS_START | DIR_CW,    RFS_CCW_BEGIN, RFS_START,     RFS_CCW_NEXT | DIR_CCW},         //  1  0  0 -1
    // RFS_CCW_FINAL
    {RFS_START | DIR_CCW,   RFS_START,     RFS_CCW_FINAL, RFS_CCW_NEXT | DIR_CW},          // -1  0  0  1
    // RFS_CCW_NEXT
    {RFS_START,             RFS_CCW_BEGIN | DIR_CW, RFS_CCW_FINAL | DIR_CCW, RFS_CCW_NEXT},//  0  1 -1  0
};

 /*
 *   Position   Bit1   Bit2
 *   ----------------------
 *     Step1     0      0
 *      1/4      1      0
 *      1/2      1      1
 *      3/4      0      1
 *     Step2     0      0
 */

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
	moduleData* encMData;

	typedef struct{
		int16_t encoderValue;		// Encoder value 0-127 or 0-16383
		int16_t encoderValuePrev;	// Previous encoder value
		uint16_t pulseCounter;		// Amount of encoder state changes
		bool switchInputState;		// Logic state of the input (could match the HW state, or not)
	}encoderBankData;
	encoderBankData** eBankData;

	typedef struct {
		int8_t encoderPosition;		// +1 or -1
		int8_t encoderPositionPrev<;		// +1 or -1
		uint8_t encoderState;			// Logic state of encoder inputs
		uint32_t millisUpdatePrev;		// Millis of last encoder check
		uint8_t encoderChange;        	// Goes true when a change in the encoder state is detected

		uint8_t a, a0, b, b0, c0, d0;

		uint8_t switchHWState;			// Logic state of the button
		uint8_t switchHWStatePrev;		// Previous logic state of the button
		
		uint32_t swBounceMillisPrev;	// Last debounce check

		// Running average filter variables
	    uint8_t filterIndex;            // Indice que recorre los valores del filtro de suavizado
	    uint8_t filterCount;
	    uint16_t filterSum;
	    uint16_t filterSamples[FILTER_SIZE_ENCODER];
	    
	}encoderData;
	encoderData* eData;

	void SetNextAddress(MCP23S17*, byte);
	void SwitchCheck(byte, byte);
	void EncoderCheck(byte, byte);
	void AddToPriority(byte);
	void SetFeedback(uint8_t, uint8_t, uint8_t, uint8_t);
	void FilterClear(uint8_t);
  	int16_t FilterGetNewAverage(uint8_t, uint16_t);

public:
	void Init(uint8_t,uint8_t, SPIClass*);
	void Read();
	uint16_t GetEncoderValue(uint8_t);
	bool GetEncoderSwitchValue(uint8_t);
	uint8_t GetModuleOrientation(uint8_t);

};


#endif
