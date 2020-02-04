#define MAX_SECTION_SIZE  128

enum ytxIOStructure
{
    START,
    ID1,
    ID2,
    ID3,
    MESSAGE_STATUS,
    WISH,
    MESSAGE_TYPE,
    BANK,
    BLOCK,
    SECTION,
    DATA,
    REQUEST_ID = BANK,
    CAPTURE_STATE = BANK
};

enum ytxIOWish
{
  GET,
  SET,
};

enum ytxIOMessageTypes
{
  specialRequests,
  configurationMessages,
  componentInfoMessages,
  statusMessages,
};


enum ytxIOStatus
{
  noStatus,
  validTransaction,
  statusError,
  msgTypeError,
  handshakeError,
  wishError,
  bankError,
  blockError,
  sectionError,
  sizeError,
  N_ERRORS
};

/*! \brief Encode System Exclusive messages.
 SysEx messages are encoded to guarantee transmission of data bytes higher than
 127 without breaking the MIDI protocol. Use this static method to convert the
 data you want to send.
 \param inData The data to encode.
 \param outSysEx The output buffer where to store the encoded message.
 \param inLength The length of the input buffer.
 \return The length of the encoded output buffer.
 @see decodeSysEx
 Code inspired from Ruin & Wesen's SysEx encoder/decoder - http://ruinwesen.com
 */
uint8_t encodeSysEx(uint8_t* inData, uint8_t* outSysEx, uint8_t inLength)
{
    uint8_t outLength  = 0;     // Num bytes in output array.
    uint8_t count          = 0;     // Num 7bytes in a block.
    outSysEx[0]         = 0;

    for (unsigned i = 0; i < inLength; ++i)
    {
        const uint8_t data = inData[i];
        const uint8_t msb  = data >> 7;
        const uint8_t body = data & 0x7f;

        outSysEx[0] |= (msb << count);
        outSysEx[1 + count] = body;

        if (count++ == 6)
        {
            outSysEx   += 8;
            outLength  += 8;
            outSysEx[0] = 0;
            count       = 0;
        }
    }
    return outLength + count + (count != 0 ? 1 : 0);
}

/*! \brief Decode System Exclusive messages.
 SysEx messages are encoded to guarantee transmission of data bytes higher than
 127 without breaking the MIDI protocol. Use this static method to reassemble
 your received message.
 \param inSysEx The SysEx data received from MIDI in.
 \param outData    The output buffer where to store the decrypted message.
 \param inLength The length of the input buffer.
 \return The length of the output buffer.
 @see encodeSysEx @see getSysExArrayLength
 Code inspired from Ruin & Wesen's SysEx encoder/decoder - http://ruinwesen.com
 */
uint8_t decodeSysEx(uint8_t* inSysEx, uint8_t* outData, uint8_t inLength)
{
    uint8_t count  = 0;
    uint8_t msbStorage = 0;

    for (unsigned i = 0; i < inLength; ++i)
    {
        if ((i % 8) == 0)
        {
            msbStorage = inSysEx[i];
        }
        else
        {
            outData[count++] = inSysEx[i] | ((msbStorage & 1) << 7);
            msbStorage >>= 1;
        }
    }
    return count;
}

#define DEBUG_SYSEX
void handleSystemExclusive(byte *message, unsigned size)
{
    static uint32_t antMicrosSysex = 0;
    static bool waitingAckAfterGet = false;
//    SerialUSB.println();
//    if(antMicrosSysex) SerialUSB.println(micros()-antMicrosSysex);

    SetStatusLED(STATUS_BLINK, 1, STATUS_FB_CONFIG);
    
    if(message[ytxIOStructure::ID1]=='y' && 
       message[ytxIOStructure::ID2]=='t' && 
       message[ytxIOStructure::ID3]=='x')
    { 
    #ifdef DEBUG_SYSEX
//      SerialUSB.print("Message size: ");
//      SerialUSB.println(size);
//  
//      SerialUSB.println("RAW MESSAGE");
//      SerialUSB.println();
//      for(int i = 0; i<size; i++){
//        SerialUSB.print(message[i]);
//        SerialUSB.print("\t");
//        if(i>0 && !(i%16)) SerialUSB.println();
//      }
//      SerialUSB.println();
    #endif
      int8_t error = 0;
      
      if(message[ytxIOStructure::MESSAGE_TYPE] == ytxIOMessageTypes::configurationMessages)
      {
         // SerialUSB.println("CONFIG OK");
          void *destination;
          uint8_t decodedPayloadSize;

          uint8_t encodedPayloadSize = size - ytxIOStructure::SECTION-2; //ignore headers and F0 F7
          
          if(message[ytxIOStructure::BLOCK] < BLOCKS_COUNT) 
          {
            if(message[ytxIOStructure::SECTION] < memHost->SectionCount(message[ytxIOStructure::BLOCK]))
            {
              destination = memHost->Address(message[ytxIOStructure::BLOCK],message[ytxIOStructure::SECTION]);
              
              if(message[ytxIOStructure::WISH] == ytxIOWish::SET)
              {
                uint8_t decodedPayload[MAX_SECTION_SIZE];
                decodedPayloadSize = decodeSysEx(&message[ytxIOStructure::DATA],decodedPayload,encodedPayloadSize);
                #ifdef DEBUG_SYSEX
                SerialUSB.println();
                SerialUSB.print("DECODED DATA SIZE: ");SerialUSB.print(decodedPayloadSize);SerialUSB.println(" BYTES");
                SerialUSB.print("BLOCK RECEIVED: ");SerialUSB.print(message[ytxIOStructure::BLOCK]);
                SerialUSB.print("\tSIZE OF BLOCK: ");SerialUSB.println(memHost->SectionSize(message[ytxIOStructure::BLOCK]));
                for(int i = 0; i<decodedPayloadSize; i++){
                  SerialUSB.print(decodedPayload[i], HEX);
                  SerialUSB.print("\t");
                  if(i>0 && !(i%16)) SerialUSB.println();
                }
                SerialUSB.println();
                #endif
                
                if(decodedPayloadSize == memHost->SectionSize(message[ytxIOStructure::BLOCK]))
                {
                  #ifdef DEBUG_SYSEX
//                  SerialUSB.println("\n Tamaño concuerda. Escribiendo en EEPROM");
                  #endif
                  memcpy(destination,decodedPayload,decodedPayloadSize);

                  memHost->WriteToEEPROM(message[ytxIOStructure::BANK],message[ytxIOStructure::BLOCK],message[ytxIOStructure::SECTION],destination);
//                  printConfig(message[ytxIOStructure::BLOCK], message[ytxIOStructure::SECTION]);
                }
                else{
                  error = ytxIOStatus::sizeError;
                }
               
              }
              else if(message[ytxIOStructure::WISH] == ytxIOWish::GET)
              {
                  uint8_t sysexBlock[MAX_SECTION_SIZE*2];
                  uint8_t sectionData[MAX_SECTION_SIZE];
                  
                  sysexBlock[ytxIOStructure::ID1] = 'y';
                  sysexBlock[ytxIOStructure::ID2] = 't';
                  sysexBlock[ytxIOStructure::ID3] = 'x';
                  sysexBlock[ytxIOStructure::MESSAGE_STATUS] = 0; //MESSAGE PART
                  sysexBlock[ytxIOStructure::WISH] = ytxIOWish::SET;
                  sysexBlock[ytxIOStructure::MESSAGE_TYPE] = ytxIOMessageTypes::configurationMessages;
                  sysexBlock[ytxIOStructure::BANK] = message[ytxIOStructure::BANK]; //bank
                  sysexBlock[ytxIOStructure::BLOCK] = message[ytxIOStructure::BLOCK]; //block
                  sysexBlock[ytxIOStructure::SECTION] = message[ytxIOStructure::SECTION]; //section

                  memHost->ReadFromEEPROM(message[ytxIOStructure::BANK],message[ytxIOStructure::BLOCK],message[ytxIOStructure::SECTION],sectionData);

                  int sysexSize = encodeSysEx(sectionData,&sysexBlock[ytxIOStructure::DATA],memHost->SectionSize(message[ytxIOStructure::BLOCK]));
          
                  MIDI.sendSysEx(ytxIOStructure::SECTION + sysexSize,&sysexBlock[1]);
                  waitingAckAfterGet = true;
              }else
                error = ytxIOStatus::wishError;
            }
            else
              error = ytxIOStatus::sectionError;
          }
          else
            error = ytxIOStatus::blockError;

          if(!error)    SendAck();  
          else          SendError(error, message);

      }else if(message[ytxIOStructure::MESSAGE_TYPE] == ytxIOMessageTypes::specialRequests){
        if(message[ytxIOStructure::REQUEST_ID] == 1){
          SendAck();
        }
      }else if(message[ytxIOStructure::MESSAGE_TYPE] == ytxIOMessageTypes::componentInfoMessages){
        componentInfoEnabled = message[ytxIOStructure::CAPTURE_STATE];
        SendAck();
      }else{
        error = ytxIOStatus::msgTypeError;
        SendError(error, message);
      }
    }
}

void SendError(uint8_t error, byte *recvdMessage){
  uint8_t statusMsgSize = ytxIOStructure::SECTION+1;
  uint8_t sysexBlock[statusMsgSize] = {0};
  
  sysexBlock[ytxIOStructure::START] = 0xF0;
  sysexBlock[ytxIOStructure::ID1] = 'y';
  sysexBlock[ytxIOStructure::ID2] = 't';
  sysexBlock[ytxIOStructure::ID3] = 'x';
  sysexBlock[ytxIOStructure::MESSAGE_STATUS] = error;

  if(error == ytxIOStatus::sizeError){
    sysexBlock[ytxIOStructure::BANK] = recvdMessage[ytxIOStructure::BANK];
    sysexBlock[ytxIOStructure::BLOCK] = recvdMessage[ytxIOStructure::BLOCK];
    sysexBlock[ytxIOStructure::SECTION] = recvdMessage[ytxIOStructure::SECTION];
    statusMsgSize = ytxIOStructure::SECTION+1;
    sysexBlock[ytxIOStructure::SECTION+1] = 0xF7;
    MIDI.sendSysEx(statusMsgSize, &sysexBlock[1]);
  }else{
    statusMsgSize = ytxIOStructure::MESSAGE_STATUS+1;
    sysexBlock[ytxIOStructure::MESSAGE_STATUS+1] = 0xF7;
    MIDI.sendSysEx(statusMsgSize, &sysexBlock[1]);
  }
  #ifdef DEBUG_SYSEX
  SerialUSB.println ("Message sent: ");
  SerialUSB.print("Size: ");SerialUSB.println(statusMsgSize);
  
  for(int i = 0; i <= statusMsgSize; i++){
    SerialUSB.print(sysexBlock[i]);
    SerialUSB.print("\t");
    if(i>0 && !(i%16)) SerialUSB.println();
  }
  SerialUSB.println();
  #endif
}
void SendAck(){
  uint8_t statusMsgSize = ytxIOStructure::MESSAGE_STATUS+1;
  uint8_t sysexBlock[statusMsgSize];
  
  sysexBlock[ytxIOStructure::START] = 0xF0;
  sysexBlock[ytxIOStructure::ID1] = 'y';
  sysexBlock[ytxIOStructure::ID2] = 't';
  sysexBlock[ytxIOStructure::ID3] = 'x';
  sysexBlock[ytxIOStructure::MESSAGE_STATUS] = 1;
  sysexBlock[ytxIOStructure::MESSAGE_STATUS+1] = 0xF7;
  MIDI.sendSysEx(statusMsgSize, &sysexBlock[1]);
#ifdef DEBUG_SYSEX
  SerialUSB.println ("Message sent: ");
  SerialUSB.print("Size: ");SerialUSB.println(statusMsgSize);
  
  for(int i = 0; i <= statusMsgSize; i++){
    SerialUSB.print(sysexBlock[i]);
    SerialUSB.print("\t");
    if(i>0 && !(i%16)) SerialUSB.println();
  }
  SerialUSB.println();
#endif
}
