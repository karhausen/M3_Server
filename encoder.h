#pragma once
#include <Arduino.h>

#include "encoder_config.h"

enum class EncButtonEvent : uint8_t {
  None,
  Click,
  LongPress
};

void encoderInit();

/// Gibt seit letztem Aufruf die Schrittzahl zurück (negativ/positiv)
int16_t encoderGetAndClearDelta();

/// Button-Events (entprellt)
EncButtonEvent encoderGetButtonEvent();
