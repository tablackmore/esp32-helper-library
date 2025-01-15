#include "ConfigWebSocket.h"
const char *ConfigWebSocket::WEBSOCKET_PATH = "/config";

ConfigWebSocket &ConfigWebSocket::getInstance()
{
    static ConfigWebSocket instance;
    return instance;
}

void ConfigWebSocket::begin(AsyncWebServer *server)
{
    ws.onEvent([this](AsyncWebSocket *s, AsyncWebSocketClient *c,
                      AwsEventType t, void *arg, uint8_t *d, size_t l)
               { handleEvent(s, c, t, arg, d, l); });
    server->addHandler(&ws);
}
void ConfigWebSocket::handleEvent(AsyncWebSocket *server,
                                  AsyncWebSocketClient *client,
                                  AwsEventType type,
                                  void *arg,
                                  uint8_t *data,
                                  size_t len)
{
    if (type == WS_EVT_DATA)
    {
        AwsFrameInfo *info = (AwsFrameInfo *)arg;
        if (info->opcode == WS_TEXT)
        {
            String message = String((char *)data).substring(0, len);
            JsonDocument doc;
            DeserializationError error = deserializeJson(doc, message);
            if (error)
                return;

            String type = doc["type"];
            if (type == "scanNetworks")
            {
                WiFiScanner::getInstance().subscribe([client](const String &networkList)
                                                     {
                    if (client && client->status() == WS_CONNECTED) {
                        JsonDocument networksDoc;
                        deserializeJson(networksDoc, networkList);
                        
                        JsonDocument response;
                        response["type"] = "networkList";
                        response["networks"] = networksDoc.as<JsonArray>();
                        
                        String responseStr;
                        serializeJson(response, responseStr);
                        client->text(responseStr);
                    } });
            }
            else if (type == "connect")
            {
                WiFiScanner::getInstance().connectToNetwork(
                    doc["ssid"].as<String>(),
                    doc["password"].as<String>(),
                    [client](const String &status)
                    {
                        if (client && client->status() == WS_CONNECTED)
                        {
                            client->text(status);
                        }
                    });
            }
        }
    }
}