#include "headers/Flow.h"
#include "headers/Hachi.h"
#include "headers/Display.h"

Flow::Flow() {
}

void Flow::Init() {
  SERIALPRINTLN("Flow::Init");
  // Draw(true);
}

void Flow::SetDisplay(Display *display) {
  this->display = display;
}

bool Flow::IsMuted() {
  return true;
}

void Flow::SetMuted(bool muted) {
}


/***** Clock ************************************************************/

void Flow::Start() {
}

void Flow::Stop() {
}

void Flow::Pulse(uint16_t measureCounter, uint16_t sixteenthCounter, uint16_t pulseCounter) {
}


/***** Events ************************************************************/

void Flow::GridEvent(uint8_t row, uint8_t column, uint8_t pressed) {
}

void Flow::ButtonEvent(uint8_t row, uint8_t column, uint8_t pressed) {
}

void Flow::KeyEvent(uint8_t column, uint8_t pressed) {

}


/***** UI display methods ************************************************************/

uint8_t Flow::getColor() {
  return BRT_BLUE;
}

uint8_t Flow::getDimColor() {
  return DIM_BLUE;
}


/***** Drawing methods ************************************************************/

void Flow::Draw(bool update) {
  for (int row = 0; row < GRID_ROWS; row++) {
    for (int column = 0; column < GRID_COLUMNS; column++) {
      display->setGrid(row, column, ABS_BLACK);
    }
  }

  if (update) display->Update();
}



/***** MIDI ************************************************************/


/***** Misc ************************************************************/

