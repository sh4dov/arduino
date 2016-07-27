// Menu.h

#include "IAction.h"

#ifndef _MENU_h
#define _MENU_h

#if defined(ARDUINO) && ARDUINO >= 100
    #include "arduino.h"
#else
    #include "WProgram.h"
#endif


class Menu
{
private:
    String* name;
    IAction *action;

public:
    Menu(String* name, IAction *action);
    void execute();
    String getName();
};

#endif

