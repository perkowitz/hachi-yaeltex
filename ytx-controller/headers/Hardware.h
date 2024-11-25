#ifndef HARDWARE_H
#define HARDWARE_H

#include <stdint.h>
#include "Arduino.h"
#include "FeedbackClass.h"

typedef signed long  s32;
typedef signed short s16;
typedef signed char  s8;

typedef unsigned long long u64;
typedef unsigned long  u32;
typedef unsigned short u16;
typedef unsigned char  u8;

typedef enum {GRID, BUTTON, KEY, UNKNOWN} digital_type;

// Numbers of various kinds of digitals.
static const uint8_t GRID_ROWS = 8;
static const uint8_t GRID_COLUMNS = 16;
static const uint8_t GRID_HALF_COUNT = 8;
static const uint8_t BUTTON_ROWS = 6;
static const uint8_t BUTTON_COLUMNS = 8;
static const uint8_t KEY_ROWS = 1;
static const uint8_t KEY_COLUMNS = 12;
static const uint8_t DIGITAL_COUNT = 208;
static const u8 ENCODER_COUNT = 8;

#define DEBUG_NO_LEDS false
#define NO_COLOR 255
#define MIDI_ALL_NOTES_OFF 123

class Hardware {

  public:

    struct HachiDigital {
        digital_type type;
        uint8_t row;
        uint8_t column;
    };

    Hardware();

    void Init();

    bool getHachiEnabled(void);
    bool getControlEnabled(void);
    void setHachiEnabled(bool enabled);
    void setControlEnabled(bool enabled);

    bool clockFromUsb = true;
    bool clockFromDin = true;


    // Convert a Yaeltex digital index to Hachi types, and vice-versa.
    HachiDigital fromDigital(uint16_t dInput);
    uint16_t toDigital(digital_type type, uint16_t row, uint16_t column);
    uint16_t toDigital(HachiDigital digital);

    void setGrid(uint16_t row, uint16_t column, uint16_t color);
    void setButton(uint16_t row, uint16_t column, uint16_t color);
    void setKey(uint16_t column, uint16_t color);
    void setByIndex(uint16_t index, uint16_t color);

    void setEncoderColor(u16 index, u8 color);
    void setEncoderAccentColor(u16 index, u8 color);
    void setEncoderValue(u16 index, u8 value);
    void setEncoder(u16 index, u8 value, u8 color, u8 accentColor);

    void Update();
    void ClearGrid();
    void FillGrid(uint8_t color);
    void DrawPalette();
    void ResetDrawing();

    void SendMidiNote(uint8_t channel, uint8_t note, uint8_t velocity);
    void SendMidiCc(uint8_t channel, uint8_t controller, uint8_t value);
    void SendAllNotesOff(boolean sendIndividualNotes);

    uint16_t currentValue[DIGITAL_COUNT];
    void CurrentValues();

  private:
    // For converting between Yaeltex digitals and Hachi.
    static const uint8_t GRID_START_INDEX = 0;
    static const uint8_t GRID_MID_INDEX = 64;
    static const uint8_t KEY_START_INDEX = 160;
    static const uint8_t BUTTON_ROW0_INDEX = 128;
    static const uint8_t BUTTON_ROW2_INDEX = 144;
    static const uint8_t BUTTON_ROW4_INDEX = 172;


    bool hachiEnabled = true;
    bool controlEnabled = false;

    uint8_t initialized = false;

    bool sendToUsb = true;
    bool sendToDin = true;

};


#endif
