#include "MidiHandler.h"
#include "../midi/MidiMessage.h"
#include "../config/MidiMapping.h"

// BLE-MIDI includes
#define BLEMIDI_ESP32_NimBLE_INCLUDE_CPP
#include <BLEMIDI_Transport.h>
#include <hardware/BLEMIDI_ESP32_NimBLE.h>
#include <MIDI.h>

namespace
{
    struct Serial2MIDISettings : public midi::DefaultSettings
    {
        static const long BaudRate = 31250;
    };

    // Create static MIDI instances
    MIDI_CREATE_CUSTOM_INSTANCE(HardwareSerial, Serial2, serialMidi, Serial2MIDISettings);
    BLEMIDI_CREATE_DEFAULT_INSTANCE();
}

MidiHandler::MidiHandler()
{
    midiQueue = xQueueCreate(QUEUE_SIZE, sizeof(MidiMessage));
}

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
    serialMidi.begin(MIDI_CHANNEL_OMNI);

    shouldRun = true;
    xTaskCreatePinnedToCore(
        midiTask,        // Task function
        "MIDI Task",     // Name
        8192,            // Stack size
        this,            // Parameter
        1,               // Priority
        &midiTaskHandle, // Task handle
        0                // Core ID (0 or 1)
    );

    Serial.println("DEBUG: BLE MIDI Started Successfully");
    Serial.println("DEBUG: Device Name: ESP32 MIDI");
    Serial.println("DEBUG: Waiting for connections...");
}

void MidiHandler::end()
{
    shouldRun = false;
    if (midiTaskHandle != nullptr)
    {
        vTaskDelete(midiTaskHandle);
        midiTaskHandle = nullptr;
    }
    Serial2.end();
}

void MidiHandler::midiTask(void *parameter)
{
    MidiHandler *handler = static_cast<MidiHandler *>(parameter);
    MidiMessage msg;

    while (handler->shouldRun)
    {
        if (handler->isConnected)
        {
            MIDI.read();
            serialMidi.read();
        }

        if (xQueueReceive(handler->midiQueue, &msg, 0) == pdTRUE)
        {
            switch (msg.type)
            {
            case MidiMessage::NOTE_ON:
                MIDI.sendNoteOn(msg.data1, msg.data2, msg.channel);
                serialMidi.sendNoteOn(msg.data1, msg.data2, msg.channel);
                break;
            case MidiMessage::NOTE_OFF:
                MIDI.sendNoteOff(msg.data1, msg.data2, msg.channel);
                serialMidi.sendNoteOff(msg.data1, msg.data2, msg.channel);
                break;
            case MidiMessage::CONTROL_CHANGE:
                MIDI.sendControlChange(msg.data1, msg.data2, msg.channel);
                serialMidi.sendControlChange(msg.data1, msg.data2, msg.channel);
                break;
            case MidiMessage::PITCH_BEND:
                // Pitch bend expects a single 14-bit value
                MIDI.sendPitchBend(msg.data1 | (msg.data2 << 7), msg.channel);
                serialMidi.sendPitchBend(msg.data1 | (msg.data2 << 7), msg.channel);
                break;
            }
        }
        vTaskDelay(1); // Small delay to prevent watchdog issues
    }
    vTaskDelete(NULL);
}

void MidiHandler::sendNoteOn(uint8_t note, uint8_t velocity, uint8_t channel)
{
    MidiMessage msg{MidiMessage::NOTE_ON, note, velocity, channel};
    xQueueSend(midiQueue, &msg, 0);
    Serial.printf("DEBUG: Note on: %d, velocity: %d, channel: %d\n", note, velocity, channel);
}

void MidiHandler::sendNoteOff(uint8_t note, uint8_t velocity, uint8_t channel)
{
    MidiMessage msg{MidiMessage::NOTE_OFF, note, velocity, channel};
    xQueueSend(midiQueue, &msg, 0);
    Serial.printf("DEBUG: Note off: %d, velocity: %d, channel: %d\n", note, velocity, channel);
}

void MidiHandler::sendControlChange(uint8_t controller, uint8_t value, uint8_t channel)
{
    MidiMessage msg{MidiMessage::CONTROL_CHANGE, controller, value, channel};
    xQueueSend(midiQueue, &msg, 0);
    Serial.printf("DEBUG: Control change: %d, value: %d, channel: %d\n", controller, value, channel);
}

void MidiHandler::sendPitchBend(uint16_t value, uint8_t channel)
{
    // Split 14-bit value into two 7-bit values
    MidiMessage msg{
        MidiMessage::PITCH_BEND,
        static_cast<uint8_t>(value & 0x7F),        // LSB
        static_cast<uint8_t>((value >> 7) & 0x7F), // MSB
        channel};
    xQueueSend(midiQueue, &msg, 0);
    Serial.printf("DEBUG: Pitch bend: %d, channel: %d\n", value, channel);
}

MidiHandler::~MidiHandler()
{
    if (midiTaskHandle != nullptr)
    {
        shouldRun = false;
        vTaskDelete(midiTaskHandle);
    }
    vQueueDelete(midiQueue);
}