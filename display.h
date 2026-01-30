#pragma once
#include <Arduino.h>

enum class RadioMode : uint8_t {
  A1A,
  J3E_PLUS,
  J3E_MINUS,
  UNKNOWN
};


bool displayInit();
void displayTick();

void displaySetConnected(bool connected);
void displaySetMode(RadioMode mode);
void displaySetFrequencyHz(uint32_t hz);

void displayMenuNext();
void displayMenuPrev();
