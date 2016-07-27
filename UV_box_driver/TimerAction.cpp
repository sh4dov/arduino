// 
// 
// 

#include "TimerAction.h"

TimerAction::TimerAction(byte triggerPin, PCD8544* lcd, Keyboard* keyboard, ITimerData* timerData, IBackLight* backLight){
    this->triggerPin = triggerPin;
    this->lcd = lcd;
    this->keyboard = keyboard;
    this->timerData = timerData;
    this->backLight = backLight;
}

void TimerAction::execute(){
    bool run = true;
    Timer timer = timerData->getTimer();
    printInfo(timer);

    while (run){
        if (keyboard->isPressed(enter)){
            run = false;
            start(timer);
        }

        if (keyboard->isPressed(up) ||
            keyboard->isPressed(down) ||
            keyboard->isPressed(left) ||
            keyboard->isPressed(right)){
            run = false;
        }
    }
}

void TimerAction::start(Timer timer){
    digitalWrite(triggerPin, HIGH);
    backLight->turnOn();

    while (timer.tact()){
        lcd->clear();
        lcd->setCursor(0, 2);
        lcd->print("   ");
        print(timer);

        delay(1000);
    }

    digitalWrite(triggerPin, LOW);
    backLight->toDefault();
}

void TimerAction::printInfo(Timer timer){
    lcd->clear();
    lcd->setCursor(0, 0);
    lcd->print(timerData->getTimerName());
    lcd->setCursor(0, 1);
    lcd->print("set to:");
    lcd->setCursor(0, 2);
    lcd->print("   ");
    print(timer);
    lcd->setCursor(0, 4);
    lcd->print("press enter");
    lcd->setCursor(0, 5);
    lcd->print("to continue");
    delay(500);
}

void TimerAction::print(){
}

void TimerAction::print(Timer timer){
    print(timer.hours);
    lcd->print(":");
    print(timer.minutes);
    lcd->print(":");
    print(timer.seconds);
}

void TimerAction::print(byte data){
    if (data < 10){
        lcd->print(0);
    }

    lcd->print(data);
}