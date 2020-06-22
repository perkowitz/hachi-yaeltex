/*
 * timer.h
 *
 * Created: 6/22/2020 12:50:16 PM
 *  Author: Franco
 */ 


#ifndef TIMER_H_
#define TIMER_H_

#include <Arduino.h>
#include <MIDI.h>
#include <midi_Defs.h>
#include <midi_UsbTransport.h>
#include "board_definitions.h"

// Timer interrupt
void TC5_Handler (void);

//Configures the TC to generate output events at the sample frequency.
//Configures the TC in Frequency Generation mode, with an event output once
//each time the audio sample frequency period expires.
void tcConfigure(int sampleRate);

//This function enables TC5 and waits for it to be ready
void tcStartCounter();

//Function that is used to check if TC5 is done syncing
//returns true when it is done syncing
bool tcIsSyncing();

//Reset TC5
void tcReset();

//disable TC5
void tcDisable();

#endif /* TIMER_H_ */