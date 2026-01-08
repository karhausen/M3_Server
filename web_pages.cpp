#include "web_pages.h"

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