#include "Server.h"

Server::Server(Logger *logger) :
    server(80),
    logger(logger)
{
}

void Server::setup()
{
    server.on("/", std::bind(&Server::handleRoot, this, std::placeholders::_1));

    server.begin();
    logger->println("Server started");    
}

void Server::handle()
{   
    const int max = 10;
    char buf[max + 1];
    buf[max] = 0;
    int i = 0;

    while(Serial.available())
    {
        buf[i] = (char)Serial.read();
        i++;
        if(i == max)
        {
            logger->print(buf);
            i = 0;
        }
    }

    if(i > 0)
    {
        buf[i] = 0;
        logger->print(buf);
    }
}

void Server::handleRoot(AsyncWebServerRequest *request)
{
    String src = "<html><head><script>setTimeout(function(){window.location.reload(1);}, 5000);</script></head><body><div>SerialToWeb logs:<div>";

    src += logger->getLog();

    src += "</body></html>";

    request->send(200, "text/html", src);
}