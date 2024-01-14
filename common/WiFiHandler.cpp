#include "WiFiHandler.h"

WiFiHandler::WiFiHandler(Logger *logger, IDriver *driver, String ssid, String password):
                                                                                        subnet(255, 255, 0, 0)
{
    this->logger = logger;
    this->driver = driver;
    this->ssid = ssid;
    this->password = password;
}

WiFiHandler::WiFiHandler(Logger *logger, IDriver *driver, String ssid, String password, IPAddress *ip, IPAddress *gateway):                                                                                                 
                                                                                                WiFiHandler(logger, driver, ssid, password)                                                                                                
{
    this->ip = ip;
    this->gateway = gateway;
}

void WiFiHandler::setup()
{
    WiFi.mode(WIFI_STA);

    if(this->ip != nullptr && this->gateway != nullptr)
    {
        if(!WiFi.config(*this->ip, *this->gateway, this->subnet))
        {
            logger->println("Cannot set static ip");
        }
    }

    WiFi.begin(ssid, password);

    pinMode(LED_PIN, OUTPUT);
    bool isOn = true;
    while(WiFi.status() != WL_CONNECTED)
    {
        logger->print(".");
        digitalWrite(LED_PIN, isOn ? HIGH : LOW);
        isOn = !isOn;
        delay(100);
    }

    digitalWrite(LED_PIN, LOW);

    wifiDisconnectHandler = WiFi.onStationModeDisconnected(std::bind(&WiFiHandler::onWifiDisconnect, this, std::placeholders::_1));
    wifiConnectedHandler = WiFi.onStationModeConnected(std::bind(&WiFiHandler::onWifiConnected, this, std::placeholders::_1));

    Serial.println("");
    logger->print("Connected to ");
    logger->println(ssid);
    logger->print("IP address: ");
    logger->println(WiFi.localIP().toString());

    driver->setConnected();
    driver->setup();
}

void WiFiHandler::handle()
{
    driver->handle();
}

void WiFiHandler::onWifiDisconnect(const WiFiEventStationModeDisconnected &event)
{
    digitalWrite(LED_PIN, HIGH);
    logger->println("Disconnected");
    driver->setDisconnected();
    WiFi.disconnect();
    WiFi.begin(ssid, password);
}

void WiFiHandler::onWifiConnected(const WiFiEventStationModeConnected &event)
{
    digitalWrite(LED_PIN, LOW);
    logger->println("Connected");
    driver->setConnected();
}