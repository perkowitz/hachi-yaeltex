#include "loader.h"

ytxHeaderType fileHeader = {"ytx",LOADER_VERSION};

int loader::loadInitializationFile()
{
    int fileOK =  (int)fileExists(initPath);

    if(fileOK)
    {
        QFile file;

        file.setFileName(initPath);


        if (file.open(QIODevice::ReadOnly) && file.size()==YTX_INI_FILE_SIZE)
        {
            QDataStream in(&file);
            ytxHeaderType header;

            in.readRawData((char*)&header,sizeof(fileHeader));

            if(memcmp(&header,&fileHeader,sizeof(fileHeader))==0)
            {
                in.readRawData((char*)&context,sizeof(context));
            }
        }
    }

    return fileOK;
}

void loader::saveInitializationFile()
{
    context.x = this->x();
    context.y = this->y();
    context.width = this->width();
    context.height = this->height();

    if(lastPath.length()>sizeof(context.path))
        lastPath.clear();
    strcpy(context.path,path().toStdString().c_str());

    QFile file;
    file.setFileName(initPath);


    if (file.open(QIODevice::WriteOnly))
    {
        QDataStream out(&file);
        //Header
        out.writeRawData((char*)&fileHeader,sizeof(fileHeader));
        //Data
        out.writeRawData((char*)&context,sizeof(context));

        file.close();
    }
}
bool loader::fileExists(QString path)
{
    QFileInfo check_file(path);
    // check if file exists and if yes: Is it really a file and no directory?
    if (check_file.exists() && check_file.isFile())
        return true;
    else
        return false;
}

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

    int index = lastPath.lastIndexOf("/");
    lastPath = lastPath.remove(index,lastPath.length()-index);
}


void loader::firmwareUpdate()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open File"),this->path(),tr("BIN (*.bin)"));

    if(!fileName.isEmpty())
    {
        setLastPath(fileName);
        createSysExFirmwareMessages(fileName);

        QFile file;
        file.setFileName(fileName);
        int targetSize = file.size();
        qDebug()<<"size: "<<targetSize;
        QString error;

        QObject *sender = QObject::sender();
        if(sender==ui->mainSoftwarePush)
        {
            if(targetSize>YTX_MAIN_MIN_SIZE && targetSize<YTX_MAIN_MAX_SIZE)
                firmwareUpdateBehavior(BEGIN_FIRMWARE_UPTADE,REQUEST_UPLOAD_SELF);
            else
                error = "\n\nWrong file size!\n\nExpected 32kb to 224kb file";
        }
        else if(sender==ui->auxSoftwarePush)
        {
            if(targetSize>YTX_AUX_MIN_SIZE && targetSize<YTX_AUX_MAX_SIZE)
                firmwareUpdateBehavior(BEGIN_FIRMWARE_UPTADE,REQUEST_UPLOAD_OTHER);
            else
                error = "\n\nWrong file size!\n\nExpected 4kb to 28kb file";
        }

        if(!error.isNull())
        {
            QPixmap logo = QPixmap(":/img/logo.png");

            QMessageBox msgBox;
            msgBox.setIconPixmap(logo);
            msgBox.setText(error);
            msgBox.setStandardButtons(QMessageBox::Ok);;

            msgBox.exec();
        }
    }
}
