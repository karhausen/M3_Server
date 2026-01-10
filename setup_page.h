#pragma once
#include <Arduino.h>


static const char SETUP_HTML[] PROGMEM = R"HTML(
<!doctype html><html lang="de">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Setup</title>
<style>
  body{font-family:system-ui,Arial;margin:16px;max-width:760px}
  .card{border:1px solid #ddd;border-radius:12px;padding:12px}
  input{padding:10px;border-radius:10px;border:1px solid #ccc;width:100%;margin:6px 0}
  .row{display:flex;gap:10px;flex-wrap:wrap}
  .btn{padding:10px 12px;border-radius:10px;border:1px solid #ccc;background:#f7f7f7;cursor:pointer}
  .btn.primary{background:#e9f2ff;border-color:#b7d4ff}
  .muted{color:#666;font-size:12px}
</style>
</head>
<body>
<h2>Setup</h2>

<div class="card">
  <h3>STA WLAN</h3>
  <div class="muted">SSID & Passwort werden im ESP32 gespeichert (stromlos persistent).</div>

  <label>SSID</label>
  <input id="ssid" placeholder="WLAN-Name">

  <label>Passwort</label>
  <input id="pass" placeholder="Passwort" type="password">

  <div class="row" style="margin-top:10px">
    <button class="btn primary" onclick="save()">Speichern</button>
    <button class="btn" onclick="reboot()">Reboot</button>
    <button class="btn" onclick="location.href='/'">Zurück</button>
  </div>

  <div id="msg" class="muted" style="margin-top:10px"></div>
</div>

<script>
async function save(){
  const ssid = document.getElementById('ssid').value || "";
  const pass = document.getElementById('pass').value || "";
  const r = await fetch('/api/wifi', {
    method:'POST',
    headers:{'Content-Type':'application/json'},
    body: JSON.stringify({ssid, pass})
  });
  const t = await r.text();
  document.getElementById('msg').textContent = t;
}

async function reboot(){
  document.getElementById('msg').textContent = "Rebooting...";
  await fetch('/api/reboot', {method:'POST'});
}
</script>
</body></html>
)HTML";