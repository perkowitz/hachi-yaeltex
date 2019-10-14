#ifndef DEFINES_H
#define DEFINES_H

//----------------------------------------------------------------------------------------------------
// DEFINES
//----------------------------------------------------------------------------------------------------

#define DIGITAL_PORTS		2
#define MODULES_PER_PORT	8
#define MAX_DIGITAL_MODULES	DIGITAL_PORTS*MODULES_PER_PORT

// Note On and Note Off values
#define NOTE_ON   127
#define NOTE_OFF  0

// LED States
#define OUT_OFF 	0
#define OUT_BLINK 	1
#define OUT_ON 		2

// On and Off labels
#define ON  1
#define OFF 0

// Pull up mode for digital inputs
#define PULLUP      1

#define KEYBOARD_MILLIS	100

//----------------------------------------------------------------------------------------------------
// ENCODERS
//----------------------------------------------------------------------------------------------------s

#define MAX_ENCODER_MODS	8
#define MAX_ENCODER_MODS	8

#define FAST_SPEED_MILLIS	4
#define MID4_SPEED_MILLIS	6
#define MID3_SPEED_MILLIS	8
#define MID2_SPEED_MILLIS	10
#define MID1_SPEED_MILLIS	16

#define SLOW_SPEED_COUNT	4
#define MID_SPEED_COUNT		2
#define FAST_SPEED_COUNT	1

#define SLOW_SPEED			1
#define MID1_SPEED			2
#define MID2_SPEED			3
#define MID3_SPEED			4
#define MID4_SPEED			5
#define FAST_SPEED			6

#define FILTER_SIZE_ENCODER	4


//----------------------------------------------------------------------------------------------------
// ANALOG
//----------------------------------------------------------------------------------------------------

#define ANALOG_PORTS				4
#define ANALOGS_PER_PORT			16
#define ANALOG_MODULES_PER_PORT		8
#define ANALOG_MODULES_PER_MOD		ANALOGS_PER_PORT/ANALOG_MODULES_PER_PORT
#define MAX_ANALOG_MODULES			ANALOG_PORTS*ANALOG_MODULES_PER_PORT


#define ANALOG_INCREASING   0
#define ANALOG_DECREASING   1

#define NOISE_THRESHOLD_RAW  20 
#define FILTER_SIZE_ANALOG	 10 

#define MUX_A                0            // Mux A identifier
#define MUX_B                1            // Mux B identifier
#define MUX_C                2            // Mux C identifier
#define MUX_D                3            // Mux D identifier

#define MUX_A_PIN            A4           // Mux A pin
#define MUX_B_PIN            A3           // Mux B pin
#define MUX_C_PIN            A1           // Mux C pin
#define MUX_D_PIN            A2           // Mux D pin

#define NUM_MUX              4            // Number of multiplexers to address
#define NUM_MUX_CHANNELS     16           // Number of multiplexing channels
#define MAX_NUMBER_ANALOG	 NUM_MUX*NUM_MUX_CHANNELS       

#define PRESCALER_4 	0x000
#define PRESCALER_8 	0x100
#define PRESCALER_16 	0x200
#define PRESCALER_32 	0x300
#define PRESCALER_64 	0x400
#define PRESCALER_128 	0x500
#define PRESCALER_256 	0x600
#define PRESCALER_512 	0x700

#define RESOL_12BIT		0x00
#define RESOL_10BIT		0x20
#define RESOL_8BIT		0x30

//----------------------------------------------------------------------------------------------------
// DIGITAL
//----------------------------------------------------------------------------------------------------

#define N_DIGITAL_MODULES         1
#define N_DIGITAL_INPUTS_X_MOD    4

#define NUM_DIGITAL_INPUTS        N_DIGITAL_MODULES*N_DIGITAL_INPUTS_X_MOD

#define DIGITAL_SCAN_INTERVAL	48

//----------------------------------------------------------------------------------------------------
// FEEDBACK
//----------------------------------------------------------------------------------------------------

// STATUS LED
// Which pin on the Arduino is connected to the NeoPixels?
// On a Trinket or Gemma we suggest changing this to 1
#define STATUS_LED_PIN          PIN_LED_TXL
#define BUTTON_LED_PIN1         11
#define BUTTON_LED_PIN2         5

// How many NeoPixels are attached to the Arduino?
#define N_STATUS_PIXEL      1

#define NUM_STATUS_LED      0

// ELEMENT FEEDBACK

// COMMANDS
#define CMD_ALL_LEDS_OFF	0xA5
#define NEW_FRAME_BYTE		0xA6
#define INIT_VALUES			0xA7
#define CHANGE_BRIGHTNESS	0xA8


#define ENCODER_CHANGE_FRAME		0x00
#define ENCODER_SWITCH_CHANGE_FRAME	0x01
#define DIGITAL1_CHANGE_FRAME		0x02
#define DIGITAL2_CHANGE_FRAME		0x03
#define ANALOG_CHANGE_FRAME			0x04
#define BANK_CHANGE_FRAME			0x05

// BRIGHTNESS
#define BRIGHNESS_WO_POWER		30
#define BRIGHNESS_WITH_POWER	90

// BLINK INTERVALS
#define STATUS_MIDI_BLINK_INTERVAL 		15
#define STATUS_CONFIG_BLINK_INTERVAL 	100
#define STATUS_ERROR_BLINK_INTERVAL 	1000

#define WALK_SIZE     25
#define FILL_SIZE     14
#define EQ_SIZE       13
#define SPREAD_SIZE   14
        
#define STATUS_LED_BRIGHTNESS 	30

#define R_INDEX	0
#define G_INDEX	1
#define B_INDEX	2

#define COLOR_RANGE_0 	0
#define COLOR_RANGE_1 	3
#define COLOR_RANGE_2 	7
#define COLOR_RANGE_3 	15
#define COLOR_RANGE_4 	31
#define COLOR_RANGE_5 	63
#define COLOR_RANGE_6 	126
#define COLOR_RANGE_7 	127

//----------------------------------------------------------------------------------------------------
// COMMS - SERIAL - MIDI
//----------------------------------------------------------------------------------------------------

#define MIDI_CHANNEL      1        // Encoder value will be sent with CC#74 over channel 1 over MIDI port or USB

// SysEx commands
#define CONFIG_MODE       1    // PC->hw : Activate monitor mode
#define CONFIG_ACK        2    // HW->pc : Acknowledge the config mode
#define DUMP_TO_HW        3    // PC->hw : Partial EEPROM dump from PC
#define DUMP_OK           4    // HW->pc : Ack from dump properly saved
#define EXIT_CONFIG       5    // HW->pc : Deactivate monitor mode
#define EXIT_CONFIG_ACK   6    // HW->pc : Ack from exit config mode

#define CONFIG_OFF        0
#define CONFIG_ON         1

#define MIDI_USB          0
#define MIDI_HW           1


//----------------------------------------------------------------------------------------------------
// MACROS
//----------------------------------------------------------------------------------------------------
#define SET_BIT64(port, bit)   ((port) |= (uint64_t)(1 << (bit)))
#define CLEAR_BIT64(port, bit) ((port) &= (uint64_t)~(1 << (bit)))
#define IS_BIT_SET64(port, bit) (((port) & (uint64_t)(1 << (bit))) ? 1 : 0)
#define IS_BIT_CLEAR64(port, bit) (((port) & (uint64_t)(1 << (bit))) == 0 ? 1 : 0)

#endif