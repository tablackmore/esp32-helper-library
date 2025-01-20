#include "WebServer.h"
#include <ESPmDNS.h>

WebServer::WebServer() {}

void WebServer::begin()
{
    setupStaticRoutes();
    setupCaptivePortalRoutes();

    // Start mDNS
    if (!MDNS.begin("esp32"))
    {
        Serial.println("Error starting mDNS");
    }
    else
    {
        MDNS.addService("http", "tcp", 80);
        Serial.println("mDNS responder started at http://esp32.local");
    }

    server.begin();
    Serial.println("Web Server started");
}

void WebServer::setupStaticRoutes()
{
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
        if (SPIFFS.exists("/favicon.ico.gz")) {
            AsyncWebServerResponse *response = request->beginResponse(SPIFFS, "/shared/favicon.ico.gz", "image/x-icon");
            response->addHeader("Content-Encoding", "gzip");
            request->send(response);
        } else {
            request->send(SPIFFS, "web/favicon.ico", "image/x-icon");
        } });

    // Generic file handler
    server.on("/*", HTTP_GET, [this](AsyncWebServerRequest *request)
              {

        String path = "/web" + request->url();
        String gzPath = path + ".gz";

        // Directory handling
        if (path.endsWith("/")) {
            path += "index.html";
            gzPath = path + ".gz";
        } else {
            int lastSlash = path.lastIndexOf('/');
            int lastDot = path.lastIndexOf('.');
            bool hasNoExtension = (lastDot == -1 || lastDot < lastSlash);
            
            if (hasNoExtension) {
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