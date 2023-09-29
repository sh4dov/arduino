#pragma once

#include <Arduino.h>
#include <WiFiHandler.h>

class Server : public IDriver
{
    private:        
        AsyncWebServer server;
        Logger *logger;
  
        void handleRoot(AsyncWebServerRequest *request);
  
    public:
        Server(Logger *logger);
        void setup();
        void handle(); 
};