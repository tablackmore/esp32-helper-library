#include <Arduino.h>
#include <WiFi.h>
#include <SPIFFS.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <ESPmDNS.h>
#include <DNSServer.h>
#include "utils/WiFiScanner.h"
#include "config/ConfigWebSocket.h"
#include "midi/MidiWebSocket.h"

const char *ssid = "Test_Network";
const char *password = "12345678";
const char *hostname = "esp32";

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");
DNSServer dnsServer;

String getContentType(const String &path)
{
  if (path.endsWith(".html"))
    return "text/html";
  else if (path.endsWith(".css"))
    return "text/css";
  else if (path.endsWith(".js"))
    return "application/javascript";
  else if (path.endsWith(".json"))
    return "application/json";
  else if (path.endsWith(".ico"))
    return "image/x-icon";
  else if (path.endsWith(".png"))
    return "image/png";
  else if (path.endsWith(".jpg"))
    return "image/jpeg";
  else if (path.endsWith(".svg"))
    return "image/svg+xml";
  else if (path.endsWith(".txt"))
    return "text/plain";
  return "application/octet-stream";
}

void setup()
{
  Serial.begin(115200);
  if (SPIFFS.begin(true))
  {
    Serial.println("File system mounted SPIFFS");
  }
  else
  {
    Serial.println("Failed to mount SPIFFS");
    return;
  }
  delay(1000);

  Serial.println("\nListing SPIFFS files:");
  File root = SPIFFS.open("/");
  File file = root.openNextFile();
  while (file)
  {
    Serial.println(file.name());
    file = root.openNextFile();
  }

  // 1. Configure WiFi
  WiFi.persistent(false);
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAPConfig(IPAddress(192, 168, 4, 1), IPAddress(192, 168, 4, 1), IPAddress(255, 255, 255, 0));
  WiFi.softAP(ssid, password);
  delay(4000);
  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);

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
                    
        String redirectPath = isAP ? "/ap/index.html" : "/sta/index.html";
        
        // Send 302 (Found/Temporary) redirect
        request->redirect(redirectPath); });

  // Also handle the Apple's CaptiveNetwork Support
  server.on("/hotspot-detect.html", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    AsyncWebServerResponse *response = request->beginResponse(200, "text/html", 
      "<HTML><HEAD><TITLE>Success</TITLE></HEAD><BODY>Success</BODY></HTML>");
    response->addHeader("Cache-Control", "no-cache, no-store, must-revalidate");
    response->addHeader("Pragma", "no-cache");
    response->addHeader("Expires", "-1");
    request->send(response); });

  server.on("/generate_204", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->redirect("/ap/index.html"); });
  server.on("/redirect", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->redirect("/ap/index.html"); });
  server.on("/success.txt", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(200, "text/plain", "success"); });

  // Handle other web content
  server.on("/*", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    if (request->url() == "/ap/index.html") return; // Skip as it's handled above
    
    String path = "/web" + request->url();  // Prepend /web to all requests
    String gzPath = path + ".gz";
    
    // Check if path might be a directory
    if (path.endsWith("/")) {
        path += "index.html";
        gzPath = path + ".gz";
    } else {
        // Check if path has no extension
        int lastSlash = path.lastIndexOf('/');
        int lastDot = path.lastIndexOf('.');
        bool hasNoExtension = (lastDot == -1 || lastDot < lastSlash);
        
        if (hasNoExtension) {
            // If no extension, try as directory with index.html
            String testPath = path + "/index.html";
            String testGzPath = testPath + ".gz";
            if (SPIFFS.exists(testGzPath) || SPIFFS.exists(testPath)) {
                path = testPath;
                gzPath = testGzPath;
            }
        }
    }
    
    Serial.println("Looking for file: " + gzPath);
    if (SPIFFS.exists(gzPath)) {
        AsyncWebServerResponse *response = request->beginResponse(SPIFFS, gzPath, getContentType(request->url()));
        response->addHeader("Content-Encoding", "gzip");
        request->send(response);
    } else if (SPIFFS.exists(path)) {
        request->send(SPIFFS, path, getContentType(request->url()));
    } else {
        Serial.println("File not found: " + path);
        request->send(404);
    } });

  // Modify favicon handler
  server.on("/favicon.ico", HTTP_GET, [](AsyncWebServerRequest *request)
            {
        if (SPIFFS.exists("/favicon.ico.gz")) {
            AsyncWebServerResponse *response = request->beginResponse(SPIFFS, "/shared/favicon.ico.gz", "image/x-icon");
            response->addHeader("Content-Encoding", "gzip");
            request->send(response);
        } else {
            request->send(SPIFFS, "web/favicon.ico", "image/x-icon");
        } });

  delay(200); // Allow server to start
  ConfigWebSocket::getInstance().begin(&server);
  MidiWebSocket::getInstance().begin(&server);

  server.begin();
  delay(500); // Allow server to start
  Serial.println("Web Server started");

  MidiWebSocket::getInstance().startBluetooth();
}

void loop()
{
  static unsigned long lastDnsCheck = 0;
  const unsigned long DNS_CHECK_INTERVAL = 100; // Check DNS every 100ms

  MidiWebSocket::getInstance().update();

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

void onProgrammingMode()
{
  MidiWebSocket::getInstance().end(); // Close Serial2 MIDI connection
  delay(100);                         // Give it a moment to finish any pending operations
}