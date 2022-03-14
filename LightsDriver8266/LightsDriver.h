#ifndef LIGHTSDRIVER_H
#define LIGHTSDRIVER_H

#include <NTPClient.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <ArduinoJson.h>
#include <ArduinoOTA.h>
#include "FS.h"
#include "htmlSrc.h"

class LightsDriver
{
private:
    IPAddress ip;
    ESP8266WebServer server;
    IPAddress gateway;
    IPAddress subnet;
    IPAddress dns1;
    IPAddress dns2;
    const char *ssid;
    const char *pwd;
    WiFiEventHandler wifiDisconnectHandler;
    WiFiUDP ntpUDP;
    NTPClient timeClient;
    int autoVal[4] = {0, 0, 0, 0};
    unsigned long timeout = 0;
    unsigned long nextRead = 0;
    int timer = 0;
    bool otaEnabled = false;
    int *leds;
    int vals[4] = {255, 255, 255, 255};
    int state[4] = {0, 0, 0, 0};
    int autoState[4] = {0, 0, 0, 0};
    String *names;
    const char *instanceName;
    byte ledsCount;
    int *detectors;
    byte detectorsCount;
    int from = 7;
    int to = 15;

    void handleTimeEvents();
    bool isDarkTime();
    void getTime();
    String generateLedHtml(int n);
    void handleRoot();
    void handleNotFound();
    void onWifiDisconnect(const WiFiEventStationModeDisconnected &event);
    void serveAuto();
    void changeAutoLed(int enabled);
    int isAutoEnabled();
    void handleReset();
    void handleOTA();
    void handleSaveAuto();
    void handleSave();
    void handleOnOff();
    void handleBrightness();
    void handleAuto();
    void handleLed();
    int getMaxAutoVal();
    void handleConf();
    void addCORSHeaders();
    void handleOptions();

public:
    LightsDriver(IPAddress &ip, const char *ssid, const char *pwd, int leds[], byte ledsCount, int detectors[], byte detectorsCount, String *names, const char *instanceName);
    void begin();
    void handle();
};

#endif