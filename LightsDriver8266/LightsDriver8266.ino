#include <NTPClient.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <ArduinoJson.h>
#include <ArduinoOTA.h>
#include "FS.h"
#include "LightsDriver.h"
#include "pwd.h"

ESP8266WebServer server(80);
IPAddress gateway(192, 168, 100, 1);
IPAddress subnet(255, 255, 255, 0);
IPAddress dns1(192, 168, 100, 1);
IPAddress dns2(8, 8, 8, 8);

WiFiEventHandler wifiDisconnectHandler;
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 3600);

int autoPin = 12;
int autoVal = 0;
int lastAutoVal = 0;
unsigned long timeout = 0;

unsigned long nextRead = 0;
int timer = 0;

IPAddress ip(192, 168, 100, 32);

bool otaEnabled = false;

int leds[] = {5, 4, 14, 13};
int vals[] = {255, 255, 255, 255};
int state[] = {0, 0, 0, 0};

int autoState[] = {0, 0, 0, 1};

String names[] = {"Oswietlenie gorne", "Oswietlenie szafek", "Oswietlenie blat", "Oswietlenie podloga"};

//LightsDriver ld(ip);

void setup(void)
{
    //pinMode(led, OUTPUT);
    for (byte i = 0; i < getNumberOfLeds(); i++)
    {
        analogWrite(leds[i], 0);
    }

    pinMode(autoPin, INPUT);

    Serial.begin(115200);
    WiFi.mode(WIFI_STA);
    WiFi.config(ip, gateway, subnet, dns1, dns2);
    WiFi.begin(ssid, password);
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

    wifiDisconnectHandler = WiFi.onStationModeDisconnected(onWifiDisconnect);

    Serial.println("");
    Serial.print("Connected to ");
    Serial.println(ssid);
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());

    if (MDNS.begin("esp8266"))
    {
        Serial.println("MDNS responder started");
    }

    server.on("/", handleRoot);
    server.on("/led", HTTP_POST, handleLed);
    server.on("/onoff", HTTP_POST, handleOnOff);
    server.on("/brightness", HTTP_POST, handleBrightness);
    server.on("/save", HTTP_POST, handleSave);
    server.on("/ota", handleOTA);
    server.on("/reset", handleReset);
    server.on("/auto", HTTP_POST, handleAuto);
    server.onNotFound(handleNotFound);
    server.begin();
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
                vals[i] = arr[i].as<int>();
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
                autoState[i] = arr[i].as<int>();
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

void loop(void)
{
    server.handleClient();

    handleTimeEvents();

    if (otaEnabled)
    {
        ArduinoOTA.handle();
    }
}

void handleTimeEvents()
{
    if (nextRead < millis())
    {
        timer++;
        if (timer > 36000)
        {
            timer = 0;
        }
        nextRead = millis() + 100;

        if (timer % 5 == 0)
        {
            lastAutoVal = autoVal;
            autoVal = digitalRead(autoPin);
            serveAuto();
        }

        if (timer % 600 == 0)
        {
            getTime();
        }
    }
}

bool isDarkTime()
{
    if ((timeClient.getHours() > 15 || timeClient.getHours() < 7))
    {
        return true;
    }

    return false;
}

void getTime()
{
    timeClient.update();
    Serial.println();
    Serial.print("Current time: ");
    Serial.println(timeClient.getFormattedTime());
}

String generateLedHtml(int n)
{
    String src = "<h1>";
    src += names[n];
    src += "</h1><input class=\"btn\" type=\"button\" value=\"";
    src += (state[n] == 0 ? "on" : "off");
    src += "\" onclick=\"turnOnOff(";
    src += String(n + 1);
    src += ", ";
    src += (state[n] == 0 ? "1" : "0");
    src += ")\"><input class=\"range\" type=\"range\" value=\"";
    src += String(vals[n]);
    src += "\" min=\"0\" max=\"255\" oninput=\"changeBrightness(";
    src += String(n + 1);
    src += ", this.value)\">";

    src += "<input class=\"checkbox\" type=\"checkbox\" oninput=\"changeAuto(";
    src += String(n + 1);
    src += ")\" id=\"c";
    src += String(n + 1);
    src += "\" ";
    src += (autoState[n] == 0 ? "" : "checked");
    src += "><label for=\"c";
    src += String(n + 1);
    src += "\" class=\"label\">Auto</label>";

    return src;
}

byte getNumberOfLeds()
{
    return sizeof(leds) / sizeof(leds[0]);
}

void handleRoot()
{
    String src = htmlsrc1;

    for (int i = 0; i < getNumberOfLeds(); i++)
    {
        src += generateLedHtml(i);
    }

    src += htmlsrc2;

    server.send(200, "text/html", src);
}

void handleNotFound()
{
    String message = "File Not Found\n\n";
    message += "URI: ";
    message += server.uri();
    message += "\nMethod: ";
    message += (server.method() == HTTP_GET) ? "GET" : "POST";
    message += "\nArguments: ";
    message += server.args();
    message += "\n";

    for (uint8_t i = 0; i < server.args(); i++)
    {
        message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
    }

    server.send(404, "text/plain", message);
}

void onWifiDisconnect(const WiFiEventStationModeDisconnected &event)
{
    Serial.println("Disconnected from Wi-Fi, trying to connect...");
    WiFi.disconnect();
    WiFi.begin(ssid, password);
}

void serveAuto()
{
    if (isAutoEnabled() == 0)
    {
        return;
    }

    unsigned long now = millis();
    if (timeout == 0 && autoVal > 0 && isDarkTime())
    {
        timeout = now + 60000; // 60s
        Serial.println("auto started");
        Serial.println(timeout);
        Serial.println(now);
        changeAutoLed(1);
        return;
    }

    if (timeout < now && timeout > 0 && autoVal == 0)
    {
        Serial.println("auto stopped");
        Serial.println(timeout);
        Serial.println(now);
        changeAutoLed(0);
        timeout = 0;
        return;
    }

    if (autoVal > 0 && isDarkTime())
    {
        Serial.println("auto extending");
        timeout = now + 60000;
    }
}

void changeAutoLed(int enabled)
{
    for (int i = 0; i < getNumberOfLeds(); i++)
    {
        if (autoState[i] > 0)
        {
            state[i] = enabled;
            analogWrite(leds[i], state[i] == 0 ? 0 : vals[i]);
        }
    }
}

int isAutoEnabled()
{
    for (int i = 0; i < getNumberOfLeds(); i++)
    {
        if (autoState[i] > 0)
        {
            return 1;
        }
    }

    return 0;
}

void handleReset()
{
    otaEnabled = false;
    ESP.reset();
}

void handleOTA()
{
    otaEnabled = true;

    Serial.println("OTA enabled");

    server.send(200, "text/plain", "OTA enabled");
}

void handleSaveAuto()
{
    File cfg = SPIFFS.open("/autoState.json", "w");
    if (cfg)
    {
        DynamicJsonDocument doc(200);
        JsonArray arr = doc.to<JsonArray>();
        for (int i = 0; i < getNumberOfLeds(); i++)
        {
            arr.add(autoState[i]);
        }
        serializeJson(doc, cfg);
        cfg.close();

        server.send(200);

        return;
    }
    else
    {
        server.send(500, "text/plain", "Cannot open file");
        return;
    }

    server.send(500);
}

void handleSave()
{
    File cfg = SPIFFS.open("/vals.json", "w");
    if (cfg)
    {
        DynamicJsonDocument doc(200);
        JsonArray arr = doc.to<JsonArray>();
        for (int i = 0; i < getNumberOfLeds(); i++)
        {
            arr.add(vals[i]);
        }
        serializeJson(doc, cfg);
        cfg.close();

        server.send(200);

        return;
    }
    else
    {
        server.send(500, "text/plain", "Cannot open file");
        return;
    }

    server.send(500);
}

void handleOnOff()
{
    DynamicJsonDocument doc(200);
    deserializeJson(doc, server.arg("plain"));
    int id = doc["id"];
    int val = doc["value"];

    state[id - 1] = val;
    if (val > 0 && vals[id - 1] == 0)
    {
        vals[id - 1] = 255;
    }
    analogWrite(leds[id - 1], val == 0 ? 0 : vals[id - 1]);

    Serial.print("onoff ");
    Serial.print(id);
    Serial.print(" value: ");
    Serial.println(val);

    server.sendHeader("location", "/");
    server.send(303);
}

void handleBrightness()
{
    DynamicJsonDocument doc(200);
    deserializeJson(doc, server.arg("plain"));
    int id = doc["id"];
    int val = doc["value"];

    vals[id - 1] = val;
    state[id - 1] = val == 0 ? 0 : 1;
    analogWrite(leds[id - 1], vals[id - 1]);

    Serial.print("brightness ");
    Serial.print(id);
    Serial.print(" value: ");
    Serial.println(val);

    server.sendHeader("location", "/");
    server.send(303);
}

void handleAuto()
{
    DynamicJsonDocument doc(200);
    deserializeJson(doc, server.arg("plain"));
    int id = doc["id"];
    int val = doc["value"];

    autoState[id - 1] = val;

    handleSaveAuto();

    server.sendHeader("location", "/");
    server.send(303);
}

void handleLed()
{
    if (!server.hasArg("id") || !server.hasArg("val") || server.arg("id") == NULL || server.arg("val") == NULL)
    {
        server.send(400, "text/plain", "400: Invalid request");
    }

    int id = server.arg("id").toInt();
    int val = server.arg("val").toInt();
    vals[id - 1] = val;
    analogWrite(leds[id - 1], val);

    Serial.print("id: ");
    Serial.println(server.arg("id"));
    Serial.print("value: ");
    Serial.println(server.arg("val"));

    server.sendHeader("location", "/");
    server.send(303);
}