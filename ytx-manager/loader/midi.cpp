#include "loader.h"

void loader::midiInit()
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

    QTimer *timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(midiPull()));
    timer->start(25);

    QTimer *timer2 = new QTimer(this);
    connect(timer2, SIGNAL(timeout()), this, SLOT(checkMidiConection()));
    timer2->start(1000);
    flagNumeratePorts = 1;

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
            previusMidiDevice.clear();
            disconnectLoader();
        }
    });
}

void loader::disconnectLoader()
{
    midiin->closePort();
    midiout->closePort();

    selectedMidiDevice = tr("None");
    flagReadyToUpload = 0;

    updateStatus();
}

void loader::connectToDevice(int index)
{
    if(index>=0 && index<midiout->getPortCount())
    {
        //puertos de salida
        if(midiout->isPortOpen())
        {
            midiout->closePort();
        }

        midiout->openPort(index);
        if(midiout->isPortOpen())
        {
            selectedMidiDevice = outputPortsName->at(index);

            int inputIndex=-1;

            if(selectedMidiDevice.contains("KilomuxBOOT"))
            {
                for(int i=0;i<inputPortsName->count();i++)
                {
                    if(inputPortsName->at(i).contains("KilomuxBOOT"))
                    {
                        inputIndex = i;
                        break;
                    }
                }
            }
            else
            {
                QList<int> outputPortindex;
                for(int i=0;i<outputPortsName->count();i++)
                {
                    if(outputPortsName->at(i)==selectedMidiDevice)
                    {
                        outputPortindex<<i;
                    }
                }

                int outFilteredIndex = outputPortindex.indexOf(index);

                QList<int> inputPortindex;
                for(int i=0;i<inputPortsName->count();i++)
                {
                    qDebug()<<inputPortsName->at(i);
                    if(inputPortsName->at(i).contains(selectedMidiDevice))
                    {
                        inputPortindex<<i;
                    }
                }

                if(outFilteredIndex<inputPortindex.count())
                    inputIndex = inputPortindex[outFilteredIndex];
            }


            if(inputIndex!=-1)
            {
                if(midiin->isPortOpen())
                {
                    midiin->closePort();
                }

                flagNumeratePorts = 0;

                try
                {
                    midiin->openPort(inputIndex);
                }
                catch ( RtMidiError &error )
                {
                    disconnectLoader();

                    QPixmap logo = QPixmap(":/img/logo.png");
                    QMessageBox msgBox;
                    msgBox.setIconPixmap(logo);
                    msgBox.setText(tr("MIDI Device"));
                    msgBox.setInformativeText(tr("Device busy or taken by another program!"));
                    msgBox.setDefaultButton(QMessageBox::Ok);
                    msgBox.exec();

                    return;
                }

                if(selectedMidiDevice.contains("KilomuxBOOT"))
                    flagReadyToUpload = 1;
                else
                {
                    flagReadyToUpload = 0;

                    QPixmap logo = QPixmap(":/img/logo.png");
                    QMessageBox msgBox;
                    msgBox.setIconPixmap(logo);
                    msgBox.setText(tr("MIDI Device"));
                    msgBox.setInformativeText(tr("Succesfull conection! Press OK to reboot into bootmode"));
                    msgBox.setDefaultButton(QMessageBox::Ok);
                    msgBox.exec();

                    std::vector<unsigned char> message;

                    message.clear();

                    message.push_back(REQUEST_BOOT_MODE);

                    for(int j=0;j<sizeof(manufacturerHeader);j++)
                        message.insert(message.begin()+j,manufacturerHeader[j]);

                    message.push_back(getCheckSum(message,message.size()));

                    //SysEx Header
                    message.insert(message.begin(),240);
                    message.push_back( 247 );

                    midiout->sendMessage( &message );
                }

                flagNumeratePorts = 1;

                // Don't ignore sysex, timing, or active sensing messages.
                midiin->ignoreTypes( false, false, false );

            }
            else {
                qDebug()<<"no puedo conectar la entrada";
            }

            updateStatus();
        }
    }
}
void loader::checkMidiConection()
{
    if(flagNumeratePorts)
    {
        searchPorts();

        if(midiin->isPortOpen() || midiout->isPortOpen())
        {
            if(outputPortsName->contains(selectedMidiDevice))
            {

            }
            else
            {
                previusMidiDevice = selectedMidiDevice;
                disconnectLoader();
            }
        }
        else
        {
            if(outputPortsName->contains(previusMidiDevice))
            {
                connectToDevice(outputPortsName->indexOf(previusMidiDevice));
            }
            else
            {
                for (int i=0;i<outputPortsName->size();i++)
                {
                    if(outputPortsName->at(i).contains("BOOT"))
                    {
                        connectToDevice(i);
                        break;
                    }
                }
            }
        }
    }
}

void loader::searchPorts()
{
    inputPortsName->clear();
    outputPortsName->clear();

    midiPortsList(outputPortsName,0);
    midiPortsList(inputPortsName,1);


    if(outputPortsName->count() && !midiDialog->isVisible())
    {
        midiDialog->midiDevice->clear();
        midiDialog->midiDevice->addItem(tr("Select"));
        for(int i=0;i<outputPortsName->count();i++)
        {
            //qDebug()<<outputPortsName->at(i);
            if(outputPortsName->at(i).contains(PORT_NAME_FILTER))
            {
                outputPortsIndex<<i;
                midiDialog->midiDevice->addItem(outputPortsName->at(i));
            }
        }

    }
}

void loader::midiPortsList(QStringList *list,int direction)
{
    std::string portName;

    if(direction)
    {
        // Check inputs.
        unsigned int nPorts = midiin->getPortCount();

        if(nPorts)
        {
            list->clear();
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
                QString port = QString().fromStdString(portName);

                list->append(port.left(port.lastIndexOf(QChar(' '))));
            }
        }
    }
    else
    {
        // Check outputs.
        unsigned int nPorts = midiout->getPortCount();

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

                QString port = QString().fromStdString(portName);
                list->append(port.left(port.lastIndexOf(QChar(' '))));
            }
        }
    }
}

void loader::updateStatus()
{
    ui->statusLabel->setText(tr("Device: %1").arg(selectedMidiDevice));

    if(flagReadyToUpload)
        ui->statusLabel->setStyleSheet("color: green");
    else
        ui->statusLabel->setStyleSheet("color: black");

    ui->actionFirmware_Update->setEnabled(flagReadyToUpload);
    ui->actionAux_Firmware_Update->setEnabled(flagReadyToUpload);
}

uint8_t loader::getCRC8(const std::vector<unsigned char> data, int len)
{
  int i=0;
  uint8_t crc = 0x00;
  while (len--)
  {
    uint8_t extract = data[i];i++;
    for (uint8_t tempI = 8; tempI; tempI--)
    {
      uint8_t sum = (crc ^ extract) & 0x01;
      crc >>= 1;
      if (sum)
        crc ^= 0x8C;

      extract >>= 1;
    }
  }
  return crc&0x7F;
}

unsigned char loader::getCheckSum(const std::vector<unsigned char> data, int len)
{
    unsigned char checksum = 0;

    for (int i = 0; i < len; i++)
        checksum ^= data[i];

    return checksum &= 0x7f;
}

unsigned char loader::getCheckSum(uint8_t *data, int len)
{
    unsigned char checksum = 0;

    for (int i = 0; i < len; i++)
        checksum ^= data[i];

    return checksum &= 0x7f;
}
bool loader::isSysExMessage(std::vector<unsigned char> message)
{
    if(message[0]==240 && message[message.size()-1]==247)
        return true;

    return false;
}

void loader::midiPull()
{

    std::vector<unsigned char> message;
    double stamp = midiin->getMessage( &message );

    //if(flagIgnoreMidiIn)
      //  return;

    int nBytes = message.size();


    if(nBytes>0 && isSysExMessage(message))
    {

        //erase sysex delimiters
        message.erase(message.begin());
        message.erase(message.end()-1);

        //bootloader order
        if(message[0] == SYSEX_ID0 && message[1]==SYSEX_ID1 &&  message[2]==SYSEX_ID2)
        {
            if(flagFirmwareUpdate)
            {
                if(message[ytxIOStructure::MESSAGE_STATUS]==STATUS_ACK)
                    firmwareUpdateBehavior(ACK_FIRMWARE_UPTADE);
                else if(message[ytxIOStructure::MESSAGE_STATUS]==STATUS_NAK)
                    firmwareUpdateBehavior(NAK_FIRMWARE_UPTADE);
            }
        }
    }
    else
    {

        for (int i=0; i<nBytes; i++ )
          qDebug() << "Byte " << i << " = " << (int)message[i] << ", ";
        if ( nBytes > 0 )
          qDebug() << "stamp = " << stamp;


    }
}

void loader::sendSysEx(uint8_t size,uint8_t *data)
{
    std::vector<unsigned char> sysexBlock;
    sysexBlock.clear();
    for (int i=0;i<size;i++)
        sysexBlock.push_back(data[i]);

    sysexBlock.insert(sysexBlock.begin(),240);
    sysexBlock.push_back( 247 );
    midiout->sendMessage( &sysexBlock );
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

