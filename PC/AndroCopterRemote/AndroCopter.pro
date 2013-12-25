#-------------------------------------------------
#
# Project created by QtCreator 2012-08-09T18:01:02
#
#-------------------------------------------------

QT       += core gui network phonon

TARGET = AndroCopterRemote
TEMPLATE = app


SOURCES += main.cpp mainwindow.cpp \
    gamepad.cpp \
    plotter.cpp \
    pid.cpp \
    spacespin.cpp

HEADERS  += mainwindow.h \
    gamepad.h \
    plotter.h \
    constants.h \
    pid.h \
    spacespin.h

FORMS    += mainwindow.ui

RESOURCES +=

# Icon of the application executable.
win32:RC_FILE += win_icon.rc

# The SDL path. Change it according to your installation.
INCLUDEPATH += D:/rbaud/Programmation/Librairies/SDL-2.0/include
LIBS += -LD:/rbaud/Programmation/Librairies/SDL-2.0/lib -lSDL2

