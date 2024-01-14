#pragma once

#include <Arduino.h>
#include <ESPAsyncWebServer.h>

#include <Logger.h>

#define LED_PIN 2

class IDriver
{
    public:
        virtual void setDisconnected() {}
        virtual void setConnected() {}
        virtual void setup() {}
        virtual void handle() {}
};

class WiFiHandler
{
    private:
        Logger *logger;
        IDriver *driver;
        IPAddress *ip;
        IPAddress *gateway;
        IPAddress subnet;
        String ssid;
        String password;
        WiFiEventHandler wifiDisconnectHandler;
        WiFiEventHandler wifiConnectedHandler;

        void onWifiDisconnect(const WiFiEventStationModeDisconnected &event);
        void onWifiConnected(const WiFiEventStationModeConnected &event);

    public:
        WiFiHandler(Logger *logger, IDriver *driver, String ssid, String password);
        WiFiHandler(Logger *logger, IDriver *driver, String ssid, String password, IPAddress *ip, IPAddress *gateway);
        void setup();
        void handle();
};