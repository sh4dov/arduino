#include "SocketDriver.h"

SocketDriver::SocketDriver(IPAddress &id,
                           const char *ssid,
                           const char *pwd) : server(80),
                                              gateway(192, 168, 100, 1),
                                              subnet(255, 255, 255, 0),
                                              dns1(192, 168, 100, 1),
                                              dns2(8, 8, 8, 8),
                                              timeClient(ntpUDP, "pool.ntp.org", 3600)
{
    this->ip = ip;
    this->ssid = ssid;
    this->pwd = pwd;
}

void SocketDriver::begin()
{
    Serial.begin(9600);

    Serial.println("SocketDriver");

    WiFi.mode(WIFI_STA);
    WiFi.config(this->ip, this->gateway, this->subnet, this->dns1, this->dns2);
    WiFi.begin(this->ssid, this->pwd);

    // Wait for connection
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
    }

    this->wifiDisconnectHandler = WiFi.onStationModeDisconnected(std::bind(&SocketDriver::onWifiDisconnect, this, std::placeholders::_1));
    this->wifiConnectedHandler = WiFi.onStationModeConnected(std::bind(&SocketDriver::onWifiConnected, this, std::placeholders::_1));

    this->isConnected = true;
    MDNS.begin("esp8266");

    this->server.on("/", std::bind(&SocketDriver::handleRoot, this));
    this->server.on("/ota", std::bind(&SocketDriver::handleOTA, this));
    this->server.on("/reset", std::bind(&SocketDriver::handleReset, this));
    this->server.on("/off", std::bind(&SocketDriver::handleOff, this));
    this->server.on("/on", std::bind(&SocketDriver::handleOn, this));

    this->server.on("/ota", HTTP_OPTIONS, std::bind(&SocketDriver::handleOptions, this));
    this->server.on("/reset", HTTP_OPTIONS, std::bind(&SocketDriver::handleOptions, this));
    this->server.on("/off", HTTP_OPTIONS, std::bind(&SocketDriver::handleOptions, this));
    this->server.on("/on", HTTP_OPTIONS, std::bind(&SocketDriver::handleOptions, this));

    this->server.onNotFound(std::bind(&SocketDriver::handleNotFound, this));
    this->server.begin();

    ArduinoOTA.setHostname("SocketDriver");

    ArduinoOTA.onStart([]() {});

    ArduinoOTA.onEnd([]() {});
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {});
    ArduinoOTA.onError([](ota_error_t error) {});
    ArduinoOTA.begin();
    getTime();
    handlePvVoltage(true);

    nextRead = millis() + 100;
}

void SocketDriver::onWifiDisconnect(const WiFiEventStationModeDisconnected &event)
{
    this->isConnected = false;
    WiFi.disconnect();
    WiFi.begin(this->ssid, this->pwd);
}

void SocketDriver::onWifiConnected(const WiFiEventStationModeConnected &event)
{
    this->isConnected = true;
}

void SocketDriver::handleNotFound()
{
    String message = "File Not Found\n\n";
    message += "URI: ";
    message += this->server.uri();
    message += "\nMethod: ";
    message += (this->server.method() == HTTP_GET) ? "GET" : "POST";
    message += "\nArguments: ";
    message += this->server.args();
    message += "\n";

    for (uint8_t i = 0; i < this->server.args(); i++)
    {
        message += " " + this->server.argName(i) + ": " + this->server.arg(i) + "\n";
    }

    this->addCORSHeaders();
    this->server.send(404, "text/plain", message);
}

void SocketDriver::addCORSHeaders()
{
    this->server.sendHeader("Access-Control-Allow-Origin", "*");
    this->server.sendHeader("Access-Control-Allow-Methods", "DELETE, POST, GET, OPTIONS");
    this->server.sendHeader("Access-Control-Allow-Headers", "Content-Type, Authorization, X-Requested-With");
}

void SocketDriver::getTime()
{
    this->timeClient.update();
}

void SocketDriver::handle()
{
    this->server.handleClient();

    this->handleTimeEvents();

    if (this->otaEnabled)
    {
        ArduinoOTA.handle();
    }
}

void SocketDriver::handleTimeEvents()
{
    if (this->nextRead < millis())
    {
        this->timer++;
        if (this->timer > 36000)
        {
            this->timer = 0;
        }
        this->nextRead = millis() + 100;

        if (this->timer % 600 == 0)
        {
            this->getTime();
            this->handlePvVoltage(false);
        }

        // 10min
        if (this->timer % 6000 == 0)
        {
            this->handlePvVoltage(true);
        }
    }
}

void SocketDriver::handleOptions()
{
    this->addCORSHeaders();
    this->server.send(200);
}

void SocketDriver::handleOTA()
{
    this->otaEnabled = true;

    this->addCORSHeaders();
    this->server.send(200, "text/plain", "OTA enabled");
}

void SocketDriver::handleReset()
{
    this->otaEnabled = false;
    ESP.reset();
}

void SocketDriver::handleRoot()
{
    String src = "<div>Socket Driver</div>";

    this->server.send(200, "text/html", src);
}

void SocketDriver::handleOff()
{
    analogWrite(5, 0);
    this->addCORSHeaders();
    this->server.send(200, "text/plain", "off");
}

void SocketDriver::handleOn()
{
    analogWrite(5, 255);
    this->addCORSHeaders();
    this->server.send(200, "text/plain", "on");
}

void SocketDriver::handlePvVoltage(bool checkOn)
{
    Serial.println("[HTTP] begin...");
    if (this->http.begin(this->client, "http://192.168.100.49/params"))
    {
        Serial.print("[HTTP] GET...\n");
        // start connection and send HTTP header
        int httpCode = http.GET();

        // httpCode will be negative on error
        if (httpCode > 0)
        {
            // HTTP header has been send and Server response header has been handled
            Serial.printf("[HTTP] GET... code: %d\n", httpCode);

            // file found at server
            if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY)
            {
                String payload = http.getString();
                Serial.println(payload);

                int pvVoltage = payload.substring(64, 67).toInt();
                int pvPower = payload.substring(97, 102).toInt();
                Serial.println(pvVoltage);
                Serial.println(pvPower);

                if (checkOn && (pvVoltage >= 260 || (pvVoltage > 120 && pvPower > 10)))
                {
                    analogWrite(5, 255);
                }
                else if (!checkOn && pvVoltage < 260 && pvPower < 10)
                {
                    analogWrite(5, 0);
                }
            }
        }
        else
        {
            Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
        }

        http.end();
    }
    else
    {
        Serial.printf("[HTTP} Unable to connect\n");
    }
}