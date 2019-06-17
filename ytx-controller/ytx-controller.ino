// Git test
#include <KilomuxV2.h>

#include <Adafruit_NeoPixel.h>
#include <extEEPROM.h>
#include <MIDI.h>
#include <SPI.h>

#include "MCP23S17.h"  // Majenko
#include "headers/Defines.h"
#include "headers/types.h"
#include "headers/AnalogInputs.h"


//----------------------------------------------------------------------------------------------------
// INPUTS DEFS AND VARS
KilomuxV2 KmBoard;             // Kilomux Shield


//----------------------------------------------------------------------------------------------------
// ANALOG VARIABLES
//----------------------------------------------------------------------------------------------------

AnalogInputs analogHw;



//----------------------------------------------------------------------------------------------------
// ENCODER VARIABLES
//----------------------------------------------------------------------------------------------------

#include "MCP23S17.h"  // Majenko

const uint8_t encodersMCPChipSelect = 2;

// setup the port expander
MCP23S17 encodersMCP[N_ENC_MODULES];

uint16_t encoderValue[NUM_ENCODERS] = {0};
uint8_t encoderState[NUM_ENCODERS] = {0};
uint16_t encoderPrevValue[NUM_ENCODERS];
int16_t encoderPosition[NUM_ENCODERS] = {0};
uint32_t antMillisEncoderUpdate[NUM_ENCODERS] = {0};
int pulseCounter[NUM_ENCODERS] = {0};

// encoder pin connections to MCP23S17
//    EncNo { Encoder pinA  GPAx, Encoder pinB  GPAy },
const int encoderPins[N_ENCODERS_X_MOD][2] = {
  {1, 0},  // enc:0 AA GPA0,GPA1 - pins 21/22 on MCP23017
  {4, 3},   // enc:1 BB GPA3,GPA4 - pins 24/25 on MCP23017
  {14, 15},  // enc:0 AA GPA0,GPA1 - pins 21/22 on MCP23017                                              // enc:0 AA GPA0,GPA1 - pins 21/22 on MCP23017
  {11, 12}   // enc:1 BB GPA3,GPA4 - pins 24/25 on MCP23017
};

// button on encoder: A  B
const int encoderSwitchPins[N_ENCODERS_X_MOD] = { 2, 5, 13, 10 };

//----------------------------------------------------------------------------------------------------
// DIGITAL INPUTS VARIABLES
//----------------------------------------------------------------------------------------------------

const uint8_t digitalMCPChipSelect1 = 7;
const uint8_t digitalMCPChipSelect2 = 10;

// setup the port expander
MCP23S17 digitalMCP[N_DIGITAL_MODULES];

bool digitalInputState[NUM_DIGITAL_INPUTS] = {0};
bool digitalInputPrevState[NUM_DIGITAL_INPUTS] = {0};

bool updated = false;
uint32_t change[NUM_ENCODERS] = {false};        // goes true when a change in the encoder state is detected

const int digitalInputPins[N_ENCODERS_X_MOD] = { 0, 1, 2, 3 };

// Next MCP address pins
const int a0pin = 6;
const int a1pin = 7;
const int a2pin = 8;


//----------------------------------------------------------------------------------------------------
// COMMS - MIDI AND SERIAL VARIABLES AND OBJECTS
//----------------------------------------------------------------------------------------------------

#if defined(USBCON)
#include <midi_UsbTransport.h>

static const unsigned sUsbTransportBufferSize = 256;
typedef midi::UsbTransport<sUsbTransportBufferSize> UsbTransport;

UsbTransport sUsbTransport;

struct MySettings : public midi::DefaultSettings
{
  static const bool Use1ByteParsing = false;
  static const unsigned SysExMaxSize = 256; // Accept SysEx messages up to 1024 bytes long.
  static const bool UseRunningStatus = false; // My devices seem to be ok with it.
};

MIDI_CREATE_CUSTOM_INSTANCE(UsbTransport, sUsbTransport, MIDI, MySettings);

// Create a 'MIDI' object using MySettings bound to Serial1.
MIDI_CREATE_CUSTOM_INSTANCE(HardwareSerial, Serial1, MIDIHW, MySettings);

#else // No USB available, fallback to Serial
MIDI_CREATE_DEFAULT_INSTANCE();
#endif

uint16_t dataPacketSize;
bool receivingSysEx = 0;

//----------------------------------------------------------------------------------------------------
// COMMS - EEPROM VARIABLES AND OBJECTS
//----------------------------------------------------------------------------------------------------

extEEPROM eep(kbits_512, 1, 128);//device size, number of devices, page size

memoryHost *memHost;

ytxConfigurationType *config;
ytxDigitaltype *digital;
ytxEncoderType *encoder;
ytxAnalogType *analog;
ytxFeedbackType *feedback;

//----------------------------------------------------------------------------------------------------
// FEEDBACK VARIABLES AND OBJECTS
//----------------------------------------------------------------------------------------------------

// When we setup the NeoPixel library, we tell it how many pixels, and which pin to use to send signals.
// Note that for older NeoPixel strips you might need to change the third parameter--see the strandtest
// example for more information on possible values.
Adafruit_NeoPixel statusLED = Adafruit_NeoPixel(NUMPIXELS, STATUS_LED_PIN, NEO_GRB + NEO_KHZ800);

Adafruit_NeoPixel buttonLEDs1 = Adafruit_NeoPixel(NUM_BUT_PIXELS, BUTTON_LED_PIN1, NEO_GRB + NEO_KHZ800);   //THESE GO TO SAMD11
Adafruit_NeoPixel buttonLEDs2 = Adafruit_NeoPixel(NUM_BUT_PIXELS, BUTTON_LED_PIN2, NEO_GRB + NEO_KHZ800);

typedef enum SerialBytes {
  msgLength = 0, nRing, ringStateH, ringStateL, R, G, B, checkSum_MSB, checkSum_LSB, CRC, ENDOFFRAME
};

byte indexRgbList = 0;
const byte rgbList[4][3] = {{0, 0, 96},
                            {32, 0, 64},
                            {64, 0, 32},
                            {96, 0, 0}
};


bool flagBlinkStatusLED = 0;
static byte blinkCountStatusLED = 0;
bool outputBlinkState = 0;
unsigned long millisPrevLED = 0;               // Variable para guardar ms
byte midiStatusLED = 0;

unsigned short int ledState = 0;
unsigned long ringState[N_RINGS] = {0};  //The LED output is based on a scaled veryson of the rotary encoder counter
unsigned long ringPrevState[N_RINGS] = {0};  //The LED output is based on a scaled veryson of the rotary encoder counter
/*This is a 2 dimensional array with 3 LED sequences.  The outer array is the sequence;
  the inner arrays are the values to output at each step of each sequence.  The output values
  are 16 bit hex values (hex math is actually easier here!).  An LED will be on if its
  corresponding binary bit is a one, for example: 0x7 = 0000000000000111 and the first 3 LEDs
  will be on.

  The data type must be 'unsigned int' if the sequence uses the bottom LED since it's value is 0x8000 (out of range for signed int).
*/
unsigned int walk[32] =  {0x0, 0x1, 0x3, 0x2, 0x6, 0x4, 0xC, 0x8, 0x18, 0x10, 0x30, 0x20, 0x60, 0x40, 0xC0, 0x80,
                          0x180, 0x100, 0x300, 0x200, 0x600, 0x400, 0xC00, 0x800, 0x1800, 0x1000, 0x3000, 0x2000, 0x6000, 0x4000, 0xC000, 0x8000
                         };
unsigned int fill[16] =  {0x1, 0x3, 0x7, 0xf, 0x1f, 0x3f, 0x7f, 0xff, 0x1ff, 0x3ff, 0x7ff, 0xfff, 0x1fff, 0x3fff, 0x7fff, 0xffff};
unsigned int spread[16] =  {0x1, 0x3, 0x7, 0xf, 0x1f, 0x3f, 0x7f, 0xff, 0x1ff, 0x3ff, 0x7ff, 0xfff, 0x1fff, 0x3fff, 0x7fff, 0xffff};


//----------------------------------------------------------------------------------------------------
// GENERAL VARS
unsigned long currentTime, midiReadTime;
unsigned long loopTime;

unsigned int nLoops = 0;
unsigned long antMicros;


void Rainbow(Adafruit_NeoPixel *strip, uint8_t wait) {
  uint16_t i, j;

  for (j = 0; j < 256; j++) {
    for (i = 0; i < strip->numPixels(); i++) {
      strip->setPixelColor(i, Wheel(strip, (i * 1 + j) & 255));
      //encoderLEDStrip1.setPixelColor(1, WheelG((i*1+j) & 255));
    }
    strip->show();
    delay(wait);
  }
}

// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(Adafruit_NeoPixel *strip, byte WheelPos) {
  if (WheelPos < 85) {
    return strip->Color(WheelPos * 3, 255 - WheelPos * 3, 0);
  }
  else if (WheelPos < 170) {
    WheelPos -= 85;
    return strip->Color(255 - WheelPos * 3, 0, WheelPos * 3);
  }
  else {
    WheelPos -= 170;
    return strip->Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
}
