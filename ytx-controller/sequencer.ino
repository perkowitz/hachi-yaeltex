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

#include "headers/sequencer.h"

void InitSequencer(void) {
	currentSequencer = 0;
	for(int seq = 0; seq < NUM_SEQUENCERS; seq++){
		seqSettings[seq].stateOfSteps   = 0;
		seqSettings[seq].tempoTimer     = 0;
    seqSettings[seq].currentStep    = 0;
    for(int step = 0; step < SEQ_MAX_STEPS; step++){
      seqSettings[seq].steps[step].offMillis = 0;
      seqSettings[seq].steps[step].stepButtonPrevState  = false;
      seqSettings[seq].steps[step].setUpButtonPrevState = false;
      seqSettings[seq].steps[step].settingUp            = false;
      seqSettings[seq].steps[step].notePlaying      = 0;
      seqSettings[seq].steps[step].channelPlaying   = 0;
      seqSettings[seq].steps[step].currentNote      = 64;
      seqSettings[seq].steps[step].currentOctave    = 3;
      seqSettings[seq].steps[step].currentScale     = 0;
      seqSettings[seq].steps[step].currentVelocity  = 100;
      seqSettings[seq].steps[step].currentDuration  = 300;
    
    } 
    seqSettings[seq].currentSpeed     = 4; 
    seqSettings[seq].sequencerPlaying = true; 
    seqSettings[seq].masterMode       = false;   
    seqSettings[seq].finalStep        = SEQ_MAX_STEPS;
	}

	feedbackHw.SetChangeDigitalFeedback(0, 127, true, NO_SHIFTER, NO_BANK_UPDATE);
	
	antMicrosSequencerInterval = micros();
}

void UpdateSequencer(void) {

  CheckUserInput();
  SendNotesOff();

  for(int seq = 0; seq < NUM_SEQUENCERS; seq++){
    if (seqSettings[seq].sequencerPlaying) {
      if (seqSettings[seq].masterMode) {
        if (micros() - antMicrosSequencerInterval > SEQUENCER_MIN_TIME_INTERVAL) {
          // if (seqSettings[seq].tempoTimer++ >= pow((8-seqSettings[seq].currentSpeed),2)*MASTER_SPEED) {
          if(++seqSettings[seq].tempoTimer == (SEQ_MAX_SPEED-seqSettings[seq].currentSpeed)) {		// SEQ SPEED
            seqSettings[seq].tempoTimer = 0;
            SeqNextStep(seq);
          }
          antMicrosSequencerInterval = micros();
        }
      }
    }
  }
}

// void SysTick_Handler(void)
// {

// }

void SeqNextStep(int seq) {
  uint8_t prevStep = seqSettings[seq].currentStep;
  bool prevStepState = bitRead(seqSettings[seq].stateOfSteps, seqSettings[seq].currentStep);

  // GET NEXT STEP
  seqSettings[seq].currentStep++;
  seqSettings[seq].currentStep %= seqSettings[seq].finalStep;

  // SerialUSB.print("Step: "); SerialUSB.print(seqSettings[seq].currentStep);
  bool currentStepState = bitRead(seqSettings[seq].stateOfSteps, seqSettings[seq].currentStep);
  // SerialUSB.print("\tState: "); SerialUSB.println(currentStepState);
  //if (!setupMode){
    // TURN LED FROM CURRENT STEP ON
	feedbackHw.SetChangeDigitalFeedback(StepButtons[prevStep],                      64,	 prevStepState, NO_SHIFTER, NO_BANK_UPDATE);
	feedbackHw.SetChangeDigitalFeedback(StepButtons[seqSettings[seq].currentStep],  127, true, 				  NO_SHIFTER, NO_BANK_UPDATE);
  // }
	
  // SEND NOTE ON FOR CURRENT STEP  	
  if (currentStepState) SendStepNote(seq, seqSettings[seq].currentStep, NOTE_ON);
}

void CheckUserInput(void){
	bool newButtonState = 0;
	bool currentStepState = 0;
  static unsigned long antMillisLongPress = 0;
	for (int step = 0; step < SEQ_MAX_STEPS; step++){
		newButtonState = digitalHw.GetDigitalState(StepButtons[step]);
		currentStepState = bitRead(seqSettings[currentSequencer].stateOfSteps, step);
		if(newButtonState != seqSettings[currentSequencer].steps[step].stepButtonPrevState){
      if(newButtonState){
        bitWrite(seqSettings[currentSequencer].stateOfSteps, step, !currentStepState);
        feedbackHw.SetChangeDigitalFeedback(StepButtons[step], 64, !currentStepState,   NO_SHIFTER, NO_BANK_UPDATE);
      }
      seqSettings[currentSequencer].steps[step].stepButtonPrevState = newButtonState;
		}

    bool setUpButtonState = digitalHw.GetDigitalState(SetUpButtons[step]);
    if(setUpButtonState != seqSettings[currentSequencer].steps[step].setUpButtonPrevState){
      if(setUpButtonState && !seqSettings[currentSequencer].steps[step].settingUp){ 
        seqSettings[currentSequencer].steps[step].settingUp = true;
        feedbackHw.SetChangeDigitalFeedback(SetUpButtons[step], 32, true,   NO_SHIFTER, NO_BANK_UPDATE);
        encoderHw.SetEncoderValue(currentBank, 0, map(seqSettings[currentSequencer].steps[step].currentNote, 0, 7, 0, 127));
        encoderHw.SetEncoderValue(currentBank, 1, map(seqSettings[currentSequencer].steps[step].currentOctave, 0, 10, 0, 127));
        encoderHw.SetEncoderValue(currentBank, 2, map(seqSettings[currentSequencer].steps[step].currentScale, 0, 7, 0, 127));
        encoderHw.SetEncoderValue(currentBank, 3, seqSettings[currentSequencer].steps[step].currentVelocity);
        encoderHw.SetEncoderValue(currentBank, 4, map(seqSettings[currentSequencer].steps[step].currentDuration, 0, 2500, 0, 127));
        for(int step2 = 0; step2 < SEQ_MAX_STEPS; step2++){
          if(step != step2){
            seqSettings[currentSequencer].steps[step2].settingUp = false;
            feedbackHw.SetChangeDigitalFeedback(SetUpButtons[step2], 32, false,   NO_SHIFTER, NO_BANK_UPDATE);
          }
        }
      }
      seqSettings[currentSequencer].steps[step].setUpButtonPrevState = setUpButtonState;
      
    }

    if(seqSettings[currentSequencer].steps[step].settingUp){      
      seqSettings[currentSequencer].steps[step].currentNote = map(encoderHw.GetEncoderValue(0), 0, 127, 0, 7);
      seqSettings[currentSequencer].steps[step].currentOctave = map(encoderHw.GetEncoderValue(1), 0, 127, 0, 10);
      seqSettings[currentSequencer].steps[step].currentScale = map(encoderHw.GetEncoderValue(2), 0, 127, 0, 7);
      seqSettings[currentSequencer].steps[step].currentVelocity = encoderHw.GetEncoderValue(3);
      seqSettings[currentSequencer].steps[step].currentDuration = map(encoderHw.GetEncoderValue(4), 0, 127, 0, 2500);
    }
	}
  seqSettings[currentSequencer].currentSpeed = map(encoderHw.GetEncoderValue(7), 0, 127, 0, SEQ_MAX_SPEED-1);
}

void SendStepNote(byte seq, byte step, byte noteOn) {
  uint8_t stepNote = seqSettings[seq].steps[step].currentNote;
  uint8_t stepOctave = seqSettings[seq].steps[step].currentOctave;
  uint8_t stepScale = seqSettings[seq].steps[step].currentScale;
  uint8_t stepVelocity = seqSettings[seq].steps[step].currentVelocity;
  uint8_t stepDuration = seqSettings[seq].steps[step].currentDuration;

  byte noteToSend = scales[stepScale][stepNote] + stepOctave*OCTAVE_JUMP;
  
  seqSettings[currentSequencer].steps[step].offMillis       = millis()+stepDuration;
  seqSettings[currentSequencer].steps[step].notePlaying     = noteToSend;
  seqSettings[currentSequencer].steps[step].channelPlaying  = seq+1;

  MIDI.sendNoteOn(noteToSend, stepVelocity, seq+1);
}
void SendNotesOff(void){
  // NOTES OFF
  for(int step = 0; step < SEQ_MAX_STEPS; step++){
    if(seqSettings[currentSequencer].steps[step].offMillis){
      if(millis() > seqSettings[currentSequencer].steps[step].offMillis){
        
        MIDI.sendNoteOff( seqSettings[currentSequencer].steps[step].notePlaying, 
                          0, 
                          seqSettings[currentSequencer].steps[step].channelPlaying);
        
        // Serial.print("Note "); Serial.print(steps[iSeq][i].note);
        // Serial.print("\tChannel "); Serial.print(steps[iSeq][i].channel);
        // Serial.println("\tOFF"); 
        
        seqSettings[currentSequencer].steps[step].offMillis = 0;
      }
    }
  }  
}
