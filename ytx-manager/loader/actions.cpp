#include "loader.h"

void loader::createActions()
{
    connect(ui->actionExit,SIGNAL(triggered(bool)),this,SLOT(close()));
    connect(ui->actionFirmware_Update,SIGNAL(triggered(bool)),this,SLOT(firmwareUpdate()));
    connect(ui->actionAux_Firmware_Update,SIGNAL(triggered(bool)),this,SLOT(firmwareUpdate()));
    connect(ui->actionEEPROM_erase,SIGNAL(triggered(bool)),this,SLOT(EEPROM_erase()));

    #ifndef __UNIX_JACK__
      ui->actionFirmware_Update->setDisabled(1);
      ui->actionAux_Firmware_Update->setDisabled(1);
      ui->actionEEPROM_erase->setDisabled(1);

    #endif

    midiportsAct = ui->optionsMenu->addAction(tr("MIDI Device"));
    connect(midiportsAct,SIGNAL(triggered()),midiDialog,SLOT(show()));
}


void loader::EEPROM_erase()
{
    QPixmap logo = QPixmap(":/img/logo.png");

    QMessageBox msgBox;
    msgBox.setIconPixmap(logo);
    msgBox.setText(tr("Erase Confirmation"));
    msgBox.setInformativeText(tr("This action will erase all your controller's configuration. Are you brave enough?"));
    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    msgBox.setDefaultButton(QMessageBox::Yes);
    msgBox.setButtonText(QMessageBox::Yes, tr("Yes"));
    msgBox.setButtonText(QMessageBox::No, tr("No"));

    if(msgBox.exec()==QMessageBox::Yes)
    {
        if(midiout->isPortOpen())
        {
            std::vector<unsigned char> message;

            message.clear();

            message.push_back(REQUEST_EEPROM_ERASE);

            for(int j=0;j<sizeof(manufacturerHeader);j++)
                message.insert(message.begin()+j,manufacturerHeader[j]);

            message.push_back(getCheckSum(message,message.size()));

            //SysEx Header
            message.insert(message.begin(),240);
            message.push_back( 247 );

            midiout->sendMessage( &message );
        }
    }
}
