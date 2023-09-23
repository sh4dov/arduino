#pragma once

#include <Arduino.h>
#include <AsyncJson.h>
#include <ArduinoJson.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>

#include "LedHandler.h"
#include "Logger.h"
#include "TimeService.h"
#include "htmlSrc.h"

class LightDriver {
    private:
        LedHandler ledHandler;
        AsyncWebServer server;
        bool isConnected = false;        
        TimeService *timeService;
        TickTwo timer;
        Logger *logger;

        void handleNotFound(AsyncWebServerRequest *request);
        void handleLog(AsyncWebServerRequest *request);
        void handleOnOff(AsyncWebServerRequest *request, JsonVariant &json);
        void handleChangeBrightness(AsyncWebServerRequest *request, JsonVariant &json);
        void handleRoot(AsyncWebServerRequest *request);
        void sendResponse(AsyncWebServerRequest *request, String msg);
        void handleTimedEvents();
        String generateLedHtml();

    public:
        LightDriver(Logger *logger, TimeService *timeService);
        void setup();
        void handle(); 
        void setDisconnected();
        void setConnected();
};