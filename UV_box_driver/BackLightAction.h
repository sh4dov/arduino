// BackLightAction.h

#include "IAction.h"
#include "ISaveable.h"
#include "DataStorage.h"
#include "Keyboard.h"

#include <PCD8544.h>

#ifndef _BACKLIGHTACTION_h
#define _BACKLIGHTACTION_h

#if defined(ARDUINO) && ARDUINO >= 100
    #include "arduino.h"
#else
    #include "WProgram.h"
#endif

class IBackLight{
public:
    virtual void turnOn() = 0;
    virtual void toDefault() = 0;
};

class BackLightAction : public IAction, private ISaveable, public IBackLight {
private:
    byte pin;
    bool isOn = false;
    DataStorage* storage;
    PCD8544* lcd;
    Keyboard* keyboard;

    virtual byte getDataLength();
    void updateBackLight();
    void print();
    void turnOn();
    void toDefault();
    void load();

public:
    BackLightAction(byte pin, DataStorage* storage, PCD8544* lcd, Keyboard* keyboard);

    virtual void execute();
};

#endif

