enum switchStates
{
  NO_PRESS,                    
  PRESS,                       
  SUSTAINED,                   
  HOLD,                        
  RELEASE
};



#define SWITCHS_COUNT 1

uint8_t currSwitchState[SWITCHS_COUNT];

uint32_t mean = 0;
uint32_t threshold = 0;

void initSwitches()
{
  if (! qt_1.begin())  
    Serial.println("Failed to begin qt on pin A0");
    
  for(int i=0;i<32;i++)
  {
    mean += qt_1.measure(); 
    delay(100);
  }
  mean /= 32;
  threshold = mean*1.025;
  
}
void processSwitches()
{
  static volatile uint16_t switchStateCnt[SWITCHS_COUNT];
  static uint8_t currSwitchRead[SWITCHS_COUNT];
  static uint8_t prevSwitchRead[SWITCHS_COUNT];

  for(uint8_t i=0;i<SWITCHS_COUNT;i++)
  {
    uint32_t measure = qt_1.measure(); //bloquea entre 200 y 500 us 
    if(measure>threshold)
      currSwitchRead[i] = 0;
    else
      currSwitchRead[i] = 1;
  }

    for(uint8_t i=0;i<SWITCHS_COUNT;i++)
    {
      if(prevSwitchRead[i] != currSwitchRead[i])
        switchStateCnt[i]=0;

      //pressed
      if(currSwitchRead[i] == 0)
      {
        switchStateCnt[i]++;
        
        if(switchStateCnt[i] == UMBRAL_SWITCH_DEBOUNCE)
        {
            currSwitchState[i] = PRESS;
            systemEventFlags |= HANDLE_SWITCHES;
        }
        else if(switchStateCnt[i] == UMBRAL_SWITCH_HOLD)
        {
            currSwitchState[i] = HOLD;
            systemEventFlags |= HANDLE_SWITCHES;
        }
      }
      //not pressed
      else 
      {  
        if(currSwitchState[i] != NO_PRESS) 
        {
          if(currSwitchState[i] == SUSTAINED)
          {
            if(++switchStateCnt[i] == UMBRAL_SWITCH_DEBOUNCE)
            {
              currSwitchState[i] = RELEASE;
              systemEventFlags |= HANDLE_SWITCHES;
            }
          }
          else
            currSwitchState[i] = NO_PRESS;
        }
      }

      prevSwitchRead[i] = currSwitchRead[i];
    }
}


void handleSwitchEvents()
{
  for (uint8_t i = 0; i < SWITCHS_COUNT; i++)
  {
    if(currSwitchState[i] != NO_PRESS)
    {
      //PUSHES
      if(currSwitchState[i] == PRESS)
      {
        currSwitchState[i] = SUSTAINED;
        handlePressOneButton(i);
      }
      //HOLDS
      else if (currSwitchState[i] == HOLD)
      {
        currSwitchState[i] = SUSTAINED;
        handleHoldOneButton(i);
      }
      //RELEASES
      else if (currSwitchState[i] == RELEASE)
      {
        currSwitchState[i] = NO_PRESS;
        handleReleaseButton(i);
      }
    }
  }
}
