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
  SERIALPRINTLN("Flow::Init idx=" + String(index) + ", memsize=" + sizeof(memory));
  this->index = index;
  this->display = display;

  for (int pattern = 0; pattern < PATTERN_COUNT; pattern++) {
    ClearPattern(pattern);
  }

  // Draw(true);
}

void Flow::SetColors(uint8_t primaryColor, uint8_t primaryDimColor) {
  
}

uint32_t Flow::GetMemSize() {
  return sizeof(memory);
}

uint8_t Flow::GetIndex() {
  return index;
}

bool Flow::IsMuted() {
  return true;
}

void Flow::SetMuted(bool muted) {
}


/***** Clock ************************************************************/

void Flow::Start() {
}

void Flow::Stop() {
}

void Flow::Pulse(uint16_t measureCounter, uint16_t sixteenthCounter, uint16_t pulseCounter) {
}


/***** Events ************************************************************/

void Flow::GridEvent(uint8_t row, uint8_t column, uint8_t pressed) {
  if (!pressed) return;

  // setting a marker
  u8 previous = GetPatternGrid(currentPatternIndex, row, StageIndex(column));
  bool turn_on = (previous != currentMarker);

  // remove the old marker that was at this row
  UpdateStage(&stages[column], row, column, previous, false);

  if (turn_on) {
    SetPatternGrid(currentPatternIndex, row, StageIndex(column), currentMarker);
    display->setGrid(row, StageIndex(column), marker_colors[currentMarker]);
    // add the new marker
    UpdateStage(&stages[StageIndex(column)], row, column, currentMarker, true);
  } else {
    SetPatternGrid(currentPatternIndex, row, StageIndex(column), OFF_MARKER);
    display->setGrid(row, StageIndex(column), marker_colors[OFF_MARKER]);
  }

  PrintStage(&stages[StageIndex(column)]);

  DrawStages(false);
}

void Flow::ButtonEvent(uint8_t row, uint8_t column, uint8_t pressed) {
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

}


/***** UI display methods ************************************************************/

uint8_t Flow::getColor() {
  return BRT_PURPLE;
}

uint8_t Flow::getDimColor() {
  return DIM_PURPLE;
}


/***** Drawing methods ************************************************************/

void Flow::Draw(bool update) {
  // SERIALPRINTLN("Flow::Draw")
  // display->FillModule(ABS_BLACK);
  DrawPalette(false);
  DrawStages(false);

  if (update) display->Update();
}

void Flow::DrawPalette(bool update) {
  // SERIALPRINTLN("Flow::DrawPalette")
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

void Flow::DrawStages(bool update) {
  // SERIALPRINTLN("Flow::DrawStages")
  for (int stage = 0; stage < STAGE_COUNT; stage++) {
    for (int row = 0; row < ROW_COUNT; row++) {
      u8 marker = GetPatternGrid(currentPatternIndex, row, stage);
      if (marker < MARKER_COUNT) {
        display->setGrid(row, stage, marker_colors[marker]);
      }
    }
  }

  // draw the pattern-mod stage
  for (int i = 0; i < ROW_COUNT; i++) {
    u8 marker = memory.patterns[currentPatternIndex].grid[i][PATTERN_MOD_STAGE];
    if (marker < MARKER_COUNT) {
      display->setButton(PATTERN_MOD_ROW, i, marker_colors[marker]);
    }
  }

  if (update) display->Update();
}

void Flow::DrawTracksEnabled(Display *useDisplay, uint8_t gridRow) {

}



/***** MIDI ************************************************************/


/***** Misc ************************************************************/

void Flow::ClearPattern(int patternIndex) {
  for (int stage = 0; stage < STAGE_COUNT; stage++) {
    for (int row = 0; row < ROW_COUNT; row++) {
      memory.patterns[patternIndex].grid[row][stage] = OFF_MARKER;
    }
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
//   the column number, not the mapped value (stage_index)
void Flow::UpdateStage(Stage *stage, u8 row, u8 column, u8 marker, bool turn_on) {

	s8 inc = turn_on ? 1 : -1;

	switch (marker) {
		case NOTE_MARKER:
			if (turn_on && stage->note_count > 0) {
				u8 oldNote = stage->note;
				stage->note_count--;
				stage->note = OUT_OF_RANGE;
        SetPatternGrid(currentPatternIndex, oldNote, StageIndex(column), OFF_MARKER);
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
			stage->accidental += inc;
			break;

		case FLAT_MARKER:
			stage->accidental -= inc;
			break;

		case OCTAVE_UP_MARKER:
			stage->octave += inc;
			break;

		case OCTAVE_DOWN_MARKER:
			stage->octave -= inc;
			break;

		case VELOCITY_UP_MARKER:
			stage->velocity += inc;
			break;

		case VELOCITY_DOWN_MARKER:
			stage->velocity -= inc;
			break;

		case EXTEND_MARKER:
			stage->extend += inc;
			break;

		case REPEAT_MARKER:
			stage->repeat += inc;
			break;

		case TIE_MARKER:
			stage->tie += inc;
			break;

		case LEGATO_MARKER:
			stage->legato += inc;
			break;

		case SKIP_MARKER:
			stage->skip += inc;
			break;

		case RANDOM_MARKER:
			stage->random += inc;
			break;
	}
}

void PrintStage(Stage *stage) {
  SERIALPRINT("Stage: note=" + String(stage->note) + ", nc=" + String(stage->note_count) + ", acc=" + String(stage->accidental));
  SERIALPRINT(", oct=" + String(stage->octave) + ", v=" + String(stage->velocity));
  SERIALPRINT(", ext=" + String(stage->extend) + ", rpt=" + String(stage->repeat));
  SERIALPRINT(", tie=" + String(stage->tie) + ", skip=" + String(stage->skip));
  SERIALPRINTLN(", leg=" + String(stage->legato) + ", rnd=" + String(stage->random));
}
