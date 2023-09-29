#pragma once

#include <Arduino.h>

class Logger {
    private:
        String log = "";
        bool logToSerial;
        unsigned int maxLog;        

    public:
        Logger(bool logToSerial = true, unsigned int maxLog = 1000) : logToSerial(logToSerial), maxLog(maxLog) {}
        void print(String str);
        void print(const char c[]);
        void println(String str);
        void println(const char c[]);
        String getLog();
};