#include "ui.h"
#include "display.h"   // displayMenuNext/Prev, displaySetMode, displaySetConnected, displaySetFrequencyHz, etc.

// -------- Main Menu Items --------
enum class MainItem : uint8_t { Freq = 0, Mode = 1, Preset = 2, Conn = 3 };

enum class UiState : uint8_t {
  MainMenu,
  TuneFreq,
  ModeMenu,
  PresetMenu
};

static UiState st = UiState::MainMenu;

// Dummy Daten
static uint32_t freqHz = 14074000UL;
static bool connected = false;
static uint8_t modeIndex = 0;
static uint8_t presetIndex = 0;

// Untermenüs (Dummy Labels)
static const char* MODE_ITEMS[]   = {"CW", "USB", "LSB", "AM", "FM"};
static const uint8_t MODE_COUNT   = sizeof(MODE_ITEMS)/sizeof(MODE_ITEMS[0]);

static const char* PRESET_ITEMS[] = {"P1 7.030", "P2 14.074", "P3 28.400", "P4 145.500"};
static const uint8_t PRESET_COUNT = sizeof(PRESET_ITEMS)/sizeof(PRESET_ITEMS[0]);

// UI-Auswahlzustände
static MainItem mainSel = MainItem::Freq;
static uint8_t subSel = 0; // Auswahl in ModeMenu/PresetMenu

// Tuning Step (Dummy)
static uint32_t stepHz = 100UL;

// Hilfsfunktionen: Display-Menütext umschalten (optional, je nach deinem Display-API)
static void showMainMenu() {
  // Falls du im Display die Labels dynamisch setzen willst:
  // displaySetMenuLabels({"Freq","Mode","Preset","Conn"}, 4);
  // Fürs Erste reicht: du hast die Labels fest im Display.
  Serial.println("[UI] MainMenu");
}

static void showModeMenu() {
  Serial.println("[UI] ModeMenu");
  Serial.print("     Selected: ");
  Serial.println(MODE_ITEMS[subSel]);
}

static void showPresetMenu() {
  Serial.println("[UI] PresetMenu");
  Serial.print("     Selected: ");
  Serial.println(PRESET_ITEMS[subSel]);
}

static void enterState(UiState next) {
  st = next;

  switch (st) {
    case UiState::MainMenu:
      showMainMenu();
      break;
    case UiState::TuneFreq:
      Serial.println("[UI] TuneFreq (rotate changes frequency, long press back)");
      break;
    case UiState::ModeMenu:
      subSel = 0;
      showModeMenu();
      break;
    case UiState::PresetMenu:
      subSel = 0;
      showPresetMenu();
      break;
  }
}

// Mapping: mainSel <-> display menu index
static uint8_t mainSelIndex() { return static_cast<uint8_t>(mainSel); }
static void setMainSelFromIndex(uint8_t idx) { mainSel = static_cast<MainItem>(idx % 4); }

// Wenn du im Display schon menu_index nutzt, dann sollte mainSel damit synchron sein.
// Hier gehen wir davon aus: displayMenuNext/Prev ändern das visuelle Highlight.
// mainSel halten wir parallel aktuell.
static void mainNext() {
  uint8_t idx = (mainSelIndex() + 1) % 4;
  setMainSelFromIndex(idx);
  displayMenuNext();
}
static void mainPrev() {
  uint8_t idx = (mainSelIndex() == 0) ? 3 : (mainSelIndex() - 1);
  setMainSelFromIndex(idx);
  displayMenuPrev();
}

static void subNext(uint8_t count) {
  subSel = (subSel + 1) % count;
}
static void subPrev(uint8_t count) {
  subSel = (subSel == 0) ? (count - 1) : (subSel - 1);
}

// Actions (noch Dummy -> Serial)
static void actionToggleConn() {
  connected = !connected;
  displaySetConnected(connected);
  Serial.print("[ACTION] Conn -> ");
  Serial.println(connected ? "connected" : "disconnected");
}

static void actionApplyMode(uint8_t idx) {
  Serial.print("[ACTION] Mode -> ");
  Serial.println(MODE_ITEMS[idx]);

  // Hier später echtes Mapping auf RadioMode
  // z.B. displaySetMode(RadioMode::A1A) ...
}

static void actionApplyPreset(uint8_t idx) {
  Serial.print("[ACTION] Preset -> ");
  Serial.println(PRESET_ITEMS[idx]);

  // Dummy: setze Frequenz grob je nach Preset
  if (idx == 0) freqHz = 7030000UL;
  if (idx == 1) freqHz = 14074000UL;
  if (idx == 2) freqHz = 28400000UL;
  if (idx == 3) freqHz = 145500000UL;

  displaySetFrequencyHz(freqHz);
}

static void tuneStep(int8_t steps) {
  // steps: +/-
  int64_t f = (int64_t)freqHz + (int64_t)steps * (int64_t)stepHz;

  // Grenzen (Dummy): 1.5 kHz .. 30 MHz (du kannst hier deine echten Grenzen setzen)
  if (f < 1500) f = 1500;
  if (f > 30000000LL) f = 30000000LL;

  freqHz = (uint32_t)f;
  displaySetFrequencyHz(freqHz);

  Serial.print("[TUNE] Freq = ");
  Serial.print(freqHz);
  Serial.println(" Hz");
}

void ui_init() {
  enterState(UiState::MainMenu);
  displaySetFrequencyHz(freqHz);
  displaySetConnected(connected);
}

void ui_handleEncoder(const EncoderEvent& ev) {
  // 1) Drehung
  if (ev.steps != 0) {
    int8_t s = ev.steps;

    switch (st) {
      case UiState::MainMenu:
        // Menü bewegen
        while (s > 0) { mainNext(); s--; }
        while (s < 0) { mainPrev(); s++; }
        break;

      case UiState::TuneFreq:
        tuneStep(s);
        break;

      case UiState::ModeMenu:
        while (s > 0) { subNext(MODE_COUNT); s--; }
        while (s < 0) { subPrev(MODE_COUNT); s++; }
        showModeMenu();
        break;

      case UiState::PresetMenu:
        while (s > 0) { subNext(PRESET_COUNT); s--; }
        while (s < 0) { subPrev(PRESET_COUNT); s++; }
        showPresetMenu();
        break;
    }
  }

  // 2) Button
  if (ev.button == EncButtonEvent::Click) {
    switch (st) {
      case UiState::MainMenu:
        // Je nach Auswahl in Zustand wechseln / Aktion
        if (mainSel == MainItem::Freq) {
          enterState(UiState::TuneFreq);
        } else if (mainSel == MainItem::Mode) {
          enterState(UiState::ModeMenu);
        } else if (mainSel == MainItem::Preset) {
          enterState(UiState::PresetMenu);
        } else if (mainSel == MainItem::Conn) {
          actionToggleConn();          // sofort ausführen
          enterState(UiState::MainMenu); // bleibt im Menü
        }
        break;

      case UiState::TuneFreq:
        // kurzer Klick im TuneFreq könnte z.B. Step wechseln (optional)
        Serial.println("[UI] Click in TuneFreq (optional: change step)");
        break;

      case UiState::ModeMenu:
        actionApplyMode(subSel);
        enterState(UiState::MainMenu);  // zurück
        break;

      case UiState::PresetMenu:
        actionApplyPreset(subSel);
        enterState(UiState::MainMenu);  // zurück
        break;
    }
  }
  else if (ev.button == EncButtonEvent::LongPress) {
    switch (st) {
      case UiState::TuneFreq:
      case UiState::ModeMenu:
      case UiState::PresetMenu:
        Serial.println("[UI] LongPress -> back to MainMenu");
        enterState(UiState::MainMenu);
        break;
      case UiState::MainMenu:
        // optional: nix oder “Schnellfunktion”
        break;
    }
  }
}
