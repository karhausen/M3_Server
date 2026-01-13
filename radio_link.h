#pragma once
#include <Arduino.h>

void radio_init();
void radio_loop();                 // regelmäßig aufrufen
bool radio_is_ready();             // z.B. Serial2 ok / optional Handshake

// High-level API (von GUI genutzt)
void radio_send_connect();
void radio_send_disconnect();
void radio_send_preset(const String& preset);
void radio_send_mode(const String& mode);
void radio_send_freq(uint32_t hz);

// Optional: Zugriff aufs letzte RX / Status
String radio_last_rx_line();
uint32_t radio_last_tx_ms();
