/*!
* \file main.cpp
* \brief The starting point of the program.
* \author Romain Baud
* \version 0.1
* \date 2012.11.26
*
* \mainpage AndroCopterRemote
* \author Romain Baud
* \section Introduction
* iFly remote is a software to control an smartphone running the AndroCopter
* app.
* It uses mainly the Qt 4 library, and is multi-platform.
*
* \section Features
* -Graphical monitoring of the iFly state.
* -Control with a XBox 360 gamepad.
*
* \section Instructions
* Read the documentation enclosed in the project report.
*/

#include <QtWidgets>
#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

	// Set the name and company of the application. This will be used to save
	// the application settings.
    QCoreApplication::setOrganizationName("Romain Baud");
    QCoreApplication::setApplicationName("AndroCopter remote");

	// Create the main window, and display it. The Qt main loop will now run.
    MainWindow w;
    w.show();
    
    return a.exec();
}
