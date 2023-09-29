#include <Arduino.h>

#include <TimeService.h>
#include <Logger.h>
#include <HttpAsyncClient.h>

#include "Server.h"

#include "pwd.h"

Logger logger(true, 1500U);
Server server(&logger);
WiFiHandler wifiHandler(&logger, &server, ssid, password);

void setup() 
{
  Serial.begin(115200);  
  
  wifiHandler.setup();
}

void loop() 
{
  wifiHandler.handle();
}
