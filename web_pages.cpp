#include "web_pages.h"

const char INDEX_HTML[] PROGMEM = R"HTML(

<!doctype html><html lang="de">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Radio Remote</title>

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

  body{font-family:system-ui,Arial;margin:16px;max-width:760px;background:var(--bg);color:var(--fg)}
  .row{display:flex;gap:12px;flex-wrap:wrap}
  .card{border:1px solid var(--border);border-radius:12px;padding:12px;flex:1;min-width:260px;background:var(--card)}
  .topbar{display:flex;align-items:center;justify-content:space-between;margin-bottom:12px}
  .pill{padding:6px 10px;border-radius:999px;border:1px solid var(--border);background:var(--card)}
  .led{width:10px;height:10px;border-radius:50%;display:inline-block;margin-right:8px}
  .btn{padding:10px 12px;border-radius:10px;border:1px solid var(--btnBorder);background:var(--btn);cursor:pointer;color:var(--fg)}
  .btn.primary{background:var(--btnPrimary);border-color:var(--btnPrimaryBorder)}
  .btn:active{transform:scale(0.98)}
  input{padding:10px;border-radius:10px;border:1px solid var(--btnBorder);width:100%;background:var(--card);color:var(--fg)}
  .seg{display:flex;border:1px solid var(--btnBorder);border-radius:10px;overflow:hidden;flex-wrap:wrap}
  .seg button{flex:1;border:0;padding:10px;cursor:pointer;background:var(--btn);min-width:60px;color:var(--fg)}
  .seg button.active{background:var(--btnPrimary)}
  .grid{display:grid;grid-template-columns:repeat(6,1fr);gap:8px}
  .log{height:140px;overflow:auto;background:var(--logBg);color:var(--logFg);padding:10px;border-radius:10px;
       font-family:ui-monospace,monospace;font-size:12px}
  .muted{color:var(--muted);font-size:12px}
</style>

</head>
<body>

<div class="topbar">
  <div style="display:flex; gap:10px; align-items:center">
    <button id="darkBtn" class="btn" onclick="toggleDark()" title="Dark Mode umschalten">🌙</button>

    <div class="pill">
      <span id="led" class="led" style="background:#c00"></span>
      <span id="connText">Disconnected</span>
    </div>
  </div>

<div>
  <button id="connBtn" class="btn primary" onclick="toggleConnect()">Connect</button>
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

 <div
  id="freqDisplay"
  style="
    font-size:40px;
    font-weight:800;
    letter-spacing:2px;
    text-align:center;
    user-select:none;
    padding:8px 0;
    font-family:ui-monospace,monospace;
  ">
  14 074 000 Hz
</div>
  
</div>


<div class="card" style="margin-top:12px">
  <h3>Log</h3>
  <div id="log" class="log"></div>
</div>

<script>

(function enableWheelSwipe(){
  const el = document.getElementById('freqDisplay');
  if(!el) return;

  let sx=0, sy=0;
  let lastAction = 0;
  const TH = 25;      // Pixel-Schwelle
  const RATE = 80;    // ms zwischen Aktionen

  el.style.touchAction = "none"; // wichtig, damit der Browser nicht scrollt/zoomt

  el.addEventListener('touchstart', (e)=>{
    const t = e.touches[0];
    sx = t.clientX; sy = t.clientY;
  }, {passive:true});

  el.addEventListener('touchmove', async (e)=>{
    const t = e.touches[0];
    const dx = t.clientX - sx;
    const dy = t.clientY - sy;

    const now = Date.now();
    if(now - lastAction < RATE) return;

    if(Math.abs(dy) > Math.abs(dx) && Math.abs(dy) > TH){
      lastAction = now;
      sy = t.clientY; // reset, damit man weiterwischen kann
      if(dy < 0) await freqPlus(); else await freqMinus();
    }
    else if(Math.abs(dx) > Math.abs(dy) && Math.abs(dx) > TH){
      lastAction = now;
      sx = t.clientX;
      if(dx > 0) stepRight(); else stepLeft();
    }
  }, {passive:true});
})();


//------------------------------------------------------------

function logLine(s){
  const el=document.getElementById('log');
  el.textContent += s + "\n"; // "\\n"
  el.scrollTop = el.scrollHeight;
}

window.addEventListener('error', (e) => {
  logLine("JS ERROR: " + (e.message || e));
});

window.addEventListener('unhandledrejection', (e) => {
  logLine("PROMISE ERROR: " + (e.reason?.message || e.reason || e));
});


function setConn(connected){
  document.getElementById('led').style.background = connected ? '#0a0' : '#c00';
  document.getElementById('connText').textContent = connected ? 'Connected' : 'Disconnected';

  const b = document.getElementById('connBtn');
  if(b){
    b.textContent = connected ? "Disconnect" : "Connect";
    b.classList.toggle('primary', !connected); // primary nur bei "Connect"
  }
}

async function toggleConnect(){
  const isConnected = document.getElementById('connText').textContent === 'Connected';
  await sendCmd(isConnected ? 'disconnect' : 'connect');
}


function markPreset(p){
  const ids = ["Plain","1","2","3","4","5","6","7","8","9"];
  ids.forEach(x=>{
    const id = (x==="Plain") ? "pPlain" : ("p"+x);
    const el = document.getElementById(id);
    if(el) el.classList.toggle('active', x===p);
  });
}

async function refreshState(){
  try{
    const r = await fetch('/api/state');
    const st = await r.json();

    setConn(!!st.radio_connected);

    currentFreqHz = parseInt(st.freq_hz, 10) || currentFreqHz;
    updateFreqUI();


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
  const btn = document.getElementById('connBtn');
  if(btn) btn.disabled = true;

  const body = JSON.stringify({cmd, ...payload});
  logLine("" + body);

  try{
    const r = await fetch('/api/cmd', {method:'POST', headers:{'Content-Type':'application/json'}, body});
    const t = await r.text();
    logLine("" + t);
  } catch(e){
    logLine("ERROR: no response");
  } finally {
    await refreshState();
    if(btn) btn.disabled = false;
  }
}

function setPreset(p){
  sendCmd('preset', {value:p});
}

function setMode(m){
  sendCmd('mode', {value:m});
}


// ------- Frequency "front panel" control -------
const FREQ_MIN = 1500;
const FREQ_MAX = 30000000;

let currentFreqHz = 14074000;



// 1 Hz .. 10 MHz
const STEPS = [1,10,100,1000,10000,100000,1000000,10000000];
let stepIndex = 3; // 1 kHz

function clamp(v, lo, hi){ return Math.max(lo, Math.min(hi, v)); }

function fmtHz(n){
  const s = String(Math.trunc(n));
  return s.replace(/\B(?=(\d{3})+(?!\d))/g, " ") + " Hz";
}

// genau EINE Stelle unterstreichen (aktive Digit-Position)
function fmtHzWithUnderline(freq, step){
  const raw = String(Math.trunc(freq)).padStart(8, '0'); // bis 30 MHz: 8 Stellen
  const len = raw.length;

  const posFromRight = Math.log10(step) | 0;      // 1->0, 10->1, ...
  const activeIndex  = len - 1 - posFromRight;    // Index im String

  let out = "";
  for(let i=0;i<len;i++){
    const ch = raw[i];
    out += (i === activeIndex) ? `<u>${ch}</u>` : ch;

    const fromRight = len - i - 1;
    if(fromRight % 3 === 0 && i !== len - 1) out += " ";
  }
  return out + " Hz";
}

function updateFreqUI(){
  const fd = document.getElementById('freqDisplay');
  if(fd) fd.innerHTML = fmtHzWithUnderline(currentFreqHz, STEPS[stepIndex]);

  const sl = document.getElementById('stepLabel');
  if(sl) sl.textContent = fmtHz(STEPS[stepIndex]);
}

async function pushFreq(){
  currentFreqHz = clamp(currentFreqHz, FREQ_MIN, FREQ_MAX);
  updateFreqUI();
  // logLine(JSON.stringify({cmd:"freq", hz: currentFreqHz}));   // <— HARTE Debug-Zeile
  await sendCmd('freq', { hz: currentFreqHz });
}

async function freqPlus(){
  currentFreqHz = clamp(currentFreqHz + STEPS[stepIndex], FREQ_MIN, FREQ_MAX);
  await pushFreq();
}

async function freqMinus(){
  currentFreqHz = clamp(currentFreqHz - STEPS[stepIndex], FREQ_MIN, FREQ_MAX);
  await pushFreq();
}

function stepLeft(){
  // gröber
  stepIndex = clamp(stepIndex + 1, 0, STEPS.length - 1);
  updateFreqUI();
}

function stepRight(){
  // feiner
  stepIndex = clamp(stepIndex - 1, 0, STEPS.length - 1);
  updateFreqUI();
}



function applyDark(isDark){
  document.body.classList.toggle('dark', isDark);
  const b = document.getElementById('darkBtn');
  if(b) b.textContent = isDark ? "☀️" : "🌙";
}

function toggleDark(){
  const isDark = !document.body.classList.contains('dark');
  applyDark(isDark);
  try{ localStorage.setItem('dark', isDark ? "1" : "0"); }catch(e){}
}

// beim Start wiederherstellen
(function(){
  let isDark = false;
  try{ isDark = localStorage.getItem('dark') === "1"; }catch(e){}
  applyDark(isDark);
})();

updateFreqUI();
refreshState();
setInterval(refreshState, 1500);
</script>

</body></html>

)HTML";