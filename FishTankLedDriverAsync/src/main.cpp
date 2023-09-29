#include <DNSServer.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <ESPAsyncWebServer.h>

#include <TimeService.h>
#include <Logger.h>

#include "LightDriver.h"

#include "pwd.h"

TimeService timeService;
Logger logger;
LightDriver driver(&logger, &timeService);
WiFiHandler wifiHandler(&logger, &driver, ssid, password);

void setup() {
  Serial.begin(115200);
  
  wifiHandler.setup();

  timeService.begin();
  logger.println("Current time: " + timeService.toString());
}

void loop() {
  wifiHandler.handle();
}