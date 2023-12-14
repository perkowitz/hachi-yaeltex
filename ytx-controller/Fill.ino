#include "headers/Fill.h"

int Fill::stutter1[STEPS_PER_MEASURE] = { 0, 1, 2, 3, 0, 1, 2, 3, 0, 1, 0, 1, 0, 0, 0, 0 };
int Fill::stutter2[STEPS_PER_MEASURE] = { 0, 1, 2, 3, 0, 1, 0, 1, 0, 0, 0, 0, -1, -1, -1, -1, };
int Fill::stutter3[STEPS_PER_MEASURE] = { 4, 5, 6, 7, 4, 5, 6, 7, 4, 5, 4, 7, 12, 13, 4, 12 };
int Fill::stutter4[STEPS_PER_MEASURE] = { 2, 3, 4, 5, 10, 11, 12, 13, 2, 3, 4, 1, 10, -1, 12, -1};
int Fill::half[STEPS_PER_MEASURE] = { 0, -1, 1, -1, 2, -1, 3, -1, 4, -1, 5, -1, 6, -1, 7, -1 };
int Fill::cut1[STEPS_PER_MEASURE] = { 0, 1, 2, 4, 5, 6, 8, 9, 10, 12, 13, 14, 0, 1, 0, 1 };
int Fill::cut2[STEPS_PER_MEASURE] = { 0, 2, 4, 6, 0, 2, 4, 6, 8, 10, 12, 14, 8, 10, 12, 14, };
int Fill::backward1[STEPS_PER_MEASURE] = { 0, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1};
int Fill::backward2[STEPS_PER_MEASURE] = { 12, 13, 14, 15, 8, 9, 10, 11, 4, 5, 6, 7, 0, 1, 2, 3 };
int Fill::drop4[STEPS_PER_MEASURE] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 0, -1, -1, -1 };
int Fill::drop8[STEPS_PER_MEASURE] = { 0, 1, 2, 3, 4, 5, 6, 7, 0, -1, -1, -1 , -1, -1, -1, -1 };
int Fill::dropSeq[STEPS_PER_MEASURE] = { -1, 1, 2, 3, -1, 5, 6, 7, -1, 9, 10, 11, -1, 13, 14, 15 };
int Fill::onlySeq[STEPS_PER_MEASURE] = { 0, -1, -1, -1, 4, -1, -1, -1, 8, -1, -1, -1, 12, -1, -1, -1 };
int Fill::eightSeq[STEPS_PER_MEASURE] = { 0, -1, 2, -1, 4, -1, 6, -1, 8, -1, 10, -1, 12, -1, 14, -1 };
int Fill::offbeat1[STEPS_PER_MEASURE] = { 4, 1, 2, 3, 12, 5, 6, 7, 4, 9, 10, 11, 12, 13, 14, 15 };
int Fill::offbeatRepeat[STEPS_PER_MEASURE] = { 4, 1, 2, 3, 12, 1, 2, 3, 4, -1, 2, -1, 12, -1, 12, -1 };
int Fill::offbeatSpare[STEPS_PER_MEASURE] = { 4, -1, -1, -1, 4, -1, -1, -1, 4, -1, -1, -1, 4, 4, -1, 0 };

const int* Fill::fills[FILL_PATTERN_COUNT] = { 
        stutter1, stutter2, stutter3, stutter4, 
        half, cut1, cut2, 
        backward1, backward2, 
        drop4, drop8, dropSeq, onlySeq, eightSeq, 
        offbeat1, offbeatRepeat, offbeatSpare 
};

const int* Fill::GetFillPattern(u8 index) {
  if (index < FILL_PATTERN_COUNT) {
    return fills[index];
  }
  return ChooseFillPattern();
}

const int* Fill::ChooseFillPattern() {
  u8 r = random(FILL_PATTERN_COUNT);
  return fills[r];
}

const int* Fill::ShufflePattern() {
  static int r[STEPS_PER_MEASURE];
  for (int i = 0; i < STEPS_PER_MEASURE; i++) {
    r[i] = random(STEPS_PER_MEASURE);
  }
  return r;
}

