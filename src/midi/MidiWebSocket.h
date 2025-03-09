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
    void startBluetooth() { MidiHandler::getInstance().enableBluetooth(); }
    void end() { MidiHandler::getInstance().disableBluetooth(); }

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
    void handleWebSocketData(AsyncWebSocketClient *client, void *arg, uint8_t *data, size_t len);
    void handleBluetoothCommand(AsyncWebSocketClient *client, bool enable);
};

#endif