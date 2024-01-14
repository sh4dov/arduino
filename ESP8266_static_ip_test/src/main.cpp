#include <DNSServer.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <ESPAsyncWebServer.h>

#include <TimeService.h>
#include <Logger.h>
#include <WifiHandler.h>

#include "pwd.h"

class Driver : public IDriver 
{
    private:
    AsyncWebServer server;

    void sendResponse(AsyncWebServerRequest *request, String msg)
    {
        request->send(200, "text/html", msg);
    }

    void handleRoot(AsyncWebServerRequest *request)
    {
        this->sendResponse(request, "OK");
    }

    public:
    Driver(): server(80) 
    {
    }

    void setup()
    {
        this->server.on("/", std::bind(&Driver::handleRoot, this, std::placeholders::_1));
        this->server.onNotFound(std::bind(&Driver::handleRoot, this, std::placeholders::_1));
        this->server.begin();
    }       
};

TimeService timeService;
Logger logger;
Driver driver;
IPAddress ip(192, 168, 1, 2);
IPAddress gateway(192, 168, 100, 1);
WiFiHandler wifiHandler(&logger, &driver, ssid, password);
bool isTimeSetup = false;

void setup() {
  Serial.begin(115200);
  
  wifiHandler.setup();

  timeService.begin();
  logger.println("Current time: " + timeService.toString());
}

void loop() {
  wifiHandler.handle();
  if(!isTimeSetup && !timeService.isCorrect())
  {
    timeService.update();    
  }
  else if(!isTimeSetup && timeService.isCorrect())
  {
    isTimeSetup = true;
    logger.println("Current time: " + timeService.toString());
  }
}