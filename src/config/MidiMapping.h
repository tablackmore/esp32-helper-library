#pragma once

#include <cstdint>
#include "HardwareConfig.h"

namespace MidiMapping
{
    // MIDI Channel Configuration
    constexpr uint8_t DEFAULT_CHANNEL = 0; // MIDI channel 1 (0-15)

    // Control Change (CC) Numbers
    constexpr uint8_t MODULATION = 1;       // CC#1: Modulation wheel
    constexpr uint8_t PITCH_BEND_CC = 0xE0; // Special case: Pitch Bend is not a CC

    // ADC to MIDI value mapping
    constexpr uint16_t MIDI_MAX = 127;           // Max value for CC messages
    constexpr uint16_t PITCH_BEND_MAX = 16383;   // 14-bit value for pitch bend (0x3FFF)
    constexpr uint16_t PITCH_BEND_CENTER = 8192; // Center position (0x2000)

    // Function to get MIDI CC based on pin
    inline uint8_t getMidiCC(uint8_t pin)
    {
        switch (pin)
        {
        case HardwareConfig::JOYSTICK_1_X_PIN:
            return PITCH_BEND_CC;
        case HardwareConfig::JOYSTICK_1_Y_PIN:
            return MODULATION;
        default:
            return 0; // Undefined
        }
    }
}