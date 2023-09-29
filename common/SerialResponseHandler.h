#pragma once

#include <Arduino.h>
#include <HttpAsyncClient.h>

class SerialResponseHandler : public ResponseHandler
{
    public:
        void onError(String error);
        void onConnect();
        void onDisconnect();
        void onRawData(void *data, size_t len);
        void onHeader(String header);
        void onData(String data);
        bool parseDataAsString() { return true; }
};