#include <Arduino.h>
#include <WiFi.h>
#include <SPIFFS.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <ESPmDNS.h>
#include <DNSServer.h>
#include "WiFiScanner.h"

const char *ssid = "Test_Network";
const char *password = "12345678";
const char *hostname = "esp32";

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");
DNSServer dnsServer;

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
  {
    Serial.println("Failed to mount SPIFFS");
    return;
  }

  // 1. Configure WiFi
  WiFi.persistent(false);
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAPConfig(IPAddress(192, 168, 4, 1), IPAddress(192, 168, 4, 1), IPAddress(255, 255, 255, 0));
  WiFi.softAP(ssid, password);
  delay(500);

  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);

  // 2. Start DNS Server
  dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
  dnsServer.start(53, "*", IP);
  delay(100); // Allow DNS to start

  // 3. Configure mDNS after DNS is ready
  if (!MDNS.begin(hostname))
  {
    Serial.println("Error starting mDNS");
  }
  else
  {
    MDNS.addService("http", "tcp", 80);
    Serial.println("mDNS responder started at http://esp32.local");
  }

  // Serve different files based on connection type
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
            {
        IPAddress remote_ip = request->client()->remoteIP();
        
        // Check if client is connected via AP (192.168.4.x)
        bool isAP = remote_ip[0] == 192 && 
                    remote_ip[1] == 168 && 
                    remote_ip[2] == 4;
                    
        String path = isAP ? "/ap/index.html" : "/sta/index.html";
        request->send(SPIFFS, path, "text/html"); });

  // Serve static files from appropriate folders
  server.serveStatic("/ap/", SPIFFS, "/ap/");
  server.serveStatic("/sta/", SPIFFS, "/sta/");
  server.serveStatic("/shared/", SPIFFS, "/shared/");

  // Special case for favicon.ico at root
  server.on("/favicon.ico", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(SPIFFS, "/shared/favicon.ico", "image/x-icon"); });

  ws.onEvent(onWebSocketEvent);
  server.addHandler(&ws);
  server.begin();
}

void loop()
{
  static unsigned long lastDnsCheck = 0;
  const unsigned long DNS_CHECK_INTERVAL = 100; // Check DNS every 100ms

  unsigned long currentMillis = millis();
  if (currentMillis - lastDnsCheck >= DNS_CHECK_INTERVAL)
  {
    lastDnsCheck = currentMillis;
    try
    {
      dnsServer.processNextRequest();
    }
    catch (...)
    {
      delay(1);
    }
  }

  WiFiScanner::getInstance().checkScanResult();
}