#include "MidiHandler.h"
#include <BLEMIDI_Transport.h>
#include <hardware/BLEMIDI_ESP32_NimBLE.h>
#include <MIDI.h>

namespace
{
    // Create MIDI instance for Serial2 (Hardware UART)
    struct Serial2MIDISettings : public midi::DefaultSettings
    {
        static const long BaudRate = 31250;
    };

    static MIDI_CREATE_CUSTOM_INSTANCE(HardwareSerial, Serial2, SERIALMIDI, Serial2MIDISettings);
    static BLEMIDI_CREATE_DEFAULT_INSTANCE();
}

MidiHandler::MidiHandler() {}

void MidiHandler::begin()
{
    Serial2.begin(31250, SERIAL_8N1, 13, 14);

    BLEMIDI.setHandleConnected([]()
                               {
        getInstance().isConnected = true;
        Serial.println("DEBUG: Connected to BLE MIDI"); });

    BLEMIDI.setHandleDisconnected([]()
                                  {
        getInstance().isConnected = false;
        Serial.println("DEBUG: Disconnected from BLE MIDI"); });

    MIDI.begin();
    SERIALMIDI.begin(MIDI_CHANNEL_OMNI);

    Serial.println("DEBUG: BLE MIDI Started Successfully");
    Serial.println("DEBUG: Device Name: ESP32 MIDI");
    Serial.println("DEBUG: Waiting for connections...");
}

void MidiHandler::update()
{
    if (isConnected)
    {
        MIDI.read();
        SERIALMIDI.read();
    }
}

void MidiHandler::end()
{
    Serial2.end();
}

void MidiHandler::sendNoteOn(uint8_t note, uint8_t velocity, uint8_t channel)
{
    MIDI.sendNoteOn(note, velocity, channel);
    SERIALMIDI.sendNoteOn(note, velocity, channel);
    Serial.printf("DEBUG: Note on: %d, velocity: %d, channel: %d\n", note, velocity, channel);
}

void MidiHandler::sendNoteOff(uint8_t note, uint8_t velocity, uint8_t channel)
{
    MIDI.sendNoteOff(note, velocity, channel);
    SERIALMIDI.sendNoteOff(note, velocity, channel);
    Serial.printf("DEBUG: Note off: %d, velocity: %d, channel: %d\n", note, velocity, channel);
}

void MidiHandler::sendControlChange(uint8_t controller, uint8_t value, uint8_t channel)
{
    MIDI.sendControlChange(controller, value, channel);
    SERIALMIDI.sendControlChange(controller, value, channel);
    Serial.printf("DEBUG: Control change: %d, value: %d, channel: %d\n", controller, value, channel);
}