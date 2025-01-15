#include "MidiWebSocket.h"
#include <BLEMIDI_Transport.h>
#include <hardware/BLEMIDI_ESP32_NimBLE.h>

BLEMIDI_CREATE_DEFAULT_INSTANCE();
unsigned long t0 = millis();
bool isConnected = false;

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
void MidiWebSocket::startBluetooth()
{
    BLEMIDI.setHandleConnected([]()
                               {
                                   isConnected = true;
                                   // digitalWrite(LED_BUILTIN, HIGH);
                                   Serial.printf("DEBUG: Connected to BLE MIDI\n"); });
    BLEMIDI.setHandleDisconnected([]()
                                  {
                                      isConnected = false;
                                      // digitalWrite(LED_BUILTIN, LOW);
                                      Serial.printf("DEBUG: Disconnected from BLE MIDI\n"); });
    MIDI.begin();
    Serial.println("DEBUG: BLE MIDI Started Successfully");
    Serial.println("DEBUG: Device Name: ESP32 MIDI");
    Serial.println("DEBUG: Waiting for connections...");
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
                Serial.printf("DEBUG: Note on: %d, velocity: %d, channel: %d\n", note, velocity, channel);
                MIDI.sendNoteOn(note, velocity, channel);
            }
            else if (type == "noteOff")
            {
                uint8_t note = doc["note"];
                uint8_t velocity = doc["velocity"] | 0;
                MIDI.sendNoteOff(note, velocity, channel);
                Serial.printf("DEBUG: Note off: %d, velocity: %d, channel: %d\n", note, velocity, channel);
            }
            else if (type == "controlChange")
            {
                uint8_t controller = doc["controller"];
                uint8_t value = doc["value"];
                MIDI.sendControlChange(controller, value, channel);
                Serial.printf("DEBUG: Control change: %d, value: %d, channel: %d\n", controller, value, channel);
            }
        }
    }
}

void MidiWebSocket::update()
{
    if (isConnected)
    {
        MIDI.read();
    }
}