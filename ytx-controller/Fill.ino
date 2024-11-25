#include "headers/Fill.h"

int Fill::stutter1[FILL_STEP_COUNT] = { 0, 1, 2, 3, 0, 1, 2, 3, 0, 1, 0, 1, 0, 0, 0, 0 };
int Fill::stutter2[FILL_STEP_COUNT] = { 0, 1, 2, 3, 0, 1, 0, 1, 0, 0, 0, 0, -1, -1, -1, -1, };
int Fill::stutter3[FILL_STEP_COUNT] = { 4, 5, 6, 7, 4, 5, 6, 7, 4, 5, 4, 7, 12, 13, 4, 12 };
int Fill::stutter4[FILL_STEP_COUNT] = { 2, 3, 4, 5, 10, 11, 12, 13, 2, 3, 4, 1, 10, -1, 12, -1};
int Fill::half[FILL_STEP_COUNT] = { 0, -1, 1, -1, 2, -1, 3, -1, 4, -1, 5, -1, 6, -1, 7, -1 };
int Fill::cut1[FILL_STEP_COUNT] = { 0, 1, 2, 4, 5, 6, 8, 9, 10, 12, 13, 14, 0, 1, 0, 1 };
int Fill::cut2[FILL_STEP_COUNT] = { 0, 2, 4, 6, 0, 2, 4, 6, 8, 10, 12, 14, 8, 10, 12, 14, };
int Fill::backward1[FILL_STEP_COUNT] = { 0, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1};
int Fill::backward2[FILL_STEP_COUNT] = { 12, 13, 14, 15, 8, 9, 10, 11, 4, 5, 6, 7, 0, 1, 2, 3 };
int Fill::drop4[FILL_STEP_COUNT] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 0, -1, -1, -1 };
int Fill::drop8[FILL_STEP_COUNT] = { 0, 1, 2, 3, 4, 5, 6, 7, 0, -1, -1, -1 , -1, -1, -1, -1 };
int Fill::dropSeq[FILL_STEP_COUNT] = { -1, 1, 2, 3, -1, 5, 6, 7, -1, 9, 10, 11, -1, 13, 14, 15 };
int Fill::onlySeq[FILL_STEP_COUNT] = { 0, -1, -1, -1, 4, -1, -1, -1, 8, -1, -1, -1, 12, -1, -1, -1 };
int Fill::eightSeq[FILL_STEP_COUNT] = { 0, -1, 2, -1, 4, -1, 6, -1, 8, -1, 10, -1, 12, -1, 14, -1 };
int Fill::offbeat1[FILL_STEP_COUNT] = { 4, 1, 2, 3, 12, 5, 6, 7, 4, 9, 10, 11, 12, 13, 14, 15 };
int Fill::offbeatRepeat[FILL_STEP_COUNT] = { 4, 1, 2, 3, 12, 1, 2, 3, 4, -1, 2, -1, 12, -1, 12, -1 };
int Fill::offbeatSpare[FILL_STEP_COUNT] = { 4, -1, -1, -1, 4, -1, -1, -1, 4, -1, -1, -1, 4, 4, -1, 0 };

const int* Fill::fills[FILL_PATTERN_COUNT] = { 
        stutter1, stutter2, stutter3, stutter4, 
        half, cut1, cut2, 
        backward1, backward2, 
        drop4, drop8, dropSeq, onlySeq, eightSeq, 
        offbeat1, offbeatRepeat, offbeatSpare 
};

const int* Fill::GetFillPattern(u8 index) {
  if (index == CHOOSE_RANDOM_FILL || index >= FILL_PATTERN_COUNT) {
    return ChooseFillPattern();
  }
  return fills[index];
}

const int* Fill::ChooseFillPattern() {
  return fills[ChooseFillIndex()];
}

const int* Fill::ShufflePattern() {
  static int r[FILL_STEP_COUNT];
  for (int i = 0; i < FILL_STEP_COUNT; i++) {
    r[i] = random(FILL_STEP_COUNT);
  }
  return r;
}

u8 Fill::ChooseFillIndex() {
  return random(FILL_PATTERN_COUNT);
}

int Fill::MapFillPattern(u8 fillPattern, u8 currentStep) {
  if (fillPattern < 0 || fillPattern >= FILL_PATTERN_COUNT) {
    return currentStep;
  }
  return fills[fillPattern][currentStep];
}

