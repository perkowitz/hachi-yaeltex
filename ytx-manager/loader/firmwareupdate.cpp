#include "loader.h"

#define MCU_PAGE_SIZE      64
#define PAGES_PER_BLOCK    2

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
uint8_t loader::encodeSysEx(QByteArray inData, std::vector<unsigned char> *outSysEx, uint8_t inLength)
{
    uint8_t outLength  = 0;     // Num bytes in output array.
    uint8_t count = 0;     // Num 7bytes in a block.
    uint8_t index = 0;

    outSysEx->clear();
    outSysEx->push_back(0);

    for (unsigned i = 0; i < inLength; ++i)
    {
        const uint8_t data = (uint8_t)inData[i];
        const uint8_t msb  = data >> 7;
        const uint8_t body = data & 0x7f;

        outSysEx->at(index) |= (msb << count);
        outSysEx->push_back(body);

        if (count++ == 6)
        {
            index   += 8;
            outLength  += 8;
            if(i < (inLength-1))
                outSysEx->push_back(0);
            count       = 0;
        }
    }
    return outLength + count + (count != 0 ? 1 : 0);
}

void loader::firmwareUpdateBehavior(int parameter,unsigned char option)
{
    static int state;
    static int messageIndex;
    std::vector<unsigned char> message;

    message.clear();

    if(parameter==BEGIN_FIRMWARE_UPTADE)
        state=FIRMWARE_UPDATE_STATE_BEGIN;
    else if(parameter==SEND_BLOCK_FIRMWARE_UPTADE)
        state=FIRMWARE_UPDATE_STATE_SEND;
    else if(parameter==END_BLOCK_FIRMWARE_UPTADE)
        state=FIRMWARE_UPDATE_STATE_JUMP_TO_APP;

    SLEEP(5);

    switch (state)
    {
        case FIRMWARE_UPDATE_STATE_BEGIN:
            flagFirmwareUpdate = 1;
            flagNumeratePorts = 0;
            messageIndex=0;
            message.push_back(option);

            state = FIRMWARE_UPDATE_STATE_BEGIN_WAIT;

            break;
        case FIRMWARE_UPDATE_STATE_BEGIN_WAIT:
            if(parameter==ACK_FIRMWARE_UPTADE)
            {
                if(progress != 0)
                    progress->deleteLater();

                progress = new QProgressDialog("Updating Firmware...", "Abort", 0, firmwareSysExMessages.size());
                progress->setMinimumDuration(500);
                firmwareUpdateBehavior(SEND_BLOCK_FIRMWARE_UPTADE);
                return;
            }
            break;
        case FIRMWARE_UPDATE_STATE_SEND:
            message = firmwareSysExMessages[messageIndex];

            message.insert(message.begin(),REQUEST_FIRM_DATA_UPLOAD);

            state = FIRMWARE_UPDATE_STATE_SEND_WAIT;

            break;
        case FIRMWARE_UPDATE_STATE_SEND_WAIT:
            if(parameter==ACK_FIRMWARE_UPTADE)
            {
                progress->setValue(++messageIndex);

                if(messageIndex>firmwareSysExMessages.size()-1)
                {
                    firmwareUpdateBehavior(END_BLOCK_FIRMWARE_UPTADE);
                    return;
                }
            }
            else if(parameter==NAK_FIRMWARE_UPTADE)
            {

            }
            firmwareUpdateBehavior(SEND_BLOCK_FIRMWARE_UPTADE);
            return;
        case FIRMWARE_UPDATE_STATE_JUMP_TO_APP:
            message.push_back(REQUEST_RST);
            break;
        default:
            break;
    }

    if(message.size()>0)
    {
        int j;

        for(int j=0;j<sizeof(manufacturerHeader);j++)
            message.insert(message.begin()+j,manufacturerHeader[j]);

        if(state==FIRMWARE_UPDATE_STATE_SEND_WAIT)
        {
            message.push_back(getCheckSum(message,message.size()));
        }
        else if(state==FIRMWARE_UPDATE_STATE_JUMP_TO_APP)
        {
            flagFirmwareUpdate = 0;
            flagNumeratePorts = 1;
        }
        //SysEx Header
        message.insert(message.begin(),240);
        message.push_back( 247 );

        midiout->sendMessage( &message );
    }
}

void loader::createSysExFirmwareMessages(QString fileName)
{
    QFile file(fileName);

    firmwareSysExMessages.clear();

    if (!file.open(QIODevice::ReadOnly))
        return;

    //open file and read all
    QByteArray blob = file.readAll();

    int hexSize = blob.size();

    QList <QByteArray> firmwareBlock;

    //create blocks of MCU_PAGE_SIZE*PAGES_PER_BLOCK size
    while(hexSize>0)
    {
        if(hexSize>(MCU_PAGE_SIZE*PAGES_PER_BLOCK))
        {
            firmwareBlock.append(blob.mid(0,MCU_PAGE_SIZE*PAGES_PER_BLOCK));
            blob.remove(0,MCU_PAGE_SIZE*PAGES_PER_BLOCK);
        }
        else
        {
            QByteArray aux = blob;
            aux.append(QByteArray(MCU_PAGE_SIZE*PAGES_PER_BLOCK-hexSize,0));

            firmwareBlock.append(aux);
            blob.remove(0,hexSize);
        }

        hexSize = blob.size();
    }

    //append address to blocks
    for(int i=0;i<firmwareBlock.count();i++)
    {
        int byteSize = firmwareBlock[i].count();

        uint32_t address = byteSize*i;

        std::vector<unsigned char> message;

        firmwareBlock[i].insert(0,(char *)&address,sizeof(address));

        uint8_t encodeCount = encodeSysEx(firmwareBlock.at(i),&message,firmwareBlock.at(i).size());

        firmwareSysExMessages.append(message);
    }
}
