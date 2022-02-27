#include "LightsDriver.h"

LightsDriver::LightsDriver(IPAddress &ip,
                           const char *ssid,
                           const char *pwd,
                           int leds[],
                           byte ledsCount,
                           String *names,
                           const char *instanceName) : server(80),
                                                       gateway(192, 168, 100, 1),
                                                       subnet(255, 255, 255, 0),
                                                       dns1(192, 168, 100, 1),
                                                       dns2(8, 8, 8, 8),
                                                       timeClient(ntpUDP, "pool.ntp.org", 3600)
{
    this->ip = ip;
    this->ssid = ssid;
    this->pwd = pwd;
    this->leds = leds;
    this->names = names;
    this->instanceName = instanceName;
    this->ledsCount = ledsCount;
}

void LightsDriver::begin()
{
    for (byte i = 0; i < this->getNumberOfLeds(); i++)
    {
        analogWrite(this->leds[i], 0);
    }

    pinMode(this->autoPin, INPUT);

    Serial.begin(115200);

    WiFi.mode(WIFI_STA);
    WiFi.config(this->ip, this->gateway, this->subnet, this->dns1, this->dns2);
    WiFi.begin(this->ssid, this->pwd);
    Serial.println("");
    Serial.println("Lighs Driver started");
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

    this->wifiDisconnectHandler = WiFi.onStationModeDisconnected(std::bind(&LightsDriver::onWifiDisconnect, this, std::placeholders::_1));

    Serial.println("");
    Serial.print("Connected to ");
    Serial.println(ssid);
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());

    Serial.println("led pins: ");
    for (int i = 0; i < this->getNumberOfLeds(); i++)
    {
        Serial.print(this->leds[i]);
        Serial.print(" - ");
        Serial.println(this->names[i]);
    }

    if (MDNS.begin("esp8266"))
    {
        Serial.println("MDNS responder started");
    }

    this->server.on("/", std::bind(&LightsDriver::handleRoot, this));
    this->server.on("/led", HTTP_POST, std::bind(&LightsDriver::handleLed, this));
    this->server.on("/onoff", HTTP_POST, std::bind(&LightsDriver::handleOnOff, this));
    this->server.on("/brightness", HTTP_POST, std::bind(&LightsDriver::handleBrightness, this));
    this->server.on("/save", HTTP_POST, std::bind(&LightsDriver::handleSave, this));
    this->server.on("/ota", std::bind(&LightsDriver::handleOTA, this));
    this->server.on("/reset", std::bind(&LightsDriver::handleReset, this));
    this->server.on("/auto", HTTP_POST, std::bind(&LightsDriver::handleAuto, this));
    this->server.onNotFound(std::bind(&LightsDriver::handleNotFound, this));
    this->server.begin();
    Serial.println("HTTP server started");

    Serial.println("Loading configuration");

    if (SPIFFS.begin())
    {
        File cfg = SPIFFS.open("/vals.json", "r");
        if (cfg)
        {
            Serial.println("Opened file");
            DynamicJsonDocument doc(200);
            deserializeJson(doc, cfg);
            JsonArray arr = doc.as<JsonArray>();
            for (int i = 0; i < getNumberOfLeds(); i++)
            {
                this->vals[i] = arr[i].as<int>();
            }
            cfg.close();

            Serial.println("Configuration loaded");
        }

        cfg = SPIFFS.open("/autoState.json", "r");
        if (cfg)
        {
            Serial.println("Opened file");
            DynamicJsonDocument doc(200);
            deserializeJson(doc, cfg);
            JsonArray arr = doc.as<JsonArray>();
            for (int i = 0; i < getNumberOfLeds(); i++)
            {
                this->autoState[i] = arr[i].as<int>();
            }
            cfg.close();

            Serial.println("Configuration loaded");
        }
    }

    ArduinoOTA.setHostname("LightsDriverKuchnia");

    ArduinoOTA.onStart([]() {
        String type;
        if (ArduinoOTA.getCommand() == U_FLASH)
            type = "sketch";
        else // U_SPIFFS
            type = "filesystem";

        // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
        Serial.println("Start updating " + type);
    });
    ArduinoOTA.onEnd([]() {
        Serial.println("\nEnd");
    });
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
        Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    });
    ArduinoOTA.onError([](ota_error_t error) {
        Serial.printf("Error[%u]: ", error);
        if (error == OTA_AUTH_ERROR)
            Serial.println("Auth Failed");
        else if (error == OTA_BEGIN_ERROR)
            Serial.println("Begin Failed");
        else if (error == OTA_CONNECT_ERROR)
            Serial.println("Connect Failed");
        else if (error == OTA_RECEIVE_ERROR)
            Serial.println("Receive Failed");
        else if (error == OTA_END_ERROR)
            Serial.println("End Failed");
    });
    ArduinoOTA.begin();

    Serial.println("Waiting for communication");
    getTime();

    nextRead = millis() + 100;
}

void LightsDriver::handle()
{
    this->server.handleClient();

    this->handleTimeEvents();

    if (this->otaEnabled)
    {
        ArduinoOTA.handle();
    }
}

void LightsDriver::handleTimeEvents()
{
    if (this->nextRead < millis())
    {
        this->timer++;
        if (this->timer > 36000)
        {
            this->timer = 0;
        }
        this->nextRead = millis() + 100;

        if (this->timer % 5 == 0)
        {
            this->lastAutoVal = this->autoVal;
            this->autoVal = digitalRead(this->autoPin);
            this->serveAuto();
        }

        if (this->timer % 600 == 0)
        {
            this->getTime();
        }
    }
}

bool LightsDriver::isDarkTime()
{
    if ((this->timeClient.getHours() >= 15 || this->timeClient.getHours() <= 7))
    {
        return true;
    }

    return false;
}

void LightsDriver::getTime()
{
    this->timeClient.update();
    Serial.println();
    Serial.print("Current time: ");
    Serial.println(this->timeClient.getFormattedTime());
}

String LightsDriver::generateLedHtml(int n)
{
    String src = "<h1>";
    src += this->names[n];
    src += "</h1><input class=\"btn\" type=\"button\" value=\"";
    src += (this->state[n] == 0 ? "on" : "off");
    src += "\" onclick=\"turnOnOff(";
    src += String(n + 1);
    src += ", ";
    src += (this->state[n] == 0 ? "1" : "0");
    src += ")\"><input class=\"range\" type=\"range\" value=\"";
    src += String(this->vals[n]);
    src += "\" min=\"0\" max=\"255\" oninput=\"changeBrightness(";
    src += String(n + 1);
    src += ", this.value)\">";

    src += "<input class=\"checkbox\" type=\"checkbox\" oninput=\"changeAuto(";
    src += String(n + 1);
    src += ")\" id=\"c";
    src += String(n + 1);
    src += "\" ";
    src += (this->autoState[n] == 0 ? "" : "checked");
    src += "><label for=\"c";
    src += String(n + 1);
    src += "\" class=\"label\">Auto</label>";

    return src;
}

byte LightsDriver::getNumberOfLeds()
{
    return this->ledsCount;
}

void LightsDriver::handleRoot()
{
    String src = htmlsrc1;

    for (int i = 0; i < this->getNumberOfLeds(); i++)
    {
        src += this->generateLedHtml(i);
    }

    src += htmlsrc2;

    this->server.send(200, "text/html", src);
}

void LightsDriver::handleNotFound()
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

    this->server.send(404, "text/plain", message);
}

void LightsDriver::onWifiDisconnect(const WiFiEventStationModeDisconnected &event)
{
    Serial.println("Disconnected from Wi-Fi, trying to connect...");
    WiFi.disconnect();
    WiFi.begin(this->ssid, this->pwd);
}

void LightsDriver::serveAuto()
{
    if (this->isAutoEnabled() == 0)
    {
        return;
    }

    unsigned long now = millis();
    if (this->timeout == 0 && this->autoVal > 0 && this->isDarkTime())
    {
        this->timeout = now + 60000; // 60s
        Serial.println("auto started");
        Serial.println(this->timeout);
        Serial.println(now);
        this->changeAutoLed(1);
        return;
    }

    if (this->timeout < now && this->timeout > 0 && this->autoVal == 0)
    {
        Serial.println("auto stopped");
        Serial.println(this->timeout);
        Serial.println(now);
        this->changeAutoLed(0);
        this->timeout = 0;
        return;
    }

    if (this->autoVal > 0 && this->isDarkTime())
    {
        Serial.println("auto extending");
        this->timeout = now + 60000;
    }
}

void LightsDriver::changeAutoLed(int enabled)
{
    for (int i = 0; i < this->getNumberOfLeds(); i++)
    {
        if (this->autoState[i] > 0)
        {
            this->state[i] = enabled;
            analogWrite(this->leds[i], this->state[i] == 0 ? 0 : this->vals[i]);
        }
    }
}

int LightsDriver::isAutoEnabled()
{
    for (int i = 0; i < this->getNumberOfLeds(); i++)
    {
        if (this->autoState[i] > 0)
        {
            return 1;
        }
    }

    return 0;
}

void LightsDriver::handleReset()
{
    this->otaEnabled = false;
    ESP.reset();
}

void LightsDriver::handleOTA()
{
    this->otaEnabled = true;

    Serial.println("OTA enabled");

    this->server.send(200, "text/plain", "OTA enabled");
}

void LightsDriver::handleSaveAuto()
{
    File cfg = SPIFFS.open("/autoState.json", "w");
    if (cfg)
    {
        DynamicJsonDocument doc(200);
        JsonArray arr = doc.to<JsonArray>();
        for (int i = 0; i < this->getNumberOfLeds(); i++)
        {
            arr.add(this->autoState[i]);
        }
        serializeJson(doc, cfg);
        cfg.close();

        this->server.send(200);

        return;
    }
    else
    {
        this->server.send(500, "text/plain", "Cannot open file");
        return;
    }

    this->server.send(500);
}

void LightsDriver::handleSave()
{
    File cfg = SPIFFS.open("/vals.json", "w");
    if (cfg)
    {
        DynamicJsonDocument doc(200);
        JsonArray arr = doc.to<JsonArray>();
        for (int i = 0; i < this->getNumberOfLeds(); i++)
        {
            arr.add(this->vals[i]);
        }
        serializeJson(doc, cfg);
        cfg.close();

        this->server.send(200);

        return;
    }
    else
    {
        this->server.send(500, "text/plain", "Cannot open file");
        return;
    }

    this->server.send(500);
}

void LightsDriver::handleOnOff()
{
    DynamicJsonDocument doc(200);
    deserializeJson(doc, this->server.arg("plain"));
    int id = doc["id"];
    int val = doc["value"];

    this->state[id - 1] = val;
    if (val > 0 && this->vals[id - 1] == 0)
    {
        this->vals[id - 1] = 255;
    }
    analogWrite(this->leds[id - 1], val == 0 ? 0 : this->vals[id - 1]);

    Serial.print("onoff ");
    Serial.print(id);
    Serial.print(" value: ");
    Serial.println(val);

    this->server.sendHeader("location", "/");
    this->server.send(303);
}

void LightsDriver::handleBrightness()
{
    DynamicJsonDocument doc(200);
    deserializeJson(doc, this->server.arg("plain"));
    int id = doc["id"];
    int val = doc["value"];

    this->vals[id - 1] = val;
    this->state[id - 1] = val == 0 ? 0 : 1;
    analogWrite(this->leds[id - 1], this->vals[id - 1]);

    Serial.print("brightness ");
    Serial.print(id);
    Serial.print(" value: ");
    Serial.println(val);

    this->server.sendHeader("location", "/");
    this->server.send(303);
}

void LightsDriver::handleAuto()
{
    DynamicJsonDocument doc(200);
    deserializeJson(doc, this->server.arg("plain"));
    int id = doc["id"];
    int val = doc["value"];

    this->autoState[id - 1] = val;

    this->handleSaveAuto();

    this->server.sendHeader("location", "/");
    this->server.send(303);
}

void LightsDriver::handleLed()
{
    if (!this->server.hasArg("id") || !this->server.hasArg("val") || this->server.arg("id") == NULL || this->server.arg("val") == NULL)
    {
        this->server.send(400, "text/plain", "400: Invalid request");
    }

    int id = this->server.arg("id").toInt();
    int val = this->server.arg("val").toInt();
    this->vals[id - 1] = val;
    analogWrite(this->leds[id - 1], val);

    Serial.print("id: ");
    Serial.println(this->server.arg("id"));
    Serial.print("value: ");
    Serial.println(this->server.arg("val"));

    this->server.sendHeader("location", "/");
    this->server.send(303);
}