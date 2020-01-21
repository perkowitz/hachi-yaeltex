#pragma once

#include <asf.h>

#define NUM_LEDS_ENCODER		16
#define LED_YTX_PIN				PIN_PA02

#define ENC1_STRIP_PIN			PIN_PA17
#define ENC2_STRIP_PIN			PIN_PA16
#define DIG1_STRIP_PIN			PIN_PA08
#define DIG2_STRIP_PIN			PIN_PA09
#define FB_STRIP_PIN			PIN_PA15

// 	system_gclk_gen_get_hz(GCLK_GENERATOR_0) -> 32768 KHz
//	32768*366 = ~12 MHz
//	48 MHz / 12MHz = 4 veces por segundo entra a la interrupcion => 250ms
#define ONE_SEC	system_gclk_gen_get_hz(GCLK_GENERATOR_0)/1000
#define ONE_SEC_TICKS			1000
#define QUARTER_SEC_TICKS		250

#define BAUD_RATE	1000000

#define CMD_ALL_LEDS_OFF	0xA5
#define NEW_FRAME_BYTE		0xA6
#define INIT_VALUES			0xA7
#define CHANGE_BRIGHTNESS	0xA8

#define LED_BLINK_TICKS	ONE_SEC_TICKS
#define LED_SHOW_TICKS	20
#define NP_OFF			0

//#define ONE_SEC	48000000

#define ENCODER_CHANGE_FRAME		0x00
#define ENCODER_SWITCH_CHANGE_FRAME	0x01
#define DIGITAL1_CHANGE_FRAME		0x02
#define DIGITAL2_CHANGE_FRAME		0x03
#define FB_CHANGE_FRAME				0x04
#define BANK_CHANGE_FRAME			0x05

#define ENCODER_MASK_H			0x3FFE
#define ENCODER_MASK_V			0xE3FF
#define ENCODER_SWITCH_H_ON		0xC001
#define ENCODER_SWITCH_V_ON		0x1C00

volatile uint32_t ticks = LED_BLINK_TICKS;
bool activeRainbow = false;
bool ledsUpdateOk = true;
volatile uint8_t tickShow = LED_SHOW_TICKS;

enum MsgFrame{
	msgLength = 0, frameType, nRing, orientation,ringStateH, ringStateL, currentValue, 
	minVal, maxVal, R, G, B, checkSum_MSB, checkSum_LSB, ENC_ENDOFFRAME,
	nDigital = nRing, digitalState = ringStateH
};

enum InitFrame{
	nEncoders, nAnalog, nDigitals1, nDigitals2, nBrightness, INIT_ENDOFFRAME
};
enum LedStrips{
	ENCODER1_STRIP, ENCODER2_STRIP, DIGITAL1_STRIP, DIGITAL2_STRIP, FB_STRIP, LAST_STRIP
};
//! [rx_buffer_var]
#define MAX_RX_BUFFER_LENGTH   ENC_ENDOFFRAME+1
volatile uint8_t rxArrayIndex = 0;
volatile bool rxComplete = false;
volatile bool rcvdInitValues = false;
volatile bool receivingInit = false;
volatile bool receivingBrightness = false;

typedef struct{
	uint8_t updateStrip;	// update strip
	uint8_t updateN;		// update ring
	uint8_t updateValue;	// update value
	uint8_t updateMin;	// update min value
	uint8_t updateMax;	// update min value
	uint8_t updateO;		// update orientation
	uint16_t updateState;	// each LED on or off
	uint8_t updateR;		// update R intensity
	uint8_t updateG;		// update G intensity
	uint8_t updateB;		// update B intensity
} RingBufferData;
#define RING_BUFFER_LENGTH	16
volatile RingBufferData ringBuffer[RING_BUFFER_LENGTH];
volatile uint8_t readIdx = 0;
volatile uint8_t writeIdx = 0;
volatile bool changeBrightnessFlag = false;
volatile uint8_t turnAllOffFlag = false;
volatile uint8_t onGoingFrame = false;
volatile uint8_t rx_buffer[MAX_RX_BUFFER_LENGTH];

void usart_read_callback(struct usart_module *const usart_module);
void usart_write_callback(struct usart_module *const usart_module);

void RX_Handler  ( void );

void configure_usart(void);
void configure_usart_callbacks(void);

uint16_t checkSum(const uint8_t *data, uint8_t len);
uint8_t CRC8(const uint8_t *data, uint8_t len);

void UpdateLEDs(uint8_t nStrip, uint8_t nToChange, uint8_t newValue, 
				uint8_t minVal, uint8_t maxVal, bool vertical, uint16_t newState, 
				uint8_t intR, uint8_t intG, uint8_t intB);

long mapl(long x, long in_min, long in_max, long out_min, long out_max);

//! [module_inst]
struct usart_module usart_instance;
//! [module_inst]

uint16_t indexChanged = 0;

bool receivingLEDdata = false;
volatile bool ledShow = false;
uint8_t whichStripToShow = 0;

bool blinkLED = false;
uint8_t blinkLEDcount = 0;

uint8_t numStripsOn = 0;
uint8_t numEncoders = 0;
uint8_t numDigitals1 = 0;
uint8_t numDigitals2 = 0;
uint8_t numAnalogFb = 0;
uint8_t currentBrightness = 0;
//! [rx_buffer_var]
