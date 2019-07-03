#ifndef FEEDBACK_H
#define FEEDBACK_H

#include "modules.h"
#include <Adafruit_NeoPixel.h>


#define WALK_SIZE     26
#define FILL_SIZE     14
#define EQ_SIZE       13
#define SPREAD_SIZE   14

class Feedback{

private:

	// STATUS LED
	Adafruit_NeoPixel statusLED = Adafruit_NeoPixel(NUMPIXELS, STATUS_LED_PIN, NEO_GRB + NEO_KHZ800);
	
	bool flagBlinkStatusLED = 0;
	uint8_t blinkCountStatusLED = 0;
	bool outputBlinkState = 0;
	unsigned long millisPrevLED = 0;               // Variable para guardar ms
	uint8_t statusLEDfbType = 0;

	// SERIAL FRAME FOR UPDATING LEDs
	typedef enum SerialBytes {
	  msgLength = 0, nRing, ringStateH, ringStateL, R, G, B, checkSum_MSB, checkSum_LSB, CRC, ENDOFFRAME
	};


	unsigned short int ringStateIndex = 0;
	unsigned int allRingState[N_RINGS] = {0};  //The LED output is based on a scaled veryson of the rotary encoder counter
	unsigned int allRingStatePrev[N_RINGS] = {0};  //The LED output is based on a scaled veryson of the rotary encoder counter

	uint8_t indexRgbList = 0;
	const uint8_t rgbList[4][3] =  {{0, 0, 96},
		                            {32, 0, 64},
		                            {64, 0, 32},
		                            {96, 0, 0}};

	/*This is a 2 dimensional array with 3 LED sequences.  The outer array is the sequence;
	  the inner arrays are the values to output at each step of each sequence.  The output values
	  are 16 bit hex values (hex math is actually easier here!).  An LED will be on if its
	  corresponding binary bit is a one, for example: 0x7 = 0000000000000111 and the first 3 LEDs
	  will be on.

	  The data type must be 'unsigned int' if the sequence uses the bottom LED since it's value is 0x8000 (out of range for signed int).
	*/

	const unsigned int walk[2][WALK_SIZE] = {{0x0000, 0x0002, 0x0006, 0x0004, 0x000C, 0x0008, 0x0018, 
		                                      0x0010, 0x0030, 0x0020, 0x0060, 0x0040, 0x00C0, 0x0080, 
		                                      0x0180, 0x0100, 0x0300, 0x0200, 0x0600, 0x0400, 0x0C00, 
		                                      0x0800, 0x1800, 0x1000, 0x3000, 0x2000},
		                                     {0x0000, 0x2000, 0x6000, 0x4000, 0xC000, 0x8000, 0x8001,
		                                      0x0001, 0x0003, 0x0002, 0x0006, 0x0004, 0x000C, 0x0008,
		                                      0x0018, 0x0010, 0x0030, 0x0020, 0x0060, 0x0040, 0x00C0,
		                                      0x0080, 0x0180, 0x0100, 0x0300, 0x0200}};
	                              
	const unsigned int fill[2][FILL_SIZE] = {{0x0000, 0x0002, 0x0006, 0x000E, 0x001E, 0x003E, 0x007E, 
		                                      0x00FE, 0x01FE, 0x03FE, 0x07FE, 0x0FFE, 0x1FFE, 0x3FFE},
		                                     {0x0000, 0x2000, 0x6000, 0xE000, 0xE001, 0xE003, 0xE007, 
		                                      0xE00F, 0xE01F, 0xE03F, 0xE07F, 0xE0FF, 0xE1FF, 0xE3FF}};
	                              
	const unsigned int eq[2][EQ_SIZE] =     {{0x00FE, 0x00FC, 0x00F8, 0x00F0, 0x00E0, 0x00C0, 0x0080, 
		                                      0x0180, 0x0380, 0x0780, 0x0F80, 0x1F80, 0x3F80},
		                                     {0xE00F, 0xC00F, 0x800F, 0x000F, 0x000E, 0x000C, 0x0008, 
		                                      0x0018, 0x0038, 0x0078, 0x00F8, 0x01F8, 0x03F8}};
	                              
	const unsigned int spread[2][SPREAD_SIZE] =   {{0x0000, 0x0080, 0x00C0, 0x01C0, 0x01E0, 0x03E0, 0x03F0,
			                                        0x07F0, 0x07F8, 0x0FF8, 0x0FFC, 0x1FFC, 0x1FFE, 0x3FFE},
			                                       {0x0000, 0x0008, 0x000C, 0x001C, 0x001E, 0x003E, 0x003F,
			                                        0x007F, 0x807F, 0x80FF, 0xC0FF, 0xC1FF, 0xE1FF, 0xE3FF}};



public:
	void Init();
	void SetStatusLED(uint8_t, bool);
	void UpdateStatusLED();
	void Update(uint8_t type, uint8_t encNo, uint8_t val, uint8_t encoderOrientation);
};

//----------------------------------------------------------------------------------------------------
// FEEDBACK VARIABLES AND OBJECTS
//----------------------------------------------------------------------------------------------------

Feedback feedbackHw;

#endif
