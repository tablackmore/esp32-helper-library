#pragma once

#include <cstdint>
#include "HardwareConfig.h"

namespace MidiMapping
{
    // MIDI Channel Configuration
    constexpr uint8_t DEFAULT_CHANNEL = 0; // MIDI channel 1 (0-15)

    // Common MIDI CC Numbers
    constexpr uint8_t CC_PITCH_BEND = 0xE0;   // Special case: Pitch Bend is not a CC
    constexpr uint8_t CC_MODULATION = 1;      // Modulation Wheel
    constexpr uint8_t CC_BREATH = 2;          // Breath Controller
    constexpr uint8_t CC_VOLUME = 7;          // Channel Volume
    constexpr uint8_t CC_PAN = 10;            // Pan
    constexpr uint8_t CC_EXPRESSION = 11;     // Expression Controller
    constexpr uint8_t CC_SUSTAIN = 64;        // Sustain Pedal (on/off)
    constexpr uint8_t CC_PORTAMENTO = 65;     // Portamento (on/off)
    constexpr uint8_t CC_SOSTENUTO = 66;      // Sostenuto Pedal
    constexpr uint8_t CC_SOFT_PEDAL = 67;     // Soft Pedal
    constexpr uint8_t CC_RESONANCE = 71;      // Filter Resonance
    constexpr uint8_t CC_RELEASE = 72;        // Release Time
    constexpr uint8_t CC_ATTACK = 73;         // Attack Time
    constexpr uint8_t CC_CUTOFF = 74;         // Filter Cutoff
    constexpr uint8_t CC_DECAY = 75;          // Decay Time
    constexpr uint8_t CC_PORTAMENTO_TIME = 5; // Portamento Time
    constexpr uint8_t CC_ALL_SOUND_OFF = 120; // All Sound Off
    constexpr uint8_t CC_ALL_NOTES_OFF = 123; // All Notes Off

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
            return CC_PITCH_BEND;
        case HardwareConfig::JOYSTICK_1_Y_PIN:
            return CC_MODULATION;
        case HardwareConfig::JOYSTICK_2_X_PIN:
            return CC_CUTOFF;
        case HardwareConfig::JOYSTICK_2_Y_PIN:
            return CC_DECAY;
        default:
            return 0; // Undefined
        }
    }
}