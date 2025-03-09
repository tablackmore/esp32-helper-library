#include "WebServer.h"
#include "../config/HardwareConfig.h"
#include <ESPmDNS.h>
#include <Arduino.h>

WebServer::WebServer()
{
    // Initialize WiFi status LED
    pinMode(HardwareConfig::WIFI_ACTIVE_LED_PIN, OUTPUT);
    digitalWrite(HardwareConfig::WIFI_ACTIVE_LED_PIN, LOW);
}

void WebServer::begin()
{
    setupStaticRoutes();
    setupCaptivePortalRoutes();
    server.begin();
    Serial.println("Web Server started");

    // Turn on LED when WiFi is ready
    digitalWrite(HardwareConfig::WIFI_ACTIVE_LED_PIN, HIGH);
}

void WebServer::stop()
{
    digitalWrite(HardwareConfig::WIFI_ACTIVE_LED_PIN, LOW);
    server.end();
}

void WebServer::setupStaticRoutes()
{
    // Debug logging
    Serial.println("Setting up static routes");

    // Root route with AP/STA detection
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
              {
        IPAddress remote_ip = request->client()->remoteIP();
        bool isAP = remote_ip[0] == 192 && 
                    remote_ip[1] == 168 && 
                    remote_ip[2] == 4;
        String redirectPath = isAP ? "/ap/index.html" : "/sta/index.html";
        request->redirect(redirectPath); });

    // Handle favicon
    server.on("/favicon.ico", HTTP_GET, [](AsyncWebServerRequest *request)
              {
        Serial.println("Favicon requested");
        if (SPIFFS.exists("/web/favicon.ico.gz")) {
            request->send(SPIFFS, "/web/favicon.ico.gz", "image/x-icon", true);
        } else if (SPIFFS.exists("/web/favicon.ico")) {
            request->send(SPIFFS, "/web/favicon.ico", "image/x-icon");
        } else {
            request->send(404);
        } });

    // Generic file handler with better error checking
    server.onNotFound([this](AsyncWebServerRequest *request)
                      {
        if (request->method() != HTTP_GET) {
            request->send(405);
            return;
        }

        String path = "/web" + request->url();
        String gzPath = path + ".gz";

        Serial.print("Looking for file: "); Serial.println(gzPath);
        
        // Check if files exist before trying to serve them
        bool gzExists = SPIFFS.exists(gzPath);
        bool fileExists = SPIFFS.exists(path);
        
        if (!gzExists && !fileExists) {
            Serial.println("File not found: " + path);
            request->send(404);
            return;
        }

        // Use a try-catch block to prevent crashes
        try {
            String contentType = getContentType(path);
            if (gzExists) {
                AsyncWebServerResponse *response = request->beginResponse(SPIFFS, gzPath, contentType);
                if (response) {
                    response->addHeader("Content-Encoding", "gzip");
                    request->send(response);
                } else {
                    Serial.println("Failed to create response for: " + gzPath);
                    request->send(500);
                }
            } else {
                request->send(SPIFFS, path, contentType);
            }
        } catch (...) {
            Serial.println("Exception while serving: " + path);
            request->send(500);
        } });

    Serial.println("Static routes setup complete");
}

void WebServer::setupCaptivePortalRoutes()
{
    // Apple CaptiveNetwork Support
    server.on("/hotspot-detect.html", HTTP_GET, [](AsyncWebServerRequest *request)
              {
        AsyncWebServerResponse *response = request->beginResponse(200, "text/html", 
            "<HTML><HEAD><TITLE>Success</TITLE></HEAD><BODY>Success</BODY></HTML>");
        response->addHeader("Cache-Control", "no-cache, no-store, must-revalidate");
        response->addHeader("Pragma", "no-cache");
        response->addHeader("Expires", "-1");
        request->send(response); });

    // Other captive portal endpoints
    server.on("/generate_204", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->redirect("/ap/index.html"); });

    server.on("/redirect", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->redirect("/ap/index.html"); });

    server.on("/success.txt", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send(200, "text/plain", "success"); });
}

String WebServer::getContentType(const String &path)
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