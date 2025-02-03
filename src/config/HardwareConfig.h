#pragma once

#include <cstdint>

namespace HardwareConfig
{
    // LED Pins - These are valid for ESP32-DEV
    constexpr uint8_t WIFI_ACTIVE_LED_PIN = 2; // Built-in LED on most ESP32-DEV boards
    constexpr uint8_t BLE_ACTIVE_LED_PIN = 4;

    // Analog Inputs - Make sure these are ADC1 pins
    // ESP32 ADC1 pins: 32, 33, 34, 35, 36, 37, 38, 39
    constexpr uint8_t JOYSTICK_1_X_PIN = 32;
    constexpr uint8_t JOYSTICK_1_Y_PIN = 33;
    constexpr uint8_t JOYSTICK_2_X_PIN = 34;
    constexpr uint8_t JOYSTICK_2_Y_PIN = 35;

    // Potentiometer Pins
    constexpr uint8_t NUM_POTS = 4; // Maximum number of potentiometers
    constexpr uint8_t POT_PINS[NUM_POTS] = {
        JOYSTICK_1_X_PIN, JOYSTICK_1_Y_PIN, JOYSTICK_2_X_PIN, JOYSTICK_2_Y_PIN};

    // ADC Configuration
    constexpr uint8_t ADC_RESOLUTION = 12;                        // 12-bit resolution
    constexpr uint16_t ADC_MAX_VALUE = (1 << ADC_RESOLUTION) - 1; // 2^12 - 1 = 4095
    constexpr uint8_t NOISE_THRESHOLD = 40;                       // Ignore changes smaller than this
}