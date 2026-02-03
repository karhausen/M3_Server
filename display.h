#pragma once
#include <Arduino.h>
#include "config.h"


bool displayInit();
void displayTick();

// Status im Header
void displaySetConnected(bool connected);
void displaySetMode(RadioMode mode);

// Main Value
void displaySetFrequencyHz(uint32_t hz);

void displaySetTuneCursor(uint8_t idx);   // 0..4
void displaySetTuneSelect(bool on);       // Cursor-Select aktiv?


// Footer-Menü
void displaySetMenuLabels(const char* const* labels, uint8_t count);
void displaySetMenuIndex(uint8_t index);
uint8_t displayGetMenuIndex();

// Marker
void displaySetTuneMarker(bool on);

// void displayMenuNext();
// void displayMenuPrev();
