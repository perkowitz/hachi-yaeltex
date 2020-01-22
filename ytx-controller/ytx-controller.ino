#include <Keyboard.h>
#include <Adafruit_NeoPixel.h>
#include <extEEPROM.h>
#include <MIDI.h>
#include <midi_Defs.h>

#include "lib/SPIExpander/SPIExpander.h"  // Majenko library
#include "headers/Defines.h"
#include "headers/types.h"
#include "headers/modules.h"
#include "headers/AnalogInputs.h"
#include "headers/EncoderInputs.h"
#include "headers/DigitalInputs.h"
#include "headers/FeedbackClass.h"

//----------------------------------------------------------------------------------------------------
// ENVIRONMENT VARIABLES
//----------------------------------------------------------------------------------------------------
uint8_t currentBank = 0;
bool enableProcessing = false;

uint32_t antMicrosLoop; 

bool keyboardReleaseFlag = false;
uint32_t millisKeyboardPress = 0;

uint8_t pinResetSAMD11 = 38;
uint8_t pinExternalVoltage = 13;

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

DigitalInputs digitalHw;

//----------------------------------------------------------------------------------------------------
// FEEDBACK VARIABLES AND OBJECTS
//----------------------------------------------------------------------------------------------------

FeedbackClass feedbackHw;

enum statusLEDtypes
{
    STATUS_FB_NONE,
    STATUS_FB_CONFIG,
    STATUS_FB_MIDI_OUT,
    STATUS_FB_MIDI_IN,
    STATUS_FB_ERROR,
    STATUS_LAST
};
enum statusLEDstates
{
    STATUS_OFF,
    STATUS_BLINK,
    STATUS_ON
};

// STATUS LED
Adafruit_NeoPixel statusLED;
uint8_t flagBlinkStatusLED;
uint8_t blinkCountStatusLED;
uint8_t statusLEDfbType;
int16_t blinkInterval = 0;
bool lastStatusLEDState;
uint32_t millisStatusPrev;
bool firstTime;

uint32_t off = statusLED.Color(0, 0, 0);
uint32_t red = statusLED.Color(STATUS_LED_BRIGHTNESS, 0, 0);
uint32_t green = statusLED.Color(0, STATUS_LED_BRIGHTNESS, 0);
uint32_t blue = statusLED.Color(0, 0, STATUS_LED_BRIGHTNESS);
uint32_t magenta = statusLED.Color(STATUS_LED_BRIGHTNESS/2, 0, STATUS_LED_BRIGHTNESS/2);
uint32_t cyan = statusLED.Color(0, STATUS_LED_BRIGHTNESS/2, STATUS_LED_BRIGHTNESS/2);
uint32_t yellow = statusLED.Color(STATUS_LED_BRIGHTNESS/2, STATUS_LED_BRIGHTNESS/2, 0);
uint32_t white = statusLED.Color(STATUS_LED_BRIGHTNESS/3, STATUS_LED_BRIGHTNESS/3, STATUS_LED_BRIGHTNESS/3);

uint8_t indexRgbList = 0;
uint32_t antMillisPowerChange = 0;
bool powerChangeFlag = false;

uint32_t statusLEDColor[statusLEDtypes::STATUS_LAST] = {off, green, yellow, magenta, red}; 
  
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
  static const unsigned SysExMaxSize = 256; // Accept SysEx messages up to 256 bytes long.
  static const bool UseRunningStatus = false; // My devices seem to be ok with it.
};

MIDI_CREATE_CUSTOM_INSTANCE(UsbTransport, sUsbTransport, MIDI, MySettings);

// Create a 'MIDI' object using MySettings bound to Serial1.
MIDI_CREATE_CUSTOM_INSTANCE(HardwareSerial, Serial1, MIDIHW, MySettings);

#else // No USB available, fallback to Serial
MIDI_CREATE_DEFAULT_INSTANCE();

#endif

#define NRPN_TIMEOUT_MS   50

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

typedef struct __attribute__((packed)){
  uint8_t port : 4;
  uint8_t msgType : 4;
  uint8_t channel : 4;
  uint8_t parameter;
  uint8_t value;
}midiMsgBuffer;

#define MIDI_MSG_BUF_LEN    32
midiMsgBuffer midiMsgBuf[MIDI_MSG_BUF_LEN];

void Rainbow(Adafruit_NeoPixel *strip, uint8_t wait) {
  uint16_t i, j;

  for (j = 0; j < 256; j++) {
    for (i = 0; i < strip->numPixels(); i++) {
      strip->setPixelColor(i, Wheel(strip, (i * 1 + j) & 255));
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
