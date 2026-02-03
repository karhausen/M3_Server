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

uint8_t tuneCursor = 2;   // 1kHz default
bool tuneSelect = false;


static inline void markDirty() {
  uiDirty = true;
}

static const char* modeToText(RadioMode m) {
  switch (m) {
    case RadioMode::CW:  return "[CW]";
    case RadioMode::USB: return "[USB]";
    case RadioMode::LSB: return "[LSB]";
    case RadioMode::AM:  return "[AM]";
    default:             return "[----]";
  }
}

struct UiState {
  RadioMode mode = RadioMode::UNKNOWN;
  uint32_t freq_hz = 1500;
  bool connected = false;

  bool tuneMarker = false;  
  uint8_t tuneCursor = 2; // default 1 KHZ
  bool tuneSelect = false;
  
  static constexpr uint8_t MENU_MAX = 4;
  const char* menu[MENU_MAX] = {"Freq", "Mode", "Preset", "Conn"};
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

  // Mode links
  display.setCursor(0, 4);
  display.print(modeToText(s.mode));

  // Connection rechts
  const char* conn = s.connected ? "[connected]" : "[disconnected]";
  int16_t w = textWidthPx(conn);
  display.setCursor(OLED_W - w, 4);
  display.print(conn);

  display.drawLine(0, UI_HEADER_H - 1, OLED_W - 1, UI_HEADER_H - 1, SH110X_WHITE);
}

static void drawTuneMarker(const UiState& s) {
  if (!s.tuneMarker) return;

  display.setTextSize(1);
  display.setTextColor(SH110X_WHITE);

  display.setCursor(0, UI_HEADER_H + 2);
  display.print("TUNE");

  if (s.tuneSelect) {
    display.setCursor(30, UI_HEADER_H + 2);
    display.print("SEL");   // oder "CUR"
  }
}

static void underlineRange(int16_t x, int16_t y, int16_t w) {
  int16_t uy = y + 18; // bei TextSize 2 meist passend
  display.drawLine(x, uy, x + w - 1, uy, SH110X_WHITE);
}


void displaySetTuneCursor(uint8_t idx) {
  if (ui.tuneCursor == idx) return;
  ui.tuneCursor = idx;
  markDirty();
}

void displaySetTuneSelect(bool on) {
  if (ui.tuneSelect == on) return;
  ui.tuneSelect = on;
  markDirty();
}

static void formatKHz3(char* out, size_t outSize, uint32_t hz) {
  uint32_t whole = hz / 1000UL; // kHz
  uint32_t frac  = hz % 1000UL; // Hz -> .xxx kHz
  snprintf(out, outSize, "%lu.%03lu",
           (unsigned long)whole, (unsigned long)frac);
}


static void formatMHz6(char* out, size_t outSize, uint32_t hz) {
  uint32_t whole = hz / 1000000UL;
  uint32_t frac  = hz % 1000000UL; // .xxxxxx MHz
  snprintf(out, outSize, "%lu.%06lu",
           (unsigned long)whole, (unsigned long)frac);
}

static void drawFrequency(const UiState& s) {
  char value[16];
  FreqUnit unit;
  const bool useMHz = (s.freq_hz >= 30000000UL);
  const char* unitStr = nullptr;


  if (useMHz) {
    formatMHz6(value, sizeof(value), s.freq_hz);
    unitStr = "MHz";
  } else {
    formatKHz3(value, sizeof(value), s.freq_hz);
    unitStr = "kHz";
  }

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

  int16_t ux = x + valueW + 4;   // kleiner Abstand rechts
  int16_t uy = y + 8;            // optisch mittig zur Zahl

  // display.setCursor(ux, uy);
  display.setCursor(OLED_W - 24, UI_HEADER_H + 2);
  display.print(unitStr);

  // Cursor-Unterstreichung (nur wenn Tune aktiv)
  if (s.tuneMarker) {
  const char* dot = strchr(value, '.');
  if (dot) {
    // Split links und rechts
    char left[16] = {0};
    size_t leftLen = (size_t)(dot - value);
    if (leftLen >= sizeof(left)) leftLen = sizeof(left) - 1;
    memcpy(left, value, leftLen);
    left[leftLen] = 0;

    char point[2] = {'.', 0};

    // Rechte Seite (Nachkommastellen)
    const char* frac = dot + 1;
    int fracLen = (int)strlen(frac);

    display.setTextSize(2);

    int16_t w_left  = textWidthPx(left);
    int16_t w_point = textWidthPx(point);

    // ---- kHz oder MHz? anhand Nachkommastellenlänge entscheiden ----
    // kHz3 => fracLen==3, MHz6 => fracLen==6
    if (fracLen == 3) {
      // kHz: value = "<kHz>.<hhh>"
      // idx: 0=1MHz,1=100kHz,2=1kHz,3=100Hz,4=1Hz

      int L = (int)leftLen;

      // idx0: MHz-Teil = left ohne letzte 3 Stellen (falls vorhanden)
      if (s.tuneCursor == 0) {
        int split = L - 3;
        if (split > 0) {
          char mhzPart[16] = {0};
          memcpy(mhzPart, left, (size_t)split);
          mhzPart[split] = 0;
          int16_t w_mhz = textWidthPx(mhzPart);
          underlineRange(x, y, w_mhz);
        } else {
          // unter 1 MHz: underline ganzen Integerteil
          underlineRange(x, y, w_left);
        }
      }
      // idx1: 100kHz-Ziffer (3. von rechts) -> position L-3
      else if (s.tuneCursor == 1) {
        int pos = L - 3;
        if (pos >= 0 && pos < L) {
          char pre[16] = {0};
          memcpy(pre, left, (size_t)pos);
          pre[pos] = 0;

          char dig[2] = { left[pos], 0 };

          int16_t w_pre = textWidthPx(pre);
          int16_t w_dig = textWidthPx(dig);

          underlineRange(x + w_pre, y, w_dig);
        }
      }
      // idx2: 1kHz-Ziffer (letzte) -> position L-1
      else if (s.tuneCursor == 2) {
        int pos = L - 1;
        if (pos >= 0 && pos < L) {
          char pre[16] = {0};
          memcpy(pre, left, (size_t)pos);
          pre[pos] = 0;

          char dig[2] = { left[pos], 0 };

          int16_t w_pre = textWidthPx(pre);
          int16_t w_dig = textWidthPx(dig);

          underlineRange(x + w_pre, y, w_dig);
        }
      }
      // idx3: 100Hz = 1. Nachkommastelle
      else if (s.tuneCursor == 3) {
        char d0[2] = { frac[0], 0 };
        int16_t w_d0 = textWidthPx(d0);
        underlineRange(x + w_left + w_point, y, w_d0);
      }
      // idx4: 1Hz = 3. Nachkommastelle
      else if (s.tuneCursor == 4) {
        char d0[2] = { frac[0], 0 };
        char d1[2] = { frac[1], 0 };
        char d2[2] = { frac[2], 0 };

        int16_t w_d0 = textWidthPx(d0);
        int16_t w_d1 = textWidthPx(d1);
        int16_t w_d2 = textWidthPx(d2);

        underlineRange(x + w_left + w_point + w_d0 + w_d1, y, w_d2);
      }
    }
    else if (fracLen == 6) {
      // MHz: value = "<MHz>.<ffffff>"
      // idx: 0=1MHz,1=100kHz,2=1kHz,3=100Hz,4=1Hz

      if (s.tuneCursor == 0) {
        underlineRange(x, y, w_left);
      } else {
        // welcher Nachkommastellen-Index?
        int fracIndex = -1;
        if (s.tuneCursor == 1) fracIndex = 0; // 100kHz
        if (s.tuneCursor == 2) fracIndex = 2; // 1kHz
        if (s.tuneCursor == 3) fracIndex = 3; // 100Hz
        if (s.tuneCursor == 4) fracIndex = 5; // 1Hz

        if (fracIndex >= 0 && fracIndex < fracLen) {
          // Breite der Nachkommastellen bis zur Zielstelle aufsummieren
          int16_t w_preFrac = 0;
          for (int i = 0; i < fracIndex; i++) {
            char di[2] = { frac[i], 0 };
            w_preFrac += textWidthPx(di);
          }
          char target[2] = { frac[fracIndex], 0 };
          int16_t w_target = textWidthPx(target);

          underlineRange(x + w_left + w_point + w_preFrac, y, w_target);
        }
      }
    }
  }
}


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

void displaySetTuneMarker(bool on) {
  if (ui.tuneMarker == on) return;
  ui.tuneMarker = on;
  markDirty();
}


void displaySetMenuLabels(const char* const* labels, uint8_t count) {
  if (count > UiState::MENU_MAX) count = UiState::MENU_MAX;

  // Nur markieren wenn sich wirklich was ändert
  bool changed = (ui.menu_count != count);
  ui.menu_count = count;

  for (uint8_t i = 0; i < count; i++) {
    if (ui.menu[i] != labels[i]) changed = true;
    ui.menu[i] = labels[i];
  }

  // Falls count kleiner wurde, rest egal
  if (ui.menu_index >= ui.menu_count) {
    ui.menu_index = 0;
    changed = true;
  }

  if (changed) markDirty();
}

void displaySetMenuIndex(uint8_t index) {
  if (ui.menu_count == 0) return;
  index %= ui.menu_count;
  if (ui.menu_index == index) return;
  ui.menu_index = index;
  markDirty();
}

uint8_t displayGetMenuIndex() {
  return ui.menu_index;
}

void displayRender() {
  display.clearDisplay();

  drawHeader(ui);
  drawTuneMarker(ui);
  drawFrequency(ui);     // bzw. drawFrequency__ bei dir
  drawFooterMenu(ui);

  display.display();

  uiDirty = false;
  lastRenderMs = millis();
}


void displayTick() {
  uint32_t now = millis();
  if (uiDirty && (now - lastRenderMs >= UI_MIN_REFRESH_MS)) {
    displayRender();
    uiDirty = false;
    lastRenderMs = now;
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

