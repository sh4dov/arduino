#include <NTPClient.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <ArduinoJson.h>
#include <ArduinoOTA.h>
#include "FS.h"
#include "LightsDriver.h"
#include "pwd.h"

IPAddress ip(192, 168, 100, 33);
int leds[] = {5, 4};
String names[] = {"Plafon", "Glowne"};

LightsDriver ld(ip, ssid, password, leds, names, "LightsDriverKD");

void setup(void)
{
    ld.begin();
}

void loop(void)
{
    ld.handle();
}