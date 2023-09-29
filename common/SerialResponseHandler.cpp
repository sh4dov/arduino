#include <SerialResponseHandler.h>

void SerialResponseHandler::onError(String error)
{
    Serial.println(error);
}

void SerialResponseHandler::onConnect()
{
    Serial.println("Connected");
}

void SerialResponseHandler::onDisconnect()
{
    Serial.println("Disconnected");
}

void SerialResponseHandler::onRawData(void *data, size_t len)
{
    Serial.println("Raw data size: " + String(len));
}

void SerialResponseHandler::onHeader(String header)
{
    Serial.println("Header:");
    Serial.println(header);
}

void SerialResponseHandler::onData(String data)
{
    Serial.println("Data size: " + String(data.length()));
    Serial.println("Data:");
    Serial.println(data);
}