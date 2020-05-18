/*

Author/s: Franco Grassano - Franco Zaccra

MIT License

Copyright (c) 2020 - Yaeltex

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

*/

#define MAX_SECTION_SIZE  256

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

enum ytxIOSpecialRequests
{
  handshakeRequest = 0x01,
  fwVersion = 0x10,
  hwVersion = 0x11,
  reboot = 0x12,
  bootloaderMode = 0x13,
  selffwUpload = 0x14,
  samd11fwUpload = 0x15,
  enableProc = 0x16,
  disbleProc = 0x17,
  eraseEEPROM = 0x18,
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
void handleSystemExclusiveUSB(byte *message, unsigned size){
  // SerialUSB.println("SysEx arrived via USB");
  handleSystemExclusive(message, size, MIDI_USB);
}
void handleSystemExclusiveHW(byte *message, unsigned size){
  // SerialUSB.println("SysEx arrived via HW");
  handleSystemExclusive(message, size, MIDI_HW);
}

void handleSystemExclusive(byte *message, unsigned size, bool midiSrc)
{
    static uint32_t antMicrosSysex = 0;
    static bool waitingAckAfterGet = false;
//    if(antMicrosSysex) SerialUSB.println(micros()-antMicrosSysex);

    if(message[ytxIOStructure::ID1] == 'y' && 
       message[ytxIOStructure::ID2] == 't' && 
       message[ytxIOStructure::ID3] == 'x')
    { 
    #ifdef DEBUG_SYSEX
     SerialUSB.print("Message size: ");
     SerialUSB.println(size);
 
     SerialUSB.println("RAW MESSAGE");
     SerialUSB.println();
     for(int i = 0; i<size; i++){
       SerialUSB.print(message[i]);
       SerialUSB.print("\t");
       if(i>0 && !(i%16)) SerialUSB.println();
     }
     SerialUSB.println();
    #endif
      int8_t error = 0;
      
      if(message[ytxIOStructure::MESSAGE_TYPE] == ytxIOMessageTypes::configurationMessages)
      {
         // SerialUSB.println("CONFIG OK");
          void *destination;
          uint8_t decodedPayloadSize;

          uint8_t encodedPayloadSize = size - ytxIOStructure::SECTION-2; //ignore headers and F0 F7
          if(message[ytxIOStructure::BANK] < MAX_BANKS){
            if(message[ytxIOStructure::BLOCK] < BLOCKS_COUNT){
              SerialUSB.print("SECTION RECEIVED: ");SerialUSB.println(message[ytxIOStructure::SECTION]);
              SerialUSB.print("MAX SECTIONS: ");SerialUSB.println(memHost->SectionCount(message[ytxIOStructure::BLOCK]));
              if(message[ytxIOStructure::SECTION] < memHost->SectionCount(message[ytxIOStructure::BLOCK]))
              {              
                if(message[ytxIOStructure::WISH] == ytxIOWish::SET){
                  uint8_t decodedPayload[MAX_SECTION_SIZE];
                  decodedPayloadSize = decodeSysEx(&message[ytxIOStructure::DATA],decodedPayload,encodedPayloadSize);
                  #ifdef DEBUG_SYSEX
                  SerialUSB.println();
                  
                  SerialUSB.print("\nDECODED DATA SIZE: ");SerialUSB.print(decodedPayloadSize);SerialUSB.println(" BYTES");
                  SerialUSB.print("EXPECTED DATA SIZE: ");SerialUSB.print(memHost->SectionSize(message[ytxIOStructure::BLOCK]));SerialUSB.println(" BYTES");
                
                  for(int i = 0; i<decodedPayloadSize; i++){
                    SerialUSB.print(decodedPayload[i], HEX);
                    SerialUSB.print("\t");
                    if(i>0 && !(i%16)) SerialUSB.println();
                  }
                  SerialUSB.println();
                  #endif
                  
                  if(decodedPayloadSize == memHost->SectionSize(message[ytxIOStructure::BLOCK])){
                    SerialUSB.println("\n Bloque recibido. Tamaño concuerda. Escribiendo en EEPROM...");

                    if(message[ytxIOStructure::BLOCK] == 0){
                      ytxConfigurationType* payload = (ytxConfigurationType *) decodedPayload;
                      // SerialUSB.println("\n Block 0 received");
                      if(memcmp(&config->inputs,&payload->inputs,sizeof(config->inputs))){
                        SerialUSB.println("\n Input config changed");
                        enableProcessing = false;
                        validConfigInEEPROM = false;

                        //reconfigure
                        memHost->DisarmBlocks();

                        memHost->ConfigureBlock(ytxIOBLOCK::Configuration, 1, sizeof(ytxConfigurationType), true, false);
                        memHost->ConfigureBlock(ytxIOBLOCK::Encoder, payload->inputs.encoderCount, sizeof(ytxEncoderType), false);
                        memHost->ConfigureBlock(ytxIOBLOCK::Analog, payload->inputs.analogCount, sizeof(ytxAnalogType), false);
                        memHost->ConfigureBlock(ytxIOBLOCK::Digital, payload->inputs.digitalCount, sizeof(ytxDigitaltype), false);
                        memHost->ConfigureBlock(ytxIOBLOCK::Feedback, payload->inputs.feedbackCount, sizeof(ytxFeedbackType), false);
                        memHost->LayoutBanks(false);  
                      }
                    }
                    if(validConfigInEEPROM && (message[ytxIOStructure::BANK] != currentBank)){
                      destination = memHost->Address(message[ytxIOStructure::BLOCK],message[ytxIOStructure::SECTION]);
                      memcpy(destination,decodedPayload,decodedPayloadSize);
                    }

                    SerialUSB.print("BANK RECEIVED: ");SerialUSB.print(message[ytxIOStructure::BANK]);
                    SerialUSB.print("\tBLOCK RECEIVED: ");SerialUSB.print(message[ytxIOStructure::BLOCK]);
                    SerialUSB.print("\tSECTION RECEIVED: ");SerialUSB.print(message[ytxIOStructure::SECTION]);
                    SerialUSB.print("\tSIZE OF BLOCK: ");SerialUSB.print(memHost->SectionSize(message[ytxIOStructure::BLOCK]));
                    memHost->WriteToEEPROM(message[ytxIOStructure::BANK],message[ytxIOStructure::BLOCK],message[ytxIOStructure::SECTION],decodedPayload);
                  }
                  else{
                    error = ytxIOStatus::sizeError;
                  }
                 
                }
                else if(message[ytxIOStructure::WISH] == ytxIOWish::GET)
                {
                    SerialUSB.println("GET REQUEST RECEIVED");
                    SerialUSB.print("REQUESTED BANK: "); SerialUSB.print(message[ytxIOStructure::BANK]);
                    SerialUSB.print("\tREQUESTED BLOCK: "); SerialUSB.print(message[ytxIOStructure::BLOCK]);
                    SerialUSB.print("\tREQUESTED SECTION: "); SerialUSB.println(message[ytxIOStructure::SECTION]);
                    
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

                    SerialUSB.println ("Message sent: ");
                    SerialUSB.print("Size: ");SerialUSB.println(memHost->SectionSize(message[ytxIOStructure::BLOCK]));

                    for(int i = 0; i<memHost->SectionSize(message[ytxIOStructure::BLOCK]); i++){
                      SerialUSB.print(sectionData[i], HEX);
                      SerialUSB.print("\t");
                      if(i>0 && !(i%16)) SerialUSB.println();
                    }
                    SerialUSB.println();

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
          else
            error = ytxIOStatus::bankError;
                
          if(!error)    SendAck();  
          else          SendError(error, message);

      }else if(message[ytxIOStructure::MESSAGE_TYPE] == ytxIOMessageTypes::specialRequests){
        if(message[ytxIOStructure::REQUEST_ID] == ytxIOSpecialRequests::handshakeRequest){
          SendAck();
        }else if(message[ytxIOStructure::REQUEST_ID] == ytxIOSpecialRequests::bootloaderMode){
          config->board.bootFlag = 1;                                            
          byte bootFlagState = 0;
          eep.read(BOOT_SIGN_ADDR, (byte *) &bootFlagState, sizeof(bootFlagState));
          bootFlagState |= 1;
          eep.write(BOOT_SIGN_ADDR, (byte *) &bootFlagState, sizeof(bootFlagState));

          SelfReset();
        }else if(message[ytxIOStructure::REQUEST_ID] == ytxIOSpecialRequests::fwVersion){
          byte fwVersionByte = 0;
          // eep.read(BOOT_SIGN_ADDR, (byte *) &bootFlagState, sizeof(bootFlagState));
          // bootFlagState |= 1;
          // eep.write(BOOT_SIGN_ADDR, (byte *) &bootFlagState, sizeof(bootFlagState));

          // SelfReset();
        }else if(message[ytxIOStructure::REQUEST_ID] == ytxIOSpecialRequests::eraseEEPROM){
          feedbackHw.SendCommand(CMD_ALL_LEDS_OFF);
          SetStatusLED(STATUS_BLINK, 4, statusLEDtypes::STATUS_FB_EEPROM);
          eeErase(128, 0, 65535);
          SelfReset();
        }else if(message[ytxIOStructure::REQUEST_ID] == ytxIOSpecialRequests::reboot){                  
          feedbackHw.SendCommand(CMD_ALL_LEDS_OFF);
          SendAck();
          SelfReset();
        }

      }else if(message[ytxIOStructure::MESSAGE_TYPE] == ytxIOMessageTypes::componentInfoMessages){
//        componentInfoEnabled = message[ytxIOStructure::CAPTURE_STATE];
        componentInfoEnabled  = !componentInfoEnabled;
       // SerialUSB.println(componentInfoEnabled);
        SendAck();
      }else{
        error = ytxIOStatus::msgTypeError;
        SendError(error, message);
      }
    }else{
      // If it is not meant to get to a yaeltex controller, route it accordingly
      SerialUSB.println("NOT YTX SYSEX MESSAGE");
      if(midiSrc == MIDI_HW){  // IN FROM MIDI HW
        if(config->midiConfig.midiMergeFlags & MIDI_MERGE_FLAGS_HW_USB){    // Send to MIDI USB port
          // SerialUSB.println("HW -> USB");
          MIDI.sendSysEx(size, message, true);
        }
        if(config->midiConfig.midiMergeFlags & MIDI_MERGE_FLAGS_HW_HW){     // Send to MIDI DIN port
          // SerialUSB.println("HW -> HW");
          MIDIHW.sendSysEx(size, message, true);
        }
      }else{        // IN FROM MIDI USB
        if(config->midiConfig.midiMergeFlags & MIDI_MERGE_FLAGS_USB_USB){   // Send to MIDI USB port
          // SerialUSB.println("USB -> USB");
          MIDI.sendSysEx(size, message, true);
        }
        if(config->midiConfig.midiMergeFlags & MIDI_MERGE_FLAGS_USB_HW){    // Send to MIDI DIN port
          // SerialUSB.println("USB -> HW");
          MIDIHW.sendSysEx(size, message, true);
        }
      }
    }
}

void SendComponentInfo(uint8_t componentType, uint16_t index){
  uint8_t statusMsgSize = ytxIOStructure::SECTION+1;
  uint8_t sysexBlock[statusMsgSize];

  sysexBlock[ytxIOStructure::START] = 0xF0;
  sysexBlock[ytxIOStructure::ID1] = 'y';
  sysexBlock[ytxIOStructure::ID2] = 't';
  sysexBlock[ytxIOStructure::ID3] = 'x';
  sysexBlock[ytxIOStructure::MESSAGE_STATUS] = 0;
  sysexBlock[ytxIOStructure::WISH] = 1;
  sysexBlock[ytxIOStructure::MESSAGE_TYPE] = ytxIOMessageTypes::componentInfoMessages;
  sysexBlock[ytxIOStructure::BANK] = 0;
  sysexBlock[ytxIOStructure::BLOCK] = componentType;
  sysexBlock[ytxIOStructure::SECTION] = GetHardwareID(componentType, index);  
  sysexBlock[ytxIOStructure::SECTION+1] = 0xF7;

  lastComponentInfoId = sysexBlock[ytxIOStructure::SECTION];
  
  MIDI.sendSysEx(statusMsgSize, sysexBlock, true);
  SetStatusLED(STATUS_BLINK, 1, statusLEDtypes::STATUS_FB_CONFIG_OUT);

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

uint16_t GetHardwareID(uint8_t componentType, uint16_t index){

  switch (componentType) {
    case ytxIOBLOCK::Encoder: {
      return index;
    } break;
    case ytxIOBLOCK::Digital: {
      return index + config->inputs.encoderCount;  
    } break;
    case ytxIOBLOCK::Analog: {
      return index + config->inputs.encoderCount + config->inputs.digitalCount;  
    } break;
    default:{
      return 0;
    }break;
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
  SetStatusLED(STATUS_BLINK, 1, statusLEDtypes::STATUS_FB_CONFIG_OUT);

  #ifdef DEBUG_SYSEX
  SerialUSB.print ("ERROR CODE: ");
  SerialUSB.println (error, HEX);
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

  SetStatusLED(STATUS_BLINK, 1, statusLEDtypes::STATUS_FB_CONFIG_OUT);

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
