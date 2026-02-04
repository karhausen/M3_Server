#include "radio_link.h"
#include "display.h"

static HardwareSerial& R = Serial2;

// --- TX Queue (ring buffer) ---
static const int QSIZE = 20;
static String q[QSIZE];
static volatile int qHead = 0;
static volatile int qTail = 0;

static uint32_t lastTxMs = 0;
static const uint32_t TX_GAP_MS = 25;

static String rxLine;
static String lastRx;
static bool ready = false;

static String rxBuf;
static String lastLine;

static bool q_empty(){ return qHead == qTail; }
static bool q_full(){ return ((qTail + 1) % QSIZE) == qHead; }

static bool q_push(const String& s){
  if(q_full()) return false;
  q[qTail] = s;
  qTail = (qTail + 1) % QSIZE;
  if (RADIO_DEBUG_MIRROR) Serial.println("[q_push][RADIO] " + s);
  return true;
}

static bool q_pop(String& out){
  if(q_empty()) return false;
  out = q[qHead];
  qHead = (qHead + 1) % QSIZE;
  if (RADIO_DEBUG_MIRROR) Serial.println("[q_pop][RADIO] " + out);
  return true;
}

// ---------- Protocol helpers ----------
static String radio_build(const String& cmd, const String& param = ""){
  String s;
  s.reserve(64);
  s += RADIO_HEADER;    // "\nDM:"
  s += cmd;             // "FF SRF" oder "REMOTE SENTER2,0" ...
  if(param.length()){
    s += RADIO_DELIMITER; // "" laut Doku
    s += param;           // "30100000"
  }
  s += RADIO_FOOTER;    // "\r"
  return s;
}

// Für OPEN/close gibt's KEIN "DM:" Prefix, nur <LF>O<CR>
static String radio_open_serial(){
  return String("\n") + "O" + "\r";
}

// ---------- State machine ----------
// enum class RadioState : uint8_t { BOOT, WAIT_OPEN_ACK, COM_PORT_IS_OPEN, WAIT_CONNECT_ACK, WAIT_DISCONNECT_ACK, READY };
// static RadioState st = RadioState::BOOT;


static void enqueueOrDrop(const String& s){
  if(!q_push(s)){
    if (RADIO_DEBUG_MIRROR) Serial.println("[enqueueOrDrop][RADIO] TX queue full, drop!");
  } else
  {if (RADIO_DEBUG_MIRROR) Serial.println("[enqueueOrDrop][RADIO] enqueued: " + s);}
}

static void sendNow(const String& s){
  R.print(s);
  lastTxMs = millis();
  if (RADIO_DEBUG_MIRROR) Serial.println("[sendNow][RADIO TX] " + s);
}

static void radio_start_communication(){
  // Boot: OPEN senden
  if (RADIO_DEBUG_MIRROR) Serial.println("[radio_start_communication][RADIO] try to open comport");
  sendNow(radio_open_serial());
  global_radio_state.state = RadioState::WAIT_OPEN_ACK;
  if (RADIO_STATE_MIRROR) Serial.println("[State]->WAIT_OPEN_ACK");
}

// --- Remote control ---
static String cmd_remoteOn()  {
  if (RADIO_DEBUG_MIRROR) Serial.println("[cmd_remoteOn]");
  return radio_build("REMOTE SENTER2,0"); 
}

static String cmd_remoteOff() { 
  if (RADIO_DEBUG_MIRROR) Serial.println("[cmd_remoteOff]");
  return radio_build("REMOTE SENTER0"); 
}

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

bool radio_is_ready(){
  return global_radio_state.state == RadioState::READY;
}

void radio_init(){
  R.begin(RADIO_BAUD, SERIAL_8N1, RADIO_RX_PIN, RADIO_TX_PIN);
  rxBuf.reserve(128);
  lastLine.reserve(128);
  radio_send_disconnect();  // wenn radio schon online
  
  global_radio_state.state = RadioState::BOOT;
  if (RADIO_STATE_MIRROR) Serial.println("[State]->BOOT");
  if (RADIO_DEBUG_MIRROR) Serial.println("[radio_init][RADIO] init");
  radio_start_communication();
}


// ---------- RX parsing ----------
static void run_state_machine(const String& line){
  lastLine = line;
  if (RADIO_DEBUG_MIRROR) Serial.println("[run_state_machine][RADIO RX] " + lastLine);

  // Doku: open-ack: "o"
  if(global_radio_state.state == RadioState::WAIT_OPEN_ACK){
    if(line == "o"){
      if (RADIO_DEBUG_MIRROR) Serial.println("[run_state_machine][RADIO RX] response: " + line);
      // Remote operational preset 0 aktivieren
      global_radio_state.state = RadioState::COM_PORT_IS_OPEN;
      if (RADIO_STATE_MIRROR) Serial.println("[State]->COM_PORT_IS_OPEN");
      // ----- auto-connect -----
      // sendNow(radio_build("REMOTE SENTER2,0"));
      // global_radio_state.state = RadioState::WAIT_CONNECT_ACK;
      // if (RADIO_STATE_MIRROR) Serial.println("[State]->WAIT_CONNECT_ACK");
      return;
    }
  }

  if(global_radio_state.state == RadioState::COM_PORT_IS_OPEN){
    
  }
  if(global_radio_state.state == RadioState::READY){
    
  }

  // ------------- connect / disconnect ------------------------

  if(global_radio_state.state == RadioState::WAIT_CONNECT_ACK){
    if(line == "ds100ENTER"){
      if (RADIO_DEBUG_MIRROR) Serial.println("[run_state_machine][RADIO RX] tried to disconnect, but we're already disconnected!");
      if (RADIO_STATE_MIRROR) Serial.println("[State]->COM_PORT_IS_OPEN (ds100)");
      global_radio_state.radio_connected = true;
    }
    if(line == "ds"){ // line == "ds"
      global_radio_state.state = RadioState::READY;
      if (RADIO_STATE_MIRROR) Serial.println("[State]->READY (ds)");
      global_radio_state.radio_connected = true;
      displaySetConnected(global_radio_state.radio_connected);
      // return;
    }
  }

  if(global_radio_state.state == RadioState::WAIT_DISCONNECT_ACK){
    if(line == "ds100ENTER"){
      if (RADIO_DEBUG_MIRROR) Serial.println("[run_state_machine][RADIO RX] tried to disconnect, but we're already disconnected!");
      if (RADIO_STATE_MIRROR) Serial.println("[State]->COM_PORT_IS_OPEN");
    }
    if(line == "ds"){
      global_radio_state.state = RadioState::COM_PORT_IS_OPEN;
      if (RADIO_STATE_MIRROR) Serial.println("[State]->COM_PORT_IS_OPEN");
      global_radio_state.radio_connected = false;
      displaySetConnected(global_radio_state.radio_connected);
    }
  }

  //--------------------------- set / change modulation mode ------------------------

  if(global_radio_state.state == RadioState::WAIT_SET_MODE_ACK){
    if(line == "ds"){
      global_radio_state.state = RadioState::READY;
      if (RADIO_STATE_MIRROR) {
        Serial.print("[run_state_machine][WAIT_SET_MODE_ACK][State]->");
        Serial.println(radio_state_to_string(global_radio_state.state));
        Serial.print("[run_state_machine][RADIO RX] Radio Mode was set to:");
        Serial.println(global_radio_state.mode_str);
      }
      if (RADIO_STATE_MIRROR) Serial.println("[State]->READY");
      if (RADIO_DEBUG_MIRROR) {
        Serial.print("[run_state_machine][radio_mode]->actual: ");
        Serial.println(radio_mode_to_string(global_radio_state.mode));
        Serial.print("[run_state_machine][radio_mode]->desired: ");
        Serial.println(radio_mode_to_string(global_radio_state.desired_mode));
      }
      global_radio_state.mode = global_radio_state.desired_mode;
      displaySetMode(global_radio_state.mode);
      global_radio_state.mode_str = radio_mode_to_string(global_radio_state.mode);

    }
  }

  // Doku: get-response: "dg...."
  // Beispiel: dgRF72125000;TF60000000
  if(line.startsWith("dg")){
    String payload = line.substring(2); // nach "dg"
    // Tokens split by ';'
    int start = 0;
    while(true){
      int sep = payload.indexOf(RADIO_CMD_SEPARATOR, start);
      String tok = (sep < 0) ? payload.substring(start) : payload.substring(start, sep);
      tok.trim();
      if(tok.length()){
        // RF<Hz> / TF<Hz>
        if(tok.startsWith("RF")){
          uint32_t hz = (uint32_t)tok.substring(2).toInt();
          if(hz > 0) global_radio_state.freq_hz = hz; // du nutzt aktuell UI für RX freq
        }
        // ggf. TF später nutzen
      }
      if(sep < 0) break;
      start = sep + 1;
    }
  }
}

static void radio_read_rx(){
  while(R.available() > 0){
    char c = (char)R.read();

    if(c == '\n'){
      // Start-of-frame LF -> ignorieren wir, wir sammeln nur bis CR
      // Serial.println("[radio]->[cr]");
      continue;
    }
    if(c == '\r'){
      // End-of-frame
      String line = rxBuf;
      line.trim();
      if (line.charAt(0) == '\n') {
        line.remove(0, 1);
      }

      if (RADIO_DEBUG_MIRROR) Serial.print("[radio]->[lf] " + line);
      rxBuf = "";
      // line.trim();
      if(line.length()) run_state_machine(line);
      continue;
    }

    if(rxBuf.length() < 200) rxBuf += c;
  }
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

// ---------- TX flush ----------
static void radio_flush_tx(){
  if(global_radio_state.state != RadioState::READY){
    return; // erst nach Handshake senden!
  }
  if(q_empty()) return;

  uint32_t now = millis();
  if(now - lastTxMs < TX_GAP_MS) return;

  String out;
  if(q_pop(out)){
    if (RADIO_DEBUG_MIRROR) Serial.println("[radio_flush_tx][RADIO] Try to send: " + out);
    sendNow(out);
  }
}

void radio_loop(){
  radio_read_rx();
  radio_flush_tx();
}

// ---------- High-level commands ----------
void radio_send_connect(){
  // enqueueOrDrop(radio_build("REMOTE SENTER2,0"));
  sendNow(radio_build("REMOTE SENTER2,0"));
  global_radio_state.state = RadioState::WAIT_CONNECT_ACK;
  if (RADIO_STATE_MIRROR) Serial.println("[State]->WAIT_CONNECT_ACK");
}

void radio_send_disconnect(){
  // enqueueOrDrop(radio_build("REMOTE SENTER0"));
  sendNow(radio_build("REMOTE SENTER0"));
  global_radio_state.state = RadioState::WAIT_DISCONNECT_ACK;
  if (RADIO_STATE_MIRROR) Serial.println("[State]->WAIT_DISCONNECT_ACK");
}

static int presetToPage(const String& preset){
  if(preset.equalsIgnoreCase("Platin")) return 0;
  int p = preset.toInt();
  if(p < 0) p = 0;
  if(p > 9) p = 9;
  return p;
}

void radio_send_preset(const String& preset){
  // laut deiner Liste: "GR SPRS" + page
  enqueueOrDrop(radio_build("GR SPRS", String(presetToPage(preset))));
}

void radio_send_mode(const String& mode){
  // Mapping gemäß deiner Liste
  if(mode == "CW") {
    sendNow(radio_build("FF SMD8" ));
    global_radio_state.desired_mode = RadioMode::CW;
  }       
  else if(mode == "AM")  {
    sendNow(radio_build("FF SMD9" ));
    global_radio_state.desired_mode = RadioMode::AM;
  }    
  else if(mode == "FM")  {
    sendNow(radio_build("FF SMD17"));
    global_radio_state.desired_mode = RadioMode::FM;
  }
  else if(mode == "USB") {
    sendNow(radio_build("FF SMD12"));
    global_radio_state.desired_mode = RadioMode::USB;
  }
  else if(mode == "LSB") {
    sendNow(radio_build("FF SMD14"));
    global_radio_state.desired_mode = RadioMode::LSB;
  }
  else {
    Serial.print("[radio_send_mode]unknown radio_mode: ");
    Serial.println(mode);
  }
  delay(500);
  global_radio_state.state = RadioState::WAIT_SET_MODE_ACK;
  if (RADIO_STATE_MIRROR) {
    Serial.print("[radio_send_mode][State]->");
    Serial.println(radio_state_to_string(global_radio_state.state));
  }
}

void radio_send_freq(uint32_t hz){
  // laut Doku: FF SRF30100000 (ohne Leerzeichen)
  enqueueOrDrop(radio_build("FF SRF" + String(hz) + RADIO_CMD_SEPARATOR + "TF" + String(hz) ));
}

void radio_send_rx_freq(uint32_t hz){
  // laut Doku: FF SRF30100000 (ohne Leerzeichen)
  enqueueOrDrop(radio_build("FF SRF" + String(hz) ));
}

void radio_send_raw(const String& core){
  if (RADIO_DEBUG_MIRROR) Serial.println("[radio_send_raw][RADIO] " + core);
  enqueueOrDrop(radio_build(core));
}

void radio_query_rx_tx_freq(){
  // Multi-command inquiry: "FF GRF;TF"
  // -> hier bauen wir den kompletten cmd-String inkl ';'
  enqueueOrDrop(radio_build(String("FF GRF") + RADIO_CMD_SEPARATOR + "TF"));
}

void radio_query_rxfreq(){
  enqueueOrDrop(cmd_getRxFreq());
}

void radio_query_presetpage(){
  enqueueOrDrop(cmd_getPresetPage());
}
