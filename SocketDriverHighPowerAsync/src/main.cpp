#include <Arduino.h>

#include <TimeService.h>
#include <Logger.h>
#include <HttpAsyncClient.h>

#include "Server.h"

#include "pwd.h"

TimeService timeService;
Logger logger;
Server server(&logger);
WiFiHandler wifiHandler(&logger, &server, ssid, password);

void setup() 
{  
  Serial.begin(115200);  
  
  wifiHandler.setup();

  timeService.begin();
  logger.println("Current time: " + timeService.toString());  
}

void loop() 
{
  wifiHandler.handle();
}
