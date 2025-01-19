#include "ServerSetup.h"
#include <ESPAsyncWebServer.h>
#include <ESPmDNS.h>
#include "WebServer.h"

AsyncWebServer *ServerSetup::initializeServer()
{
    AsyncWebServer *server = WebServer::getInstance().getServer();

    if (!MDNS.begin("esp32"))
    {
        Serial.println("Error starting mDNS");
    }
    else
    {
        MDNS.addService("http", "tcp", 80);
        Serial.println("mDNS responder started at http://esp32.local");
    }

    WebServer::getInstance().begin();
    return server;
}