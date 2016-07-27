// IDataStorage.h

#include "ISaveable.h"

#include <EEPROM.h>

#ifndef _DATASTORAGE_h
#define _DATASTORAGE_h

#if defined(ARDUINO) && ARDUINO >= 100
    #include "arduino.h"
#else
    #include "WProgram.h"
#endif


class DataStorage{
private:
    struct DataInfo{
        ISaveable *saveable;
        int index;
    };

    byte capacity;
    byte taken = 0;
    int nextIndex = 0;
    DataInfo* dataInfo;

    int getIndex(ISaveable* saveable);

public:
    DataStorage(byte capacity);

    byte loadByte(ISaveable* saveable);
    bool loadBool(ISaveable* saveable);
    unsigned long loadLong(ISaveable* saveable);
    void save(ISaveable* saveable, byte data);
    void save(ISaveable* saveable, bool data);
    void save(ISaveable* saveable, unsigned long data);
    void registerSaveable(ISaveable* saveable);
};


#endif

