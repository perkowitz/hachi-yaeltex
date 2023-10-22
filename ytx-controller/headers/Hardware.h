#ifndef HARDWARE_H
#define HARDWARE_H

#include <stdint.h>
#include "Arduino.h"
#include "FeedbackClass.h"
#include "IDisplayReceiver.h"

typedef enum {GRID, BUTTON, KEY, UNKNOWN} digital_type;

// Numbers of various kinds of digitals.
static const uint8_t GRID_ROWS = 8;
static const uint8_t GRID_COLUMNS = 16;
static const uint8_t GRID_HALF_COUNT = 8;
static const uint8_t BUTTON_ROWS = 6;
static const uint8_t BUTTON_COLUMNS = 8;
static const uint8_t KEY_ROWS = 1;
static const uint8_t KEY_COLUMNS = 12;


class Hardware: public IDisplayReceiver {

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


    // Convert a Yaeltex digital index to Hachi types, and vice-versa.
    HachiDigital fromDigital(uint16_t dInput);
    uint16_t toDigital(digital_type type, uint16_t row, uint16_t column);
    uint16_t toDigital(HachiDigital digital);

    // Implement IDisplayReceiver
    void setGrid(uint16_t row, uint16_t column, uint16_t color);
    void setButton(uint16_t row, uint16_t column, uint16_t color);
    void setKey(uint16_t column, uint16_t color);
    void setByIndex(uint16_t index, uint16_t color);
    void Update();

    void ClearGrid();
    void DrawPalette();

    void SendMidiNote(uint8_t channel, uint8_t note, uint8_t velocity);

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
