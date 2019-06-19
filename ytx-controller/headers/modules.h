#ifndef MODULES_H
#define MODULES_H
#include <stdint.h>

enum EncoderModuleTypes{
	ENCODER_NONE,
	E41H,
	E41V
};
enum DigitalModuleTypes{
	DIGITAL_NONE,
	RB41,
	RB42,
	RB82,
	ARC41
};
enum AnalogModuleTypes{
	ANALOG_NONE,
	P41,		// 4 rotary potentiometers
	F41,		// 4 slide potentiometers, 45mm travel
	JAF,		// 2 axis joystick, fixed (return to center)
	JAL			// 2 axis joystick, loose (no return to center)
};
enum FeedbackModuleTypes{
	FB_NONE,	
	A11,		// Feeback to analog module in MUX 1, position 1	
	A12,		// Feeback to analog module in MUX 1, position 2
	A13,		// Feeback to analog module in MUX 1, position 3
	A14,		// Feeback to analog module in MUX 1, position 4
	A21,		// Feeback to analog module in MUX 2, position 1
	A22,		// Feeback to analog module in MUX 2, position 2
	A23,		// Feeback to analog module in MUX 2, position 3
	A24,		// Feeback to analog module in MUX 2, position 4
	A31,		// Feeback to analog module in MUX 3, position 1
	A32,		// Feeback to analog module in MUX 3, position 2
	A33,		// Feeback to analog module in MUX 3, position 3
	A34,		// Feeback to analog module in MUX 3, position 4
	A41,		// Feeback to analog module in MUX 4, position 1
	A42,		// Feeback to analog module in MUX 4, position 2
	A43,		// Feeback to analog module in MUX 4, position 3
	A44,		// Feeback to analog module in MUX 4, position 4
};

typedef struct ytxModule{
	uint8_t nEncoders;
	uint8_t nDigital;
	uint8_t nAnalog;
	uint8_t nLedsPerControl;
};

typedef struct{
	ytxModule components {4, 0, 0, 16};

	// encoder pin connections to MCP23S17
	const int encPins[components.nEncoders][2] = {
	  {1, 0},  
	  {4, 3},   
	  {14, 15},  
	  {11, 12}   
	};
	// buttons on each encoder
	const int encSwitchPins[components.nEncoders] = { 2, 5, 13, 10 };	

}ytxE41Module;

typedef struct{
	ytxModule components {0, 4, 0, 1};

	// encoder pin connections to MCP23S17
	const int buttonPins[components.nDigital] = { 0, 1, 2, 3 };

}ytxRB41Module;

typedef struct{
	ytxModule components {0, 8, 0, 1};

	// encoder pin connections to MCP23S17
	const int buttonPins[components.nDigital] = { 0, 1, 2, 3, 4, 5, 9, 10 };

}ytxRB42Module;

// ytxModule E41 = {4, 0, 0, 16};
// ytxModule F41 = {0, 0, 4, 0}; 
// ytxModule P41 = {0, 0, 4, 0};
// ytxModule ARC41 = {0, 4, 0, 4};
// ytxModule RB41 = {0, 4, 0, 4};
// ytxModule RB42 = {0, 8, 0, 8};
// ytxModule RB82 = {0, 16, 0, 16};

#endif