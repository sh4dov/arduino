// Keyboard.h

#ifndef _KEYBOARD_h
#define _KEYBOARD_h

#if defined(ARDUINO) && ARDUINO >= 100
    #include "arduino.h"
#else
    #include "WProgram.h"
#endif

enum Key{
    up = 0,
    down,
    left,
    right,
    enter
};

class Keyboard{
private:
    byte keyPins[5];

public:
    Keyboard(byte upPin, byte downPin, byte leftPin, byte rightPin, byte enterPin);

    bool isPressed(Key key);
};

#endif

