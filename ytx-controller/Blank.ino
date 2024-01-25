#include "headers/Blank.h"
#include "headers/Hachi.h"
#include "headers/Display.h"

Blank::Blank() {
}

void Blank::Init(uint8_t index, Display *display) {
  // SERIALPRINTLN("Blank::Init idx=" + String(index) + ", memsize=" + sizeof(memory) + ", freemem=" + String(FreeMemory()));
  this->index = index;
  this->display = display;
}

void Blank::SetColors(uint8_t primaryColor, uint8_t primaryDimColor) {
  
}

uint32_t Blank::GetStorageSize() {
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

void Blank::EncoderEvent(u8 encoder, u8 value) {
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
  display->FillModule(ABS_BLACK, true, true, true);

  if (update) display->Update();
}

void Blank::DrawTracksEnabled(Display *useDisplay, uint8_t gridRow) {
  for (int column = 0; column < GRID_COLUMNS; column++) {
    useDisplay->setGrid(gridRow, column, ABS_BLACK);
  }
}

/***** performance features ************************************************************/

void Blank::InstafillOn(u8 index /*= CHOOSE_RANDOM_FILL*/) {
}

void Blank::InstafillOff() {
}

void Blank::JumpOn(u8 step) {
}

void Blank::JumpOff() {
}

void Blank::SetScale(u8 tonic, bit_array_16 scale) {
}

void Blank::ClearScale() {
}


/***** MIDI ************************************************************/


/***** Misc ************************************************************/

