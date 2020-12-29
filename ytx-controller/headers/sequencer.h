/*

Author/s: Franco Grassano - Franco Zaccra

MIT License

Copyright (c) 2020 - Yaeltex

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

*/

#ifndef SEQUENCER_H
#define SEQUENCER_H

const uint8_t ListClockMenu[] = {120, 96, 72, 48, 24, 12, 6, 3};
const uint8_t ListMasterSpeed[] = {100, 75, 50, 40, 30, 15, 10, 5};


#define NUM_SEQUENCERS					    1
#define SEQ_MAX_STEPS       			  16
#define MS_CLOCK                    25
#define MASTER_SPEED                1
#define SEQ_MAX_SPEED      				  8
#define	SEQUENCER_MIN_TIME_INTERVAL 10000  // (10ms)
#define SEQ_BUTTON_LONG_PRESS_MS    1000

#define OCTAVE_JUMP   12
#define CM2           0
#define CM1           1
#define C0            2
#define C1            3
#define C2            4
#define C3            5
#define C4            6
#define C5            7
#define C6            8
#define C7            9
#define C8            10

#define MASTER_SLAVE_BUTTON   64
#define PLAY_STOP_BUTTON      65
#define MODIFY_ALL_BUTTON     72

#define NOTE_ENCODER          0
#define OCTAVE_ENCODER        1
#define SCALE_ENCODER         2
#define VELOCITY_ENCODER      3
#define DURATION_ENCODER      4
#define SPEED_ENCODER         7

#define MASTER_MODE_COLOR     103
#define SLAVE_MODE_COLOR      10
#define PLAY_COLOR            46
#define STOP_COLOR            4
#define MODIFIER_COLOR        32
#define STEP_ON_COLOR         64
#define CURRENT_STEP_COLOR    127

const byte scales[8][8] = { {0,   2,  4,  5,  7,  9,  11, 12},  // MAJOR
                            {0,   2,  3,  5,  7,  8,  10, 12},  // MINOR
                            {2,   4,  5,  7,  9,  11, 12, 14},  // DORIAN
                            {4,   5,  7,  9,  11, 12, 14, 16},  // PHRYGIAN
                            {7,   9,  11, 12, 14, 16, 17, 19},  // MIXOLYDIAN
                            {9,   11, 12, 14, 16, 17, 19, 21},  // AEOLIAN
                            {12,  14, 16, 17, 19, 21, 23, 24},  // IONIAN
                            {11,  12, 14, 16, 17, 19, 21, 23}}; // LOCRIAN


const uint8_t StepButtons[SEQ_MAX_STEPS] = {0, 1, 2, 3, 4, 5, 6, 7, 80, 81, 82, 83, 84, 85, 86, 87};
const uint8_t Modifiers[SEQ_MAX_STEPS] = {8, 9, 10, 11, 12, 13, 14, 15, 88, 89, 90, 91, 92, 93, 94, 95};


uint8_t currentSequencer = 0;
uint32_t antMicrosSequencerInterval = 0;

typedef struct __attribute__((packed)){
  uint8_t currentNote;
  uint8_t currentVelocity;
  uint16_t currentDuration;
  uint8_t currentOctave:4;
  uint8_t currentScale:4;
  uint8_t notePlaying;
  uint8_t channelPlaying;
  uint8_t stepButtonPrevState:1;
  uint8_t modifierButtonPrevState:1;
  uint8_t settingUp:1;
  uint8_t unused:5;
  unsigned long offMillis;
}stepSettings;

typedef struct __attribute__((packed)){
  uint16_t stateOfSteps;
  uint16_t tempoTimer;
	uint8_t currentStep;
	uint8_t currentSpeed;
  stepSettings steps[SEQ_MAX_STEPS];
	uint8_t sequencerPlaying:1;
  uint8_t playButtonPrevState:1;
	uint8_t masterMode:1;
  uint8_t masterModeButtonPrevState:1;
  uint8_t modifyAll:1;
  uint8_t unused:4;
	uint8_t firstStep;
  uint8_t finalStep;
}seqSettingsStruct;

seqSettingsStruct seqSettings[NUM_SEQUENCERS];


#endif