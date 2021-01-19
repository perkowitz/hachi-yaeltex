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
      seqSettings[seq].steps[step].stepButtonPrevState      = false;
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
        if (micros() - antMicrosSequencerInterval > SEQUENCER_MIN_TIME_INTERVAL) {
          // if (seqSettings[seq].tempoTimer++ >= pow((8-seqSettings[seq].currentSpeed),2)*MASTER_SPEED) {
          seqSettings[seq].tempoTimer++;
          seqSettings[seq].tempoTimer %= ListMasterSpeed[seqSettings[seq].currentSpeed];
          if(!seqSettings[seq].tempoTimer) {		// SEQ SPEED
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
  feedbackHw.SetChangeDigitalFeedback(StepButtons[prevStep][0],                      STEP_ON_COLOR,      prevStepState,  NO_SHIFTER, NO_BANK_UPDATE);
  for(int i = 1; i < 8; i++){
    feedbackHw.SetChangeDigitalFeedback(StepButtons[prevStep][i],                      STEP_ON_COLOR,      false,  NO_SHIFTER, NO_BANK_UPDATE);
  }
  for(int i = 0; i < 8; i++){
	  feedbackHw.SetChangeDigitalFeedback(StepButtons[seqSettings[seq].currentStep][i],  CURRENT_STEP_COLOR, true, 				  NO_SHIFTER, NO_BANK_UPDATE);
  }
  // }
	
  // SEND NOTE ON FOR CURRENT STEP  	
  if (currentStepState){
    SendStepNote(seq, seqSettings[seq].currentStep, NOTE_ON);
    // SerialUSB.println("NOTE SENT");
  } 
}

void CheckUserInput(void){
	// GLOBAL BUTTONS
  bool masterModeButtonState = digitalHw.GetDigitalState(64);
  if(masterModeButtonState != seqSettings[currentSequencer].masterModeButtonPrevState){
    seqSettings[currentSequencer].masterModeButtonPrevState = masterModeButtonState;
    if(masterModeButtonState){
      seqSettings[currentSequencer].masterMode = !seqSettings[currentSequencer].masterMode;
      if(seqSettings[currentSequencer].masterMode){
        feedbackHw.SetChangeDigitalFeedback(64, MASTER_MODE_COLOR,  true,   NO_SHIFTER, NO_BANK_UPDATE);
      }else{
        feedbackHw.SetChangeDigitalFeedback(64, SLAVE_MODE_COLOR,   true,   NO_SHIFTER, NO_BANK_UPDATE);
      }
    }
  }
  bool playButtonState = digitalHw.GetDigitalState(65);
  if(playButtonState != seqSettings[currentSequencer].playButtonPrevState){
    seqSettings[currentSequencer].playButtonPrevState = playButtonState;
    if(playButtonState){
      seqSettings[currentSequencer].sequencerPlaying = !seqSettings[currentSequencer].sequencerPlaying;
      seqSettings[currentSequencer].tempoTimer = 0;
      if(seqSettings[currentSequencer].sequencerPlaying){
        feedbackHw.SetChangeDigitalFeedback(65, PLAY_COLOR, true,   NO_SHIFTER, NO_BANK_UPDATE);
      }else{
        feedbackHw.SetChangeDigitalFeedback(65, STOP_COLOR, true,   NO_SHIFTER, NO_BANK_UPDATE);
      }
    }
  }

  bool modifyAllButtonState = digitalHw.GetDigitalState(72);
  if(modifyAllButtonState){
    seqSettings[currentSequencer].modifyAll = true;
    feedbackHw.SetChangeDigitalFeedback(72, 127, true,    NO_SHIFTER, NO_BANK_UPDATE);
  } 
  else{
    feedbackHw.SetChangeDigitalFeedback(72, 127, false,   NO_SHIFTER, NO_BANK_UPDATE);
    seqSettings[currentSequencer].modifyAll = false;
  }                      
  

  // STEPS
  bool stepOnButtonState = 0;
  bool currentStepState = 0;
  static unsigned long antMillisLongPress = 0;
	for (int step = 0; step < SEQ_MAX_STEPS; step++){
		stepOnButtonState = digitalHw.GetDigitalState(StepButtons[step][0]);
		currentStepState = bitRead(seqSettings[currentSequencer].stateOfSteps, step);
		if(stepOnButtonState != seqSettings[currentSequencer].steps[step].stepButtonPrevState){
      if(stepOnButtonState){
        bitWrite(seqSettings[currentSequencer].stateOfSteps, step, !currentStepState);
        feedbackHw.SetChangeDigitalFeedback(StepButtons[step][0], STEP_ON_COLOR, !currentStepState,   NO_SHIFTER, NO_BANK_UPDATE);
      }
      seqSettings[currentSequencer].steps[step].stepButtonPrevState = stepOnButtonState;
		}

    // STEP MODIFICATORS
    bool ModifierState = digitalHw.GetDigitalState(Modifiers[step]);
    if(ModifierState != seqSettings[currentSequencer].steps[step].modifierButtonPrevState){
      if(ModifierState && !seqSettings[currentSequencer].steps[step].settingUp){ 
        seqSettings[currentSequencer].steps[step].settingUp = true;
        feedbackHw.SetChangeDigitalFeedback(Modifiers[step], MODIFIER_COLOR, true,   NO_SHIFTER, NO_BANK_UPDATE);
        encoderHw.SetEncoderValue(currentBank, NOTE_ENCODER,    seqSettings[currentSequencer].steps[step].currentNote);
        encoderHw.SetEncoderValue(currentBank, OCTAVE_ENCODER,  seqSettings[currentSequencer].steps[step].currentOctave);
        encoderHw.SetEncoderValue(currentBank, SCALE_ENCODER,   seqSettings[currentSequencer].steps[step].currentScale);
        encoderHw.SetEncoderValue(currentBank, VELOCITY_ENCODER,seqSettings[currentSequencer].steps[step].currentVelocity);
        encoderHw.SetEncoderValue(currentBank, DURATION_ENCODER,seqSettings[currentSequencer].steps[step].currentDuration);
        SerialUSB.print(seqSettings[currentSequencer].steps[step].currentNote); SerialUSB.print("\t");
        SerialUSB.print(seqSettings[currentSequencer].steps[step].currentOctave); SerialUSB.print("\t");
        SerialUSB.print(seqSettings[currentSequencer].steps[step].currentScale); SerialUSB.print("\t");
        SerialUSB.print(seqSettings[currentSequencer].steps[step].currentVelocity); SerialUSB.print("\t");
        SerialUSB.print(seqSettings[currentSequencer].steps[step].currentDuration); SerialUSB.print("\n");

        for(int step2 = 0; step2 < SEQ_MAX_STEPS; step2++){
          if(step != step2){
            seqSettings[currentSequencer].steps[step2].settingUp = false;
            feedbackHw.SetChangeDigitalFeedback(Modifiers[step2], MODIFIER_COLOR, false,   NO_SHIFTER, NO_BANK_UPDATE);
          }
        }
      }
      seqSettings[currentSequencer].steps[step].modifierButtonPrevState = ModifierState;
      
    }

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

void SendStepNote(byte seq, byte step, byte noteOn) {
  uint8_t stepNote      = map(seqSettings[seq].steps[step].currentNote, 0, 128, 0, 8);
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
