// Interface for an object that receives display events -- turning LEDs on/off, setting colors, etc.

#ifndef IDISPLAY_RECEIVER_H
#define IDISPLAY_RECEIVER_H

#include <stdint.h>
#include "Arduino.h"


class IDisplayReceiver {

  public:
    virtual void setGrid(uint16_t row, uint16_t index, uint16_t color) = 0;
    virtual void setButton(uint16_t row, uint16_t index, uint16_t color) = 0;
    virtual void setKey(uint16_t index, uint16_t color) = 0;
    virtual void setByIndex(uint16_t index, uint16_t color) = 0;
    virtual void Update() = 0;

  private:

};




#endif
