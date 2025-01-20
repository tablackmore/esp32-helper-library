#pragma once

#include <Arduino.h>

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

    void sendNoteOn(uint8_t note, uint8_t velocity, uint8_t channel);
    void sendNoteOff(uint8_t note, uint8_t velocity, uint8_t channel);
    void sendControlChange(uint8_t controller, uint8_t value, uint8_t channel);

private:
    MidiHandler();
    ~MidiHandler();

    static void midiTask(void *parameter); // RTOS task
    TaskHandle_t midiTaskHandle = nullptr;

    bool isConnected = false;
    bool shouldRun = false;

    // Queue for MIDI messages
    static const uint8_t QUEUE_SIZE = 32;
    QueueHandle_t midiQueue;

    struct MidiMessage
    {
        enum Type
        {
            NOTE_ON,
            NOTE_OFF,
            CONTROL_CHANGE
        } type;
        uint8_t data1; // note or controller
        uint8_t data2; // velocity or value
        uint8_t channel;
    };
};