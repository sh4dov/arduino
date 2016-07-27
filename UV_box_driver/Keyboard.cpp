// 
// 
// 

#include "Keyboard.h"

Keyboard::Keyboard(byte upPin, byte downPin, byte leftPin, byte rightPin, byte enterPin){
    keyPins[up] = upPin;
    keyPins[down] = downPin;
    keyPins[left] = leftPin;
    keyPins[right] = rightPin;
    keyPins[enter] = enterPin;
}

bool Keyboard::isPressed(Key key){
    return digitalRead(keyPins[key]);
}
