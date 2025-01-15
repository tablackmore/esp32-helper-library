#ifndef WEBSOCKET_BASE_H
#define WEBSOCKET_BASE_H

#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>

class WebSocketBase
{
protected:
    AsyncWebSocket ws;

    WebSocketBase(const char *endpoint) : ws(endpoint) {}
    virtual ~WebSocketBase() {}

    virtual void handleEvent(AsyncWebSocket *server,
                             AsyncWebSocketClient *client,
                             AwsEventType type,
                             void *arg,
                             uint8_t *data,
                             size_t len) = 0;

public:
    void begin(AsyncWebServer *server)
    {
        ws.onEvent([this](AsyncWebSocket *s, AsyncWebSocketClient *c,
                          AwsEventType t, void *arg, uint8_t *d, size_t l)
                   { handleEvent(s, c, t, arg, d, l); });
        server->addHandler(&ws);
    }
};

#endif