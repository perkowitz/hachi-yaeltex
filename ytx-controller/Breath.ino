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
    // SERIALPRINTLN("    stut=" + String(stuttering) + ", fp=" + String(fillPattern) + ", step=" + String(currentStep));
    this->sixteenthCounter = sixteenthCounter;
    this->measureCounter = measureCounter;
    if (!stuttering) {
      if (fillPattern >= 0) {
        currentStep = Fill::MapFillPattern(fillPattern, sixteenthCounter);
      } else if (sixteenthCounter == 0) {
        currentStep = 0;
      } else {
        currentStep = (currentStep + 1) % 16;
      }
    }
    Draw(true);
  }
}


/***** Events ************************************************************/

void Breath::GridEvent(uint8_t row, uint8_t column, uint8_t pressed) {

  if (row >= B_FIRST_MODULE_ROW && row < B_FIRST_MODULE_ROW + moduleCount) {
    // enable/disable tracks
    if (pressed) {
      modules[row - B_FIRST_MODULE_ROW]->ToggleTrack(column);
      modules[row - B_FIRST_MODULE_ROW]->DrawTracksEnabled(display, row);
    }
  } else if (row == CLOCK_ROW) {
    if (pressed) {
      stuttering = true;
      currentStep = column;
      for (int i = 0; i < moduleCount; i++) {
        modules[i]->JumpOn(column);
      }
    } else {
      for (int i = 0; i < moduleCount; i++) {
        modules[i]->JumpOff();
      }
      stuttering = false;
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
      lastFill = fillPattern;
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
  } else if (index == B_LAST_FILL_BUTTON) {
    if (pressed) {
      display->setByIndex(B_LAST_FILL_BUTTON, AUTOFILL_ON_COLOR);
      fillPattern = lastFill;
      for (int i = 0; i < moduleCount; i++) {
        modules[i]->InstafillOn(fillPattern);
      }
    } else {
      fillPattern = -1;
      for (int i = 0; i < moduleCount; i++) {
        modules[i]->InstafillOff();
      }
      display->setByIndex(B_LAST_FILL_BUTTON, AUTOFILL_OFF_COLOR);
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

  int row = B_FIRST_MODULE_ROW;
  for (int i = 0; i < B_MAX_MODULES; i++) {
    modules[i]->DrawTracksEnabled(display, row);
    row++;
  }

  display->DrawClock(CLOCK_ROW, measureCounter, sixteenthCounter, currentStep);

  DrawButtons(false);

  if (update) display->Update();
}

void Breath::DrawButtons(bool update) {
  display->setByIndex(B_ALGORITHMIC_FILL_BUTTON, AUTOFILL_OFF_COLOR);
  display->setByIndex(B_LAST_FILL_BUTTON, AUTOFILL_OFF_COLOR);
}

void Breath::DrawTracksEnabled(Display *useDisplay, uint8_t gridRow) {
}

/***** performance features ************************************************************/

void Breath::InstafillOn(u8 index /*= CHOOSE_RANDOM_FILL*/) {
}

void Breath::InstafillOff() {
}

void Breath::JumpOn(u8 step) {
}

void Breath::JumpOff() {
}


/***** MIDI ************************************************************/


/***** Misc ************************************************************/

