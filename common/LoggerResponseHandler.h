#pragma once

#include <HttpAsyncClient.h>
#include <Logger.h>

class LoggerResponseHandler : public ResponseHandler
{
    private:
        Logger *logger;

    public:
        LoggerResponseHandler(Logger *logger) : logger(logger) {};
        virtual void onError(String error);
        virtual void onConnect() {}
        virtual void onDisconnect() {}
        virtual void onRawData(void *data, size_t len) {}
        virtual void onHeader(String header) {}
        virtual void onData(String data);
        virtual bool parseDataAsString() { return true; }
};