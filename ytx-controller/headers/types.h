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

#ifndef TYPES_H
#define TYPES_H
#include <stdint.h>

#define SIGNATURE_CHAR  0xCF
#define SIGNATURE_ADDR  0

#define COMMENT_LEN     8
#define DEVICE_LEN      15
#define SERIAL_NUM_LEN  9

#define BOOT_SIGN_ADDR  3

// COMMS TYPES

enum midiPortsType{
    midi_none,
    midi_usb,
    midi_hw,
    midi_hw_usb
};

enum takeOverTypes{
    takeover_none,
    takeover_pickup,
    takeover_valueScaling
};

// GENERAL CONFIG DATA

typedef struct __attribute__((packed))
{
    struct{
        uint8_t signature;
        uint8_t fwVersion;
        uint8_t hwVersion;
        uint8_t bootFlag:1;     // BIT 0: BOOT FLAG
        uint8_t takeoverMode:2; // BIT 0: BOOT FLAG
        uint8_t rainbowOn:1;
        uint8_t unusedFlags:4;     // BIT 0: BOOT FLAG
        uint16_t qtyMessages7bit;
        uint16_t qtyMessages14bit;
        uint16_t pid;
        char serialNumber[SERIAL_NUM_LEN+1];
        char deviceName[DEVICE_LEN+1];
    }board;
    
    struct{
        uint8_t encoder[8];
        uint8_t analog[4][8];
        uint8_t digital[2][8];
        uint8_t feedback[8];
    }hwMapping;

    struct{
        uint8_t encoderCount;
        uint8_t analogCount;
        uint16_t digitalCount;
        uint8_t feedbackCount;
        uint8_t unused;
    }inputs;
    
    struct{
        uint8_t count;
        uint8_t momToggFlags;      // era uint32_t (franco)
        uint16_t shifterId[MAX_BANKS];
    }banks;

    struct{
        uint8_t midiMergeFlags : 4;
        uint8_t unused : 4;
    }midiConfig;

}ytxConfigurationType;


// FEEDBACK TYPES


enum feedbackSource
{
    fb_src_local,
    fb_src_usb,
    fb_src_midi_hw,
    fb_src_midi_usb
};

enum feedbackLocalBehaviour
{
    fb_lb_on_with_press,
    fb_lb_always_on,
};

enum encoderRotaryFeedbackMode{
    fb_walk,
    fb_spread,
    fb_fill,
    fb_eq,
};

// FEEDBACK DATA

// CHEQUEAR CONTRA MAPA DE MEMORIA Y ARI
typedef struct __attribute__((packed))
{
     uint8_t type : 2;                  // not used
     uint8_t source : 2;
     uint8_t message : 4;
     uint8_t localBehaviour : 4;
     uint8_t channel : 4;
     uint8_t parameterLSB : 7;
     uint8_t colorRangeEnable : 1;
     uint8_t parameterMSB : 7;
     uint8_t unused : 1;
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

enum rotaryMessageTypes{
    rotary_msg_none,
    rotary_msg_note,
    rotary_msg_cc,
    rotary_msg_vu_cc,
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
    switch_mode_message,
    switch_mode_shift_rot,
    switch_mode_fine,
    switch_mode_2cc,
    switch_mode_quick_shift,
    switch_mode_quick_shift_note,
    switch_mode_velocity
};
enum switchActions
{
    switch_momentary,
    switch_toggle
};
enum switchDoubleClickModes
{
    switch_doubleClick_none,
    switch_doubleClick_2min,
    switch_doubleClick_2center,
    switch_doubleClick_2max
};

enum switchMessageTypes{
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
        uint8_t speed : 2;
        uint8_t hwMode : 1;
        uint8_t unused : 5;             // UNUSED 5 BITS
    }mode;
    struct{
        uint8_t channel : 4;
        uint8_t message : 4;
        uint8_t midiPort : 2;
        uint8_t unused : 6;             // UNUSED 6 BITS
        uint8_t parameter[6];
        char comment[COMMENT_LEN+1];
    }rotaryConfig;
    struct{
        uint8_t mode : 4;
        uint8_t doubleClick : 3;
        uint8_t action : 1;
        uint8_t channel : 4;
        uint8_t message : 4;
        uint8_t midiPort : 2;
        uint8_t unused : 6;             // UNUSED 6 BITS
        uint8_t parameter[6];
    }switchConfig;
    struct{
        uint8_t channel : 4;
        uint8_t source : 2;
        uint8_t mode : 2;
        uint8_t message : 4;
        uint8_t unused : 4;
        uint8_t parameterLSB : 7;       // UNUSED 1 BIT
        uint8_t unused1 : 1;
        uint8_t parameterMSB : 7;       // UNUSED 1 BIT
        uint8_t unused2 : 1;
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
        uint8_t message : 4;
        uint8_t midiPort : 2;
        uint8_t action : 1;
        uint8_t unused : 1;             // UNUSED 1 BIT
        uint8_t channel : 4;            
        uint8_t unused1 : 4;            // UNUSED 4 BITS
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
    analog_msg_key
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
    uint8_t message :4;
    uint8_t midiPort : 2;
    uint8_t joystickMode : 2;
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
  uint16_t sectionCount;     // How many sections
  uint16_t eepBaseAddress;  // Start address in EEPROM for this block
  void * ramBaseAddress;    // Start address in RAM for this block
  bool unique;  
}blockDescriptor;

class memoryHost
{
  public:
    memoryHost(extEEPROM *,uint8_t blocks);
    
    void ConfigureBlock(uint8_t,uint16_t,uint8_t,bool);
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
    uint16_t SectionCount(uint8_t);

    void* AllocateRAM(uint16_t);
    void FreeRAM(void*);

    
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
