#pragma once

#include <Arduino.h>
#include <ESPAsyncWebServer.h>

#include <Logger.h>

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
        String ssid;
        String password;
        WiFiEventHandler wifiDisconnectHandler;
        WiFiEventHandler wifiConnectedHandler;

        void onWifiDisconnect(const WiFiEventStationModeDisconnected &event);
        void onWifiConnected(const WiFiEventStationModeConnected &event);

    public:
        WiFiHandler(Logger *logger, IDriver *driver, String ssid, String password);
        void setup();
        void handle();
};