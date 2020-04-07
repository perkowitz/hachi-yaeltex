#include "loader.h"


QString loader::path()
{
    QString path;

    if(lastPath.isEmpty())
        path = QDir::homePath();
    else
        path = lastPath;
    return path;
}

void loader::setLastPath(QString path)
{
    lastPath = path;
    lastFile = path;

    int index = lastPath.lastIndexOf("/");
    lastPath = lastPath.remove(index,lastPath.length()-index);

    this->setWindowTitle(QString("loader - ") + path);
}


void loader::firmwareUpdate()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open File"),this->path(),tr("BIN (*.bin)"));

    if(!fileName.isEmpty())
    {
        createSysExFirmwareMessages(fileName);

        QObject *sender = QObject::sender();
        if(sender==ui->actionFirmware_Update)
            firmwareUpdateBehavior(BEGIN_FIRMWARE_UPTADE,REQUEST_UPLOAD_SELF);
        else if(sender==ui->actionAux_Firmware_Update)
            firmwareUpdateBehavior(BEGIN_FIRMWARE_UPTADE,REQUEST_UPLOAD_OTHER);
    }
}
