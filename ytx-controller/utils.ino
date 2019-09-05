void printPointer(void* pointer){
  char buffer[30];
  sprintf(buffer, "%p", pointer);
  SerialUSB.println(buffer);
  sprintf(buffer, "");
}
uint16_t checkSum(const uint8_t *data, uint8_t len) 
{
  uint16_t sum = 0x00;
  for (uint8_t i = 0; i < len; i++)
    sum ^= data[i];

  return sum;
}

//CRC-8 - algoritmo basato sulle formule di CRC-8 di Dallas/Maxim
//codice pubblicato sotto licenza GNU GPL 3.0
uint8_t CRC8(const uint8_t *data, uint8_t len) 
{
  uint8_t crc = 0x00;
  while (len--) {
    uint8_t extract = *data++;
    for (uint8_t tempI = 8; tempI; tempI--) 
    {
      uint8_t sum = (crc ^ extract) & 0x01;
      crc >>= 1;
      if (sum) {
        crc ^= 0x8C;
      }
      extract >>= 1;
    }
  }
  return crc&0x7F;
}

void ResetFBMicro(){
  digitalWrite(pinResetSAMD11, LOW);
  delay(5);
  digitalWrite(pinResetSAMD11, HIGH);
  delay(5);
}

void ChangeBrigthnessISR(void){
  uint8_t powerAdapterConnected = !digitalRead(pinExternalVoltage);
  static int sumBright = 0;
  if(powerAdapterConnected){
    //SerialUSB.println("Power connected");
    feedbackHw.SendCommand(CHANGE_BRIGHTNESS);
    feedbackHw.SendCommand(BRIGHNESS_WITH_POWER);
    //feedbackHw.SetStatusLED(STATUS_BLINK, 3, STATUS_FB_CONFIG);
  }else{
    
    feedbackHw.SendCommand(CHANGE_BRIGHTNESS);
//    sumBright += 10;
    feedbackHw.SendCommand(BRIGHNESS_WO_POWER);
//    feedbackHw.SendCommand(BRIGHNESS_WO_POWER+sumBright);
    //SerialUSB.println(BRIGHNESS_WO_POWER+sumBright);
    //feedbackHw.SetStatusLED(STATUS_BLINK, 3, STATUS_FB_CONFIG);
  }
}

long mapl(long x, long in_min, long in_max, long out_min, long out_max)
{
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}
