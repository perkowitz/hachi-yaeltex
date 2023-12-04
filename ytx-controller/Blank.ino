#include "headers/Blank.h"
#include "headers/Hachi.h"
#include "headers/Display.h"

Blank::Blank() {
}

void Blank::Init(uint8_t index, Display *display) {
  SERIALPRINTLN("Blank::Init idx=" + String(index) + ", memsize=" + sizeof(memory));
  this->index = index;
  this->display = display;
}

void Blank::SetColors(uint8_t primaryColor, uint8_t primaryDimColor) {
  
}

uint32_t Blank::GetMemSize() {
  return sizeof(memory);
}

uint8_t Blank::GetIndex() {
  return index;
}

bool Blank::IsMuted() {
  return true;
}

void Blank::SetMuted(bool muted) {
}



/***** Clock ************************************************************/

void Blank::Start() {
}

void Blank::Stop() {
}

void Blank::Pulse(uint16_t measureCounter, uint16_t sixteenthCounter, uint16_t pulseCounter) {
}


/***** Events ************************************************************/

void Blank::GridEvent(uint8_t row, uint8_t column, uint8_t pressed) {
}

void Blank::ButtonEvent(uint8_t row, uint8_t column, uint8_t pressed) {
}

void Blank::KeyEvent(uint8_t column, uint8_t pressed) {

}

void Blank::ToggleTrack(uint8_t trackNumber) {

}


/***** UI display methods ************************************************************/

uint8_t Blank::getColor() {
  return ABS_BLACK;
}

uint8_t Blank::getDimColor() {
  return ABS_BLACK;
}


/***** Drawing methods ************************************************************/

void Blank::Draw(bool update) {
  display->FillModule(ABS_BLACK);

  if (update) display->Update();
}

void Blank::DrawTracksEnabled(Display *useDisplay, uint8_t gridRow) {
}


/***** MIDI ************************************************************/


/***** Misc ************************************************************/

