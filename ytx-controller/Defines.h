
//----------------------------------------------------------------------------------------------------
// DEFINES
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
// ENCODERS
//----------------------------------------------------------------------------------------------------

#define N_ENC_MODULES     8
#define N_ENCODERS_X_MOD  4

#define NUM_ENCODERS      N_ENC_MODULES*N_ENCODERS_X_MOD

#define N_RINGS           NUM_ENCODERS
#define N_STEPS_RING      16

#define MAX_ENC_VAL       127       // CC#74 -> VCF's cutoff freq

//----------------------------------------------------------------------------------------------------
// ANALOG
//----------------------------------------------------------------------------------------------------

#define MAX_NUMBER_ANALOG	64        // We'll read the 8 inputs of por A1.

#define ANALOG_INCREASING   0
#define ANALOG_DECREASING   1

#define NOISE_THRESHOLD     2 

//----------------------------------------------------------------------------------------------------
// DIGITAL
//----------------------------------------------------------------------------------------------------

#define N_DIGITAL_MODULES         1
#define N_DIGITAL_INPUTS_X_MOD    4

#define NUM_DIGITAL_INPUTS        N_DIGITAL_MODULES*N_DIGITAL_INPUTS_X_MOD


//----------------------------------------------------------------------------------------------------
// FEEDBACK
//----------------------------------------------------------------------------------------------------

// Which pin on the Arduino is connected to the NeoPixels?
// On a Trinket or Gemma we suggest changing this to 1
#define STATUS_LED_PIN          PIN_LED_TXL
#define BUTTON_LED_PIN1         11
#define BUTTON_LED_PIN2         5

// How many NeoPixels are attached to the Arduino?
#define NUMPIXELS           1
#define NUM_BUT_PIXELS      NUM_DIGITAL_INPUTS

#define NUM_STATUS_LED      0

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
// FEEDBACK
//----------------------------------------------------------------------------------------------------

#define STATUS_CONFIG_BLINK_INTERVAL 100
#define STATUS_MIDI_BLINK_INTERVAL 15


//----------------------------------------------------------------------------------------------------
// MACROS
//----------------------------------------------------------------------------------------------------
#define SET_BIT64(port, bit)   ((port) |= (uint64_t)(1 << (bit)))
#define CLEAR_BIT64(port, bit) ((port) &= (uint64_t)~(1 << (bit)))
#define IS_BIT_SET64(port, bit) (((port) & (uint64_t)(1 << (bit))) ? 1 : 0)
#define IS_BIT_CLEAR64(port, bit) (((port) & (uint64_t)(1 << (bit))) == 0 ? 1 : 0)
