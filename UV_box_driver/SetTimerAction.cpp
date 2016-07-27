// 
// 
// 

#include "SetTimerAction.h"


byte SetTimerAction::getDataLength(){
    return 4;
}

Timer SetTimerAction::getTimer(){
    loadTime();
    return Timer(hours, minutes, seconds);
}

SetTimerAction::SetTimerAction(String* name, DataStorage* storage, PCD8544* lcd, Keyboard* keyboard){
    this->name = name;
    this->storage = storage;
    this->lcd = lcd;
    this->keyboard = keyboard;

    storage->registerSaveable(this);
}

void SetTimerAction::loadTime(){
    unsigned long time = storage->loadLong(this);

    hours = time / (60 * 60);
    minutes = time / 60 - (hours * 60);
    seconds = time % 60;

    fixTime();
}

void SetTimerAction::fixTime(){
    if (hours > maxHours){
        hours = maxHours;
    }

    if (minutes > maxMinutes){
        minutes = maxMinutes;
    }

    if (seconds > maxSeconds){
        seconds = maxSeconds;
    }
}

void SetTimerAction::execute(){
    bool run = true;
    loadTime();
    print();  

    while (run){
        if (keyboard->isPressed(up)){
            if (mode == Seconds){
                seconds = seconds >= maxSeconds ? 0 : seconds + 1;
            }
            else if (mode == Minutes){
                minutes = minutes >= maxMinutes ? 0 : minutes + 1;
            }
            else if (mode == Hours){
                hours = hours >= maxHours ? 0 : hours + 1;
            }

            print();
        }

        if (keyboard->isPressed(down)){
            if (mode == Seconds){
                seconds = seconds <= 0 ? maxSeconds : seconds - 1;
            }
            else if (mode == Minutes){
                minutes = minutes <= 0 ? maxMinutes : minutes - 1;
            }
            else if (mode == Hours){
                hours = hours <= 0 ? maxHours : hours - 1;
            }

            print();
        }

        if (keyboard->isPressed(left) && mode < Hours){
            mode = (SetTimerMode)(mode + 1);
            print();
        }

        if (keyboard->isPressed(right) && mode > Seconds){
            mode = (SetTimerMode)(mode - 1);
            print();
        }

        if (keyboard->isPressed(enter)){
            run = false;
            
            unsigned long time = seconds + (unsigned long)minutes * 60 + (unsigned long)hours * 60 * 60;
            Serial.println(time);
            storage->save(this, time);
        }
    }
}

String SetTimerAction::getTimerName(){
    return *name;
}

void SetTimerAction::print(){
    lcd->clear();
    lcd->setCursor(0, 0);
    lcd->print(*name);
    lcd->setCursor(0, 2);
    lcd->print("   ");
    print(hours);
    lcd->print(":");
    print(minutes);
    lcd->print(":");
    print(seconds);

    lcd->setCursor(0, 4);
    switch (mode){
    case Hours:
        lcd->print("Hours");
        break;

    case Minutes:
        lcd->print("Minutes");
        break;

    case Seconds:
        lcd->print("Seconds");
        break;
    }

    delay(500);
}

void SetTimerAction::print(byte data){
    if (data < 10){
        lcd->print("0");
    }
    lcd->print(data);
}

Timer::Timer(byte hours, byte minutes, byte seconds){
    this->hours = hours;
    this->minutes = minutes;
    this->seconds = seconds;
}

bool Timer::tact(){
    if (seconds > 0){
        seconds--;
        return true;
    }
    else if (minutes > 0){
        minutes--;
        seconds = 59;
        return true;
    }
    else if (hours > 0){
        hours--;
        minutes = 59;
        seconds = 59;
        return true;
    }
    else{
        return false;
    }
}