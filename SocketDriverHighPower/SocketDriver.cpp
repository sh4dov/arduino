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

    Serial.println("HighPowerSocketDriver");

    WiFi.mode(WIFI_STA);
    WiFi.config(this->ip, this->gateway, this->subnet, this->dns1, this->dns2);
    WiFi.begin(this->ssid, this->pwd);

    int c = 0;
    uint8_t s;
    // Wait for connection
    while (WiFi.status() != WL_CONNECTED)
    {
        c++;

        if (c > 20)
        {
            Serial.println("");
            s = WiFi.status();

            switch (s)
            {
            case WL_NO_SHIELD:
                Serial.println("shield status");
                break;

            case WL_IDLE_STATUS:
                Serial.println("idle status");
                break;

            case WL_NO_SSID_AVAIL:
                Serial.println("no ssid available");
                break;

            case WL_SCAN_COMPLETED:
                Serial.println("scan completed");
                break;

            case WL_CONNECT_FAILED:
                Serial.println("connect failed");
                break;

            case WL_CONNECTION_LOST:
                Serial.println("connection lost");
                break;

            case WL_DISCONNECTED:
                Serial.println("disconnected");
                break;

            default:
                Serial.print("unknown status ");
                Serial.println(s);
            }

            c = 0;
        }
        delay(500);
        Serial.print(".");
    }

    this->wifiDisconnectHandler = WiFi.onStationModeDisconnected(std::bind(&SocketDriver::onWifiDisconnect, this, std::placeholders::_1));
    this->wifiConnectedHandler = WiFi.onStationModeConnected(std::bind(&SocketDriver::onWifiConnected, this, std::placeholders::_1));

    Serial.println("");
    Serial.print("Connected to ");
    Serial.println(ssid);
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());

    this->isConnected = true;
    MDNS.begin("HighPowerSocketDriver");

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

        // 1min
        if (this->timer % 600 == 0 && !starting)
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
    String src = "<div>High power Socket Driver is ";
    src += (this->isOn ? "on" : "off");
    src += "</div>";
    src += "<div>Status: ";
    src += this->status;
    src += "</div>";

    this->addCORSHeaders();
    this->server.send(200, "text/html", src);
}

void SocketDriver::handleOff()
{
    analogWrite(5, 0);
    this->isOn = false;
    this->addCORSHeaders();
    this->server.send(200, "text/plain", "off");
}

void SocketDriver::handleOn()
{
    analogWrite(5, 255);
    this->isOn = true;
    this->addCORSHeaders();
    this->server.send(200, "text/plain", "on");
}

void SocketDriver::handlePvVoltage(bool checkOn)
{
    Serial.println("[HTTP] begin...");
    if (this->http.begin(this->client, "http://192.168.100.30/api/paramsCache"))
    {
        Serial.print("[HTTP] GET...\n");
        // start connection and send HTTP header
        this->http.setTimeout(30000);
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
                long pvPower = payload.substring(97, 102).toInt();
                long activePower = payload.substring(27, 31).toInt();
                Serial.println(pvVoltage);
                Serial.println(pvPower);
                Serial.println(activePower);

                if (checkOn && (pvVoltage >= 260))
                {
                    this->isOn = true;
                    this->status = "Ok";
                }
                else if (!checkOn && pvPower < (activePower - 100UL))
                {
                    this->isOn = false;
                    this->status = "low PV input";
                }
            }
        }
        else
        {
            Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
            this->isOn = false;
            this->status = http.errorToString(httpCode);
        }

        http.end();
    }
    else
    {
        Serial.printf("[HTTP} Unable to connect\n");
        this->isOn = false;
        this->status = "Unable to connect";
    }

    analogWrite(5, this->isOn ? 255 : 0);
}