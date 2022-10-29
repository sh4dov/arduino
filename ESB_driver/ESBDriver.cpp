#include "ESBDriver.h"

ESBDriver::ESBDriver(IPAddress &id,
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

void ESBDriver::begin()
{
    Serial.begin(2400);

    WiFi.mode(WIFI_STA);
    WiFi.config(this->ip, this->gateway, this->subnet, this->dns1, this->dns2);
    WiFi.begin(this->ssid, this->pwd);

    // Wait for connection
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
    }

    this->wifiDisconnectHandler = WiFi.onStationModeDisconnected(std::bind(&ESBDriver::onWifiDisconnect, this, std::placeholders::_1));
    this->wifiConnectedHandler = WiFi.onStationModeConnected(std::bind(&ESBDriver::onWifiConnected, this, std::placeholders::_1));

    this->isConnected = true;
    MDNS.begin("esp8266");

    this->server.on("/", std::bind(&ESBDriver::handleRoot, this));
    this->server.on("/ota", std::bind(&ESBDriver::handleOTA, this));
    this->server.on("/reset", std::bind(&ESBDriver::handleReset, this));
    this->server.on("/params", std::bind(&ESBDriver::handleParams, this));
    this->server.on("/stats", std::bind(&ESBDriver::handleStats, this));

    this->server.on("/ota", HTTP_OPTIONS, std::bind(&ESBDriver::handleOptions, this));
    this->server.on("/reset", HTTP_OPTIONS, std::bind(&ESBDriver::handleOptions, this));
    this->server.on("/params", HTTP_OPTIONS, std::bind(&ESBDriver::handleOptions, this));
    this->server.on("/stats", HTTP_OPTIONS, std::bind(&ESBDriver::handleOptions, this));

    this->server.onNotFound(std::bind(&ESBDriver::handleNotFound, this));
    this->server.begin();

    ArduinoOTA.setHostname("ESBDriver");

    ArduinoOTA.onStart([]() {});

    ArduinoOTA.onEnd([]() {});
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {});
    ArduinoOTA.onError([](ota_error_t error) {});
    ArduinoOTA.begin();
    getTime();

    nextRead = millis() + 100;
}

void ESBDriver::onWifiDisconnect(const WiFiEventStationModeDisconnected &event)
{
    this->isConnected = false;
    WiFi.disconnect();
    WiFi.begin(this->ssid, this->pwd);
}

void ESBDriver::onWifiConnected(const WiFiEventStationModeConnected &event)
{
    this->isConnected = true;
}

void ESBDriver::handleNotFound()
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

void ESBDriver::addCORSHeaders()
{
    this->server.sendHeader("Access-Control-Allow-Origin", "*");
    this->server.sendHeader("Access-Control-Allow-Methods", "DELETE, POST, GET, OPTIONS");
    this->server.sendHeader("Access-Control-Allow-Headers", "Content-Type, Authorization, X-Requested-With");
}

void ESBDriver::getTime()
{
    this->timeClient.update();
}

void ESBDriver::handle()
{
    this->server.handleClient();

    this->handleTimeEvents();

    if (this->otaEnabled)
    {
        ArduinoOTA.handle();
    }
}

void ESBDriver::handleTimeEvents()
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
        }
    }
}

void ESBDriver::handleOptions()
{
    this->addCORSHeaders();
    this->server.send(200);
}

void ESBDriver::handleOTA()
{
    this->otaEnabled = true;

    this->addCORSHeaders();
    this->server.send(200, "text/plain", "OTA enabled");
}

void ESBDriver::handleReset()
{
    this->otaEnabled = false;
    ESP.reset();
}

void ESBDriver::handleRoot()
{
    String src = "<div>ESB Driver</div>";
    src += "<div><a href=\"/params\">params</a></div>";
    src += "<div><a href=\"/stats\">stats</a></div>";

    this->server.send(200, "text/html", src);
}

void ESBDriver::handleParams()
{
    byte qpigs[8] = {0x51, 0x50, 0x49, 0x47, 0x53, 0xB7, 0xA9, 0x0D};

    this->sendHelloCommands();
    this->write(qpigs, sizeof(qpigs));
    String result = Serial.readString();
    result.remove(0, 1);

    this->server.send(200, "text/plain", result);
}

void ESBDriver::handleStats()
{
    this->sendHelloCommands();

    CRC16 crc;
    String query = "QEY";
    DateTime date = this->getDate();
    query += date.year;

    for (byte i = 0; i < query.length(); i++)
    {
        Serial.write(query[i]);
        crc.add(query[i]);
    }

    uint16_t c = crc.getCRC();
    Serial.write((byte)(c & 0xFF));
    Serial.write((byte)((c >> 8) & 0xFF));
    Serial.write(0x0D);

    String yearConsumption = Serial.readString();
    yearConsumption.remove(0, 1);

    crc.reset();

    query = "QEM";
    query += date.year;
    query += date.month > 9 ? String(date.month) : ("0" + date.month);
    for (byte i = 0; i < query.length(); i++)
    {
        Serial.write(query[i]);
        crc.add(query[i]);
    }

    c = crc.getCRC();
    Serial.write((byte)(c & 0xFF));
    Serial.write((byte)((c >> 8) & 0xFF));
    Serial.write(0x0D);

    String monthConsumption = Serial.readString();
    monthConsumption.remove(0, 1);

    crc.reset();

    query = "QED";
    query += date.year;
    query += date.month > 9 ? String(date.month) : ("0" + date.month);
    query += date.day > 9 ? String(date.day) : ("0" + date.day);
    for (byte i = 0; i < query.length(); i++)
    {
        Serial.write(query[i]);
        crc.add(query[i]);
    }

    c = crc.getCRC();
    Serial.write((byte)(c & 0xFF));
    Serial.write((byte)((c >> 8) & 0xFF));
    Serial.write(0x0D);

    String dayConsumption = Serial.readString();
    dayConsumption.remove(0, 1);

    this->server.send(200, "text/plain", yearConsumption + "." + monthConsumption + "." + dayConsumption);
}

void ESBDriver::sendHelloCommands()
{
    byte qpi[6] = {0x51, 0x50, 0x49, 0xBE, 0xAC, 0x0D};
    byte qmn[6] = {0x51, 0x4D, 0x4E, 0xBB, 0x64, 0x0D};
    byte qid[6] = {0x51, 0x49, 0x44, 0xD6, 0xEA, 0x0D};

    this->write(qpi, sizeof(qpi));
    Serial.readString();
    this->write(qmn, sizeof(qmn));
    Serial.readString();
    this->write(qid, sizeof(qid));
    Serial.readString();
}

void ESBDriver::write(byte *buf, byte lenght)
{
    for (byte i = 0; i < lenght; i++)
    {
        Serial.write(buf[i]);
    }
}

DateTime ESBDriver::getDate()
{
    unsigned long src = this->timeClient.getEpochTime();
    DateTime date;

    // Number of days in month
    // in normal year
    int daysOfMonth[] = {31, 28, 31, 30, 31, 30,
                         31, 31, 30, 31, 30, 31};

    long int currYear, daysTillNow, extraTime, extraDays,
        index, day, month, hours, minutes, seconds,
        flag = 0;

    // Calculate total days unix time T
    daysTillNow = src / (24 * 60 * 60);
    extraTime = src % (24 * 60 * 60);
    currYear = 1970;

    // Calculating current year
    while (true)
    {
        if (currYear % 400 == 0 || (currYear % 4 == 0 && currYear % 100 != 0))
        {
            if (daysTillNow < 366)
            {
                break;
            }
            daysTillNow -= 366;
        }
        else
        {
            if (daysTillNow < 365)
            {
                break;
            }
            daysTillNow -= 365;
        }
        currYear += 1;
    }
    // Updating extradays because it
    // will give days till previous day
    // and we have include current day
    extraDays = daysTillNow + 1;

    if (currYear % 400 == 0 || (currYear % 4 == 0 && currYear % 100 != 0))
        flag = 1;

    // Calculating MONTH and DAY
    month = 0, index = 0;
    if (flag == 1)
    {
        while (true)
        {

            if (index == 1)
            {
                if (extraDays - 29 < 0)
                    break;
                month += 1;
                extraDays -= 29;
            }
            else
            {
                if (extraDays - daysOfMonth[index] < 0)
                {
                    break;
                }
                month += 1;
                extraDays -= daysOfMonth[index];
            }
            index += 1;
        }
    }
    else
    {
        while (true)
        {

            if (extraDays - daysOfMonth[index] < 0)
            {
                break;
            }
            month += 1;
            extraDays -= daysOfMonth[index];
            index += 1;
        }
    }

    // Current Month
    if (extraDays > 0)
    {
        month += 1;
        day = extraDays;
    }
    else
    {
        if (month == 2 && flag == 1)
            day = 29;
        else
        {
            day = daysOfMonth[month - 1];
        }
    }

    // Calculating HH:MM:YYYY
    hours = extraTime / 3600;
    minutes = (extraTime % 3600) / 60;
    seconds = (extraTime % 3600) % 60;

    date.year = currYear;
    date.month = month;
    date.day = day;
    date.hour = hours;
    date.minute = minutes;
    date.second = seconds;

    return date;
}