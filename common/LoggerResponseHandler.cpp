#include <LoggerResponseHandler.h>

void LoggerResponseHandler::onError(String error)
{
    logger->println(error);
}

void LoggerResponseHandler::onData(String data)
{
    logger->println("Data:");
    logger->println(data);
}