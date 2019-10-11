#ifndef FEEDBACK_CLASS_H
#define FEEDBACK_CLASS_H

#include "modules.h"
#include "Defines.h"
#include <Adafruit_NeoPixel.h>

const uint8_t PROGMEM gamma8[] = {
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  1,  1,  1,
    1,  1,  1,  1,  1,  1,  1,  1,  1,  2,  2,  2,  2,  2,  2,  2,
    2,  3,  3,  3,  3,  3,  3,  3,  4,  4,  4,  4,  4,  5,  5,  5,
    5,  6,  6,  6,  6,  7,  7,  7,  7,  8,  8,  8,  9,  9,  9, 10,
   10, 10, 11, 11, 11, 12, 12, 13, 13, 13, 14, 14, 15, 15, 16, 16,
   17, 17, 18, 18, 19, 19, 20, 20, 21, 21, 22, 22, 23, 24, 24, 25,
   25, 26, 27, 27, 28, 29, 29, 30, 31, 32, 32, 33, 34, 35, 35, 36,
   37, 38, 39, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 50,
   51, 52, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 66, 67, 68,
   69, 70, 72, 73, 74, 75, 77, 78, 79, 81, 82, 83, 85, 86, 87, 89,
   90, 92, 93, 95, 96, 98, 99,101,102,104,105,107,109,110,112,114,
  115,117,119,120,122,124,126,127,129,131,133,135,137,138,140,142,
  144,146,148,150,152,154,156,158,160,162,164,167,169,171,173,175,
  177,180,182,184,186,189,191,193,196,198,200,203,205,208,210,213,
  215,218,220,223,225,228,231,233,236,239,241,244,247,249,252,255 };

const uint8_t PROGMEM colorRangeTable[16][3] = {
	{0,0,0},
	{120,0,0},
	{180,0,0},
	{240,0,0},
	{0,120,0},
	{0,180,0},
	{0,240,0},
	{0,0,120},
	{0,0,180},
	{0,0,240},
	{120,0,120},
	{0,120,120},
	{120,120,0},
	{193,80,52},
	{80,193,52},
	{200,200,200}
};
// SERIAL FRAME FOR UPDATING LEDs
typedef enum SerialBytes {
  msgLength = 0, frameType, nRing, ringStateH, ringStateL, R, G, B, checkSum_MSB, checkSum_LSB, CRC, ENDOFFRAME
};

#define TX_BYTES 	ENDOFFRAME+1

class FeedbackClass{

private:
	void AddCheckSum();
	void SendFeedbackData();
	void SendDataIfChanged();
	void FillFrameWithEncoderData();
	void FillFrameWithDigitalData();

	uint8_t nBanks;
	uint8_t nEncoders;
	uint8_t nDigital;
	uint8_t nIndependent;

	uint8_t feedbackUpdateFlag;
	bool feedbackDataToSend;
	bool updatingBankFeedback;

	uint8_t indexChanged;
	uint8_t newValue;
	uint8_t orientation;
	uint8_t currentBrightness;

	uint8_t sendSerialBuffer[TX_BYTES] = {};
 
	typedef struct{
		uint16_t encRingState;  //The LED output is based on a scaled veryson of the rotary encoder counter
		uint16_t encRingStatePrev;  //The LED output is based on a scaled veryson of the rotary encoder counter
		uint8_t ringStateIndex;
		uint8_t colorIndexPrev;
	}encFeedbackData;
	encFeedbackData** encFbData;
	
	uint16_t **digitalFbState;

		                            

	/*This is a 2 dimensional array with 3 LED sequences.  The outer array is the sequence;
	  the inner arrays are the values to output at each step of each sequence.  The output values
	  are 16 bit hex values (hex math is actually easier here!).  An LED will be on if its
	  corresponding binary bit is a one, for example: 0x7 = 0000000000000111 and the first 3 LEDs
	  will be on.

	  The data type must be 'unsigned int' if the sequence uses the bottom LED since it's value is 0x8000 (out of range for signed int).
	*/

	const unsigned int walk[2][WALK_SIZE] = {{0x0002, 0x0006, 0x0004, 0x000C, 0x0008, 0x0018, 
		                                      0x0010, 0x0030, 0x0020, 0x0060, 0x0040, 0x00C0, 0x0080, 
		                                      0x0180, 0x0100, 0x0300, 0x0200, 0x0600, 0x0400, 0x0C00, 
		                                      0x0800, 0x1800, 0x1000, 0x3000, 0x2000},
		                                     {0x2000, 0x6000, 0x4000, 0xC000, 0x8000, 0x8001,
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
	void Init(uint8_t, uint8_t, uint8_t, uint8_t);
	void InitPower();
	void Update();
	void SetChangeEncoderFeedback(uint8_t, uint8_t, uint16_t, uint8_t);
	void SetChangeDigitalFeedback(uint16_t, uint16_t);
	void SetChangeIndependentFeedback(uint8_t, uint16_t, uint16_t);
	void SetBankChangeFeedback();
	void SendCommand(uint8_t);
	void SendResetToBootloader();
	void *GetEncoderFBPtr();
	//static void ChangeBrigthnessISR();

};

#endif
