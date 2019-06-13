#-------------------------------------------------
#
# Project created by QtCreator 2017-05-28T19:57:23
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TEMPLATE = app


SOURCES += main.cpp\
        ytxTester.cpp \
        RtMidi.cpp

HEADERS  += ytxTester.h \
    defines.h \
    types.h

FORMS    += ytxTester.ui



unix {

    DESTDIR = release
    jack {
        TARGET = ytxTester-jack
        DEFINES += __UNIX_JACK__
        LIBS += -ljack
    }
    else {
        TARGET = ytxTester-alsa
        DEFINES += __LINUX_ALSA__
        LIBS += -lasound -lpthread
    }
}
win32:TARGET = ytxTester-win32
win32:DEFINES += __WINDOWS_MM__
win32:LIBS += -lwinmm
