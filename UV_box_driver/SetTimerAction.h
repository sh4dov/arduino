// SetTimerAction.h

#include "IAction.h"
#include "ISaveable.h"
#include "DataStorage.h"
#include "Keyboard.h"

#include <PCD8544.h>

#ifndef _SETTIMERACTION_h
#define _SETTIMERACTION_h

#if defined(ARDUINO) && ARDUINO >= 100
    #include "arduino.h"
#else
    #include "WProgram.h"
#endif

class Timer{
public:
    byte minutes, seconds, hours;
    Timer(byte hours, byte minutes, byte seconds);

    bool tact();
};

class ITimerData{
public:
    virtual Timer getTimer() = 0;
    virtual String getTimerName() = 0;
};

enum SetTimerMode{
    Seconds = 1,
    Minutes = 2,
    Hours = 3
};

class SetTimerAction : public IAction, public ITimerData, private ISaveable {
private:
    const byte maxHours = 59;
    const byte maxMinutes = 59;
    const byte maxSeconds = 59;
    byte hours = 0;
    byte minutes = 0;
    byte seconds = 0;
    SetTimerMode mode = Seconds;
    DataStorage* storage;
    PCD8544* lcd;
    Keyboard* keyboard;
    String* name;

    void fixTime();
    void print();
    void print(byte data);
    void loadTime();
    
    virtual byte getDataLength();
    

public:
    SetTimerAction(String* name, DataStorage* storage, PCD8544* lcd, Keyboard* keyboard);

    virtual void execute();
    virtual Timer getTimer();
    virtual String getTimerName();
};

#endif

