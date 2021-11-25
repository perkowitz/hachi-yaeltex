/* This example shows how to use continuous mode to take
range measurements with the VL53L0X. It is based on
vl53l0x_ContinuousRanging_Example.c from the VL53L0X API.

The range readings are in units of mm. */

#include <Wire.h>
#include <VL53L0X.h>

VL53L0X sensor;

int ledPin = LED_BUILTIN;    // LED connected to digital pin 9
int dacPin = 2;    // LED connected to digital pin 9
void setup()
{
  SerialUSB.begin(250000);
  Wire.begin();
  
  sensor.setTimeout(500);
  if (!sensor.init())
  {
    SerialUSB.println("Failed to detect and initialize sensor!");
    //while (1) {}
  }
  
  // Start continuous back-to-back mode (take readings as
  // fast as possible).  To use continuous timed mode
  // instead, provide a desired inter-measurement period in
  // ms (e.g. sensor.startContinuous(100)).
  sensor.startContinuous();
  sensor.setMeasurementTimingBudget(20000);

  analogWriteResolution(10); // Set analog out resolution to max, 10-bits
}


uint32_t antMillis=0;
uint32_t rate=0,antDistance=0;

void loop() 
{ 
  if(millis()-antMillis>1000)
  {
    antMillis = millis();
    
    SerialUSB.print("Rate: ");
    SerialUSB.println(rate);
    SerialUSB.println(sensor.getAddress());

    rate = 0;
  }
  
  int distance = sensor.readRangeContinuousMillimeters()-50;
  
  if (sensor.timeoutOccurred()) { SerialUSB.print(" TIMEOUT"); }
  
  distance = constrain(distance,20,500);
  //SerialUSB.println(distance);
  if(distance!=antDistance)
  {
    rate++;
    antDistance = distance;
  } 
}
