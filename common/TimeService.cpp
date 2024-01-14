#include "TimeService.h"

void TimeService::begin()
{
    configTzTime(MYTZ, "pool.ntp.org");
    this->_now = time(nullptr);
    if(!this->isCorrect())
    {
        this->update();
    }
}

tm* TimeService::now()
{
    this->update();
    struct tm* tm;
    tm = localtime(&this->_now);
    tm->tm_year += 1900;
    return tm;    
}

String TimeService::toString()
{
    this->update();
    struct tm* tm;
    tm = this->now();
    return toString(tm);
}

String TimeService::toString(tm* tm)
{
    char buf[72];            
    sprintf(buf, "%02d.%02d.%04d %02d:%02d:%02d",
        tm->tm_mday,
        tm->tm_mon,
        tm->tm_year,
        tm->tm_hour,
        tm->tm_min,
        tm->tm_sec);
    return String(buf);
}

void TimeService::update()
{
    this->_now = time(nullptr);
}

bool TimeService::isCorrect()
{
    return this->_now > SECS_YR_2000;
}