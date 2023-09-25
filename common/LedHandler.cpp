#include "LedHandler.h"

LedHandler::LedHandler(int ledPin) : timer(std::bind(&LedHandler::handleLed, this), 100)
{
    this->ledPin = ledPin;
}

void LedHandler::setMaxValue(int value)
{
    this->maxValue = value;
    this->valueToSet = value;
}

int LedHandler::getMaxValue()
{
    return this->maxValue;
}

void LedHandler::turnOn() 
{
    this->valueToSet = this->maxValue;
}

void LedHandler::turnOff()
{
    this->valueToSet = 0;
}

void LedHandler::setValue(int value)
{
    this->valueToSet = value;
}

void LedHandler::setup()
{
    pinMode(this->ledPin, OUTPUT);
    this->timer.start();
}

void LedHandler::handle()
{
    this->timer.update();
}

void LedHandler::handleLed()
{
    if (this->value == this->valueToSet)
    {
        return;
    }

    int newValue;
    bool wasChanged = false;
    if (this->valueToSet > this->value)
    {
        newValue = this->value + this->interval;
        this->value = newValue > this->valueToSet ? this->valueToSet : newValue;
        wasChanged = true;
    } else 
    {
        newValue = this->value - this->interval;
        this->value = newValue < this->valueToSet ? this->valueToSet : newValue;
        wasChanged = true;
    }

    if(wasChanged)
    {
        analogWrite(this->ledPin, this->value);
    }
}

int LedHandler::getValue() {
    return this->value;
}

bool LedHandler::isOn()
{
    return this->valueToSet > 0;
}