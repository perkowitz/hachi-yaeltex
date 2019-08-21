#ifndef TYPES_H
#define TYPES_H
#include <stdint.h>

#define SIGNATURE_CHAR  0xA5
#define SIGNATURE_ADDR  65535

#define COMMENT_LEN 8

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
        uint8_t digitalCount;
        uint8_t feedbackCount;
    }inputs;
    
    struct{
        uint8_t count;
        uint16_t shifterId[16];
        uint32_t momToggFlags;
    }banks;
    
}ytxConfigurationType;


enum feedbackSource
{
    fb_src_local,
    fb_src_usb,
    fb_src_midi,
    fb_src_midi_usb
};


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

enum encoderMessageTypes{
    rotary_enc_none,
    rotary_enc_note,
    rotary_enc_cc,
    rotary_enc_pc_rel,
    rotary_enc_nrpn,
    rotary_enc_rpn,
    rotary_enc_pb,
    rotary_enc_key,
    rotary_enc_msg_size
};

enum midiPortsType{
    midi_none,
    midi_usb,
    midi_hw,
    midi_hw_usb
};

enum encoderRotaryFeedbackMode{
    fb_walk,
    fb_fill,
    fb_eq,
    fb_spread
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
    switch_keyLeft,
    switch_modifierLeft,
    switch_keyRight,
    switch_modifierRight
};

enum switchConfigMIDIParameters
{
    switch_parameterLow,
    switch_parameterHigh_note,
    switch_minValueLow,
    switch_minValueHigh
};
enum switchActions
{
    switch_momentary,
    switch_toggle
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
        uint8_t maxValueLow  : 7;
        uint8_t maxValueHigh : 7;
        uint8_t parameter[4];
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

enum digitalMessageTypes{
    digital_none,
    digital_note,
    digital_cc,
    digital_pc,
    digital_pc_m,
    digital_pc_p,
    digital_nrpn,
    digital_rpn,
    digital_pb,
    digital_ks
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

typedef struct __attribute__((packed))
{
    struct{
        uint8_t action : 1;
        uint8_t unused : 1;
        uint8_t midiPort : 2;
        uint8_t message :4;
        uint8_t channel : 4;
        uint8_t parameter[6];
        char comment[COMMENT_LEN+1];
    }actionConfig;
    ytxFeedbackType feedback;

}ytxDigitaltype;


enum analogMessageTypes{
    analog_none,
    analog_note,
    analog_cc,
    analog_pc,
    analog_pc_m,
    analog_pc_p,
    analog_nrpn,
    analog_rpn,
    analog_pb,
    analog_ks
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

enum ytxIOBLOCK
{
  Configuration,
  Button,
  Encoder,
  Analog,
  Feedback,
  BLOCKS_COUNT
};

typedef struct __attribute__((packed))
{
  uint8_t sectionSize;
  uint8_t sectionCount;
  uint16_t eepBaseAddress;
  void * ramBaseAddress;
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
    uint16_t eepIndex;
    
    void *bankChunk;
    uint16_t bankSize;
    
};

#endif // TYPES_H
