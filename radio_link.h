#pragma once
#include <Arduino.h>
#include "config.h"

// enum class RadioState : uint8_t { BOOT, WAIT_OPEN_ACK, COM_PORT_IS_OPEN, WAIT_CONNECT_ACK, WAIT_DISCONNECT_ACK, READY, WAIT_SET_MODE_ACK };
// static RadioState radio_state = RadioState::BOOT;

const __FlashStringHelper* getRadioStateString();

void radio_init();
void radio_loop();                 // regelmäßig aufrufen

bool radio_is_ready();             // z.B. Serial2 ok / optional Handshake

// High-level API (von GUI genutzt)
void radio_send_connect();
void radio_send_disconnect();
void radio_send_preset(const String& preset);
void radio_send_mode(const String& mode);
void radio_send_freq(uint32_t hz);
void radio_send_rx_freq(uint32_t hz);

// Optional: Zugriff aufs letzte RX / Status
String radio_last_rx_line();
uint32_t radio_last_tx_ms();

void radio_send_raw(const String& core); // core ohne EOL
void radio_query_rxfreq();
// Queries (optional)
void radio_query_rx_tx_freq();
void radio_query_mode();
void radio_query_presetpage();