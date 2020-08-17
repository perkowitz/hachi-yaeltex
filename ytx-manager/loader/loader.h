#ifndef LOADER_H
#define LOADER_H

#include "ui_loader.h"
#include "defines.h"


#include <QMainWindow>
#include <QTranslator>
#include <QApplication>
#include <QFontDatabase>
#include <iostream>
#include <cstdlib>
#include <signal.h>
#include "RtMidi.h"
#include <QDebug>
#include <QTimer>
#include <QAction>
#include <QContextMenuEvent>
#include <QShortcut>
#include <QFile>
#include <QFileDialog>
#include <QDir>
#include <QSortFilterProxyModel>
#include <QFileSystemModel>
#include <QPushButton>
#include <QSlider>
#include <QLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QRadioButton>
#include <QLabel>
#include <QButtonGroup>
#include <QGroupBox>
#include <QStyledItemDelegate>
#include <QPainter>
#include <QTime>
#include <QListWidget>
#include <QProxyStyle>
#include <QSignalMapper>
#include <QGridLayout>
#include <QSpinBox>
#include <QSizePolicy>
#include <QDir>
#include <stdint.h>
#include <QMessageBox>
#include <QDialogButtonBox>
#include <QProgressDialog>
#include <QThread>
#include <QSplashScreen>
#include <QLineEdit>
#include <QScrollBar>
#include <QRadioButton>
#include <QHeaderView>
#include <QUndoCommand>
#include <QPoint>
#include <QComboBox>


class midiPortsDialog : public QDialog
{
    Q_OBJECT

public:
    midiPortsDialog(QWidget *parent);

    QComboBox *midiDevice;
    QPushButton *accept;
};

QT_USE_NAMESPACE
namespace Ui {
class FirmwareLoader;
}

class loader : public QMainWindow
{
    Q_OBJECT

public:
    explicit loader(QWidget *parent = 0);
    ~loader();

private slots:
    void midiPull();

    void EEPROM_erase();

    void firmwareUpdate();

    void connectToDevice(int index);

    void searchPorts();

    void checkMidiConection();

private:

    Ui::FirmwareLoader *ui;

    //Styles
    QString spinBoxstyleSheet,labelStyleSheet,containerStyleSheet;

    //Midi Objects
    RtMidiIn  *midiin;
    RtMidiOut *midiout;
    QStringList *inputPortsName;
    QStringList *outputPortsName;

    //Actions & Menus
    QUndoStack *undoStack;
    QAction *undoAction;
    QAction *redoAction;
    QAction *copyAction;
    QAction *pasteAction;

    QAction *midiportsAct;
    QMenu *inputPortMenu;
    QMenu *outputPortMenu;
    QList<QAction*> inputPortsActions;
    QList<QAction*> outputPortsActions;
    QList <int> outputPortsIndex;

    //Global
    midiPortsDialog *midiDialog;
    QString selectedMidiInPort;
    QString selectedMidiOutPort;
    QString previusMidiDevice;
    QString selectedMidiDevice;
    QList <std::vector<unsigned char> > firmwareSysExMessages;

    uint8_t manufacturerHeader[6] = {SYSEX_ID0,SYSEX_ID1,SYSEX_ID2,0,0,0};

    QProgressDialog *progress;

    QString lastPath;
    QString lastFile;

    int flagFirmwareUpdate;
    int flagIgnoreMidiIn;
    int flagNumeratePorts;
    int flagReadyToUpload;

    void createActions();

    void updateStatus();

    QString path();

    void setLastPath(QString path);

    void midiInit();

    void midiPortsList(QStringList *list, int direction);

    void disconnectLoader();

    void createSysExFirmwareMessages(QString fileName);

    void firmwareUpdateBehavior(int parameter, unsigned char option=0);

    uint8_t encodeSysEx(QByteArray inData, std::vector<unsigned char> *outSysEx, uint8_t inLength);

    unsigned char getCheckSum(const std::vector<unsigned char> data, int len);

    unsigned char getCheckSum(uint8_t *data, int len);

    uint8_t getCRC8(const std::vector<unsigned char> data, int len);

    bool isSysExMessage(std::vector<unsigned char> message);

    int min(int,int);

    void sendSysEx(uint8_t size,uint8_t *data);

    void updateChannelsName();

protected:
    virtual void resizeEvent(QResizeEvent *event);
    virtual void closeEvent(QCloseEvent *event);
    //Q_DECL_OVERRIDE
};



#endif // loader_H
