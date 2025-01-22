#pragma once

#include <cstdint>

namespace HardwareConfig
{
    // LED Pins
    constexpr uint8_t WIFI_ACTIVE_LED = 2; // Built-in LED on most ESP32 dev boards
    constexpr uint8_t BLE_ACTIVE_LED = 4;  // LED to indicate BLE activity

    // Add other hardware pins here as needed
    // constexpr uint8_t SOME_OTHER_PIN = X;
}