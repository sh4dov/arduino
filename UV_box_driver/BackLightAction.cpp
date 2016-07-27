// 
// 
// 

#include "BackLightAction.h"

BackLightAction::BackLightAction(byte pin, DataStorage* storage, PCD8544* lcd, Keyboard* keyboard){
    this->pin = pin;
    this->lcd = lcd;
    this->storage = storage;
    this->keyboard = keyboard;

    storage->registerSaveable(this);

    load();
    updateBackLight();
}

byte BackLightAction::getDataLength(){
    return sizeof(isOn);
}

void BackLightAction::execute(){
    bool run = true;
    print();

    while (run){
        if (keyboard->isPressed(up) || keyboard->isPressed(down)){
            isOn = !isOn;
            updateBackLight();
            print();
        }

        if (keyboard->isPressed(enter)){
            run = false;
            storage->save(this, isOn);
            updateBackLight();
        }

        if (keyboard->isPressed(left) || keyboard->isPressed(right)){
            run = false;
            isOn = storage->loadBool(this);
            updateBackLight();
        }
    }
}

void BackLightAction::load(){
    isOn = storage->loadBool(this);
}

void BackLightAction::turnOn(){
    isOn = true;
    updateBackLight();
}

void BackLightAction::toDefault(){
    load();
    updateBackLight();
}

void BackLightAction::print(){
    lcd->clear();
    lcd->setCursor(0, 2);
    lcd->print("back light:   ");
    lcd->print(isOn ? "on" : "off");
    delay(500);
}

void BackLightAction::updateBackLight(){
    digitalWrite(pin, isOn ? LOW : HIGH);
}