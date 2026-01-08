#pragma once
#include <Arduino.h>

// WLAN STA
static const char* STA_SSID = "DEIN_WLAN";
static const char* STA_PASS = "DEIN_PASSWORT";

// Fallback AP
static const char* AP_SSID  = "RadioRemote-ESP32";
static const char* AP_PASS  = "12345678";   // min. 8 Zeichen
static const uint8_t AP_CH  = 6;
static const bool AP_HIDE   = false;

static const uint32_t STA_TIMEOUT_MS = 10'000;
static const bool KEEP_AP_ALSO_WHEN_STA_OK = false;

// Serial
static const uint32_t SERIAL_BAUD = 115200;