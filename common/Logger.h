#pragma once

#include <Arduino.h>

class Logger {
    private:
        String log = "";
        unsigned int maxLog = 1000;

    public:
        void print(String str);
        void print(const char c[]);
        void println(String str);
        void println(const char c[]);
        String getLog();
};