/* This minimal example shows how to get single-shot range
measurements from the VL6180X.

The range readings are in units of mm. */

#include <Wire.h>
#include <VL6180X.h>
#include <VL53L0X.h>
#include <MovingAverage.h>

#include <MIDI.h>
#include <midi_Defs.h>
#include <midi_UsbTransport.h>

static const unsigned sUsbTransportBufferSize = 1024;
typedef midi::UsbTransport<sUsbTransportBufferSize> UsbTransport;

UsbTransport sUsbTransport;

struct MySettings : public midi::DefaultSettings
{
  static const bool Use1ByteParsing = false;
  static const unsigned SysExMaxSize = 1024; // Accept SysEx messages.
  static const bool UseRunningStatus = false; // My devices seem to be ok with it.
};

// USB instance
MIDI_CREATE_CUSTOM_INSTANCE(UsbTransport, sUsbTransport, MIDI, MySettings);

#define SENSOR2

VL6180X sensor1;
#ifdef SENSOR2
  VL53L0X sensor2;
#endif

#define AVERAGE_SIZE 5

uint32_t mode = 0;
uint32_t antMillis=0;
uint32_t rate[2]={0},distance[2]={0},antDistance[2]={0};
uint32_t average[2][AVERAGE_SIZE]={0};
uint32_t averageIndex[2]={0};
uint32_t controller[2]={68,69};

MovingAverage<int> movingAverage(AVERAGE_SIZE, 0);

#define MIN_DISTANCE 20
#define MAX_DISTANCE 350

void setup() 
{
  SerialUSB.begin(250000);

  MIDI.begin(MIDI_CHANNEL_OMNI); // Se inicializa la comunicación MIDI por USB.
  MIDI.turnThruOff();
  MIDI.setHandleNoteOn(handleNoteOn);
  
  Wire.begin();
  
  pinMode(1,OUTPUT);
  digitalWrite(1,LOW);
  pinMode(2,OUTPUT);
  digitalWrite(2,LOW);


  digitalWrite(1,HIGH);
  sensor1.init();
  sensor1.configureDefault();
  sensor1.setTimeout(500);
  sensor1.setAddress(0x07);
  sensor1.setScaling(2);


  #ifdef SENSOR2
  digitalWrite(2,HIGH);
  sensor2.setTimeout(500);
  sensor2.init();
  // Start continuous back-to-back mode (take readings as
  // fast as possible).  To use continuous timed mode
  // instead, provide a desired inter-measurement period in
  // ms (e.g. sensor.startContinuous(100)).
  sensor2.startContinuous();
  sensor2.setMeasurementTimingBudget(20000);
  #endif
  //
}

void loop() 
{ 
  MIDI.read();

  distance[0] = sensor1.readRangeSingleMillimeters();
  distance[0] = map(distance[0],200,510,20,325);

  #ifdef SENSOR2
    distance[1] = sensor2.readRangeContinuousMillimeters()-40;
  #else
    distance[1] = 0;
  #endif

  for(int i=0;i<2;i++)
  {
    distance[i] = constrain(distance[i],MIN_DISTANCE,MAX_DISTANCE);

    if(distance[i]!=antDistance[i])
    {
      rate[i]++;
      if(mode==1)
      {
        uint32_t acc=0;

        average[i][averageIndex[i]] = distance[i];

        for(int j=0;j<AVERAGE_SIZE;j++)
          acc+=average[i][j];
        acc /= AVERAGE_SIZE;
        
        distance[i] = acc;

        if(++averageIndex[i]==AVERAGE_SIZE)
          averageIndex[i] = 0;
        
      }
      else if(mode==2)
      {
        movingAverage.push(distance[i]);
        distance[i] = movingAverage.get();
      }  
      uint32_t ccValue = map(distance[i],MIN_DISTANCE,MAX_DISTANCE,0,127);
      MIDI.sendControlChange(controller[i], ccValue&0x7f, 1);
      antDistance[i] = distance[i];    
    } 
  }
  SerialUSB.print("\t");
  SerialUSB.print(distance[0]);
  SerialUSB.print(" ");
  SerialUSB.println(distance[1]);
  if(millis()-antMillis>1000)
  {
    antMillis = millis();
    
    for(int i=0;i<2;i++)
    {
      // SerialUSB.print("Rate: ");
      // SerialUSB.println(rate[i]);
      rate[i] = 0;
    }
  }

  if(SerialUSB.available())
  {
    char cmd = SerialUSB.read();
    if(cmd == 'n')
    {
      mode = 0;
    }
    else if(cmd == 'a')
    {
      mode = 1;
    }
    else if(cmd == 'm')
    {
      mode = 2;
    }
  }
}

void handleNoteOn(byte channel, byte note, byte velocity)
{
  mode = constrain(note,0,2);
}