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

IPAddress ip(192, 168, 100, 34);
int leds[] = {5, 4};
int detectors[] = {12, 13};
String names[] = {"Plafon", "Glowne"};
const char *instance = "LightsDriverKG";

LightsDriver ld(ip, ssid, password, leds, sizeof(leds) / sizeof(leds[0]), detectors, sizeof(detectors) / sizeof(detectors[0]), names, instance);

void setup(void)
{
    ld.begin();
}

void loop(void)
{
    ld.handle();
}