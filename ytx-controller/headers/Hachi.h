#ifndef HACHI_H
#define HACHI_H

#include <stdint.h>
#include "Arduino.h"
#include "FeedbackClass.h"
#include "Display.h"
#include "Quake.h"
#include "Flow.h"
#include "Breath.h"
#include "Blank.h"

#define MODULE_TYPE_COUNT 4
typedef enum {QUAKE, FLOW, BREATH, BLANK } module_type;

// various rows and settings
#define MODULE_SELECT_BUTTON_ROW 0
#define MODULE_MUTE_BUTTON_ROW 1
#define MODULE_COUNT 8
#define SET_ENABLED true
#define SET_DISABLED false

#define H_MIDI_CHANNEL_ROW 7
#define H_SETTINGS_ROW 5
#define H_RESET_START_COLUMN 0
#define H_RESET_END_COLUMN 3
#define NUM_AUTOFILL_INTERVALS 4

// important buttons mapped to indices
#define START_BUTTON 152
#define PANIC_BUTTON 153
#define PALETTE_BUTTON 144
#define GLOBAL_SETTINGS_BUTTON 154

// global control colors
#define BUTTON_OFF DK_GRAY
#define BUTTON_ON WHITE
#define START_NOT_RUNNING DIM_RED
#define START_RUNNING DIM_RED
#define START_PULSE_MEASURE BRT_YELLOW
#define START_PULSE BRT_RED
#define PANIC_OFF DIM_YELLOW
#define PANIC_ON BRT_YELLOW

// standard colors across modules
#define H_SAVE_OFF_COLOR BRT_GREEN
#define H_SAVE_ON_COLOR WHITE
#define H_LOAD_OFF_COLOR DIM_GREEN
#define H_LOAD_ON_COLOR WHITE
#define H_COPY_OFF_COLOR BRT_CYAN
#define H_COPY_ON_COLOR WHITE
#define H_CLEAR_OFF_COLOR DIM_CYAN
#define H_CLEAR_ON_COLOR WHITE
#define H_MIDI_COLOR BRT_RED
#define H_MIDI_DIM_COLOR DIM_RED
#define H_PATTERN_OFF_COLOR DIM_SKY_BLUE
#define H_PATTERN_CURRENT_COLOR BRT_SKY_BLUE
#define H_PATTERN_NEXT_COLOR DK_GRAY
#define H_FILL_COLOR BRT_MAGENTA
#define H_FILL_DIM_COLOR DIM_MAGENTA
#define H_CLOCK_MEASURE_COLOR MED_GRAY
#define H_CLOCK_SIXTEENTH_COLOR WHITE
#define H_CLOCK_STEP_COLOR BRT_YELLOW



static const uint8_t PPQN = 24;
static const uint8_t PULSES_16TH = PPQN / 4;
const uint32_t PULSE_FACTOR = 60000000 / PPQN;
static const uint16_t NOT_RUNNING_MICROS_UPDATE_LONG = 5 * 1000 * 1000; // 5 seconds
static const uint16_t NOT_RUNNING_MICROS_UPDATE = 100 * 1000; // 100 millis


class Hachi {
  public:

    Hachi();
    void Init();
    void Loop();

    // respond to clock
    void Start();
    void Stop();
    void Pulse();
    int pulseCount = 0;

    // Called from Yaeltex code when a digital (button) event occurs.
    void DigitalEvent(uint16_t dInput, uint16_t pressed);

    // Called from Yaeltex code when an encoder event occurs.
    void EncoderEvent(uint8_t enc, int8_t value);

    // Implement IControlReceiver
    void GridEvent(uint8_t x, uint8_t y, uint8_t pressed);
    void ButtonEvent(uint8_t x, uint8_t y, uint8_t pressed);
    void KeyEvent(uint8_t x, uint8_t pressed);
    
    // void saveModuleMemory(IModule *module, byte *data);
    void saveModuleMemory(IModule *module, uint32_t offset, uint32_t size, byte *data);
    // void loadModuleMemory(IModule *module, byte *data);
    void loadModuleMemory(IModule *module, uint32_t offset, uint32_t size, byte *data);

    void Logo(void);
    void Logo2(void);

  private:

    bool hachiEnabled = true;

    uint8_t initialized = false;
    uint8_t color = BRT_RED;

    uint32_t lastMicrosLong;
    uint32_t lastMicros;
    uint16_t tempo;
    uint32_t pulseMicros;
    uint32_t lastPulseMicros;

    uint16_t pulseCounter;
    uint16_t sixteenthCounter;
    uint16_t measureCounter;

    IModule *modules[MODULE_COUNT];
    Display *moduleDisplays[MODULE_COUNT];
    uint32_t moduleMemoryOffsets[MODULE_COUNT];
    IModule *selectedModule = nullptr;
    uint8_t selectedModuleIndex = 0;

    bool running = false;
    bool internalClockRunning = false;
    bool debugging = false;

    bool SpecialEvent(uint16_t dInput, uint16_t pressed);

    int16_t moduleTypeSizes[MODULE_TYPE_COUNT] = {-1, -1, -1, -1, };

    void Draw(bool update);
    void DrawModuleButtons(bool update);
    void DrawButtons(bool update);
    void setTempo(uint16_t newTempo);
    void LogoH(uint8_t row, uint8_t column, uint8_t color);
    void LogoA(uint8_t row, uint8_t column, uint8_t color);
    void LogoC(uint8_t row, uint8_t column, uint8_t color);
    void LogoI(uint8_t row, uint8_t column, uint8_t color);

};


#endif
