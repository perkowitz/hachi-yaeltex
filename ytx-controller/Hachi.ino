#include "headers/Hachi.h"

// 3 quake, 3 flow
static module_type moduleConfig[MODULE_COUNT] = { QUAKE, QUAKE, QUAKE, FLOW, FLOW, FLOW, BLANK, BREATH };
static u8 moduleColor[MODULE_COUNT] = { BRT_RED, BRT_ORANGE, BRT_BROWN, BRT_BLUE, BRT_CYAN, BRT_BLUE_GRAY, ABS_BLACK, WHITE };
static u8 moduleDimColor[MODULE_COUNT] = { DIM_RED, DIM_ORANGE, DIM_BROWN, DIM_BLUE, DIM_CYAN, DIM_BLUE_GRAY, ABS_BLACK, WHITE };
// static u8 moduleColor[MODULE_COUNT] = { BRT_RED, BRT_RED, BRT_RED, BRT_BLUE, BRT_BLUE, BRT_BLUE, ABS_BLACK, WHITE };
// static u8 moduleDimColor[MODULE_COUNT] = { DIM_RED, DIM_RED, DIM_RED, DIM_BLUE, DIM_BLUE, DIM_BLUE, ABS_BLACK, WHITE };

// 2 quake, 3 flow
// static module_type moduleConfig[MODULE_COUNT] = { QUAKE, QUAKE, BLANK, FLOW, FLOW, FLOW, BLANK, BREATH };
// static u8 moduleColor[MODULE_COUNT] = { BRT_RED, BRT_ORANGE, ABS_BLACK, BRT_BLUE, BRT_CYAN, BRT_BLUE_GRAY, ABS_BLACK, WHITE };
// static u8 moduleDimColor[MODULE_COUNT] = { DIM_RED, DIM_ORANGE, ABS_BLACK, DIM_BLUE, DIM_CYAN, DIM_BLUE_GRAY, ABS_BLACK, WHITE };

Hachi::Hachi() {
  // SERIALPRINTLN("Hachi Constructor");
  // initialized = 1;
  // hardware.Init();
  // lastMicros = micros();
  // lastPulseMicros = lastMicros;

  // // modules[0] = new Quake(&hardware);
  // // selectedModuleIndex = 0;
  // // selectedModule = modules[selectedModuleIndex];

  // pulseCounter = 0;
  // sixteenthCounter = 0;
  // measureCounter = 0;

  // setTempo(120);

}

void Hachi::Init() {
  // SERIALPRINTLN("Hachi::Init");
  if (initialized == 1) return;

  initialized = 1;
  hardware.Init();
  lastMicros = micros();
  lastPulseMicros = lastMicros;

  uint32_t freemem = FreeMemory();
  uint32_t lastFreemem = freemem;
  SERIALPRINTLN("Hachi::Init freemem=" + String(lastFreemem));

  uint32_t addressOffset = 0;
  Breath *breath = new Breath();
  freemem = FreeMemory(); 
  moduleTypeSizes[2] = lastFreemem - freemem;   // 2 is the size for breath
  SERIALPRINT("Module: Breath. ");
  SERIALPRINT("Uses storage=" + String(breath->GetStorageSize()) + ", mem=" + String(moduleTypeSizes[2]) + ". ");
  SERIALPRINTLN("System freemem=" + String(freemem));
  lastFreemem = freemem;

  for (int m = 0; m < MODULE_COUNT; m++) {
    moduleMemoryOffsets[m] = addressOffset;
    module_type moduleType = moduleConfig[m];
    freemem = FreeMemory();
    switch (moduleType) {
      case QUAKE:
        if (moduleTypeSizes[0] == -1 || moduleTypeSizes[0] < freemem) {   // 0 is the size for quake
          Quake *q = new Quake();
          modules[m] = q;
          breath->AddQuake(q);  // TODO: still needed?
          moduleTypeSizes[0] = lastFreemem - freemem;  // 0 is the size for quake
          SERIALPRINT("Module " + String(m) + ": Quake. ");
          SERIALPRINT("Uses storage=" + String(modules[m]->GetStorageSize()) + ", mem=" + String(moduleTypeSizes[0]) + ". ");
          SERIALPRINTLN("System freemem=" + String(freemem));
        } else {
          SERIALPRINTLN("*** Insufficient memory to create Quake module. Need " + String(moduleTypeSizes[0]) + " bytes.");
          modules[m] = new Blank();
        }
        break;
      case FLOW:
        if (moduleTypeSizes[1] == -1 || moduleTypeSizes[1] < freemem) {   // 1 is the size for flow
          modules[m] = new Flow();
          moduleTypeSizes[1] = lastFreemem - freemem;  // 1 is the size for flow
          SERIALPRINT("Module " + String(m) + ": Flow. ");
          SERIALPRINT("Uses storage=" + String(modules[m]->GetStorageSize()) + ", mem=" + String(moduleTypeSizes[1]) + ". ");
          SERIALPRINTLN("System freemem=" + String(freemem));
        } else {
          SERIALPRINTLN("*** Insufficient memory to create Flow module. Need " + String(moduleTypeSizes[1]) + " bytes.");
          modules[m] = new Blank();
          SERIALPRINT("Module " + String(m) + ": Blank. ");
          SERIALPRINT("Uses storage=" + String(modules[m]->GetStorageSize()) + ", mem=-. ");
          SERIALPRINTLN("System freemem=" + String(FreeMemory()));
        }
        break;
      case BREATH:
        modules[m] = breath;  // we only ever have one of these
        SERIALPRINTLN("Module " + String(m) + ": Breath. ");
        SERIALPRINT("Uses storage=" + String(modules[m]->GetStorageSize()) + ", mem=-. ");
        SERIALPRINTLN("System freemem=" + String(FreeMemory()));
        break;
      case BLANK:
        modules[m] = new Blank();
        SERIALPRINT("Module " + String(m) + ": Blank. ");
        SERIALPRINT("Uses storage=" + String(modules[m]->GetStorageSize()) + ", mem=-. ");
        SERIALPRINTLN("System freemem=" + String(FreeMemory()));
        break;
    }
    lastFreemem = freemem;

    breath->AddModule(modules[m]);
    Display *display = new Display();
    display->setHardware(&hardware);
    if (m == 0) {
      display->setEnabled(true);
    } else {
      display->setEnabled(false);
    }
    moduleDisplays[m] = display;
    modules[m]->Init(m, display);
    modules[m]->SetColors(moduleColor[m], moduleDimColor[m]);
    addressOffset += modules[m]->GetStorageSize();
  }
  selectedModuleIndex = 0;
  selectedModule = modules[selectedModuleIndex]; 
  SERIALPRINTLN("Loaded " + String(MODULE_COUNT) + " modules using " + String(addressOffset) + " bytes of storage.");
  SERIALPRINTLN("Free memory: " + String(FreeMemory()));

  pulseCounter = 0;
  sixteenthCounter = 0;
  measureCounter = 0;

  setTempo(120);
  hardware.ResetDrawing();
  // Draw(true);    // this doesn't actually work
}


/***** Clock ************************************/
void Hachi::Start() {
  lastPulseMicros = micros();
  pulseCount = 0;
  pulseCounter = 0;
  sixteenthCounter = 0;
  measureCounter = 0;
  for (int m = 0; m < MODULE_COUNT; m++) {
    modules[m]->Start();
  }
  running = true;
}

void Hachi::Stop() {
  lastPulseMicros = micros();
  running = false;
  internalClockRunning = false;
  pulseCount = 0;
  pulseCounter = 0;
  sixteenthCounter = 0;
  measureCounter = 0;
  for (int m = 0; m < MODULE_COUNT; m++) {
    modules[m]->Stop();
  }
}

void Hachi::Pulse() {
  // SERIALPRINTLN("Hachi::Pulse: meas=" + String(measureCounter) + ", 16=" + String(sixteenthCounter) + ", p=" + String(pulseCounter));
  for (int m = 0; m < MODULE_COUNT; m++) {
    modules[m]->Pulse(measureCounter, sixteenthCounter, pulseCounter);
  }

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
    // Draw(true);
  }

  pulseCounter++;
  if (pulseCounter % PULSES_16TH == 0) {
    sixteenthCounter = (sixteenthCounter + 1) % 16;
    if (sixteenthCounter == 0) {
      // SERIALPRINTLN("Measure: thisM=" + String(thisMicros));
      measureCounter++;
    }      
  }
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
    // SERIALPRINTLN("Hachi::Loop init=" + String(initialized));
    Init();
    // Logo();
  }

  uint32_t thisMicros = micros();

  if (running && internalClockRunning) {
    // TODO: instead of updating the lastMicros time to the current, just increment by the interval
    // this keeps the average time at the interval instead of drifting & jittering
    uint32_t pulsesNeeded = (thisMicros - lastPulseMicros) / pulseMicros;
    if (pulsesNeeded > 1) {
      SERIALPRINTLN("Pulses needed: " + String(pulsesNeeded));
    }
    for (uint8_t p = 0; p < pulsesNeeded; p++) {
      Pulse();
      lastPulseMicros = thisMicros;
      lastMicros = thisMicros;
    }
  } else if (running && pulseCount > 0) {
    while (pulseCount > 0) {
      Pulse();
      pulseCount--;
    }
    // pulseCount = 0;
    // pulseCount--;
    // Pulse();
  } 
  
  if (!running && thisMicros - lastMicrosLong > NOT_RUNNING_MICROS_UPDATE_LONG) {
    // what you do periodically when the sequencer isn't running
    lastMicrosLong = thisMicros;
    hardware.ResetDrawing();
    // Draw(true);
// this doesn't make it any more responsive when it's not running. I've tried
  // } else if (!running && thisMicros - lastMicros > NOT_RUNNING_MICROS_UPDATE) {
  //   // what you do periodically when the sequencer isn't running
  //   lastMicros = thisMicros;
  //   Draw(true);
  }

}

void Hachi::Draw(bool update) {
  // SERIALPRINTLN("Hachi:Draw");
  selectedModule->Draw(false);
  DrawModuleButtons(false);
  DrawButtons(false);

  if (update) hardware.Update();
}

void Hachi::DrawModuleButtons(bool update) {
  // SERIALPRINTLN("Hachi:DrawModuleButtons");
  for (int m = 0; m < MODULE_COUNT; m++) {
    // SERIALPRINTLN("    " + String(m));
    uint8_t color = ABS_BLACK;
    if (m == selectedModuleIndex) {
      if (modules[m] != nullptr) {
        color = modules[m]->getColor();
        // color = WHITE;
      }
    } else if (modules[m] != nullptr) {
      color = modules[m]->getDimColor();
    }
    // SERIALPRINTLN("    color=" + String(color));
    hardware.setButton(MODULE_SELECT_BUTTON_ROW, m, color);

    color = ABS_BLACK;
    if (modules[m] != nullptr) {
      if (modules[m]->IsMuted()) {
        // color = modules[m]->getDimColor();
        color = ABS_BLACK;
      } else {
        // color = modules[m]->getDimColor();
        color = DK_GRAY;
      }
    }    
    hardware.setButton(MODULE_MUTE_BUTTON_ROW, m, color);
  }  

  if (update) hardware.Update();
}

void Hachi::DrawButtons(bool update) {
  // SERIALPRINTLN("Hachi:DrawButtons");
  hardware.setByIndex(START_BUTTON, running ? START_RUNNING : START_NOT_RUNNING);
  hardware.setByIndex(PANIC_BUTTON, PANIC_OFF);
  hardware.setByIndex(GLOBAL_SETTINGS_BUTTON, BUTTON_OFF);
  hardware.setByIndex(PALETTE_BUTTON, BUTTON_OFF);

  if (update) hardware.Update();
}

void Hachi::DigitalEvent(uint16_t dInput, uint16_t pressed) {
  // first check if it's a special control
  // SERIALPRINTLN("DigitalEvent: idx=" + String(dInput) + ", state=" + String(pressed));
  bool found = SpecialEvent(dInput, pressed);
  if (found) return;

  Hardware::HachiDigital digital = hardware.fromDigital(dInput);
  // SERIALPRINTLN("    type=" + String(digital.type));
  switch (digital.type) {
    case GRID:
      // the entire grid belongs to modules
      selectedModule->GridEvent(digital.row, digital.column, pressed);
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
        if (running && internalClockRunning) {
          internalClockRunning = false;
          Stop();
        } else if (!running) {
          internalClockRunning = true;
          Start();
        }
        hardware.setByIndex(START_BUTTON, running ? START_RUNNING : START_NOT_RUNNING);
        Draw(true);
      }
      break;
    case PANIC_BUTTON:
      if (pressed) {
        Draw(true);
        hardware.setByIndex(PANIC_BUTTON, PANIC_ON);
      } else {
        hardware.setByIndex(PANIC_BUTTON, PANIC_OFF);
        DrawButtons(true);
      }
      break;
    case GLOBAL_SETTINGS_BUTTON:
      if (pressed) {
        hardware.ResetDrawing();
        hardware.ClearGrid();
        hardware.setByIndex(GLOBAL_SETTINGS_BUTTON, BUTTON_ON);
        Draw(true);
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
        hardware.ResetDrawing();
        Draw(true);
      }
      break;
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
    if (pressed) {
      moduleDisplays[selectedModuleIndex]->setEnabled(false);
      selectedModuleIndex = column;
      selectedModule = modules[selectedModuleIndex];
      moduleDisplays[selectedModuleIndex]->setEnabled(true);
      selectedModule->Draw(true);
      DrawModuleButtons(true);
    }
  } else if (row == MODULE_MUTE_BUTTON_ROW) {
    if (pressed) {
      modules[column]->SetMuted(!modules[column]->IsMuted());
      DrawModuleButtons(true);
    }
  } else {
    selectedModule->ButtonEvent(row, column, pressed);
  }
}

void Hachi::KeyEvent(uint8_t column, uint8_t pressed) {
  if (!initialized) return;

  selectedModule->KeyEvent(column, pressed);
}

void Hachi::EncoderEvent(uint8_t enc, int8_t value) {
  if (!initialized) return;

  selectedModule->EncoderEvent(enc, value);
}

// X bpm means a beat's duration is 1min/X, or 60sec/X
// or 60,000msec/X, or 60M micros/X; the duration of a beat is 60M/X micros
// For Y PPQN, the duration of a pulse is 60M/(X*Y) micros
// so the pulse factor P = 60M/ppqn, such that for tempo X, the number of micros per pulse is P/X
void Hachi::setTempo(uint16_t newTempo) {
  tempo = newTempo;
  pulseMicros = PULSE_FACTOR / tempo;
  // SERIALPRINTLN("Hachi::setTempo: tempo=" + String(tempo) + ", pulseMicros=" + String(pulseMicros));
}

void Hachi::saveModuleMemory(IModule *module, uint32_t offset, uint32_t size, byte *data) {
  uint32_t address = moduleMemoryOffsets[module->GetIndex()] + offset;
  // SERIALPRINTLN("Hachi::saveModuleMemory, m=" + String(module->GetIndex()) + ", offs=" + moduleMemoryOffsets[module->GetIndex()] + ", addr=" + String(address));
  memHost->saveHachiData(address, size, (byte*)data);
}

// void Hachi::loadModuleMemory(IModule *module, byte *data) {
//   // uint8_t index = module->GetIndex();
//   // SERIALPRINTLN("Hachi::loadModuleMemory, m=" + String(index) + ", offs=" + moduleMemoryOffsets[index]);
//   memHost->loadHachiData(moduleMemoryOffsets[module->GetIndex()], module->GetStorageSize(), (byte*)data);
// }

void Hachi::loadModuleMemory(IModule *module, uint32_t offset, uint32_t size, byte *data) {
  uint32_t address = moduleMemoryOffsets[module->GetIndex()] + offset;
  // SERIALPRINTLN("Hachi::loadModuleMemory, m=" + String(module->GetIndex()) + ", offs=" + moduleMemoryOffsets[module->GetIndex()] + ", addr=" + String(address));
  memHost->loadHachiData(address, size, (byte*)data);
}





void Hachi::Logo2() {
  SERIALPRINTLN("Hachi::Logo");
  uint8_t color1 = WHITE;
  uint8_t color2 = BRT_SKY_BLUE;
  uint8_t color3 = DIM_BLUE_GRAY;
  uint8_t delayMillis = 2000;

  hardware.ClearGrid();
  LogoH(2, 0, color1);
  hardware.Update();
  delay(delayMillis);

  LogoA(2, 4, color2);
  hardware.Update();
  delay(delayMillis);

  LogoC(2, 8, color3);
  hardware.Update();
  delay(delayMillis);

  LogoH(2, 11, color1);
  hardware.Update();
  delay(delayMillis);

  LogoI(2, 15, color3);
  hardware.Update();
  delay(delayMillis);

  // for (uint8_t i = 0; i < 16; i++) {
  //   hardware.setGrid(0, i, color3);
  //   hardware.setGrid(7, i, color3);
  // }
  // hardware.Update();

  SERIALPRINTLN("Hachi::Logo -- done");

}

void Hachi::Logo() {

  uint8_t color1 = WHITE;
  uint8_t color2 = BRT_SKY_BLUE;
  uint8_t color3 = DIM_BLUE_GRAY;
  uint8_t delayMillis = 2000;

  hardware.FillGrid(color3);
  LogoH(0, 6, color2);
  LogoA(0, 10, color1);
  LogoC(4, 3, color1);
  LogoH(4, 6, color2);
  LogoI(4, 10, color1);

  hardware.Update();
  delay(delayMillis);

  LogoH(0, 6, ABS_BLACK);
  LogoA(0, 10, ABS_BLACK);
  LogoC(4, 3, ABS_BLACK);
  LogoH(4, 6, ABS_BLACK);
  LogoI(4, 10, ABS_BLACK);

  hardware.Update();
  delay(delayMillis);

  for (uint8_t i = 0; i < 7; i++) {
    for (uint8_t j = 0; j < 8; j++) {
      hardware.setGrid(i, j, ABS_BLACK);
    }
  }

  hardware.Update();

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


