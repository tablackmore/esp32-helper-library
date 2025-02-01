#pragma once

#include "../midi/MidiMessage.h"
#include <Arduino.h>

// Forward declarations only - no implementation details
namespace midi
{
    class DefaultSettings;
}

namespace bleMidi
{
    class BLEMIDI_ESP32_NimBLE;
    struct MySettings;
}

class MidiHandler
{
public:
    static MidiHandler &getInstance()
    {
        static MidiHandler instance;
        return instance;
    }

    void begin();
    void end();

    // MIDI Sending Functions
    void sendNoteOn(uint8_t note, uint8_t velocity, uint8_t channel);
    void sendNoteOff(uint8_t note, uint8_t velocity, uint8_t channel);
    void sendControlChange(uint8_t controller, uint8_t value, uint8_t channel);
    void sendPitchBend(uint16_t value, uint8_t channel);

    void enableBluetooth() { isConnected = true; }
    void disableBluetooth() { isConnected = false; }
    bool isBluetoothEnabled() const { return isConnected; }

private:
    static constexpr size_t QUEUE_SIZE = 32;

    MidiHandler();
    ~MidiHandler();
    MidiHandler(const MidiHandler &) = delete;
    MidiHandler &operator=(const MidiHandler &) = delete;

    static void midiTask(void *parameter);

    QueueHandle_t midiQueue;
    TaskHandle_t midiTaskHandle = nullptr;
    bool shouldRun = false;
    bool isConnected = false;
};