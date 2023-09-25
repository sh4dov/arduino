#include "WiFiHandler.h"

WiFiHandler::WiFiHandler(Logger *logger, IDriver *driver, String ssid, String password)
{
    this->logger = logger;
    this->driver = driver;
    this->ssid = ssid;
    this->password = password;
}

void WiFiHandler::setup()
{
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);

    while(WiFi.status() != WL_CONNECTED)
    {
        Serial.print(".");
    }

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
  logger->println("Disconnected");
  driver->setDisconnected();
  WiFi.disconnect();
  WiFi.begin(ssid, password);
}

void WiFiHandler::onWifiConnected(const WiFiEventStationModeConnected &event)
{
  logger->println("Connected");
  driver->setConnected();
}