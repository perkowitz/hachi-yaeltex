#include "headers/Hachi.h"

Hachi::Hachi() {

}

void Hachi::Init() {
  // SERIALPRINTLN("Hachi::Init");
  initialized = 1;
  hardware.Init();
  lastMicros = micros();
  lastPulseMicros = lastMicros;

  pulseCounter = 0;
  sixteenthCounter = 0;
  measureCounter = 0;

  setTempo(120);

}

/* Hachi::Loop()
 * 
 * This is called on every clock loop (~200 micros). 
 * Based on the tempo, we need to calculate when to send a pulse (based on PPQN, pulseMicros).
 * Based on the pulses, need to know when a step happens.
 * However, if we've missed pulses due to some delay, we need to make up the missing pulses.
 */
void Hachi::Loop() {
  if (!initialized) {
    SERIALPRINTLN("Hachi::Loop init=" + String(initialized));
    Init();

    // eventually this will be a list of module interface objects etc etc
    selectedModule.Init();

    // Logo();
    Draw(true);
  }

  uint32_t thisMicros = micros();

  if (running) {
    uint32_t pulsesNeeded = (thisMicros - lastPulseMicros) / pulseMicros;
    if (pulsesNeeded > 1) {
      SERIALPRINTLN("Pulses needed: " + String(pulsesNeeded));
    }
    for (uint8_t p = 0; p < pulsesNeeded; p++) {
      // whatever you do on every pulse
      selectedModule.Pulse(measureCounter, sixteenthCounter, pulseCounter);

      // draw the clock button
      if (sixteenthCounter % 16 == 0) {
        // beginning of measure
        hardware.setByIndex(START_BUTTON, START_PULSE_MEASURE);
      } else if (sixteenthCounter % 4 == 0) {
        // each beat (1/4 note)
        hardware.setByIndex(START_BUTTON, START_PULSE);
      } else if (pulseCounter % PULSES_16TH == 0) {
        // each 16th note
        hardware.setByIndex(START_BUTTON, START_RUNNING);
      }
      if (pulseCounter % PULSES_16TH == 0) {
        Draw(true);
      }

      pulseCounter++;
      if (pulseCounter % PULSES_16TH == 0) {
        sixteenthCounter = (sixteenthCounter + 1) % 16;
        if (sixteenthCounter == 0) {
          // SERIALPRINTLN("Measure: thisM=" + String(thisMicros));
          measureCounter++;
        }      
      }
      lastPulseMicros = thisMicros;
      lastMicros = thisMicros;
    }
  } else if (thisMicros - lastMicros > NOT_RUNNING_MICROS_UPDATE) {
    // what you do periodically when the sequencer isn't running
    lastMicros = thisMicros;
    Draw(true);
  }



  // if (running && thisMicros - lastPulseMicros >= pulseMicros) {
  //   if (debugging) SERIALPRINTLN("Hachi::Loop running, micros=" + String(thisMicros));
  //   lastPulseMicros = thisMicros;
  //   // whatever you do on every pulse
  //   if (debugging) SERIALPRINTLN("    ...Loop: m=" + String(measureCounter) + ", 16=" + String(sixteenthCounter) + ", p=" + String(pulseCounter));
  //   selectedModule.Pulse(measureCounter, sixteenthCounter, pulseCounter);

  //   if (sixteenthCounter % 16 == 0) {
  //     // beginning of measure
  //     hardware.setByIndex(START_BUTTON, START_PULSE_MEASURE);
  //   } else if (sixteenthCounter % 4 == 0) {
  //     // each beat (1/4 note)
  //     hardware.setByIndex(START_BUTTON, START_PULSE);
  //   } else if (pulseCounter % PULSES_16TH == 0) {
  //     // each 16th note
  //     hardware.setByIndex(START_BUTTON, START_RUNNING);
  //   }

  //   if (pulseCounter % PULSES_16TH == 0) {
  //     Draw(true);
  //   }

  //   pulseCounter++;
  //   if (pulseCounter % PULSES_16TH == 0) {
  //     sixteenthCounter = (sixteenthCounter + 1) % 16;
  //   }
  //   if (sixteenthCounter == 0) {
  //     measureCounter++;
  //   }
  // }

}

void Hachi::Draw(bool update) {
  DrawButtons(false);
  selectedModule.Draw(false);

  if (update) hardware.Update();
}

void Hachi::DrawButtons(bool update) {
  hardware.setByIndex(START_BUTTON, running ? START_RUNNING : START_NOT_RUNNING);
  hardware.setByIndex(PANIC_BUTTON, PANIC_OFF);
  hardware.setByIndex(GLOBAL_SETTINGS_BUTTON, BUTTON_OFF);
  hardware.setByIndex(PALETTE_BUTTON, BUTTON_OFF);
  hardware.setByIndex(DEBUG_BUTTON, debugging ? BUTTON_ON : BUTTON_OFF);

  if (update) hardware.Update();
}

void Hachi::DigitalEvent(uint16_t dInput, uint16_t pressed) {
  // ignore button releases FOR NOW
  // pressed = !pressed;   // this appears to be flipped from what I'd expect

  // first check if it's a special control
  // SERIALPRINTLN("DigitalEvent: idx=" + String(dInput) + ", state=" + String(pressed));
  bool found = SpecialEvent(dInput, pressed);
  if (found) return;

  Hardware::HachiDigital digital = hardware.fromDigital(dInput);
  // SERIALPRINTLN("    type=" + String(digital.type));
  switch (digital.type) {
    case GRID:
      // the entire grid belongs to modules
      selectedModule.GridEvent(digital.row, digital.column, pressed);
      break;
    case BUTTON:
      ButtonEvent(digital.row, digital.column, pressed);
      break;
    case KEY:
      KeyEvent(digital.column, pressed);
      break;
    default:
      SERIALPRINTLN("DigitalEvent: no event type found for input=" + String(dInput));
  }

}

bool Hachi::SpecialEvent(uint16_t dInput, uint16_t pressed) {
  switch (dInput) {
    case START_BUTTON:
      if (pressed) {
        lastPulseMicros = micros();
        running = !running;
        pulseCounter = 0;
        sixteenthCounter = 0;
        measureCounter = 0;
        hardware.setByIndex(START_BUTTON, running ? START_RUNNING : START_NOT_RUNNING);
      }
      break;
    case PANIC_BUTTON:
      if (pressed) {
        hardware.setByIndex(PANIC_BUTTON, PANIC_ON);
        setTempo(100);
      } else {
        hardware.setByIndex(PANIC_BUTTON, PANIC_OFF);
        setTempo(120);
      }
      break;
    case GLOBAL_SETTINGS_BUTTON:
      if (pressed) {
        hardware.setByIndex(GLOBAL_SETTINGS_BUTTON, BUTTON_ON);
      } else {
        hardware.setByIndex(GLOBAL_SETTINGS_BUTTON, BUTTON_OFF);
      }
      break;
    case PALETTE_BUTTON:
      if (pressed) {
        hardware.setByIndex(PALETTE_BUTTON, BUTTON_ON);
        hardware.DrawPalette();
      } else {
        hardware.setByIndex(PALETTE_BUTTON, BUTTON_OFF);
        hardware.ClearGrid();
      }
      break;
    case DEBUG_BUTTON:
      if (pressed) {
        debugging = !debugging;
        DrawButtons(true);
      }
    default:
      return false;
  }

  return true;
}


void Hachi::GridEvent(uint8_t row, uint8_t column, uint8_t pressed) {
  if (!initialized) return;


}

void Hachi::ButtonEvent(uint8_t row, uint8_t column, uint8_t pressed) {
  if (!initialized) return;

  if (row == MODULE_SELECT_BUTTON_ROW) {
    // select a module 
  } else if (row == MODULE_MUTE_BUTTON_ROW) {
    // mute a module
  } else {
    selectedModule.ButtonEvent(row, column, pressed);
  }
}

void Hachi::KeyEvent(uint8_t column, uint8_t pressed) {
  if (!initialized) return;

}

void Hachi::EncoderEvent(uint8_t enc, int8_t value) {
    if (!initialized) return;

}

// X bpm means a beat's duration is 1min/X, or 60sec/X
// or 60,000msec/X, or 60M micros/X; the duration of a beat is 60M/X micros
// For Y PPQN, the duration of a pulse is 60M/(X*Y) micros
// so the pulse factor P = 60M/ppqn, such that for tempo X, the number of micros per pulse is P/X
void Hachi::setTempo(uint16_t newTempo) {
  tempo = newTempo;
  pulseMicros = PULSE_FACTOR / tempo;
  SERIALPRINTLN("Hachi::setTempo: tempo=" + String(tempo) + ", pulseMicros=" + String(pulseMicros));
}

void Hachi::savePatternData(uint8_t module, uint8_t pattern, uint16_t size, byte *data) {
  // for now, only store for module 0
  if (module != 0) return;

  uint32_t addressOffset = pattern * size;
  memHost->saveHachiData(addressOffset, size, (byte*)data);
}

void Hachi::loadPatternData(uint8_t module, uint8_t pattern, uint16_t size, byte *data) {
  // for now, only load for module 0
  if (module != 0) return;

  uint32_t addressOffset = pattern * size;
  memHost->loadHachiData(addressOffset, size, (byte*)data);
}



void Hachi::Logo() {
  // SERIALPRINTLN("Hachi::Logo");
  uint8_t color = DK_BLUE_GRAY;
  uint8_t delayMillis = 2000;

  LogoH(0, 14, color);
  delay(delayMillis);
  hardware.Update();
  LogoA(0, 18, WHITE);
  delay(delayMillis);
  hardware.Update();
  LogoC(8, 11, color);
  delay(delayMillis);
  hardware.Update();
  LogoH(8, 14, WHITE);
  delay(delayMillis);
  hardware.Update();
  LogoI(8, 18, color);
  delay(delayMillis);
  hardware.Update();

  for (uint8_t i = 0; i < 16; i++) {
    hardware.setGrid(0, i, color);
    hardware.setGrid(15, i, color);
  }
  hardware.Update();

  // SERIALPRINTLN("Hachi::Logo -- done");
  delay(10000);

}


void Hachi::LogoH(uint8_t row, uint8_t column, uint8_t color) {
  for (uint8_t i = 0; i < 4; i++) {
    hardware.setGrid(row + i, column, color);
    hardware.setGrid(row + i, column + 3, color);
    hardware.setGrid(row + 2, column + i, color);
  }
}

void Hachi::LogoA(uint8_t row, uint8_t column, uint8_t color) {
  for (uint8_t i = 0; i < 4; i++) {
    hardware.setGrid(row + i, column, color);
    hardware.setGrid(row + i, column + 3, color);
    hardware.setGrid(row, column + i, color);
    hardware.setGrid(row + 2, column + i, color);
  }
}

void Hachi::LogoC(uint8_t row, uint8_t column, uint8_t color) {
  for (uint8_t i = 0; i < 3; i++) {
    hardware.setGrid(row + i, column, color);
    hardware.setGrid(row, column + i, color);
    hardware.setGrid(row + 3, column + i, color);
  }
}

void Hachi::LogoI(uint8_t row, uint8_t column, uint8_t color) {
  for (uint8_t i = 0; i < 4; i++) {
    hardware.setGrid(row + i, column, color);
  }
}


