#include "pwd.h"
#include "acu_driver8266.h"

IPAddress ip(192, 168, 100, 60);

ACUDrivier driver(ip, ssid, password);

void setup()
{
    driver.begin();
}

void loop()
{
    driver.handle();
}
