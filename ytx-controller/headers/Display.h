// Interface for an object that receives display events -- turning LEDs on/off, setting colors, etc.

#ifndef DISPLAY_H
#define DISPLAY_H

#include <stdint.h>
#include "Arduino.h"


class Display {

  public:
    Display();

    // draw various UX components
    void setGrid(uint16_t row, uint16_t column, uint16_t color);
    void setButton(uint16_t row, uint16_t column, uint16_t color);
    void setKey(uint16_t column, uint16_t color);
    void setByIndex(uint16_t index, uint16_t color);
    void FillGrid(uint16_t color);
    void FillGrid(uint16_t color, u8 startRow, u8 endRow);
    void FillModule(uint16_t color, bool doGrid, bool doButtons, bool doKeys);

    // force update of all components
    void Update();

    void setHardware(Hardware *hardware);
    void setEnabled(bool enabled);

    void DrawValueOnGrid(u8 value, u8 color);
    void DrawClock(u8 row, u8 measureCounter, u8 sixteenthCounter, u8 stepCounter);

  private:
    Hardware *hardware;
    bool enabled = false;

};




#endif
