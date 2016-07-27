#ifndef _ISAVEABLE_H
#define _ISAVEABLE_H

#if defined(ARDUINO) && ARDUINO >= 100
#include "arduino.h"
#else
#include "WProgram.h"
#endif

class ISaveable {
public:
    virtual byte getDataLength() = 0;
};

#endif