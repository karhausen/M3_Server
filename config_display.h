#pragma once
#include <Arduino.h>

// I2C Pins (ESP32 Standard oft 21/22)
static constexpr uint8_t OLED_SDA = 21;
static constexpr uint8_t OLED_SCL = 22;

// I2C Adresse (meist 0x3C, manchmal 0x3D)
static constexpr uint8_t OLED_ADDR = 0x3C;

// Display
static constexpr int OLED_W = 128;
static constexpr int OLED_H = 128;
static constexpr uint8_t OLED_ROTATION = 3;


// UI Bereiche
static constexpr int UI_HEADER_H = 16;
static constexpr int UI_FOOTER_H = 24;

// Refresh-Limit (optional)
static constexpr uint32_t UI_MIN_REFRESH_MS = 50;

