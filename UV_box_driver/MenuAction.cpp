// 
// 
// 

#include "MenuAction.h"

MenuAction::MenuAction(std::list<Menu> menu, PCD8544* lcd, Keyboard* keyboard, bool isCancellable){
    this->menu = menu;
    this->lcd = lcd;
    this->keyboard = keyboard;
    this->isCancellable = isCancellable;
}

void MenuAction::execute(){
    bool run = true;
    currentMenu = menu.begin();
    
    printMenu();

    while (run){
        if (keyboard->isPressed(up) && currentMenu != menu.begin())
        {
            currentMenu--;
            printMenu();
        }

        if (keyboard->isPressed(down) && &*currentMenu != &menu.back()){
            currentMenu++;
            printMenu();
        }

        if (keyboard->isPressed(enter)){
            currentMenu->execute();
            printMenu();
            
        }

        if (isCancellable && (keyboard->isPressed(left) || keyboard->isPressed(right))){
            run = false;
        }
    }
}

void MenuAction::printMenu(){
    byte line = 0;
    lcd->clear();

        
    for (std::list<Menu>::iterator i = menu.begin(); i != menu.end(); i++){
        lcd->setCursor(0, line++);

        if (i == currentMenu){
            lcd->print(">");
        }
        else{
            lcd->print(" ");
        }

        Serial.print("Menu: ");
        Serial.println(i->getName());
        lcd->print(i->getName());

        if (i == currentMenu){
            lcd->print("<");
        }
    }

    delay(500);
}
