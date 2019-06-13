#ifndef ytxTester_H
#define ytxTester_H

#include <QMainWindow>
#include "RtMidi.h"
#include <qdebug.h>
#include <QTimer>
#include <QFontDatabase>    
#include <QFile>
#include "types.h"
#include <QComboBox>
#include <QDialog>

//MIDI SYSEX
#define SYSEX_DEVICE_OMNI  				0x7F
#define SYSEX_ID_MANUFACTURER			0x60

#define SYSEX_ID_MODEL_OCTACONTROL      0x07
#define SYSEX_ID_MODEL_SEQUENCER        0x08

#define SYSEX_CMD_SET_SERIAL            8

// Platform-dependent sleep routines.
#if defined(__WINDOWS_MM__)
  #include <windows.h>
  #define SLEEP( milliseconds ) Sleep( (DWORD) milliseconds )
#else // Unix variants
  #include <unistd.h>
  #define SLEEP( milliseconds ) usleep( (unsigned long) (milliseconds * 1000.0) )
#endif
class midiPortsDialog : public QDialog
{
    Q_OBJECT

public:
    midiPortsDialog(QWidget *parent);

    QComboBox *midiDevice;
    QPushButton *accept;
};

namespace Ui {
class ytxTester;
}

class ytxTester : public QMainWindow
{
    Q_OBJECT

public:
    explicit ytxTester(QWidget *parent = 0);
    ~ytxTester();

private slots:

    void on_sendSerial_clicked();

    void midiPull();

private:
    Ui::ytxTester *ui;

    //Midi Objects

    RtMidiOut *midiout;
    RtMidiIn *midiin;
    midiPortsDialog *midiDialog;
    QStringList *inputPortsName;
    QStringList *outputPortsName;
    QString selectedMidiDevice;
    QList<int>  outputPortsIndex;

    //Actions & Menus
    QAction *outputPortMenu;

    ytxConfigurationType config;
    ytxEncoderType encoder;
    ytxAnalogType analog;
    ytxDigitaltype button;
    ytxFeedbackType feedback;

    QList <uint8_t> productId;
    void midiInit();
    void outputPortsList(QStringList *);
    void inputPortsList(QStringList *);
    void connectToDevice(int);
    void numeratePorts();
    unsigned char getCheckSum(const std::vector<unsigned char> data, int len);
    bool isSysExMessage(std::vector<unsigned char>);
    void sendSysEx(uint8_t ,uint8_t *);
};

#endif // ytxTester_H
