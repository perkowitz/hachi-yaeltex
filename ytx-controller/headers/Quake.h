// Basic drum machine module.

#ifndef QUAKE_H
#define QUAKE_H

#include <stdint.h>
#include "Arduino.h"
#include "Hardware.h"
#include "IModule.h"
#include "Display.h"

typedef enum {PATTERN, ALGORITHMIC, BOTH} autofill_type;

#define STEPS_PER_MEASURE 16
#define MEASURES_PER_PATTERN 4
#define TRACKS_PER_PATTERN 16
#define NUM_PATTERNS 2
#define NUM_AUTOFILL_INTERVALS 4

// velocity = (v_setting + 1) * (128 / V_L) - 1
/*  
velocity                  = (v_setting + 1) * (128 / V_L) - 1
velo + 1                  = (v_setting + 1) * (128 / V_L)
(velo + 1) / (128/V_L)    = v_setting + 1
(velo + 1) / (128/V_L) -1 = v_setting
*/
// v-setting = (velocity + 1) / (128 / VELOCITY_LEVELS) + 1;

// 0 <= v_setting < V_L
#define VELOCITY_LEVELS 8

#define CLOCK_ROW 0
#define MODE_ROW 1
#define TRACK_ENABLED_ROW 2
#define TRACK_SELECT_ROW 3
#define FIRST_MEASURES_ROW 4
#define VELOCITY_ROW 5

#define INITIAL_VELOCITY 79

#define MEASURE_SELECT_MIN_COLUMN 0
#define MEASURE_SELECT_MAX_COLUMN 3
#define MEASURE_MODE_MIN_COLUMN 4
#define MEASURE_MODE_MAX_COLUMN 7
#define AUTOFILL_INTERVAL_MIN_COLUMN 12
#define AUTOFILL_INTERVAL_MAX_COLUMN 15
#define AUTOFILL_TYPE_COLUMN 11

// class Quake {
class Quake: public IModule {

  public:
    Quake();

    void Init();
    void Draw(bool update);
    void SetDisplay(Display *display);

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

    /***** Structs for storing sequence data. *****/
    struct Measure {
      int8_t steps[STEPS_PER_MEASURE];
    };

    struct Track {
      Measure measures[MEASURES_PER_PATTERN];
    };

    struct Pattern {
      Track tracks[TRACKS_PER_PATTERN];
    };

    /***** Private vars. *****/
    // const static PATTERN_MEM_SIZE = sizeof(patterns[0]);
    const static uint16_t PATTERN_MEM_SIZE = TRACKS_PER_PATTERN * MEASURES_PER_PATTERN * STEPS_PER_MEASURE;

    Display *display = nullptr;
    bool muted = false;

    uint8_t midi_notes[TRACKS_PER_PATTERN] = { 36, 37, 38, 39, 40, 41, 43, 45, 42, 46, 44, 49, 47, 48, 50, 51 };

    Pattern patterns[NUM_PATTERNS];
    bool trackEnabled[16];  

    Pattern currentPattern = patterns[0];
    uint8_t currentMeasure = 0;
    uint8_t currentStep = 0;
    uint8_t selectedTrack = 0;
    uint8_t selectedMeasure = 0;
    uint8_t selectedStep = 0;
    uint8_t measureMode = 0;

    bool soundingTracks[TRACKS_PER_PATTERN];

    uint8_t autofillIntervals[NUM_AUTOFILL_INTERVALS] = { 4, 8, 12, 16 };
    int8_t autofillIntervalSetting = -1;   // -1 = disabled
    autofill_type autofillType = PATTERN;
    bool autofillPlaying = false;

    uint8_t measureReset = 1;
    uint8_t midiChannel = 10;   // this is not zero-indexed!


    /***** Private methods. *****/
    // draw the UI
    void DrawTracks(bool update);
    void DrawMeasures(bool update);
    void DrawButtons(bool update);
    
    // MIDI functionality
    void SendNotes();
    void AllNotesOff();
    void ClearSoundingNotes();
    uint8_t toVelocity(uint8_t v);
    uint8_t fromVelocity(uint8_t velocity);

    // Misc
    void Reset();
    void ResetPattern(Pattern pattern);
    bool isFill(uint8_t measure);
    void NextMeasure(uint8_t measureCounter);
    int RandomFillPattern();
    int RandomAlgorithmicPattern();

};

// important buttons mapped to indices
#define QUAKE_SAVE_BUTTON 157
#define QUAKE_LOAD_BUTTON 149
#define QUAKE_TEST_BUTTON 158

// main palette colors
// #define PRIMARY_COLOR BRT_GREEN
// #define PRIMARY_DIM_COLOR DIM_GREEN
#define PRIMARY_COLOR BRT_RED
#define PRIMARY_DIM_COLOR DIM_RED
#define SECONDARY_COLOR BRT_RED
#define SECONDARY_DIM_COLOR DIM_RED
// #define SECONDARY_COLOR BRT_MAGENTA
// #define SECONDARY_DIM_COLOR DIM_MAGENTA
#define ACCENT_COLOR BRT_YELLOW
#define ACCENT_DIM_COLOR DIM_YELLOW
#define OFF_COLOR DK_GRAY
#define ON_COLOR WHITE

// element colors
#define TRACK_ENABLED_OFF_COLOR PRIMARY_DIM_COLOR
#define TRACK_ENABLED_ON_COLOR PRIMARY_COLOR
#define TRACK_ENABLED_OFF_PLAY_COLOR ACCENT_DIM_COLOR
#define TRACK_ENABLED_ON_PLAY_COLOR ON_COLOR
#define TRACK_SELECT_OFF_COLOR OFF_COLOR
#define TRACK_SELECT_SELECTED_COLOR ON_COLOR
#define STEPS_OFF_COLOR ABS_BLACK
#define STEPS_ON_COLOR OFF_COLOR
#define STEPS_FILL_ON_COLOR SECONDARY_DIM_COLOR
#define STEPS_OFF_SELECT_COLOR ACCENT_DIM_COLOR
#define STEPS_ON_SELECT_COLOR ACCENT_COLOR
#define VELOCITY_OFF_COLOR PRIMARY_DIM_COLOR
#define VELOCITY_ON_COLOR ON_COLOR
#define MEASURE_SELECT_OFF_COLOR OFF_COLOR
#define MEASURE_SELECT_SELECTED_COLOR ON_COLOR
#define MEASURE_SELECT_PLAYING_COLOR ACCENT_COLOR
#define MEASURE_SELECT_AUTOFILL_COLOR SECONDARY_COLOR
#define MEASURE_MODE_OFF_COLOR SECONDARY_DIM_COLOR
#define MEASURE_MODE_ON_COLOR SECONDARY_COLOR
#define AUTOFILL_OFF_COLOR SECONDARY_DIM_COLOR
#define AUTOFILL_ON_COLOR SECONDARY_COLOR




#endif
