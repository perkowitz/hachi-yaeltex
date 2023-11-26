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

void Breath::AddQuake(Quake *quake) {
  if (quakeCount < BREATH_MAX_QUAKES) {
    quakes[quakeCount] = quake;
    quakeCount++;
  }
}


/***** Clock ************************************************************/

void Breath::Start() {
}

void Breath::Stop() {
}

void Breath::Pulse(uint16_t measureCounter, uint16_t sixteenthCounter, uint16_t pulseCounter) {
  if (pulseCounter % PULSES_16TH == 0) {
    // SERIALPRINTLN("Breath::Pulse 16th, m=" + String(measureCounter) + ", 16=" + String(sixteenthCounter));
    this->sixteenthCounter = sixteenthCounter;
    this->measureCounter = measureCounter;
    Draw(true);
  }
}


/***** Events ************************************************************/

void Breath::GridEvent(uint8_t row, uint8_t column, uint8_t pressed) {

  if (row >= BREATH_FIRST_QUAKE_ROW && row < BREATH_FIRST_QUAKE_ROW + quakeCount) {
    // enable/disable tracks in quake modules
    if (pressed) {
      quakes[row - BREATH_FIRST_QUAKE_ROW]->ToggleTrack(column);
      quakes[row - BREATH_FIRST_QUAKE_ROW]->DrawTracksEnabled(display, row);
    }
  }

}

void Breath::ButtonEvent(uint8_t row, uint8_t column, uint8_t pressed) {
}

void Breath::KeyEvent(uint8_t column, uint8_t pressed) {

}

void Breath::ToggleTrack(uint8_t trackNumber) {

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
  display->FillGrid(ABS_BLACK);

  for (int k = 0; k < KEY_COLUMNS; k++) {
    display->setKey(k, ABS_BLACK);
  }

  for (int r = 4; r < 6; r++) {
    for (int c = 0; c < BUTTON_COLUMNS; c++) {
      display->setButton(r, c, ABS_BLACK);
    }
  }

  int row = BREATH_FIRST_QUAKE_ROW;
  for (int i = 0; i < quakeCount; i++) {
    quakes[i]->DrawTracksEnabled(display, row);
    row++;
  }

  // draw the clock row
  for (int column = 0; column < STEPS_PER_MEASURE; column++) {
    uint8_t color = ABS_BLACK;
    if (column == measureCounter % STEPS_PER_MEASURE) {
      color = OFF_COLOR;
    // } else if (column == currentStep) {
    //   color = ACCENT_COLOR;
    } else if (column == sixteenthCounter) {
      color = ON_COLOR;
    }
    display->setGrid(CLOCK_ROW, column, color);
  }

  DrawButtons(false);

  if (update) display->Update();
}

void Breath::DrawButtons(bool update) {
  display->setByIndex(BREATH_ALGORITHMIC_FILL_BUTTON, AUTOFILL_OFF_COLOR);
  display->setByIndex(BREATH_TRACK_SHUFFLE_BUTTON, TRACK_SHUFFLE_OFF_COLOR);
}

void Breath::DrawTracksEnabled(Display *useDisplay, uint8_t gridRow) {
}



/***** MIDI ************************************************************/


/***** Misc ************************************************************/

