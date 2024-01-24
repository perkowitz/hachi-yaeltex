#include "headers/Breath.h"
#include "headers/Hachi.h"
#include "headers/Display.h"

Breath::Breath() {
}

void Breath::Init(uint8_t index, Display *display) {
  // SERIALPRINTLN("Breath::Init idx=" + String(index) + ", memsize=" + sizeof(memory) + ", freemem=" + String(FreeMemory()));
  this->index = index;
  this->display = display;
  Load();
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
  return muted;
}

void Breath::SetMuted(bool muted) {
  this->muted = muted;
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
  BassNoteOff();
  ChordNotesOff();
  currentStep = 0;
  currentChordStep = lastChordStep;  // on first pulse it will advance to first step
  currentRoot = memory.patterns[currentChordPattern].steps[currentChordStep].root;
  currentChord = chords[memory.patterns[currentChordPattern].steps[currentChordStep].chordIndex];
}

void Breath::Stop() {
  BassNoteOff();
  ChordNotesOff();
  Save();
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
        ChordNotesOff();
        BassNoteOff();
        currentChordStep = (currentChordStep + 1) % 16;
        if (currentChordStep < firstChordStep || currentChordStep > lastChordStep) {
          currentChordStep = firstChordStep;
        }
        currentRoot = memory.patterns[currentChordPattern].steps[currentChordStep].root;
        currentChord = chords[memory.patterns[currentChordPattern].steps[currentChordStep].chordIndex];
      } else {
        currentStep = (currentStep + 1) % 16;
      }
    }

    if (BitArray16_Get(memory.patterns[currentChordPattern].chordSequence, sixteenthCounter)) {
      PlayChord(currentRoot, currentChord);
    }

    if (BitArray16_Get(memory.patterns[currentChordPattern].bassSequence, sixteenthCounter)) {
      PlayBass(currentRoot);
    }

    Draw(true);
  }
}


/***** Events ************************************************************/

void Breath::GridEvent(uint8_t row, uint8_t column, uint8_t pressed) {

  if (chordMode) {
    ChordModeGridEvent(row, column, pressed);
    return;
  }

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

void Breath::ChordModeGridEvent(uint8_t row, uint8_t column, uint8_t pressed) {
  if (!chordMode) return;

  uint8_t index = hardware.toDigital(GRID, row, column);
  if (index == B_CHORD_ENABLE_BUTTON) {
    if (pressed) {
      chordEnabled = !chordEnabled;
      if (!chordEnabled) {
        ChordNotesOff();
      }
      DrawChordMode(true);
    }
  } else if (index == B_BASS_ENABLE_BUTTON) {
    if (pressed) {
      bassEnabled = !bassEnabled;
      if (!bassEnabled) {
        BassNoteOff();
      }
      DrawChordMode(true);
    }
  } else if (row == B_SCALE_ROW && column < B_SCALE_COUNT) {
    if (pressed) {
      // change the current scale when pressed, so we can display it on the keys
      editingScale = true;
      currentScaleIndex = column;
      switch (column) {
        case 0:
          currentScale = MAJOR_SCALE;
          break;
        case 1:
          currentScale = MINOR_SCALE;
          break;
        case 2:
          currentScale = OCTATONIC_SCALE;
          break;
        case 3:
          currentScale = WTF_SCALE;
          break;          
      }
      DrawScale(true);
    } else {
      // but don't send the scale to modules until button is released (so tonic can be changed first)
      editingScale = false;
      if (currentTonic != -1) {
        for (int i = 0; i < moduleCount; i++) {
          modules[i]->SetScale(currentTonic, currentScale);
        }
      }
      DrawChord(currentRoot, currentChord, true);
    }
  } else if (row == B_CHORD_ROW && column < B_CHORD_COUNT) {
    if (pressed) {
      memory.patterns[currentChordPattern].steps[selectedChordStep].chordIndex = column;
      editingStep = true;
      DrawChord(memory.patterns[currentChordPattern].steps[selectedChordStep].root, chords[column], true);
      Save();
    } else {
      editingStep = false;
    }
  } else if (row == B_STEP_PLAY_ROW) {
    if (pressed) {
      if (selectingPlayStep) {
        if (column >= firstChordStep) {
          lastChordStep = column;
        } else {
          lastChordStep = firstChordStep;
          firstChordStep = column;
        }
        selectingPlayStep = false;
      } else {
        firstChordStep = lastChordStep = column;
        selectingPlayStep = true;
      }
    } else {
      selectingPlayStep = false;
    }
    DrawChordMode(true);
  } else if (row == B_STEP_SELECT_ROW) {
    if (pressed) {
      editingStep = true;
      selectedChordStep = column;
    } else {
      editingStep = false;
    }
    DrawChordMode(true);
  } else if (row == B_CHORD_SEQ_ROW) {
    if (pressed) {
      memory.patterns[currentChordPattern].chordSequence = BitArray16_Toggle(memory.patterns[currentChordPattern].chordSequence, column);
      DrawChordMode(true);
      Save();
    }
  } else if (row == B_BASS_SEQ_ROW) {
    if (pressed) {
      memory.patterns[currentChordPattern].bassSequence = BitArray16_Toggle(memory.patterns[currentChordPattern].bassSequence, column);
      DrawChordMode(true);
      Save();
    }
  }

}

void Breath::ButtonEvent(uint8_t row, uint8_t column, uint8_t pressed) {

  uint8_t index = hardware.toDigital(BUTTON, row, column);
  // SERIALPRINTLN("Breath::ButtonEvent: idx=" + String(index));
  if (index == B_CHORD_MODE_BUTTON) {
    if (pressed) {
      chordMode = !chordMode;
      Draw(true);
    }
  }
}

void Breath::KeyEvent(uint8_t column, uint8_t pressed) {

  if (chordMode) {
    if (editingScale) {
      if (pressed) {
        currentTonic = column;
        DrawScale(true);
      }
    } else {
      if (pressed) {
        memory.patterns[currentChordPattern].steps[selectedChordStep].root = column;
        editingStep = true;
        DrawChord(memory.patterns[currentChordPattern].steps[selectedChordStep].root, chords[memory.patterns[currentChordPattern].steps[selectedChordStep].chordIndex], true);
        Save();
      } else {
        editingStep = false;
      }
    }
    return;
  }

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

  if (chordMode) {
    DrawChordMode(false);
  } else {
    DrawModuleTracks(false);
  }
  
  DrawButtons(false);

  if (update) display->Update();
}

void Breath::DrawModuleTracks(bool update) {
  if (chordMode) return;

  display->FillModule(ABS_BLACK, false, true, true);
  display->DrawClock(CLOCK_ROW, measureCounter, sixteenthCounter, currentStep);

  int row = B_FIRST_MODULE_ROW;
  for (int i = 0; i < B_MAX_MODULES; i++) {
    modules[i]->DrawTracksEnabled(display, row);
    row++;
  }

  display->setByIndex(B_ALGORITHMIC_FILL_BUTTON, AUTOFILL_OFF_COLOR);
  display->setByIndex(B_LAST_FILL_BUTTON, AUTOFILL_OFF_COLOR);

  if (update) display->Update();
}

void Breath::DrawChordMode(bool update) {
  if (!chordMode) return;

  display->DrawClock(CLOCK_ROW, measureCounter, sixteenthCounter, currentStep);

  for (int row = 1; row < GRID_ROWS; row++) {
    for (int column = 0; column < GRID_COLUMNS; column++) {
      u8 color = ABS_BLACK;
      if (row == B_SCALE_ROW && column < B_SCALE_COUNT) {
        color = column == currentScaleIndex ? SCALE_ON_COLOR : SCALE_OFF_COLOR;
      } else if (row == B_CHORD_ROW && column < B_CHORD_COUNT && editingStep) {
        color = column == memory.patterns[currentChordPattern].steps[selectedChordStep].chordIndex ? CHORD_ON_COLOR : GetChordColor(chords[column]);
      } else if (row == B_CHORD_ROW && column < B_CHORD_COUNT) {
        color = column == memory.patterns[currentChordPattern].steps[currentChordStep].chordIndex ? CHORD_ON_COLOR : GetChordColor(chords[column]);
      } else if (row == B_STEP_PLAY_ROW) {
        if (column == currentChordStep) {
          color = STEP_PLAY_ON_COLOR;
        } else if (column >= firstChordStep && column <= lastChordStep) {
          color = STEP_PLAY_NEXT_COLOR;
        } else {
          color = STEP_PLAY_OFF_COLOR;
        }
      } else if (row == B_STEP_SELECT_ROW) {
        color = column == selectedChordStep ? STEP_SELECT_ON_COLOR : STEP_SELECT_OFF_COLOR;
      } else if (row == B_CHORD_SEQ_ROW) {
        color = BitArray16_Get(memory.patterns[currentChordPattern].chordSequence, column) ? CHORD_SEQ_ON_COLOR : CHORD_SEQ_OFF_COLOR;
      } else if (row == B_BASS_SEQ_ROW) {
        color = BitArray16_Get(memory.patterns[currentChordPattern].bassSequence, column) ? BASS_SEQ_ON_COLOR : BASS_SEQ_OFF_COLOR;
      }

      display->setGrid(row, column, color);
    }
  }

  if (editingScale) {
    DrawScale(false);
  } else if (editingStep) {
    DrawChord(memory.patterns[currentChordPattern].steps[selectedChordStep].root, chords[memory.patterns[currentChordPattern].steps[selectedChordStep].chordIndex], false);
  } else {
    DrawChord(currentRoot, currentChord, false);
  }

  u8 color = chordEnabled ? CHORD_SEQ_ACCENT_COLOR : CHORD_SEQ_ON_COLOR;
  display->setByIndex(B_CHORD_ENABLE_BUTTON, color);
  color = bassEnabled ? BASS_SEQ_ACCENT_COLOR : BASS_SEQ_ON_COLOR;
  display->setByIndex(B_BASS_ENABLE_BUTTON, color);
  

  if (update) display->Update();
}

void Breath::DrawScale(bool update) {
  if (!chordMode) return;

  for (int column = 0; column < KEY_COLUMNS; column++) {
    u8 color = KEYS_OFF_COLOR;
    if (column == currentTonic) {
      color = KEYS_TONIC_COLOR;
    } else if (currentScaleIndex != -1 && BitArray16_Get(currentScale, (column - currentTonic + 12) % 12)) {
      color = KEYS_SCALE_COLOR;
    }
    display->setKey(column, color);
  }

  if (update) display->Update();
}

void Breath::DrawChord(int root, bit_array_16 chord, bool update) {
  if (!chordMode) return;

  for (int column = 0; column < KEY_COLUMNS; column++) {
    u8 color = KEYS_OFF_COLOR;
    if (column == root) {
      color = KEYS_ROOT_COLOR;
    } else if (BitArray16_Get(chord, (column - root + 12) % 12)) {
      color = GetChordColor(chord);
    }
    display->setKey(column, color);
  }

  if (update) display->Update();
}

void Breath::DrawButtons(bool update) {
  display->setByIndex(B_CHORD_MODE_BUTTON, chordMode ? ACCENT_COLOR : ACCENT_DIM_COLOR);

  if (update) display->Update();
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

void Breath::SetScale(u8 tonic, bit_array_16 scale) {
}

void Breath::ClearScale() {
}

/***** MIDI ************************************************************/

void Breath::ChordNotesOff() {
  for (int i = 0; i < MAX_CHORD_NOTES; i++) {
    if (playingChordNotes[i] >= 0) {
      hardware.SendMidiNote(chordMidiChannel, playingChordNotes[i], 0);
      playingChordNotes[i] = -1;
    }
  }
}

void Breath::PlayChord(u8 root, bit_array_16 chord) {
  if (muted || !chordEnabled) return;

  chordMidiChannel = memory.midiChannel;
  ChordNotesOff();

  int i = 0;
  for (int n = 0; n < 12; n++) {
    if (BitArray16_Get(chord, n)) {
      u8 note = chordPlayOctave * 12 + root + n;
      hardware.SendMidiNote(chordMidiChannel, chordPlayOctave * 12 + root + n, chordPlayVelocity);
      if (i < MAX_CHORD_NOTES) {
        playingChordNotes[i] = note;
        i++;
      } else {
        SERIALPRINTLN("Error: number of chord notes exceeded allowable limit of " + String(MAX_CHORD_NOTES));
      }
    }
  }
}

void Breath::BassNoteOff() {
  if (playingBassNote != -1) {
    hardware.SendMidiNote(bassMidiChannel, playingBassNote, 0);
  }
}

void Breath::PlayBass(u8 root) {
  if (muted || !bassEnabled) return;

  bassMidiChannel = (memory.midiChannel + 1) % 16;
  BassNoteOff();

  u8 note = bassPlayOctave * 12 + root;
  hardware.SendMidiNote(bassMidiChannel, note, bassPlayVelocity);
  playingBassNote = note;
}


/***** Misc ************************************************************/

u8 Breath::GetChordColor(bit_array_16 chord) {
  if (BitArray16_Get(chord, MAJOR_THIRD)) {
    return CHORD_MAJOR_COLOR;
  } else if (BitArray16_Get(chord, MINOR_THIRD)) {
    return CHORD_MINOR_COLOR;
  }
  return CHORD_OTHER_COLOR;
}

void Breath::Save() {
  hachi.saveModuleMemory(this, 0, sizeof(memory), (byte*)&memory);
}

void Breath::Load() {
  hachi.loadModuleMemory(this, 0, sizeof(memory), (byte*)&memory);
}

