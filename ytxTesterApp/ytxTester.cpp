#include "ytxTester.h"
#include "ui_ytxTester.h"

void ytxTester::midiInit()
{
    // Create an api map.
    std::map<int, std::string> apiMap;
    apiMap[RtMidi::MACOSX_CORE] = "OS-X CoreMidi";
    apiMap[RtMidi::WINDOWS_MM] = "Windows MultiMedia";
    apiMap[RtMidi::UNIX_JACK] = "Jack Client";
    apiMap[RtMidi::LINUX_ALSA] = "Linux ALSA";
    apiMap[RtMidi::RTMIDI_DUMMY] = "RtMidi Dummy";

    midiin = 0;
    midiout = 0;

    QTimer *timer1 = new QTimer(this);
    connect(timer1, SIGNAL(timeout()), this, SLOT(midiPull()));
    timer1->start(25);

    //RtMidiIn constructor
    try
    {
        midiin = new RtMidiIn();
        std::cout << "\nCurrent input API: " << apiMap[ midiin->getCurrentApi() ] << std::endl;

    }
    catch ( RtMidiError &error )
    {
        error.printMessage();
        exit( EXIT_FAILURE );
    }

    // RtMidiOut constructor
    try {
    midiout = new RtMidiOut();
    }
    catch ( RtMidiError &error )
    {
        error.printMessage();
        exit( EXIT_FAILURE );
    }

    inputPortsName = new QStringList();
    outputPortsName = new QStringList();

    midiDialog = new midiPortsDialog(this);
    midiDialog->hide();

    connect(midiDialog->accept,&QPushButton::clicked,[this]()
    {
        int index = midiDialog->midiDevice->currentIndex();

        if(index)
            connectToDevice(outputPortsIndex[index-1]);
        else
        {
            //previusMidiDevice.clear();
            //disconnectManager();
        }
    });

}
void ytxTester::outputPortsList(QStringList *list)
{
    std::string portName;
    // Check outputs.
    unsigned int nPorts = midiout->getPortCount();

    qDebug() << "There are " << nPorts << " MIDI output ports available.";
    if(nPorts)
    {
        list->clear();
        for ( unsigned int i=0; i<nPorts; i++ )
        {
            try
            {
              portName = midiout->getPortName(i);
            }
            catch (RtMidiError &error)
            {
              error.printMessage();
            }

            //qDebug() << "  Output Port #" << i+1 << ": " << QString().fromStdString(portName);
            list->append(QString().fromStdString(portName));
        }
    }
}

void ytxTester::inputPortsList(QStringList *list)
{
    std::string portName;
    // Check inputs.
    unsigned int nPorts = midiin->getPortCount();
    qDebug() << "There are " << nPorts << " MIDI input sources available.";

    if(nPorts)
    {
        for ( unsigned int i=0; i<nPorts; i++ )
        {
            try
            {
              portName = midiin->getPortName(i);
            }
            catch ( RtMidiError &error )
            {
              error.printMessage();
            }

            list->append(QString().fromStdString(portName));
        }
    }
}

ytxTester::ytxTester(QWidget *parent) : QMainWindow(parent),ui(new Ui::ytxTester)
{
    ui->setupUi(this);

    midiInit();

    outputPortMenu = ui->optionsMenu->addAction(tr("MIDI Device"));

    connect(outputPortMenu,&QAction::triggered,[this]()
    {
        numeratePorts();

        QStringList outputDevices = outputPortsName->filter("Kilomux");

        if(outputPortsName->count())
        {
            midiDialog->midiDevice->clear();
            midiDialog->midiDevice->addItem(tr("None"));
            midiDialog->midiDevice->addItems(outputDevices);
        }

        midiDialog->show();
    });

    ui->labelPortOut->setText("Device: none");

    ui->radioInt->setChecked(1);

    qDebug()<<"size of conf: "<<sizeof (ytxConfigurationType)<<"size of encoder: "<<sizeof (ytxEncoderType)<<"size of feedback: "<<sizeof (ytxFeedbackType)<<"size of digital: "<<sizeof (ytxDigitaltype)<<"size of analog: "<<sizeof (ytxAnalogType);

    encoder.mode.hwMod = 1;
    encoder.mode.speed = 2;
    encoder.rotaryConfig.message = 6;
    encoder.rotaryConfig.channel = 10;
    encoder.rotaryConfig.midiPort = 1;

    for (int i=0;i<6;i++)
    {
        encoder.rotaryConfig.parameter[i] = i+1;
    }

    sprintf(encoder.rotaryConfig.comment,"ytx Enc");

    config.inputs.encodersCount = 32;
    config.inputs.digitalsCount = 32;
    config.inputs.analogsCount = 32;
    config.inputs.feedbacksCount = 32;

    QFile file;
    file.setFileName("test.ytx");

    if (file.open(QIODevice::WriteOnly))
    {


        QDataStream out(&file);

        out.writeRawData((char*)&encoder,sizeof(encoder));

        file.close();

    }
}

ytxTester::~ytxTester()
{
    delete ui;
}

unsigned char ytxTester::getCheckSum(const std::vector<unsigned char> data, int len)
{
    unsigned char checksum = 0;

    for (int i = 0; i < len; i++)
        checksum ^= data[i];

    return checksum &= 0x7f;
}

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

uint8_t decodeSysEx(const std::vector<unsigned char> inSysEx, uint8_t* outData, uint8_t inLength)
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


void ytxTester::sendSysEx(uint8_t size,uint8_t *data)
{
    std::vector<unsigned char> sysexBlock;
    sysexBlock.clear();
    for (int i=0;i<size;i++)
        sysexBlock.push_back(data[i]);
    qDebug()<<sysexBlock;
    sysexBlock.insert(sysexBlock.begin(),240);
    sysexBlock.push_back( 247 );
    midiout->sendMessage( &sysexBlock );
}

void ytxTester::on_sendSerial_clicked()
{
    if(midiout->isPortOpen())
    {
        static int i= 0;

        std::vector<unsigned char> message;
        message.clear();

        uint8_t sysexBlock[256];

        uint8_t sizes[]={sizeof(ytxConfigurationType),sizeof(ytxDigitaltype),
                         sizeof (ytxEncoderType),sizeof (ytxAnalogType),
                         sizeof (ytxFeedbackType)};
        uint8_t *pointers[]={(uint8_t*)&config,(uint8_t*)&button,(uint8_t*)&encoder,(uint8_t*)&analog,(uint8_t*)&feedback};

        sysexBlock[ytxIOStructure::ID1] = 'y';
        sysexBlock[ytxIOStructure::ID2] = 't';
        sysexBlock[ytxIOStructure::ID3] = 'x';
        sysexBlock[ytxIOStructure::MESSAGE_PART] = 0; //MESSAGE PART
        sysexBlock[ytxIOStructure::WISH] = ytxIOWish::SET;
        sysexBlock[ytxIOStructure::MESSAGE_TYPE] = ytxIOMessageTypes::configurationMessages;
        sysexBlock[ytxIOStructure::SECTION] = 0; //section

        sysexBlock[ytxIOStructure::BLOCK] = i;

        for(int j=0;j<5;j++)
        {
            sysexBlock[ytxIOStructure::BANK] = j; //block

            uint8_t rawBlock[128];
            uint8_t rawSize = sizes[i];

            memcpy(rawBlock,pointers[i],rawSize);


            int sysexSize = encodeSysEx(rawBlock,&sysexBlock[ytxIOStructure::VALUE],rawSize);
            //qDebug()<<"sysex size: "<<sysexSize;

            sendSysEx(ytxIOStructure::SECTION + sysexSize,&sysexBlock[1]);

            SLEEP(100);


            sysexBlock[ytxIOStructure::WISH] = ytxIOWish::GET;
            sendSysEx(ytxIOStructure::SECTION,&sysexBlock[1]);

            if(i==0)
                break;

        }
        if(++i==5)
            i=0;
            //static int j = 32;
            //qDebug()<<"sysex size: "<<j;
            //sendSysEx(j++,&sysexBlock[1]);

            /*
            for(int j=32;j<128;j++)
            {
                sendSysEx(j,&sysexBlock[1]);
                SLEEP(100);
            }
            */
    }
}



void ytxTester::numeratePorts()
{
    inputPortsName->clear();
    outputPortsName->clear();
    outputPortsIndex.clear();

    inputPortsList(inputPortsName);
    outputPortsList(outputPortsName);

    for(int i=0;i<outputPortsName->count();i++)
    {
        if(outputPortsName->at(i).contains("Kilomux"))
        {
            outputPortsIndex<<i;
        }
    }
}

void ytxTester::connectToDevice(int index)
{
    if(index>=0 && index<midiout->getPortCount())
    {
        qDebug()<<"conect";
        //puertos de salida
        if(midiout->isPortOpen())
        {
            midiout->closePort();
        }

        midiout->openPort(index);
        selectedMidiDevice = outputPortsName->at(index);

        int inputIndex=-1;

        QList<int> inputPortindex;
        for(int i=0;i<inputPortsName->count();i++)
        {
            qDebug()<<inputPortsName->at(i);
            if(inputPortsName->at(i).contains("Kilomux"))
            {
                inputPortindex<<i;
            }
        }

        //if(outputPortsIndex.indexOf(index)<inputPortindex.count())
            inputIndex = inputPortindex[outputPortsIndex.indexOf(index)];


        if(inputIndex!=-1)
        {
            if(midiin->isPortOpen())
            {
                midiin->closePort();
            }

            midiin->openPort(inputIndex);
            // Don't ignore sysex, timing, or active sensing messages.
            midiin->ignoreTypes( false, false, false );
        }
        else {
            qDebug()<<"no puedo conectar la entrada";
        }

        if(midiin->isPortOpen()&& midiout->isPortOpen())
            ui->labelPortOut->setText(QString("Device: %1").arg(selectedMidiDevice));
    }
}

bool ytxTester::isSysExMessage(std::vector<unsigned char> message)
{
    if(message[0]==240 && message[message.size()-1]==247)
        return true;

    return false;
}


void ytxTester::midiPull()
{
    if(midiin->isPortOpen())
    {
        std::vector<unsigned char> message;
        double stamp = midiin->getMessage( &message );
        int nBytes = message.size();

        while(nBytes>0)
        {
            if(isSysExMessage(message))
            {

                //erase sysex delimiters
                message.erase(message.begin());
                message.erase(message.end()-1);

                qDebug()<<"SysEx recibido, count: "<<message.size();
                qDebug()<<message;
            }
            else
            {
                for (int i=0; i<nBytes; i++ )
                  qDebug() << "Byte " << i << " = " << (int)message[i] << ", ";
                if ( nBytes > 0 )
                  qDebug() << "stamp = " << stamp;

            }
            message.clear();
            stamp = midiin->getMessage( &message );
            nBytes = message.size();
        }
    }
}


midiPortsDialog::midiPortsDialog(QWidget *parent) : QDialog(parent)
{
    QVBoxLayout *verticalLayout = new QVBoxLayout();
    midiDevice = new QComboBox();
    accept = new QPushButton(tr("Ok"));
    verticalLayout->addWidget(new QLabel("MIDI Device"));
    verticalLayout->addWidget(midiDevice);
    verticalLayout->addWidget(accept);

    connect(accept,SIGNAL(clicked()),this,SLOT(hide()));
    this->setLayout(verticalLayout);
}
