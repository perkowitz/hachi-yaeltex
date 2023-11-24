#include "headers/Breath.h"
#include "headers/Hachi.h"
#include "headers/Display.h"

Breath::Breath() {
}

void Breath::Init(uint8_t index, Display *display) {
  SERIALPRINTLN("Breath::Init idx=" + String(index) + ", memsize=" + sizeof(memory));
  this->index = index;
  this->display = display;
  // Draw(true);
}

void Breath::SetColors(uint8_t primaryColor, uint8_t primaryDimColor) {
  
}

uint32_t Breath::GetMemSize() {
  return sizeof(memory);
}

uint8_t Breath::GetIndex() {
  return index;
}

bool Breath::IsMuted() {
  return true;
}

void Breath::SetMuted(bool muted) {
}


/***** Clock ************************************************************/

void Breath::Start() {
}

void Breath::Stop() {
}

void Breath::Pulse(uint16_t measureCounter, uint16_t sixteenthCounter, uint16_t pulseCounter) {
}


/***** Events ************************************************************/

void Breath::GridEvent(uint8_t row, uint8_t column, uint8_t pressed) {
}

void Breath::ButtonEvent(uint8_t row, uint8_t column, uint8_t pressed) {
}

void Breath::KeyEvent(uint8_t column, uint8_t pressed) {

}


/***** UI display methods ************************************************************/

uint8_t Breath::getColor() {
  return WHITE;
}

uint8_t Breath::getDimColor() {
  return DK_GRAY;
}


/***** Drawing methods ************************************************************/

void Breath::Draw(bool update) {
  for (int row = 0; row < GRID_ROWS; row++) {
    for (int column = 0; column < GRID_COLUMNS; column++) {
      display->setGrid(row, column, ABS_BLACK);
    }
  }

  if (update) display->Update();
}



/***** MIDI ************************************************************/


/***** Misc ************************************************************/

