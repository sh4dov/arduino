#include <DNSServer.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include "ESPAsyncWebServer.h"
#include "LightDriver.h"
#include "Logger.h"
#include "TimeService.h"

#include "pwd.h"

TimeService timeService;
WiFiUDP ntpUDP;
Logger logger;
LightDriver *driver;

WiFiEventHandler wifiConnectHandler;
WiFiEventHandler wifiDisconnectHandler;

void onWifiDisconnect(const WiFiEventStationModeDisconnected &event)
{
  logger.println("Disconnected");
  driver->setDisconnected();
  WiFi.disconnect();
  WiFi.begin(ssid, password);
}

void onWifiConnected(const WiFiEventStationModeConnected &event)
{
  logger.println("Connected");
  driver->setConnected();
}

void setupWiFi()
{
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while(WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
  }

  wifiDisconnectHandler = WiFi.onStationModeDisconnected(onWifiDisconnect);
  wifiConnectHandler = WiFi.onStationModeConnected(onWifiConnected);

  Serial.println("");
  logger.print("Connected to ");
  logger.println(ssid);
  logger.print("IP address: ");
  logger.println(WiFi.localIP().toString());
}

void setupNtp()
{
  timeService.begin();

  logger.println("Current time: " + timeService.toString());
}

void setup() {
  Serial.begin(115200);
  setupWiFi();
  setupNtp();

  driver = new LightDriver(&logger, &timeService);
  driver->setConnected();
  driver->setup();  
}

void loop() {
  driver->handle();
}