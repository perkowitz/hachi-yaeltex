
uint8_t CRC8(const uint8_t *data, uint8_t len);
uint16_t checkSum(volatile uint8_t *data, uint8_t len);
uint8_t encodeSysEx(uint8_t* inData, uint8_t* outSysEx, uint8_t inLength);
uint8_t decodeSysEx(volatile uint8_t* inSysEx, volatile uint8_t* outData, uint8_t inLength);
