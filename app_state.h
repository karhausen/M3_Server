#pragma once
#include <Arduino.h>

struct AppState {
  bool radio_connected = false;
  uint32_t freq_hz = 14074000;
  String mode = "USB";
  String preset = "Plain";   // "Plain" oder "1".."9"
};

// extern AppState g_state;