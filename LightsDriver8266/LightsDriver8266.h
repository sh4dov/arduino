#include "pwd.h"

ESP8266WebServer server(80);
IPAddress gateway(192, 168, 100, 1);
IPAddress subnet(255, 255, 255, 0);
IPAddress dns1(192, 168, 100, 1);
IPAddress dns2(8, 8, 8, 8);

WiFiEventHandler wifiDisconnectHandler;
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 3600);

int autoPin = 12;
int autoVal = 0;
int lastAutoVal = 0;
unsigned long timeout = 0;

unsigned long nextRead = 0;
int timer = 0;