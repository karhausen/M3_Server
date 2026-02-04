#pragma once
#include <Arduino.h>
#include "config.h"
#include "encoder.h"   // EncButtonEvent, EncoderEvent

void ui_init();
void ui_handleEncoder(const EncoderEvent& ev);
