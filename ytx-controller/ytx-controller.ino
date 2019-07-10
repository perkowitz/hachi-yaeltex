#include <Keyboard.h>
#include <Adafruit_NeoPixel.h>
#include <extEEPROM.h>
#include <MIDI.h>

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
uint8_t currentBank;
bool enableProcessing = true;
unsigned long currentTime, midiReadTime;
unsigned long loopTime;

unsigned int nLoops = 0;
unsigned long antMicros;

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
