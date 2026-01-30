#include "display.h"
#include "config_display.h"

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>


// -------- Display Objekt --------
// Adafruit_SH1107 erwartet Breite/Höhe und &Wire
static Adafruit_SH1107 display(OLED_W, OLED_H, &Wire, -1);

static bool uiDirty = true;          // beim Start einmal zeichnen
static uint32_t lastRenderMs = 0;

static inline void markDirty() {
  uiDirty = true;
}


static const char* modeToText(RadioMode m) {
  switch (m) {
    case RadioMode::A1A:      return "[A1A]";
    case RadioMode::J3E_PLUS: return "[J3E+]";
    case RadioMode::J3E_MINUS:return "[J3E-]";
    default:                 return "[----]";
  }
}

struct UiState {
  RadioMode mode = RadioMode::UNKNOWN;
  bool connected = false;
  uint32_t freq_hz = 1500;

  static constexpr uint8_t MENU_MAX = 4;
  const char* menu[MENU_MAX] = {"RX", "TX", "STEP", "SET"};
  uint8_t menu_count = 4;
  uint8_t menu_index = 0;
};

static UiState ui;

enum class FreqUnit : uint8_t {
  KHZ,
  MHZ
};

// ---------- Helpers ----------


void formatFrequency(char* value,
                     size_t valueSize,
                     FreqUnit& unit,
                     uint32_t hz)
{
  if (hz < 30000000UL) {          // < 30 MHz → kHz
    unit = FreqUnit::KHZ;

    uint32_t whole = hz / 1000UL;
    uint32_t frac  = hz % 1000UL;

    snprintf(value, valueSize,
             "%lu.%03lu",
             (unsigned long)whole,
             (unsigned long)frac);
  } else {                        // ≥ 30 MHz → MHz
    unit = FreqUnit::MHZ;

    uint32_t whole = hz / 1000000UL;
    uint32_t frac  = (hz % 1000000UL) / 1000UL;

    snprintf(value, valueSize,
             "%lu.%03lu",
             (unsigned long)whole,
             (unsigned long)frac);
  }
}


// Textbreite in Pixeln (bei aktueller TextSize)
static int16_t textWidthPx(const char* s) {
  int16_t x1, y1;
  uint16_t w, h;
  display.getTextBounds(s, 0, 0, &x1, &y1, &w, &h);
  return (int16_t)w;
}

// ---------- Zeichnen ----------
static void drawHeader(const UiState& s) {
  display.setTextSize(1);
  display.setTextColor(SH110X_WHITE);
  display.setCursor(0, 4);
  display.print(modeToText(s.mode));

  const char* conn = s.connected ? "[connected]" : "[disconnected]";
  int16_t w = textWidthPx(conn);
  display.setCursor(OLED_W - w, 4);
  display.print(conn);

  display.drawLine(0, UI_HEADER_H - 1, OLED_W - 1, UI_HEADER_H - 1, SH110X_WHITE);
}


static void drawFrequency(const UiState& s) {
  char value[16];
  FreqUnit unit;

  formatFrequency(value, sizeof(value), unit, s.freq_hz);

  // ---------- große Zahl ----------
  display.setTextColor(SH110X_WHITE);
  display.setTextSize(2);

  int16_t valueW = textWidthPx(value);
  int16_t x = (OLED_W - valueW) / 2;

  int mainTop = UI_HEADER_H;
  int mainBottom = OLED_H - UI_FOOTER_H;
  int mainH = mainBottom - mainTop;

  int16_t valueH = 16; // TextSize 2 ≈ 16px
  int16_t y = mainTop + (mainH - valueH) / 2;

  display.setCursor(x, y);
  display.print(value);

  // ---------- Einheit ----------
  display.setTextSize(1);

  const char* unitStr = (unit == FreqUnit::KHZ) ? "kHz" : "MHz";

  int16_t ux = x + valueW + 4;   // kleiner Abstand rechts
  int16_t uy = y + 8;            // optisch mittig zur Zahl

  // display.setCursor(ux, uy);
  display.setCursor(OLED_W - 24, UI_HEADER_H + 12);
  display.print(unitStr);
}


static void drawFooterMenu(const UiState& s) {
  int yTop = OLED_H - UI_FOOTER_H;
  display.drawLine(0, yTop, OLED_W - 1, yTop, SH110X_WHITE);

  display.setTextSize(1);

  uint8_t n = s.menu_count;
  if (n == 0) return;

  int slotW = OLED_W / n;

  for (uint8_t i = 0; i < n; i++) {
    const char* label = s.menu[i];
    int16_t w = textWidthPx(label);

    int xCenter = i * slotW + slotW / 2;
    int x = xCenter - w / 2;
    int y = yTop + 8; // mittig im Footer

    if (i == s.menu_index) {
      // Highlight: gefülltes Rechteck + schwarzer Text
      int padX = 4, padY = 2;
      int boxW = w + padX * 2;
      int boxH = 12 + padY * 2;

      int bx = x - padX;
      int by = y - padY;

      display.fillRoundRect(bx, by, boxW, boxH, 2, SH110X_WHITE);
      display.setTextColor(SH110X_BLACK);
      display.setCursor(x, y);
      display.print(label);
      display.setTextColor(SH110X_WHITE);
    } else {
      display.setCursor(x, y);
      display.print(label);
    }
  }
}

// ---------- Public API ----------
bool displayInit() {
  Wire.begin(OLED_SDA, OLED_SCL);

  if (!display.begin(OLED_ADDR, true)) { // true = reset (soft)
    return false;
  }
  
  display.setRotation(OLED_ROTATION);  // 90° im Uhrzeigersinn

  display.clearDisplay();
  display.display();
  return true;
}

void displaySetConnected(bool connected) {
  if (ui.connected == connected) return;
  ui.connected = connected;
  markDirty();
}

void displaySetMode(RadioMode mode) {
  if (ui.mode == mode) return;
  ui.mode = mode;
  markDirty();
}

void displaySetFrequencyHz(uint32_t hz) {
  if (ui.freq_hz == hz) return;
  ui.freq_hz = hz;
  markDirty();
}

void displayMenuNext() {
  if (ui.menu_count == 0) return;
  uint8_t next = (ui.menu_index + 1) % ui.menu_count;
  if (ui.menu_index == next) return;
  ui.menu_index = next;
  markDirty();
}

void displayMenuPrev() {
  if (ui.menu_count == 0) return;
  uint8_t prev = (ui.menu_index == 0) ? (ui.menu_count - 1) : (ui.menu_index - 1);
  if (ui.menu_index == prev) return;
  ui.menu_index = prev;
  markDirty();
}

void displayRender() {
  display.clearDisplay();
  drawHeader(ui);
  drawFrequency(ui);     // bzw. drawFrequency__ bei dir
  drawFooterMenu(ui);
  display.display();

  uiDirty = false;
  lastRenderMs = millis();
}


void displayTick() {
  uint32_t now = millis();

  // optional: Schutz gegen zu häufiges Rendern
  if (uiDirty && (now - lastRenderMs >= UI_MIN_REFRESH_MS)) {
    displayRender();
  }
}

static constexpr uint32_t UI_MAX_STALE_MS = 3000;

void displayTickPeriodic() {
  uint32_t now = millis();

  bool stale = (now - lastRenderMs) > UI_MAX_STALE_MS;

  if ((uiDirty || stale) && (now - lastRenderMs >= UI_MIN_REFRESH_MS)) {
    displayRender();
  }
}

void displayForceRefresh() {
  markDirty();
}

