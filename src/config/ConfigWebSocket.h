#ifndef CONFIG_WEBSOCKET_H
#define CONFIG_WEBSOCKET_H

#include "../utils/WebSocketBase.h"
#include "../utils/WiFiScanner.h"

class ConfigWebSocket
{
public:
    static ConfigWebSocket &getInstance();
    void begin(AsyncWebServer *server);

private:
    static const char *WEBSOCKET_PATH;
    AsyncWebSocket ws;

    ConfigWebSocket() : ws(WEBSOCKET_PATH) {}
    void handleEvent(AsyncWebSocket *server,
                     AsyncWebSocketClient *client,
                     AwsEventType type,
                     void *arg,
                     uint8_t *data,
                     size_t len);
};

#endif