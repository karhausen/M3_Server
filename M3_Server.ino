#include <WiFi.h>
#include <WebServer.h>

// ===================== KONFIG =====================
const char* STA_SSID = "DEIN_WLAN";
const char* STA_PASS = "DEIN_PASSWORT";

// Fallback-Access-Point (für Gelände)
const char* AP_SSID  = "RadioRemote-ESP32";
const char* AP_PASS  = "12345678";   // min. 8 Zeichen
const uint8_t AP_CH  = 6;
const bool AP_HIDE   = false;

// 10s Timeout für STA
const uint32_t STA_TIMEOUT_MS = 10'000;

// Wenn true: AP läuft auch wenn STA ok (WIFI_AP_STA) – "Notfallzugang"
const bool KEEP_AP_ALSO_WHEN_STA_OK = false;

// ===================== STATE =====================
WebServer server(80);

struct {
  bool connected = false;     // "Radio connected" (UI-Schalter), NICHT WiFi
  uint32_t freq_hz = 14074000;
  String mode = "USB";
  String preset = "Plain";   // "Plain" oder "1".."9"
} state;

bool staConnected = false;
bool apStarted = false;

// ===================== UI (HTML) =====================
const char INDEX_HTML[] PROGMEM = R"HTML(
<!doctype html><html lang="de">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Radio Remote</title>
<style>
  body{font-family:system-ui,Arial;margin:16px;max-width:760px}
  .row{display:flex;gap:12px;flex-wrap:wrap}
  .card{border:1px solid #ddd;border-radius:12px;padding:12px;flex:1;min-width:260px}
  .topbar{display:flex;align-items:center;justify-content:space-between;margin-bottom:12px}
  .pill{padding:6px 10px;border-radius:999px;border:1px solid #ddd}
  .led{width:10px;height:10px;border-radius:50%;display:inline-block;margin-right:8px}
  .btn{padding:10px 12px;border-radius:10px;border:1px solid #ccc;background:#f7f7f7;cursor:pointer}
  .btn.primary{background:#e9f2ff;border-color:#b7d4ff}
  .btn:active{transform:scale(0.98)}
  input{padding:10px;border-radius:10px;border:1px solid #ccc;width:100%}
  .seg{display:flex;border:1px solid #ccc;border-radius:10px;overflow:hidden;flex-wrap:wrap}
  .seg button{flex:1;border:0;padding:10px;cursor:pointer;background:#f7f7f7;min-width:60px}
  .seg button.active{background:#e9f2ff}
  .grid{display:grid;grid-template-columns:repeat(6,1fr);gap:8px}
  .log{height:140px;overflow:auto;background:#111;color:#0f0;padding:10px;border-radius:10px;
       font-family:ui-monospace,monospace;font-size:12px}
  .muted{color:#666;font-size:12px}
</style>
</head>
<body>

<div class="topbar">
  <div class="pill">
    <span id="led" class="led" style="background:#c00"></span>
    <span id="connText">Disconnected</span>
  </div>
  <div>
    <button class="btn primary" onclick="sendCmd('connect')">Connect</button>
    <button class="btn" onclick="sendCmd('disconnect')">Disconnect</button>
  </div>
</div>

<div class="muted" id="netInfo"></div>

<div class="row" style="margin-top:12px">
  <div class="card">
    <h3>Presets</h3>
    <div class="seg" style="gap:0">
      <button id="pPlatin" class="active" onclick="setPreset('Plain')">Plain</button>
      <button id="p1" onclick="setPreset('1')">1</button>
      <button id="p2" onclick="setPreset('2')">2</button>
      <button id="p3" onclick="setPreset('3')">3</button>
      <button id="p4" onclick="setPreset('4')">4</button>
      <button id="p5" onclick="setPreset('5')">5</button>
      <button id="p6" onclick="setPreset('6')">6</button>
      <button id="p7" onclick="setPreset('7')">7</button>
      <button id="p8" onclick="setPreset('8')">8</button>
      <button id="p9" onclick="setPreset('9')">9</button>
    </div>
  </div>

  <div class="card">
    <h3>Betriebsart</h3>
    <div class="seg">
      <button id="mLSB" onclick="setMode('LSB')">LSB</button>
      <button id="mUSB" class="active" onclick="setMode('USB')">USB</button>
      <button id="mCW" onclick="setMode('CW')">CW</button>
    </div>
  </div>
</div>

<div class="card" style="margin-top:12px">
  <h3>Frequenz</h3>
  <div class="row">
    <div style="flex:2">
      <input id="freq" inputmode="numeric" value="14074000">
    </div>
    <div style="flex:1">
      <button class="btn primary" onclick="setFreq()">Set</button>
    </div>
  </div>

  <div style="margin-top:10px" class="grid">
    <button class="btn" onclick="nudge(-1000000)">-1M</button>
    <button class="btn" onclick="nudge(-100000)">-100k</button>
    <button class="btn" onclick="nudge(-10000)">-10k</button>
    <button class="btn" onclick="nudge(-1000)">-1k</button>
    <button class="btn" onclick="nudge(-100)">-100</button>
    <button class="btn" onclick="nudge(-10)">-10</button>

    <button class="btn" onclick="nudge(10)">+10</button>
    <button class="btn" onclick="nudge(100)">+100</button>
    <button class="btn" onclick="nudge(1000)">+1k</button>
    <button class="btn" onclick="nudge(10000)">+10k</button>
    <button class="btn" onclick="nudge(100000)">+100k</button>
    <button class="btn" onclick="nudge(1000000)">+1M</button>
  </div>
</div>

<div class="card" style="margin-top:12px">
  <h3>Log</h3>
  <div id="log" class="log"></div>
</div>

<script>
function logLine(s){
  const el=document.getElementById('log');
  el.textContent += s + "\\n";
  el.scrollTop = el.scrollHeight;
}
function setConn(connected){
  document.getElementById('led').style.background = connected ? '#0a0' : '#c00';
  document.getElementById('connText').textContent = connected ? 'Connected' : 'Disconnected';
}

function markPreset(p){
  const ids = ["Platin","1","2","3","4","5","6","7","8","9"];
  ids.forEach(x=>{
    const id = (x==="Platin") ? "pPlatin" : ("p"+x);
    const el = document.getElementById(id);
    if(el) el.classList.toggle('active', x===p);
  });
}

async function refreshState(){
  try{
    const r = await fetch('/api/state');
    const st = await r.json();

    setConn(!!st.radio_connected);
    document.getElementById('freq').value = st.freq_hz;

    ['LSB','USB','CW'].forEach(x=>{
      document.getElementById('m'+x).classList.toggle('active', st.mode===x);
    });

    markPreset(st.preset || "Platin");

    document.getElementById('netInfo').textContent =
      `WiFi: ${st.wifi_mode} | STA: ${st.sta_ip || '-'} | AP: ${st.ap_ip || '-'}`;
  }catch(e){
    // ignore
  }
}

async function sendCmd(cmd, payload={}){
  const body = JSON.stringify({cmd, ...payload});
  logLine("> " + body);
  const r = await fetch('/api/cmd', {method:'POST', headers:{'Content-Type':'application/json'}, body});
  const t = await r.text();
  logLine("< " + t);
  await refreshState();
}

function setPreset(p){
  sendCmd('preset', {value:p});
}
function setMode(m){
  sendCmd('mode', {value:m});
}
function setFreq(){
  const hz = parseInt(document.getElementById('freq').value,10)||0;
  sendCmd('freq', {hz});
}
function nudge(delta){
  const f = parseInt(document.getElementById('freq').value,10)||0;
  const nf = Math.max(0, f + delta);
  document.getElementById('freq').value = nf;
  sendCmd('freq', {hz:nf});
}

refreshState();
setInterval(refreshState, 1500);
</script>

</body></html>
)HTML";

// ===================== WIFI =====================
bool connectSTA_withTimeout(uint32_t timeoutMs) {
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);
  WiFi.begin(STA_SSID, STA_PASS);

  Serial.printf("STA: connecting to '%s' ...\n", STA_SSID);

  uint32_t t0 = millis();
  while (WiFi.status() != WL_CONNECTED && (millis() - t0) < timeoutMs) {
    delay(250);
    Serial.print(".");
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("STA: connected!");
    Serial.print("STA IP: ");
    Serial.println(WiFi.localIP());
    return true;
  }

  Serial.println("STA: connect timeout.");
  return false;
}

bool startAP() {
  if (KEEP_AP_ALSO_WHEN_STA_OK && staConnected) WiFi.mode(WIFI_AP_STA);
  else WiFi.mode(WIFI_AP);

  WiFi.setSleep(false);

  IPAddress apIP(192,168,4,1);
  IPAddress gw(192,168,4,1);
  IPAddress mask(255,255,255,0);
  WiFi.softAPConfig(apIP, gw, mask);

  Serial.printf("AP: starting '%s' ...\n", AP_SSID);

  bool ok = WiFi.softAP(AP_SSID, AP_PASS, AP_CH, AP_HIDE);
  if (!ok) {
    Serial.println("AP: failed to start!");
    return false;
  }

  Serial.println("AP: started!");
  Serial.print("AP IP: ");
  Serial.println(WiFi.softAPIP());
  return true;
}

void setupWiFi() {
  Serial.println();
  Serial.println("=== WiFi setup ===");

  staConnected = connectSTA_withTimeout(STA_TIMEOUT_MS);

  if (!staConnected) {
    apStarted = startAP();
  } else {
    if (KEEP_AP_ALSO_WHEN_STA_OK) apStarted = startAP();
    else {
      apStarted = false;
      Serial.println("AP: not started (STA OK).");
    }
  }

  Serial.printf("Result: STA=%s, AP=%s\n",
                staConnected ? "ON" : "OFF",
                apStarted ? "ON" : "OFF");
}

// ===================== HTTP HANDLER =====================
void handleRoot() {
  server.send(200, "text/html; charset=utf-8", INDEX_HTML);
}

String readBody() {
  if (server.hasArg("plain")) return server.arg("plain");
  return "";
}

// sehr simples JSON-Feld auslesen: "key":"value"
String extractJsonString(const String& body, const char* key) {
  String k = String("\"") + key + "\":\"";
  int p = body.indexOf(k);
  if (p < 0) return "";
  int s = p + k.length();
  int e = body.indexOf("\"", s);
  if (e <= s) return "";
  return body.substring(s, e);
}

// sehr simples JSON-Feld auslesen: "key":123
long extractJsonNumber(const String& body, const char* key) {
  String k = String("\"") + key + "\":";
  int p = body.indexOf(k);
  if (p < 0) return 0;
  int s = p + k.length();
  int e1 = body.indexOf("}", s);
  int e2 = body.indexOf(",", s);
  int e = (e2 > 0 && e2 < e1) ? e2 : e1;
  if (e <= s) return 0;
  return body.substring(s, e).toInt();
}

void handleCmd() {
  String body = readBody();

  // Dummy-Ausgabe auf Serial0 (später: Serial2)
  Serial.println("[API CMD] " + body);

  String cmd = extractJsonString(body, "cmd");

  if (cmd == "connect") state.connected = true;
  else if (cmd == "disconnect") state.connected = false;
  else if (cmd == "preset") {
    String v = extractJsonString(body, "value");
    if (v.length()) state.preset = v;
  }
  else if (cmd == "mode") {
    String v = extractJsonString(body, "value");
    if (v.length()) state.mode = v;
  }
  else if (cmd == "freq") {
    long hz = extractJsonNumber(body, "hz");
    if (hz >= 0) state.freq_hz = (uint32_t)hz;
  }

  server.send(200, "text/plain", "OK");
}

void handleState() {
  String wifiMode;
  wifi_mode_t mode = WiFi.getMode();
  if (mode == WIFI_MODE_STA) wifiMode = "STA";
  else if (mode == WIFI_MODE_AP) wifiMode = "AP";
  else if (mode == WIFI_MODE_APSTA) wifiMode = "AP+STA";
  else wifiMode = "OFF";

  String staIp = (WiFi.status() == WL_CONNECTED) ? WiFi.localIP().toString() : "";
  String apIp  = (WiFi.getMode() == WIFI_MODE_AP || WiFi.getMode() == WIFI_MODE_APSTA) ? WiFi.softAPIP().toString() : "";

  String json = "{";
  json += "\"radio_connected\":" + String(state.connected ? "true" : "false") + ",";
  json += "\"freq_hz\":" + String(state.freq_hz) + ",";
  json += "\"mode\":\"" + state.mode + "\",";
  json += "\"preset\":\"" + state.preset + "\",";
  json += "\"wifi_mode\":\"" + wifiMode + "\",";
  json += "\"sta_ip\":\"" + staIp + "\",";
  json += "\"ap_ip\":\"" + apIp + "\"";
  json += "}";

  server.send(200, "application/json", json);
}

// ===================== SETUP/LOOP =====================
void setup() {
  Serial.begin(115200);
  delay(200);

  // Serial2 später aktivieren:
  // Serial2.begin(19200, SERIAL_8N1, RX2, TX2);

  setupWiFi();

  server.on("/", HTTP_GET, handleRoot);
  server.on("/api/cmd", HTTP_POST, handleCmd);
  server.on("/api/state", HTTP_GET, handleState);
  server.begin();

  Serial.println("HTTP server started.");
  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("Open (STA): http://");
    Serial.print(WiFi.localIP());
    Serial.println("/");
  }
  if (WiFi.getMode() == WIFI_MODE_AP || WiFi.getMode() == WIFI_MODE_APSTA) {
    Serial.print("Open (AP):  http://");
    Serial.print(WiFi.softAPIP());
    Serial.println("/");
  }
}

void loop() {
  server.handleClient();
}