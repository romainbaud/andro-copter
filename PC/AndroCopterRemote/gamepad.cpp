#include "gamepad.h"

#include <SDL2/SDL.h>

Gamepad::Gamepad()
{
    joystick = 0;

    // Setup SDL.
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK);
    SDL_JoystickEventState(SDL_ENABLE);
}

Gamepad::~Gamepad()
{
    if(joystick != 0)
    {
        SDL_JoystickClose(joystick);
        joystick = 0;
    }

    SDL_JoystickEventState(SDL_DISABLE);
    SDL_QuitSubSystem(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK);
}

QStringList Gamepad::getGamepadsList()
{
    int nGamepads = SDL_NumJoysticks();

    QStringList list;

    for(int i=0; i<nGamepads; i++)
        list << SDL_JoystickNameForIndex(i);

    return list;
}

void Gamepad::startMonitoring(int index)
{
    joystick = SDL_JoystickOpen(index);

    if(joystick == 0)
        qDebug() << "Error while opening the joystick, index:" << index;

    name = SDL_JoystickName(joystick);
    axes.resize(SDL_JoystickNumAxes(joystick));
    hats.resize(SDL_JoystickNumHats(joystick));
    buttons.resize(SDL_JoystickNumButtons(joystick));
}

QString Gamepad::getName()
{
    return name;
}

QVector<int> Gamepad::getAxes()
{
    SDL_JoystickUpdate();

    for(int i=0; i<axes.size(); i++)
        axes[i] = SDL_JoystickGetAxis(joystick, i);

    return axes;
}

QVector<int> Gamepad::getHats()
{
    SDL_JoystickUpdate();

    for(int i=0; i<hats.size(); i++)
        hats[i] = SDL_JoystickGetHat(joystick, i);

    return hats;
}

QVector<bool> Gamepad::getButtons()
{
    SDL_JoystickUpdate();

    for(int i=0; i<buttons.size(); i++)
        buttons[i] = SDL_JoystickGetButton(joystick, i);

    return buttons;
}

bool Gamepad::isGamepadStillConnected()
{
    return SDL_JoystickGetAttached(joystick);
}
