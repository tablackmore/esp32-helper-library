#pragma once

#include <cstdint>

struct MidiMessage
{
    enum Type
    {
        NOTE_ON,
        NOTE_OFF,
        CONTROL_CHANGE,
        PITCH_BEND // Added PITCH_BEND
    } type;
    uint8_t data1;
    uint8_t data2;
    uint8_t channel;
};