// Basic flow sequencer module.

#ifndef FLOW_H
#define FLOW_H

#include <stdint.h>
#include "Arduino.h"
#include "Hardware.h"
#include "IModule.h"
#include "Display.h"

#define STAGE_COUNT 16
#define ROW_COUNT 8
#define PATTERN_COUNT 1
#define FLOW_LENGTH 8
#define OUT_OF_RANGE -1

#define DEFAULT_OCTAVE 5
#define DEFAULT_VELOCITY 63
#define VELOCITY_DELTA 31

// define markers and indices that we'll use to story and point to colors
// markers
#define OFF_MARKER 0
#define NOTE_MARKER 1
#define SHARP_MARKER 2
#define FLAT_MARKER 3
#define OCTAVE_UP_MARKER 4
#define OCTAVE_DOWN_MARKER 5
#define VELOCITY_UP_MARKER 6
#define VELOCITY_DOWN_MARKER 7
#define EXTEND_MARKER 8
#define REPEAT_MARKER 9
#define TIE_MARKER 10
#define SKIP_MARKER 11
#define LEGATO_MARKER 12
#define RANDOM_MARKER 13

#define MARKER_COUNT (RANDOM_MARKER + 1)
#define PATTERN_MOD_ROW 5
#define PATTERN_MOD_STAGE (STAGE_COUNT + 1)


typedef struct Stage {
	int8_t note_count = 0;
	uint8_t note = OUT_OF_RANGE;
	int8_t octave = 0;
	int8_t velocity = 0;
	int8_t accidental = 0;
	int8_t extend = 0;
	int8_t repeat = 0;
	int8_t tie = 0;
	int8_t legato = 0;
	int8_t skip = 0;
	int8_t random = 0;
};

typedef struct StageOrder {
	uint8_t order[STAGE_COUNT];
};

typedef struct Pattern {
	uint8_t reset;
	uint8_t grid[ROW_COUNT][STAGE_COUNT + 1];
};


typedef struct Memory {
  uint8_t midiChannel = 1; // this is not zero-indexed!
  uint8_t measureReset = 1;
	Pattern patterns[PATTERN_COUNT];
};

const u8 note_map[8] = { 12, 11, 9, 7, 5, 4, 2, 0 };  // maps the major scale to note intervals (pads are low to high)




class Flow: public IModule {

  public:
    Flow();

    void Init(uint8_t index, Display *display);
    void Draw(bool update);
    void DrawTracksEnabled(Display *display, uint8_t gridRow);
    void SetColors(uint8_t primaryColor, uint8_t primaryDimColor);
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
    void ToggleTrack(uint8_t trackNumber);

    // UI display
    uint8_t getColor();
    uint8_t getDimColor();


  private:

    // static uint8_t marker_colors[MARKER_COUNT] = {
    //   ABS_BLACK, BRT_SKY_BLUE, BRT_CYAN, DIM_CYAN,  // off, note, sharp, flat
    //   BRT_ORANGE, DIM_ORANGE, BRT_GREEN, DIM_GREEN, // octave up, octave down, velo up, velo down
    //   DIM_BLUE, BRT_PINK, BRT_PURPLE, BRT_RED,      // extend, repeat, tie, skip
    //   DK_GRAY, BRT_YELLOW                           // legato, random
    // };


    Memory memory;

    Display *display = nullptr;
    uint8_t index;
    bool muted = false;

    Stage stages[STAGE_COUNT];

    bool inSettings = false;

    uint8_t currentMarker = OFF_MARKER;
    uint8_t currentPatternIndex = 0;
    u8 currentStageIndex = 0;
    u8 currentRepeat = 0;
    u8 currentExtend = 0;
    u8 currentNote = 0;
    u8 currentNoteColumn = 0;
    u8 currentNoteRow = 0;

    void DrawPalette(bool update);
    void DrawStages(bool update);
    void ClearPattern(int patternIndex);
    u8 GetPatternGrid(u8 patternIndex, u8 row, u8 column);
    void SetPatternGrid(u8 patternIndex, u8 row, u8 column, u8 value);
    u8 StageIndex(u8 stage);
    u8 GetNote(Stage *stage);
    u8 GetVelocity(Stage *stage);
    void NoteOff();
    void UpdateStage(Stage *stage, u8 row, u8 column, u8 marker, bool turn_on);


};



#endif
