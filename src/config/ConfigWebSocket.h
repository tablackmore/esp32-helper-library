#ifndef CONFIG_WEBSOCKET_H
#define CONFIG_WEBSOCKET_H

#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>

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

    void handleWebSocketData(AsyncWebSocketClient *client,
                             void *arg,
                             uint8_t *data,
                             size_t len);

    void handleScanNetworksRequest(AsyncWebSocketClient *client);
    void handleConnectToNetworkRequest(AsyncWebSocketClient *client, const char *ssid, const char *password);
    void handleClearSavedNetworkRequest(AsyncWebSocketClient *client);
    void handleGetFilesRequest(AsyncWebSocketClient *client);
    void handleGetFileContentRequest(AsyncWebSocketClient *client, const char *filename);
    void handleSaveFileRequest(AsyncWebSocketClient *client, const char *filename, const char *content);
    void handleDeleteFileRequest(AsyncWebSocketClient *client, const char *filename);
};

#endif