#ifndef ESBDRIVER_H
#define ESBDRIVER_H

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

typedef enum
{
    None = 0,
    Params = 2,
    Stats = 4,
    QEY = 8,
    QEM = 16,
    QED = 32
} Operation;

typedef enum
{
    Unknown = 0,
    sbu = 1,
    sub = 2
} WorkType;

class ESBDriver
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
    byte operationQueue = Operation::None;
    WorkType workType = WorkType::Unknown;

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
    void handleParams();
    void handleStats();
    void handleQEY();
    void handleQEM();
    void handleQED();
    void handleQEH();
    void handleQE(const char *qe, byte lenght);
    void handleSBU();
    void handleSUB();
    void handleWorkType();
    void sendHelloCommands();
    void setWorkType();
    DateTime getDate();
    void write(byte *buf, byte lenght);

public:
    ESBDriver(IPAddress &ip, const char *ssid, const char *pwd);
    void begin();
    void handle();
};

#endif