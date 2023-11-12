// Interface for an object that receives control events -- button presses, etc.

#ifndef ICONTROL_RECEIVER_H
#define ICONTROL_RECEIVER_H

#include <stdint.h>
#include "Arduino.h"


class IControlReceiver {

  public:
    virtual void GridEvent(uint8_t x, uint8_t y, uint8_t pressed) = 0;
    virtual void ButtonEvent(uint8_t x, uint8_t y, uint8_t pressed) = 0;
    virtual void KeyEvent(uint8_t x, uint8_t pressed) = 0;

  private:

};


#endif
