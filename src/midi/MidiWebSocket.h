#ifndef MIDI_WEBSOCKET_H
#define MIDI_WEBSOCKET_H

#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include "MidiHandler.h"

class MidiWebSocket
{
public:
    static MidiWebSocket &getInstance();
    void begin(AsyncWebServer *server);
    void startBluetooth() { MidiHandler::getInstance().begin(); }
    void end() { MidiHandler::getInstance().end(); }

private:
    static const char *WEBSOCKET_PATH;
    AsyncWebSocket ws;

    MidiWebSocket() : ws(WEBSOCKET_PATH) {}
    void handleEvent(AsyncWebSocket *server,
                     AsyncWebSocketClient *client,
                     AwsEventType type,
                     void *arg,
                     uint8_t *data,
                     size_t len);
};

#endif