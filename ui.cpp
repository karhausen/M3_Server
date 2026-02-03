#include <Arduino.h>
#include "ui.h"
#include "display.h"
#include "radio_link.h"

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

enum class TuneCursor : uint8_t {
  MHZ,     // 1 MHz
  KHZ100,
  KHZ,     // 1 kHz
  HZ100,   // 100 Hz
  Hz
};

static bool tune_select = false;     // false=tune, true=cursor-select
static uint8_t tune_step_idx = 2;    // default: 1 kHz (Index 2)

static uint32_t stepHzFromIdx(uint8_t idx) {
  switch (idx) {
    case 0: return 1000000UL; // 1 MHz
    case 1: return 100000UL;  // 100 kHz
    case 2: return 1000UL;    // 1 kHz
    case 3: return 100UL;     // 100 Hz
    case 4: return 1UL;       // 1 Hz
    default: return 1000UL;
  }
}

static TuneCursor tuneCursor = TuneCursor::KHZ; // default im Tune-Mode

static UiState st = UiState::MainMenu;

// "Model" (Dummy Daten)
static uint32_t freqHz = 14074000UL;
static bool connected = false;
static RadioMode activeMode = global_radio_state.mode; // RadioMode::CW; // default

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
      setFooterMain();
      displaySetTuneMarker(true);

      tune_select = false;
      tune_step_idx = 2;               // 1 kHz Start
      displaySetTuneCursor(tune_step_idx);
      displaySetTuneSelect(false);     // (neu, siehe Display-Teil unten)

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
  Serial.print("[ACTION] Conn -> ");
  if (global_radio_state.radio_connected){
    radio_send_disconnect();
  } else
  {
    radio_send_connect();
  }
  delay(2000);
  // displaySetConnected(global_radio_state.radio_connected);
  Serial.println(global_radio_state.radio_connected ? "connected" : "disconnected");
}

// Dummy Action: Mode setzen
static void actionSetModeFromIndex(uint8_t idx) {
  // RadioMode newMode = RadioMode::UNKNOWN;
  const char* name = "----";

  switch (idx) {
    case 0: global_radio_state.desired_mode = RadioMode::CW;  name = "CW";  break;
    case 1: global_radio_state.desired_mode = RadioMode::USB; name = "USB"; break;
    case 2: global_radio_state.desired_mode = RadioMode::LSB; name = "LSB"; break;
    case 3: global_radio_state.desired_mode = RadioMode::AM;  name = "AM";  break;
    default: break;
  }

  // activeMode = newMode;
  // displaySetMode(newMode);
  radio_send_mode(name);
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

static uint32_t stepForCursor(TuneCursor c) {
  switch (c) {
    case TuneCursor::MHZ:  return 1000000UL;
    case TuneCursor::KHZ:  return 1000UL;
    case TuneCursor::HZ100:return 100UL;
    default:               return 1000UL;
  }
}

static void tuneCursorNext() {
  // Zyklus: MHZ -> KHZ -> HZ100 -> MHZ ...
  if (tuneCursor == TuneCursor::MHZ) tuneCursor = TuneCursor::KHZ;
  else if (tuneCursor == TuneCursor::KHZ) tuneCursor = TuneCursor::HZ100;
  else tuneCursor = TuneCursor::MHZ;

  Serial.print("[TUNE] Cursor -> ");
  Serial.println(
    tuneCursor == TuneCursor::MHZ ? "MHz" :
    tuneCursor == TuneCursor::KHZ ? "kHz" : "100Hz"
  );

  // Display muss wissen, was unterstrichen werden soll:
  displaySetTuneCursor((uint8_t)tuneCursor);
}

static void tuneCursorMove(int8_t steps) {
  // wrap 0..4
  int16_t idx = (int16_t)tune_step_idx + steps;
  while (idx < 0) idx += 5;
  idx %= 5;
  tune_step_idx = (uint8_t)idx;

  // Display Unterstreichung updaten:
  displaySetTuneCursor(tune_step_idx);
  Serial.print("[TUNE] Cursor step -> idx=");
  Serial.print(tune_step_idx);
  Serial.print(" stepHz=");
  Serial.println(stepHzFromIdx(tune_step_idx));
}

static void tuneBySteps(int8_t steps) {
  if (steps == 0) return;

  uint32_t step = stepHzFromIdx(tune_step_idx);
  int64_t f = (int64_t)freqHz + (int64_t)steps * (int64_t)step;

  if (f < (int64_t)FREQ_MIN_HZ) f = FREQ_MIN_HZ;
  if (f > (int64_t)FREQ_MAX_HZ) f = FREQ_MAX_HZ;

  freqHz = (uint32_t)f;
  displaySetFrequencyHz(freqHz);

  Serial.print("[TUNE] step=");
  Serial.print(step);
  Serial.print(" Hz  Freq=");
  Serial.println(freqHz);
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
        if (tune_select) {
          // Cursor schieben
          int16_t i = (int16_t)tune_step_idx + ev.steps;
          while (i < 0) i += 5;
          i %= 5;
          tune_step_idx = (uint8_t)i;
          displaySetTuneCursor(tune_step_idx);
        } else {
          // Tunen
          uint32_t step = stepHzFromIdx(tune_step_idx);
          tuneBySteps(ev.steps);
        }
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
          tuneCursor = TuneCursor::KHZ;   // ???
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
        // toggle zwischen Cursor-Select und Tunen
        tune_select = !tune_select;
        displaySetTuneSelect(tune_select);

        Serial.print("[TUNE] ");
        Serial.println(tune_select ? "Cursor-Select ON" : "Tune ON");
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
