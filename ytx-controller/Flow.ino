#include "headers/Flow.h"
#include "headers/Hachi.h"
#include "headers/Display.h"

Flow::Flow() {
}

void Flow::Init(uint8_t index, Display *display) {
  SERIALPRINTLN("Flow::Init idx=" + String(index) + ", memsize=" + sizeof(memory));
  this->index = index;
  this->display = display;
  // Draw(true);
}

void Flow::SetColors(uint8_t primaryColor, uint8_t primaryDimColor) {
  
}

uint32_t Flow::GetMemSize() {
  return sizeof(memory);
}

uint8_t Flow::GetIndex() {
  return index;
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

void Flow::ToggleTrack(uint8_t trackNumber) {

}


/***** UI display methods ************************************************************/

uint8_t Flow::getColor() {
  return BRT_PURPLE;
}

uint8_t Flow::getDimColor() {
  return DIM_PURPLE;
}


/***** Drawing methods ************************************************************/

void Flow::Draw(bool update) {
  display->FillGrid(ABS_BLACK);

  for (int k = 0; k < KEY_COLUMNS; k++) {
    display->setKey(k, ABS_BLACK);
  }

  for (int r = 4; r < 6; r++) {
    for (int c = 0; c < BUTTON_COLUMNS; c++) {
      display->setButton(r, c, ABS_BLACK);
    }
  }

  if (update) display->Update();
}

void Flow::DrawTracksEnabled(Display *useDisplay, uint8_t gridRow) {

}



/***** MIDI ************************************************************/


/***** Misc ************************************************************/

