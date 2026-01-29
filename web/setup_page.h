#pragma once
#include <Arduino.h>


static const char SETUP_HTML[] PROGMEM = R"HTML(
<!doctype html><html lang="de">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Setup</title>
<style>
  :root{
    --bg:#ffffff;
    --fg:#111111;
    --muted:#666666;
    --card:#ffffff;

    --border:#c9cdd6;

    --btn:#f7f7f7;
    --btnBorder:#b9beca;
    --btnPrimary:#e9f2ff;
    --btnPrimaryBorder:#b7d4ff;
    --logBg:#111111;
    --logFg:#00ff00;
  }
  body.dark{
    --bg:#0f1115;
    --fg:#e8eaf0;
    --muted:#9aa3b2;
    --card:#141923;
    --border:#2a3242;
    --btn:#1a2230;
    --btnBorder:#2a3242;
    --btnPrimary:#1b2a44;
    --btnPrimaryBorder:#34507a;
    --logBg:#0b0d11;
    --logFg:#7CFC00;
  }

  body{
  font-family:system-ui,Arial;
  margin:16px;
  max-width:760px;
  background:var(--bg);
  color:var(--fg);
}
  
  .card{
  background:var(--card);
  border:1px solid var(--border);
  border-radius:12px;
  padding:12px;
}
  
  input{
    padding:10px;
    border-radius:10px;
    border:1px solid var(--btnBorder);
    width:80%;
    background:var(--card);
    color:var(--fg)
  }
  
  .row{display:flex;gap:10px;flex-wrap:wrap}
  
  .btn{
  background:var(--btn);
  border:1px solid var(--btnBorder);
  border-radius:10px;
  padding:10px 12px;
  cursor:pointer;
  color:var(--fg);
}
  
  .btn.primary{background:var(--btnPrimary);border-color:var(--btnPrimaryBorder)}
  
  .muted{ color:var(--muted); font-size:12px; }
</style>
</head>
<body>
<h2>Setup</h2>

<div class="card">
  <h3>STA WLAN</h3>
  <div class="muted">SSID & Passwort werden im ESP32 gespeichert (stromlos persistent).</div>

  <div class="row" style="margin-top:10px">
    <label>SSID</label>
    <input id="ssid" placeholder="WLAN-Name">
  </div>
  <div class="row" style="margin-top:10px">
    <label>Passwort</label>
    <input id="pass" placeholder="Passwort" type="password">
  </div>

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

function applyDark(isDark){
  document.body.classList.toggle('dark', isDark);
}

(function(){
  try{
    const isDark = localStorage.getItem('dark') === "1";
    applyDark(isDark);
  }catch(e){}
})();

function toggleDark(){
  const isDark = !document.body.classList.contains('dark');
  document.body.classList.toggle('dark', isDark);
  try{ localStorage.setItem('dark', isDark ? "1" : "0"); }catch(e){}
}

</script>
</body></html>
)HTML";