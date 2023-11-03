// Interface for an object that receives control events -- button presses, etc.

#ifndef IMODULE_H
#define IMODULE_H

#include <stdint.h>
#include "Arduino.h"
#include "IDisplay.h"


class IModule {

  public:
    virtual void Init() = 0;
    virtual void Draw(bool update) = 0;
    virtual void SetDisplay(IDisplay *display) = 0;

    // syncs to an external clock
    virtual void Start() = 0;
    virtual void Stop() = 0;
    virtual void Pulse(uint16_t measureCounter, uint16_t sixteenthCounter, uint16_t pulseCounter) = 0;

    // receives UX events
    virtual void GridEvent(uint8_t x, uint8_t y, uint8_t pressed) = 0;
    virtual void ButtonEvent(uint8_t x, uint8_t y, uint8_t pressed) = 0;
    virtual void KeyEvent(uint8_t x, uint8_t pressed) = 0;

  private:

};


#endif
