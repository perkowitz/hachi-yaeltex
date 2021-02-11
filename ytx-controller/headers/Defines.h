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

// #define WAIT_FOR_SERIAL
// #define INIT_CONFIG
// #define PRINT_CONFIG
// #define PRINT_MIDI_BUFFER
// #define PRINT_EEPROM
// #define DEBUG_SYSEX
// #define START_ERASE_EEPROM

#if !defined(INIT_CONFIG)
#define USE_KWHAT_COUNT_BUFFER
#endif

#define FW_VERSION_MAJOR      0
#define FW_VERSION_MINOR      15

#define HW_VERSION_MAJOR      1
#define HW_VERSION_MINOR      0

#define FW_VERSION_ADDR       1
#define HW_VERSION_ADDR       3

#define BOOT_FLAGS_ADDR       5

#define FACTORY_RESET_MASK    (1<<4)

#define MICROCHIP_VID         0x04D8  // Microchip's USB sub-licensing program (https://www.microchip.com/usblicensing)
#define DEFAULT_PID           0xEBCA  // Assigned to Yaeltex

#define MAX_BANKS             8

#define DIGITAL_PORT_1        0
#define DIGITAL_PORT_2        1
#define DIGITAL_PORTS         2
#define MODULES_PER_PORT      8
#define MAX_DIGITAL_MODULES   DIGITAL_PORTS*MODULES_PER_PORT

#define MAX_DIGITAL_AMOUNT    256
#define MAX_ENCODER_AMOUNT    32
#define MAX_ANALOG_AMOUNT     64

// Note On and Note Off values
#define NOTE_ON       127
#define NOTE_OFF      0

// LED States
#define OUT_OFF         0
#define OUT_BLINK       1
#define OUT_ON          2

// On and Off labels
#define ON              1
#define OFF             0

// Pull up mode for digital inputs
#define PULLUP          1

#define KEYBOARD_MILLIS           25
#define KEYBOARD_MILLIS_ANALOG    1
#define PRIORITY_ELAPSE_TIME_MS   500
#define WATCHDOG_RESET_MS         100
#define SAVE_CONTROLLER_STATE_MS  10000

#if defined(SERIAL_DEBUG)
#define SERIALPRINT(a)        { SerialUSB.print(a)     }
#define SERIALPRINTLN(a)      { SerialUSB.println(a)   }
#define SERIALPRINTF(a, f)    { SerialUSB.print(a,f)   }
#define SERIALPRINTLNF(a, f)  { SerialUSB.println(a,f) }
#else
#define SERIALPRINT(a)        {}
#define SERIALPRINTLN(a)      {}
#define SERIALPRINTF(a, f)    {}
#define SERIALPRINTLNF(a, f)  {}
#endif

//----------------------------------------------------------------------------------------------------
// ENCODERS
//----------------------------------------------------------------------------------------------------s

#define IS_ENCODER_ROT_7_BIT(eIndex)   (  encoder[eIndex].rotaryConfig.message == rotary_msg_note   ||  \
                                          encoder[eIndex].rotaryConfig.message == rotary_msg_cc     ||  \
                                          encoder[eIndex].rotaryConfig.message == rotary_msg_vu_cc  ||  \
                                          encoder[eIndex].rotaryConfig.message == rotary_msg_pc_rel  )


#define IS_ENCODER_ROT_FB_7_BIT(eIndex)    (  encoder[eIndex].rotaryFeedback.message == rotary_msg_note   ||  \
                                              encoder[eIndex].rotaryFeedback.message == rotary_msg_cc     ||  \
                                              encoder[eIndex].rotaryFeedback.message == rotary_msg_vu_cc  ||  \
                                              encoder[eIndex].rotaryFeedback.message == rotary_msg_pc_rel )

#define IS_ENCODER_SW_7_BIT(eIndex)    (  encoder[eIndex].switchConfig.mode == switchModes::switch_mode_shift_rot   ?   \
                                         (encoder[eIndex].switchConfig.message == rotary_msg_note   ||    \
                                          encoder[eIndex].switchConfig.message == rotary_msg_cc     ||    \
                                          encoder[eIndex].switchConfig.message == rotary_msg_vu_cc  ||    \
                                          encoder[eIndex].switchConfig.message == rotary_msg_pc_rel)                :   \
                                         (encoder[eIndex].switchConfig.message == switch_msg_note   ||    \
                                          encoder[eIndex].switchConfig.message == switch_msg_cc     ||    \
                                          encoder[eIndex].switchConfig.message == switch_msg_pc     ||    \
                                          encoder[eIndex].switchConfig.message == switch_msg_pc_m   ||    \
                                          encoder[eIndex].switchConfig.message == switch_msg_pc_p ))

#define IS_ENCODER_SW_FB_7_BIT(eIndex)    ( encoder[eIndex].switchConfig.mode == switchModes::switch_mode_shift_rot  ?   \
                                           (encoder[eIndex].switchFeedback.message == rotary_msg_note   ||    \
                                            encoder[eIndex].switchFeedback.message == rotary_msg_cc     ||    \
                                            encoder[eIndex].switchFeedback.message == rotary_msg_vu_cc  ||    \
                                            encoder[eIndex].switchFeedback.message == rotary_msg_pc_rel)              :   \
                                           (encoder[eIndex].switchFeedback.message == switch_msg_note   ||    \
                                            encoder[eIndex].switchFeedback.message == switch_msg_cc     ||    \
                                            encoder[eIndex].switchFeedback.message == switch_msg_pc     ||    \
                                            encoder[eIndex].switchFeedback.message == switch_msg_pc_m   ||    \
                                            encoder[eIndex].switchFeedback.message == switch_msg_pc_p))

#define IS_ENCODER_ROT_14_BIT(eIndex)  (  encoder[eIndex].rotaryConfig.message == rotary_msg_nrpn ||  \
                                          encoder[eIndex].rotaryConfig.message == rotary_msg_rpn  ||  \
                                          encoder[eIndex].rotaryConfig.message == rotary_msg_pb   ||  \
                                          encoder[eIndex].rotaryConfig.message == rotary_msg_key)

#define IS_ENCODER_ROT_FB_14_BIT(eIndex)  ( encoder[eIndex].rotaryFeedback.message == rotary_msg_nrpn  ||  \
                                            encoder[eIndex].rotaryFeedback.message == rotary_msg_rpn   ||  \
                                            encoder[eIndex].rotaryFeedback.message == rotary_msg_pb    ||  \
                                            encoder[eIndex].rotaryFeedback.message == rotary_msg_key)

#define IS_ENCODER_SW_14_BIT(eIndex)   (  encoder[eIndex].switchConfig.mode == switchModes::switch_mode_shift_rot   ?   \
                                         (encoder[eIndex].switchConfig.message == rotary_msg_nrpn   ||  \
                                          encoder[eIndex].switchConfig.message == rotary_msg_rpn    ||  \
                                          encoder[eIndex].switchConfig.message == rotary_msg_pb     ||  \
                                          encoder[eIndex].switchConfig.message == rotary_msg_key)                    :   \
                                         (encoder[eIndex].switchConfig.message == switch_msg_nrpn   ||  \
                                          encoder[eIndex].switchConfig.message == switch_msg_rpn    ||  \
                                          encoder[eIndex].switchConfig.message == switch_msg_pb     ||  \
                                          encoder[eIndex].switchConfig.message == switch_msg_key))

#define IS_ENCODER_SW_FB_14_BIT(eIndex)    (encoder[eIndex].switchConfig.mode == switchModes::switch_mode_shift_rot   ?   \
                                           (encoder[eIndex].switchFeedback.message == rotary_msg_nrpn   ||  \
                                            encoder[eIndex].switchFeedback.message == rotary_msg_rpn    ||  \
                                            encoder[eIndex].switchFeedback.message == rotary_msg_pb)                    :   \
                                           (encoder[eIndex].switchFeedback.message == switch_msg_nrpn   ||  \
                                            encoder[eIndex].switchFeedback.message == switch_msg_rpn    ||  \
                                            encoder[eIndex].switchFeedback.message == switch_msg_pb     ||  \
                                            encoder[eIndex].switchFeedback.message == switch_msg_key))
    
#define MAX_ENCODER_MODS      8

// #define FAST_SPEED_MILLIS  4
// #define MID4_SPEED_MILLIS  6
// #define MID3_SPEED_MILLIS  8
// #define MID2_SPEED_MILLIS  10
// #define MID1_SPEED_MILLIS  16

#define DOUBLE_CLICK_WAIT     200
#define SWITCH_DEBOUNCE_WAIT  20
#define LONG_CLICK_WAIT       1000             // Change to this when implementing long press
// #define LONG_CLICK_WAIT       DOUBLE_CLICK_WAIT   // to prevent long press being ignored

// Millisecond thresholds to calculate non-detent encoder speed
// uint8_t nonDetentMillisSpeedThresholds[] = {8, 12, 15, 20, 25};
#define FAST_SPEED_MILLIS  8
#define MID4_SPEED_MILLIS  12
#define MID3_SPEED_MILLIS  15
#define MID2_SPEED_MILLIS  20
#define MID1_SPEED_MILLIS  25

// Millisecond thresholds to calculate detented encoder speed
// uint8_t detentMillisSpeedThresholds[] = {10, 20, 30, 40, 50};
#define D_FAST_SPEED_MILLIS    10
#define D_MID4_SPEED_MILLIS    20
#define D_MID3_SPEED_MILLIS    30
#define D_MID2_SPEED_MILLIS    40
#define D_MID1_SPEED_MILLIS    50

#define SLOW_SPEED_COUNT      1
#define MID_SPEED_COUNT       1
#define FAST_SPEED_COUNT      1

// Value that each speed adds to current encoder value
//uint8_t encoderAccelSpeed[6] = {1, 2, 3, 4, 6, 8};

#define SLOW_SPEED        1
#define MID1_SPEED        2
#define MID2_SPEED        3
#define MID3_SPEED        4
#define MID4_SPEED        5
#define FAST_SPEED        7


//----------------------------------------------------------------------------------------------------
// ANALOG
//----------------------------------------------------------------------------------------------------

#define IS_ANALOG_7_BIT(aIndex)    (  analog[aIndex].message == analogMessageTypes::analog_msg_note   ||  \
                                      analog[aIndex].message == analogMessageTypes::analog_msg_cc     ||  \
                                      analog[aIndex].message == analogMessageTypes::analog_msg_pc     ||  \
                                      analog[aIndex].message == analogMessageTypes::analog_msg_pc_m   ||  \
                                      analog[aIndex].message == analogMessageTypes::analog_msg_pc_p  )

#define IS_ANALOG_14_BIT(aIndex)   (    analog[aIndex].message == analogMessageTypes::analog_msg_nrpn   ||  \
                                        analog[aIndex].message == analogMessageTypes::analog_msg_rpn    ||  \
                                        analog[aIndex].message == analogMessageTypes::analog_msg_pb     ||  \
                                        analog[aIndex].message == analogMessageTypes::analog_msg_key   )

#define IS_ANALOG_FB_7_BIT(aIndex)    (  analog[aIndex].feedback.message == analogMessageTypes::analog_msg_note   ||  \
                                         analog[aIndex].feedback.message == analogMessageTypes::analog_msg_cc     ||  \
                                         analog[aIndex].feedback.message == analogMessageTypes::analog_msg_pc     ||  \
                                         analog[aIndex].feedback.message == analogMessageTypes::analog_msg_pc_m   ||  \
                                        analog[aIndex].feedback.message == analogMessageTypes::analog_msg_pc_p)

#define IS_ANALOG_FB_14_BIT(aIndex)   (   analog[aIndex].feedback.message == analogMessageTypes::analog_msg_nrpn   ||   \
                                           analog[aIndex].feedback.message == analogMessageTypes::analog_msg_rpn   ||   \
                                           analog[aIndex].feedback.message == analogMessageTypes::analog_msg_pb    ||   \
                                          analog[aIndex].feedback.message == analogMessageTypes::analog_msg_key  )

#define ANALOG_PORTS              4
#define ANALOGS_PER_PORT          16
#define ANALOG_MODULES_PER_PORT   8
#define ANALOG_MODULES_PER_MOD    ANALOGS_PER_PORT/ANALOG_MODULES_PER_PORT
#define MAX_ANALOG_MODULES        ANALOG_PORTS*ANALOG_MODULES_PER_PORT

// set low and high limits to adjust for VCC and GND noise
#define RAW_THRESHOLD       5

#define ANALOG_INCREASING       0
#define ANALOG_DECREASING       1

#define NOISE_THRESHOLD_RAW_14BIT  20   // 20 - works great
#define NOISE_THRESHOLD_RAW_7BIT   15   // 10 - works great

#define FILTER_SIZE_ANALOG   4

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
#define MAX_NUMBER_ANALOG   NUM_MUX*NUM_MUX_CHANNELS       

#define PRESCALER_4     0x000
#define PRESCALER_8     0x100
#define PRESCALER_16    0x200
#define PRESCALER_32    0x300
#define PRESCALER_64    0x400
#define PRESCALER_128   0x500
#define PRESCALER_256   0x600
#define PRESCALER_512   0x700

#define RESOL_12BIT     0x00
#define RESOL_10BIT     0x20
#define RESOL_8BIT      0x30

//----------------------------------------------------------------------------------------------------
// DIGITAL
//----------------------------------------------------------------------------------------------------

#define IS_DIGITAL_7_BIT(dIndex)   (  digital[dIndex].actionConfig.message == digitalMessageTypes::digital_msg_note   ||  \
                                      digital[dIndex].actionConfig.message == digitalMessageTypes::digital_msg_cc     ||  \
                                      digital[dIndex].actionConfig.message == digitalMessageTypes::digital_msg_pc   ||  \
                                      digital[dIndex].actionConfig.message == digitalMessageTypes::digital_msg_pc_m   ||  \
                                      digital[dIndex].actionConfig.message == digitalMessageTypes::digital_msg_pc_p   )

#define IS_DIGITAL_14_BIT(dIndex)  (  digital[dIndex].actionConfig.message == digitalMessageTypes::digital_msg_nrpn   ||  \
                                      digital[dIndex].actionConfig.message == digitalMessageTypes::digital_msg_rpn  ||  \
                                      digital[dIndex].actionConfig.message == digitalMessageTypes::digital_msg_pb   ||  \
                                      digital[dIndex].actionConfig.message == digitalMessageTypes::digital_msg_key )

#define IS_DIGITAL_FB_7_BIT(dIndex)    (  digital[dIndex].feedback.message == digitalMessageTypes::digital_msg_note   ||  \
                                         digital[dIndex].feedback.message == digitalMessageTypes::digital_msg_cc     ||  \
                                         digital[dIndex].feedback.message == digitalMessageTypes::digital_msg_pc    ||  \
                                         digital[dIndex].feedback.message == digitalMessageTypes::digital_msg_pc_m    ||  \
                                        digital[dIndex].feedback.message == digitalMessageTypes::digital_msg_pc_p    )

#define IS_DIGITAL_FB_14_BIT(dIndex)  (  digital[dIndex].feedback.message == digitalMessageTypes::digital_msg_nrpn   ||   \
                                         digital[dIndex].feedback.message == digitalMessageTypes::digital_msg_rpn   ||   \
                                         digital[dIndex].feedback.message == digitalMessageTypes::digital_msg_pb    ||  \
                                        digital[dIndex].feedback.message == digitalMessageTypes::digital_msg_key )

#define N_DIGITAL_MODULES           1
#define N_DIGITAL_INPUTS_X_MOD      4

#define VELOCITY_SESITIVITY_OFF     0xFF

#define NUM_DIGITAL_INPUTS          N_DIGITAL_MODULES*N_DIGITAL_INPUTS_X_MOD

#define DIGITAL_SCAN_INTERVAL       36

//----------------------------------------------------------------------------------------------------
// FEEDBACK
//----------------------------------------------------------------------------------------------------

// STATUS LED
// Which pin on the Arduino is connected to the NeoPixels?
// On a Trinket or Gemma we suggest changing this to 1
#define STATUS_LED_PIN            PIN_LED_TXL

// How many NeoPixels are attached to the Arduino?
#define N_STATUS_PIXEL            1

#define NUM_STATUS_LED            0

#define MAX_WAIT_MORE_DATA_MS     5
#define EXTERNAL_FEEDBACK         true

// ELEMENT FEEDBACK
#define FEEDBACK_UPDATE_BUFFER_SIZE   256 // = 256 dig + (32 rot + 32 enc) switch (analog has no fb yet)

// COMMANDS
#define ACK_CMD                 0xAA
#define NEW_FRAME_BYTE          0xF0
#define BURST_INIT              0xF1
#define BURST_END               0xF2
#define CMD_ALL_LEDS_OFF        0xF3
#define CMD_ALL_LEDS_ON         0xF4
#define INIT_VALUES             0xF5
#define CHANGE_BRIGHTNESS       0xF6
#define END_OF_RAINBOW          0xF7
#define CHECKSUM_ERROR          0xF8
#define SHOW_IN_PROGRESS        0xF9
#define SHOW_END                0xFA
#define CMD_RAINBOW_START       0xFB
#define RESET_HAPPENED          0xFC
#define END_OF_FRAME_BYTE       0xFF


#define ENCODER_CHANGE_FRAME            0x00
#define ENCODER_DOUBLE_FRAME            0x01
#define ENCODER_VUMETER_FRAME           0x02
#define ENCODER_SWITCH_CHANGE_FRAME     0x03
#define DIGITAL1_CHANGE_FRAME           0x04
#define DIGITAL2_CHANGE_FRAME           0x05
#define ANALOG_CHANGE_FRAME             0x06
#define BANK_CHANGE_FRAME               0x07

// BRIGHTNESS
#define BRIGHTNESS_WOP                    25
#define BRIGHTNESS_WOP_32_ENC             18
#define BRIGHTNESS_WITH_POWER             60
#define BANK_OFF_BRIGHTNESS_FACTOR_WP    2/3
#define BANK_OFF_BRIGHTNESS_FACTOR_WOP   3/5

// BLINK INTERVALS
#define STATUS_MIDI_BLINK_INTERVAL            15
#define STATUS_CONFIG_BLINK_INTERVAL          100
#define STATUS_ERROR_BLINK_INTERVAL           200
#define STATUS_CONFIG_ERROR_BLINK_INTERVAL    1000

#define SPOT_SIZE               26
#define S_SPOT_SIZE             14
#define FILL_SIZE               14
#define PIVOT_SIZE              14
#define MIRROR_SIZE             14
        
#define STATUS_LED_BRIGHTNESS   180

#define R_INDEX                 0
#define G_INDEX                 1
#define B_INDEX                 2

#define BANK_INDICATOR_COLOR_R  0xDA
#define BANK_INDICATOR_COLOR_G  0xA5
#define BANK_INDICATOR_COLOR_B  0x20

#define COLOR_RANGE_0           0
#define COLOR_RANGE_1           3
#define COLOR_RANGE_2           7
#define COLOR_RANGE_3           15
#define COLOR_RANGE_4           31
#define COLOR_RANGE_5           63
#define COLOR_RANGE_6           126
#define COLOR_RANGE_7           127

#define NO_SHIFTER        false
#define IS_SHIFTER        true
#define NO_BANK_UPDATE    false
#define BANK_UPDATE       true
#define NO_BLINK          0

#define NO_QSTB_LOAD      false
#define QSTB_LOAD         true

//----------------------------------------------------------------------------------------------------
// COMMS - SERIAL - MIDI
//----------------------------------------------------------------------------------------------------

// SysEx commands

#define MIDI_USB                  0
#define MIDI_HW                   1

#define MIDI_MERGE_FLAGS_USB_USB  0x01
#define MIDI_MERGE_FLAGS_USB_HW   0x02
#define MIDI_MERGE_FLAGS_HW_USB   0x04
#define MIDI_MERGE_FLAGS_HW_HW    0x08

#define MIDI_BUF_MAX_LEN          1000

#define SPI_SPEED                 2000000

#define VUMETER_CHANNEL           15    // CHANNEL 16
#define VALUE_TO_COLOR_CHANNEL    15    // CHANNEL 16
#define BANK_CHANGE_CHANNEL       15    // CHANNEL 16
#define SPLIT_MODE_CHANNEL        15

/*! Enumeration of MIDI types */
enum MidiTypeYTX: uint8_t
{
    InvalidType           = 0x0,    ///< For notifying errors
    NRPN                  = 0x6,    ///< NRPN
    RPN                   = 0x7,    ///< RPN
    NoteOff               = 0x8,    ///< Note Off
    NoteOn                = 0x9,    ///< Note On
    AfterTouchPoly        = 0xA,    ///< Polyphonic AfterTouch
    ControlChange         = 0xB,    ///< Control Change / Channel Mode
    ProgramChange         = 0xC,    ///< Program Change
    AfterTouchChannel     = 0xD,    ///< Channel (monophonic) AfterTouch
    PitchBend             = 0xE,    ///< Pitch Bend
};

#endif