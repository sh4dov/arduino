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

IPAddress ip(192, 168, 100, 35);
int leds[] = {5};
String names[] = {"FishTank"};
const char *instance = "LightsDriverFT";

LightsDriver ld(ip, ssid, password, leds, sizeof(leds) / sizeof(leds[0]), names, instance);

void setup() {
  ld.begin();
}

void loop() {
  ld.handle();
}
