#include "loader.h"

loader::loader(QWidget *parent) : QMainWindow(parent),ui(new Ui::FirmwareLoader)
{
    ui->setupUi(this);

    //set progress dialog pointer to NULL
    progress = 0;

    flagReadyToUpload = 0;
    selectedMidiDevice = "None";

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
        event->accept();

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










