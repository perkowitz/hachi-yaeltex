// Basic flow sequencer module.

#ifndef FLOW_H
#define FLOW_H

#include <stdint.h>
#include "Arduino.h"
#include "Hardware.h"
#include "IModule.h"
#include "Display.h"



class Flow: public IModule {

  public:
    Flow();

    void Init(uint8_t index, Display *display);
    void Draw(bool update);
    // void SetDisplay(Display *display);
    uint32_t GetMemSize();
    uint8_t GetIndex();

    bool IsMuted();
    void SetMuted(bool muted);

    // syncs to an external clock
    void Start();
    void Stop();
    void Pulse(uint16_t measureCounter, uint16_t sixteenthCounter, uint16_t pulseCounter);

    // receives UX events
    void GridEvent(uint8_t row, uint8_t column, uint8_t pressed);
    void ButtonEvent(uint8_t row, uint8_t column, uint8_t pressed);
    void KeyEvent(uint8_t column, uint8_t pressed);

    // UI display
    uint8_t getColor();
    uint8_t getDimColor();


  private:

    struct Memory {
      uint8_t midiChannel = 10; // this is not zero-indexed!
    } memory;

    Display *display = nullptr;
    uint8_t index;


};


#endif
