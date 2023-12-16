// Interface for an object that receives control events -- button presses, etc.

#ifndef IMODULE_H
#define IMODULE_H

#include <stdint.h>
#include "Arduino.h"
#include "Display.h"


class IModule {

  public:
    virtual void Init(uint8_t index, Display *display) = 0;
    virtual void Draw(bool update) = 0;
    virtual void DrawTracksEnabled(Display *display, uint8_t gridRow) = 0;
    virtual void SetColors(uint8_t primaryColor, uint8_t primaryDimColor) = 0;
    virtual uint32_t GetMemSize() = 0;
    virtual uint8_t GetIndex() = 0;

    virtual bool IsMuted() = 0;
    virtual void SetMuted(bool muted) = 0;

    // syncs to an external clock
    virtual void Start() = 0;
    virtual void Stop() = 0;
    virtual void Pulse(uint16_t measureCounter, uint16_t sixteenthCounter, uint16_t pulseCounter) = 0;

    // receives UX events
    virtual void GridEvent(uint8_t x, uint8_t y, uint8_t pressed) = 0;
    virtual void ButtonEvent(uint8_t x, uint8_t y, uint8_t pressed) = 0;
    virtual void KeyEvent(uint8_t x, uint8_t pressed) = 0;
    virtual void ToggleTrack(uint8_t trackNumber) = 0;

    // UI display
    virtual uint8_t getColor() = 0;
    virtual uint8_t getDimColor() = 0;

    // performance features
    virtual void InstafillOn(u8 index) = 0;
    virtual void InstafillOff() = 0;

  private:

};


#endif
