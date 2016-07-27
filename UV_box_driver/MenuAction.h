// MenuAction.h

#include "IAction.h"
#include "Menu.h"
#include "Keyboard.h"

#include <PCD8544.h>
#include <StandardCplusplus.h>
#include <list>

#ifndef _MENUACTION_h
#define _MENUACTION_h

#if defined(ARDUINO) && ARDUINO >= 100
    #include "arduino.h"
#else
    #include "WProgram.h"
#endif

class MenuAction : public IAction {
private:
    std::list<Menu> menu;
    PCD8544* lcd;
    Keyboard* keyboard;
    std::list<Menu>::iterator currentMenu;
    bool isCancellable = false;

    void printMenu();

public:
    MenuAction(std::list<Menu> menu, PCD8544* lcd, Keyboard* keyboard, bool isCancellable);

    virtual void execute();
};

#endif

