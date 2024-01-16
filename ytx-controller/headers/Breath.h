// Basic flow sequencer module.

#ifndef BREATH_H
#define BREATH_H

#include <stdint.h>
#include "Arduino.h"
#include "Hardware.h"
#include "Hachi.h"
#include "IModule.h"
#include "Display.h"
#include "Fill.h"

#define B_MAX_MODULES 7
#define B_MAX_QUAKES 3
#define B_FIRST_QUAKE_ROW 1
#define B_FIRST_MODULE_ROW 1
#define B_ALGORITHMIC_FILL_BUTTON 164
#define B_LAST_FILL_BUTTON 165
// #define B_TRACK_SHUFFLE_BUTTON 166
#define B_CHORD_MODE_BUTTON 159
#define CHORD_ROW 6
#define SCALE_COUNT 4
#define CHORD_PATTERN_ROW 7

// major scale in binary is 1010 1101 0101   (on the 12-tone scale, 1= included in the major scale)
//   reversed is 1010 1011 0101
//   in hex that is ab5
// minor scale is 1011 0101 1010 (natural minor / relative minor)
//   reversed is 0101 1010 1101
//   in hex, 5ad
// WTF is 1001 1011 0101; backwards 1010 1101 1001; hex ad9
#define MAJOR_SCALE 0xab5
#define MINOR_SCALE 0x5ad
#define WTF_SCALE 0xad9


class Breath: public IModule {

  public:
    Breath();

    void Init(uint8_t index, Display *display);
    void Draw(bool update);
    void DrawTracksEnabled(Display *display, uint8_t gridRow);
    void SetColors(uint8_t primaryColor, uint8_t primaryDimColor);
    uint32_t GetStorageSize();
    uint8_t GetIndex();

    void AddModule(IModule *module);
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

    struct Memory {
      uint8_t midiChannel = 10; // this is not zero-indexed!
    } memory;

    void DrawModuleTracks(bool update);
    void DrawChordMode(bool update);
    void DrawButtons(bool update);

    Display *display = nullptr;

    uint8_t measureCounter = 0;
    uint8_t sixteenthCounter = 0;
    s8 currentStep = 0;
    int lastFill = 0;
    bool stuttering = false;
    bool chordMode = false;

    uint8_t index;
    Quake *quakes[B_MAX_QUAKES];
    IModule *modules[B_MAX_MODULES];
    int quakeCount = 0;
    int moduleCount = 0;
    int fillPattern = -1;

    int currentScaleIndex = 0;
    bit_array_16 currentScale = MAJOR_SCALE;
    int currentRoot = 0;

};


#endif
