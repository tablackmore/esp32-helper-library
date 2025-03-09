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
  Serial.println("Mounting SPIFFS...");
  if (!SPIFFS.begin(true))
  {
    Serial.println("Failed to mount SPIFFS");
    return;
  }
  Serial.println("File system mounted SPIFFS");

  // List all files in SPIFFS
  Serial.println("Files in SPIFFS:");
  File root = SPIFFS.open("/");
  File file = root.openNextFile();
  while (file)
  {
    Serial.printf("  %s (%d bytes)\n", file.name(), file.size());
    file = root.openNextFile();
  }

  delay(1000);

  // Configure WiFi
  Serial.println("Configuring WiFi...");
  WiFiManager::getInstance().setHostname(hostname);
  WiFiManager::getInstance().beginAccessPoint(ssid, password);

  // Initialize DNS and Web services
  Serial.println("Initializing DNS and Web services...");
  DnsManager::getInstance().begin(WiFiManager::getInstance().getAccessPointIP(), hostname);
  WebServer::getInstance().begin();

  // Setup WebSocket handlers
  Serial.println("Setting up WebSocket handlers...");
  AsyncWebServer *server = WebServer::getInstance().getServer();
  ConfigWebSocket::getInstance().begin(server);
  MidiWebSocket::getInstance().begin(server);

  // Try to connect to saved WiFi network
  Serial.println("Attempting to connect to saved WiFi network...");
  WiFiManager::getInstance().tryLoadSavedNetwork([](bool success)
                                                 {
    if (success) {
      Serial.println("=== CONNECTED TO SAVED NETWORK SUCCESSFULLY ===");
      IPAddress staIP = WiFi.localIP();
      Serial.print("STA IP address: ");
      Serial.println(staIP);
      Serial.printf("Gateway: %s, Subnet: %s\n", 
                   WiFi.gatewayIP().toString().c_str(), 
                   WiFi.subnetMask().toString().c_str());
    } else {
      Serial.println("=== FAILED TO CONNECT TO SAVED NETWORK ===");
      Serial.printf("Current WiFi status: %d\n", WiFi.status());
      
      // Check if credentials file exists
      if (SPIFFS.exists("/wifi_credentials.json")) {
        Serial.println("Credentials file exists but connection failed");
        
        // Try to read the file for debugging
        File file = SPIFFS.open("/wifi_credentials.json", "r");
        if (file) {
          String content = file.readString();
          file.close();
          Serial.printf("Credentials file content: '%s'\n", content.c_str());
        } else {
          Serial.println("Could not open credentials file for reading");
        }
      } else {
        Serial.println("No credentials file found");
      }
    } });

  // Initialize MIDI
  Serial.println("Initializing MIDI...");
  delay(500);
  MidiWebSocket::getInstance().startBluetooth();

  // PotentiometerReader::getInstance().begin();

  Serial.println("Setup complete");
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