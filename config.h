#pragma once
#include <Arduino.h>

#include <string>
#include <cstdint>

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

static const bool RADIO_DEBUG_MIRROR = true; 
static const bool RADIO_STATE_MIRROR = true;

enum class RadioMode : uint8_t {
  CW,
  USB,
  LSB,
  AM,
  FM,
  UNKNOWN
};

String radio_mode_to_string(RadioMode mode);

enum class RadioState : uint8_t { BOOT, WAIT_OPEN_ACK, COM_PORT_IS_OPEN, WAIT_CONNECT_ACK, WAIT_DISCONNECT_ACK, READY, WAIT_SET_MODE_ACK };

String radio_state_to_string(RadioState state);


//--------------------------------------------------
// Global Radio State
//--------------------------------------------------


struct GlobalRadioState {
  RadioState state = RadioState::BOOT;
  RadioMode mode = RadioMode::UNKNOWN;
  RadioMode desired_mode = RadioMode::UNKNOWN;
  String mode_str = "USB";

  uint32_t freq_hz = 1500;
  bool radio_connected = false;
  
  String preset = "Plain";   // "Plain" oder "1".."9"

  bool tuneMarker = false;  
  uint8_t tuneCursor = 2; // default 1 KHZ
  bool tuneSelect = false;
  
  static constexpr uint8_t MENU_MAX = 4;
  const char* menu[MENU_MAX] = {"Freq", "Mode", "Preset", "Conn"};
  uint8_t menu_count = 4;
  uint8_t menu_index = 0;
};

extern GlobalRadioState global_radio_state;


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
