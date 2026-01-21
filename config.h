#pragma once
#include <Arduino.h>

// Fallback AP
static const char* AP_SSID  = "RadioRemote-ESP32";
static const char* AP_PASS  = "12345678";   // min. 8 Zeichen
static const uint8_t AP_CH  = 6;
static const bool AP_HIDE   = false;

static const uint32_t STA_TIMEOUT_MS = 10'000;
static const bool KEEP_AP_ALSO_WHEN_STA_OK = false;

// Serial
static const uint32_t SERIAL_BAUD = 115200;

// Serial2 fürs Radio
static const int RADIO_RX_PIN = 16;     // anpassen
static const int RADIO_TX_PIN = 17;     // anpassen
static const uint32_t RADIO_BAUD = 19200; // anpassen
static const uint32_t RADIO_TX_GAP_MS = 30; // Mindestabstand zwischen Commands
static const bool RADIO_DEBUG_MIRROR = false; // send/recv zusätzlich auf Serial0 loggen
static const bool RADIO_STATE_MIRROR = true;

// -------------------------------------------------
// Radio protocol framing
// -------------------------------------------------

// Prefix vor JEDEM Radiobefehl
static const char* RADIO_HEADER = "\nDM:";

// Optionales Suffix (meist CRLF)
static const char* RADIO_FOOTER = "\r";

// Trenner zwischen Befehl und Parameter(n)
static const char* RADIO_DELIMITER = " ";

// Trenner zwischen mehreren Befehlen im selben Frame
static const char  RADIO_CMD_SEPARATOR = ';';
