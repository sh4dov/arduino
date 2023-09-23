#pragma once

#include <Arduino.h>
#include <TickTwo.h>

class LedHandler {
    private:
        int ledPin;
        int maxValue = 255;
        int value = 0;
        int valueToSet = 0;
        byte interval = 1;
        TickTwo timer;

        void handleLed();

    public:
        LedHandler(int ledPin);
        void setMaxValue(int value);
        int getMaxValue();
        void turnOn();
        void turnOff();
        void setValue(int value);
        void setup();
        void handle();
        int getValue();
        bool isOn();
};