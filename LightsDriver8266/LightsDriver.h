#pragma once

#include <NTPClient.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <ArduinoJson.h>
#include <ArduinoOTA.h>
#include "FS.h"
#include "htmlSrc.h"

class LightsDriver
{
private:
    IPAddress ip;

public:
    LightsDriver(IPAddress &ip);
};