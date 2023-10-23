#ifndef HACHI_H
#define HACHI_H

#include <stdint.h>
#include "Arduino.h"
#include "FeedbackClass.h"
#include "IControlReceiver.h"
#include "IDisplaySender.h"
#include "Quake.h"

#define MODULE_SELECT_BUTTON_ROW 0
#define MODULE_MUTE_BUTTON_ROW 1

static const uint8_t PPQN = 24;
static const uint8_t PULSES_16TH = PPQN / 4;
const uint32_t PULSE_FACTOR = 60000000 / PPQN;
static const uint16_t NOT_RUNNING_MICROS_UPDATE = 1000000;

// class Hachi: public IControlReceiver, public IDisplaySender {
class Hachi: public IControlReceiver {
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
    
    // Implement IDisplaySender
    // void setDisplayReceiver(IDisplayReceiver& receiver);

    void savePatternData(uint8_t module, uint8_t pattern, uint16_t size, byte *data);
    void loadPatternData(uint8_t module, uint8_t pattern, uint16_t size, byte *data);

    void Logo(void);

  private:

    bool hachiEnabled = true;

    uint8_t initialized = false;
    uint8_t color = BRT_RED;

    uint32_t lastMicros;
    uint16_t tempo;
    uint32_t pulseMicros;
    uint32_t lastPulseMicros;

    uint16_t pulseCounter;
    uint16_t sixteenthCounter;
    uint16_t measureCounter;

    Quake selectedModule;

    bool running = false;
    bool internalClockRunning = false;
    bool debugging = false;

    bool SpecialEvent(uint16_t dInput, uint16_t pressed);


    void Draw(bool update);
    void DrawButtons(bool update);
    void setTempo(uint16_t newTempo);
    void LogoH(uint8_t row, uint8_t column, uint8_t color);
    void LogoA(uint8_t row, uint8_t column, uint8_t color);
    void LogoC(uint8_t row, uint8_t column, uint8_t color);
    void LogoI(uint8_t row, uint8_t column, uint8_t color);

};

// important buttons mapped to indices
#define START_BUTTON 152
#define PANIC_BUTTON 153
#define PALETTE_BUTTON 154
#define GLOBAL_SETTINGS_BUTTON 155
#define DEBUG_BUTTON 156

// colors
#define BUTTON_OFF DK_GRAY
#define BUTTON_ON WHITE
#define START_NOT_RUNNING DIM_RED
#define START_RUNNING DIM_RED
#define START_PULSE_MEASURE BRT_YELLOW
#define START_PULSE BRT_RED
#define PANIC_OFF DIM_YELLOW
#define PANIC_ON BRT_YELLOW
#define SAVE_OFF_COLOR BRT_GREEN
#define SAVE_ON_COLOR WHITE
#define LOAD_OFF_COLOR DIM_GREEN
#define LOAD_ON_COLOR WHITE



#endif
