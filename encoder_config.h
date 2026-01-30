#pragma once
#include <Arduino.h>

// -------- Encoder Pins --------
static constexpr uint8_t ENC_A   = 32;   // CLK
static constexpr uint8_t ENC_B   = 33;   // DT
static constexpr uint8_t ENC_BTN = 25;   // SW

// -------- Verhalten --------
// Anzahl Interrupt-Ticks pro Rastung
// KY-040 meistens 4
static constexpr int8_t ENC_TICKS_PER_DETENT = 4;

// Button
static constexpr uint32_t ENC_BTN_DEBOUNCE_MS  = 25;
static constexpr uint32_t ENC_BTN_LONGPRESS_MS = 600;
