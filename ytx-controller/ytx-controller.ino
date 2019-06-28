#include <KilomuxV2.h>

#include <Adafruit_NeoPixel.h>
#include <extEEPROM.h>
#include <MIDI.h>

#include "headers/Defines.h"
#include "headers/types.h"
#include "headers/modules.h"
#include "headers/AnalogInputs.h"
#include "headers/EncoderInputs.h"


//----------------------------------------------------------------------------------------------------
// INPUTS DEFS AND VARS
KilomuxV2 KmBoard;             // Kilomux Shield

//----------------------------------------------------------------------------------------------------
// ENVIRONMENT VARIABLES
//----------------------------------------------------------------------------------------------------
uint8_t currentBank;

//----------------------------------------------------------------------------------------------------
// ANALOG VARIABLES
//----------------------------------------------------------------------------------------------------

AnalogInputs analogHw;

//----------------------------------------------------------------------------------------------------
// ENCODER VARIABLES
//----------------------------------------------------------------------------------------------------

EncoderInputs encoderHw;

//----------------------------------------------------------------------------------------------------
// DIGITAL INPUTS VARIABLES
//----------------------------------------------------------------------------------------------------

const uint8_t digitalMCPChipSelect1 = 7;
const uint8_t digitalMCPChipSelect2 = 10;

// setup the port expander
MCP23S17 digitalMCP[N_DIGITAL_MODULES];
const int digitalInputPins[N_ENCODERS_X_MOD] = { 0, 1, 2, 3 };

bool digitalInputState[NUM_DIGITAL_INPUTS] = {0};
bool digitalInputPrevState[NUM_DIGITAL_INPUTS] = {0};

bool updated = false;

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
                            {96, 0, 0}};


bool flagBlinkStatusLED = 0;
static byte blinkCountStatusLED = 0;
bool outputBlinkState = 0;
unsigned long millisPrevLED = 0;               // Variable para guardar ms
byte midiStatusLED = 0;

unsigned short int ledState = 0;
unsigned short int ringStateIndex = 0;
unsigned int encoderRingState[N_RINGS] = {0};  //The LED output is based on a scaled veryson of the rotary encoder counter
unsigned int allRingState[N_RINGS] = {0};  //The LED output is based on a scaled veryson of the rotary encoder counter
unsigned int allRingStatePrev[N_RINGS] = {0};  //The LED output is based on a scaled veryson of the rotary encoder counter

/*This is a 2 dimensional array with 3 LED sequences.  The outer array is the sequence;
  the inner arrays are the values to output at each step of each sequence.  The output values
  are 16 bit hex values (hex math is actually easier here!).  An LED will be on if its
  corresponding binary bit is a one, for example: 0x7 = 0000000000000111 and the first 3 LEDs
  will be on.

  The data type must be 'unsigned int' if the sequence uses the bottom LED since it's value is 0x8000 (out of range for signed int).
*/

#define WALK_SIZE     26
#define FILL_SIZE     14
#define EQ_SIZE       13
#define SPREAD_SIZE   14

unsigned int walk[2][WALK_SIZE] =   {{0x0000, 0x0002, 0x0006, 0x0004, 0x000C, 0x0008, 0x0018, 
                                      0x0010, 0x0030, 0x0020, 0x0060, 0x0040, 0x00C0, 0x0080, 
                                      0x0180, 0x0100, 0x0300, 0x0200, 0x0600, 0x0400, 0x0C00, 
                                      0x0800, 0x1800, 0x1000, 0x3000, 0x2000},
                                     {0x0000, 0x2000, 0x6000, 0x4000, 0xC000, 0x8000, 0x8001,
                                      0x0001, 0x0003, 0x0002, 0x0006, 0x0004, 0x000C, 0x0008,
                                      0x0018, 0x0010, 0x0030, 0x0020, 0x0060, 0x0040, 0x00C0,
                                      0x0080, 0x0180, 0x0100, 0x0300, 0x0200}};
                              
unsigned int fill[2][FILL_SIZE] =   {{0x0000, 0x0002, 0x0006, 0x000E, 0x001E, 0x003E, 0x007E, 
                                      0x00FE, 0x01FE, 0x03FE, 0x07FE, 0x0FFE, 0x1FFE, 0x3FFE},
                                     {0x0000, 0x2000, 0x6000, 0xE000, 0xE001, 0xE003, 0xE007, 
                                      0xE00F, 0xE01F, 0xE03F, 0xE07F, 0xE0FF, 0xE1FF, 0xE3FF}};
                              
unsigned int eq[2][EQ_SIZE] =       {{0x00FE, 0x00FC, 0x00F8, 0x00F0, 0x00E0, 0x00C0, 0x0080, 
                                      0x0180, 0x0380, 0x0780, 0x0F80, 0x1F80, 0x3F80},
                                     {0xE00F, 0xC00F, 0x800F, 0x000F, 0x000E, 0x000C, 0x0008, 
                                      0x0018, 0x0038, 0x0078, 0x00F8, 0x01F8, 0x03F8}};
                              
unsigned int spread[2][SPREAD_SIZE] = {{0x0000, 0x0080, 0x00C0, 0x01C0, 0x01E0, 0x03E0, 0x03F0,
                                        0x07F0, 0x07F8, 0x0FF8, 0x0FFC, 0x1FFC, 0x1FFE, 0x3FFE},
                                       {0x0000, 0x0008, 0x000C, 0x001C, 0x001E, 0x003E, 0x003F,
                                        0x007F, 0x807F, 0x80FF, 0xC0FF, 0xC1FF, 0xE1FF, 0xE3FF}};

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
