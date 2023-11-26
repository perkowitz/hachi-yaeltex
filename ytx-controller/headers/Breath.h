// Basic flow sequencer module.

#ifndef BREATH_H
#define BREATH_H

#include <stdint.h>
#include "Arduino.h"
#include "Hardware.h"
#include "IModule.h"
#include "Display.h"

#define BREATH_MAX_QUAKES 3
#define BREATH_FIRST_QUAKE_ROW 1
#define BREATH_ALGORITHMIC_FILL_BUTTON 164
#define BREATH_TRACK_SHUFFLE_BUTTON 165


class Breath: public IModule {

  public:
    Breath();

    void Init(uint8_t index, Display *display);
    void Draw(bool update);
    void DrawTracksEnabled(Display *display, uint8_t gridRow);
    void SetColors(uint8_t primaryColor, uint8_t primaryDimColor);
    uint32_t GetMemSize();
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


  private:

    struct Memory {
      uint8_t midiChannel = 10; // this is not zero-indexed!
    } memory;

    void DrawButtons(bool update);


    uint8_t measureCounter = 0;
    uint8_t sixteenthCounter = 0;

    Display *display = nullptr;
    uint8_t index;
    Quake *quakes[BREATH_MAX_QUAKES];
    int quakeCount = 0;


};


#endif
