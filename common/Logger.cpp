#include "Logger.h"

void Logger::print(String str)
{
    if(logToSerial)
    {
        Serial.print(str);
    }
    
    str = str;

    if (str.length() > this->maxLog)
    {
        this->log = str.substring(str.length() - this->maxLog);
        return;
    }

    unsigned int newStringLength = this->log.length() + str.length();
    if (newStringLength > this->maxLog)
    {
        this->log = this->log.substring(newStringLength - this->maxLog) + str;
        return;
    }

    this->log += str;
}

void Logger::print(const char c[])
{
    this->print(String(c));
}

void Logger::println(String str)
{
    this->print(str + "\r\n");
}

void Logger::println(const char c[])
{
    this->println(String(c));
}

String Logger::getLog()
{
    String result = "Logs:</br>" + this->log;
    result.replace("\r\n", "</br>");
    return result;
}