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

    DESTDIR = release
    jack {
        TARGET = loader-jack
        DEFINES += __UNIX_JACK__
        LIBS += -ljack
    }
    else {
        TARGET = loader-alsa
        DEFINES += __LINUX_ALSA__
        LIBS += -lasound -lpthread
    }
}
macx{
    ICON = icono.icns
    DEFINES += __MACOSX_CORE__
    LIBS += -framework CoreMIDI -framework CoreAudio -framework CoreFoundation
}

win32{
    TARGET = loader-win32
    DEFINES += __WINDOWS_MM__
    LIBS += -lwinmm
    RC_ICONS = img/icon.ico
}
