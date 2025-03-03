#include "headers/Quake.h"
#include "headers/Hachi.h"
#include "headers/Display.h"
#include "headers/Fill.h"

Quake::Quake() {
}

void Quake::Init(uint8_t index, Display *display) {
  // SERIALPRINTLN("Quake::Init idx=" + String(index) + ", storage=" + String(GetStorageSize()) + ", freemem=" + String(FreeMemory()));
  this->index = index;
  this->display = display;
  ResetTrackEnabled();
  if (index < 7) {
    memory.midiChannel = index + 10;
  } else {
    memory.midiChannel = 10;
  }
  LoadSettings();

  ResetCurrentPattern();
  // Draw(true);
}

void Quake::SetColors(uint8_t primaryColor, uint8_t primaryDimColor) {
  this->primaryColor = primaryColor;
  this->primaryDimColor = primaryDimColor;  
}

uint32_t Quake::GetStorageSize() {
  // SERIALPRINTLN("Quake::GetMemSize: mem=" + String(sizeof(memory)) + ", pat=" + String(sizeof(*currentPattern)));
  return sizeof(memory) + Q_PATTERN_COUNT * sizeof(*currentPattern);
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
    // if (sixteenthCounter == 0 && memory.measureReset == 1 && !stuttering) {   // TODO: add measure reset
    if (sixteenthCounter == 0 && !stuttering) {
      currentStep = 0;
      NextMeasure(measureCounter);
    }
    SendNotes();

    this->sixteenthCounter = sixteenthCounter;
    this->measureCounter = measureCounter;

    DrawTracks(false);
    DrawMeasures(false);
    //DrawButtons(false);   // we skip this here because it resets some of the button colors
    DrawOptions(true);
  }
}


/***** Events ************************************************************/

void Quake::GridEvent(uint8_t row, uint8_t column, uint8_t pressed) {

  uint8_t index = hardware.toDigital(GRID, row, column);

  // if we're in the middle of a copy or clear operation
  if (copying) {
    Copying(GRID, row, column, pressed);
    return;
  } else if (clearing) {
    Clearing(GRID, row, column, pressed);
    return;
  } else if (copying) {
    Copying(GRID, row, column, pressed);
    return;
  } else if (inSettings) {
    if (editingNotesTrack != -1) {
      if (pressed) {
        //display->setGrid()   turn off old value?
        display->setGrid(row, column, H_MIDI_COLOR);
        memory.midiNotes[editingNotesTrack] = row * 16 + column;
        editingNotesTrack = -1;
        SaveSettings();
        Draw(true);
      }
    } else if (row == H_MIDI_CHANNEL_ROW) {
      AllNotesOff();
      memory.midiChannel = column + 1;
      SaveSettings();
      DrawSettings(true);
    } else if (row == H_SETTINGS_ROW && column >= H_RESET_START_COLUMN && column <= H_RESET_END_COLUMN) {
      if (column - H_RESET_START_COLUMN + 1 == memory.measureReset) {
        memory.measureReset = 1;   // not allowing reset=0  
      } else {
        memory.measureReset = column - H_RESET_START_COLUMN + 1;
      }
      DrawSettings(true);
    } else if (row == Q_NOTE_SETTINGS_ROW) {
      if (pressed) {
        editingNotesTrack = column;
        display->DrawValueOnGrid(memory.midiNotes[column], H_MIDI_COLOR);
        display->Update();
      }
    } else if (index == Q_NOTE_RESET_BUTTON) {
      if (pressed) {
        display->setByIndex(Q_NOTE_RESET_BUTTON, ON_COLOR);
        display->Update();
        SetMidiNotes();
        SaveSettings();
      } else {
        display->setByIndex(Q_NOTE_RESET_BUTTON, H_MIDI_DIM_COLOR);
        display->Update();
      }
    }
    return;
  }

  if (row == TRACK_ENABLED_ROW) {
    // enable/disable tracks
    if (pressed) {
      memory.trackEnabled = BitArray16_Toggle(memory.trackEnabled, column);
      SaveSettings();
      DrawTracks(SAVING);
    }
  } else if (row == TRACK_SELECT_ROW) {
    // select track for editing
    if (pressed) {
      if (inPerfMode) {
        hardware.SendMidiNote(memory.midiChannel, memory.midiNotes[column], 100);
        display->setGrid(TRACK_SELECT_ROW, column, ACCENT_COLOR);
      } else {
        selectedTrack = column;
        DrawTracks(true);
        DrawMeasures(true);
      }
    } else {
      if (inPerfMode) {
        hardware.SendMidiNote(memory.midiChannel, memory.midiNotes[column], 0);
        display->setGrid(TRACK_SELECT_ROW, column, ACCENT_DIM_COLOR);
      }
    }
  } else if (row == MODE_ROW && column >= MEASURE_MODE_MIN_COLUMN && column <= MEASURE_MODE_MAX_COLUMN) {
    // set the playback mode for measures
    if (pressed) {
      currentPattern->measureMode = column - MEASURE_MODE_MIN_COLUMN;
      DrawOptions(true);
    }
  } else if (row == MODE_ROW && column >= Q_AUTOFILL_INTERVAL_MIN_COLUMN && column <= Q_AUTOFILL_INTERVAL_MAX_COLUMN) {
    // set the autofill interval
    if (pressed) {
      if (currentPattern->autofillIntervalSetting == column - Q_AUTOFILL_INTERVAL_MIN_COLUMN) {
        currentPattern->autofillIntervalSetting = -1;
      } else {
        currentPattern->autofillIntervalSetting = column - Q_AUTOFILL_INTERVAL_MIN_COLUMN;
      }
      DrawOptions(true);
    }
  } else if (row == MODE_ROW && column >= STUTTER_LENGTH_MIN_COLUMN && column <= STUTTER_LENGTH_MAX_COLUMN) {
    // set the stutter length
    if (pressed) {
      memory.stutterLength = column - STUTTER_LENGTH_MIN_COLUMN;
      SaveSettings();
      DrawOptions(true);
    }
  } else if (row == MODE_ROW && column == Q_AUTOFILL_TYPE_COLUMN) {
    if (pressed) {
      if (currentPattern->autofillType == PATTERN) {
        currentPattern->autofillType = ALGORITHMIC;
      } else if (currentPattern->autofillType == ALGORITHMIC) {
        currentPattern->autofillType = BOTH;
      } else {
        currentPattern->autofillType = PATTERN;
      }
      DrawOptions(true);
    }
  } else if (row >= FIRST_MEASURES_ROW && row < FIRST_MEASURES_ROW + MEASURES_PER_PATTERN) {
    uint8_t measure = row - FIRST_MEASURES_ROW;
    if (inPerfMode) {
      if (pressed) {
        currentMeasure = measure;
        JumpOn(column);
      } else {
        JumpOff();
      }
    } else {
      // edit steps in the pattern
      if (pressed) {
        selectedStep = column;
        selectedMeasure = measure;
        u8 v = NibArray16_GetValue(currentPattern->tracks[selectedTrack].measures[measure].steps, column);
        bool f = NibArray16_GetFlag(currentPattern->tracks[selectedTrack].measures[measure].steps, column);
        if (!f && v == 0) {
          NibArray16_SetFlag(currentPattern->tracks[selectedTrack].measures[measure].steps, column, true);
          NibArray16_SetValue(currentPattern->tracks[selectedTrack].measures[measure].steps, column, INITIAL_VELOCITY);
        } else {
          NibArray16_ToggleFlag(currentPattern->tracks[selectedTrack].measures[measure].steps, column);
        }
        DrawMeasures(true);
      }
    }
  } else if (inSettings && row == H_MIDI_CHANNEL_ROW) {
    memory.midiChannel = column + 1;
    DrawSettings(true);
  }

}

void Quake::ButtonEvent(uint8_t row, uint8_t column, uint8_t pressed) {
  // SERIALPRINTLN("Quake::ButtonEvent row=" + String(row) + ", col=" + String(column) + ", pr=" + String(pressed));

  uint8_t index = hardware.toDigital(BUTTON, row, column);

  // if we're in the middle of a copy or clear operation
  if (copying) {
    if (index == Q_COPY_BUTTON && !pressed) {
      copying = false;
      copyingFirst = false;
      display->setByIndex(Q_COPY_BUTTON, H_COPY_OFF_COLOR);
      display->Update();
      return;
    }
    Copying(BUTTON, row, column, pressed);
    return;
  } else if (clearing) {
    if (index == Q_CLEAR_BUTTON && !pressed) {
      clearing = false;
      display->setByIndex(Q_CLEAR_BUTTON, H_CLEAR_OFF_COLOR);
      display->Update();
      return;
    }
    Clearing(BUTTON, row, column, pressed);
    return;
  } else if (copying) {
    Copying(BUTTON, row, column, pressed);
    return;
  } else if (inPerfMode) {
    return;
  }

  if (index == Q_SAVE_BUTTON) {
    if (pressed) {
      display->setByIndex(Q_SAVE_BUTTON, H_SAVE_ON_COLOR);
      // hachi.saveModuleMemory(this, (byte*)&memory);
      // hachi.saveModuleMemory(this, 0, sizeof(memory), (byte*)&memory);
      // hachi.saveModuleMemory(this, 0, sizeof(memory), (byte*)&memory);
      SavePattern();
    } else {
      display->setByIndex(Q_SAVE_BUTTON, H_SAVE_OFF_COLOR);
    }
  } else if (index == Q_COPY_BUTTON) {
    if (pressed) {
      copying = true;
      copyingFirst = false;
      display->setByIndex(Q_COPY_BUTTON, H_COPY_ON_COLOR);
      display->Update();
    } else {
      copying = false;
      copyingFirst = false;
      display->setByIndex(Q_COPY_BUTTON, H_COPY_OFF_COLOR);
      display->Update();
    }
  } else if (index == Q_CLEAR_BUTTON) {
    if (pressed) {
      clearing = true;
      display->setByIndex(Q_CLEAR_BUTTON, H_CLEAR_ON_COLOR);
      display->Update();
    } else {
      clearing = false;
      display->setByIndex(Q_CLEAR_BUTTON, H_CLEAR_OFF_COLOR);
      display->Update();
    }
  } else if (index == Q_SETTINGS_BUTTON) {
    if (pressed) {
      inSettings = !inSettings;
      if (inSettings) {
        display->setByIndex(Q_SETTINGS_BUTTON, ON_COLOR);
        DrawSettings(true);
      } else {
        editingNotesTrack = -1;
        Draw(true);
      }
    }
  } else if (row == VELOCITY_ROW) {
    if (pressed) {
      NibArray16_SetValue(currentPattern->tracks[selectedTrack].measures[selectedMeasure].steps, selectedStep, VELOCITY_LEVELS - column - 1);
      DrawMeasures(true);
    }
  } else if (row == PATTERN_ROW && column < Q_PATTERN_COUNT) {
    if (pressed) {
      nextPatternIndex = column;
      LoadPattern();
      DrawTracks(true);
    }
  }
}

void Quake::KeyEvent(uint8_t column, uint8_t pressed) {

  uint8_t index = hardware.toDigital(KEY, 0, column);
  if (column >= Q_INSTAFILL_MIN_KEY && column <= Q_INSTAFILL_MAX_KEY) {
    if (pressed) {
      display->setKey(column, H_FILL_COLOR);
      inInstafill = true;
      originalMeasure = column;
    } else {
      inInstafill = false;
      display->setKey(column, PRIMARY_DIM_COLOR);
    }
  } else if (index == Q_TRACK_SHUFFLE_BUTTON) {
    if (pressed) {
      display->setByIndex(Q_TRACK_SHUFFLE_BUTTON, TRACK_SHUFFLE_ON_COLOR);
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
      display->setByIndex(Q_TRACK_SHUFFLE_BUTTON, TRACK_SHUFFLE_OFF_COLOR);
      display->Update();
    }
  } else if (index == Q_ALGORITHMIC_FILL_BUTTON) {
    if (pressed) {
      InstafillOn(CHOOSE_RANDOM_FILL);
    } else {
      InstafillOff();
    }    
  } else if (index == Q_LAST_FILL_BUTTON) {
    if (pressed) {
      InstafillOn(lastFill);
    } else {
      InstafillOff();
    }    
  } else if (index == Q_STUTTER_BUTTON) {
    if (pressed) {
      display->setKey(column, PERF_COLOR);
      JumpOn(currentStep);
    } else {
      JumpOff();
      display->setKey(column, PERF_DIM_COLOR);
    }
  } else if (index == Q_PERF_MODE_BUTTON) {
    if (pressed) {
      inPerfMode = !inPerfMode;
      display->setKey(column, inPerfMode ? PERF_COLOR : PERF_DIM_COLOR);
      DrawMeasures(false);
      DrawOptions(true);
    }
  }
}

void Quake::EncoderEvent(u8 encoder, u8 value) {
  // SERIALPRINTLN("Quake::EncoderEvent, enc=" + String(encoder) + ", val=" + String(value));
  controllerValues[encoder] = value;
  hardware.SendMidiCc(memory.midiChannel, controllerNumbers[encoder], value);
  DrawEncoders(true);
}

void Quake::ToggleTrack(uint8_t trackNumber) {
  memory.trackEnabled = BitArray16_Toggle(memory.trackEnabled, trackNumber);
  SaveSettings();
  // DrawTracks(true);  // probably drawing is controlled by external caller
}


/***** UI display methods ************************************************************/

uint8_t Quake::getColor() {
  return PRIMARY_COLOR;
}

uint8_t Quake::getDimColor() {
  return PRIMARY_DIM_COLOR;
}


/***** performance features ************************************************************/

void Quake::InstafillOn(u8 index /*= CHOOSE_RANDOM_FILL*/) {
  display->setByIndex(Q_ALGORITHMIC_FILL_BUTTON, AUTOFILL_ON_COLOR);
  display->Update();
  if (index == CHOOSE_RANDOM_FILL) {
    index = Fill::ChooseFillIndex();
    lastFill = index;
  }
  const int *fillPattern = Fill::GetFillPattern(index);
  for (int t = 0; t < STEPS_PER_MEASURE; t++) {
    patternMap[t] = fillPattern[t];
  }
  AllNotesOff();
  instafillPlaying = true;
}

void Quake::InstafillOff() {
  for (int t = 0; t < STEPS_PER_MEASURE; t++) {
    patternMap[t] = t;
  }
  AllNotesOff();
  display->setByIndex(Q_ALGORITHMIC_FILL_BUTTON, AUTOFILL_OFF_COLOR);
  display->Update();
  instafillPlaying = false;
}

void Quake::JumpOn(u8 step) {
  currentStep = (step - 1) % STEPS_PER_MEASURE;
  stuttering = true;
  stutterStep = step;
}

void Quake::JumpOff() {
  stuttering = false;
}

void Quake::SetScale(u8 tonic, bit_array_16 scale) {
}

void Quake::ClearScale() {
}

void Quake::SetChord(u8 tonic, bit_array_16 chord) {
}

void Quake::ClearChord() {
}



/***** Drawing methods ************************************************************/

void Quake::Draw(bool update) {
  // SERIALPRINTLN("Quake:Draw");
  DrawTracks(false);
  DrawMeasures(false);
  DrawButtons(false);
  DrawOptions(false);
  DrawEncoders(false);
  if (inSettings) DrawSettings(false);

  if (update) display->Update();
}

void Quake::DrawEncoders(bool update) {
  for (u8 enc = 0; enc < 8; enc++) {
    display->setEncoder(enc, controllerValues[enc], primaryColor, primaryColor);
  }

  if (update) display->Update();
}

void Quake::DrawTracks(bool update) {
  // SERIALPRINTLN("Quake:DrawTracks");
  if (inSettings) return;

  for (int i=0; i < TRACKS_PER_PATTERN; i++) {
    uint16_t color = TRACK_ENABLED_OFF_COLOR;
    if (BitArray16_Get(memory.trackEnabled, i)) {
      color = TRACK_ENABLED_ON_COLOR;
      if (soundingTracks[i]) {
        color = TRACK_ENABLED_ON_PLAY_COLOR;
      }
    } else if (soundingTracks[i]) {
      color = TRACK_ENABLED_OFF_PLAY_COLOR;
    }
    // hardware.CurrentValues();
    display->setGrid(TRACK_ENABLED_ROW, i, color);

    color = TRACK_SELECT_OFF_COLOR;
    if (inPerfMode) {
      color = PRIMARY_DIM_COLOR;
    } else if (i == selectedTrack) {
      color = TRACK_SELECT_SELECTED_COLOR;
    }
    display->setGrid(TRACK_SELECT_ROW, i, color);
  }

  // draw the clock row
  display->DrawClock(CLOCK_ROW, measureCounter, sixteenthCounter, patternMap[currentStep]);

  // draw patterns
  for (int p = 0; p < Q_PATTERN_COUNT; p++) {
    uint8_t color = H_PATTERN_OFF_COLOR;
    if (p == memory.currentPatternIndex) {
      color = H_PATTERN_CURRENT_COLOR;
    } else if (p == nextPatternIndex) {
      color = H_PATTERN_NEXT_COLOR;
    }
    display->setButton(PATTERN_ROW, p, color);
  }

  if (update) display->Update();
}

void Quake::DrawMeasures(bool update) {
  // SERIALPRINTLN("Quake:DrawMeasures");
  if (inSettings) return;

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
        if (NibArray16_GetFlag(currentPattern->tracks[selectedTrack].measures[m].steps, i)) {
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
  u8 velocity = NibArray16_GetValue(currentPattern->tracks[selectedTrack].measures[selectedMeasure].steps, selectedStep);
  for (int i = 0; i < VELOCITY_LEVELS; i++) {
    uint8_t color = VELOCITY_OFF_COLOR;
    if (i == velocity) {
      color = VELOCITY_ON_COLOR;
    }
    display->setButton(VELOCITY_ROW, VELOCITY_LEVELS - i - 1, color);
  }

  if (update) display->Update();
}

void Quake::DrawButtons(bool update) {
  // SERIALPRINTLN("Quake:DrawButtons");
  display->setByIndex(Q_SAVE_BUTTON, H_SAVE_OFF_COLOR);
  display->setByIndex(Q_COPY_BUTTON, copying ? H_COPY_ON_COLOR : H_COPY_OFF_COLOR);
  display->setByIndex(Q_CLEAR_BUTTON, clearing ? H_CLEAR_ON_COLOR : H_CLEAR_OFF_COLOR);
  display->setByIndex(Q_SETTINGS_BUTTON, inSettings ? ON_COLOR : OFF_COLOR);
  display->setByIndex(Q_ALGORITHMIC_FILL_BUTTON, AUTOFILL_OFF_COLOR);
  display->setByIndex(Q_LAST_FILL_BUTTON, AUTOFILL_OFF_COLOR);
  display->setByIndex(Q_TRACK_SHUFFLE_BUTTON, TRACK_SHUFFLE_OFF_COLOR);
  display->setByIndex(Q_PERF_MODE_BUTTON, inPerfMode ? PERF_COLOR : PERF_DIM_COLOR);
  display->setByIndex(Q_STUTTER_BUTTON, stuttering ? PERF_COLOR : PERF_DIM_COLOR);

  // draw instafill pattern buttons
  uint8_t color = PRIMARY_DIM_COLOR;
  for (int column = Q_INSTAFILL_MIN_KEY; column <= Q_INSTAFILL_MAX_KEY; column++) {
    display->setKey(column, color);
  }

  if (update) display->Update();
}

void Quake::DrawOptions(bool update) {
  if (inSettings) return;
 
  // draw current/select measure
  for (int m = 0; m < MEASURES_PER_PATTERN; m++) {
    uint8_t color = MEASURE_SELECT_OFF_COLOR;
    if (m == currentMeasure && autofillPlaying) {
      color = MEASURE_SELECT_AUTO_FILL_COLOR;
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
    if (column - MEASURE_MODE_MIN_COLUMN == currentPattern->measureMode) {
      color = MEASURE_MODE_ON_COLOR;
    } else if (column - MEASURE_MODE_MIN_COLUMN > currentPattern->measureMode) {
      color = H_FILL_DIM_COLOR;
    }
    display->setGrid(MODE_ROW, column, color);
  }

  // draw autofill interval
  for (int column = Q_AUTOFILL_INTERVAL_MIN_COLUMN; column <= Q_AUTOFILL_INTERVAL_MAX_COLUMN; column++) {
    uint8_t color = AUTOFILL_OFF_COLOR;
    if (column - Q_AUTOFILL_INTERVAL_MIN_COLUMN == currentPattern->autofillIntervalSetting) {
      color = AUTOFILL_ON_COLOR;
    }
    display->setGrid(MODE_ROW, column, color);
  }

  // draw autofill type
  uint8_t color = AUTOFILL_OFF_COLOR;  // type PATTERN 
  if (currentPattern->autofillType == ALGORITHMIC) {
    color = AUTOFILL_ON_COLOR;
  } else if (currentPattern->autofillType == BOTH) {
    color = ON_COLOR;
  }
  display->setGrid(MODE_ROW, Q_AUTOFILL_TYPE_COLUMN, color);

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

void Quake::DrawSettings(bool update) {
  if (!inSettings) return;
  
  for (int row = 0; row < GRID_ROWS; row++) {
    for (int column = 0; column < GRID_COLUMNS; column++) {
      u8 index = hardware.toDigital(GRID, row, column);
      u8 color = ABS_BLACK;
      if (index == Q_NOTE_RESET_BUTTON) {
        color = H_MIDI_COLOR;
      } else if (row == H_MIDI_CHANNEL_ROW) {
        color = OFF_COLOR;
        if (column == memory.midiChannel - 1) {   // midi channel is 1-indexed
          color = ON_COLOR;
        }
      } else if (row == H_SETTINGS_ROW && column >= H_RESET_START_COLUMN && column <= H_RESET_END_COLUMN) {
        color = OFF_COLOR;
        if (column - H_RESET_START_COLUMN + 1 == memory.measureReset) {   // reset is 1-indexed
          color = ON_COLOR;
        }
      } else if (row == Q_NOTE_SETTINGS_ROW) {
        color = H_MIDI_DIM_COLOR;
      }
      display->setGrid(row, column, color);
    }
  }

  if (update) display->Update();
}
    
// Draw enabled tracks at an arbitrary row using module colors.
// Intended to be called by another module displaying track status.
void Quake::DrawTracksEnabled(Display *useDisplay, uint8_t gridRow) {
  for (int i=0; i < TRACKS_PER_PATTERN; i++) {
    uint16_t color = ABS_BLACK;
    // if (autofillPlaying) {
    //   color = AUTOFILL_OFF_COLOR;
    // }
    if (BitArray16_Get(memory.trackEnabled, i)) {
      color = getDimColor();
      if (autofillPlaying || instafillPlaying) {
        color = H_FILL_DIM_COLOR;
      }
      if (soundingTracks[i]) {
        color = muted ? ACCENT_DIM_COLOR : ACCENT_COLOR;
      }
    } else if (soundingTracks[i]) {
      color = OFF_COLOR;
    }
    useDisplay->setGrid(gridRow, i, color);
  }
}



/***** MIDI ************************************************************/

void Quake::SendNotes() {
  for (uint8_t track = 0; track < TRACKS_PER_PATTERN; track++) {
    // send a note off if the track was previously sounding; even if the module is now muted
    if (soundingTracks[track]) {
      hardware.SendMidiNote(memory.midiChannel, memory.midiNotes[trackMap[track]], 0);
      soundingTracks[track] = false;
    }

    // if we're doing an instafill, use the notes from the instafill pattern
    uint8_t measure = currentMeasure;
    if (inInstafill) {
      measure = originalMeasure;
    }

    int step = patternMap[currentStep];
    if (step >= 0) {            // in fills, a step number of -1 means a silent step
      if (NibArray16_GetFlag(currentPattern->tracks[track].measures[measure].steps, step)) {
        // send a note if one is set and the module is not muted
        if (!muted && BitArray16_Get(memory.trackEnabled, track)) {
          u8 v = NibArray16_GetValue(currentPattern->tracks[track].measures[measure].steps, step);
          v = v * 18 + 1;   // so 0->1  and 7->127
          hardware.SendMidiNote(memory.midiChannel, memory.midiNotes[trackMap[track]], v);
        }
        soundingTracks[track] = true;
      }
    }
  }
}

void Quake::AllNotesOff() {
  for (uint8_t track = 0; track < TRACKS_PER_PATTERN; track++) {
    // send a note off for the track regardless of whether we think it was sounding
    hardware.SendMidiNote(memory.midiChannel, memory.midiNotes[track], 0);
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


/***** Copy/Clear ************************************************************/

// Copying
// Copy one object to another. Requires two taps (source and destination), so need to
// remember first tap and then decide what to do on second tap.
void Quake::Copying(digital_type type, uint8_t row, uint8_t column, uint8_t pressed) {
  if (!pressed) return;

  SERIALPRINTLN("Q:Copying, type=" + String(type) + ", r=" + String(row) + ", c=" + String(column));
  
  if (!copyingFirst) {
    SERIALPRINTLN("    First!");
    copyDigital.type = type;
    copyDigital.row = row;
    copyDigital.column = column;
    copyingFirst = true;
    return;
  }

  // first and second tap have to be for same kind of thing
  if (copyDigital.type != type || copyDigital.row != row) {
    copying = false;
    copyingFirst = false;
    return;
  }

  SERIALPRINTLN("    Second!");
  if (type == BUTTON && row == PATTERN_ROW) {
    // copy pattern
    // TODO: this
    // Draw(true);
  } else if (type == GRID && row == TRACK_SELECT_ROW && copyDigital.column == column) {
    // copy first measure of track to other measures
    CopyTrackMeasures(column);
  } else if (type == GRID && row == TRACK_SELECT_ROW) {
    // copy track
    SERIALPRINTLN("    Copying track");
    CopyTrack(copyDigital.column, column);
    Draw(true);
  } else if (type == GRID && row == MODE_ROW 
              && column >= MEASURE_SELECT_MIN_COLUMN && column <= MEASURE_SELECT_MAX_COLUMN 
              && copyDigital.column >= MEASURE_SELECT_MIN_COLUMN && copyDigital.column <= MEASURE_SELECT_MAX_COLUMN) {
    // copy measure
    SERIALPRINTLN("    Copying measure");
    if (copyDigital.column == column) return;  // don't copy measure to itself
    CopyMeasure(copyDigital.column - MEASURE_SELECT_MIN_COLUMN, column - MEASURE_SELECT_MIN_COLUMN);
    Draw(true); 
  }

  copying = false;
  copyingFirst = false;
  
}

void Quake::Clearing(digital_type type, uint8_t row, uint8_t column, uint8_t pressed) {
  if (!pressed) return;
  
  if (type == BUTTON && row == PATTERN_ROW) {
    // clear pattern
    // TODO: this resets the current in-memory pattern regardless of which pattern button you push
    ResetCurrentPattern(); 
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

void Quake::CopyTrack(u8 sourceTrack, u8 destTrack) {
  for (uint8_t measure = 0; measure < MEASURES_PER_PATTERN; measure++) {
    for (uint8_t step = 0; step < STEPS_PER_MEASURE; step++) {
      u8 nib = NibArray16_Get(currentPattern->tracks[sourceTrack].measures[measure].steps, step); // get the flag and value
      NibArray16_Set(currentPattern->tracks[destTrack].measures[measure].steps, step, nib); // set the flag and value
    }      
  }
}

void Quake::CopyMeasure(u8 sourceMeasure, u8 destMeasure) {
  for (uint8_t track = 0; track < TRACKS_PER_PATTERN; track++) {
    for (uint8_t step = 0; step < STEPS_PER_MEASURE; step++) {
      u8 nib = NibArray16_Get(currentPattern->tracks[track].measures[sourceMeasure].steps, step); // get the flag and value
      NibArray16_Set(currentPattern->tracks[track].measures[destMeasure].steps, step, nib); // set the flag and value
    }      
  }
}

// CopyTrackMeasures
// Copies first measure of a track to all other measures.
void Quake::CopyTrackMeasures(u8 trackIndex) {
  for (uint8_t step = 0; step < STEPS_PER_MEASURE; step++) {
    u8 nib = NibArray16_Get(currentPattern->tracks[trackIndex].measures[0].steps, step); // get the flag and value
    NibArray16_Set(currentPattern->tracks[trackIndex].measures[1].steps, step, nib); // set the flag and value
    NibArray16_Set(currentPattern->tracks[trackIndex].measures[2].steps, step, nib); // set the flag and value
    NibArray16_Set(currentPattern->tracks[trackIndex].measures[3].steps, step, nib); // set the flag and value
  }
}

void Quake::ResetCurrentPattern() {
  // SERIALPRINTLN("Quake::ResetPattern, p=" + String(patternIndex));
  for (uint8_t track = 0; track < TRACKS_PER_PATTERN; track++) {
    for (uint8_t measure = 0; measure < MEASURES_PER_PATTERN; measure++) {
      for (uint8_t step = 0; step < STEPS_PER_MEASURE; step++) {
        NibArray16_Set(currentPattern->tracks[track].measures[measure].steps, step, 0); // set the flag and value both to 0
      }      
    }
  }
  currentPattern->autofillIntervalSetting = -1;
  currentPattern->autofillType = PATTERN;
  currentPattern->measureMode = 0;
}

void Quake::ResetTrack(uint8_t trackIndex) {
  for (uint8_t measure = 0; measure < MEASURES_PER_PATTERN; measure++) {
    for (uint8_t step = 0; step < STEPS_PER_MEASURE; step++) {
      NibArray16_Set(currentPattern->tracks[trackIndex].measures[measure].steps, step, 0); // set the flag and value both to 0
    }      
  }
}

void Quake::ResetMeasure(uint8_t measureIndex) {
  for (uint8_t track = 0; track < TRACKS_PER_PATTERN; track++) {
    for (uint8_t step = 0; step < STEPS_PER_MEASURE; step++) {
      NibArray16_Set(currentPattern->tracks[track].measures[measureIndex].steps, step, 0); // set the flag and value both to 0
    }      
  }
}


/***** Misc ************************************************************/

void Quake::ResetTrackEnabled() {
  for (uint8_t track = 0; track < TRACKS_PER_PATTERN; track++) {
    BitArray16_Set(memory.trackEnabled, track, true);
  }
  selectedTrack = 0;
}

// void Quake::ResetPattern(uint8_t patternIndex) {
//   // SERIALPRINTLN("Quake::ResetPattern, p=" + String(patternIndex));
//   for (uint8_t track = 0; track < TRACKS_PER_PATTERN; track++) {
//     for (uint8_t measure = 0; measure < MEASURES_PER_PATTERN; measure++) {
//       for (uint8_t step = 0; step < STEPS_PER_MEASURE; step++) {
//         memory.patterns[patternIndex].tracks[track].measures[measure].steps[step] = 0;
//       }      
//     }
//   }
// }

bool Quake::isFill(uint8_t measure) {
  return measure > currentPattern->measureMode;
}

void Quake::NextMeasure(uint8_t measureCounter) {

  // if there's a next pattern queued up, switch to it and finish
  if (nextPatternIndex != -1) {
    memory.currentPatternIndex = nextPatternIndex;
    nextPatternIndex = -1;
    Pattern *tmp = currentPattern;
    currentPattern = nextPattern;
    nextPattern = tmp;
    currentMeasure = 0;
    return;
  }

  if (autofillPlaying) {
    currentMeasure = 0;
    autofillPlaying = false;
    for (int t = 0; t < STEPS_PER_MEASURE; t++) {
      patternMap[t] = t;
    }
  } else if (currentPattern->measureMode == 2) {
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
    currentMeasure = (currentMeasure + 1) % (currentPattern->measureMode + 1);
  }

  if (currentPattern->autofillIntervalSetting >= 0 && currentPattern->autofillIntervalSetting < NUM_AUTOFILL_INTERVALS)  {
    int i = autofillIntervals[currentPattern->autofillIntervalSetting];
    if (measureCounter % i == i - 1) {
      int r = random(2);
      if (currentPattern->autofillType == PATTERN || (currentPattern->autofillType == BOTH && r == 0)) {
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

  // if current measure gets misaligned, this will bring it back
  if (measureCounter % 4 == 0) {
    currentMeasure = 0;
  }
}

int Quake::RandomFillPattern() {
  int range = MEASURES_PER_PATTERN - currentPattern->measureMode - 1;
  if (range == 0) return -1;
  return currentPattern->measureMode + 1 + random(range);
}

void Quake::SelectAlgorithmicFill() {
  const int *fillPattern = Fill::ChooseFillPattern();
  for (int t = 0; t < STEPS_PER_MEASURE; t++) {
    patternMap[t] = fillPattern[t];
  }
}

void Quake::SavePattern() {
  uint32_t offset = sizeof(memory) + memory.currentPatternIndex * sizeof(*currentPattern);
  hachi.saveModuleMemory(this, offset, sizeof(*currentPattern), (byte*)currentPattern);
}

void Quake::LoadPattern() {
  uint32_t offset = sizeof(memory) + nextPatternIndex * sizeof(*currentPattern);
  hachi.loadModuleMemory(this, offset, sizeof(*nextPattern), (byte*)nextPattern);
}

void Quake::SaveSettings() {
  hachi.saveModuleMemory(this, 0, sizeof(memory), (byte*)&memory);
}

void Quake::LoadSettings() {
  hachi.loadModuleMemory(this, 0, sizeof(memory), (byte*)&memory);
}

void Quake::Save() {
  SaveSettings();
  // Only the current pattern is kept in memory, so we only ever need to save that one pattern
  SavePattern();
}

void Quake::Load() {
  LoadSettings();
  // Only the current pattern is kept in memory, so we only ever need to load that one pattern
  LoadPattern();
}


void Quake::SetMidiNotes() {
  u8 midiNotes[TRACKS_PER_PATTERN] = { 36, 37, 38, 39, 40, 41, 43, 45, 42, 46, 44, 49, 47, 48, 50, 51 };
  for (int i = 0; i < TRACKS_PER_PATTERN; i++) {
    memory.midiNotes[i] = midiNotes[i];
  }
}



