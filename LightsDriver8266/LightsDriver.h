#pragma once

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
    char *ssid;
    char *pwd;
    WiFiEventHandler wifiDisconnectHandler;
    WiFiUDP ntpUDP;
    NTPClient timeClient;
    int autoPin = 12;
    int autoVal = 0;
    int lastAutoVal = 0;
    unsigned long timeout = 0;
    unsigned long nextRead = 0;
    int timer = 0;
    bool otaEnabled = false;
    int *leds;
    int vals[4] = {255, 255, 255, 255};
    int state[4] = {0, 0, 0, 0};
    int autoState[4] = {0, 0, 0, 0};
    String *names;
    String instanceName;

    void handleTimeEvents();
    bool isDarkTime();
    void getTime();
    byte getNumberOfLeds();
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

public:
    LightsDriver(IPAddress &ip, char *ssid, char *pwd, int *leds, String *names, char *instanceName);
    void begin();
    void handle();
};