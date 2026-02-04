#include "config.h"
// #include <string>

GlobalRadioState global_radio_state;

String radio_mode_to_string(RadioMode mode)
{
    switch (mode) {
        case RadioMode::CW:      return "CW";
        case RadioMode::USB:     return "USB";
        case RadioMode::LSB:     return "LSB";
        case RadioMode::AM:      return "AM";
        case RadioMode::FM:      return "FM";
        case RadioMode::UNKNOWN: return "UNKNOWN";
        default:                 return "INVALID";
    }
}

// BOOT, WAIT_OPEN_ACK, COM_PORT_IS_OPEN, WAIT_CONNECT_ACK, WAIT_DISCONNECT_ACK, READY, WAIT_SET_MODE_ACK

String radio_state_to_string(RadioState state)
{
  switch (state)
  {
    case RadioState::BOOT:                return F("BOOT");
    case RadioState::WAIT_OPEN_ACK:       return F("WAIT_OPEN_ACK");
    case RadioState::COM_PORT_IS_OPEN:    return F("COM_PORT_IS_OPEN");
    case RadioState::WAIT_CONNECT_ACK:    return F("WAIT_CONNECT_ACK");
    case RadioState::WAIT_DISCONNECT_ACK: return F("WAIT_DISCONNECT_ACK");
    case RadioState::READY:               return F("READY");
    case RadioState::WAIT_SET_MODE_ACK:   return F("WAIT_SET_MODE_ACK");
    default:                              return F("UNKNOWN");
  }
}