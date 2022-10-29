#include "pwd.h"
#include "ESBDriver.h"

IPAddress ip(192, 168, 100, 60);

ESBDriver driver(ip, ssid, password);

void setup()
{
    driver.begin();
}

void loop()
{
    driver.handle();
}
