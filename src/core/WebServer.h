#pragma once

#include <ESPAsyncWebServer.h>
#include <SPIFFS.h>

class WebServer
{
public:
    static WebServer &getInstance()
    {
        static WebServer instance;
        return instance;
    }

    void begin();
    AsyncWebServer *getServer() { return &server; }

private:
    WebServer();
    void setupStaticRoutes();
    void setupCaptivePortalRoutes();
    String getContentType(const String &path);

    AsyncWebServer server{80};
};