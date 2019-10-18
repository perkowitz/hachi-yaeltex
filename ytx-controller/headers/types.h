#ifndef TYPES_H
#define TYPES_H
#include <stdint.h>

#define SIGNATURE_CHAR  0xA5
#define SIGNATURE_ADDR  65535

#define COMMENT_LEN 8

// COMMS TYPES

enum midiPortsType{
    midi_none,
    midi_usb,
    midi_hw,
    midi_hw_usb
};

enum statusLEDtypes
{
    STATUS_FB_NONE,
    STATUS_FB_CONFIG,
    STATUS_FB_INPUT_CHANGED,
    STATUS_FB_ERROR,
    STATUS_LAST
};
enum statusLEDstates
{
    STATUS_OFF,
    STATUS_BLINK,
    STATUS_ON
};

// GENERAL CONFIG DATA

typedef struct __attribute__((packed))
{
    struct{
        uint8_t fwVersion;
        uint8_t hwVersion;
        uint8_t signature;
        uint8_t pid;
    }board;
    
    struct{
        uint8_t encoder[8];
        uint8_t analog[4][8];
        uint8_t digital[2][8];
        uint8_t feedback[8];
    }hwMapping;
    uint8_t midiMergeFlags : 4;
    uint8_t unused : 4;
    struct{
        uint8_t encoderCount;
        uint8_t analogCount;
        uint16_t digitalCount;
        uint8_t feedbackCount;
    }inputs;
    
    struct{
        uint8_t count;
        uint16_t shifterId[16];
        uint16_t momToggFlags;      // era uint32_t (franco)
    }banks;
    
}ytxConfigurationType;


// FEEDBACK TYPES


enum feedbackSource
{
    fb_src_local,
    fb_src_usb,
    fb_src_midi,
    fb_src_midi_usb
};


// FEEDBACK DATA

// CHEQUEAR CONTRA MAPA DE MEMORIA Y ARI
typedef struct __attribute__((packed))
{
     uint8_t type : 2;
     uint8_t source : 2;
     uint8_t message :4;
     uint8_t channel : 4;
     uint8_t localBehaviour : 4;
     uint8_t colorRangeEnable : 1;
     uint8_t parameterLSB : 7;
     uint8_t unused : 1;
     uint8_t parameterMSB : 7;
     uint8_t color[3];
     uint8_t colorRange0 : 4;
     uint8_t colorRange1 : 4;
     uint8_t colorRange2 : 4;
     uint8_t colorRange3 : 4;
     uint8_t colorRange4 : 4;
     uint8_t colorRange5 : 4;
     uint8_t colorRange6 : 4;
     uint8_t colorRange7 : 4;
}ytxFeedbackType;


// ENCODER TYPES

enum encoderRotaryFeedbackMode{
    fb_walk,
    fb_fill,
    fb_eq,
    fb_spread
};

enum rotaryMessageTypes{
    rotary_msg_none,
    rotary_msg_note,
    rotary_msg_cc,
    rotary_msg_pc_rel,
    rotary_msg_nrpn,
    rotary_msg_rpn,
    rotary_msg_pb,
    rotary_msg_key,
    rotary_msg_msg_size
};



enum encoderRotarySpeed{
    rot_variable_speed,
    rot_slow_speed,
    rot_mid_speed,
    rot_fast_speed
};

enum rotaryConfigKeyboardParameters
{
    rotary_keyLeft,
    rotary_modifierLeft,
    rotary_keyRight,
    rotary_modifierRight
};

enum rotaryConfigMIDIParameters
{
    rotary_LSB,
    rotary_MSB,
    rotary_minLSB,
    rotary_minMSB,
    rotary_maxLSB,
    rotary_maxMSB
};

enum switchConfigKeyboardParameters
{
    switch_key,
    switch_modifier,
};

enum switchConfigMIDIParameters
{
    switch_parameter_LSB,
    switch_parameter_MSB,
    switch_minValue_LSB,
    switch_minValue_MSB,
    switch_maxValue_LSB,
    switch_maxValue_MSB
};
enum switchModes
{
    switch_mode_none,
    switch_mode_midi_message,
    switch_mode_shift_rot,
    switch_mode_fine,
    switch_mode_2cc,
    switch_mode_quick_shift,
    switch_mode_quick_shift_note,
};
enum switchActions
{
    switch_momentary,
    switch_toggle
};

enum switchMessageTypes{
    switch_msg_none,
    switch_msg_note,
    switch_msg_cc,
    switch_msg_pc,
    switch_msg_pc_m,
    switch_msg_pc_p,
    switch_msg_nrpn,
    switch_msg_rpn,
    switch_msg_pb,
    switch_msg_key
};

// ENCODER DATA

typedef struct __attribute__((packed))
{
    struct{
        uint8_t hwMod : 1;
        uint8_t speed : 2;
        uint8_t unused : 5;
    }mode;
    struct{
        uint8_t message : 4;
        uint8_t channel : 4;
        uint8_t midiPort : 2;
        uint8_t unused : 6;
        uint8_t parameter[6];
        char comment[COMMENT_LEN+1];
    }rotaryConfig;
    struct{
        uint8_t action : 1;
        uint8_t doubleClick : 3;
        uint8_t mode : 4;
        uint8_t message : 4;
        uint8_t channel : 4;
        uint8_t midiPort : 2;
        uint8_t parameter[6];
    }switchConfig;
    struct{
        uint8_t mode : 2;
        uint8_t source : 2;
        uint8_t channel : 4;
        uint8_t message : 4;
        uint8_t unused : 4;
        uint8_t parameterLSB : 7;
        uint8_t parameterMSB : 7;
        uint8_t color[3];
    }rotaryFeedback;
    ytxFeedbackType switchFeedback;
}ytxEncoderType;

////////////////////////////////////////////////////////////////////
// DIGITAL TYPES 

enum digitalMessageTypes{
    digital_msg_none,
    digital_msg_note,
    digital_msg_cc,
    digital_msg_pc,
    digital_msg_pc_m,
    digital_msg_pc_p,
    digital_msg_nrpn,
    digital_msg_rpn,
    digital_msg_pb,
    digital_msg_key
};

enum digitalConfigParameters
{
    digital_LSB,
    digital_MSB,
    digital_minLSB,
    digital_minMSB,
    digital_maxLSB,
    digital_maxMSB
};

enum digitalConfigKeyboardParameters
{
    digital_key,
    digital_modifier,
};

// DIGITAL DATA

typedef struct __attribute__((packed))
{
    struct{
        uint8_t action : 1;
        uint8_t unused : 1;
        uint8_t midiPort : 2;
        uint8_t message : 4;
        uint8_t channel : 4;
        uint8_t parameter[6];
        char comment[COMMENT_LEN+1];
    }actionConfig;
    ytxFeedbackType feedback;

}ytxDigitaltype;

////////////////////////////////////////////////////////////////////
// ANALOG TYPES

enum analogMessageTypes{
    analog_msg_none,
    analog_msg_note,
    analog_msg_cc,
    analog_msg_pc,
    analog_msg_pc_m,
    analog_msg_pc_p,
    analog_msg_nrpn,
    analog_msg_rpn,
    analog_msg_pb,
    analog_msg_ks
};

enum analogConfigKeyboardParameters
{
    analog_key,
    analog_modifier
};

enum analogConfigMIDIParameters
{
    analog_LSB,
    analog_MSB,
    analog_minLSB,
    analog_minMSB,
    analog_maxLSB,
    analog_maxMSB
};

// ANALOG DATA
typedef struct __attribute__((packed))
{
    uint8_t type : 4;
    uint8_t midiPort : 2;
    uint8_t joystickMode : 2;
    uint8_t message :4;
    uint8_t channel : 4;
    uint8_t parameter[6];
    char comment[COMMENT_LEN+1];
    ytxFeedbackType feedback;
}ytxAnalogType;



////////////////////////////////////////////////////////////////////
// MEM HOST DECLARATION

enum ytxIOBLOCK
{
  Configuration,
  Encoder,
  Analog,
  Digital,
  Feedback,
  BLOCKS_COUNT
};

typedef struct __attribute__((packed))
{
  uint8_t sectionSize;      // Size in bytes
  uint8_t sectionCount;     // How many sections
  uint16_t eepBaseAddress;  // Start address in EEPROM for this block
  void * ramBaseAddress;    // Start address in RAM for this block
  bool unique;  
}blockDescriptor;

class memoryHost
{
  public:
    memoryHost(extEEPROM *,uint8_t blocks);
    
    void ConfigureBlock(uint8_t,uint8_t,uint8_t,bool);
    void LayoutBanks();
    uint8_t LoadBank(uint8_t);
    void SaveBank(uint8_t);

    int8_t GetCurrentBank();

    uint8_t LoadBankSingleSection(uint8_t, uint8_t, uint8_t);
    
    void ReadFromEEPROM(uint8_t,uint8_t,uint8_t,void *);
    void WriteToEEPROM(uint8_t,uint8_t,uint8_t,void *);
    
    void* Block(uint8_t);
    void* Address(uint8_t,uint8_t);
    uint8_t SectionSize(uint8_t);
    uint8_t SectionCount(uint8_t);

    void* AllocateRAM(uint16_t);

    
  private:
    uint8_t blocksCount;
    blockDescriptor *descriptors;       

    extEEPROM *eep;
    uint16_t eepIndex;      // Points to the start of the bank area
    
    void *bankChunk;
    uint16_t bankSize;
    int8_t bankNow;
    
};

#endif // TYPES_H
