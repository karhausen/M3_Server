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