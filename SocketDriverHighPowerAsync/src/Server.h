#pragma once

#include <Arduino.h>
#include <TickTwo.h>

#include <WiFiHandler.h>
#include <HttpAsyncClient.h>
#include <LoggerResponseHandler.h>

struct pvInfo
{
    int voltage;
    long unsigned power;
    long unsigned activePower;
    String error;
};

class ParamsResponseHandler : public LoggerResponseHandler
{
    private:
        pvInfo info;        

    public:
        ParamsResponseHandler(Logger *logger) : LoggerResponseHandler(logger) {}
        void onData(String data);
        void onError(String error);
        pvInfo getInfo() { return info; }
};

class Server : public IDriver
{
    private:        
        AsyncWebServer server;
        TickTwo checkParamsTimer;
        TickTwo checkPinTimer;
        HttpAsyncClient client;
        Logger *logger;
        ParamsResponseHandler handler;
        pvInfo info;
        bool isOn = false;
        int pin = 5;

        void handleCheckParamsEvent();
        void handlePinEvent();
        void handleRoot(AsyncWebServerRequest *request);
        void handleLog(AsyncWebServerRequest *request);        
        void handleOn(AsyncWebServerRequest *request);        
        void handleOff(AsyncWebServerRequest *request);        
        void turnOn();
        void turnOff();

    public:
        Server(Logger *logger);
        void setup();
        void handle(); 
        void setDisconnected();
        void setConnected();
};