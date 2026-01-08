#pragma once
#include <Arduino.h>

// Dummy: schreibt auf Serial (später Serial2)
void radio_send_connect();
void radio_send_disconnect();
void radio_send_preset(const String& preset);
void radio_send_mode(const String& mode);
void radio_send_freq(uint32_t hz);