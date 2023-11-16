#include "headers/Quake.h"
#include "headers/Hachi.h"
#include "headers/Display.h"

Quake::Quake() {
}

void Quake::Init(uint8_t index, Display *display) {
  SERIALPRINTLN("Quake::Init idx=" + String(index));
  this->index = index;
  this->display = display;
  Reset();
  for (int p = 0; p < NUM_PATTERNS; p++) {
    ResetPattern(p);
  }
  // Draw(true);
}

// void Quake::SetDisplay(Display *display) {
//   this->display = display;
// }

uint32_t Quake::GetMemSize() {
  return sizeof(memory);
}

uint8_t Quake::GetIndex() {
  return index;
}

bool Quake::IsMuted() {
  return muted;
}

void Quake::SetMuted(bool muted) {
  this->muted = muted;
}


/***** Clock ************************************************************/

void Quake::Start() {
  currentMeasure = currentStep = 0;
  ClearSoundingNotes();
  // Draw(true);
}

void Quake::Stop() {
  currentMeasure = currentStep = 0;
  AllNotesOff();
  // Draw(true);
}

void Quake::Pulse(uint16_t measureCounter, uint16_t sixteenthCounter, uint16_t pulseCounter) {
  if (pulseCounter % PULSES_16TH == 0) {
    // SERIALPRINTLN("Quake::Pulse 16th, m=" + String(measureCounter) + ", 16=" + String(sixteenthCounter));
    currentStep = (currentStep + 1) % STEPS_PER_MEASURE;
    if (sixteenthCounter == 0 && memory.measureReset == 1) {
      currentStep = 0;
      NextMeasure(measureCounter);
    }
    SendNotes();
    Draw(true);
  }
}


/***** Events ************************************************************/

void Quake::GridEvent(uint8_t row, uint8_t column, uint8_t pressed) {
  if (!pressed) return; // not using long holds right now

  if (row == TRACK_ENABLED_ROW) {
    // enable/disable tracks
    memory.trackEnabled[column] = !memory.trackEnabled[column];
    DrawTracks(true);
  } else if (row == TRACK_SELECT_ROW) {
    // select track for editing
    selectedTrack = column;
    DrawTracks(false);
    DrawMeasures(true);
  } else if (row == MODE_ROW && column >= MEASURE_MODE_MIN_COLUMN && column <= MEASURE_MODE_MAX_COLUMN) {
    // set the playback mode for measures
    memory.measureMode = column - MEASURE_MODE_MIN_COLUMN;
    Draw(true);
  } else if (row == MODE_ROW && column >= AUTOFILL_INTERVAL_MIN_COLUMN && column <= AUTOFILL_INTERVAL_MAX_COLUMN) {
    // set the autofill interval
    if (memory.autofillIntervalSetting == column - AUTOFILL_INTERVAL_MIN_COLUMN) {
      memory.autofillIntervalSetting = -1;
    } else {
      memory.autofillIntervalSetting = column - AUTOFILL_INTERVAL_MIN_COLUMN;
    }
    DrawButtons(true);
  } else if (row == MODE_ROW && column == AUTOFILL_TYPE_COLUMN) {
    if (memory.autofillType == PATTERN) {
      memory.autofillType = ALGORITHMIC;
    } else if (memory.autofillType == ALGORITHMIC) {
      memory.autofillType = BOTH;
    } else {
      memory.autofillType = PATTERN;
    }
  } else if (row >= FIRST_MEASURES_ROW && row < FIRST_MEASURES_ROW + MEASURES_PER_PATTERN) {
    // edit steps in the pattern
    uint8_t measure = row - FIRST_MEASURES_ROW;
    selectedStep = column;
    selectedMeasure = measure;
    int8_t v = currentPattern->tracks[selectedTrack].measures[measure].steps[column];
    if (v == 0) {
      v = INITIAL_VELOCITY;
    } else {
      v *= -1;
    }
    currentPattern->tracks[selectedTrack].measures[measure].steps[column] = v;
    DrawMeasures(true);
  }

}

void Quake::ButtonEvent(uint8_t row, uint8_t column, uint8_t pressed) {
  // SERIALPRINTLN("Quake::ButtonEvent row=" + String(row) + ", col=" + String(column) + ", pr=" + String(pressed));

  uint8_t index = hardware.toDigital(BUTTON, row, column);
  if (index == QUAKE_SAVE_BUTTON) {
    if (pressed) {
      display->setByIndex(QUAKE_SAVE_BUTTON, SAVE_ON_COLOR);
      hachi.saveModuleMemory(this, (byte*)&memory);
    } else {
      display->setByIndex(QUAKE_SAVE_BUTTON, SAVE_OFF_COLOR);
    }
  } else if (index == QUAKE_LOAD_BUTTON) {
    if (pressed) {
      display->setByIndex(QUAKE_LOAD_BUTTON, LOAD_ON_COLOR);
      hachi.loadModuleMemory(this, (byte*)&memory);
      DrawMeasures(true);
    } else {
      display->setByIndex(QUAKE_LOAD_BUTTON, LOAD_OFF_COLOR);
    }
  } else if (index == QUAKE_TRACK_SHUFFLE_BUTTON) {
    if (pressed) {
      display->setByIndex(QUAKE_TRACK_SHUFFLE_BUTTON, TRACK_SHUFFLE_ON_COLOR);
      display->Update();
      int r = random(TRACKS_PER_PATTERN - 1) + 1;
      for (int t = 0; t < TRACKS_PER_PATTERN; t++) {
        trackMap[t] = (t + r) % TRACKS_PER_PATTERN;
      }
      AllNotesOff();
    } else {
      for (int t = 0; t < TRACKS_PER_PATTERN; t++) {
        trackMap[t] = t;
      }
      AllNotesOff();
      display->setByIndex(QUAKE_TRACK_SHUFFLE_BUTTON, TRACK_SHUFFLE_OFF_COLOR);
      display->Update();
    }
  } else if (index == QUAKE_PATTERN_SHUFFLE_BUTTON) {
    if (pressed) {
      display->setByIndex(QUAKE_PATTERN_SHUFFLE_BUTTON, TRACK_SHUFFLE_ON_COLOR);
      display->Update();
      for (int t = 0; t < STEPS_PER_MEASURE; t++) {
        patternMap[t] = random(STEPS_PER_MEASURE);
      }
      AllNotesOff();
    } else {
      for (int t = 0; t < STEPS_PER_MEASURE; t++) {
        patternMap[t] = t;
      }
      AllNotesOff();
      display->setByIndex(QUAKE_PATTERN_SHUFFLE_BUTTON, TRACK_SHUFFLE_OFF_COLOR);
      display->Update();
    }
  } else if (index == QUAKE_CLEAR_BUTTON) {
    if (pressed) {
      display->setByIndex(QUAKE_CLEAR_BUTTON, ON_COLOR);
      ResetSelectedTrack();
      DrawMeasures(true);
    } else {
      display->setByIndex(QUAKE_CLEAR_BUTTON, PRIMARY_DIM_COLOR);
    }
  } else if (index == QUAKE_PATTERN_FILL_BUTTON) {
    if (pressed) {
      display->setByIndex(QUAKE_PATTERN_FILL_BUTTON, AUTOFILL_ON_COLOR);
      DrawButtons(true);
    } else {
      display->setByIndex(QUAKE_PATTERN_FILL_BUTTON, AUTOFILL_OFF_COLOR);
      DrawButtons(true);
    }
  } else if (index == QUAKE_ALGORITHMIC_FILL_BUTTON) {
    if (pressed) {
      display->setByIndex(QUAKE_ALGORITHMIC_FILL_BUTTON, AUTOFILL_ON_COLOR);
      DrawButtons(true);
    } else {
      display->setByIndex(QUAKE_ALGORITHMIC_FILL_BUTTON, AUTOFILL_OFF_COLOR);
      DrawButtons(true);
    }
  } else if (row == VELOCITY_ROW) {
    if (pressed) {
      int velocity = toVelocity(VELOCITY_LEVELS - column - 1);
      // SERIALPRINTLN("ButtonEvent: row=" + String(row) + ", col=" + String(column) + ", vel=" + String(velocity));
      currentPattern->tracks[selectedTrack].measures[selectedMeasure].steps[selectedStep] = velocity;
      DrawMeasures(true);
    }
  }
}

void Quake::KeyEvent(uint8_t column, uint8_t pressed) {

}


/***** UI display methods ************************************************************/

uint8_t Quake::getColor() {
  return PRIMARY_COLOR;
}

uint8_t Quake::getDimColor() {
  return PRIMARY_DIM_COLOR;
}


/***** Drawing methods ************************************************************/

void Quake::Draw(bool update) {
  // SERIALPRINTLN("Quake:Draw");
  // SERIALPRINTLN("QDraw v16=" + String(hardware.currentValue[16]));
  DrawTracks(false);
  DrawMeasures(false);
  DrawButtons(false);

  if (update) display->Update();
}

void Quake::DrawTracks(bool update) {
  // SERIALPRINTLN("Quake:DrawTracks");
  // SERIALPRINTLN("QDrawTracks1 v16=" + String(hardware.currentValue[16]));

  for (int i=0; i < TRACKS_PER_PATTERN; i++) {
    // SERIALPRINTLN("QDrawTracks2 v16=" + String(hardware.currentValue[16]));
    uint16_t color = TRACK_ENABLED_OFF_COLOR;
    if (memory.trackEnabled[i]) {
      color = TRACK_ENABLED_ON_COLOR;
      if (soundingTracks[i]) {
        color = TRACK_ENABLED_ON_PLAY_COLOR;
      }
    } else if (soundingTracks[i]) {
      // TODO: this currently doesn't light up, because soundTracks isn't set if track is muted
      color = TRACK_ENABLED_OFF_PLAY_COLOR;
    }
    // hardware.CurrentValues();
    display->setGrid(TRACK_ENABLED_ROW, i, color);

    color = TRACK_SELECT_OFF_COLOR;
    if (i == selectedTrack) {
      color = TRACK_SELECT_SELECTED_COLOR;
    }
    display->setGrid(TRACK_SELECT_ROW, i, color);
  }

  // SERIALPRINTLN("QDrawTracks3 v16=" + String(hardware.currentValue[16]));
  display->setGrid(CLOCK_ROW, currentStep, ON_COLOR);
  display->setGrid(CLOCK_ROW, (currentStep + STEPS_PER_MEASURE - 1) % STEPS_PER_MEASURE, ABS_BLACK);

  if (update) display->Update();
}

void Quake::DrawMeasures(bool update) {
  // SERIALPRINTLN("Quake:DrawMeasures");
  // if (receiver == nullptr) return;

  for (int m = 0; m < MEASURES_PER_PATTERN; m++) {
    for (int i=0; i < STEPS_PER_MEASURE; i++) {
      uint8_t row = FIRST_MEASURES_ROW + m;
      uint8_t color = STEPS_OFF_COLOR;
      if (currentPattern->tracks[selectedTrack].measures[m].steps[i] > 0) {
        color = STEPS_ON_COLOR;
        if (isFill(m)) {
          color = STEPS_FILL_ON_COLOR;
        }
        if (i == selectedStep && m == selectedMeasure) {
          color = STEPS_ON_SELECT_COLOR;
        }
      } else if (i == selectedStep && m == selectedMeasure) {
        color = STEPS_OFF_SELECT_COLOR;
      }
      display->setGrid(row, i, color);
    }
  }

  // draw velocity buttons
  int velocity = currentPattern->tracks[selectedTrack].measures[selectedMeasure].steps[selectedStep];
  int vMapped = fromVelocity(velocity);
  // SERIALPRINTLN("DrawMeasures: velo=" + String(velocity) + ", vM=" + String(vMapped));
  for (int i = 0; i < VELOCITY_LEVELS; i++) {
    uint8_t color = VELOCITY_OFF_COLOR;
    if (vMapped == i) {
      color = VELOCITY_ON_COLOR;
    }
    display->setButton(VELOCITY_ROW, VELOCITY_LEVELS - i - 1, color);
  }

  if (update) display->Update();
}

void Quake::DrawButtons(bool update) {
  // SERIALPRINTLN("Quake:DrawButtons");
  display->setByIndex(QUAKE_LOAD_BUTTON, LOAD_OFF_COLOR);
  display->setByIndex(QUAKE_SAVE_BUTTON, SAVE_OFF_COLOR);
  // display->setByIndex(QUAKE_TRACK_SHUFFLE_BUTTON, TRACK_SHUFFLE_OFF_COLOR);
  // display->setByIndex(QUAKE_PATTERN_SHUFFLE_BUTTON, TRACK_SHUFFLE_OFF_COLOR);
  display->setByIndex(QUAKE_CLEAR_BUTTON, PRIMARY_DIM_COLOR);

  // draw current/select measure
  for (int m = 0; m < MEASURES_PER_PATTERN; m++) {
    uint8_t color = MEASURE_SELECT_OFF_COLOR;
    if (m == currentMeasure && autofillPlaying) {
      color = MEASURE_SELECT_AUTOFILL_COLOR;
    } else if (m == currentMeasure) {
      color = MEASURE_SELECT_PLAYING_COLOR;
    } else if (m == selectedMeasure) {
      // not using this right now
      // color = MEASURE_SELECT_SELECTED_COLOR;
    }
    display->setGrid(MODE_ROW, MEASURE_SELECT_MIN_COLUMN + m, color);
  }

  // draw measure mode
  for (int column = MEASURE_MODE_MIN_COLUMN; column <= MEASURE_MODE_MAX_COLUMN; column++) {
    uint8_t color = MEASURE_MODE_OFF_COLOR;
    if (column - MEASURE_MODE_MIN_COLUMN == memory.measureMode) {
      color = MEASURE_MODE_ON_COLOR;
    }
    display->setGrid(MODE_ROW, column, color);
  }

  // draw autofill interval
  for (int column = AUTOFILL_INTERVAL_MIN_COLUMN; column <= AUTOFILL_INTERVAL_MAX_COLUMN; column++) {
    uint8_t color = AUTOFILL_OFF_COLOR;
    if (column - AUTOFILL_INTERVAL_MIN_COLUMN == memory.autofillIntervalSetting) {
      color = AUTOFILL_ON_COLOR;
    }
    display->setGrid(MODE_ROW, column, color);
  }

  // draw autofill type
  uint8_t color = AUTOFILL_OFF_COLOR;  // type PATTERN 
  if (memory.autofillType == ALGORITHMIC) {
    color = AUTOFILL_ON_COLOR;
  } else if (memory.autofillType == BOTH) {
    color = ON_COLOR;
  }
  display->setGrid(MODE_ROW, AUTOFILL_TYPE_COLUMN, color);

  if (update) display->Update();
}
    


/***** MIDI ************************************************************/

void Quake::SendNotes() {
  for (uint8_t track = 0; track < TRACKS_PER_PATTERN; track++) {
    // send a note off if the track was previously sounding; even if the module is now muted
    if (soundingTracks[track]) {
      hardware.SendMidiNote(memory.midiChannel, midi_notes[trackMap[track]], 0);
      soundingTracks[track] = false;
    }

    // send a note if one is set and the module is not muted
    if (!muted && memory.trackEnabled[track]) {
      int8_t v = currentPattern->tracks[track].measures[currentMeasure].steps[patternMap[currentStep]]; 
      if (v > 0) {
        hardware.SendMidiNote(memory.midiChannel, midi_notes[trackMap[track]], v);
        soundingTracks[track] = true;
      }
    }
  }
}

void Quake::AllNotesOff() {
  for (uint8_t track = 0; track < TRACKS_PER_PATTERN; track++) {
    // send a note off for the track regardless of whether we think it was sounding
    hardware.SendMidiNote(memory.midiChannel, midi_notes[track], 0);
    soundingTracks[track] = false;
  }
}

void Quake::ClearSoundingNotes() {
  for (uint8_t track = 0; track < TRACKS_PER_PATTERN; track++) {
    soundingTracks[track] = false;
  }
}

uint8_t Quake::toVelocity(uint8_t v) {
  return (v + 1) * (128 / VELOCITY_LEVELS) - 1;
}
uint8_t Quake::fromVelocity(uint8_t velocity) {
  return (velocity + 1) / (128 / VELOCITY_LEVELS) - 1;  
}


/***** Misc ************************************************************/

void Quake::Reset() {
  for (uint8_t track = 0; track < TRACKS_PER_PATTERN; track++) {
    memory.trackEnabled[track] = true;
  }
  selectedTrack = 0;
}

void Quake::ResetPattern(uint8_t patternIndex) {
  SERIALPRINTLN("Quake::ResetPattern, p=" + String(patternIndex));
  for (uint8_t track = 0; track < TRACKS_PER_PATTERN; track++) {
    for (uint8_t measure = 0; measure < MEASURES_PER_PATTERN; measure++) {
      for (uint8_t step = 0; step < STEPS_PER_MEASURE; step++) {
        memory.patterns[patternIndex].tracks[track].measures[measure].steps[step] = 0;
      }      
    }
  }
}

void Quake::ResetSelectedTrack() {
  for (uint8_t measure = 0; measure < MEASURES_PER_PATTERN; measure++) {
    for (uint8_t step = 0; step < STEPS_PER_MEASURE; step++) {
      currentPattern->tracks[selectedTrack].measures[measure].steps[step] = 0;
    }      
  }
}

bool Quake::isFill(uint8_t measure) {
  return measure > memory.measureMode;
}

void Quake::NextMeasure(uint8_t measureCounter) {
  if (autofillPlaying) {
    currentMeasure = 0;
    autofillPlaying = false;
  } else if (memory.measureMode == 2) {
    // mode 2 plays in order ABACABAC instead of ABCABC
    switch (measureCounter % 4) {
      case 0:
        currentMeasure = 0;
        break;
      case 1:
        currentMeasure = 1;
        break;
      case 2:
        currentMeasure = 0;
        break;
      case 3:
        currentMeasure = 2;
        break;
      default:
        currentMeasure = 0;
    }
  } else {
    currentMeasure = (currentMeasure + 1) % (memory.measureMode + 1);
  }

  if (memory.autofillIntervalSetting >= 0 && memory.autofillIntervalSetting < NUM_AUTOFILL_INTERVALS)  {
    int i = autofillIntervals[memory.autofillIntervalSetting];
    if (measureCounter % i == i - 1) {
      int r = random(2);
      if (memory.autofillType == PATTERN || (memory.autofillType == BOTH && r == 0)) {
        int fill = RandomFillPattern();
        if (fill != -1) {
          currentMeasure = fill;
          autofillPlaying = true;
        }
      } else {
        currentMeasure = RandomAlgorithmicPattern();
        autofillPlaying = true;
      }
    }
  }
}

int Quake::RandomFillPattern() {
  int range = MEASURES_PER_PATTERN - memory.measureMode - 1;
  if (range == 0) return -1;
  return memory.measureMode + 1 + random(range);
}

int Quake::RandomAlgorithmicPattern() {
  return random(16) + 4;
}

