

void setNextAddress(MCP23S17 mcpX, byte addr){
  mcpX.pinMode(a0pin,OUTPUT);
  mcpX.pinMode(a1pin,OUTPUT);
  mcpX.pinMode(a2pin,OUTPUT);


  mcpX.digitalWrite(a2pin,(addr>>2)&1);
  mcpX.digitalWrite(a1pin,(addr>>1)&1);
  mcpX.digitalWrite(a0pin, addr&1);
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
