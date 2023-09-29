#include <HttpAsyncClient.h>

void HttpAsyncClient::httpGet(ResponseHandler *handler, String url)
{
    urlInfo info = parse(handler, url);
    String header = "GET " + url + " HTTP/1.1\r\nHost: " + info.host + "\r\n\r\n";
    
    if(info.hasError)
    {
        return;
    }

    AsyncClient *client = new AsyncClient();    

    client->onConnect(std::bind(&HttpAsyncClient::onConnect, this, std::placeholders::_1, std::placeholders::_2, handler, header));
    client->onDisconnect(std::bind(&HttpAsyncClient::onDisconnect, this, std::placeholders::_1, std::placeholders::_2, handler));
    client->onData(std::bind(&HttpAsyncClient::onData, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, handler));
    client->onError(std::bind(&HttpAsyncClient::onError, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, handler));

    if(!client->connect(info.host.c_str(), info.port))
    {
        handler->onError("Cannot connect");
        delete client;
    }
}

void HttpAsyncClient::onConnect(void *arg, AsyncClient *client, ResponseHandler *handler, String header)
{
    client->onError(NULL, NULL);
    handler->onConnect();                    
    client->write(header.c_str());
}

void HttpAsyncClient::onDisconnect(void *arg, AsyncClient *client, ResponseHandler *handler)
{
    handler->onDisconnect();
    delete client;
}

void HttpAsyncClient::onData(void *arg, AsyncClient *client, void *data, size_t len, ResponseHandler *handler)
{
    handler->onRawData(data, len);

    String d = String((char *)data).substring(0, len);

    if(!handler->parseDataAsString())
    {
        return;
    }

    responseInfo ri = parseResponse(handler, d);
    handler->onHeader(ri.header);
    handler->onData(ri.content);
}

void HttpAsyncClient::onError(void *arg, AsyncClient *client, int error, ResponseHandler *handler)
{
    handler->onError("Connection error: " + error);
    delete client;
}

responseInfo HttpAsyncClient::parseResponse(ResponseHandler *handler, String data)
{
    responseInfo result;
    result.content = "";
    result.header = "";
    int idx1 = 0, idx2 = 0;
    String contentLength = "Content-Length: ";
    idx1 = data.indexOf(contentLength);
    if(idx1 < 0)
    {
        handler->onError("Content-Length header not found");
        return result;
    }

    idx1 += contentLength.length();
    idx2 = data.indexOf('\r', idx1);
    if(idx2 < 0)
    {
        handler->onError("Cannot determinate value of Content-Length");
        return result;
    }

    String len = data.substring(idx1, idx2);
    idx1 = data.length() - len.toInt();
    result.content = data.substring(idx1);
    result.header = data.substring(0, idx1);

    return result;
}

urlInfo HttpAsyncClient::parse(ResponseHandler *handler, String url)
{
    const char* invalidUrl = "Invalid url";
    int idx1 = 0, idx2 = 0;
    urlInfo result;
    result.port = 0;
    result.host = "";
    result.hasError = true;

    idx1 = url.indexOf(":");
    if(idx1 < 0)
    {
        handler->onError(invalidUrl);
        return result;
    }

    idx1 += 3; // skip ://
    idx2 = url.indexOf(":", idx1);
    if(idx2 < 0)
    {
        result.port = 80;
        idx2 = url.indexOf('/', idx1);
        result.host = idx2 < 0 ? url.substring(idx1) : url.substring(idx1, idx2);
    }
    else
    {
        result.host = url.substring(idx1, idx2);
        idx1 = idx2 + 1;
        idx2 = url.indexOf('/', idx1);
        if(idx2 < 0)
        {
            result.port = url.substring(idx1).toInt();
        }

        if((idx2 < 0 && !result.port) || idx1 == idx2)
        {
            handler->onError(invalidUrl);
            return result;
        }
        
        if(idx2 >= 0)
        {
            String p = url.substring(idx1, idx2);
            result.port = p.toInt(); // port                
        }
    }

    if(!result.port)
    {
        handler->onError(invalidUrl);
    }
    else 
    {
        result.hasError = false;
    }

    return result;
}