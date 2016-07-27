// TimerAction.h

#include "IAction.h"
#include "ISaveable.h"
#include "Keyboard.h"
#include "SetTimerAction.h"
#include "BackLightAction.h"

#include <PCD8544.h>

#ifndef _TIMERACTION_h
#define _TIMERACTION_h

#if defined(ARDUINO) && ARDUINO >= 100
    #include "arduino.h"
#else
    #include "WProgram.h"
#endif

class TimerAction : public IAction{
private:
    PCD8544* lcd;
    Keyboard* keyboard;
    ITimerData* timerData;
    IBackLight* backLight;
    byte triggerPin;

    void print();
    void printInfo(Timer timer);
    void print(Timer timer);
    void print(byte data);
    void start(Timer timer);

public:
    TimerAction(byte triggerPin, PCD8544* lcd, Keyboard* keyboard, ITimerData* timerData, IBackLight* backLight);

    virtual void execute();
};

#endif

