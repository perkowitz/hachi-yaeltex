#include "headers/Flow.h"
#include "headers/Hachi.h"
#include "headers/display.h"

static uint8_t marker_colors[MARKER_COUNT] = {
  ABS_BLACK, BRT_SKY_BLUE, BRT_CYAN, DIM_CYAN,  // off, note, sharp, flat
  BRT_ORANGE, DIM_ORANGE, BRT_GREEN, DIM_GREEN, // octave up, octave down, velo up, velo down
  DIM_BLUE, BRT_PINK, BRT_PURPLE, BRT_RED,      // extend, repeat, tie, skip
  DK_GRAY, BRT_YELLOW                           // legato, random
};



Flow::Flow() {
}

void Flow::Init(uint8_t index, Display *display) {
  SERIALPRINTLN("Flow::Init idx=" + String(index) + ", memsize=" + sizeof(memory) + ", freemem=" + String(FreeMemory()));
  this->index = index;
  this->display = display;

  for (int pattern = 0; pattern < F_PATTERN_COUNT; pattern++) {
    ClearPattern(pattern);
  }

  for (int stage = 0; stage < STAGE_COUNT; stage++) {
    stagesEnabled[stage] = true;
    stagesSkipped[stage] = false;
  }

  // Draw(true);
}

void Flow::SetColors(uint8_t primaryColor, uint8_t primaryDimColor) {
  this->primaryColor = primaryColor;
  this->primaryDimColor = primaryDimColor;    
}

uint32_t Flow::GetMemSize() {
  return sizeof(memory);
}

uint8_t Flow::GetIndex() {
  return index;
}

bool Flow::IsMuted() {
  return muted;
}

void Flow::SetMuted(bool muted) {
  NoteOff();
  this->muted = muted;
}



/***** Clock ************************************************************/

void Flow::Start() {
  NoteOff();
  currentStageIndex = currentRepeat = currentExtend = currentNote = 0;  
}

void Flow::Stop() {
  NoteOff();
  currentStageIndex = currentRepeat = currentExtend = currentNote = 0;  
}

void Flow::Pulse(uint16_t measureCounter, uint16_t sixteenthCounter, uint16_t pulseCounter) {

  if (pulseCounter % PULSES_16TH == 0) {

		// update water if it's on
		// if (water_on) {
		// 	draw_water();
		// }

    // if a new pattern is queued, load it at the start of the measure
    if (sixteenthCounter == 0 && nextPatternIndex != -1) {
      memory.currentPatternIndex = nextPatternIndex;
      nextPatternIndex = -1;
			currentStageIndex = currentRepeat = currentExtend = 0;
      LoadStages(memory.currentPatternIndex);
      DrawPatterns(false);
    } else if ((sixteenthCounter == 0 && memory.measureReset == 1) ||
				(sixteenthCounter == 0 && memory.measureReset > 1 && measureCounter % memory.measureReset == 0)) {
				// (currentStageIndex == 0 && memory.measureReset == 0)) {
      // reset at the beginning of the measure (if measureReset is set)
      // reset=1: reset every measure; reset=2-4: every that many measures
      // reset=0: never reset (ie reset at end of pattern)
			currentStageIndex = currentRepeat = currentExtend = 0;
		}

    // if ((sixteenthCounter == 0 && memory.measureReset == 1) ||
		// 		(sixteenthCounter == 0 && memory.measureReset > 1 && measureCounter % memory.measureReset == 0)) {
		// 		// (currentStageIndex == 0 && memory.measureReset == 0)) {
    //   // reset at the beginning of the measure (if measureReset is set)
    //   // reset=1: reset every measure; reset=2-4: every that many measures
    //   // reset=0: never reset (ie reset at end of pattern)
		// 	currentStageIndex = currentRepeat = currentExtend = 0;
		// }

    DrawStages(true);

		// copy the stage and apply randomness to it if needed.
		Stage stage = stages[StageIndex(currentStageIndex)];
		// for (int i = 0; i < stage.random + stages[PATTERN_MOD_STAGE].random; i++) {
		// 	u8 r = rand() % RANDOM_MARKER_COUNT;
		// 	update_stage(&stage, 0, StageIndex(currentStageIndex), random_markers[r], true);
		// }

		// add in pattern mods (note & velocity mods computed later; random already computed)
		// stage.extend += stages[PATTERN_MOD_STAGE].extend;
		// stage.repeat += stages[PATTERN_MOD_STAGE].repeat;

		// if it's a tie or extension>0, do nothing
		// if it's legato, send previous note off after new note on
		// otherwise, send previous note off first
		// also highlight the current playing note on pads in PLAYING_NOTE_COLOR
		// and then turn it back to NOTE_MARKER when it stops
		if (stage.tie <= 0 && currentExtend == 0) {
			if (stage.legato <= 0 && currentNote != OUT_OF_RANGE 
          || !stagesEnabled[StageIndex(currentStageIndex)]) {
				NoteOff();
				if (!inSettings) {
//					draw_pad(currentNoteRow, currentNoteColumn, NOTE_MARKER);
//					draw_stage(currentNoteColumn);
				}
			}

			u8 previous_note = currentNote;
			u8 previous_note_column = currentNoteColumn;
			if (stage.note_count > 0 && stage.note != OUT_OF_RANGE && stagesEnabled[StageIndex(currentStageIndex)]) {
				u8 n = GetNote(&stage);
        if (!muted) {
          hardware.SendMidiNote(memory.midiChannel, n, GetVelocity(&stage));
        }

				if (!inSettings) {
//					draw_pad(stage.note,currentStage, PLAYING_NOTE_COLOR);
				}
				currentNote = n;
				currentNoteRow = stage.note;
				currentNoteColumn = currentStageIndex;
			}

			if (stage.legato > 0 && previous_note != OUT_OF_RANGE) {
        if (!muted) {
          hardware.SendMidiNote(memory.midiChannel, previous_note, 0);
        }
				if (!inSettings) {
//					draw_pad(previous_note_row, previous_note_column, NOTE_MARKER);
//					draw_stage(previous_note_column);
				}
			}
		}

		// now increment current extension
		// if that would exceed the stage's extension count, increment the repeats count
		// if that would exceed the stage's repeat count, go to the next stage
		currentExtend++;
		if (currentExtend > stage.extend) {
			currentExtend = 0;
			currentRepeat++;
		}
		if (currentRepeat > stage.repeat) {
			currentRepeat = 0;
			previousStageIndex = currentStageIndex;
			currentStageIndex = (currentStageIndex + 1) % STAGE_COUNT;
			// if every stage is set to skip, it will replay the current stage
			while ((stages[StageIndex(currentStageIndex)].skip > 0 || stagesSkipped[StageIndex(currentStageIndex)])
            && currentStageIndex != previousStageIndex) {
				currentStageIndex = (currentStageIndex + 1) % STAGE_COUNT;
			}
//			if (currentStageIndex == 0) {
//				u8 np = get_continue_pattern();
//				if (np != memory.currentPatternIndex) {
//					change_pattern(np);
//				}
//				currentStageIndex = currentRepeat = currentExtend = 0;
//			}
		}

	}

}


/***** Events ************************************************************/

void Flow::GridEvent(uint8_t row, uint8_t column, uint8_t pressed) {
  if (!pressed) return;

 if (inSettings) {
    if (row == MIDI_CHANNEL_ROW) {
      NoteOff();
      memory.midiChannel = column + 1;  // midi channel is 1-indexed
      // SaveOrLoadSettings(SAVING);
      DrawSettings(true);
    } else if (row == F_SETTINGS_ROW && column >= F_RESET_START_COLUMN && column <= F_RESET_END_COLUMN) {
      if (column - F_RESET_START_COLUMN + 1 == memory.measureReset) {
        memory.measureReset = 0;
      } else {
        memory.measureReset = column - F_RESET_START_COLUMN + 1;
      }
      DrawSettings(true);
    }
    return;
  }

  if (inPerfMode) {
    if (row == F_STAGES_ENABLED_ROW) {
      stagesEnabled[column] = !stagesEnabled[column];
    } else if (row == F_STAGES_SKIPPED_ROW) {
      stagesSkipped[column] = !stagesSkipped[column];
    } else {
      currentStageIndex = column;
      currentExtend = currentRepeat = 0;
    }
  } else {
    // setting a marker
    // SERIALPRINTLN("Flow::GridEvent, set a marker, stage=" + String(column));
    u8 previous = GetPatternGrid(memory.currentPatternIndex, row, StageIndex(column));
    bool turn_on = (previous != currentMarker);

    // SERIALPRINTLN("BEFORE:")
    // SERIALPRINT("  This stage: ");
    // PrintStage(column, &stages[StageIndex(column)]);
    // SERIALPRINT("  Extra stage: ");
    // PrintStage(PATTERN_MOD_STAGE, &stages[PATTERN_MOD_STAGE]);

    // remove the old marker that was at this row
    UpdateStage(&stages[column], row, column, previous, false);

    if (turn_on) {
      SetPatternGrid(memory.currentPatternIndex, row, StageIndex(column), currentMarker);
      display->setGrid(row, StageIndex(column), marker_colors[currentMarker]);
      // add the new marker
      UpdateStage(&stages[StageIndex(column)], row, column, currentMarker, true);
    } else {
      SetPatternGrid(memory.currentPatternIndex, row, StageIndex(column), OFF_MARKER);
      display->setGrid(row, StageIndex(column), marker_colors[OFF_MARKER]);
    }

    // SERIALPRINTLN("AFTER:")
    // SERIALPRINT("  This stage: ");
    // PrintStage(column, &stages[StageIndex(column)]);
    // SERIALPRINT("  Extra stage: ");
    // PrintStage(PATTERN_MOD_STAGE, &stages[PATTERN_MOD_STAGE]);

    // // PrintStage(column, &stages[StageIndex(column)]);
    // SERIALPRINTLN("*************\n");
  }

  DrawStages(false);
}

void Flow::ButtonEvent(uint8_t row, uint8_t column, uint8_t pressed) {
  u8 index = hardware.toDigital(BUTTON, row, column);

  if (index == F_PERF_MODE_BUTTON) {
    if (pressed) {
      inPerfMode = !inPerfMode;
      Draw(true);
    }
  } else if (index == F_SETTINGS_BUTTON) {
    if (pressed) {
      inSettings = !inSettings;
      if (inSettings) {
        DrawSettings(false);
      }
      Draw(true);
    }
  } else if (index == F_SAVE_BUTTON) {
    if (pressed) {
      display->setByIndex(F_SAVE_BUTTON, H_SAVE_ON_COLOR);
      Save();
    } else {
      display->setByIndex(F_SAVE_BUTTON, H_SAVE_OFF_COLOR);
    }
  } else if (index == F_LOAD_BUTTON) {
    if (pressed) {
      display->setByIndex(F_LOAD_BUTTON, H_LOAD_ON_COLOR);
      Load();
    } else {
      display->setByIndex(F_LOAD_BUTTON, H_LOAD_OFF_COLOR);
    }
  } else if (row == PATTERN_ROW && column < F_PATTERN_COUNT) {
    if (pressed) {
      nextPatternIndex = column;
      DrawPatterns(true);
    }
  }
}

void Flow::KeyEvent(uint8_t column, uint8_t pressed) {
  if (!pressed) return;

  switch (column) {
    case 0:
      // indicator
      break;
    case 1:
      currentMarker = OFF_MARKER;
      break;
    case 2:
      currentMarker = NOTE_MARKER;
      break;
    case 3:
      if (currentMarker == SHARP_MARKER) {
        currentMarker = FLAT_MARKER;
      } else {
        currentMarker = SHARP_MARKER;
      }
      break;
    case 4:
      if (currentMarker == OCTAVE_UP_MARKER) {
        currentMarker = OCTAVE_DOWN_MARKER;
      } else {
        currentMarker = OCTAVE_UP_MARKER;
      }
      break;
    case 5:
      if (currentMarker == VELOCITY_UP_MARKER) {
        currentMarker = VELOCITY_DOWN_MARKER;
      } else {
        currentMarker = VELOCITY_UP_MARKER;
      }
      break;
    case 6:
      currentMarker = EXTEND_MARKER;
      break;
    case 7:
      currentMarker = REPEAT_MARKER;
      break;
    case 8:
      currentMarker = TIE_MARKER;
      break;
    case 9:
      currentMarker = SKIP_MARKER;
      break;
    case 10:
      currentMarker = LEGATO_MARKER;
      break;
    case 11:
      currentMarker = RANDOM_MARKER;
      break;
  }

  display->setKey(0, marker_colors[currentMarker]);
}

void Flow::ToggleTrack(uint8_t trackNumber) {
  stagesEnabled[trackNumber] = !stagesEnabled[trackNumber];
}


/***** UI display methods ************************************************************/

uint8_t Flow::getColor() {
  return primaryColor;
}

uint8_t Flow::getDimColor() {
  return primaryDimColor;
}


/***** Drawing methods ************************************************************/

void Flow::Draw(bool update) {
  // SERIALPRINTLN("Flow::Draw")
  display->FillModule(ABS_BLACK, false, true, false);
  DrawPalette(false);
  DrawPatterns(false);
  DrawButtons(false);
  if (inSettings) {
    DrawSettings(false);
  } else {
    DrawStages(false);
  }

  if (update) display->Update();
}

void Flow::DrawPalette(bool update) {
  // SERIALPRINTLN("Flow::DrawPalette")
  if (inPerfMode) {
    for (int key = 0; key < KEY_COLUMNS; key++) {
      display->setKey(key, ABS_BLACK);
    }
  } else {
    display->setKey(0, marker_colors[currentMarker]);
    display->setKey(1, marker_colors[OFF_MARKER]);
    display->setKey(2, marker_colors[NOTE_MARKER]);
    display->setKey(3, marker_colors[SHARP_MARKER]);
    display->setKey(4, marker_colors[OCTAVE_UP_MARKER]);
    display->setKey(5, marker_colors[VELOCITY_UP_MARKER]);
    display->setKey(6, marker_colors[EXTEND_MARKER]);
    display->setKey(7, marker_colors[REPEAT_MARKER]);
    display->setKey(8, marker_colors[TIE_MARKER]);
    display->setKey(9, marker_colors[SKIP_MARKER]);
    display->setKey(10, marker_colors[LEGATO_MARKER]);
    display->setKey(11, marker_colors[RANDOM_MARKER]);

    if (update) display->Update();
  }
}

void Flow::DrawStages(bool update) {
  // SERIALPRINTLN("Flow::DrawStages")
  if (inSettings) return;

  for (int stage = 0; stage < STAGE_COUNT; stage++) {
    for (int row = 0; row < ROW_COUNT; row++) {
      u8 marker = GetPatternGrid(memory.currentPatternIndex, row, stage);
      u8 color = marker_colors[OFF_MARKER];
      if (marker < MARKER_COUNT) {
        color = marker_colors[marker];
      }

      // highlight the current stage
      if (stage == currentStageIndex && marker == OFF_MARKER) {
        color = primaryDimColor;
      } else if (stage == currentStageIndex && marker == NOTE_MARKER) {
        color = WHITE;
      // } else if (stage == currentStageIndex) {   // draw other markers with the module color
      //   color = primaryColor;
      }
      display->setGrid(row, stage, color);
    }

    if (inPerfMode) {
      for (int stage = 0; stage < STAGE_COUNT; stage++) {
        display->setGrid(F_STAGES_ENABLED_ROW, stage, stagesEnabled[stage] ? F_STAGE_ENABLED_ON_COLOR : F_STAGE_ENABLED_OFF_COLOR);
        display->setGrid(F_STAGES_SKIPPED_ROW, stage, stagesSkipped[stage] ? primaryDimColor : primaryColor);
      }
    }
  }

  // draw the pattern-mod stage
  for (int i = 0; i < ROW_COUNT; i++) {
    u8 marker = memory.patterns[memory.currentPatternIndex].grid[PATTERN_MOD_STAGE][i];
    if (marker < MARKER_COUNT) {
      display->setButton(PATTERN_MOD_STAGE, i, marker_colors[marker]);
    }
  }

  if (update) display->Update();
}

void Flow::DrawPatterns(bool update) {
  for (int p = 0; p < F_PATTERN_COUNT; p++) {
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

void Flow::DrawButtons(bool update) {
  display->setByIndex(F_PERF_MODE_BUTTON, inPerfMode ? PERF_COLOR : PERF_DIM_COLOR);
  display->setByIndex(F_SETTINGS_BUTTON, inSettings ? ON_COLOR : OFF_COLOR);
  display->setByIndex(F_SAVE_BUTTON, H_SAVE_OFF_COLOR);
  display->setByIndex(F_LOAD_BUTTON, H_LOAD_OFF_COLOR);

  if (update) display->Update();
}

void Flow::DrawSettings(bool update) {
  if (!inSettings) return;

  for (int row = 0; row < GRID_ROWS; row++) {
    for (int column = 0; column < GRID_COLUMNS; column++) {
      u8 color = ABS_BLACK;
      if (row == MIDI_CHANNEL_ROW) {
        color = OFF_COLOR;
        if (column == memory.midiChannel - 1) {   // midi channel is 1-indexed
          color = ON_COLOR;
        }
      } else if (row == F_SETTINGS_ROW && column >= F_RESET_START_COLUMN && column <= F_RESET_END_COLUMN) {
        color = OFF_COLOR;
        if (column - F_RESET_START_COLUMN + 1 == memory.measureReset) {   // reset is 1-indexed
          color = ON_COLOR;
        }
      }
      display->setGrid(row, column, color);
    }
  }

  if (update) display->Update();
}
    
void Flow::DrawTracksEnabled(Display *useDisplay, uint8_t gridRow) {
  for (int stage = 0; stage < STAGE_COUNT; stage++) {
    u8 color = ABS_BLACK;
    if (stagesEnabled[stage]) {
      color = VDK_GRAY;
      if (stages[stage].note_count > 0) {
        color = primaryDimColor;
      }
      if (stage == previousStageIndex) {   // pulse sets to next stage, so we're one ahead here
        if (stages[stage].note_count > 0) {
          color = BRT_YELLOW;
        } else if (stages[stage].tie > 0) {
          color = LT_GRAY;
        }
      }
    }
    useDisplay->setGrid(gridRow, stage, color);
  }
}



/***** MIDI ************************************************************/


/***** Misc ************************************************************/

void Flow::ClearPattern(int patternIndex) {
  for (int stage = 0; stage < STAGE_COUNT + 1; stage++) {   // include PATTERN_MOD_STAGE 
    for (int row = 0; row < ROW_COUNT; row++) {
      memory.patterns[patternIndex].grid[row][stage] = OFF_MARKER;
    }
  }

  // and do pattern mod stage
  for (int row = 0; row < ROW_COUNT; row++) {
    memory.patterns[patternIndex].grid[row][PATTERN_MOD_STAGE] = OFF_MARKER;
  }
}

u8 Flow::GetPatternGrid(u8 patternIndex, u8 row, u8 column) {
  return memory.patterns[patternIndex].grid[row][column];
}

void Flow::SetPatternGrid(u8 patternIndex, u8 row, u8 column, u8 value) {
  memory.patterns[patternIndex].grid[row][column] = value;
}

// TODO: will be used for shuffling
u8 Flow::StageIndex(u8 stage) {
  return stage;
}

// update_stage updates stage settings when a marker is changed.
//   if turn_on=true, a marker was added; if false, a marker was removed.
//   for some markers (NOTE), row placement matters.
// Note: the column param is where to draw any update; when shuffling, send
//   the column number, not the mapped value (StageIndex)
void Flow::UpdateStage(Stage *stage, u8 row, u8 column, u8 marker, bool turn_on) {
  // SERIALPRINTLN("Flow::UpdateStage: r=" + String(row) + ", c=" + String(column) + ", mk=" + String(marker) + ", on=" + String(turn_on));
  // SERIALPRINT("    ");
  // PrintStage(column, stage);

	s8 inc = turn_on ? 1 : -1;

	switch (marker) {
		case NOTE_MARKER:
      // SERIALPRINTLN("    NOTE");
			if (turn_on && stage->note_count > 0) {
				u8 oldNote = stage->note;
				stage->note_count--;
				stage->note = OUT_OF_RANGE;
        SetPatternGrid(memory.currentPatternIndex, oldNote, StageIndex(column), OFF_MARKER);
        display->setGrid(oldNote, column, marker_colors[OFF_MARKER]);
			}
			stage->note_count += inc;
			if (turn_on) {
				stage->note = row;
			} else {
				stage->note = OUT_OF_RANGE;
			}
			break;

		case SHARP_MARKER:
      // SERIALPRINTLN("    SHARP");
			stage->accidental += inc;
			break;

		case FLAT_MARKER:
      // SERIALPRINTLN("    FLAT");
			stage->accidental -= inc;
			break;

		case OCTAVE_UP_MARKER:
      // SERIALPRINTLN("    OCT UP");
			stage->octave += inc;
			break;

		case OCTAVE_DOWN_MARKER:
      // SERIALPRINTLN("    OCT DOWN");
			stage->octave -= inc;
			break;

		case VELOCITY_UP_MARKER:
      // SERIALPRINTLN("    VEL UP");
			stage->velocity += inc;
			break;

		case VELOCITY_DOWN_MARKER:
      // SERIALPRINTLN("    VEL DOWN");
			stage->velocity -= inc;
			break;

		case EXTEND_MARKER:
      // SERIALPRINTLN("    EXTEND");
			stage->extend += inc;
			break;

		case REPEAT_MARKER:
      // SERIALPRINTLN("    REPEAT");
			stage->repeat += inc;
			break;

		case TIE_MARKER:
      // SERIALPRINTLN("    TIE");
			stage->tie += inc;
			break;

		case LEGATO_MARKER:
      // SERIALPRINTLN("    LEGATO");
			stage->legato += inc;
			break;

		case SKIP_MARKER:
      // SERIALPRINTLN("    SKIP");
			stage->skip += inc;
			break;

		case RANDOM_MARKER:
      // SERIALPRINTLN("    RANDOM");
			stage->random += inc;
			break;
	}

  // SERIALPRINT("    ");
  // PrintStage(column, stage);
  // SERIALPRINTLN("------------------\n");
}

// update stages data info from a grid (e.g. when it's just been loaded from memory)
void Flow::LoadStages(int patternIndex) {
  for (int stage = 0; stage < STAGE_COUNT; stage++) {
    ClearStage(stage);
  	for (int row = 0; row < ROW_COUNT; row++) {
      UpdateStage(&stages[stage], row, stage, memory.patterns[patternIndex].grid[row][stage], true);
    }
  }
}

void Flow::ClearStage(int stage) {
  stages[stage].note = OUT_OF_RANGE;
  stages[stage].note_count = stages[stage].octave = stages[stage].velocity = stages[stage].accidental = 0;
  stages[stage].extend = stages[stage].repeat = stages[stage].tie = 0;
  stages[stage].legato = stages[stage].skip = stages[stage].random = 0;
}

u8 Flow::GetNote(Stage *stage) {
	if (stage->note == OUT_OF_RANGE) {
		return OUT_OF_RANGE;
	}
	// u8 o = DEFAULT_OCTAVE + stage->octave + stages[PATTERN_MOD_STAGE].octave;
	u8 o = DEFAULT_OCTAVE + stage->octave;
	u8 n = note_map[stage->note] + stage->accidental;
	// if (check_transpose() > 0) {
	// 	// when repeat transpose is on, we only transpose repeats
	// 	u8 transpose = note_map[(stages[PATTERN_MOD_STAGE].note == OUT_OF_RANGE ? 0 : stages[PATTERN_MOD_STAGE].note)];
	// 	transpose += stages[PATTERN_MOD_STAGE].accidental;
	// 	if (transpose == 0) {
	// 		transpose = 12;
	// 	}
	// 	n += currentRepeat * transpose;

	// } else {
	// 	// regular pattern mod: just add the mod note and accidental
	// 	n += (stages[PATTERN_MOD_STAGE].note == OUT_OF_RANGE ? 0 : stages[PATTERN_MOD_STAGE].note);
	// 	n += stages[PATTERN_MOD_STAGE].accidental;
	// }
	return o * 12 + n;
}

u8 Flow::GetVelocity(Stage *stage) {
	// s8 v = DEFAULT_VELOCITY + (stage->velocity + stages[PATTERN_MOD_STAGE].velocity) * VELOCITY_DELTA;
	s8 v = DEFAULT_VELOCITY + stage->velocity * VELOCITY_DELTA;
	v = v < 1 ? 1 : (v > 127 ? 127 : v);
	return v;
}

void PrintStage(u8 stageIndex, Stage *stage) {
  SERIALPRINT("Stage: idx=" + String(stageIndex) + ", note=" + String(stage->note) + ", nc=" + String(stage->note_count) + ", acc=" + String(stage->accidental));
  SERIALPRINT(", oct=" + String(stage->octave) + ", v=" + String(stage->velocity));
  SERIALPRINT(", ext=" + String(stage->extend) + ", rpt=" + String(stage->repeat));
  SERIALPRINT(", tie=" + String(stage->tie) + ", skip=" + String(stage->skip));
  SERIALPRINTLN(", leg=" + String(stage->legato) + ", rnd=" + String(stage->random));
}

// for mod row transpose at some point
u8 check_transpose() {
	return false;
}

void Flow::NoteOff() {
	if (currentNote != OUT_OF_RANGE) {
    hardware.SendMidiNote(memory.midiChannel, currentNote, 0);
		currentNote = OUT_OF_RANGE;
	}
}

void Flow::Save() {
  hachi.saveModuleMemory(this, 0, sizeof(memory), (byte*)&memory);
}

void Flow::Load() {
  hachi.loadModuleMemory(this, 0, sizeof(memory), (byte*)&memory);
  LoadStages(memory.currentPatternIndex);
}




