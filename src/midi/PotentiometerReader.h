#pragma once
#include <Arduino.h>
#include <ResponsiveAnalogRead.h>
#include "../config/HardwareConfig.h"
#include "../config/MidiMapping.h"
#include "../midi/MidiHandler.h"

struct Potentiometer
{
    uint8_t pin;
    uint8_t cc;
    ResponsiveAnalogRead *analog;
};

class PotentiometerReader
{
public:
    static PotentiometerReader &getInstance()
    {
        static PotentiometerReader instance;
        return instance;
    }

    ~PotentiometerReader();
    void begin();
    void update();

    // Delete copy constructor and assignment operator
    PotentiometerReader(const PotentiometerReader &) = delete;
    void operator=(const PotentiometerReader &) = delete;

private:
    PotentiometerReader() = default; // Make constructor private
    Potentiometer pots[HardwareConfig::NUM_POTS];
    uint16_t mapToMidi(uint16_t value, uint16_t maxValue);
    void sendMidiValues(uint8_t cc, uint16_t value);
};