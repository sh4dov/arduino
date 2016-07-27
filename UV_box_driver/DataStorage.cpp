// 
// 
// 

#include "DataStorage.h"

byte DataStorage::loadByte(ISaveable* saveable){
    int index = getIndex(saveable);
    if (index < 0){
        return 0;
    }  
    
    return EEPROM.read(index);
}

unsigned long DataStorage::loadLong(ISaveable* saveable){
    int index = getIndex(saveable);
    if (index < 0){
        return 0;
    }

    Serial.print("loading for: ");
    Serial.println((int)saveable, HEX);

    unsigned long data = 0;
    for (byte i = 0; i < 4; i++)
    {
        data = data << 8;
        uint8_t val = EEPROM.read(index + i);
        Serial.print(val);
        data += val;
    }

    Serial.println();
    Serial.println(data);
    return data;
}

bool DataStorage::loadBool(ISaveable* saveable){
    return loadByte(saveable);
}

void DataStorage::save(ISaveable* saveable, byte data){
    int index = getIndex(saveable);
    if (index < 0){
        return;
    }

    EEPROM.write(index, data);
}

void DataStorage::save(ISaveable* saveable, unsigned long data){
    int index = getIndex(saveable);
    if (index < 0){
        return;
    }

    Serial.print("Saving for: ");
    Serial.println((int)saveable, HEX);
    Serial.println(data);

    byte shift = 3 * 8;
    for (byte i = 0; i < 4; i++){
        uint8_t val = data >> shift;
        Serial.print(val, HEX);
        Serial.print(" ");
        EEPROM.write(index + i, val);
        shift -= 8;
    }
    Serial.println();
}

void DataStorage::save(ISaveable* saveable, bool data){
    save(saveable, data ? (byte)1 : (byte)0);
}

DataStorage::DataStorage(byte capacity){
    this->capacity = capacity;
    dataInfo = new DataInfo[capacity];
}

void DataStorage::registerSaveable(ISaveable* saveable){
    if (taken >= capacity){
        return;
    }

    dataInfo[taken].saveable = saveable;
    dataInfo[taken].index = nextIndex;
    nextIndex += saveable->getDataLength();

    Serial.print("Registering saveable: ");
    Serial.println((int)(dataInfo[taken].saveable), HEX);
    Serial.print("Index: ");
    Serial.println(dataInfo[taken].index);

    taken++;
}

int DataStorage::getIndex(ISaveable* saveable){
    Serial.print("Searching index for: ");
    Serial.println((int)saveable, HEX);
    for (byte i = 0; i < taken; i++){
        if (dataInfo[i].saveable == saveable){
            Serial.print("Found index at: ");
            Serial.println(dataInfo[i].index);
            return dataInfo[i].index;
        }
    }

    Serial.println("ERROR index not found");
    return -1;
}
