#-------------------------------------------------
#
# Project created by QtCreator 2012-08-09T18:01:02
#
#-------------------------------------------------

QT += core widgets gui network

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
INCLUDEPATH += YOUR_SFML_PATH_HERE/SFML-2.3/include
LIBS += -LYOUR_SFML_PATH_HERE/SFML-2.3/lib -lsfml-main -lsfml-system -lsfml-window
