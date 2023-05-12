#include "acu_driver8266.h"

ACUDrivier::ACUDrivier(IPAddress &id,
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

void ACUDrivier::begin()
{
    Serial.begin(9600);

    WiFi.mode(WIFI_STA);
    WiFi.config(this->ip, this->gateway, this->subnet, this->dns1, this->dns2);
    WiFi.begin(this->ssid, this->pwd);

    // Wait for connection
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
    }

    this->wifiDisconnectHandler = WiFi.onStationModeDisconnected(std::bind(&ACUDrivier::onWifiDisconnect, this, std::placeholders::_1));
    this->wifiConnectedHandler = WiFi.onStationModeConnected(std::bind(&ACUDrivier::onWifiConnected, this, std::placeholders::_1));

    this->isConnected = true;
    MDNS.begin("esp8266");

    this->server.on("/", std::bind(&ACUDrivier::handleRoot, this));
    this->server.on("/ota", std::bind(&ACUDrivier::handleOTA, this));
    this->server.on("/reset", std::bind(&ACUDrivier::handleReset, this));

    this->server.on("/ota", HTTP_OPTIONS, std::bind(&ACUDrivier::handleOptions, this));
    this->server.on("/reset", HTTP_OPTIONS, std::bind(&ACUDrivier::handleOptions, this));

    this->server.onNotFound(std::bind(&ACUDrivier::handleNotFound, this));
    this->server.begin();

    ArduinoOTA.setHostname("ACUDriver");

    ArduinoOTA.onStart([]() {});

    ArduinoOTA.onEnd([]() {});
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {});
    ArduinoOTA.onError([](ota_error_t error) {});
    ArduinoOTA.begin();
    getTime();

    nextRead = millis() + 100;
}

void ACUDrivier::onWifiDisconnect(const WiFiEventStationModeDisconnected &event)
{
    this->isConnected = false;
    WiFi.disconnect();
    WiFi.begin(this->ssid, this->pwd);
}

void ACUDrivier::onWifiConnected(const WiFiEventStationModeConnected &event)
{
    this->isConnected = true;
}

void ACUDrivier::handleNotFound()
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

void ACUDrivier::addCORSHeaders()
{
    this->server.sendHeader("Access-Control-Allow-Origin", "*");
    this->server.sendHeader("Access-Control-Allow-Methods", "DELETE, POST, GET, OPTIONS");
    this->server.sendHeader("Access-Control-Allow-Headers", "Content-Type, Authorization, X-Requested-With");
}

void ACUDrivier::getTime()
{
    this->timeClient.update();
}

void ACUDrivier::handle()
{
    this->server.handleClient();

    this->handleTimeEvents();

    if (this->otaEnabled)
    {
        ArduinoOTA.handle();
    }
}

void ACUDrivier::handleTimeEvents()
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

        if(this->timer % 100 == 0){
            Serial.println("GeT");
            delay(100);
            if(Serial.available())
            {
                String result = Serial.readString();
                this->value = result.toInt();
            }
        }
    }
}

void ACUDrivier::handleOptions()
{
    this->addCORSHeaders();
    this->server.send(200);
}

void ACUDrivier::handleOTA()
{
    this->otaEnabled = true;

    this->addCORSHeaders();
    this->server.send(200, "text/plain", "OTA enabled");
}

void ACUDrivier::handleReset()
{
    this->otaEnabled = false;
    ESP.reset();
}

void ACUDrivier::handleRoot()
{
    String src = "<div>ACU Driver</div>";    

    src += String(this->value);

    this->server.send(200, "text/html", src);
}

DateTime ACUDrivier::getDate()
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