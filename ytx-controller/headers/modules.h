#ifndef MODULES_H
#define MODULES_H
#include <stdint.h>

enum FeebackTypes{
	FB_ENCODER,
	FB_ENCODER_SWITCH,
	FB_DIGITAL,
	FB_INDEPENDENT
};

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
} ytxModule;  // Add name for typedef

enum e41orientation{
	HORIZONTAL,
	VERTICAL
};

// E41 definition
typedef struct{
    ytxModule components;
    uint8_t nextAddressPin[3];
    uint8_t encPins[4][2];
    uint8_t encSwitchPins[4]; 
    uint8_t orientation;
} ytxE41Module;

ytxE41Module defE41module = {
	    .components = {4, 0, 0, 16},
	    .nextAddressPin = {6, 7, 8},
	    .encPins = {
	      {1, 0},  
	      {4, 3},   
	      {14, 15},  
	      {11, 12}   
	    },
	    .encSwitchPins = { 2, 5, 13, 10 },
	    .orientation = HORIZONTAL,
	}; 

// // CHANGE TO SAME AS E41 - INITIALIZE IN CLASS
// typedef struct{
// 	// encoder pin connections to MCP23S17
// 	uint8_t buttonPins[RB41_components.nDigital] = { 0, 1, 2, 3 };
// }ytxRB41Module;

// typedef struct{
// 	// encoder pin connections to MCP23S17
// 	uint8_t buttonPins[RB42_components.nDigital] = { 0, 1, 2, 3, 4, 5, 9, 10 };
// }ytxRB42Module;


#endif