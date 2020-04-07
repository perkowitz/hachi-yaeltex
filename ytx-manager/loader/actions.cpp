#include "loader.h"

void loader::createActions()
{
    connect(ui->actionExit,SIGNAL(triggered(bool)),this,SLOT(close()));
    connect(ui->actionFirmware_Update,SIGNAL(triggered(bool)),this,SLOT(firmwareUpdate()));
    connect(ui->actionAux_Firmware_Update,SIGNAL(triggered(bool)),this,SLOT(firmwareUpdate()));

    #ifndef __UNIX_JACK__
      ui->actionFirmware_Update->setDisabled(1);
      ui->actionAux_Firmware_Update->setDisabled(1);
    #endif

    midiportsAct = ui->optionsMenu->addAction(tr("MIDI Device"));
    connect(midiportsAct,SIGNAL(triggered()),midiDialog,SLOT(show()));
}
