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
#include "Scales.h"

#define B_STEPS_PER_PATTERN 16

#define B_MAX_MODULES 7
#define B_MAX_QUAKES 3
#define B_FIRST_QUAKE_ROW 1
#define B_FIRST_MODULE_ROW 1
#define B_ALGORITHMIC_FILL_BUTTON 164
#define B_LAST_FILL_BUTTON 165
// #define B_TRACK_SHUFFLE_BUTTON 166
#define B_CHORD_MODE_BUTTON 159
#define B_SCALE_ROW 2
#define B_SCALE_COUNT 4
#define B_STEP_PLAY_ROW 5
#define B_STEP_SELECT_ROW 6
#define B_CHORD_ROW 7
#define B_CHORD_COUNT 12
#define B_CHORD_SEQ_ROW 3
#define B_BASS_SEQ_ROW 4

#define CHORD_OFF_COLOR DIM_CYAN
#define CHORD_ON_COLOR WHITE
#define CHORD_MAJOR_COLOR DIM_BLUE_GRAY
#define CHORD_MINOR_COLOR DIM_SKY_BLUE 
#define CHORD_OTHER_COLOR DK_GRAY
#define SCALE_OFF_COLOR DK_GRAY
#define SCALE_ON_COLOR WHITE
#define STEP_PLAY_OFF_COLOR DIM_SKY_BLUE
#define STEP_PLAY_ON_COLOR BRT_SKY_BLUE
#define STEP_SELECT_OFF_COLOR DK_GRAY
#define STEP_SELECT_ON_COLOR WHITE
#define KEYS_OFF_COLOR ABS_BLACK
#define KEYS_SCALE_COLOR SCALE_OFF_COLOR
#define KEYS_TONIC_COLOR SCALE_ON_COLOR
#define KEYS_CHORD_COLOR CHORD_OFF_COLOR
// #define KEYS_ROOT_COLOR CHORD_ON_COLOR
#define KEYS_ROOT_COLOR ACCENT_COLOR
#define CHORD_SEQ_OFF_COLOR ABS_BLACK
#define CHORD_SEQ_ON_COLOR DIM_GREEN
#define BASS_SEQ_OFF_COLOR ABS_BLACK
#define BASS_SEQ_ON_COLOR DIM_RED




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
    void SetScale(u8 tonic, bit_array_16 scale);
    void ClearScale();


  private:

    typedef struct Step {
      s8 root;
      s8 chordIndex;
    };

    typedef struct Pattern {
      Step steps[B_STEPS_PER_PATTERN];
      bit_array_16 chordSequence = 0;
      bit_array_16 bassSequence = 0;
    };

    struct Memory {
      uint8_t midiChannel = 7; // this is not zero-indexed!
      bit_array_16 chords[B_CHORD_COUNT] = { 
        MAJOR_CHORD, MAJOR_6TH_CHORD, MAJOR_7TH_CHORD, MAJOR_AUG_CHORD,
        MINOR_CHORD, MINOR_6TH_CHORD, MINOR_7TH_CHORD, MINOR_DIM_CHORD,
        SUS2_CHORD, SUS4_CHORD,
        MAJOR_CHORD, MINOR_CHORD
      };
    } memory;

    void DrawModuleTracks(bool update);
    void DrawChordMode(bool update);
    void DrawChord(int root, bit_array_16 chord, bool update);
    void DrawScale(bool update);
    void DrawButtons(bool update);
    void ChordNotesOff();
    void PlayChord(u8 root, bit_array_16 chord);
    void BassNoteOff();
    void PlayBass(u8 root);
    u8 GetChordColor(bit_array_16 chord);
    void ChordModeGridEvent(uint8_t row, uint8_t column, uint8_t pressed);

    Display *display = nullptr;

    bool muted = false;
    uint8_t measureCounter = 0;
    uint8_t sixteenthCounter = 0;
    s8 currentStep = 0;
    int lastFill = 0;
    bool stuttering = false;
    bool chordMode = false;
    bool editingScale = false;
    bool editingStep = false;

    uint8_t index;
    Quake *quakes[B_MAX_QUAKES];
    IModule *modules[B_MAX_MODULES];
    int quakeCount = 0;
    int moduleCount = 0;
    int fillPattern = -1;

    u8 currentChordStep = 0;
    u8 selectedChordStep = 0;

    Pattern pattern;

    // a scale is a tonic and a set of included notes
    int currentScaleIndex = 0;
    bit_array_16 currentScale = MAJOR_SCALE;
    int currentTonic = 0;

    // a chord is a root and a set of included notes
    int currentChordIndex = 0;
    bit_array_16 currentChord = MAJOR_CHORD;
    int currentRoot = 0;
    u8 chordPlayOctave = 5;
    u8 chordPlayVelocity = 64;
    u8 bassPlayOctave = 3;
    u8 bassPlayVelocity = 80;
    u8 chordMidiChannel;
    u8 bassMidiChannel;

    s8 playingChordNotes[MAX_CHORD_NOTES];  // hold notes currently playing
    s8 playingBassNote = -1;
};


#endif
