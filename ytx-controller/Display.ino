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