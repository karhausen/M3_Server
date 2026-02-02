#pragma once
#include <Arduino.h>

enum class EncButtonEvent : uint8_t { None, Click, LongPress };

struct EncoderEvent {
  int8_t steps;              // -n / +n (Rastungen, nicht raw ticks)
  EncButtonEvent button;     // Click/LongPress
};

void encoderInit();
EncoderEvent encoderPoll();  // ersetzt encoder_loop()
