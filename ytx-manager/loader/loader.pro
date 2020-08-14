#-------------------------------------------------
#
# Project created by QtCreator 2016-09-23T16:48:24
#
#-------------------------------------------------

#CONFIG += jack
CONFIG+=release
QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TEMPLATE = app

RESOURCES += loader.qrc

SOURCES += actions.cpp\
        main.cpp\
        loader.cpp\
        midi.cpp\
        file.cpp\
        RtMidi.cpp \
    firmwareupdate.cpp

HEADERS  += loader.h\
            defines.h\
            RtMidi.h

FORMS    += loader.ui



unix {
    macx{
        ICON = img/icono.icns
        DEFINES += __MACOSX_CORE__
        LIBS += -framework CoreMIDI -framework CoreAudio -framework CoreFoundation
    }
    else{
        DESTDIR = release
        jack {
            TARGET = ytxFirmareUploader-jack
            DEFINES += __UNIX_JACK__
            LIBS += -ljack
        }
        else {
            TARGET = ytxFirmareUploader-alsa
            DEFINES += __LINUX_ALSA__
            LIBS += -lasound -lpthread
        }
    }
}

win32{
    TARGET = ytxFirmareUploader
    DEFINES += __WINDOWS_MM__
    LIBS += -lwinmm
    RC_ICONS = img/icon.ico
}



