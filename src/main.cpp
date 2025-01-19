#include <Arduino.h>
#include <WiFi.h>
#include <SPIFFS.h>
#include <ESPAsyncWebServer.h>
#include "utils/WiFiScanner.h"
#include "config/ConfigWebSocket.h"
#include "midi/MidiWebSocket.h"
#include "server/WebServer.h"
#include "server/DnsManager.h"

const char *ssid = "Test_Network";
const char *password = "12345678";
const char *hostname = "esp32";

AsyncWebSocket ws("/ws");

void setup()
{
  Serial.begin(115200);

  // Initialize SPIFFS
  if (!SPIFFS.begin(true))
  {
    Serial.println("Failed to mount SPIFFS");
    return;
  }
  Serial.println("File system mounted SPIFFS");
  delay(1000);

  // Configure WiFi Access Point
  WiFi.persistent(false);
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAPConfig(IPAddress(192, 168, 4, 1), IPAddress(192, 168, 4, 1), IPAddress(255, 255, 255, 0));
  WiFi.softAP(ssid, password);
  delay(4000);

  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);

  // Try to connect to saved WiFi network
  WiFiScanner::getInstance().tryLoadSavedNetwork([](bool success)
                                                 {
        if (success) {
            Serial.println("Connected to saved network!");
            IPAddress staIP = WiFi.localIP();
            Serial.print("STA IP address: ");
            Serial.println(staIP);
        } else {
            Serial.println("Failed to connect to saved network");
        } });

  // Initialize DNS and Web services
  DnsManager::getInstance().begin(IP, hostname);
  WebServer::getInstance().begin();

  // Setup WebSocket handlers
  AsyncWebServer *server = WebServer::getInstance().getServer();
  ConfigWebSocket::getInstance().begin(server);
  MidiWebSocket::getInstance().begin(server);

  // Initialize MIDI
  delay(500);
  MidiWebSocket::getInstance().startBluetooth();
}

void loop()
{
  MidiWebSocket::getInstance().update();
  DnsManager::getInstance().processRequests();
  WiFiScanner::getInstance().checkScanResult();
}

void onProgrammingMode()
{
  MidiWebSocket::getInstance().end(); // Close Serial2 MIDI connection
  delay(100);                         // Give it a moment to finish any pending operations
}