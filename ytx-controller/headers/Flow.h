// Basic flow sequencer module.

#ifndef FLOW_H
#define FLOW_H

#include <stdint.h>
#include "Arduino.h"
#include "Hardware.h"
#include "IModule.h"
#include "Display.h"
#include "Utility.h"
#include "Fill.h"
#include "Scales.h"

#define STAGE_COUNT 16
#define ROW_COUNT 8
#define F_PATTERN_COUNT 8
// #define FLOW_LENGTH 8
#define OUT_OF_RANGE 255

#define DEFAULT_OCTAVE 5
#define DEFAULT_VELOCITY 63
#define VELOCITY_DELTA 31

#define DEFAULT_SCALE MAJOR_SCALE
#define DEFAULT_ROOT 0

// define markers and indices that we'll use to store and point to colors
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

// pattern mod is an extra stage that modifies the whole pattern
// the PM_ROW is the button row where it appears, used for drawing
// while the PM_STAGE is where it is in the stage array, used for updating data
#define PATTERN_MOD_ROW 5
#define PATTERN_MOD_STAGE STAGE_COUNT

#define F_STAGES_ENABLED_ROW 5
#define F_STAGES_SKIPPED_ROW 4
#define F_PERF_JUMP_ROW 7
#define F_CLOCK_ROW 0
#define F_STAGE_ENABLED_OFF_COLOR DK_GRAY
#define F_STAGE_ENABLED_ON_COLOR LT_GRAY

#define F_AUTOFILL_ROW 1
#define F_AUTOFILL_INTERVAL_MIN_COLUMN 12
#define F_AUTOFILL_INTERVAL_MAX_COLUMN 15

// buttons by index
#define F_SETTINGS_BUTTON 155
#define F_SAVE_BUTTON 157
#define F_LOAD_BUTTON 149
#define F_COPY_BUTTON 156
#define F_CLEAR_BUTTON 148
#define F_PERF_MODE_BUTTON 151
#define F_STUTTER_BUTTON 159
#define F_ALGORITHMIC_FILL_BUTTON 164
#define F_LAST_FILL_BUTTON 165
#define F_SHIFT_LEFT_BUTTON 158
#define F_SHIFT_RIGHT_BUTTON 150


// stage translates the settings of the pattern into attributes
// 16 stages = 11 bytes * 16 stages = 176 bytes
// these are not saved, but recomputed from the pattern grid
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

// pattern = 17 stages * 8 rows = 136 bytes + 1 byte
typedef struct Pattern {
	u8 reset;
  s8 autofillIntervalSetting = -1;   // -1 = disabled
  u8 scaleTonic;
  bit_array_16 scale;
	uint8_t grid[ROW_COUNT][STAGE_COUNT + 1];
};


typedef struct Memory {
  uint8_t midiChannel = 1; // this is not zero-indexed!
  uint8_t measureReset = 1;
  uint8_t currentPatternIndex = 0;
	Pattern patterns[F_PATTERN_COUNT];
};





class Flow: public IModule {

  public:
    Flow();

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
    void EncoderEvent(u8 encoder, u8 value);
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
    void SetChord(u8 tonic, bit_array_16 chord);
    void ClearChord();
    void Shift(s8 direction);

    void Save();
    void Load();

  private:

    // static uint8_t marker_colors[MARKER_COUNT] = {
    //   ABS_BLACK, BRT_SKY_BLUE, BRT_CYAN, DIM_CYAN,  // off, note, sharp, flat
    //   BRT_ORANGE, DIM_ORANGE, BRT_GREEN, DIM_GREEN, // octave up, octave down, velo up, velo down
    //   DIM_BLUE, BRT_PINK, BRT_PURPLE, BRT_RED,      // extend, repeat, tie, skip
    //   DK_GRAY, BRT_YELLOW                           // legato, random
    // };

    uint8_t primaryColor = BRT_PURPLE;
    uint8_t primaryDimColor = DIM_PURPLE;

    Memory memory;

    Display *display = nullptr;
    uint8_t index;
    bool muted = false;

    Stage stages[STAGE_COUNT + 1];
    s8 stageMap[STAGE_COUNT] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 };
    uint8_t autofillIntervals[NUM_AUTOFILL_INTERVALS] = { 4, 8, 12, 16 };
    u8 note_map[8] = { 12, 11, 9, 7, 5, 4, 2, 0 };  // maps the major scale to note intervals (pads are low to high)

    u8 controllerNumbers[ENCODER_COUNT] = { 16, 17, 18, 19, 20, 21, 22, 23 };
    u8 controllerValues[ENCODER_COUNT] = { 0, 0, 0, 0, 0, 0, 0, 0 };

    bool inSettings = false;
    bool inPerfMode = false;
    bool instafilling = false;
    bool lastInstafilling = false;
    bool autofilling = false;

    bool copying = false;
    bool copyingFirst = false;
    Hardware::HachiDigital copyDigital;
    bool clearing = false;

    bool transposeEnabled = true;
    bool scaleEnabled = true;
    bool chordEnabled = true;
    s8 transpose = 0;

    uint8_t currentMarker = OFF_MARKER;
    s8 nextPatternIndex = -1;
    u8 currentStageIndex = 0;
    u8 previousStageIndex = 0;
    u8 currentRepeat = 0;
    u8 currentExtend = 0;
    u8 currentNote = 0;
    u8 currentNoteColumn = 0;
    u8 currentNoteRow = 0;
    int lastFill = 0;
    u8 measureCounter = 0;
    u8 sixteenthCounter = 0;
    bool stuttering = false;
    u8 stutterStage = 0;

    bit_array_16 stagesEnabled;
    bit_array_16 stagesSkipped;

    void DrawEncoders(bool update);
    void DrawPalette(bool update);
    void DrawStages(bool update);
    void DrawPatterns(bool update);
    void DrawButtons(bool update);
    void DrawSettings(bool update);

    void ClearPattern(int patternIndex);
    u8 GetPatternGrid(u8 patternIndex, u8 row, u8 column);
    void SetPatternGrid(u8 patternIndex, u8 row, u8 column, u8 value);
    u8 GetNote(Stage *stage);
    u8 GetVelocity(Stage *stage);
    void NoteOff();
    void UpdateStage(Stage *stage, u8 row, u8 column, u8 marker, bool turn_on);
    void LoadStages(int patternIndex);
    void ClearStage(int stage);
    void SetStageMap(u8 index);
    void ClearStageMap();
    void SetNoteMap(u8 tonic, bit_array_16 scale);

};



#endif
