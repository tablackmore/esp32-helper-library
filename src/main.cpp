#include <Arduino.h>
#include <SPIFFS.h>
#include <ESPAsyncWebServer.h>
#include "utils/WiFiScanner.h"
#include "config/ConfigWebSocket.h"
#include "midi/MidiWebSocket.h"
#include "core/WebServer.h"
#include "core/DnsManager.h"
#include "core/WiFiManager.h"
// #include "midi/PotentiometerReader.h"

const char *ssid = "Test_Network";
const char *password = "12345678";
const char *hostname = "esp32";

AsyncWebSocket ws("/ws");

void setup()
{
  Serial.begin(115200);
  delay(1000); // Give serial time to connect

  // Print memory info
  Serial.printf("Total heap: %d\n", ESP.getHeapSize());
  Serial.printf("Free heap: %d\n", ESP.getFreeHeap());

  // Initialize SPIFFS
  if (!SPIFFS.begin(true))
  {
    Serial.println("Failed to mount SPIFFS");
    return;
  }
  Serial.println("File system mounted SPIFFS");
  delay(1000);

  // Configure WiFi
  WiFiManager::getInstance().setHostname(hostname);
  WiFiManager::getInstance().beginAccessPoint(ssid, password);

  // Try to connect to saved WiFi network
  WiFiManager::getInstance().tryLoadSavedNetwork([](bool success)
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
  DnsManager::getInstance().begin(WiFiManager::getInstance().getAccessPointIP(), hostname);
  WebServer::getInstance().begin();

  // Setup WebSocket handlers
  AsyncWebServer *server = WebServer::getInstance().getServer();
  ConfigWebSocket::getInstance().begin(server);
  MidiWebSocket::getInstance().begin(server);

  // Initialize MIDI
  delay(500);
  MidiWebSocket::getInstance().startBluetooth();

  // PotentiometerReader::getInstance().begin();
}

void loop()
{
  DnsManager::getInstance().processRequests();

  // Add more debugging for WiFi scanning
  static unsigned long lastScanCheck = 0;
  if (millis() - lastScanCheck > 500)
  { // Check more frequently (every 500ms)
    WiFiManager::getInstance().checkScanResult();
    lastScanCheck = millis();
  }

  // Add periodic heap reporting
  static unsigned long lastHeapReport = 0;
  if (millis() - lastHeapReport > 10000)
  { // Every 10 seconds
    Serial.printf("Free heap: %d\n", ESP.getFreeHeap());
    lastHeapReport = millis();
  }

  // PotentiometerReader::getInstance().update();
}

void onProgrammingMode()
{
  MidiWebSocket::getInstance().end(); // Close Serial2 MIDI connection
  delay(100);                         // Give it a moment to finish any pending operations
}