#include "gamepad.h"

#include <SDL/SDL.h>

Gamepad::Gamepad()
{
    joystick = 0;

    // Setup SDL.
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK);
    SDL_JoystickEventState(SDL_ENABLE);
}

Gamepad::~Gamepad()
{
    loopAgain = false;
    wait(1000); // Let some time for the joystick thread to stop.

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
    int nGP = SDL_NumJoysticks();

    QStringList list;

    for(int i=0; i<nGP; i++)
        list << SDL_JoystickName(i);

    return list;
}

void Gamepad::startMonitoring(int index)
{
    joystick = SDL_JoystickOpen(index);

    joystickIndex = index;

    name = SDL_JoystickName(index);
    axes.resize(SDL_JoystickNumAxes(joystick));
    hats.resize(SDL_JoystickNumHats(joystick));
    buttons.resize(SDL_JoystickNumButtons(joystick));

    // Start events monitoring.
    start();
}

void Gamepad::run()
{
    SDL_Event event;

    loopAgain = true;

    while(loopAgain)
    {
        while(SDL_PollEvent(&event))
        {
            if(event.jbutton.which == joystickIndex
               || event.jaxis.which == joystickIndex
               || event.jhat.which == joystickIndex)
            {
                switch(event.type)
                {
                case SDL_JOYBUTTONDOWN:
                    buttons[event.jbutton.button] = true;
                    break;

                case SDL_JOYBUTTONUP:
                    buttons[event.jbutton.button] = false;
                    break;

                case SDL_JOYAXISMOTION:
                    axes[event.jaxis.axis] = event.jaxis.value;
                    break;

                case SDL_JOYHATMOTION:
                    hats[event.jhat.hat] = event.jhat.value;
                    break;

                default:
                    break;
                }
            }
        }

        msleep(10); // Sleep a bit to not overload the CPU.
    }
}

QString Gamepad::getName()
{
    return name;
}

QVector<int> Gamepad::getAxes()
{
    return axes;
}

QVector<int> Gamepad::getHats()
{
    return hats;
}

QVector<bool> Gamepad::getButtons()
{
    return buttons;
}

// FIXME : Does not work !
bool Gamepad::isGamepadStillConnected()
{
    return getGamepadsList().contains(name);
}
