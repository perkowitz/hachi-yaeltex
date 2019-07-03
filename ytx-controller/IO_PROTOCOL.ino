enum ytxIOStructure
{
    START,
    ID1,
    ID2,
    ID3,
    MESSAGE_PART,
    WISH,
    MESSAGE_TYPE,
    BANK,
    BLOCK,
    SECTION,
    VALUE,
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



enum ytxIOError
{
  invalidNull,
  invalidBlock,
  invalidSection,
  invalidSize,
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
void handleSystemExclusive(byte *message, unsigned size)
{
    feedbackHw.SetStatusLED(1, status_msg_fb);
      
    if(message[ytxIOStructure::ID1]=='y' && 
       message[ytxIOStructure::ID2]=='t' && 
       message[ytxIOStructure::ID3]=='x')
    {
      //SerialUSB.println("HOLA YTX");
      
      int8_t error = 0;
      
       
      if(message[ytxIOStructure::MESSAGE_TYPE] == ytxIOMessageTypes::configurationMessages)
      {
          void *destination;
          uint8_t decodedPayloadSize;

          uint8_t encodedPayloadSize = size - ytxIOStructure::SECTION -2; //ignore headers and F0 F7
          
          if(message[ytxIOStructure::BLOCK] < BLOCKS_COUNT) 
          {
            if(message[ytxIOStructure::SECTION] < memHost->SectionCount(message[ytxIOStructure::BLOCK]))
            {
              destination = memHost->Address(message[ytxIOStructure::BLOCK],message[ytxIOStructure::SECTION]);
              
              if(message[ytxIOStructure::WISH]==ytxIOWish::SET)
              {
                uint8_t decodedPayload[128];
                decodedPayloadSize = decodeSysEx(&message[ytxIOStructure::VALUE],decodedPayload,encodedPayloadSize);
    
                if(decodedPayloadSize == memHost->SectionSize(message[ytxIOStructure::BLOCK]))
                {
                  memcpy(destination,decodedPayload,decodedPayloadSize);

                  memHost->WriteToEEPROM(message[ytxIOStructure::BANK],message[ytxIOStructure::BLOCK],message[ytxIOStructure::SECTION],destination);
                  
                }
                else
                  error = ytxIOError::invalidSize;
               
              }
              else if(message[ytxIOStructure::WISH]==ytxIOWish::GET)
              {
                  uint8_t sysexBlock[256];
                  uint8_t sectionData[128];
                  
                  sysexBlock[ytxIOStructure::ID1] = 'y';
                  sysexBlock[ytxIOStructure::ID2] = 't';
                  sysexBlock[ytxIOStructure::ID3] = 'x';
                  sysexBlock[ytxIOStructure::MESSAGE_PART] = 0; //MESSAGE PART
                  sysexBlock[ytxIOStructure::WISH] = ytxIOWish::SET;
                  sysexBlock[ytxIOStructure::MESSAGE_TYPE] = ytxIOMessageTypes::configurationMessages;
                  sysexBlock[ytxIOStructure::BANK] = message[ytxIOStructure::BANK]; //block
                  sysexBlock[ytxIOStructure::BLOCK] = message[ytxIOStructure::BLOCK]; //block
                  sysexBlock[ytxIOStructure::SECTION] = message[ytxIOStructure::SECTION]; //section

                  memHost->ReadFromEEPROM(message[ytxIOStructure::BANK],message[ytxIOStructure::BLOCK],message[ytxIOStructure::SECTION],sectionData);

                  int sysexSize = encodeSysEx(sectionData,&sysexBlock[ytxIOStructure::VALUE],memHost->SectionSize(message[ytxIOStructure::BLOCK]));
          
                  MIDI.sendSysEx(ytxIOStructure::SECTION + sysexSize,&sysexBlock[1]);
              }
            }
            else
              error = ytxIOError::invalidSection;
          }
          else
            error = ytxIOError::invalidBlock;


          if(error)
          {
              uint8_t sysexBlock[15];
    
              sysexBlock[ytxIOStructure::ID1] = 'y';
              sysexBlock[ytxIOStructure::ID2] = 't';
              sysexBlock[ytxIOStructure::ID3] = 'x';
              sysexBlock[ytxIOStructure::MESSAGE_PART] = error;
              if(error == ytxIOError::invalidSize)
              {
                sysexBlock[ytxIOStructure::BLOCK] = memHost->SectionSize(message[ytxIOStructure::BLOCK]);
                sysexBlock[ytxIOStructure::SECTION] = decodedPayloadSize;
                MIDI.sendSysEx(ytxIOStructure::SECTION,&sysexBlock[1]);        
              }
              else
                MIDI.sendSysEx(ytxIOStructure::MESSAGE_PART,&sysexBlock[1]);

          }
      }
    }
}
