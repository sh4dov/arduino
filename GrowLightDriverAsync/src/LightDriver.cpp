#include "LightDriver.h"

LightDriver::LightDriver(Logger *logger, TimeService *timeService) : 
                            ledHandler(5),
                            server(80),
                            timer(std::bind(&LightDriver::handleTimedEvents, this), 1000 * 60)
{
    this->logger = logger;
    this->timeService = timeService;
}

void LightDriver::handle() 
{
    this->ledHandler.handle();
    this->timer.update();
}

void LightDriver::setup()
{
    this->ledHandler.setup();
    this->ledHandler.setValue(5);
    
    this->server.on("/", std::bind(&LightDriver::handleRoot, this, std::placeholders::_1));
    this->server.on("/log", std::bind(&LightDriver::handleLog, this, std::placeholders::_1));
    this->server.onNotFound(std::bind(&LightDriver::handleNotFound, this, std::placeholders::_1));

    AsyncCallbackJsonWebHandler* onOffhandler = new AsyncCallbackJsonWebHandler("/onoff", std::bind(&LightDriver::handleOnOff, this, std::placeholders::_1, std::placeholders::_2));
    AsyncCallbackJsonWebHandler* changeBrightnesshandler = new AsyncCallbackJsonWebHandler("/brightness", std::bind(&LightDriver::handleChangeBrightness, this, std::placeholders::_1, std::placeholders::_2));

    this->server.addHandler(onOffhandler);
    this->server.addHandler(changeBrightnesshandler);

    if(LittleFS.begin())
    {
        File cfg = LittleFS.open("/vals.json", "r");
        if(cfg)
        {
            DynamicJsonDocument doc(100);
            deserializeJson(doc, cfg);
            JsonObject obj = doc.as<JsonObject>();
            this->ledHandler.setMaxValue(obj["maxValue"]);
            cfg.close();
        }

        LittleFS.end();
    }

    this->server.begin();
    this->timer.start();
    this->logger->println("Server started");
}

void LightDriver::sendResponse(AsyncWebServerRequest *request, String msg)
{
    request->send(200, "text/html", msg);
}

void LightDriver::handleNotFound(AsyncWebServerRequest *request)
{    
    this->sendResponse(request, "Not found");
}

void LightDriver::handleLog(AsyncWebServerRequest *request)
{
    this->sendResponse(request, this->logger->getLog());
}

void LightDriver::handleOnOff(AsyncWebServerRequest *request, JsonVariant &json)
{
    const JsonObject& jsonObj = json.as<JsonObject>();
    bool value = jsonObj["value"];
    if(value)
    {
        this->ledHandler.turnOn();
    }
    else
    {
        this->ledHandler.turnOff();
    }

    this->sendResponse(request, "ok");
}

void LightDriver::handleChangeBrightness(AsyncWebServerRequest *request, JsonVariant &json)
{
    const JsonObject& jsonObj = json.as<JsonObject>();
    int value = jsonObj["value"];
    this->ledHandler.setMaxValue(value);

    if(LittleFS.begin())
    {
        File cfg = LittleFS.open("/vals.json", "w");
        DynamicJsonDocument doc(100);
        JsonObject obj = doc.to<JsonObject>();
        obj["maxValue"] = value;
        serializeJson(doc, cfg);
        cfg.close();

        LittleFS.end();
    }

    this->sendResponse(request, "ok");
}

void LightDriver::handleRoot(AsyncWebServerRequest *request)
{
    String src = htmlsrc1;

    src += this->generateLedHtml();
    src += htmlsrc2;

    this->sendResponse(request, src);
}

String LightDriver::generateLedHtml()
{
    String src = "<h1>";
    src += "Grow light led";
    src += "</h1><input class=\"btn\" type=\"button\" value=\"";
    src += (!this->ledHandler.isOn() ? "on" : "off");
    src += "\" onclick=\"turnOnOff(";
    src += (!this->ledHandler.isOn() ? "1" : "0");
    src += ")\"><input class=\"range\" type=\"range\" value=\"";
    src += String(this->ledHandler.getMaxValue());
    src += "\" min=\"0\" max=\"255\" oninput=\"changeBrightness(this.value)\">";

    return src;
}

void LightDriver::setConnected()
{
    this->isConnected = true;
    this->timer.resume();
}

void LightDriver::setDisconnected()
{
    this->isConnected = false;
    this->timer.pause();
    this->ledHandler.turnOff();
    this->logger->println("Turn off led (diconnected)");
}

void LightDriver::handleTimedEvents()
{
    tm *tm;
    tm = this->timeService->now();
    this->logger->println("");
    this->logger->print("Current time: ");
    this->logger->println(this->timeService->toString(tm));

    if(!this->isConnected) {
        this->logger->println("Turn off led (not connected)");
        this->ledHandler.turnOff();
        return;
    }

    if(this->ledHandler.getValue() > 0 && (tm->tm_hour < 10 || tm->tm_hour >= 19))
    {
        this->logger->println("Turn off led (time)");
        this->ledHandler.turnOff();
    }
    else if(!this->ledHandler.isOn() && tm->tm_hour >= 10 && tm->tm_hour < 19)
    {
        this->logger->println("Turn on led (time)");
        this->ledHandler.turnOn();
    }
}
