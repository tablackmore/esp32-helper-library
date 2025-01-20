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
    void update();
    void end();

    void sendNoteOn(uint8_t note, uint8_t velocity, uint8_t channel);
    void sendNoteOff(uint8_t note, uint8_t velocity, uint8_t channel);
    void sendControlChange(uint8_t controller, uint8_t value, uint8_t channel);

private:
    MidiHandler();
    bool isConnected = false;
};