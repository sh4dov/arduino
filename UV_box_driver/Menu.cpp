// 
// 
// 

#include "Menu.h"

Menu::Menu(String* name, IAction *action)
{
    this->name = name;
    this->action = action;
}

String Menu::getName()
{
    return *name;
}

void Menu::execute()
{
    action->execute();
}