#include <Arduino.h>
#include "ui.h"
#include "display.h"

// -------------------- Konfiguration --------------------
static constexpr uint32_t FREQ_MIN_HZ = 1500UL;
static constexpr uint32_t FREQ_MAX_HZ = 30000000UL;

// Tuning Step (Dummy) – später aus Menü/Setting
static uint32_t stepHz = 100UL;

// -------------------- Menü-Label-Sets --------------------
static const char* MAIN_LABELS[4]   = {"Freq", "Mode", "Preset", "Conn"};
static const char* MODE_LABELS[4]   = {"CW", "USB", "LSB", "AM"};
static const char* PRESET_LABELS[4] = {"P1", "P2", "P3", "P4"};

// -------------------- State Machine --------------------
enum class UiState : uint8_t {
  MainMenu,
  TuneFreq,
  ModeMenu,
  PresetMenu
};

enum class MainItem : uint8_t {
  Freq = 0,
  Mode = 1,
  Preset = 2,
  Conn = 3
};

static UiState st = UiState::MainMenu;

// "Model" (Dummy Daten)
static uint32_t freqHz = 14074000UL;
static bool connected = false;
static RadioMode activeMode = RadioMode::CW; // default

// -------------------- Helper --------------------
static void setFooterMain() {
  displaySetMenuLabels(MAIN_LABELS, 4);
}

static void setFooterMode() {
  displaySetMenuLabels(MODE_LABELS, 4);
}

static void setFooterPreset() {
  displaySetMenuLabels(PRESET_LABELS, 4);
}

static void enterState(UiState next) {
  st = next;

  switch (st) {
    case UiState::MainMenu:
      setFooterMain();
      displaySetTuneMarker(false);
      // menuIndex bleibt wie er ist (oder du setzt ihn bewusst)
      Serial.println("[UI] State -> MainMenu");
      break;

    case UiState::TuneFreq:
      // Footer darf ruhig Main-Menü bleiben (wie du wolltest)
      setFooterMain();
      displaySetTuneMarker(true);
      Serial.println("[UI] State -> TuneFreq");
      break;

    case UiState::ModeMenu:
      setFooterMode();
      displaySetTuneMarker(false);
      displaySetMenuIndex(0);
      Serial.println("[UI] State -> ModeMenu");
      break;

    case UiState::PresetMenu:
      setFooterPreset();
      displaySetTuneMarker(false);
      displaySetMenuIndex(0);
      Serial.println("[UI] State -> PresetMenu");
      break;
  }
}

// Menüindex (0..3) mit Wrap bewegen
static void menuMove(int8_t steps) {
  if (steps == 0) return;
  int16_t idx = (int16_t)displayGetMenuIndex();
  idx += steps;
  while (idx < 0) idx += 4;
  idx %= 4;
  displaySetMenuIndex((uint8_t)idx);
}

// Dummy Action: Connection toggeln
static void actionToggleConn() {
  connected = !connected;
  displaySetConnected(connected);
  Serial.print("[ACTION] Conn -> ");
  Serial.println(connected ? "connected" : "disconnected");
}

// Dummy Action: Mode setzen
static void actionSetModeFromIndex(uint8_t idx) {
  RadioMode newMode = RadioMode::UNKNOWN;
  const char* name = "----";

  switch (idx) {
    case 0: newMode = RadioMode::CW;  name = "CW";  break;
    case 1: newMode = RadioMode::USB; name = "USB"; break;
    case 2: newMode = RadioMode::LSB; name = "LSB"; break;
    case 3: newMode = RadioMode::AM;  name = "AM";  break;
    default: break;
  }

  activeMode = newMode;
  displaySetMode(newMode);

  Serial.print("[ACTION] Mode -> ");
  Serial.println(name);
}

// Dummy Action: Preset anwenden (hier: Frequenz setzen)
static void actionApplyPresetFromIndex(uint8_t idx) {
  // Beispielwerte – anpassen wie du willst
  switch (idx) {
    case 0: freqHz = 7030000UL;   break; // P1
    case 1: freqHz = 14074000UL;  break; // P2
    case 2: freqHz = 28400000UL;  break; // P3
    case 3: freqHz = 10100000UL;  break; // P4
    default: break;
  }

  displaySetFrequencyHz(freqHz);

  Serial.print("[ACTION] Preset -> P");
  Serial.print(idx + 1);
  Serial.print(" (Freq=");
  Serial.print(freqHz);
  Serial.println(" Hz)");
}

// Tune Aktion
static void tuneBySteps(int8_t steps) {
  if (steps == 0) return;

  int64_t f = (int64_t)freqHz + (int64_t)steps * (int64_t)stepHz;
  if (f < (int64_t)FREQ_MIN_HZ) f = FREQ_MIN_HZ;
  if (f > (int64_t)FREQ_MAX_HZ) f = FREQ_MAX_HZ;

  freqHz = (uint32_t)f;
  displaySetFrequencyHz(freqHz);

  Serial.print("[TUNE] Freq = ");
  Serial.print(freqHz);
  Serial.println(" Hz");
}

// -------------------- Public API --------------------
void ui_init() {
  // Initiale Anzeige
  setFooterMain();
  displaySetMenuIndex(0);
  displaySetConnected(connected);
  displaySetMode(activeMode);
  displaySetFrequencyHz(freqHz);
  displaySetTuneMarker(false);

  Serial.println("[UI] init");
  enterState(UiState::MainMenu);
}

void ui_handleEncoder(const EncoderEvent& ev) {
  // 1) Drehbewegung
  if (ev.steps != 0) {
    switch (st) {
      case UiState::MainMenu:
        menuMove(ev.steps);
        break;

      case UiState::ModeMenu:
        menuMove(ev.steps);
        Serial.print("[UI] Mode select -> ");
        Serial.println(MODE_LABELS[displayGetMenuIndex()]);
        break;

      case UiState::PresetMenu:
        menuMove(ev.steps);
        Serial.print("[UI] Preset select -> ");
        Serial.println(PRESET_LABELS[displayGetMenuIndex()]);
        break;

      case UiState::TuneFreq:
        tuneBySteps(ev.steps);
        break;
    }
  }

  // 2) Button Events
  if (ev.button == EncButtonEvent::Click) {
    switch (st) {
      case UiState::MainMenu: {
        uint8_t idx = displayGetMenuIndex();
        MainItem sel = (MainItem)idx;

        if (sel == MainItem::Freq) {
          enterState(UiState::TuneFreq);
        } else if (sel == MainItem::Mode) {
          enterState(UiState::ModeMenu);
        } else if (sel == MainItem::Preset) {
          enterState(UiState::PresetMenu);
        } else if (sel == MainItem::Conn) {
          actionToggleConn();
          enterState(UiState::MainMenu);
        }
      } break;

      case UiState::TuneFreq:
        // optional: kurzer Klick könnte Step wechseln
        Serial.println("[UI] Click in TuneFreq (optional: step change)");
        break;

      case UiState::ModeMenu: {
        uint8_t idx = displayGetMenuIndex();
        actionSetModeFromIndex(idx);
        enterState(UiState::MainMenu);
      } break;

      case UiState::PresetMenu: {
        uint8_t idx = displayGetMenuIndex();
        actionApplyPresetFromIndex(idx);
        enterState(UiState::MainMenu);
      } break;
    }
  }
  else if (ev.button == EncButtonEvent::LongPress) {
    switch (st) {
      case UiState::TuneFreq:
        Serial.println("[UI] LongPress -> back to MainMenu");
        enterState(UiState::MainMenu);
        break;

      case UiState::ModeMenu:
      case UiState::PresetMenu:
        Serial.println("[UI] LongPress -> back to MainMenu");
        enterState(UiState::MainMenu);
        break;

      case UiState::MainMenu:
        // optional: nix
        break;
    }
  }
}
