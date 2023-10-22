#include "headers/Hardware.h"

Hardware::Hardware() {

}

void Hardware::Init() {
  initialized = 1;
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
  uint16_t dIndex = toDigital(GRID, row, column);
  // SERIALPRINTLN("Hardware::setGrid row=" + String(row) + ", col=" + String(column) + ", col=" + String(color) + ", dIdx=" + String(dIndex));
  feedbackHw.SetChangeDigitalFeedback(dIndex, color, true, false, false, false, false);
  // digitalHw.SetDigitalValue(currentBank, dIndex, color);
}

void Hardware::setButton(uint16_t row, uint16_t column, uint16_t color) {
  uint16_t dIndex = toDigital(BUTTON, row, column);
  // SERIALPRINTLN("Drawing::setButton row=" + String(row) + ", col=" + String(column) + ", col=" + String(color) + ", dIdx=" + String(dIndex));
  // feedbackHw.SetChangeDigitalFeedback(dIndex, color, true, false, false, false, false);
  digitalHw.SetDigitalValue(currentBank, dIndex, color);
}

void Hardware::setKey(uint16_t column, uint16_t color) {
  uint16_t dIndex = toDigital(KEY, 0, column);
  // SERIALPRINTLN("Drawing::setKey col=" + String(column) + ", col=" + String(color) + ", dIdx=" + String(dIndex));
  feedbackHw.SetChangeDigitalFeedback(dIndex, color, true, false, false, false, false); 
  // digitalHw.SetDigitalValue(currentBank, dIndex, color);
}

void Hardware::setByIndex(uint16_t index, uint16_t color) {
  // if (index != 152) {     // it's the clock button, so it gets updated a LOT
  //   SERIALPRINTLN("Drawing::setByIndex col=" + String(index) + ", col=" + String(color));
  // }
  feedbackHw.SetChangeDigitalFeedback(index, color, true, false, false, false, false); 
  // digitalHw.SetDigitalValue(currentBank, index, color);
}

void Hardware::Update() {
  feedbackHw.Update();
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

void Hardware::ClearGrid() {
  for (uint8_t row = 0; row < GRID_ROWS; row++) {
    for (uint8_t column = 0; column < GRID_COLUMNS; column++) {
      hardware.setGrid(row, column, ABS_BLACK);
    }
  }
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


void Hardware::SendMidiNote(uint8_t channel, uint8_t note, uint8_t velocity) {
  if(velocity) {
    if (sendToUsb) MIDI.sendNoteOn( note & 0x7f, velocity & 0x7f, channel);
    if (sendToDin) MIDIHW.sendNoteOn( note & 0x7f, velocity & 0x7f, channel);
  } else {
    if (sendToUsb) MIDI.sendNoteOff( note & 0x7f, 0, channel);
    if (sendToDin) MIDIHW.sendNoteOff( note & 0x7f, 0, channel);
  }
}


