#include "headers/Hardware.h"

Hardware::Hardware() {

}

void Hardware::Init() {
  initialized = 1;
  // SERIALPRINTLN("Hardware::Init");
  ResetDrawing();
}

bool Hardware::getHachiEnabled(void) {
  return hachiEnabled;
}
bool Hardware::getControlEnabled(void) {
  return controlEnabled;
}
void Hardware::setHachiEnabled(bool enabled) {
  hachiEnabled = enabled;
}
void Hardware::setControlEnabled(bool enabled) {
  controlEnabled = enabled;
}

void Hardware::setGrid(uint16_t row, uint16_t column, uint16_t color) {
  // if (!initialized) Init();
  if (row >= GRID_ROWS || column >= GRID_COLUMNS) return;
  
  if (!DEBUG_NO_LEDS) {
    uint16_t dIndex = toDigital(GRID, row, column);
    if (currentValue[dIndex] == color) {
      // SERIALPRINTLN("Hardware::setGrid row=" + String(row) + ", col=" + String(column) + ", color=" + String(color) + ", dIdx=" + String(dIndex) + ", cV=" + String(currentValue[dIndex]) + " (NOPE)");
    } else {
      // SERIALPRINTLN("Hardware::setGrid row=" + String(row) + ", col=" + String(column) + ", color=" + String(color) + ", dIdx=" + String(dIndex) + ", cV=" + String(currentValue[dIndex]));
      currentValue[dIndex] = color;
      feedbackHw.SetChangeDigitalFeedback(dIndex, color, false, false, false, false, false);
    }
  }
}

void Hardware::setButton(uint16_t row, uint16_t column, uint16_t color) {
  // if (!initialized) Init();
  if (!DEBUG_NO_LEDS) {
    uint16_t dIndex = toDigital(BUTTON, row, column);
    if (currentValue[dIndex] != color) {
      // SERIALPRINTLN("Hardware::setButton row=" + String(row) + ", col=" + String(column) + ", color=" + String(color) + ", dIdx=" + String(dIndex) + ", cV=" + String(currentValue[dIndex]));
      feedbackHw.SetChangeDigitalFeedback(dIndex, color, false, false, false, false, false);
      // digitalHw.SetDigitalValue(currentBank, dIndex, color);
      currentValue[dIndex] = color;
    } else {
      // SERIALPRINTLN("Hardware::setButton row=" + String(row) + ", col=" + String(column) + ", color=" + String(color) + ", dIdx=" + String(dIndex) + ", cV=" + String(currentValue[dIndex]) + " (NOPE)");
    }
  } 
}

void Hardware::setKey(uint16_t column, uint16_t color) {
  // if (!initialized) Init();
  uint16_t dIndex = toDigital(KEY, 0, column);
  if (currentValue[dIndex] != color) {
    // SERIALPRINTLN("Drawing::setKey col=" + String(column) + ", col=" + String(color) + ", dIdx=" + String(dIndex));
    feedbackHw.SetChangeDigitalFeedback(dIndex, color, false, false, false, false, false); 
    // digitalHw.SetDigitalValue(currentBank, dIndex, color);
    currentValue[dIndex] = color;
  }
}

void Hardware::setByIndex(uint16_t index, uint16_t color) {
  // SERIALPRINTLN("SetByIndex1 v16=" + String(currentValue[16]));
  // if (!initialized) Init();
  // if (index != 152) {     // it's the clock button, so it gets updated a LOT
  //   SERIALPRINTLN("Drawing::setByIndex col=" + String(index) + ", col=" + String(color));
  // }
  if (currentValue[index] != color) {
    // SERIALPRINTLN("SetByIndex2 v16=" + String(currentValue[16]));
    // SERIALPRINTLN("Hardware::setByIndex idx=" + String(index) + ", color=" + String(color) + ", cV=" + String(currentValue[index]));
    feedbackHw.SetChangeDigitalFeedback(index, color, false, false, false, false, false); 
    // digitalHw.SetDigitalValue(currentBank, index, color);
    currentValue[index] = color;
  } else {
    // SERIALPRINTLN("SetByIndex3 v16=" + String(currentValue[16]));
    // SERIALPRINTLN("Hardware::setByIndex idx=" + String(index) + ", color=" + String(color) + ", cV=" + String(currentValue[index]) + " (NOPE)");
  }
  // SERIALPRINTLN("    cv[" + String(index) + "]=" + String(currentValue[index]));
  // SERIALPRINTLN("SetByIndex4 v16=" + String(currentValue[16]));

}

/***** encoders ********************************************************/

void Hardware::setEncoderColor(u16 index, u8 color) {
  feedbackHw.SetChangeEncoderFeedback(FB_ENCODER, index, color, encoderHw.GetModuleOrientation(index/4), 
    NO_SHIFTER, NO_BANK_UPDATE, COLOR_CHANGE, NO_VAL_TO_INT, EXTERNAL_FEEDBACK);
}

void Hardware::setEncoderAccentColor(u16 index, u8 color) {
  feedbackHw.SetChangeEncoderFeedback(FB_ENC_SWITCH, index, color, encoderHw.GetModuleOrientation(index/4), 
    NO_SHIFTER, NO_BANK_UPDATE, COLOR_CHANGE, NO_VAL_TO_INT, EXTERNAL_FEEDBACK);
}

void Hardware::setEncoderValue(u16 index, u8 value) {
  // feedbackHw.SetChangeEncoderFeedback(FB_ENCODER, index, value, encoderHw.GetModuleOrientation(index/4), 
  //   NO_SHIFTER, NO_BANK_UPDATE, NO_COLOR_CHANGE, NO_VAL_TO_INT, EXTERNAL_FEEDBACK);
  encoderHw.SetEncoderValue(0, index, value);   
}

void Hardware::setEncoder(u16 index, u8 value, u8 color, u8 accentColor) {
  setEncoderColor(index, color);
  setEncoderAccentColor(index, accentColor);
  setEncoderValue(index, value);
}



/***** misc ********************************************************/

// probably shouldn't even use this and just let it handle its own updates
void Hardware::Update() {
  // if (!DEBUG_NO_LEDS) {
  //   feedbackHw.Update();
  // }
}


Hardware::HachiDigital Hardware::fromDigital(uint16_t dInput) {
  Hardware::HachiDigital digital;

  digital.type = UNKNOWN;
  digital.row = 0;
  digital.column = 0;

  if (dInput >= GRID_START_INDEX && dInput < GRID_MID_INDEX) {
    // left side of pads, values go up left to right, top to bottom
    digital.type = GRID;
    digital.row = (dInput - GRID_START_INDEX) / GRID_HALF_COUNT;
    digital.column = (dInput - GRID_START_INDEX) % GRID_HALF_COUNT;
  } else if (dInput >= GRID_MID_INDEX && dInput < GRID_START_INDEX + GRID_ROWS * GRID_COLUMNS) {
    // right side of pads is weird    
    digital.type = GRID;
    digital.column = 8 + (dInput - GRID_MID_INDEX) % GRID_HALF_COUNT;
    uint8_t r = (dInput - GRID_MID_INDEX) / GRID_HALF_COUNT;
    switch (r) {
      case 0: 
        digital.row = 6;
        break;
      case 1: 
        digital.row = 7;
        break;
      case 2: 
        digital.row = 4;
        break;
      case 3: 
        digital.row = 5;
        break;
      case 4: 
        digital.row = 2;
        break;
      case 5: 
        digital.row = 3;
        break;
      case 6: 
        digital.row = 0;
        break;
      case 7: 
        digital.row = 1;
    }
  } else if (dInput >= KEY_START_INDEX && dInput < KEY_START_INDEX + KEY_COLUMNS) {
    digital.type = KEY;
    digital.row = 0;
    digital.column = dInput - KEY_START_INDEX;
  } else if (dInput >= BUTTON_ROW0_INDEX && dInput < BUTTON_ROW0_INDEX + BUTTON_COLUMNS * 2) {
    digital.type = BUTTON;
    digital.row = (dInput - BUTTON_ROW0_INDEX) / BUTTON_COLUMNS;
    digital.column = (dInput - BUTTON_ROW0_INDEX) % BUTTON_COLUMNS;
  } else if (dInput >= BUTTON_ROW2_INDEX && dInput < BUTTON_ROW2_INDEX + BUTTON_COLUMNS * 2) {
    digital.type = BUTTON;
    digital.row = 2 + (dInput - BUTTON_ROW2_INDEX) / BUTTON_COLUMNS;
    digital.column = (dInput - BUTTON_ROW2_INDEX) % BUTTON_COLUMNS;
  } else if (dInput >= BUTTON_ROW4_INDEX && dInput < BUTTON_ROW4_INDEX + BUTTON_COLUMNS * 2) {
    digital.type = BUTTON;
    digital.row = 4 + (dInput - BUTTON_ROW4_INDEX) / BUTTON_COLUMNS;
    digital.column = (dInput - BUTTON_ROW4_INDEX) % BUTTON_COLUMNS;
  }
  // SERIALPRINTLN("Event: type=" + String(press.type) + ", row=" + String(press.row) + ", col=" + String(press.column));
  
  return digital;
}


uint16_t Hardware::toDigital(digital_type type, uint16_t row, uint16_t column) {
  Hardware::HachiDigital digital;
  digital.type = type;
  digital.row = row;
  digital.column = column;
  return toDigital(digital);
}

uint16_t Hardware::toDigital(Hardware::HachiDigital digital) {

  if (digital.type == GRID && digital.column < GRID_HALF_COUNT) {
    return GRID_START_INDEX + digital.row * GRID_HALF_COUNT + digital.column;
  } else if (digital.type == GRID) {
    uint8_t r = 0;
    switch (digital.row) {
      case 0: 
        r = 6;
        break;
      case 1: 
        r = 7;
        break;
      case 2: 
        r = 4;
        break;
      case 3: 
        r = 5;
        break;
      case 4: 
        r = 2;
        break;
      case 5: 
        r = 3;
        break;
      case 6: 
        r = 0;
        break;
      case 7: 
        r = 1;
    }
    return GRID_MID_INDEX + r * GRID_HALF_COUNT + (digital.column - GRID_HALF_COUNT);
  } else if (digital.type == BUTTON) {
    switch (digital.row) {
      case 0:
      case 1:
        return BUTTON_ROW0_INDEX + digital.row * BUTTON_COLUMNS + digital.column;
      case 2:
      case 3:
        return BUTTON_ROW2_INDEX + (digital.row - 2) * BUTTON_COLUMNS + digital.column;
      case 4:
      case 5:
        return BUTTON_ROW4_INDEX + (digital.row - 4) * BUTTON_COLUMNS + digital.column;
    }
  } else if (digital.type == KEY) {
    return KEY_START_INDEX + digital.column;
  }
  return 1000;
}

void Hardware::FillGrid(uint8_t color) {
  for (uint8_t row = 0; row < GRID_ROWS; row++) {
    for (uint8_t column = 0; column < GRID_COLUMNS; column++) {
      hardware.setGrid(row, column, color);
    }
  }
}

void Hardware::ClearGrid() {
  FillGrid(ABS_BLACK);
}

void Hardware::DrawPalette() {
  uint8_t color = 0;
  for (uint8_t row = 0; row < GRID_ROWS; row++) {
    for (uint8_t column = 0; column < GRID_COLUMNS; column++) {
      hardware.setGrid(row, column, color);
      color++;
    }
  }
}

void Hardware::ResetDrawing() {
  for (uint8_t index = 0; index < DIGITAL_COUNT; index++) {
    currentValue[index] = NO_COLOR;
  }
}

void Hardware::CurrentValues() {
  // for (uint8_t index = 0; index < DIGITAL_COUNT; index++) {
  //   SERIALPRINT("(" + String(index) + ":" + String(currentValue[index]) + "), ");
  //   if (index % 16 == 15) {
  //     SERIALPRINTLN("");
  //   }
  // }
  // SERIALPRINTLN("");
}

void Hardware::SendMidiNote(uint8_t channel, uint8_t note, uint8_t velocity) {
  if (channel < 1 || channel > 16) return;   // channel is 1-indexed
  if (note > 127) return;
  if (velocity) {
    velocity = velocity > 127 ? 127 : velocity;
    if (sendToUsb) MIDI.sendNoteOn(note, velocity, channel);
    if (sendToDin) MIDIHW.sendNoteOn(note, velocity, channel);
  } else {
    if (sendToUsb) MIDI.sendNoteOff(note, 0, channel);
    if (sendToDin) MIDIHW.sendNoteOff(note, 0, channel);
  }
}

void Hardware::SendMidiCc(uint8_t channel, uint8_t controller, uint8_t value) {
  // SERIALPRINTLN("Hardware::SendMidiCc, ch=" + String(channel) + ", ctrl=" + String(controller) + ", v=" + String(value));
  if (channel < 1 || channel > 16) return;   // channel is 1-indexed
  if (controller > 127) return;
  if (sendToUsb) MIDI.sendControlChange(controller, value & 0x7f, channel);
  if (sendToDin) MIDIHW.sendControlChange(controller, value & 0x7f, channel);
}

void Hardware::SendAllNotesOff(boolean sendIndividualNotes) {
  for (u8 channel = 1; channel <= 16; channel++) {    // channel is 1-indexed
    SendMidiCc(channel, MIDI_ALL_NOTES_OFF, 127);
  }
  if (sendIndividualNotes) {
    for (u8 note = 0; note < 128; note++) {
      for (u8 channel = 1; channel <= 16; channel++) {    // channel is 1-indexed
        SendMidiNote(channel, note, 0);
      }
    }
  }
}


