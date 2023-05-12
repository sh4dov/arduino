#ifndef ACUDRIVER_H
#define ACUDRIVER_H

#include <NTPClient.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <ArduinoJson.h>
#include <ArduinoOTA.h>
#include <CRC16.h>
#include "FS.h"

struct DateTime
{
    int year;
    byte month;
    byte day;
    byte hour;
    byte minute;
    byte second;
};

class ACUDrivier
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
    WiFiEventHandler wifiConnectedHandler;
    WiFiUDP ntpUDP;
    NTPClient timeClient;
    bool isConnected = false;
    unsigned long nextRead = 0;
    int timer = 0;
    bool otaEnabled = false;
    uint value = 0;
    
    void onWifiDisconnect(const WiFiEventStationModeDisconnected &event);
    void onWifiConnected(const WiFiEventStationModeConnected &event);
    void handleNotFound();
    void addCORSHeaders();
    void getTime();
    void handleTimeEvents();
    void handleOptions();
    void handleOTA();
    void handleReset();
    void handleRoot();
    DateTime getDate();

public:
    ACUDrivier(IPAddress &ip, const char *ssid, const char *pwd);
    void begin();
    void handle();
};

#endif