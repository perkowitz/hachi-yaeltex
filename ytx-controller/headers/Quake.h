// Basic drum machine module.

#ifndef QUAKE_H
#define QUAKE_H

#include <stdint.h>
#include "Arduino.h"
#include "Hardware.h"
#include "IModule.h"
#include "Display.h"
#include "Utility.h"
#include "Fill.h"
#include "Hachi.h"

typedef enum {PATTERN, ALGORITHMIC, BOTH} autofill_type;

#define STEPS_PER_MEASURE 16
#define MEASURES_PER_PATTERN 4
#define TRACKS_PER_PATTERN 16
#define Q_PATTERN_COUNT 8
#define SAVING true
#define LOADING false

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
#define PATTERN_ROW 4

#define Q_NOTE_SETTINGS_ROW 3
#define Q_NOTE_RESET_BUTTON 16
#define NUM_AUTOFILL_INTERVALS 4

#define INITIAL_VELOCITY 4

// UI elements in the grid
#define MEASURE_SELECT_MIN_COLUMN 0
#define MEASURE_SELECT_MAX_COLUMN 3
#define MEASURE_MODE_MIN_COLUMN 4
#define MEASURE_MODE_MAX_COLUMN 7
#define Q_AUTOFILL_INTERVAL_MIN_COLUMN 12
#define Q_AUTOFILL_INTERVAL_MAX_COLUMN 15
#define Q_AUTOFILL_TYPE_COLUMN 11
#define STUTTER_LENGTH_MIN_COLUMN 8
#define STUTTER_LENGTH_MAX_COLUMN 10

// important buttons mapped to indices
#define Q_SETTINGS_BUTTON 155
#define Q_SAVE_BUTTON 157
#define Q_COPY_BUTTON 156
#define Q_CLEAR_BUTTON 148
#define Q_ALGORITHMIC_FILL_BUTTON 164
#define Q_LAST_FILL_BUTTON 165
#define Q_TRACK_SHUFFLE_BUTTON 166
#define Q_INSTAFILL_MIN_KEY 0
#define Q_INSTAFILL_MAX_KEY 3
#define Q_PERF_MODE_BUTTON 171
#define Q_STUTTER_BUTTON 170


// class Quake {
class Quake: public IModule {

  public:
    Quake();

    void Init(uint8_t index, Display *display);
    void Draw(bool update);
    void DrawTracksEnabled(Display *display, uint8_t gridRow);
    void SetColors(uint8_t primaryColor, uint8_t primaryDimColor);
    uint32_t GetStorageSize();
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
    void ToggleTrack(uint8_t trackNumber);

    // UI display
    uint8_t getColor();
    uint8_t getDimColor();

    // performance features
    void InstafillOn(u8 index = CHOOSE_RANDOM_FILL);
    void InstafillOff();
    void JumpOn(u8 step);
    void JumpOff();
    void SetScale(u8 tonic, bit_array_16 scale);
    void ClearScale();


  private:

    /***** Structs for storing sequence data. *****/
    // pattern = 16 tracks x 4 measures x 16 steps = 1024 bytes + 3 bytes
    typedef struct Measure {
      nib_array_16 steps;
    };

    typedef struct Track {
      Measure measures[MEASURES_PER_PATTERN];
    };

    typedef struct Pattern {
      Track tracks[TRACKS_PER_PATTERN];
      int8_t autofillIntervalSetting = -1;   // -1 = disabled
      autofill_type autofillType = PATTERN;
      uint8_t measureMode = 0;
    };

    typedef struct Memory {
      uint8_t midiChannel = 10; // this is not zero-indexed!
      s8 measureReset = 0;
      bit_array_16 trackEnabled;
      uint8_t currentPatternIndex = 0;
      int stutterLength = 0;
      u8 midiNotes[TRACKS_PER_PATTERN] = { 36, 37, 38, 39, 40, 41, 43, 45, 42, 46, 44, 49, 47, 48, 50, 51 };
    };

    /***** Private vars. *****/

    Memory memory; 
    uint8_t primaryColor = BRT_RED;
    uint8_t primaryDimColor = DIM_RED;

    uint8_t autofillIntervals[NUM_AUTOFILL_INTERVALS] = { 4, 8, 12, 16 };
    uint8_t trackMap[TRACKS_PER_PATTERN] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 };
    s8 patternMap[STEPS_PER_MEASURE] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 };

    Display *display = nullptr;
    uint8_t index;
    bool muted = false;

    Pattern *patternBuffer1 = new Pattern();
    Pattern *patternBuffer2 = new Pattern();
    Pattern *currentPattern = patternBuffer1;
    Pattern *nextPattern = patternBuffer2;
    int nextPatternIndex = -1;
    uint8_t currentMeasure = 0;
    uint8_t currentStep = 0;
    uint8_t selectedTrack = 0;
    uint8_t selectedMeasure = 0;
    uint8_t selectedStep = 0;
    uint8_t measureCounter = 0;
    uint8_t sixteenthCounter = 0;

    bool stuttering = false;
    uint8_t stutterStep = 0;
    bool inPerfMode = false;
    bool inInstafill = false;
    uint8_t originalMeasure = 0;
    s8 editingNotesTrack = -1;
    s8 editingNotesValue = -1;
    int lastFill = 0;
    
    bool copying = false;
    bool copyingFirst = false;
    Hardware::HachiDigital copyDigital;
    bool clearing = false;
    bool inSettings = false;

    bool soundingTracks[TRACKS_PER_PATTERN];
    bool autofillPlaying = false;
    bool instafillPlaying = false;


    /***** Private methods. *****/
    // draw the UI
    void DrawTracks(bool update);
    void DrawMeasures(bool update);
    void DrawButtons(bool update);
    void DrawOptions(bool update);
    void DrawSettings(bool update);
    
    // MIDI functionality
    void SendNotes();
    void AllNotesOff();
    void ClearSoundingNotes();
    uint8_t toVelocity(uint8_t v);
    uint8_t fromVelocity(uint8_t velocity);

    // copy/clear
    void Copying(digital_type type, uint8_t row, uint8_t column, uint8_t pressed);
    void Clearing(digital_type type, uint8_t row, uint8_t column, uint8_t pressed);
    void CopyTrack(u8 sourceTrack, u8 destTrack);
    void CopyMeasure(u8 sourceMeasure, u8 destMeasure);
    void CopyTrackMeasures(u8 trackIndex);
    void ResetCurrentPattern();
    void ResetTrack(uint8_t trackIndex);
    void ResetMeasure(uint8_t measureIndex);

    // Misc
    void ResetTrackEnabled();
    // void ResetPattern(uint8_t patternIndex);
    bool isFill(uint8_t measure);
    void NextMeasure(uint8_t measureCounter);
    int RandomFillPattern();
    void SelectAlgorithmicFill();
    void SavePattern();
    void LoadPattern();
    void SaveSettings();
    void LoadSettings();
    void SetMidiNotes();

};



// main palette colors
// #define PRIMARY_COLOR BRT_GREEN
// #define PRIMARY_DIM_COLOR DIM_GREEN
#define PRIMARY_COLOR primaryColor
#define PRIMARY_DIM_COLOR primaryDimColor
// #define SECONDARY_COLOR BRT_RED
// #define SECONDARY_DIM_COLOR DIM_RED
// #define SECONDARY_COLOR BRT_MAGENTA
// #define SECONDARY_DIM_COLOR DIM_MAGENTA
#define ACCENT_COLOR BRT_YELLOW
#define ACCENT_DIM_COLOR DIM_YELLOW
#define OFF_COLOR DK_GRAY
#define ON_COLOR WHITE
#define PERF_COLOR BRT_YELLOW
#define PERF_DIM_COLOR DIM_YELLOW

// element colors
#define TRACK_ENABLED_OFF_COLOR PRIMARY_DIM_COLOR
#define TRACK_ENABLED_ON_COLOR PRIMARY_COLOR
#define TRACK_ENABLED_OFF_PLAY_COLOR OFF_COLOR
#define TRACK_ENABLED_ON_PLAY_COLOR ON_COLOR
#define TRACK_SELECT_OFF_COLOR OFF_COLOR
#define TRACK_SELECT_SELECTED_COLOR ON_COLOR
#define STEPS_OFF_COLOR ABS_BLACK
#define STEPS_ON_COLOR PRIMARY_COLOR
#define STEPS_FILL_ON_COLOR H_FILL_DIM_COLOR
#define STEPS_OFF_SELECT_COLOR ACCENT_DIM_COLOR
#define STEPS_ON_SELECT_COLOR ACCENT_COLOR
#define VELOCITY_OFF_COLOR PRIMARY_DIM_COLOR
#define VELOCITY_ON_COLOR ON_COLOR
#define MEASURE_SELECT_OFF_COLOR OFF_COLOR
#define MEASURE_SELECT_SELECTED_COLOR ACCENT_COLOR
#define MEASURE_SELECT_PLAYING_COLOR ON_COLOR
#define MEASURE_SELECT_AUTO_FILL_COLOR H_FILL_COLOR
#define MEASURE_MODE_OFF_COLOR PRIMARY_DIM_COLOR
#define MEASURE_MODE_ON_COLOR PRIMARY_COLOR
#define AUTOFILL_OFF_COLOR H_FILL_DIM_COLOR
#define AUTOFILL_ON_COLOR H_FILL_COLOR
#define TRACK_SHUFFLE_OFF_COLOR H_FILL_DIM_COLOR
#define TRACK_SHUFFLE_ON_COLOR H_FILL_COLOR





#endif
