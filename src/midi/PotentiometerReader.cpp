#include "PotentiometerReader.h"

PotentiometerReader::~PotentiometerReader()
{
    analogReadResolution(HardwareConfig::ADC_RESOLUTION);
    for (uint8_t i = 0; i < HardwareConfig::NUM_POTS; ++i)
    {
        delete pots[i].analog;
    }
}

void PotentiometerReader::begin()
{

    for (uint8_t i = 0; i < HardwareConfig::NUM_POTS; ++i)
    {
        uint8_t pin = HardwareConfig::POT_PINS[i];
        pinMode(pin, INPUT);
        pots[i].pin = pin;
        pots[i].cc = MidiMapping::getMidiCC(pin);
        // Initialize with pin, snapMultiplier (optional, default 0.01), sleepEnable (optional, default false)
        pots[i].analog = new ResponsiveAnalogRead(pin, true);
        pots[i].analog->setAnalogResolution(HardwareConfig::ADC_MAX_VALUE);
        pots[i].analog->setActivityThreshold(HardwareConfig::NOISE_THRESHOLD);
    }
}

void PotentiometerReader::update()
{
    for (uint8_t i = 0; i < HardwareConfig::NUM_POTS; ++i)
    {
        if (pots[i].pin == 255 || pots[i].cc == 0)
            continue; // Skip unused pots or those not mapped

        pots[i].analog->update();

        if (pots[i].analog->hasChanged())
        {
            uint16_t smoothedValue = pots[i].analog->getValue();
            uint16_t midiValue = mapToMidi(smoothedValue, (pots[i].cc >= MidiMapping::PITCH_BEND_CC) ? MidiMapping::PITCH_BEND_MAX : MidiMapping::MIDI_MAX);
            sendMidiValues(pots[i].cc, midiValue);
        }
    }
}

void PotentiometerReader::sendMidiValues(uint8_t cc, uint16_t value)
{
    if (cc == MidiMapping::PITCH_BEND_CC)
    {
        MidiHandler::getInstance().sendPitchBend(
            value, MidiMapping::DEFAULT_CHANNEL);
    }
    else
    {
        MidiHandler::getInstance().sendControlChange(
            cc,
            value,
            MidiMapping::DEFAULT_CHANNEL);
    }
}

// Map ADC value to MIDI value
uint16_t PotentiometerReader::mapToMidi(uint16_t value, uint16_t maxOutput)
{
    if (maxOutput == MidiMapping::PITCH_BEND_MAX)
    {
        // For pitch bend, map to full range but ensure center position maps to PITCH_BEND_CENTER
        const uint16_t center = HardwareConfig::ADC_MAX_VALUE / 2;
        constexpr uint16_t deadzone = 300; // Increased deadzone for more stable center

        // If within deadzone, always return center position
        if (abs((int)value - (int)center) < deadzone)
        {
            return MidiMapping::PITCH_BEND_CENTER;
        }

        // Map values outside deadzone
        if (value < center - deadzone)
        {
            // Map lower half (0 to 8191)
            return map(value, 0, center - deadzone, 0, MidiMapping::PITCH_BEND_CENTER - 1);
        }
        else if (value > center + deadzone)
        {
            // Map upper half (8193 to 16383)
            return map(value, center + deadzone, HardwareConfig::ADC_MAX_VALUE,
                       MidiMapping::PITCH_BEND_CENTER + 1, MidiMapping::PITCH_BEND_MAX);
        }

        return MidiMapping::PITCH_BEND_CENTER; // Default to center
    }
    else
    {
        // For regular CC values from joystick
        const uint16_t center = HardwareConfig::ADC_MAX_VALUE / 2;
        const uint16_t deadzone = 200; // Adjust based on your joystick's stability

        // If within deadzone, return center value
        if (abs((int)value - (int)center) < deadzone)
        {
            return maxOutput / 2;
        }

        // Map the full range considering the center position
        if (value < center - deadzone)
        {
            // Lower half (0 to 63)
            return map(value, 0, center - deadzone, 0, maxOutput / 2 - 1);
        }
        else if (value > center + deadzone)
        {
            // Upper half (65 to 127)
            return map(value, center + deadzone, HardwareConfig::ADC_MAX_VALUE, maxOutput / 2 + 1, maxOutput);
        }

        return maxOutput / 2; // Default to center
    }
}