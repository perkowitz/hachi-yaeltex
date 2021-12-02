#include "Adafruit_FreeTouch.h"

//systemEventFlags
#define READ_SWITCHES      (1 << 0)
#define HANDLE_SWITCHES    (1 << 1)

#define UMBRAL_SWITCH_DEBOUNCE      10//ms
#define UMBRAL_SWITCH_COMBO         250//ms
#define UMBRAL_SWITCH_HOLD          750//ms  

uint8_t systemEventFlags = 0;

uint32_t sampleRate = 1000; //sample rate in hz, determines how often TC5_Handler is called

Adafruit_FreeTouch qt_1 = Adafruit_FreeTouch(A0, OVERSAMPLE_4, RESISTOR_20K, FREQ_MODE_NONE);

#define Serial SerialUSB
uint32_t antmicros = 0;
void setup() {
  Serial.begin(250000);
  
  Serial.println("FreeTouch test");
  // initialize digital pin LED_BUILTIN as an output.
  pinMode(LED_BUILTIN, OUTPUT);

  initSwitches();
  
  pinMode(13, OUTPUT);

  tcConfigure(sampleRate); //configure the timer to run at <sampleRate>Hertz
  tcStartCounter(); //starts the timer

  antmicros = micros();
}



void loop() {
  if(systemEventFlags & READ_SWITCHES)
  {
    processSwitches();
    systemEventFlags ^= READ_SWITCHES;
  }
  if(systemEventFlags & HANDLE_SWITCHES)
  {
      systemEventFlags ^= HANDLE_SWITCHES;
      handleSwitchEvents();  
  }
}

void handlePressOneButton(uint8_t buttonNumber)
{
  Serial.print("Press: ");
  Serial.println(buttonNumber);
  digitalWrite(13, HIGH);
}

void handleHoldOneButton(uint8_t buttonNumber)
{
  Serial.print("Hold: ");
  Serial.println(buttonNumber);
}

void handleReleaseButton(uint8_t buttonNumber)
{
  Serial.print("Release: ");
  Serial.println(buttonNumber);
  digitalWrite(13, LOW);
}
