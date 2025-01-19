#pragma once

#include <ESPAsyncWebServer.h>
#include "WebServer.h"

class ServerSetup
{
public:
    static ServerSetup &getInstance()
    {
        static ServerSetup instance;
        return instance;
    }

    AsyncWebServer *initializeServer();

private:
    ServerSetup() = default;
    ~ServerSetup() = default;
    ServerSetup(const ServerSetup &) = delete;
    ServerSetup &operator=(const ServerSetup &) = delete;
};