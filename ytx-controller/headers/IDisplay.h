// Interface for an object that receives display events -- turning LEDs on/off, setting colors, etc.

#ifndef IDISPLAY_H
#define IDISPLAY_H

#include <stdint.h>
#include "Arduino.h"


class IDisplay {

  public:
    // draw various UX components
    virtual void setGrid(uint16_t row, uint16_t index, uint16_t color) = 0;
    virtual void setButton(uint16_t row, uint16_t index, uint16_t color) = 0;
    virtual void setKey(uint16_t index, uint16_t color) = 0;
    virtual void setByIndex(uint16_t index, uint16_t color) = 0;

    // force update of all components
    virtual void Update() = 0;

  private:

};




#endif
