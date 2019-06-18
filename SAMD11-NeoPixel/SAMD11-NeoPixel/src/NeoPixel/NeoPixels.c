/*
 * CFile1.c
 *
 * Created: 28/03/2019 19:03:16
 *  Author: Franco
 */ 

#include <NeoPixels.h>

void pixelsBegin(uint16_t n, uint8_t stripPin, uint8_t t) {
	pin = stripPin;

	if(pin >= 0) {
		struct port_config pin_conf;
		port_get_config_defaults(&pin_conf);

		pin_conf.direction  = PORT_PIN_DIR_OUTPUT;
		port_pin_set_config(pin, &pin_conf);
		port_pin_set_output_level(pin, false);
	}

	// Allocate new data -- note: ALL PIXELS ARE CLEARED
	numBytes = n * 3;
	if((pixels = (uint8_t *)malloc(numBytes))) {
		memset(pixels, 0, numBytes);
		numLEDs = n;
		} else {
		numLEDs = numBytes = 0;
	}

	rOffset = (t >> 4) & 0b11; // regarding R/G/B/W offsets
	gOffset = (t >> 2) & 0b11;
	bOffset =  t       & 0b11;

	begun = true;
}

void pixelsShow(){
	if(!pixels) return;
	__disable_irq();
	// Data latch = 300+ microsecond pause in the output stream.  Rather than
	// put a delay at the end of the function, the ending time is noted and
	// the function will simply hold off (if needed) on issuing the
	// subsequent round of data until the latch time has elapsed.  This
	// allows the mainline code to start generating the next frame of data
	// rather than stalling for the latch.
	// while(!canShow());
	// endTime is a private member (rather than global var) so that multiple
	// instances on different pins can be quickly issued in succession (each
	// instance doesn't delay the next).

	// In order to make this code runtime-configurable to work with any pin,
	// SBI/CBI instructions are eschewed in favor of full PORT writes via the
	// OUT or ST instructions.  It relies on two facts: that peripheral
	// functions (such as PWM) take precedence on output pins, so our PORT-
	// wide writes won't interfere, and that interrupts are globally disabled
	// while data is being issued to the LEDs, so no other code will be
	// accessing the PORT.  The code takes an initial 'snapshot' of the PORT
	// state, computes 'pin high' and 'pin low' values, and writes these back
	// to the PORT register as needed.
	uint8_t  *ptr, *end, p, bitMask, portNum;
	uint32_t  pinMask;

	portNum =  0;
	pinMask =  (1ul <<  pin);
	ptr     =  pixels;
	end     =  ptr + numBytes;
	p       = *ptr++;
	bitMask =  0x80;

	volatile uint32_t *set = &(PORT->Group[portNum].OUTSET.reg), 
		*clr = &(PORT->Group[portNum].OUTCLR.reg);

	for(;;) {
		*set = pinMask;
		asm("nop; nop; nop; nop; nop; nop; nop; nop;");
		if(p & bitMask) {
			asm("nop; nop; nop; nop; nop; nop; nop; nop;"
			"nop; nop; nop; nop; nop; nop; nop; nop;"
			"nop; nop; nop; nop;");
			*clr = pinMask;
			} else {
			*clr = pinMask;
			asm("nop; nop; nop; nop; nop; nop; nop; nop;"
			"nop; nop; nop; nop; nop; nop; nop; nop;"
			"nop; nop; nop; nop;");
		}
		if(bitMask >>= 1) {
			asm("nop; nop; nop; nop; nop; nop; nop; nop; nop;");
			} else {
			if(ptr >= end) break;
			p       = *ptr++;
			bitMask = 0x80;
		}
	}
	__enable_irq();
	//endTime = micros(); // Save EOD time for latch on next call
}

// Set pixel color from separate R,G,B components:
void setPixelColor(
	uint16_t n, uint8_t r, uint8_t g, uint8_t b) {

	if(n < numLEDs) {
		if(brightness) { // See notes in setBrightness()
			r = (r * brightness) >> 8;
			g = (g * brightness) >> 8;
			b = (b * brightness) >> 8;
		}
		uint8_t *p;
		p = &pixels[n * 3];    // 3 bytes per pixel
		
		p[rOffset] = r;          // R,G,B always stored
		p[gOffset] = g;
		p[bOffset] = b;
	}
}

// Set pixel color from 'packed' 32-bit RGB color:
void setPixelColorC(uint16_t n, uint32_t c) {
	if(n < numLEDs) {
		uint8_t *p,
		r = (uint8_t)(c >> 16),
		g = (uint8_t)(c >>  8),
		b = (uint8_t)c;
		if(brightness) { // See notes in setBrightness()
			r = (r * brightness) >> 8;
			g = (g * brightness) >> 8;
			b = (b * brightness) >> 8;
		}
		
		p = &pixels[n * 3];
		
		p[rOffset] = r;
		p[gOffset] = g;
		p[bOffset] = b;
	}
}

// Adjust output brightness; 0=darkest (off), 255=brightest.  This does
// NOT immediately affect what's currently displayed on the LEDs.  The
// next call to show() will refresh the LEDs at this level.  However,
// this process is potentially "lossy," especially when increasing
// brightness.  The tight timing in the WS2811/WS2812 code means there
// aren't enough free cycles to perform this scaling on the fly as data
// is issued.  So we make a pass through the existing color data in RAM
// and scale it (subsequent graphics commands also work at this
// brightness level).  If there's a significant step up in brightness,
// the limited number of steps (quantization) in the old data will be
// quite visible in the re-scaled version.  For a non-destructive
// change, you'll need to re-render the full strip data.  C'est la vie.
void setBrightness(uint8_t b) {
	// Stored brightness value is different than what's passed.
	// This simplifies the actual scaling math later, allowing a fast
	// 8x8-bit multiply and taking the MSB.  'brightness' is a uint8_t,
	// adding 1 here may (intentionally) roll over...so 0 = max brightness
	// (color values are interpreted literally; no scaling), 1 = min
	// brightness (off), 255 = just below max brightness.
	uint8_t newBrightness = b + 1;
	if(newBrightness != brightness) { // Compare against prior value
		// Brightness has changed -- re-scale existing data in RAM
		uint8_t  c,
		*ptr           = pixels,
		oldBrightness = brightness - 1; // De-wrap old brightness value
		uint16_t scale;
		if(oldBrightness == 0) scale = 0; // Avoid /0
		else if(b == 255) scale = 65535 / oldBrightness;
		else scale = (((uint16_t)newBrightness << 8) - 1) / oldBrightness;
		for(uint16_t i=0; i<numBytes; i++) {
			c      = *ptr;
			*ptr++ = (c * scale) >> 8;
		}
		brightness = newBrightness;
	}
}

// Convert separate R,G,B into packed 32-bit RGB color.
// Packed format is always RGB, regardless of LED strand color order.
uint32_t getColor32(uint8_t r, uint8_t g, uint8_t b) {
	return ((uint32_t)r << 16) | ((uint32_t)g <<  8) | b;
}

void setAll(int r, int g, int b){
	for (int i = 0;i<numLEDs;i++){
		setPixelColor(i,r,g,b);
	}
}

uint16_t numPixels(void){
	return numLEDs;
}

void setPin(uint8_t p) {
	//if(begun && (pin >= 0)) pinMode(pin, INPUT);
	pin = p;
	if(begun) {
		struct port_config pin_conf;
		port_get_config_defaults(&pin_conf);

		pin_conf.direction  = PORT_PIN_DIR_OUTPUT;
		port_pin_set_config(pin, &pin_conf);
		port_pin_set_output_level(pin, false);
	}
}

// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(uint8_t WheelPos) {
	if(WheelPos < 85) {
		return getColor32(WheelPos * 3, 255 - WheelPos * 3, 0);
	}
	else if(WheelPos < 170) {
		WheelPos -= 85;
		return getColor32(255 - WheelPos * 3, 0, WheelPos * 3);
	}
	else {
		WheelPos -= 170;
		return getColor32(0, WheelPos * 3, 255 - WheelPos * 3);
	}
}


void rainbow(uint8_t wait) {
	uint16_t i, j;

	for(j=0; j<256; j++) {
		for(i=0; i<numPixels(); i++) {
			uint32_t color32 = Wheel((i*1+j) & 255);
			setPixelColorC(i, color32);
			//encoderLEDStrip1.setPixelColor(1, WheelG((i*1+j) & 255));
		}
		pixelsShow();
		delay(wait);
	}
}

void delay(int delay_time){
	delay_ms(delay_time);
}