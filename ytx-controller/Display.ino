#include "headers/Display.h"

Display::Display() {

}

void Display::setGrid(uint16_t row, uint16_t column, uint16_t color) {
  if (enabled) hardware->setGrid(row, column, color);
}

void Display::setButton(uint16_t row, uint16_t column, uint16_t color) {
  if (enabled) hardware->setButton(row, column, color);
}

void Display::setKey(uint16_t column, uint16_t color) {
  if (enabled) hardware->setKey(column, color);
}

void Display::setByIndex(uint16_t index, uint16_t color) {
  if (enabled) hardware->setByIndex(index, color);
}

void Display::FillGrid(uint16_t color) {
  if (enabled) hardware->FillGrid(color);
}

// fill only the controls that belong to modules
void Display::FillModule(uint16_t color, bool doGrid, bool doButtons, bool doKeys) {
  if (!enabled) return;

  if (doGrid) {
    hardware->FillGrid(color);
  }

  if (doButtons) {
    for (int column = 0; column < BUTTON_COLUMNS; column++) {
      hardware->setButton(4, column, color);
      hardware->setButton(5, column, color);
      if (column > 3) {
        hardware->setButton(2, column, color);
        hardware->setButton(3, column, color);
      }
    }
  }

  if (doKeys) {
    for (int key = 0; key < KEY_COLUMNS; key++) {
      hardware->setKey(key, color);
    }
  }
}

// force update of all components
void Display::Update() {
  if (enabled) hardware->Update();
}

void Display::setHardware(Hardware *hardware) {
  this->hardware = hardware;
}

void Display::setEnabled(bool enabled) {
  this->enabled = enabled;
}