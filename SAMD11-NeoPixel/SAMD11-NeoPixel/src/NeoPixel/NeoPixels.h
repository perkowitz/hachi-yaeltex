/*
 * CFile1.h
 *
 * Created: 28/03/2019 19:11:06
 *  Author: Franco
 */ 
#ifndef _NEOPIXEL_H_
#define _NEOPIXEL_H_

#include <asf.h>

#define NEO_GRB  ((1 << 6) | (1 << 4) | (0 << 2) | (2))
#define NEO_KHZ800 0x0000 // 800 KHz datastream


uint16_t
	numLEDs,       // Number of RGB LEDs in strip
	numBytes;      // Size of 'pixels' buffer below (3 or 4 bytes/pixel)
uint8_t
	brightness,
	*pixels,        // Holds LED color values (3 or 4 bytes each)
	rOffset,       // Index of red byte within each 3- or 4-byte pixel
	gOffset,       // Index of green byte
	bOffset;       // Index of blue 
uint32_t
	endTime;       // Latch timing reference
bool
	begun;
int8_t
	pin;           // Output pin number (-1 if not yet set)

 void
	pixelsBegin(uint16_t n, uint8_t stripPin, uint8_t t),
	pixelsShow(void),
	setPin(uint8_t p),
	setPixelColor(uint16_t n, uint8_t r, uint8_t g, uint8_t b),
	setPixelColorC(uint16_t n, uint32_t c),
	setAll(int r, int g, int b),
	setBrightness(uint8_t b),
	rainbow(uint8_t wait),
	delay(int delay_time);
 uint16_t 
	numPixels(void);
 uint32_t
	getColor32(uint8_t r, uint8_t g, uint8_t b),
	Wheel(uint8_t WheelPos);

#endif