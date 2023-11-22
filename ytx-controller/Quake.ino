#include "headers/Quake.h"
#include "headers/Hachi.h"
#include "headers/Display.h"

Quake::Quake() {
}

void Quake::Init(uint8_t index, Display *display) {
  SERIALPRINTLN("Quake::Init idx=" + String(index) + ", memsize=" + sizeof(memory));
  this->index = index;
  this->display = display;
  Reset();
  for (int p = 0; p < NUM_PATTERNS; p++) {
    ResetPattern(p);
  }

  memory.midiChannel = index + 10;

  // Draw(true);
}

void Quake::SetColors(uint8_t primaryColor, uint8_t primaryDimColor) {
  this->primaryColor = primaryColor;
  this->primaryDimColor = primaryDimColor;  
}

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
    if (stuttering) {
      currentStep = currentStep + 1;
      if (currentStep > stutterStep + memory.stutterLength) {
        currentStep = stutterStep;
      }
      currentStep = currentStep % STEPS_PER_MEASURE;
    } else {
      currentStep = (currentStep + 1) % STEPS_PER_MEASURE;
    }
    if (sixteenthCounter == 0 && memory.measureReset == 1 && !stuttering) {
      currentStep = 0;
      NextMeasure(measureCounter);
    }
    SendNotes();
    DrawTracks(false);
    DrawMeasures(false);
    //DrawButtons(false);   // we skip this here because it resets some of the button colors
    DrawSettings(true);
  }
}


/***** Events ************************************************************/

void Quake::GridEvent(uint8_t row, uint8_t column, uint8_t pressed) {

  // if we're in the middle of a copy or clear operation
  if (copying) {
    Copying(GRID, row, column, pressed);
    return;
  } else if (clearing) {
    Clearing(GRID, row, column, pressed);
    return;
  }

  if (row == TRACK_ENABLED_ROW) {
    // enable/disable tracks
    if (pressed) {
      memory.trackEnabled[column] = !memory.trackEnabled[column];
      DrawTracks(true);
    }
  } else if (row == TRACK_SELECT_ROW) {
    // select track for editing
    if (pressed) {
      if (inPerfMode) {
        hardware.SendMidiNote(memory.midiChannel, midi_notes[column], 100);
        display->setGrid(TRACK_SELECT_ROW, column, ACCENT_COLOR);
      } else {
        selectedTrack = column;
        DrawTracks(true);
        DrawMeasures(true);
      }
    } else {
      if (inPerfMode) {
        hardware.SendMidiNote(memory.midiChannel, midi_notes[column], 0);
        display->setGrid(TRACK_SELECT_ROW, column, ACCENT_DIM_COLOR);
      }
    }
  } else if (row == MODE_ROW && column >= MEASURE_MODE_MIN_COLUMN && column <= MEASURE_MODE_MAX_COLUMN) {
    // set the playback mode for measures
    if (pressed) {
      memory.measureMode = column - MEASURE_MODE_MIN_COLUMN;
      DrawSettings(true);
    }
  } else if (row == MODE_ROW && column >= AUTOFILL_INTERVAL_MIN_COLUMN && column <= AUTOFILL_INTERVAL_MAX_COLUMN) {
    // set the autofill interval
    if (pressed) {
      if (memory.autofillIntervalSetting == column - AUTOFILL_INTERVAL_MIN_COLUMN) {
        memory.autofillIntervalSetting = -1;
      } else {
        memory.autofillIntervalSetting = column - AUTOFILL_INTERVAL_MIN_COLUMN;
      }
      DrawSettings(true);
    }
  } else if (row == MODE_ROW && column >= STUTTER_LENGTH_MIN_COLUMN && column <= STUTTER_LENGTH_MAX_COLUMN) {
    // set the stutter length
    if (pressed) {
      memory.stutterLength = column - STUTTER_LENGTH_MIN_COLUMN;
      DrawSettings(true);
    }
  } else if (row == MODE_ROW && column == AUTOFILL_TYPE_COLUMN) {
    if (pressed) {
      if (memory.autofillType == PATTERN) {
        memory.autofillType = ALGORITHMIC;
      } else if (memory.autofillType == ALGORITHMIC) {
        memory.autofillType = BOTH;
      } else {
        memory.autofillType = PATTERN;
      }
      DrawSettings(true);
    }
  } else if (row >= FIRST_MEASURES_ROW && row < FIRST_MEASURES_ROW + MEASURES_PER_PATTERN) {
    uint8_t measure = row - FIRST_MEASURES_ROW;
    if (inPerfMode) {
      if (pressed) {
        currentMeasure = measure;
        currentStep = (column - 1) % STEPS_PER_MEASURE;
        stuttering = true;
        stutterStep = column;
      } else {
        stuttering = false;
      }
    } else {
      // edit steps in the pattern
      if (pressed) {
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
  }

}

void Quake::ButtonEvent(uint8_t row, uint8_t column, uint8_t pressed) {
  // SERIALPRINTLN("Quake::ButtonEvent row=" + String(row) + ", col=" + String(column) + ", pr=" + String(pressed));

  // if we're in the middle of a copy or clear operation
  if (copying) {
    Copying(BUTTON, row, column, pressed);
    return;
  } else if (clearing) {
    Clearing(BUTTON, row, column, pressed);
    return;
  }

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
  } else if (index == QUAKE_COPY_BUTTON) {
    if (pressed) {
      copying = true;
      display->setByIndex(QUAKE_COPY_BUTTON, ON_COLOR);
      display->Update();
    } else {
      copying = false;
      display->setByIndex(QUAKE_COPY_BUTTON, OFF_COLOR);
      display->Update();
    }
  } else if (index == QUAKE_CLEAR_BUTTON) {
    if (pressed) {
      clearing = true;
      display->setByIndex(QUAKE_CLEAR_BUTTON, ON_COLOR);
      display->Update();
    } else {
      clearing = false;
      display->setByIndex(QUAKE_CLEAR_BUTTON, PRIMARY_DIM_COLOR);
      display->Update();
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
  } else if (row == PATTERN_ROW && column < NUM_PATTERNS) {
    if (pressed) {
      nextPatternIndex = column;
      DrawTracks(true);
    }
  }
}

void Quake::KeyEvent(uint8_t column, uint8_t pressed) {

  uint8_t index = hardware.toDigital(KEY, 0, column);
  if (column >= QUAKE_INSTAFILL_MIN_KEY && column <= QUAKE_INSTAFILL_MAX_KEY) {
    if (pressed) {
      display->setKey(column, FILL_COLOR);
      inInstafill = true;
      originalMeasure = column;
    } else {
      inInstafill = false;
      display->setKey(column, PRIMARY_DIM_COLOR);
    }
  } else if (index == QUAKE_TRACK_SHUFFLE_BUTTON) {
    if (pressed) {
      display->setByIndex(QUAKE_TRACK_SHUFFLE_BUTTON, PERF_COLOR);
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
      display->setByIndex(QUAKE_TRACK_SHUFFLE_BUTTON, PERF_DIM_COLOR);
      display->Update();
    }
  // } else if (index == QUAKE_PATTERN_SHUFFLE_BUTTON) {
  //   if (pressed) {
  //     display->setByIndex(QUAKE_PATTERN_SHUFFLE_BUTTON, TRACK_SHUFFLE_ON_COLOR);
  //     display->Update();
  //     for (int t = 0; t < STEPS_PER_MEASURE; t++) {
  //       patternMap[t] = random(STEPS_PER_MEASURE);
  //     }
  //     AllNotesOff();
  //   } else {
  //     for (int t = 0; t < STEPS_PER_MEASURE; t++) {
  //       patternMap[t] = t;
  //     }
  //     AllNotesOff();
  //     display->setByIndex(QUAKE_PATTERN_SHUFFLE_BUTTON, TRACK_SHUFFLE_OFF_COLOR);
  //     display->Update();
  //   }
  } else if (index == QUAKE_ALGORITHMIC_FILL_BUTTON) {
    if (pressed) {
      display->setByIndex(QUAKE_ALGORITHMIC_FILL_BUTTON, AUTOFILL_ON_COLOR);
      display->Update();
      int *fillPattern = fills[random(FILL_PATTERN_COUNT)];
      for (int t = 0; t < STEPS_PER_MEASURE; t++) {
        patternMap[t] = fillPattern[t];
      }
      AllNotesOff();
    } else {
      for (int t = 0; t < STEPS_PER_MEASURE; t++) {
        patternMap[t] = t;
      }
      AllNotesOff();
      display->setByIndex(QUAKE_ALGORITHMIC_FILL_BUTTON, AUTOFILL_OFF_COLOR);
      display->Update();
    }    
  } else if (index == QUAKE_STUTTER_BUTTON) {
    if (pressed) {
      display->setKey(column, PERF_COLOR);
      stuttering = true;
      stutterStep = currentStep;
    } else {
      stuttering = false;
      display->setKey(column, PERF_DIM_COLOR);
    }
  } else if (index == QUAKE_PERF_MODE_BUTTON) {
    if (pressed) {
      inPerfMode = !inPerfMode;
      display->setKey(column, inPerfMode ? PERF_COLOR : PERF_DIM_COLOR);
      DrawMeasures(false);
      DrawSettings(true);
    }
  }
}

void Quake::Clearing(digital_type type, uint8_t row, uint8_t column, uint8_t pressed) {
  
  if (type == BUTTON && row == PATTERN_ROW) {
    // clear pattern
    ResetPattern(column);
    clearing = false;
    Draw(true);
  } else if (type == GRID && row == TRACK_SELECT_ROW) {
    // clear track
    ResetTrack(column);
    clearing = false;
    Draw(true);
  } else if (type == GRID && row == MODE_ROW && column >= MEASURE_SELECT_MIN_COLUMN && column <= MEASURE_SELECT_MAX_COLUMN) {
    // clear measure
    ResetMeasure(column - MEASURE_SELECT_MIN_COLUMN);
    clearing = false;
    Draw(true); 
  }

  clearing = false;
}

void Quake::Copying(digital_type type, uint8_t row, uint8_t column, uint8_t pressed) {
  
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
  DrawSettings(false);

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
    if (inPerfMode) {
      color = ACCENT_DIM_COLOR;
    } else if (i == selectedTrack) {
      color = TRACK_SELECT_SELECTED_COLOR;
    }
    display->setGrid(TRACK_SELECT_ROW, i, color);
  }

  // SERIALPRINTLN("QDrawTracks3 v16=" + String(hardware.currentValue[16]));
  display->setGrid(CLOCK_ROW, currentStep, ON_COLOR);
  display->setGrid(CLOCK_ROW, (currentStep + STEPS_PER_MEASURE - 1) % STEPS_PER_MEASURE, ABS_BLACK);

  // draw patterns
  for (int p = 0; p < NUM_PATTERNS; p++) {
    uint8_t color = PATTERN_OFF_COLOR;
    if (p == memory.currentPatternIndex) {
      color = PATTERN_CURRENT_COLOR;
    } else if (p == nextPatternIndex) {
      color = PATTERN_NEXT_COLOR;
    }
    display->setButton(PATTERN_ROW, p, color);
  }

  if (update) display->Update();
}

void Quake::DrawMeasures(bool update) {
  // SERIALPRINTLN("Quake:DrawMeasures");

  for (int m = 0; m < MEASURES_PER_PATTERN; m++) {
    for (int i=0; i < STEPS_PER_MEASURE; i++) {
      uint8_t row = FIRST_MEASURES_ROW + m;
      uint8_t color = STEPS_OFF_COLOR;
      if (inPerfMode) {
        if (m == currentMeasure && i == currentStep) {
          color = ACCENT_COLOR;
        } else if (i == currentStep) {
          color = OFF_COLOR;
        }
      } else {
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
  display->setByIndex(QUAKE_CLEAR_BUTTON, PRIMARY_DIM_COLOR);
  display->setByIndex(QUAKE_COPY_BUTTON, OFF_COLOR);
  display->setByIndex(QUAKE_ALGORITHMIC_FILL_BUTTON, AUTOFILL_OFF_COLOR);
  display->setByIndex(QUAKE_TRACK_SHUFFLE_BUTTON, PERF_DIM_COLOR);
  // display->setByIndex(QUAKE_PATTERN_SHUFFLE_BUTTON, AUTOFILL_OFF_COLOR);
  display->setByIndex(QUAKE_PERF_MODE_BUTTON, inPerfMode ? PERF_COLOR : PERF_DIM_COLOR);
  display->setByIndex(QUAKE_STUTTER_BUTTON, stuttering ? PERF_COLOR : PERF_DIM_COLOR);

  // draw instafill pattern buttons
  uint8_t color = PRIMARY_DIM_COLOR;
  for (int column = QUAKE_INSTAFILL_MIN_KEY; column <= QUAKE_INSTAFILL_MAX_KEY; column++) {
    display->setKey(column, color);
  }

  if (update) display->Update();
}

void Quake::DrawSettings(bool update) {
 
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
    } else if (column - MEASURE_MODE_MIN_COLUMN > memory.measureMode) {
      color = FILL_DIM_COLOR;
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

  // draw stutter length
  for (int column = STUTTER_LENGTH_MIN_COLUMN; column <= STUTTER_LENGTH_MAX_COLUMN; column++) {
    uint8_t color = PERF_DIM_COLOR;
    if (column - STUTTER_LENGTH_MIN_COLUMN == memory.stutterLength) {
      color = PERF_COLOR;
    }
    display->setGrid(MODE_ROW, column, color);
  }

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

    // if we're doing an instafill, use the notes from the instafill pattern
    uint8_t measure = currentMeasure;
    if (inInstafill) {
      measure = originalMeasure;
    }

    // send a note if one is set and the module is not muted
    if (!muted && memory.trackEnabled[track]) {
      int step = patternMap[currentStep];
      if (step >= 0) {
        int8_t v = currentPattern->tracks[track].measures[measure].steps[step]; 
        if (v > 0) {
          hardware.SendMidiNote(memory.midiChannel, midi_notes[trackMap[track]], v);
          soundingTracks[track] = true;
        }
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
  // SERIALPRINTLN("Quake::ResetPattern, p=" + String(patternIndex));
  for (uint8_t track = 0; track < TRACKS_PER_PATTERN; track++) {
    for (uint8_t measure = 0; measure < MEASURES_PER_PATTERN; measure++) {
      for (uint8_t step = 0; step < STEPS_PER_MEASURE; step++) {
        memory.patterns[patternIndex].tracks[track].measures[measure].steps[step] = 0;
      }      
    }
  }
}

void Quake::ResetTrack(uint8_t trackIndex) {
  for (uint8_t measure = 0; measure < MEASURES_PER_PATTERN; measure++) {
    for (uint8_t step = 0; step < STEPS_PER_MEASURE; step++) {
      currentPattern->tracks[trackIndex].measures[measure].steps[step] = 0;
    }      
  }
}

void Quake::ResetMeasure(uint8_t measureIndex) {
  for (uint8_t track = 0; track < TRACKS_PER_PATTERN; track++) {
    for (uint8_t step = 0; step < STEPS_PER_MEASURE; step++) {
      currentPattern->tracks[track].measures[measureIndex].steps[step] = 0;
    }      
  }
}

bool Quake::isFill(uint8_t measure) {
  return measure > memory.measureMode;
}

void Quake::NextMeasure(uint8_t measureCounter) {

  // if there's a next pattern queued up, switch to it and finish
  if (nextPatternIndex != -1) {
    memory.currentPatternIndex = nextPatternIndex;
    currentPattern = &memory.patterns[nextPatternIndex];
    nextPatternIndex = -1;
    return;
  }

  if (autofillPlaying) {
    currentMeasure = 0;
    autofillPlaying = false;
    for (int t = 0; t < STEPS_PER_MEASURE; t++) {
      patternMap[t] = t;
    }
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
        SelectAlgorithmicFill();
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

void Quake::SelectAlgorithmicFill() {
  int *fillPattern = fills[random(FILL_PATTERN_COUNT)];
  for (int t = 0; t < STEPS_PER_MEASURE; t++) {
    patternMap[t] = fillPattern[t];
  }
}




