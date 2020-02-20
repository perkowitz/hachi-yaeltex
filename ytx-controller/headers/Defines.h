/*

Author/s: Franco Grassano - Franco Zaccra

MIT License

Copyright (c) 2020 - Yaeltex

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

*/

#ifndef DEFINES_H
#define DEFINES_H

//----------------------------------------------------------------------------------------------------
// DEFINES
//----------------------------------------------------------------------------------------------------

#define FW_VERSION			0x01
#define HW_VERSION			0x01

#define MAX_BANKS			8

#define DIGITAL_PORTS		2
#define MODULES_PER_PORT	8
#define MAX_DIGITAL_MODULES	DIGITAL_PORTS*MODULES_PER_PORT

// Note On and Note Off values
#define NOTE_ON   127
#define NOTE_OFF  0

// LED States
#define OUT_OFF 			0
#define OUT_BLINK 			1
#define OUT_ON 				2

// On and Off labels
#define ON  				1
#define OFF 				0

// Pull up mode for digital inputs
#define PULLUP      		1

#define KEYBOARD_MILLIS				100
#define PRIORITY_ELAPSE_TIME_MS		500

//----------------------------------------------------------------------------------------------------
// ENCODERS
//----------------------------------------------------------------------------------------------------s

#define IS_ENCODER_ROT_FB_7_BIT(eIndex)		(	 encoder[eIndex].rotaryFeedback.message == rotary_msg_note 	||	\
           										               encoder[eIndex].rotaryFeedback.message == rotary_msg_cc 	||	\
                                             encoder[eIndex].rotaryFeedback.message == rotary_msg_pc_rel	)

#define IS_ENCODER_SW_FB_7_BIT(eIndex)		(	encoder[eIndex].switchFeedback.message == switch_msg_note 	||	\
                         										encoder[eIndex].switchFeedback.message == switch_msg_cc 	||	\
                         										encoder[eIndex].switchFeedback.message == switch_msg_pc		||	\
                         										encoder[eIndex].switchFeedback.message == switch_msg_pc_m	||	\
    											                  encoder[eIndex].switchFeedback.message == switch_msg_pc_p	)

#define IS_ENCODER_ROT_FB_14_BIT(eIndex)	(	encoder[eIndex].rotaryFeedback.message == rotary_msg_nrpn 	||	\
                         										encoder[eIndex].rotaryFeedback.message == rotary_msg_rpn 	||	\
                         										encoder[eIndex].rotaryFeedback.message == rotary_msg_pb	)

#define IS_ENCODER_SW_FB_14_BIT(eIndex)		(	encoder[eIndex].switchFeedback.message == switch_msg_nrpn 	||	\
                         										encoder[eIndex].switchFeedback.message == switch_msg_rpn 	||	\
                         										encoder[eIndex].switchFeedback.message == switch_msg_pb	)
    
#define MAX_ENCODER_MODS		8

// #define FAST_SPEED_MILLIS	4
// #define MID4_SPEED_MILLIS	6
// #define MID3_SPEED_MILLIS	8
// #define MID2_SPEED_MILLIS	10
// #define MID1_SPEED_MILLIS	16

// Millisecond thresholds to calculate non-detent encoder speed
#define FAST_SPEED_MILLIS	8
#define MID4_SPEED_MILLIS	12
#define MID3_SPEED_MILLIS	15
#define MID2_SPEED_MILLIS	20
#define MID1_SPEED_MILLIS	25

// Millisecond thresholds to calculate detented encoder speed
#define D_FAST_SPEED_MILLIS		10
#define D_MID4_SPEED_MILLIS		20
#define D_MID3_SPEED_MILLIS		30
#define D_MID2_SPEED_MILLIS		40
#define D_MID1_SPEED_MILLIS		50

#define SLOW_SPEED_COUNT		1
#define MID_SPEED_COUNT			1
#define FAST_SPEED_COUNT		1

// Value that each speed adds to current encoder value
#define SLOW_SPEED				1
#define MID1_SPEED				2
#define MID2_SPEED				3
#define MID3_SPEED				4
#define MID4_SPEED				6
#define FAST_SPEED				8

#define FILTER_SIZE_ENCODER		4


//----------------------------------------------------------------------------------------------------
// ANALOG
//----------------------------------------------------------------------------------------------------

#define IS_ANALOG_FB_7_BIT(aIndex)		(	analog[aIndex].feedback.message == analogMessageTypes::analog_msg_note 	||	\
           									analog[aIndex].feedback.message == analogMessageTypes::analog_msg_cc 	||	\
           									analog[aIndex].feedback.message == analogMessageTypes::analog_msg_pc	||	\
           									analog[aIndex].feedback.message == analogMessageTypes::analog_msg_pc_m	||	\
    										analog[aIndex].feedback.message == analogMessageTypes::analog_msg_pc_p	)

#define IS_ANALOG_FB_14_BIT(aIndex) 	( 	analog[aIndex].feedback.message == analogMessageTypes::analog_msg_nrpn 	|| 	\
									 		analog[aIndex].feedback.message == analogMessageTypes::analog_msg_rpn 	|| 	\
									 		analog[aIndex].feedback.message == analogMessageTypes::analog_msg_pb )

#define ANALOG_PORTS				4
#define ANALOGS_PER_PORT			16
#define ANALOG_MODULES_PER_PORT		8
#define ANALOG_MODULES_PER_MOD		ANALOGS_PER_PORT/ANALOG_MODULES_PER_PORT
#define MAX_ANALOG_MODULES			ANALOG_PORTS*ANALOG_MODULES_PER_PORT


#define ANALOG_INCREASING   		0
#define ANALOG_DECREASING   		1

#define NOISE_THRESHOLD_RAW_14BIT  20   // 20 - works great
#define NOISE_THRESHOLD_RAW_7BIT   15   // 10 - works great

#define FILTER_SIZE_ANALOG	 4

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

#define PRESCALER_4 		0x000
#define PRESCALER_8 		0x100
#define PRESCALER_16 		0x200
#define PRESCALER_32 		0x300
#define PRESCALER_64 		0x400
#define PRESCALER_128 		0x500
#define PRESCALER_256 		0x600
#define PRESCALER_512 		0x700

#define RESOL_12BIT			0x00
#define RESOL_10BIT			0x20
#define RESOL_8BIT			0x30

//----------------------------------------------------------------------------------------------------
// DIGITAL
//----------------------------------------------------------------------------------------------------

#define IS_DIGITAL_FB_7_BIT(dIndex)		(	digital[dIndex].feedback.message == digitalMessageTypes::digital_msg_note 	||	\
           									digital[dIndex].feedback.message == digitalMessageTypes::digital_msg_cc 		||	\
           									digital[dIndex].feedback.message == digitalMessageTypes::digital_msg_pc		||	\
           									digital[dIndex].feedback.message == digitalMessageTypes::digital_msg_pc_m		||	\
    										digital[dIndex].feedback.message == digitalMessageTypes::digital_msg_pc_p		)

#define IS_DIGITAL_FB_14_BIT(dIndex)	(	digital[dIndex].feedback.message == digitalMessageTypes::digital_msg_nrpn 	|| 	\
         									digital[dIndex].feedback.message == digitalMessageTypes::digital_msg_rpn 	|| 	\
         									digital[dIndex].feedback.message == digitalMessageTypes::digital_msg_pb	)
#define N_DIGITAL_MODULES         	1
#define N_DIGITAL_INPUTS_X_MOD    	4

#define NUM_DIGITAL_INPUTS        N_DIGITAL_MODULES*N_DIGITAL_INPUTS_X_MOD

#define DIGITAL_SCAN_INTERVAL		36

//----------------------------------------------------------------------------------------------------
// FEEDBACK
//----------------------------------------------------------------------------------------------------

// STATUS LED
// Which pin on the Arduino is connected to the NeoPixels?
// On a Trinket or Gemma we suggest changing this to 1
#define STATUS_LED_PIN          		PIN_LED_TXL

// How many NeoPixels are attached to the Arduino?
#define N_STATUS_PIXEL      			1

#define NUM_STATUS_LED      			0

// ELEMENT FEEDBACK
#define FEEDBACK_UPDATE_BUFFER_SIZE		256 // = 256 dig + 32 rot + 32 enc switch + 64 ang.

// COMMANDS
#define NEW_FRAME_BYTE					0xF0
#define BANK_INIT						    0xF1
#define BANK_END						    0xF2
#define CMD_ALL_LEDS_OFF				0xF3
#define INIT_VALUES						 0xF4
#define CHANGE_BRIGHTNESS				0xF5
#define END_OF_RAINBOW					0xF6
#define CHECKSUM_ERROR					0xF7
#define SHOW_IN_PROGRESS	0xF8
#define SHOW_END			0xF9
#define END_OF_FRAME_BYTE				0xFF

#define ENCODER_CHANGE_FRAME			     0x00
#define ENCODER_DOUBLE_FRAME      		 0x01
#define ENCODER_SWITCH_CHANGE_FRAME    0x02
#define DIGITAL1_CHANGE_FRAME			     0x03
#define DIGITAL2_CHANGE_FRAME			     0x04
#define ANALOG_CHANGE_FRAME				     0x05
#define BANK_CHANGE_FRAME				       0x06

// BRIGHTNESS
#define BRIGHTNESS_WO_POWER				25
#define BRIGHTNESS_WITH_POWER			60
#define BANK_OFF_BRIGHTNESS_FACTOR		2/3

// BLINK INTERVALS
#define STATUS_MIDI_BLINK_INTERVAL 		15
#define STATUS_CONFIG_BLINK_INTERVAL 	100
#define STATUS_ERROR_BLINK_INTERVAL 	1000

#define WALK_SIZE     					26
#define S_WALK_SIZE             14
#define FILL_SIZE     					14
#define EQ_SIZE       					13
#define SPREAD_SIZE   					14
        
#define STATUS_LED_BRIGHTNESS 			255

#define R_INDEX							0
#define G_INDEX							1
#define B_INDEX							2

#define COLOR_RANGE_0 					0
#define COLOR_RANGE_1 					3
#define COLOR_RANGE_2 					7
#define COLOR_RANGE_3 					15
#define COLOR_RANGE_4 					31
#define COLOR_RANGE_5 					63
#define COLOR_RANGE_6 					126
#define COLOR_RANGE_7 					127

//----------------------------------------------------------------------------------------------------
// COMMS - SERIAL - MIDI
//----------------------------------------------------------------------------------------------------

// SysEx commands

#define MIDI_USB          				0
#define MIDI_HW           				1


#define MIDI_BUF_MAX_LEN    			1000

/*! Enumeration of MIDI types */
enum MidiTypeYTX: uint8_t
{
    InvalidType           = 0x0,    ///< For notifying errors
    NRPN		          = 0x6,    ///< NRPN
    RPN			          = 0x7,    ///< RPN
    NoteOff               = 0x8,    ///< Note Off
    NoteOn                = 0x9,    ///< Note On
    AfterTouchPoly        = 0xA,    ///< Polyphonic AfterTouch
    ControlChange         = 0xB,    ///< Control Change / Channel Mode
    ProgramChange         = 0xC,    ///< Program Change
    AfterTouchChannel     = 0xD,    ///< Channel (monophonic) AfterTouch
    PitchBend             = 0xE,    ///< Pitch Bend
};

//----------------------------------------------------------------------------------------------------
// MACROS
//----------------------------------------------------------------------------------------------------
#define SET_BIT64(port, bit)   ((port) |= (uint64_t)(1 << (bit)))
#define CLEAR_BIT64(port, bit) ((port) &= (uint64_t)~(1 << (bit)))
#define IS_BIT_SET64(port, bit) (((port) & (uint64_t)(1 << (bit))) ? 1 : 0)
#define IS_BIT_CLEAR64(port, bit) (((port) & (uint64_t)(1 << (bit))) == 0 ? 1 : 0)

#endif