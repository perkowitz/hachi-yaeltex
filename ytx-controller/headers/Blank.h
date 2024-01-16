// Basic flow sequencer module.

#ifndef BLANK_H
#define BLANK_H

#include <stdint.h>
#include "Arduino.h"
#include "Hardware.h"
#include "IModule.h"
#include "Display.h"


class Blank: public IModule {

  public:
    Blank();

    void Init(uint8_t index, Display *display);
    void Draw(bool update);
    void DrawTracksEnabled(Display *display, uint8_t gridRow);
    void SetColors(uint8_t primaryColor, uint8_t primaryDimColor);
    uint32_t GetStorageSize();
    uint8_t GetIndex();

    void AddQuake(Quake *quake);

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
    void ToggleTrack(uint8_t trackNumber);

    // UI display
    uint8_t getColor();
    uint8_t getDimColor();

    // performance features
    void InstafillOn(u8 index = CHOOSE_RANDOM_FILL);
    void InstafillOff();
    void JumpOn(u8 step);
    void JumpOff();
    void SetScale(u8 root, bit_array_16 scale);
    void ClearScale();


  private:

    Display *display = nullptr;
    uint8_t index;

    struct Memory {
      uint8_t midiChannel = 1; // this is not zero-indexed!
    } memory;

};


#endif
