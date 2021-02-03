#include "loader.h"

loader::loader(QWidget *parent) : QMainWindow(parent),ui(new Ui::FirmwareLoader)
{
    ui->setupUi(this);

    //set progress dialog pointer to NULL
    progress = 0;

    flagReadyToUpload = 0;
    flagFillingPorts = 0;

    selectedMidiDevice = "None";

    initPath = QStandardPaths::standardLocations(QStandardPaths::TempLocation).at(0) + "/init_data.ytx";

    if(loadInitializationFile())
    {
        lastPath = context.path;
        this->setGeometry(context.x,context.y,context.width,context.height);
    }

    midiInit();

    createActions();

    updateStatus();
}

void loader::resizeEvent(QResizeEvent* event)
{
   QMainWindow::resizeEvent(event);


}
void loader::closeEvent(QCloseEvent *event)
{
    event->ignore();

    QPixmap logo = QPixmap(":/img/logo.png");

    QMessageBox msgBox;
    msgBox.setIconPixmap(logo);
    msgBox.setText(tr("Close Confirmation"));
    msgBox.setInformativeText(tr("Exit?"));
    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    msgBox.setDefaultButton(QMessageBox::Yes);
    msgBox.setButtonText(QMessageBox::Yes, tr("Yes"));
    msgBox.setButtonText(QMessageBox::No, tr("No"));

    if(msgBox.exec()==QMessageBox::Yes)
    {
        if(selectedMidiDevice.contains("KilomuxBOOT"))
        {
            std::vector<unsigned char> message;

            message.clear();

            message.push_back(REQUEST_RST);

            for(int j=0;j<sizeof(manufacturerHeader);j++)
                message.insert(message.begin()+j,manufacturerHeader[j]);

            message.push_back(getCheckSum(message,message.size()));

            //SysEx Header
            message.insert(message.begin(),240);
            message.push_back( 247 );

            midiout->sendMessage( &message );
        }
        saveInitializationFile();
        event->accept();
    }

}
int loader::min(int a,int b)
{
    if(a<b)
        return a;
    else
        return b;
}

loader::~loader()
{
    delete midiin;
    delete midiout;

    delete ui;
}




