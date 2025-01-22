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
    void stop();
    AsyncWebServer *getServer() { return &server; }

private:
    WebServer();
    ~WebServer() = default;
    WebServer(const WebServer &) = delete;
    WebServer &operator=(const WebServer &) = delete;

    void setupStaticRoutes();
    void setupCaptivePortalRoutes();
    String getContentType(const String &path);

    AsyncWebServer server{80};
};