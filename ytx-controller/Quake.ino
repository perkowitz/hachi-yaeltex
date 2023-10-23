#include "headers/Quake.h"
#include "headers/Hachi.h"

Quake::Quake() {

}

void Quake::Init() {
  Reset();
  ResetPattern(currentPattern);
  SERIALPRINTLN("Quake::Init pat_size=" + String(PATTERN_MEM_SIZE));
  SERIALPRINTLN("     pat_size=" + String(sizeof(patterns[0])));
  Draw(true);
}


/***** Clock ************************************************************/

void Quake::Start() {
  currentMeasure = currentStep = 0;
  ClearSoundingNotes();
  Draw(true);
}

void Quake::Stop() {
  currentMeasure = currentStep = 0;
  AllNotesOff();
  Draw(true);
}

void Quake::Pulse(uint16_t measureCounter, uint16_t sixteenthCounter, uint16_t pulseCounter) {
  if (pulseCounter % PULSES_16TH == 0) {
    currentStep = (currentStep + 1) % STEPS_PER_MEASURE;
    if (sixteenthCounter == 0 && measureReset == 1) {
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
    trackEnabled[column] = !trackEnabled[column];
    DrawTracks(true);
  } else if (row == TRACK_SELECT_ROW) {
    // select track for editing
    selectedTrack = column;
    DrawTracks(false);
    DrawMeasures(true);
  } else if (row == MODE_ROW && column >= MEASURE_MODE_MIN_COLUMN && column <= MEASURE_MODE_MAX_COLUMN) {
    // set the playback mode for measures
    measureMode = column - MEASURE_MODE_MIN_COLUMN;
    Draw(true);
  } else if (row >= FIRST_MEASURES_ROW && row < FIRST_MEASURES_ROW + MEASURES_PER_PATTERN) {
    // edit steps in the pattern
    uint8_t measure = row - FIRST_MEASURES_ROW;
    selectedStep = column;
    selectedMeasure = measure;
    int8_t v = currentPattern.tracks[selectedTrack].measures[measure].steps[column];
    if (v == 0) {
      v = INITIAL_VELOCITY;
    } else {
      v *= -1;
    }
    currentPattern.tracks[selectedTrack].measures[measure].steps[column] = v;
    DrawMeasures(true);
  }

}

void Quake::ButtonEvent(uint8_t row, uint8_t column, uint8_t pressed) {
  // SERIALPRINTLN("Quake::ButtonEvent row=" + String(row) + ", col=" + String(column) + ", pr=" + String(pressed));

  uint8_t index = hardware.toDigital(BUTTON, row, column);
  if (index == QUAKE_SAVE_BUTTON) {
    // SERIALPRINTLN("    ..QBE, save button");
    if (pressed) {
      // SERIALPRINTLN("    ..QBE, pressed");
      hardware.setByIndex(QUAKE_SAVE_BUTTON, SAVE_ON_COLOR);
      hachi.savePatternData(0, 0, sizeof(currentPattern), (byte*)&currentPattern);
    } else {
      // SERIALPRINTLN("    ..QBE, released");
      hardware.setByIndex(QUAKE_SAVE_BUTTON, SAVE_OFF_COLOR);
    }
  } else if (index == QUAKE_LOAD_BUTTON) {
    // SERIALPRINTLN("    ..QBE, load button");
    if (pressed) {
      // SERIALPRINTLN("    ..QBE, pressed");
      hardware.setByIndex(QUAKE_LOAD_BUTTON, LOAD_ON_COLOR);
      hachi.loadPatternData(0, 0, sizeof(currentPattern), (byte*)&currentPattern);
      DrawMeasures(true);
    } else {
      // SERIALPRINTLN("    ..QBE, released");
      hardware.setByIndex(QUAKE_LOAD_BUTTON, LOAD_OFF_COLOR);
    }
  } else if (index == QUAKE_TEST_BUTTON) {
    // SERIALPRINTLN("    ..QBE, test button");
    if (pressed) {
      // SERIALPRINTLN("    ..QBE, pressed");
      hardware.setByIndex(QUAKE_TEST_BUTTON, ON_COLOR);
      // TestReadWrite(true);
    } else {
      // SERIALPRINTLN("    ..QBE, released");
      hardware.setByIndex(QUAKE_TEST_BUTTON, OFF_COLOR);
      // TestReadWrite(false);
    }
  } else if (row == VELOCITY_ROW) {
    if (pressed) {
      int velocity = toVelocity(column);
      // SERIALPRINTLN("ButtonEvent: row=" + String(row) + ", col=" + String(column) + ", vel=" + String(velocity));
      currentPattern.tracks[selectedTrack].measures[selectedMeasure].steps[selectedStep] = velocity;
      DrawMeasures(true);
    }
  }
}

void Quake::KeyEvent(uint8_t column, uint8_t pressed) {

}


/***** Drawing methods ************************************************************/

void Quake::Draw(bool update) {
  DrawTracks(false);
  DrawMeasures(false);
  DrawButtons(false);

  if (update) receiver.Update();
}

void Quake::DrawTracks(bool update) {
  // if (receiver == nullptr) return;

  for (int i=0; i < TRACKS_PER_PATTERN; i++) {
    uint8_t color = TRACK_ENABLED_OFF_COLOR;
    if (trackEnabled[i]) {
      color = TRACK_ENABLED_ON_COLOR;
      if (soundingTracks[i]) {
        color = TRACK_ENABLED_ON_PLAY_COLOR;
      }
    } else if (soundingTracks[i]) {
      // TODO: this currently doesn't light up, because soundTracks isn't set if track is muted
      color = TRACK_ENABLED_OFF_PLAY_COLOR;
    }
    receiver.setGrid(TRACK_ENABLED_ROW, i, color);

    color = TRACK_SELECT_OFF_COLOR;
    if (i == selectedTrack) {
      color = TRACK_SELECT_SELECTED_COLOR;
    }
    receiver.setGrid(TRACK_SELECT_ROW, i, color);
  }

  receiver.setGrid(CLOCK_ROW, currentStep, ON_COLOR);
  receiver.setGrid(CLOCK_ROW, (currentStep + STEPS_PER_MEASURE - 1) % STEPS_PER_MEASURE, ABS_BLACK);

  if (update) receiver.Update();
}

void Quake::DrawMeasures(bool update) {
  // if (receiver == nullptr) return;

  for (int m = 0; m < MEASURES_PER_PATTERN; m++) {
    for (int i=0; i < STEPS_PER_MEASURE; i++) {
      uint8_t row = FIRST_MEASURES_ROW + m;
      uint8_t color = STEPS_OFF_COLOR;
      if (currentPattern.tracks[selectedTrack].measures[m].steps[i] > 0) {
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
      receiver.setGrid(row, i, color);
    }
  }


  int velocity = currentPattern.tracks[selectedTrack].measures[selectedMeasure].steps[selectedStep] / VELOCITY_LEVELS;
  int vMapped = fromVelocity(velocity);
  // SERIALPRINTLN("DrawMeasures: velo=" + String(velocity) + ", vM=" + String(vMapped));
  for (int i = 0; i < VELOCITY_LEVELS; i++) {
    uint8_t color = VELOCITY_OFF_COLOR;
    if (vMapped == i) {
      color = VELOCITY_ON_COLOR;
    }
    receiver.setButton(VELOCITY_ROW, i, color);
  }

  if (update) receiver.Update();
}

void Quake::DrawButtons(bool update) {
  hardware.setByIndex(QUAKE_LOAD_BUTTON, LOAD_OFF_COLOR);
  hardware.setByIndex(QUAKE_SAVE_BUTTON, SAVE_OFF_COLOR);
  hardware.setByIndex(QUAKE_TEST_BUTTON, OFF_COLOR);

  for (int m = 0; m < MEASURES_PER_PATTERN; m++) {
    uint8_t color = MEASURE_SELECT_OFF_COLOR;
    if (m == currentMeasure) {
      color = MEASURE_SELECT_PLAYING_COLOR;
    } else if (m == selectedMeasure) {
      // not using this right now
      // color = MEASURE_SELECT_SELECTED_COLOR;
    }
    hardware.setGrid(MODE_ROW, MEASURE_SELECT_MIN_COLUMN + m, color);
  }

  for (int column = MEASURE_MODE_MIN_COLUMN; column <= MEASURE_MODE_MAX_COLUMN; column++) {
    uint8_t color = MEASURE_MODE_OFF_COLOR;
    if (column - MEASURE_MODE_MIN_COLUMN == measureMode) {
      color = MEASURE_MODE_ON_COLOR;
    }
    hardware.setGrid(MODE_ROW, column, color);
  }

  if (update) hardware.Update();
}
    


/***** MIDI ************************************************************/

void Quake::SendNotes() {
  for (uint8_t track = 0; track < TRACKS_PER_PATTERN; track++) {
    // send a note off if the track was previously sounding
    if (soundingTracks[track]) {
      hardware.SendMidiNote(midiChannel, midi_notes[track], 0);
      soundingTracks[track] = false;
    }

    // send a note if one is set
    if (trackEnabled[track]) {
      int8_t v = currentPattern.tracks[track].measures[currentMeasure].steps[currentStep]; 
      if (v > 0) {
        hardware.SendMidiNote(midiChannel, midi_notes[track], v);
        soundingTracks[track] = true;
      }
    }
  }
}

void Quake::AllNotesOff() {
  for (uint8_t track = 0; track < TRACKS_PER_PATTERN; track++) {
    // send a note off for the track regardless of whether we think it was sounding
    hardware.SendMidiNote(midiChannel, midi_notes[track], 0);
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
    trackEnabled[track] = true;
  }
  selectedTrack = 0;
}

void Quake::ResetPattern(Pattern pattern) {
  for (uint8_t track = 0; track < TRACKS_PER_PATTERN; track++) {
    for (uint8_t measure = 0; measure < MEASURES_PER_PATTERN; measure++) {
      for (uint8_t step = 0; step < STEPS_PER_MEASURE; step++) {
        pattern.tracks[track].measures[measure].steps[step] = 0;
      }      
    }
  }
}

bool Quake::isFill(uint8_t measure) {
  return measure > measureMode;
}

void Quake::NextMeasure(uint8_t measureCounter) {
  if (measureMode == 2) {
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
    currentMeasure = (currentMeasure + 1) % (measureMode + 1);
  }
}



