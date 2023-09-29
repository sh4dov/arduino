#pragma once

#include <Arduino.h>
#include <ESPAsyncTCP.h>

class ResponseHandler
{
    public:
        virtual void onError(String error) {}
        virtual void onConnect() {}
        virtual void onDisconnect() {}
        virtual void onRawData(void *data, size_t len) {}
        virtual void onHeader(String header) {}
        virtual void onData(String data) {}
        /*
        When this method returns false, response will not be parsed and onHeader, onData methods will not be executed, only onRawData.
        */
        virtual bool parseDataAsString() { return false; }
};

struct urlInfo
{
    String host;
    uint16_t port; 
    bool hasError;
};

struct responseInfo
{
    String header;
    String content;
};

class HttpAsyncClient 
{
    private:
        urlInfo parse(ResponseHandler *handler, String url);
        responseInfo parseResponse(ResponseHandler *handler, String data);
        void onConnect(void *arg, AsyncClient *client, ResponseHandler *handler, String header);
        void onDisconnect(void *arg, AsyncClient *client, ResponseHandler *handler);
        void onData(void *arg, AsyncClient *client, void *data, size_t len, ResponseHandler *handler);
        void onError(void *arg, AsyncClient *client, int error, ResponseHandler *handler);

    public:
        void httpGet(ResponseHandler *handler, String url);
};