#include "radio_link.h"
#include "config.h"
#include "app_state.h"

static HardwareSerial& R = Serial2;

// --- TX Queue (ring buffer) ---
static const int QSIZE = 16;
static String q[QSIZE];
static volatile int qHead = 0;
static volatile int qTail = 0;

static uint32_t lastTxMs = 0;
static String rxLine;
static String lastRx;
static bool ready = false;

static bool q_is_empty() { return qHead == qTail; }
static bool q_is_full()  { return ((qTail + 1) % QSIZE) == qHead; }

static bool q_push(const String& s) {
  if (q_is_full()) return false;
  q[qTail] = s;
  qTail = (qTail + 1) % QSIZE;
  return true;
}

static bool q_pop(String& out) {
  if (q_is_empty()) return false;
  out = q[qHead];
  qHead = (qHead + 1) % QSIZE;
  return true;
}

// --- Command builder (Dummy / Platzhalter) ---
// TODO: hier später echtes Protokoll bauen
static String buildCommand_connect()   { return "CONNECT\n"; }
static String buildCommand_disconnect(){ return "DISCONNECT\n"; }
static String buildCommand_preset(const String& p){ return "PRESET " + p + "\n"; }
static String buildCommand_mode(const String& m){ return "MODE " + m + "\n"; }
static String buildCommand_freq(uint32_t hz){ return "FREQ " + String(hz) + "\n"; }

// --- Send helper ---
static void radio_enqueue(const String& cmd) {
  if (!q_push(cmd)) {
    if (RADIO_DEBUG_MIRROR) Serial.println("[RADIO] TX queue full! command dropped.");
  }
}

void radio_init() {
  // Serial2 starten
  R.begin(RADIO_BAUD, SERIAL_8N1, RADIO_RX_PIN, RADIO_TX_PIN);
  ready = true;

  rxLine.reserve(128);
  lastRx.reserve(128);

  if (RADIO_DEBUG_MIRROR) {
    Serial.println("[RADIO] Serial2 initialized.");
    Serial.printf("[RADIO] RX pin=%d TX pin=%d baud=%lu\n", RADIO_RX_PIN, RADIO_TX_PIN, (unsigned long)RADIO_BAUD);
  }
}

bool radio_is_ready() {
  return ready;
}

uint32_t radio_last_tx_ms() {
  return lastTxMs;
}

String radio_last_rx_line() {
  return lastRx;
}

// --- RX parser: line-based (\n). Robust bei \r\n ---
static void radio_read_rx() {
  while (R.available() > 0) {
    char c = (char)R.read();
    if (c == '\r') continue;

    if (c == '\n') {
      if (rxLine.length() > 0) {
        lastRx = rxLine;
        if (RADIO_DEBUG_MIRROR) Serial.println("[RADIO RX] " + lastRx);

        // TODO: hier später parse & g_state updaten (freq/mode/status)
        rxLine = "";
      }
      continue;
    }

    // begrenzen, damit kein RAM weg läuft
    if (rxLine.length() < 200) rxLine += c;
  }
}

// --- TX sender: mit Gap, nicht-blockierend ---
static void radio_flush_tx() {
  if (q_is_empty()) return;

  uint32_t now = millis();
  if (now - lastTxMs < RADIO_TX_GAP_MS) return;

  String cmd;
  if (!q_pop(cmd)) return;

  R.print(cmd);
  lastTxMs = now;

  if (RADIO_DEBUG_MIRROR) Serial.print("[RADIO TX] " + cmd);
}

void radio_loop() {
  if (!ready) return;
  radio_read_rx();
  radio_flush_tx();
}

// --- High-level API used by web_ui ---
void radio_send_connect() {
  radio_enqueue(buildCommand_connect());
}

void radio_send_disconnect() {
  radio_enqueue(buildCommand_disconnect());
}

void radio_send_preset(const String& preset) {
  radio_enqueue(buildCommand_preset(preset));
}

void radio_send_mode(const String& mode) {
  radio_enqueue(buildCommand_mode(mode));
}

void radio_send_freq(uint32_t hz) {
  radio_enqueue(buildCommand_freq(hz));
}
