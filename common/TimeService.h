#pragma once

#include <TZ.h>

#include <ESP8266WiFi.h>
#include <time.h>     

#define MYTZ TZ_Europe_Warsaw

#define SECS_YR_2000  (946684800UL) // the time at the start of y2k

class TimeService {
    private:
        time_t _now;        

    public:
        void update();
        void begin(); 
        tm* now();   
        bool isCorrect();
        String toString();
        String toString(tm* tm);
};