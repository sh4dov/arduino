#include "pwd.h"
#include "SocketDriver.h"

IPAddress ip(192, 168, 100, 60);

SocketDriver driver(ip, ssid, password);

void setup()
{
    driver.begin();
}

void loop()
{
    driver.handle();
}
