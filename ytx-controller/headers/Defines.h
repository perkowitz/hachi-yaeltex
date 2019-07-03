
//----------------------------------------------------------------------------------------------------
// DEFINES
//----------------------------------------------------------------------------------------------------

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

// For analog threshold filtering
#define ANALOG_UP   1      // DO NOT CHANGE
#define ANALOG_DOWN 0      // DO NOT CHANGE
#define NOISE_THR   1      // If you change this, you'll skip more values when a pot or sensor changes direction


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

#define ANALOG_INCREASING   0
#define ANALOG_DECREASING   1

#define NOISE_THRESHOLD     2 

#define MUX_A                0            // Mux A identifier
#define MUX_B                1            // Mux B identifier
#define MUX_C                2            // Mux C identifier
#define MUX_D                3            // Mux D identifier

#define MUX_A_PIN            A1           // Mux A pin
#define MUX_B_PIN            A2           // Mux B pin
#define MUX_C_PIN            A3           // Mux C pin
#define MUX_D_PIN            A4           // Mux D pin

#define MUX_A1_START         0            // Mux A1 header first pin
#define MUX_A1_END           7            // Mux A1 header last pin
#define MUX_A2_START         8            // Mux A2 header first pin
#define MUX_A2_END           15           // Mux A2 header last pin
#define MUX_B1_START         0            // Mux B1 header first pin
#define MUX_B1_END           7            // Mux B1 header last pin
#define MUX_B2_START         8            // Mux B2 header first pin
#define MUX_B2_END           15           // Mux B2 header last pin
#define MUX_B1_START         0            // Mux B1 header first pin
#define MUX_B1_END           7            // Mux B1 header last pin
#define MUX_B2_START         8            // Mux B2 header first pin
#define MUX_B2_END           15           // Mux B2 header last pin

#define NUM_MUX              4            // Number of multiplexers to address
#define NUM_MUX_CHANNELS     16           // Number of multiplexing channels
#define MAX_NUMBER_ANALOG	 NUM_MUX*NUM_MUX_CHANNELS       // We'll read the 8 inputs of por A1.

#define PRESCALER_4 0x000
#define PRESCALER_8 0x100
#define PRESCALER_16 0x200
#define PRESCALER_32 0x300
#define PRESCALER_64 0x400
#define PRESCALER_128 0x500
#define PRESCALER_256 0x600
#define PRESCALER_512 0x700

#define RESOL_12BIT	0x00
#define RESOL_10BIT	0x20
#define RESOL_8BIT	0x30

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
