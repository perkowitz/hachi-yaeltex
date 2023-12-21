#include "headers/Breath.h"
#include "headers/Hachi.h"
#include "headers/Display.h"

Breath::Breath() {
}

void Breath::Init(uint8_t index, Display *display) {
  // SERIALPRINTLN("Breath::Init idx=" + String(index) + ", memsize=" + sizeof(memory) + ", freemem=" + String(FreeMemory()));
  this->index = index;
  this->display = display;
  // Draw(true);
}

void Breath::SetColors(uint8_t primaryColor, uint8_t primaryDimColor) {
  
}

uint32_t Breath::GetStorageSize() {
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

void Breath::AddModule(IModule *module) {
  if (moduleCount < B_MAX_MODULES) {
    modules[moduleCount] = module;
    moduleCount++;
  }
}

void Breath::AddQuake(Quake *quake) {
  if (quakeCount < B_MAX_QUAKES) {
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

  // if (row >= B_FIRST_QUAKE_ROW && row < B_FIRST_QUAKE_ROW + quakeCount) {
  //   // enable/disable tracks in quake modules
  //   if (pressed) {
  //     quakes[row - B_FIRST_QUAKE_ROW]->ToggleTrack(column);
  //     quakes[row - B_FIRST_QUAKE_ROW]->DrawTracksEnabled(display, row);
  //   }
  // }

  if (row >= B_FIRST_QUAKE_ROW && row < B_FIRST_QUAKE_ROW + moduleCount) {
    // enable/disable tracks
    if (pressed) {
      modules[row - B_FIRST_QUAKE_ROW]->ToggleTrack(column);
      modules[row - B_FIRST_QUAKE_ROW]->DrawTracksEnabled(display, row);
    }
  }

}

void Breath::ButtonEvent(uint8_t row, uint8_t column, uint8_t pressed) {
}

void Breath::KeyEvent(uint8_t column, uint8_t pressed) {

  uint8_t index = hardware.toDigital(KEY, 0, column);
  if (index == B_ALGORITHMIC_FILL_BUTTON) {
    if (pressed) {
      display->setByIndex(B_ALGORITHMIC_FILL_BUTTON, AUTOFILL_ON_COLOR);
      fillPattern = Fill::ChooseFillIndex();
      for (int i = 0; i < moduleCount; i++) {
        modules[i]->InstafillOn(fillPattern);
      }
    } else {
      fillPattern = -1;
      for (int i = 0; i < moduleCount; i++) {
        modules[i]->InstafillOff();
      }
      display->setByIndex(B_ALGORITHMIC_FILL_BUTTON, AUTOFILL_OFF_COLOR);
    }
  }
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

  display->FillModule(ABS_BLACK, false, true, true);

  int row = B_FIRST_QUAKE_ROW;
  for (int i = 0; i < B_MAX_MODULES; i++) {
    modules[i]->DrawTracksEnabled(display, row);
    row++;
  }

  // draw the clock row
  for (int column = 0; column < STEPS_PER_MEASURE; column++) {
    uint8_t color = ABS_BLACK;
    if (column == measureCounter % STEPS_PER_MEASURE) {
      color = MED_GRAY;
    } else if (column == sixteenthCounter) {
      color = ON_COLOR;
    } else if (column == Fill::MapFillPattern(fillPattern, sixteenthCounter)) {
      color = ACCENT_COLOR;
    }
    display->setGrid(CLOCK_ROW, column, color);
  }

  DrawButtons(false);

  if (update) display->Update();
}

void Breath::DrawButtons(bool update) {
  display->setByIndex(B_ALGORITHMIC_FILL_BUTTON, AUTOFILL_OFF_COLOR);
  display->setByIndex(B_TRACK_SHUFFLE_BUTTON, TRACK_SHUFFLE_OFF_COLOR);
}

void Breath::DrawTracksEnabled(Display *useDisplay, uint8_t gridRow) {
}

/***** performance features ************************************************************/

void Breath::InstafillOn(u8 index /*= CHOOSE_RANDOM_FILL*/) {
}

void Breath::InstafillOff() {
}


/***** MIDI ************************************************************/


/***** Misc ************************************************************/

