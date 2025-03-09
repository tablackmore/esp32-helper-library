#include "MidiWebSocket.h"

const char *MidiWebSocket::WEBSOCKET_PATH = "/midi";

MidiWebSocket &MidiWebSocket::getInstance()
{
    static MidiWebSocket instance;
    return instance;
}

void MidiWebSocket::begin(AsyncWebServer *server)
{
    Serial.println("Initializing MIDI WebSocket...");
    ws.onEvent([this](AsyncWebSocket *s, AsyncWebSocketClient *c,
                      AwsEventType t, void *arg, uint8_t *d, size_t l)
               { handleEvent(s, c, t, arg, d, l); });
    server->addHandler(&ws);
    Serial.println("MIDI WebSocket handler added");
}

void MidiWebSocket::handleEvent(AsyncWebSocket *server,
                                AsyncWebSocketClient *client,
                                AwsEventType type,
                                void *arg,
                                uint8_t *data,
                                size_t len)
{
    switch (type)
    {
    case WS_EVT_CONNECT:
        Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
        break;
    case WS_EVT_DISCONNECT:
        Serial.printf("WebSocket client #%u disconnected\n", client->id());
        break;
    case WS_EVT_DATA:
        handleWebSocketData(client, arg, data, len);
        break;
    case WS_EVT_ERROR:
        Serial.printf("WebSocket client #%u error(%u): %s\n", client->id(), *((uint16_t *)arg), (char *)data);
        break;
    }
}

void MidiWebSocket::handleWebSocketData(AsyncWebSocketClient *client, void *arg, uint8_t *data, size_t len)
{
    AwsFrameInfo *info = (AwsFrameInfo *)arg;
    if (!info || !data || len == 0)
    {
        Serial.println("Invalid WebSocket data received");
        return;
    }

    if (info->opcode != WS_TEXT)
    {
        Serial.println("Non-text WebSocket message received");
        return;
    }

    // Create a null-terminated string from the data
    char *message = (char *)malloc(len + 1);
    if (!message)
    {
        Serial.println("Failed to allocate memory for message");
        return;
    }
    memcpy(message, data, len);
    message[len] = '\0';

    // Parse JSON
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, message);
    free(message); // Free the allocated memory

    if (error)
    {
        Serial.print("JSON parsing failed: ");
        Serial.println(error.c_str());
        return;
    }

    // Process the message
    const char *type = doc["type"] | "";
    uint8_t channel = doc["channel"] | 0;

    Serial.printf("Received message type: %s, channel: %d\n", type, channel);

    if (strcmp(type, "bluetooth") == 0)
    {
        bool enable = doc["enable"] | false;
        handleBluetoothCommand(client, enable);
    }
    else if (strcmp(type, "noteOn") == 0)
    {
        uint8_t note = doc["note"] | 0;
        uint8_t velocity = doc["velocity"] | 0;
        MidiHandler::getInstance().sendNoteOn(note, velocity, channel);
    }
    else if (strcmp(type, "noteOff") == 0)
    {
        uint8_t note = doc["note"] | 0;
        uint8_t velocity = doc["velocity"] | 0;
        MidiHandler::getInstance().sendNoteOff(note, velocity, channel);
    }
    else if (strcmp(type, "controlChange") == 0)
    {
        uint8_t controller = doc["controller"] | 0;
        uint8_t value = doc["value"] | 0;
        MidiHandler::getInstance().sendControlChange(controller, value, channel);
    }
}

void MidiWebSocket::handleBluetoothCommand(AsyncWebSocketClient *client, bool enable)
{
    if (enable)
    {
        MidiHandler::getInstance().enableBluetooth();
    }
    else
    {
        MidiHandler::getInstance().disableBluetooth();
    }

    // Send back current state
    JsonDocument response;
    response["type"] = "bluetooth";
    response["enabled"] = MidiHandler::getInstance().isBluetoothEnabled();

    String jsonResponse;
    serializeJson(response, jsonResponse);

    if (client->status() == WS_CONNECTED)
    {
        client->text(jsonResponse);
    }
}
