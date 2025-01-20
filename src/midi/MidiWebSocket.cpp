#include "MidiWebSocket.h"

const char *MidiWebSocket::WEBSOCKET_PATH = "/midi";

MidiWebSocket &MidiWebSocket::getInstance()
{
    static MidiWebSocket instance;
    return instance;
}

void MidiWebSocket::begin(AsyncWebServer *server)
{
    ws.onEvent([this](AsyncWebSocket *s, AsyncWebSocketClient *c,
                      AwsEventType t, void *arg, uint8_t *d, size_t l)
               { handleEvent(s, c, t, arg, d, l); });
    server->addHandler(&ws);
}

void MidiWebSocket::handleEvent(AsyncWebSocket *server,
                                AsyncWebSocketClient *client,
                                AwsEventType type,
                                void *arg,
                                uint8_t *data,
                                size_t len)
{
    if (type == WS_EVT_DATA)
    {
        AwsFrameInfo *info = (AwsFrameInfo *)arg;
        if (info->opcode == WS_TEXT)
        {
            String message = String((char *)data).substring(0, len);
            JsonDocument doc;
            DeserializationError error = deserializeJson(doc, message);
            if (error)
                return;

            String type = doc["type"];
            uint8_t channel = doc["channel"] | 0;

            if (type == "noteOn")
            {
                uint8_t note = doc["note"];
                uint8_t velocity = doc["velocity"];
                MidiHandler::getInstance().sendNoteOn(note, velocity, channel);
            }
            else if (type == "noteOff")
            {
                uint8_t note = doc["note"];
                uint8_t velocity = doc["velocity"] | 0;
                MidiHandler::getInstance().sendNoteOff(note, velocity, channel);
            }
            else if (type == "controlChange")
            {
                uint8_t controller = doc["controller"];
                uint8_t value = doc["value"];
                MidiHandler::getInstance().sendControlChange(controller, value, channel);
            }
        }
    }
}
