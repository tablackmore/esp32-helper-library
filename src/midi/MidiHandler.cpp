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

MidiHandler::MidiHandler()
{
    midiQueue = xQueueCreate(QUEUE_SIZE, sizeof(MidiMessage));
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
            SERIALMIDI.read();
        }

        if (xQueueReceive(handler->midiQueue, &msg, 0) == pdTRUE)
        {
            switch (msg.type)
            {
            case MidiMessage::NOTE_ON:
                MIDI.sendNoteOn(msg.data1, msg.data2, msg.channel);
                SERIALMIDI.sendNoteOn(msg.data1, msg.data2, msg.channel);
                break;
            case MidiMessage::NOTE_OFF:
                MIDI.sendNoteOff(msg.data1, msg.data2, msg.channel);
                SERIALMIDI.sendNoteOff(msg.data1, msg.data2, msg.channel);
                break;
            case MidiMessage::CONTROL_CHANGE:
                MIDI.sendControlChange(msg.data1, msg.data2, msg.channel);
                SERIALMIDI.sendControlChange(msg.data1, msg.data2, msg.channel);
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