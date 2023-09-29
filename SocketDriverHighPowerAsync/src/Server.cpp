#include "Server.h"

Server::Server(Logger *logger) :
    server(80),
    checkParamsTimer(std::bind(&Server::handleCheckParamsEvent, this), 1000 * 60),
    checkPinTimer(std::bind(&Server::handlePinEvent, this), 1000 * 10),
    logger(logger),
    handler(logger)
{
    info.error = "";
}

void Server::setup()
{
    server.on("/", std::bind(&Server::handleRoot, this, std::placeholders::_1));
    server.on("/log", std::bind(&Server::handleLog, this, std::placeholders::_1));
    server.on("/on", std::bind(&Server::handleOn, this, std::placeholders::_1));
    server.on("/off", std::bind(&Server::handleOff, this, std::placeholders::_1));


    server.begin();
    checkParamsTimer.start();
    checkPinTimer.start();
    logger->println("Server started");    
}

void Server::handle()
{
    checkParamsTimer.update();
    checkPinTimer.update();
}

void Server::setConnected()
{
    checkParamsTimer.resume();
    checkPinTimer.resume();
}

void Server::setDisconnected()
{
    checkParamsTimer.pause();
    checkPinTimer.pause();
    info.error = "Connection lost";
    turnOff();
}

void Server::handleLog(AsyncWebServerRequest *request)
{
    request->send(200, "text/html", this->logger->getLog());
}

void Server::handleOn(AsyncWebServerRequest *request)
{
    turnOn();
    request->send(200, "text/html", "on");
}

void Server::handleOff(AsyncWebServerRequest *request)
{
    turnOff();
    request->send(200, "text/html", "off");
}

void Server::handleRoot(AsyncWebServerRequest *request)
{
    String src = "<div>High power Socket Driver is ";
    src += (isOn ? "on" : "off");
    src += "</div>";
    src += "<div>Status: ";
    src += (info.error.length() > 0 ? info.error : isOn ? "OK" : "low pv input");
    src += "</div>";

    request->send(200, "text/html", src);
}

void Server::handleCheckParamsEvent()
{
    client.httpGet(&handler, "http://192.168.100.30:80/api/paramsCache");
    info = handler.getInfo();
    logger->println("voltage: " + String(info.voltage) + " power: " + String(info.power) + " active pover: " + String(info.activePower));    
}

void Server::handlePinEvent()
{
    if(!isOn && info.voltage >= 260)
    {
        turnOn();
    }
    else if(isOn && info.power < (info.activePower - 100UL))
    {
        turnOff();
    }
}

void Server::turnOn()
{
    isOn = true;
    analogWrite(pin, 255);
    logger->println("turn on");
    handleCheckParamsEvent();
}

void Server::turnOff()
{
    isOn = false;
    analogWrite(pin, 0);
    logger->println("turn off");
}

void ParamsResponseHandler::onData(String data)
{
    LoggerResponseHandler::onData(data);
    info.voltage = data.substring(64, 67).toInt();
    info.power = data.substring(97, 102).toInt();
    info.activePower = data.substring(27, 31).toInt();
    info.error = "";
}

void ParamsResponseHandler::onError(String error)
{
    LoggerResponseHandler::onError(error); 
    info.error = error;
}