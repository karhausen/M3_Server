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

static String radio_build(const String& command,
                          const String& param = "") {
  String s;
  s.reserve(64);

  s += RADIO_HEADER;     // z.B. "M:"
  s += command;          // z.B. "FF SRF"

  if (param.length()) {
    s += RADIO_DELIMITER; // z.B. " "
    s += param;           // z.B. "1500000"
  }

  s += RADIO_FOOTER;     // z.B. "\r\n"
  return s;
}

// --- Remote control ---
static String cmd_remoteOn()  { return radio_build("REMOTE SENTER2,0"); }
static String cmd_remoteOff() { return radio_build("REMOTE SENTER0"); }

// --- Frequenz ---
static String cmd_getRxFreq() { return radio_build("FF GRF"); }
static String cmd_getTxFreq() { return radio_build("FF GTF"); }

static String cmd_setRxFreq(uint32_t hz) {
  return radio_build("FF SRF", String(hz));
}
static String cmd_setTxFreq(uint32_t hz) {
  return radio_build("FF STF", String(hz));
}

// --- Presets ---
static String cmd_getPresetPage() {
  return radio_build("GR GPRS");
}
static String cmd_setPresetPage(int page) {
  return radio_build("GR SPRS", String(page));
}

// --- Modes ---
static String cmd_modeA1A()  { return radio_build("FF SMD8");  }  // CW
static String cmd_modeA3E()  { return radio_build("FF SMD9");  }  // AM
static String cmd_modeJ3EP() { return radio_build("FF SMD12"); }  // USB
static String cmd_modeJ3EM() { return radio_build("FF SMD15"); }  // LSB
static String cmd_modeF3E()  { return radio_build("FF SMD17"); }  // FM

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

static uint32_t parseLastNumber(const String& s) {
  // nimmt die letzte zusammenhängende Zifferngruppe im String
  int i = s.length() - 1;
  while (i >= 0 && (s[i] < '0' || s[i] > '9')) i--;
  if (i < 0) return 0;
  int end = i;
  while (i >= 0 && (s[i] >= '0' && s[i] <= '9')) i--;
  int start = i + 1;
  return (uint32_t)s.substring(start, end + 1).toInt();
}

// --- RX parser: line-based (\n). Robust bei \r\n ---
static void radio_read_rx() {
  while (R.available() > 0) {
    char c = (char)R.read();
    if (c == '\r') continue;

    if (c == '\n') {
      if (rxLine.length() > 0) {
        lastRx = rxLine;

        

        // ...
        if (lastRx.indexOf("GRF") >= 0) {
          uint32_t hz = parseLastNumber(lastRx);
          if (hz > 0) {
            g_state.freq_hz = hz;
          }
        }
        if (lastRx.indexOf("GPRS") >= 0) {
          uint32_t page = parseLastNumber(lastRx);
          // optional: g_state.preset = ...
        }

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
  radio_enqueue(cmd_remoteOn());
}

void radio_send_disconnect() {
  radio_enqueue(cmd_remoteOff());
}

void radio_send_freq(uint32_t hz) {
  radio_enqueue(cmd_setRxFreq(hz));
}

static int presetToPage(const String& preset) {
  // "Platin" special: ich mappe es erstmal auf 0.
  // 1..9 direkt.
  if (preset.equalsIgnoreCase("Platin")) return 0;
  int p = preset.toInt();
  if (p < 0) p = 0;
  if (p > 9) p = 9;
  return p;
}

void radio_send_preset(const String& preset) {
  radio_enqueue(cmd_setPresetPage(presetToPage(preset)));
}

void radio_send_mode(const String& mode) {
  // GUI liefert "USB"/"LSB"/"CW"
  if (mode == "CW")  radio_enqueue(cmd_modeA1A());
  else if (mode == "USB") radio_enqueue(cmd_modeJ3EP());
  else if (mode == "LSB") radio_enqueue(cmd_modeJ3EM());
  else {
    if (RADIO_DEBUG_MIRROR) Serial.println("[RADIO] Unknown mode: " + mode);
  }
}

void radio_send_raw(const String& core){
  radio_enqueue(radio_build(core));
}
void radio_query_rxfreq(){
  radio_enqueue(cmd_getRxFreq());
}
void radio_query_presetpage(){
  radio_enqueue(cmd_getPresetPage());
}
