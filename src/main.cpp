#include <Arduino.h>
#include <WiFi.h>
#include <SPIFFS.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include "WiFiScanner.h"

const char *ssid = "ESP32_Network";
const char *password = "12345678";

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

void onWebSocketEvent(AsyncWebSocket *server, AsyncWebSocketClient *client,
                      AwsEventType type, void *arg, uint8_t *data, size_t len)
{
  if (type == WS_EVT_CONNECT)
  {
    Serial.printf("WebSocket client #%u connected\n", client->id());
  }
  else if (type == WS_EVT_DISCONNECT)
  {
    Serial.printf("WebSocket client #%u disconnected\n", client->id());
  }
  else if (type == WS_EVT_DATA)
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
                      
                      JsonDocument doc;
                      doc["type"] = "networkList";
                      doc["networks"] = networksDoc.as<JsonArray>();
                      
                      String Sedoc;
                      serializeJson(doc, Sedoc);
                      client->text(Sedoc);
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

void setup()
{
  Serial.begin(115200);
  if (!SPIFFS.begin(true))
    return;

  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(ssid, password);

  server.serveStatic("/", SPIFFS, "/").setDefaultFile("index.html");
  ws.onEvent(onWebSocketEvent);
  server.addHandler(&ws);
  server.begin();
}

void loop()
{
  WiFiScanner::getInstance().checkScanResult();
}