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
    for(int h = 0; h < GRID_HEIGHT; h++){
      seqSettings[seq].stateOfSteps[h]   = 0;
    }
		seqSettings[seq].tempoTimer     = 0;
    seqSettings[seq].currentStep    = 0;
    for(int step = 0; step < SEQ_MAX_STEPS; step++){
      seqSettings[seq].steps[step].offMillis = 0;
      seqSettings[seq].steps[step].stepButtonPrevState      = 0;
      seqSettings[seq].steps[step].modifierButtonPrevState  = false;
      seqSettings[seq].steps[step].settingUp                = false;
      seqSettings[seq].steps[step].notePlaying      = 0;
      seqSettings[seq].steps[step].channelPlaying   = 0;
      seqSettings[seq].steps[step].currentNote      = 64;
      seqSettings[seq].steps[step].currentOctave    = 64;
      seqSettings[seq].steps[step].currentScale     = 0;
      seqSettings[seq].steps[step].currentVelocity  = 100;
      seqSettings[seq].steps[step].currentDuration  = 64;
    
    } 
    seqSettings[seq].currentSpeed               = 4; 
    seqSettings[seq].currentBPM                 = 60; 
    seqSettings[seq].sequencerPlaying           = true; 
    seqSettings[seq].masterMode                 = true;   
    seqSettings[seq].playButtonPrevState        = false; 
    seqSettings[seq].masterModeButtonPrevState  = false;   
    seqSettings[seq].finalStep                  = SEQ_MAX_STEPS;
	}

  if(seqSettings[currentSequencer].masterMode){
    feedbackHw.SetChangeDigitalFeedback(64, 103, true,   NO_SHIFTER, NO_BANK_UPDATE);
  }else{
    feedbackHw.SetChangeDigitalFeedback(64, 10,  true,   NO_SHIFTER, NO_BANK_UPDATE);
  }

  if(seqSettings[currentSequencer].sequencerPlaying){
    feedbackHw.SetChangeDigitalFeedback(65, 46, true,   NO_SHIFTER, NO_BANK_UPDATE);
  }else{
    feedbackHw.SetChangeDigitalFeedback(65, 4,  true,   NO_SHIFTER, NO_BANK_UPDATE);
  }

	feedbackHw.SetChangeDigitalFeedback(0, 127, true, NO_SHIFTER, NO_BANK_UPDATE);
	
	antMicrosSequencerInterval = micros();
}

void UpdateSequencer(void) {

  bool sequencerOnButtonState = digitalHw.GetDigitalState(71);
  if(sequencerOnButtonState != sequencerOnPrevButtonState){
    sequencerOnPrevButtonState = sequencerOnButtonState;
    if(sequencerOnButtonState){
      sequencerOn = !sequencerOn;  
      if(sequencerOn){
        InitSequencer();
        feedbackHw.SetChangeDigitalFeedback(71, 127, true,   NO_SHIFTER, NO_BANK_UPDATE);
      }else{
        feedbackHw.SetBankChangeFeedback(FB_BANK_CHANGED);
      }
    }
  }

  if(sequencerOn){
    CheckUserInput();
    SendNotesOff();

    for(int seq = 0; seq < NUM_SEQUENCERS; seq++){
      if (seqSettings[seq].masterMode && seqSettings[currentSequencer].sequencerPlaying) {
        if (micros() - antMicrosSequencerInterval > (60*1000000/(seqSettings[currentSequencer].currentBPM*24))) {
          // SerialUSB.print("Time elapsed: "); SerialUSB.println(micros() - antMicrosSequencerInterval);
          antMicrosSequencerInterval = micros();
          clockCounter++;
          clockCounter %= 24; 
          MIDI.sendRealTime(midi::Clock);
          if(!clockCounter)
            feedbackHw.SetChangeDigitalFeedback(BPM_BUTTON, 127, true,  NO_SHIFTER, NO_BANK_UPDATE);
          else if(clockCounter == 12)
            feedbackHw.SetChangeDigitalFeedback(BPM_BUTTON, 127, false, NO_SHIFTER, NO_BANK_UPDATE);

          // if (seqSettings[seq].tempoTimer++ >= pow((8-seqSettings[seq].currentSpeed),2)*MASTER_SPEED) {
          seqSettings[seq].tempoTimer++;
          seqSettings[seq].tempoTimer %= ListClockMenu[seqSettings[seq].currentSpeed];
          if(!seqSettings[seq].tempoTimer) {		// SEQ SPEED
            SeqNextStep(seq);
          }
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
  bool prevStepState[GRID_HEIGHT] = {0};

  for(int h = 0; h < GRID_HEIGHT; h++){
    prevStepState[h] = bitRead(seqSettings[seq].stateOfSteps[h], seqSettings[seq].currentStep);    
  }
  // GET NEXT STEP
  seqSettings[seq].currentStep++;
  seqSettings[seq].currentStep %= seqSettings[seq].finalStep;

  // SerialUSB.print("Step: "); SerialUSB.print(seqSettings[seq].currentStep);
  bool currentStepState[GRID_HEIGHT] = {0};
  for(int h = 0; h < GRID_HEIGHT; h++){
    currentStepState[h] = bitRead(seqSettings[seq].stateOfSteps[h], seqSettings[seq].currentStep);
    // TURN LED FROM CURRENT STEP ON
    feedbackHw.SetChangeDigitalFeedback(StepButtons[prevStep][h],                      STEP_ON_COLOR,      prevStepState[h],  NO_SHIFTER, NO_BANK_UPDATE);
    feedbackHw.SetChangeDigitalFeedback(StepButtons[seqSettings[seq].currentStep][h],  CURRENT_STEP_COLOR, true, 				      NO_SHIFTER, NO_BANK_UPDATE);
  
  // SEND NOTE ON FOR CURRENT STEP  	
    if (currentStepState[h]){
      SendStepNote(seq, seqSettings[seq].currentStep, NOTE_ON, GRID_HEIGHT-1-h);
      // SerialUSB.println("NOTE SENT");
    } 
  }
}

void CheckUserInput(void){
	// GLOBAL BUTTONS
  bool masterModeButtonState = digitalHw.GetDigitalState(MASTER_SLAVE_BUTTON);
  if(masterModeButtonState != seqSettings[currentSequencer].masterModeButtonPrevState){
    seqSettings[currentSequencer].masterModeButtonPrevState = masterModeButtonState;
    if(masterModeButtonState){
      seqSettings[currentSequencer].masterMode = !seqSettings[currentSequencer].masterMode;
      if(seqSettings[currentSequencer].masterMode){
        feedbackHw.SetChangeDigitalFeedback(MASTER_SLAVE_BUTTON, MASTER_MODE_COLOR,  true,   NO_SHIFTER, NO_BANK_UPDATE);
      }else{
        feedbackHw.SetChangeDigitalFeedback(MASTER_SLAVE_BUTTON, SLAVE_MODE_COLOR,   true,   NO_SHIFTER, NO_BANK_UPDATE);
      }
    }
  }
  bool playButtonState = digitalHw.GetDigitalState(PLAY_STOP_BUTTON);
  if(playButtonState != seqSettings[currentSequencer].playButtonPrevState){
    seqSettings[currentSequencer].playButtonPrevState = playButtonState;
    if(playButtonState){
      seqSettings[currentSequencer].sequencerPlaying = !seqSettings[currentSequencer].sequencerPlaying;
      seqSettings[currentSequencer].tempoTimer = 0;
      clockCounter = 0;
      if(seqSettings[currentSequencer].sequencerPlaying){

        feedbackHw.SetChangeDigitalFeedback(PLAY_STOP_BUTTON, PLAY_COLOR, true,   NO_SHIFTER, NO_BANK_UPDATE);
      }else{
        feedbackHw.SetChangeDigitalFeedback(PLAY_STOP_BUTTON, STOP_COLOR, true,   NO_SHIFTER, NO_BANK_UPDATE);
      }
    }
  }

  static bool tapTempoOn = false;
  if(seqSettings[currentSequencer].masterMode){
    bool bpmButtonState = digitalHw.GetDigitalState(BPM_BUTTON);
    if(bpmButtonState != seqSettings[currentSequencer].bpmButtonPrevState){
      seqSettings[currentSequencer].bpmButtonPrevState = bpmButtonState;
      if(bpmButtonState){
        if(!tapTempoOn){
          tapTempoOn = true;
          seqSettings[currentSequencer].antMicrosBPM = micros();
          SerialUSB.println("TAP ON");
        }else{
          float newBPM = (float) 60/((float) (micros()-seqSettings[currentSequencer].antMicrosBPM)/1000000);
          seqSettings[currentSequencer].currentBPM += newBPM;
          seqSettings[currentSequencer].currentBPM /= 2;
          seqSettings[currentSequencer].antMicrosBPM = micros();
          SerialUSB.print("Current BPM:"); SerialUSB.println(seqSettings[currentSequencer].currentBPM);
        }
      }
    }
    if(micros()-seqSettings[currentSequencer].antMicrosBPM > 5*(60*1000000/seqSettings[currentSequencer].currentBPM)){
      tapTempoOn = false;
      SerialUSB.println("TAP OFF");
    }
  }
    

  bool modifyAllButtonState = digitalHw.GetDigitalState(MODIFY_ALL_BUTTON);
  if(modifyAllButtonState){
    seqSettings[currentSequencer].modifyAll = true;
    feedbackHw.SetChangeDigitalFeedback(MODIFY_ALL_BUTTON, 127, true,    NO_SHIFTER, NO_BANK_UPDATE);
  } 
  else{
    feedbackHw.SetChangeDigitalFeedback(MODIFY_ALL_BUTTON, 127, false,   NO_SHIFTER, NO_BANK_UPDATE);
    seqSettings[currentSequencer].modifyAll = false;
  }                      
  

  // STEPS
  bool stepOnButtonState = 0;
  bool currentStepState = 0;
  static unsigned long antMillisLongPress = 0;
	for (int step = 0; step < SEQ_MAX_STEPS; step++){
    for(int h = 0; h < GRID_HEIGHT; h++){
      stepOnButtonState = digitalHw.GetDigitalState(StepButtons[step][h]);
      currentStepState = bitRead(seqSettings[currentSequencer].stateOfSteps[h], step);
      if(stepOnButtonState != bitRead(seqSettings[currentSequencer].steps[step].stepButtonPrevState, h)){
        if(stepOnButtonState){
          bitWrite(seqSettings[currentSequencer].stateOfSteps[h], step, !currentStepState);
          feedbackHw.SetChangeDigitalFeedback(StepButtons[step][h], STEP_ON_COLOR, !currentStepState,   NO_SHIFTER, NO_BANK_UPDATE);
        }
        bitWrite(seqSettings[currentSequencer].steps[step].stepButtonPrevState, h, stepOnButtonState);
      }
    }
		

    // STEP MODIFICATORS
    // bool ModifierState = digitalHw.GetDigitalState(Modifiers[step]);
    // if(ModifierState != seqSettings[currentSequencer].steps[step].modifierButtonPrevState){
    //   if(ModifierState && !seqSettings[currentSequencer].steps[step].settingUp){ 
    //     seqSettings[currentSequencer].steps[step].settingUp = true;
    //     feedbackHw.SetChangeDigitalFeedback(Modifiers[step], MODIFIER_COLOR, true,   NO_SHIFTER, NO_BANK_UPDATE);
    //     encoderHw.SetEncoderValue(currentBank, NOTE_ENCODER,    seqSettings[currentSequencer].steps[step].currentNote);
    //     encoderHw.SetEncoderValue(currentBank, OCTAVE_ENCODER,  seqSettings[currentSequencer].steps[step].currentOctave);
    //     encoderHw.SetEncoderValue(currentBank, SCALE_ENCODER,   seqSettings[currentSequencer].steps[step].currentScale);
    //     encoderHw.SetEncoderValue(currentBank, VELOCITY_ENCODER,seqSettings[currentSequencer].steps[step].currentVelocity);
    //     encoderHw.SetEncoderValue(currentBank, DURATION_ENCODER,seqSettings[currentSequencer].steps[step].currentDuration);
    //     SerialUSB.print(seqSettings[currentSequencer].steps[step].currentNote); SerialUSB.print("\t");
    //     SerialUSB.print(seqSettings[currentSequencer].steps[step].currentOctave); SerialUSB.print("\t");
    //     SerialUSB.print(seqSettings[currentSequencer].steps[step].currentScale); SerialUSB.print("\t");
    //     SerialUSB.print(seqSettings[currentSequencer].steps[step].currentVelocity); SerialUSB.print("\t");
    //     SerialUSB.print(seqSettings[currentSequencer].steps[step].currentDuration); SerialUSB.print("\n");

    //     for(int step2 = 0; step2 < SEQ_MAX_STEPS; step2++){
    //       if(step != step2){
    //         seqSettings[currentSequencer].steps[step2].settingUp = false;
    //         feedbackHw.SetChangeDigitalFeedback(Modifiers[step2], MODIFIER_COLOR, false,   NO_SHIFTER, NO_BANK_UPDATE);
    //       }
    //     }
    //   }
    //   seqSettings[currentSequencer].steps[step].modifierButtonPrevState = ModifierState;
      
    // }

    // ENCODERS
    if(seqSettings[currentSequencer].steps[step].settingUp || seqSettings[currentSequencer].modifyAll){      
      seqSettings[currentSequencer].steps[step].currentNote     = encoderHw.GetEncoderValue(NOTE_ENCODER);
      seqSettings[currentSequencer].steps[step].currentOctave   = encoderHw.GetEncoderValue(OCTAVE_ENCODER);
      seqSettings[currentSequencer].steps[step].currentScale    = encoderHw.GetEncoderValue(SCALE_ENCODER);
      seqSettings[currentSequencer].steps[step].currentVelocity = encoderHw.GetEncoderValue(VELOCITY_ENCODER);
      seqSettings[currentSequencer].steps[step].currentDuration = encoderHw.GetEncoderValue(DURATION_ENCODER);
    }
	}
  seqSettings[currentSequencer].currentSpeed = map(encoderHw.GetEncoderValue(SPEED_ENCODER), 0, 128, 0, SEQ_MAX_SPEED);
}

void SendStepNote(byte seq, byte step, byte noteOn, int height) {
  uint8_t stepNote      = height;
  uint8_t stepOctave    = map(seqSettings[seq].steps[step].currentOctave, 0, 128, 0, 11);
  uint8_t stepScale     = map(seqSettings[seq].steps[step].currentScale, 0, 128, 0, 8);
  uint8_t stepVelocity  = seqSettings[seq].steps[step].currentVelocity;
  uint16_t stepDuration  = map(seqSettings[seq].steps[step].currentDuration, 0, 128, 0, 2500);

  SerialUSB.print(stepNote); SerialUSB.print("\t");
  SerialUSB.print(stepOctave); SerialUSB.print("\t");
  SerialUSB.print(stepScale); SerialUSB.print("\t");
  SerialUSB.print(stepVelocity); SerialUSB.print("\t");
  SerialUSB.print(stepDuration); SerialUSB.print("\n");

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
