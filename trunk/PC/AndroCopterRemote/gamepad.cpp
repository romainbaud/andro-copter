#include "gamepad.h"

#include <SFML/Window/Joystick.hpp>

using namespace sf;

#define JOYSTICK_AXIS_MAX 100.0

Gamepad::Gamepad()
{
    gamepadIndex = 0;
}

Gamepad::~Gamepad()
{

}

QStringList Gamepad::getGamepadsList()
{
    Joystick::update();

    QStringList list;

    for(int i=0; Joystick::isConnected(i); i++)
    {
        list << QString::fromStdString(Joystick::getIdentification(i).name.toAnsiString());
        qDebug() << QString::fromStdString(Joystick::getIdentification(i).name.toAnsiString()) << " "
                 << Joystick::getIdentification(i).productId << " "
                 << Joystick::getIdentification(i).vendorId << endl;
    }

    return list;
}

void Gamepad::startMonitoring(int index)
{
    if(!Joystick::isConnected(index))
    {
        qDebug() << "Error while opening the joystick, index:" << index;
        return;
    }

    gamepadIndex = index;

    // Get the name of the gamepad.
    name = QString::fromStdString(Joystick::getIdentification(gamepadIndex).name.toAnsiString());

    // Count the number of axes.
    int nAxes = 0;
    while(Joystick::hasAxis(gamepadIndex, (Joystick::Axis)nAxes))
        nAxes++;

    axes.resize(nAxes);

    // Count the number of buttons.
    buttons.resize(Joystick::getButtonCount(gamepadIndex));

    qDebug() << "Detected " << nAxes << " axes and " << buttons.size() << " buttons." << endl;
}

QString Gamepad::getName()
{
    return name;
}

QVector<double> Gamepad::getAxes()
{
    Joystick::update();

    for(int i=0; i<axes.size(); i++)
        axes[i] = (double)Joystick::getAxisPosition(gamepadIndex, (Joystick::Axis)i) / JOYSTICK_AXIS_MAX;

    return axes;
}

QVector<bool> Gamepad::getButtons()
{
    Joystick::update();

    for(int i=0; i<buttons.size(); i++)
        buttons[i] = Joystick::isButtonPressed(gamepadIndex, i);

    return buttons;
}

bool Gamepad::isGamepadStillConnected()
{
    return Joystick::isConnected(gamepadIndex) &&
           (QString::fromStdString(Joystick::getIdentification(gamepadIndex).name.toAnsiString()) == name);
}
