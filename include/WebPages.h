#ifndef WEBPAGES_H
#define WEBPAGES_H

#include <Arduino.h>

// HTML dashboard – ESP32 MQTT Home Assistant DIY Kit
const char htmlPage[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="ro">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>ESP32 HA Kit</title>
<style>
*{margin:0;padding:0;box-sizing:border-box}
body{font-family:'Segoe UI',sans-serif;background:linear-gradient(135deg,#1e293b,#0f172a);min-height:100vh;padding:20px}
.container{max-width:960px;margin:0 auto;background:#fff;border-radius:16px;box-shadow:0 20px 60px rgba(0,0,0,.35);padding:28px}
h1{text-align:center;color:#1e40af;font-size:2em;margin-bottom:4px}
.subtitle{text-align:center;color:#64748b;font-size:.9em;margin-bottom:24px}
/* Tabs */
.tabs{display:flex;gap:8px;border-bottom:2px solid #e5e7eb;margin-bottom:24px}
.tab-btn{padding:10px 22px;background:none;border:none;border-bottom:3px solid transparent;font-weight:600;color:#64748b;cursor:pointer;transition:all .25s;font-size:.95em}
.tab-btn.active{color:#1e40af;border-bottom-color:#1e40af}
.tab-btn:hover{color:#1e40af}
.tab{display:none}.tab.active{display:block}
/* MQTT status bar */
.mqtt-bar{padding:10px 16px;border-radius:8px;font-weight:600;font-size:.9em;margin-bottom:20px;text-align:center}
.mqtt-ok{background:#d1fae5;color:#065f46}
.mqtt-err{background:#fee2e2;color:#991b1b}
.mqtt-unknown{background:#fef3c7;color:#92400e}
/* Cards grid */
.cards{display:grid;grid-template-columns:repeat(auto-fit,minmax(200px,1fr));gap:16px;margin-bottom:24px}
.card{border-radius:12px;padding:22px;text-align:center;color:#fff;position:relative;overflow:hidden}
.card-label{font-size:.8em;opacity:.85;margin-bottom:8px;text-transform:uppercase;letter-spacing:.05em}
.card-value{font-size:2.8em;font-weight:700;line-height:1}
.card-unit{font-size:1em;opacity:.8;margin-top:4px}
.card-temp{background:linear-gradient(135deg,#f97316,#ef4444)}
.card-hum{background:linear-gradient(135deg,#3b82f6,#6366f1)}
/* Motion card */
.card-motion{background:linear-gradient(135deg,#10b981,#059669);transition:background .4s}
.card-motion.alert{background:linear-gradient(135deg,#ef4444,#dc2626)}
.motion-circle{width:60px;height:60px;border-radius:50%;margin:8px auto;display:flex;align-items:center;justify-content:center;font-size:1.8em;background:rgba(255,255,255,.2)}
.motion-circle.pulse{animation:pulse 1s infinite}
@keyframes pulse{0%,100%{transform:scale(1);box-shadow:0 0 0 0 rgba(255,255,255,.4)}50%{transform:scale(1.08);box-shadow:0 0 0 10px rgba(255,255,255,0)}}
/* Relay card */
.card-relay{background:linear-gradient(135deg,#8b5cf6,#6d28d9)}
.relay-state{font-size:2em;font-weight:700;margin:8px 0}
.relay-btns{display:flex;gap:8px;margin-top:10px;justify-content:center}
.relay-btns button{padding:7px 18px;border:2px solid rgba(255,255,255,.6);background:rgba(255,255,255,.15);color:#fff;border-radius:6px;font-weight:700;cursor:pointer;transition:background .2s}
.relay-btns button:hover{background:rgba(255,255,255,.35)}
/* Info box */
.info-box{background:#f0f9ff;border-left:4px solid #3b82f6;padding:14px 18px;border-radius:8px;font-size:.88em;color:#1e3a5f;line-height:1.8}
.info-box ul{margin-left:18px}
/* Footer */
.footer{text-align:center;margin-top:20px;color:#94a3b8;font-size:.82em}
/* WiFi */
.wifi-status{padding:12px 16px;border-radius:8px;margin-bottom:18px;font-weight:600}
.wifi-ok{background:#d1fae5;color:#065f46}
.wifi-ap{background:#fef3c7;color:#92400e}
.form-group{margin-bottom:14px}
.form-label{display:block;margin-bottom:5px;font-weight:600;color:#1e293b}
.form-input{width:100%;padding:10px;border:2px solid #e2e8f0;border-radius:8px;font-size:1em}
.form-input:focus{outline:none;border-color:#3b82f6}
.btn-row{display:flex;gap:10px;margin-top:18px}
.btn{padding:11px 22px;border:none;border-radius:8px;font-weight:700;cursor:pointer;font-size:.95em;flex:1}
.btn-primary{background:#3b82f6;color:#fff}
.btn-danger{background:#ef4444;color:#fff}
.msg-box{padding:10px;border-radius:8px;text-align:center;margin-top:12px;display:none;font-weight:600}
/* Board info */
.bi-section{border-radius:12px;padding:18px;color:#fff;margin-bottom:14px}
.bi-title{font-size:1.1em;font-weight:700;margin-bottom:12px}
.bi-grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(160px,1fr));gap:12px}
.bi-item-label{font-size:.8em;opacity:.85}
.bi-item-value{font-size:1.1em;font-weight:700;word-break:break-all}
.bi-chip{background:linear-gradient(135deg,#667eea,#764ba2)}
.bi-mem{background:linear-gradient(135deg,#f093fb,#f5576c)}
.bi-sys{background:linear-gradient(135deg,#4facfe,#00f2fe);color:#0c2340}
.bi-sketch{background:linear-gradient(135deg,#fa709a,#fee140);color:#3b1a00}
.bi-btn{padding:11px 28px;background:#3b82f6;color:#fff;border:none;border-radius:8px;font-weight:700;cursor:pointer;font-size:.95em;display:block;margin:0 auto 20px}
/* Serial log */
.serial-toolbar{display:flex;flex-wrap:wrap;gap:9px;align-items:center;margin-bottom:12px}
.serial-toolbar .btn{flex:0 0 auto;padding:9px 16px}
.serial-check{display:flex;align-items:center;gap:7px;color:#475569;font-size:.9em;font-weight:600}
.serial-status{margin-left:auto;color:#64748b;font-size:.85em}
.serial-terminal{height:430px;overflow:auto;background:#07111f;color:#d1fae5;border:1px solid #334155;border-radius:10px;padding:14px;font:13px/1.5 Consolas,'Courier New',monospace;white-space:pre-wrap;overflow-wrap:anywhere;tab-size:2}
/* Hardware */
.hw-summary{background:#eef2ff;color:#3730a3;padding:12px 16px;border-radius:8px;margin-bottom:18px;font-weight:600}
.hw-grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(220px,1fr));gap:14px}
.hw-check{display:flex;align-items:center;gap:9px;padding:11px 0;color:#1e293b;font-weight:600}
.gpio-grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(110px,1fr));gap:8px;margin-top:12px}
.gpio-item{padding:9px;border-radius:7px;background:#f1f5f9;color:#334155;font-size:.82em}
.gpio-used{background:#dbeafe;color:#1e40af;font-weight:700}
.gpio-reserved{background:#fee2e2;color:#991b1b}
.gpio-warning{border:1px solid #f59e0b}
.pinout-wrap{margin-top:24px;padding:18px;background:#f8fafc;border:1px solid #e2e8f0;border-radius:12px;overflow-x:auto}
.pinout-title{text-align:center;color:#1e293b;font-weight:700;margin-bottom:4px}
.pinout-note{text-align:center;color:#64748b;font-size:.8em;margin-bottom:14px}
.pinout-board{display:grid;grid-template-columns:minmax(150px,1fr) minmax(150px,220px) minmax(150px,1fr);gap:10px;align-items:stretch;min-width:500px;max-width:720px;margin:0 auto}
.pin-column{display:flex;flex-direction:column;justify-content:space-around;gap:4px}
.board-pin{display:flex;align-items:center;gap:7px;min-height:28px;padding:3px 7px;border-radius:6px;background:#e2e8f0;color:#334155;font-size:.76em}
.pin-column-left .board-pin{justify-content:flex-end;text-align:right}
.pin-column-right .board-pin{justify-content:flex-start}
.pin-pad{width:13px;height:13px;border-radius:50%;background:#94a3b8;border:2px solid #fff;box-shadow:0 0 0 1px #64748b;flex:0 0 auto}
.pin-name{font-weight:700}.pin-role{font-size:.9em;opacity:.8}
.board-pin.used{background:#dbeafe;color:#1e40af}.board-pin.used .pin-pad{background:#2563eb}
.board-pin.reserved{background:#fee2e2;color:#991b1b}.board-pin.reserved .pin-pad{background:#dc2626}
.board-pin.warning{outline:1px solid #f59e0b}.board-pin.warning .pin-pad{background:#f59e0b}
.board-pin.power{background:#dcfce7;color:#166534}.board-pin.power .pin-pad{background:#16a34a}
.board-pin.control{background:#f1f5f9;color:#475569}.board-pin.control .pin-pad{background:#64748b}
.board-body{position:relative;min-height:470px;border:5px solid #075985;border-radius:15px;background:linear-gradient(90deg,#0369a1,#0284c7 48%,#0369a1);box-shadow:inset 0 0 0 2px rgba(255,255,255,.15),0 8px 20px rgba(15,23,42,.18);overflow:hidden}
.board-antenna{height:58px;margin:12px 18px 0;border-radius:8px 8px 3px 3px;background:repeating-linear-gradient(90deg,#d4a72c 0 5px,#111827 5px 9px);border:3px solid #cbd5e1}
.board-chip{width:74%;height:105px;margin:35px auto 0;border-radius:8px;background:#111827;border:4px solid #475569;color:#e2e8f0;display:flex;align-items:center;justify-content:center;text-align:center;font-size:.8em;font-weight:700;line-height:1.4;box-shadow:0 5px 12px rgba(0,0,0,.3)}
.board-profile{position:absolute;left:8px;right:8px;bottom:82px;text-align:center;color:#e0f2fe;font-size:.7em;font-weight:700}
.board-usb{position:absolute;bottom:-4px;left:50%;transform:translateX(-50%);width:72px;height:45px;border-radius:8px 8px 0 0;background:linear-gradient(#d1d5db,#64748b);border:4px solid #334155}
.board-led{position:absolute;width:10px;height:10px;border-radius:50%;background:#22c55e;box-shadow:0 0 7px #86efac;right:16px;bottom:62px}
.pinout-legend{display:flex;flex-wrap:wrap;justify-content:center;gap:8px 14px;margin-top:14px;font-size:.78em;color:#475569}
.legend-dot{display:inline-block;width:10px;height:10px;border-radius:50%;margin-right:4px;vertical-align:-1px}
@media(max-width:700px){
  body{padding:8px}.container{padding:16px}.tabs{overflow-x:auto}.tab-btn{padding:9px 12px;white-space:nowrap}
  .pinout-board{grid-template-columns:145px 135px 145px;gap:5px}.board-body{min-height:450px}
  .serial-status{width:100%;margin-left:0}.serial-terminal{height:360px}
}
</style>
</head>
<body>
<div class="container">
  <h1>&#127968; ESP32 HA Kit</h1>
  <p class="subtitle">MQTT &bull; Home Assistant &bull; DHT11 &bull; PIR &bull; Releu &bull; OLED</p>

  <div class="tabs">
    <button class="tab-btn active" onclick="showTab('monitor',this)">&#128202; Dashboard</button>
    <button class="tab-btn" onclick="showTab('mqtt',this)">&#128268; MQTT</button>
    <button class="tab-btn" onclick="showTab('wifi',this)">&#128225; WiFi</button>
    <button class="tab-btn" onclick="showTab('hardware',this)">&#128295; Hardware</button>
    <button class="tab-btn" onclick="showTab('serial',this)">&#9000; Serial Log</button>
    <button class="tab-btn" onclick="showTab('board',this)">&#128203; Info Placa</button>
  </div>

  <!-- DASHBOARD TAB -->
  <div id="monitor-tab" class="tab active">
    <div class="mqtt-bar mqtt-unknown" id="mqttBar">MQTT: se verifica...</div>

    <div class="cards">
      <!-- Temperature -->
      <div class="card card-temp">
        <div class="card-label">Temperatura</div>
        <div class="card-value" id="tempVal">--</div>
        <div class="card-unit">&deg;C &nbsp;|&nbsp; DHT11</div>
      </div>

      <!-- Humidity -->
      <div class="card card-hum">
        <div class="card-label">Umiditate</div>
        <div class="card-value" id="humVal">--</div>
        <div class="card-unit">% &nbsp;|&nbsp; DHT11</div>
      </div>

      <!-- Motion -->
      <div class="card card-motion" id="motionCard">
        <div class="card-label">Senzor Miscare</div>
        <div class="motion-circle" id="motionCircle">&#10003;</div>
        <div id="motionText" style="font-size:1.1em;font-weight:700">Fara miscare</div>
        <div style="font-size:.8em;opacity:.8;margin-top:4px">HC-SR501 &bull; <span id="dashPirPin">GPIO--</span></div>
      </div>

      <!-- Relay -->
      <div class="card card-relay">
        <div class="card-label">Releu SSR</div>
        <div class="relay-state" id="relayVal">OFF</div>
        <div style="font-size:.8em;opacity:.8;margin-bottom:6px"><span id="dashRelayPin">GPIO--</span> &bull; configurabil</div>
        <div class="relay-btns">
          <button onclick="setRelay(1)">&#9889; ON</button>
          <button onclick="setRelay(0)">&#9898; OFF</button>
        </div>
      </div>
    </div>

    <div class="info-box">
      <strong>&#128268; Cablaj hardware:</strong>
      <ul>
        <li>DHT11 DATA &rarr; <span id="wireDht">neconfigurat</span> &nbsp;|&nbsp; VCC=3.3V, GND=GND</li>
        <li>PIR HC-SR501 OUT &rarr; <span id="wirePir">neconfigurat</span></li>
        <li>Releu SSR IN &rarr; <span id="wireRelay">neconfigurat</span></li>
        <li>OLED SSD1306 SDA &rarr; <span id="wireSda">neconfigurat</span> &nbsp;|&nbsp; SCL &rarr; <span id="wireScl">neconfigurat</span></li>
        <li>LED Heartbeat &rarr; <span id="wireHeartbeat">neconfigurat</span></li>
        <li>MQTT publish la fiecare 30s sau la detectie miscare</li>
      </ul>
    </div>
  </div>

  <!-- MQTT TAB -->
  <div id="mqtt-tab" class="tab">
    <div class="mqtt-bar mqtt-unknown" id="mqttCfgBar">Verificare status MQTT...</div>

    <h3 style="margin-bottom:14px;color:#1e293b">Configuratie broker MQTT</h3>
    <p style="margin-bottom:16px;color:#64748b;font-size:.9em">
      Setarile sunt salvate in memoria flash a ESP32 si se aplica imediat dupa salvare.
      Home Assistant trebuie sa aiba un broker MQTT activ (ex. Mosquitto add-on).
    </p>

    <div class="form-group">
      <label class="form-label">Adresa IP broker <span style="color:#ef4444">*</span></label>
      <input type="text" id="mqttBroker" class="form-input" placeholder="ex. 192.168.1.100">
    </div>
    <div class="form-group">
      <label class="form-label">Port</label>
      <input type="number" id="mqttPort" class="form-input" placeholder="1883" min="1" max="65535">
    </div>
    <div class="form-group">
      <label class="form-label">Client ID</label>
      <input type="text" id="mqttClientId" class="form-input" placeholder="esp32-ha-kit">
    </div>
    <div class="form-group">
      <label class="form-label">Utilizator <span style="color:#94a3b8">(optional)</span></label>
      <input type="text" id="mqttUser" class="form-input" placeholder="las gol daca nu e necesar">
    </div>
    <div class="form-group">
      <label class="form-label">Parola <span style="color:#94a3b8">(optional)</span></label>
      <input type="password" id="mqttPass" class="form-input" placeholder="las gol daca nu e necesara">
      <small style="color:#94a3b8;font-size:.82em">Parola nu este afisata dupa salvare din motive de securitate.</small>
    </div>
    <div class="btn-row">
      <button class="btn btn-primary" onclick="saveMqttCfg()">&#128190; Salveaza si aplica</button>
      <button class="btn" style="background:#64748b;color:#fff;flex:0 0 auto" onclick="loadMqttCfg()">&#8635;</button>
    </div>
    <div class="msg-box" id="mqttMsg"></div>

    <div class="info-box" style="margin-top:18px">
      <strong>&#8505; Topice MQTT folosite:</strong>
      <ul>
        <li><code>esp32kit/state</code> &mdash; JSON: temperatura, umiditate, miscare, releu</li>
        <li><code>esp32kit/relay/state</code> &mdash; starea releului (ON / OFF)</li>
        <li><code>esp32kit/relay/command</code> &mdash; comanda releu din HA (ON / OFF)</li>
        <li>Auto-discovery Home Assistant publicat la fiecare conectare la broker</li>
      </ul>
    </div>
  </div>

  <!-- WIFI TAB -->
  <div id="wifi-tab" class="tab">
    <div class="wifi-status wifi-ap" id="wifiStatusBox">
      Verificare status WiFi...
    </div>

    <h3 style="margin-bottom:14px;color:#1e293b">Conectare la reteaua WiFi</h3>
    <p style="margin-bottom:16px;color:#64748b;font-size:.9em">
      Salveaza credentialele WiFi &ndash; ESP32 se va reconecta automat la pornire.
      Dupa salvare, brokerul MQTT devine accesibil.
    </p>

    <div class="form-group">
      <label class="form-label">SSID (Nume retea)</label>
      <input type="text" id="wifiSSID" class="form-input" placeholder="Nume retea WiFi">
    </div>
    <div class="form-group">
      <label class="form-label">Parola WiFi</label>
      <input type="password" id="wifiPass" class="form-input" placeholder="Parola WiFi">
    </div>
    <div class="btn-row">
      <button class="btn btn-primary" onclick="saveWifi()">&#128190; Salveaza si conecteaza</button>
      <button class="btn btn-danger" style="flex:0 0 auto" onclick="clearWifi()">&#128465;</button>
    </div>
    <div class="msg-box" id="wifiMsg"></div>

    <div class="info-box" style="margin-top:18px">
      <strong>&#8505; Info:</strong>
      <ul>
        <li>Configuratia este salvata in memoria flash (NVS)</li>
        <li>Daca conectarea esueaza, ESP32 revine in mod Access Point</li>
        <li>Access Point: <strong>ESP32_HAKit</strong> &nbsp;/&nbsp; <strong>12345678</strong></li>
        <li>Adresa IP AP: <strong>192.168.4.1</strong></li>
      </ul>
    </div>
  </div>

  <!-- HARDWARE TAB -->
  <div id="hardware-tab" class="tab">
    <div class="hw-summary" id="hwSummary">Se citeste configuratia hardware...</div>
    <p style="margin-bottom:16px;color:#64748b;font-size:.9em">
      Selecteaza GPIO pentru fiecare functie. Optiunea <strong>Dezactivat</strong> marcheaza
      perifericul ca neconfigurat. Modificarile sunt validate, salvate in NVS si aplicate dupa restart.
    </p>

    <div class="hw-grid">
      <div class="form-group"><label class="form-label">DHT11 DATA</label><select id="hwDht" class="form-input"></select></div>
      <div class="form-group"><label class="form-label">PIR OUT</label><select id="hwPir" class="form-input"></select></div>
      <div class="form-group"><label class="form-label">Releu IN</label><select id="hwRelay" class="form-input"></select></div>
      <div class="form-group"><label class="form-label">LED heartbeat</label><select id="hwHeartbeat" class="form-input"></select></div>
      <div class="form-group"><label class="form-label">OLED SDA</label><select id="hwSda" class="form-input"></select></div>
      <div class="form-group"><label class="form-label">OLED SCL</label><select id="hwScl" class="form-input"></select></div>
      <div class="form-group">
        <label class="form-label">Adresa OLED</label>
        <select id="hwOledAddress" class="form-input"><option value="60">0x3C</option><option value="61">0x3D</option></select>
      </div>
      <label class="hw-check"><input type="checkbox" id="hwRelayLow"> Releu activ pe LOW</label>
    </div>

    <div class="btn-row">
      <button class="btn btn-primary" onclick="saveHardware()">&#128190; Salveaza si reporneste</button>
      <button class="btn btn-danger" style="flex:0 0 auto" onclick="resetHardware()">Valori implicite</button>
    </div>
    <div class="msg-box" id="hwMsg"></div>

    <div class="pinout-wrap">
      <div class="pinout-title" id="pinoutTitle">Dispunere pini</div>
      <div class="pinout-note" id="pinoutNote">Vedere de sus. Conectorul USB este reprezentat in partea de jos.</div>
      <div class="pinout-board">
        <div class="pin-column pin-column-left" id="pinoutLeft"></div>
        <div class="board-body">
          <div class="board-antenna"></div>
          <div class="board-chip" id="pinoutChip">ESP32</div>
          <div class="board-profile" id="pinoutProfile">Profil placa</div>
          <div class="board-led"></div>
          <div class="board-usb"></div>
        </div>
        <div class="pin-column pin-column-right" id="pinoutRight"></div>
      </div>
      <div class="pinout-legend">
        <span><i class="legend-dot" style="background:#2563eb"></i>Configurat</span>
        <span><i class="legend-dot" style="background:#94a3b8"></i>Liber</span>
        <span><i class="legend-dot" style="background:#dc2626"></i>Indisponibil</span>
        <span><i class="legend-dot" style="background:#f59e0b"></i>Avertisment</span>
        <span><i class="legend-dot" style="background:#16a34a"></i>Alimentare</span>
      </div>
    </div>

    <h3 style="margin-top:24px;color:#1e293b">Inventar GPIO</h3>
    <p style="color:#64748b;font-size:.85em;margin-top:4px">Pinii de flash si cei neexpusi pe placa sunt blocati. Pinii de boot sau USB sunt permisi, dar marcati cu avertisment.</p>
    <div class="gpio-grid" id="gpioGrid"></div>
  </div>

  <!-- SERIAL LOG TAB -->
  <div id="serial-tab" class="tab">
    <div class="serial-toolbar">
      <button class="btn btn-primary" id="serialPauseBtn" onclick="toggleSerialPause()">Pauza</button>
      <button class="btn" style="background:#64748b;color:#fff" onclick="downloadSerialLog()">Descarca</button>
      <button class="btn btn-danger" onclick="clearSerialLog()">Sterge buffer</button>
      <label class="serial-check"><input type="checkbox" id="serialAutoScroll" checked> Auto-scroll</label>
      <span class="serial-status" id="serialStatus">Inactiv</span>
    </div>
    <pre class="serial-terminal" id="serialOutput">Deschide tab-ul pentru a citi jurnalul serial...</pre>
    <div class="info-box" style="margin-top:14px">
      Afisarea este o copie a mesajelor aplicatiei trimise pe USB serial la 115200 baud.
      Placa pastreaza circular ultimii 8 KB in RAM; mesajele vechi sunt suprascrise automat.
    </div>
  </div>

  <!-- BOARD INFO TAB -->
  <div id="board-tab" class="tab">
    <button class="bi-btn" onclick="loadBoardInfo()">&#128260; Incarca informatii placa</button>
    <div id="boardContent" style="display:none">
      <div class="bi-section bi-chip">
        <div class="bi-title">&#128295; Chip</div>
        <div class="bi-grid">
          <div><div class="bi-item-label">Model</div><div class="bi-item-value" id="biChipModel">-</div></div>
          <div><div class="bi-item-label">Profil build</div><div class="bi-item-value" id="biBuildProfile">-</div></div>
          <div><div class="bi-item-label">Revizie</div><div class="bi-item-value" id="biChipRev">-</div></div>
          <div><div class="bi-item-label">Nuclee CPU</div><div class="bi-item-value" id="biCores">-</div></div>
          <div><div class="bi-item-label">Frecventa</div><div class="bi-item-value" id="biFreq">-</div></div>
        </div>
      </div>
      <div class="bi-section bi-mem">
        <div class="bi-title">&#128190; Memorie</div>
        <div class="bi-grid">
          <div><div class="bi-item-label">Flash</div><div class="bi-item-value" id="biFlash">-</div></div>
          <div><div class="bi-item-label">PSRAM</div><div class="bi-item-value" id="biPsram">-</div></div>
          <div><div class="bi-item-label">Heap liber</div><div class="bi-item-value" id="biFreeHeap">-</div></div>
          <div><div class="bi-item-label">Heap min liber</div><div class="bi-item-value" id="biMinHeap">-</div></div>
        </div>
      </div>
      <div class="bi-section bi-sys">
        <div class="bi-title">&#9881;&#65039; Sistem</div>
        <div class="bi-grid">
          <div><div class="bi-item-label">SDK Version</div><div class="bi-item-value" id="biSdk">-</div></div>
          <div><div class="bi-item-label">MAC Address</div><div class="bi-item-value" id="biMac">-</div></div>
          <div><div class="bi-item-label">Uptime</div><div class="bi-item-value" id="biUptime">-</div></div>
          <div><div class="bi-item-label">Reset Reason</div><div class="bi-item-value" id="biReset">-</div></div>
        </div>
      </div>
      <div class="bi-section bi-sketch">
        <div class="bi-title">&#128230; Program</div>
        <div class="bi-grid">
          <div><div class="bi-item-label">Sketch Size</div><div class="bi-item-value" id="biSketchSize">-</div></div>
          <div><div class="bi-item-label">Spatiu liber</div><div class="bi-item-value" id="biSketchFree">-</div></div>
          <div><div class="bi-item-label">MD5</div><div class="bi-item-value" id="biMd5">-</div></div>
        </div>
      </div>
    </div>
  </div>

  <div class="footer">ESP32 MQTT Home Assistant DIY Kit &bull; OLED + DHT11 + PIR + SSR Relay<br>&copy; Automatic House Systems</div>
</div>

<script>
function showTab(name, btn) {
  document.querySelectorAll('.tab').forEach(t => t.classList.remove('active'));
  document.querySelectorAll('.tab-btn').forEach(b => b.classList.remove('active'));
  document.getElementById(name + '-tab').classList.add('active');
  btn.classList.add('active');
  if (name === 'wifi')  loadWifiStatus();
  if (name === 'mqtt')  loadMqttCfg();
  if (name === 'hardware') loadHardwareCfg();
  serialTabActive = name === 'serial';
  if (serialTabActive) loadSerialLog();
}

// ---- Dashboard ----
function updateData() {
  fetch('/data')
    .then(r => r.json())
    .then(d => {
      document.getElementById('tempVal').textContent = d.temperature !== null ? d.temperature : '--';
      document.getElementById('humVal').textContent  = d.humidity    !== null ? d.humidity    : '--';

      var mc = document.getElementById('motionCard');
      var ci = document.getElementById('motionCircle');
      var mt = document.getElementById('motionText');
      if (d.motion) {
        mc.classList.add('alert');
        ci.classList.add('pulse');
        ci.textContent = '!';
        mt.textContent = 'MISCARE DETECTATA!';
      } else {
        mc.classList.remove('alert');
        ci.classList.remove('pulse');
        ci.textContent = '\u2713';
        mt.textContent = 'Fara miscare';
      }

      document.getElementById('relayVal').textContent = d.relay ? 'ON' : 'OFF';

      var bar = document.getElementById('mqttBar');
      if (d.mqtt_connected) {
        bar.className = 'mqtt-bar mqtt-ok';
        bar.textContent = 'MQTT: Conectat la broker \u2714';
      } else {
        bar.className = 'mqtt-bar mqtt-err';
        bar.textContent = 'MQTT: Deconectat \u2014 verifica IP broker si WiFi';
      }
    })
    .catch(function() {
      document.getElementById('mqttBar').className = 'mqtt-bar mqtt-unknown';
      document.getElementById('mqttBar').textContent = 'Dashboard: eroare citire date...';
    });
}
setInterval(updateData, 2000);
updateData();

function setRelay(s) {
  fetch('/api/relay?state=' + s)
    .catch(function(){});
}

// ---- Serial Log ----
var serialCursor = 0;
var serialTabActive = false;
var serialPaused = false;
var serialLoading = false;

function loadSerialLog() {
  if (!serialTabActive || serialPaused || serialLoading) return;
  serialLoading = true;

  fetch('/api/serial_log?since=' + serialCursor, {cache:'no-store'})
    .then(function(r) {
      if (!r.ok) throw new Error('HTTP ' + r.status);
      var next = Number(r.headers.get('X-Log-Next'));
      var dropped = r.headers.get('X-Log-Dropped') === '1';
      return r.text().then(function(text) {
        return {text:text, next:next, dropped:dropped};
      });
    })
    .then(function(data) {
      var output = document.getElementById('serialOutput');
      if (serialCursor === 0) output.textContent = '';
      if (data.dropped)
        output.textContent += '\n[Web] O parte din jurnal a fost suprascrisa in buffer.\n';
      output.textContent += data.text;
      if (output.textContent.length > 50000)
        output.textContent = output.textContent.slice(-50000);
      if (Number.isFinite(data.next)) serialCursor = data.next;
      document.getElementById('serialStatus').textContent =
        'Conectat | cursor ' + serialCursor + ' | buffer placa 8 KB';
      if (document.getElementById('serialAutoScroll').checked)
        output.scrollTop = output.scrollHeight;
    })
    .catch(function() {
      document.getElementById('serialStatus').textContent = 'Eroare la citirea jurnalului';
    })
    .finally(function() { serialLoading = false; });
}

function toggleSerialPause() {
  serialPaused = !serialPaused;
  document.getElementById('serialPauseBtn').textContent = serialPaused ? 'Continua' : 'Pauza';
  document.getElementById('serialStatus').textContent =
    serialPaused ? 'Actualizare oprita' : 'Se reconecteaza...';
  if (!serialPaused) loadSerialLog();
}

function clearSerialLog() {
  fetch('/api/serial_log/clear', {method:'POST'})
    .then(function(r) {
      if (!r.ok) throw new Error('HTTP ' + r.status);
      serialCursor = 0;
      document.getElementById('serialOutput').textContent = '';
      document.getElementById('serialStatus').textContent = 'Buffer sters';
      loadSerialLog();
    })
    .catch(function() {
      document.getElementById('serialStatus').textContent = 'Stergerea bufferului a esuat';
    });
}

function downloadSerialLog() {
  var content = document.getElementById('serialOutput').textContent;
  var blob = new Blob([content], {type:'text/plain;charset=utf-8'});
  var link = document.createElement('a');
  link.href = URL.createObjectURL(blob);
  link.download = 'esp32-serial-log.txt';
  link.click();
  setTimeout(function() { URL.revokeObjectURL(link.href); }, 1000);
}

setInterval(loadSerialLog, 1000);

// ---- MQTT Config ----
function loadMqttCfg() {
  fetch('/api/mqtt_config')
    .then(r => r.json())
    .then(d => {
      document.getElementById('mqttBroker').value   = d.broker   || '';
      document.getElementById('mqttPort').value     = d.port     || 1883;
      document.getElementById('mqttClientId').value = d.client_id || 'esp32-ha-kit';
      document.getElementById('mqttUser').value     = d.user     || '';
      document.getElementById('mqttPass').value     = '';  // parola nu se returneaza

      var bar = document.getElementById('mqttCfgBar');
      if (d.connected) {
        bar.className = 'mqtt-bar mqtt-ok';
        bar.textContent = 'MQTT: Conectat la ' + d.broker + ':' + d.port + ' \u2714';
      } else {
        bar.className = 'mqtt-bar mqtt-err';
        bar.textContent = 'MQTT: Deconectat de la ' + d.broker + ':' + d.port;
      }
    })
    .catch(function() {
      document.getElementById('mqttCfgBar').className = 'mqtt-bar mqtt-unknown';
      document.getElementById('mqttCfgBar').textContent = 'Eroare citire configuratie MQTT';
    });
}

function saveMqttCfg() {
  var broker   = document.getElementById('mqttBroker').value.trim();
  var port     = document.getElementById('mqttPort').value.trim() || '1883';
  var clientId = document.getElementById('mqttClientId').value.trim() || 'esp32-ha-kit';
  var user     = document.getElementById('mqttUser').value.trim();
  var pass     = document.getElementById('mqttPass').value;

  if (!broker) { showMqttMsg('Adresa IP broker este obligatorie!', 'err'); return; }

  showMqttMsg('Se salveaza...', 'info');

  var body = 'broker=' + encodeURIComponent(broker)
           + '&port='      + encodeURIComponent(port)
           + '&client_id=' + encodeURIComponent(clientId)
           + '&user='      + encodeURIComponent(user)
           + '&pass='      + encodeURIComponent(pass);

  fetch('/api/mqtt_config', {
    method: 'POST',
    headers: {'Content-Type': 'application/x-www-form-urlencoded'},
    body: body
  })
  .then(r => r.json())
  .then(d => {
    showMqttMsg(d.message || (d.ok ? 'Salvat!' : 'Eroare!'), d.ok ? 'ok' : 'err');
    if (d.ok) setTimeout(loadMqttCfg, 3000);
  })
  .catch(function() { showMqttMsg('Eroare la salvare!', 'err'); });
}

function showMqttMsg(txt, type) {
  var m = document.getElementById('mqttMsg');
  m.textContent = txt;
  m.style.display = 'block';
  m.style.background = type === 'ok' ? '#d1fae5' : type === 'err' ? '#fee2e2' : '#dbeafe';
  m.style.color      = type === 'ok' ? '#065f46' : type === 'err' ? '#991b1b' : '#1e40af';
  if (type !== 'info') setTimeout(function(){ m.style.display='none'; }, 5000);
}

// ---- WiFi ----
function loadWifiStatus() {
  fetch('/wifi_status')
    .then(r => r.json())
    .then(d => {
      var box = document.getElementById('wifiStatusBox');
      if (d.connected) {
        box.className = 'wifi-status wifi-ok';
        box.innerHTML = '&#10003; Conectat la <strong>' + d.ssid + '</strong> &nbsp;|&nbsp; IP: ' + d.ip;
      } else {
        box.className = 'wifi-status wifi-ap';
        box.innerHTML = '&#128225; Mod Access Point &nbsp;|&nbsp; IP: ' + d.ip;
      }
      if (d.saved_ssid) document.getElementById('wifiSSID').value = d.saved_ssid;
    })
    .catch(function(){});
}

function saveWifi() {
  var ssid = document.getElementById('wifiSSID').value;
  var pass = document.getElementById('wifiPass').value;
  if (!ssid) { showMsg('Introduceti SSID!', 'err'); return; }
  showMsg('Se salveaza...', 'info');
  fetch('/wifi_config', {
    method: 'POST',
    headers: {'Content-Type': 'application/x-www-form-urlencoded'},
    body: 'ssid=' + encodeURIComponent(ssid) + '&password=' + encodeURIComponent(pass)
  })
  .then(r => r.json())
  .then(d => { showMsg(d.message, d.success ? 'ok' : 'err'); })
  .catch(function() { showMsg('Eroare la salvare!', 'err'); });
}

function clearWifi() {
  if (!confirm('Stergeti configuratia WiFi salvata?')) return;
  fetch('/wifi_clear')
    .then(r => r.json())
    .then(d => {
      showMsg(d.message, 'ok');
      document.getElementById('wifiSSID').value = '';
      document.getElementById('wifiPass').value = '';
      setTimeout(loadWifiStatus, 800);
    })
    .catch(function() { showMsg('Eroare!', 'err'); });
}

function showMsg(txt, type) {
  var m = document.getElementById('wifiMsg');
  m.textContent = txt;
  m.style.display = 'block';
  m.style.background = type === 'ok' ? '#d1fae5' : type === 'err' ? '#fee2e2' : '#dbeafe';
  m.style.color      = type === 'ok' ? '#065f46' : type === 'err' ? '#991b1b' : '#1e40af';
  if (type !== 'info') setTimeout(function(){ m.style.display='none'; }, 4000);
}

// ---- Hardware Config ----
var hardwareData = null;

var boardLayouts = {
  wroom: {
    title: 'ESP32 DevKit / WROOM - 30 pini',
    chip: 'ESP32<br>WROOM-32',
    note: 'Vedere de sus, USB jos. Profilul generic WROOM foloseste aranjamentul comun al placii cu 30 de pini.',
    left: [
      {label:'EN',type:'control'}, {label:'GPIO36 / VP',gpio:36}, {label:'GPIO39 / VN',gpio:39},
      {label:'GPIO34',gpio:34}, {label:'GPIO35',gpio:35}, {label:'GPIO32',gpio:32},
      {label:'GPIO33',gpio:33}, {label:'GPIO25',gpio:25}, {label:'GPIO26',gpio:26},
      {label:'GPIO27',gpio:27}, {label:'GPIO14',gpio:14}, {label:'GPIO12',gpio:12},
      {label:'GPIO13',gpio:13}, {label:'GND',type:'power'}, {label:'VIN / 5V',type:'power'}
    ],
    right: [
      {label:'GPIO23',gpio:23}, {label:'GPIO22',gpio:22}, {label:'TX0 / GPIO1',gpio:1},
      {label:'RX0 / GPIO3',gpio:3}, {label:'GPIO21',gpio:21}, {label:'GPIO19',gpio:19},
      {label:'GPIO18',gpio:18}, {label:'GPIO5',gpio:5}, {label:'TX2 / GPIO17',gpio:17},
      {label:'RX2 / GPIO16',gpio:16}, {label:'GPIO4',gpio:4}, {label:'GPIO2',gpio:2},
      {label:'GPIO15',gpio:15}, {label:'GND',type:'power'}, {label:'3V3',type:'power'}
    ]
  },
  c3: {
    title: 'ESP32-C3-DevKitM-1 - headere J1 / J3',
    chip: 'ESP32-C3<br>MINI-1',
    note: 'Vedere de sus, USB jos. Ordinea pinilor urmeaza headerele J1 si J3 ale ESP32-C3-DevKitM-1.',
    left: [
      {label:'GND',type:'power'}, {label:'3V3',type:'power'}, {label:'3V3',type:'power'},
      {label:'GPIO2',gpio:2}, {label:'GPIO3',gpio:3}, {label:'GND',type:'power'},
      {label:'RST',type:'control'}, {label:'GND',type:'power'}, {label:'GPIO0',gpio:0},
      {label:'GPIO1',gpio:1}, {label:'GPIO10',gpio:10}, {label:'GND',type:'power'},
      {label:'5V',type:'power'}, {label:'5V',type:'power'}, {label:'GND',type:'power'}
    ],
    right: [
      {label:'GND',type:'power'}, {label:'TX / GPIO21',gpio:21}, {label:'RX / GPIO20',gpio:20},
      {label:'GND',type:'power'}, {label:'GPIO9',gpio:9}, {label:'GPIO8 / RGB',gpio:8},
      {label:'GND',type:'power'}, {label:'GPIO7',gpio:7}, {label:'GPIO6',gpio:6},
      {label:'GPIO5',gpio:5}, {label:'GPIO4',gpio:4}, {label:'GND',type:'power'},
      {label:'USB D- / GPIO18',gpio:18}, {label:'USB D+ / GPIO19',gpio:19},
      {label:'GND',type:'power'}
    ]
  },
  s3: {
    title: 'ESP32-S3-DevKitC-1 - 44 pini, headere J1 / J3',
    chip: 'ESP32-S3<br>WROOM-1',
    note: 'Vedere de sus, USB jos. Ordinea celor 44 de pini urmeaza headerele J1 si J3 ale ESP32-S3-DevKitC-1 v1.1.',
    left: [
      {label:'3V3',type:'power'}, {label:'3V3',type:'power'}, {label:'RST',type:'control'},
      {label:'GPIO4',gpio:4}, {label:'GPIO5',gpio:5}, {label:'GPIO6',gpio:6},
      {label:'GPIO7',gpio:7}, {label:'GPIO15',gpio:15}, {label:'GPIO16',gpio:16},
      {label:'GPIO17',gpio:17}, {label:'GPIO18',gpio:18}, {label:'GPIO8 / SDA',gpio:8},
      {label:'GPIO3',gpio:3}, {label:'GPIO46',gpio:46}, {label:'GPIO9 / SCL',gpio:9},
      {label:'GPIO10',gpio:10}, {label:'GPIO11',gpio:11}, {label:'GPIO12',gpio:12},
      {label:'GPIO13',gpio:13}, {label:'GPIO14',gpio:14}, {label:'5V',type:'power'},
      {label:'GND',type:'power'}
    ],
    right: [
      {label:'GND',type:'power'}, {label:'TX / GPIO43',gpio:43}, {label:'RX / GPIO44',gpio:44},
      {label:'GPIO1',gpio:1}, {label:'GPIO2',gpio:2}, {label:'GPIO42',gpio:42},
      {label:'GPIO41',gpio:41}, {label:'GPIO40',gpio:40}, {label:'GPIO39',gpio:39},
      {label:'GPIO38 / RGB',gpio:38}, {label:'GPIO37',gpio:37}, {label:'GPIO36',gpio:36},
      {label:'GPIO35',gpio:35}, {label:'GPIO0 / BOOT',gpio:0}, {label:'GPIO45',gpio:45},
      {label:'GPIO48 / RGB',gpio:48}, {label:'GPIO47',gpio:47}, {label:'GPIO21',gpio:21},
      {label:'USB D+ / GPIO20',gpio:20}, {label:'USB D- / GPIO19',gpio:19},
      {label:'GND',type:'power'}, {label:'GND',type:'power'}
    ]
  },
  c6: {
    title: 'ESP32-C6-DevKitC-1 - headere J1 / J3',
    chip: 'ESP32-C6<br>WROOM-1',
    note: 'Vedere de sus, USB jos. Ordinea pinilor urmeaza headerele J1 si J3 ale ESP32-C6-DevKitC-1.',
    left: [
      {label:'3V3',type:'power'}, {label:'RST',type:'control'}, {label:'GPIO4',gpio:4},
      {label:'GPIO5',gpio:5}, {label:'GPIO6',gpio:6}, {label:'GPIO7',gpio:7},
      {label:'GPIO0',gpio:0}, {label:'GPIO1',gpio:1}, {label:'GPIO8 / RGB',gpio:8},
      {label:'GPIO10',gpio:10}, {label:'GPIO11',gpio:11}, {label:'GPIO2',gpio:2},
      {label:'GPIO3',gpio:3}, {label:'5V',type:'power'}, {label:'GND',type:'power'},
      {label:'NC',type:'control'}
    ],
    right: [
      {label:'GND',type:'power'}, {label:'TX / GPIO16',gpio:16}, {label:'RX / GPIO17',gpio:17},
      {label:'GPIO15',gpio:15}, {label:'GPIO23',gpio:23}, {label:'GPIO22',gpio:22},
      {label:'GPIO21',gpio:21}, {label:'GPIO20',gpio:20}, {label:'GPIO19',gpio:19},
      {label:'GPIO18',gpio:18}, {label:'GPIO9',gpio:9}, {label:'GND',type:'power'},
      {label:'USB D+ / GPIO13',gpio:13}, {label:'USB D- / GPIO12',gpio:12},
      {label:'GND',type:'power'}, {label:'NC',type:'control'}
    ]
  }
};

function gpioLabel(pin) {
  return Number(pin) >= 0 ? 'GPIO' + pin : 'neconfigurat';
}

function selectedPinRoles() {
  var roles = {};
  var fields = [
    ['hwDht', 'DHT'], ['hwPir', 'PIR'], ['hwRelay', 'releu'],
    ['hwHeartbeat', 'heartbeat'], ['hwSda', 'I2C SDA'], ['hwScl', 'I2C SCL']
  ];
  fields.forEach(function(field) {
    var element = document.getElementById(field[0]);
    var pin = element && element.options.length ? Number(element.value) : -1;
    if (pin >= 0) roles[pin] = field[1];
  });
  return roles;
}

function boardLayoutFor(profile) {
  if (profile.indexOf('S3') >= 0) return boardLayouts.s3;
  if (profile.indexOf('C3') >= 0) return boardLayouts.c3;
  if (profile.indexOf('C6') >= 0) return boardLayouts.c6;
  return boardLayouts.wroom;
}

function createBoardPin(pin, side, pinInfo, roles) {
  var item = document.createElement('div');
  item.className = 'board-pin';
  var info = pin.gpio !== undefined ? pinInfo[pin.gpio] : null;
  var role = pin.gpio !== undefined ? roles[pin.gpio] : '';

  if (pin.type) item.classList.add(pin.type);
  if (info && info.reserved) item.classList.add('reserved');
  if (role) item.classList.add('used');
  if (info && info.warning) item.classList.add('warning');

  var pad = document.createElement('span');
  pad.className = 'pin-pad';
  var text = document.createElement('span');
  var name = document.createElement('span');
  name.className = 'pin-name';
  name.textContent = pin.label;
  text.appendChild(name);
  if (role) {
    var roleText = document.createElement('span');
    roleText.className = 'pin-role';
    roleText.textContent = ' | ' + role;
    text.appendChild(roleText);
  }

  if (info) {
    var details = [];
    if (info.restriction) details.push(info.restriction);
    if (info.warning) details.push(info.warning);
    if (!info.output) details.push('doar intrare');
    item.title = details.join(' | ');
  }

  if (side === 'left') {
    item.appendChild(text);
    item.appendChild(pad);
  } else {
    item.appendChild(pad);
    item.appendChild(text);
  }
  return item;
}

function renderBoardPinout() {
  if (!hardwareData) return;
  var layout = boardLayoutFor(hardwareData.profile || '');
  var roles = selectedPinRoles();
  var pinInfo = {};
  hardwareData.pins.forEach(function(pin) { pinInfo[pin.pin] = pin; });

  document.getElementById('pinoutTitle').textContent = layout.title;
  document.getElementById('pinoutNote').textContent = layout.note;
  document.getElementById('pinoutChip').innerHTML = layout.chip;
  document.getElementById('pinoutProfile').textContent = hardwareData.profile;

  var left = document.getElementById('pinoutLeft');
  var right = document.getElementById('pinoutRight');
  left.innerHTML = '';
  right.innerHTML = '';
  layout.left.forEach(function(pin) { left.appendChild(createBoardPin(pin, 'left', pinInfo, roles)); });
  layout.right.forEach(function(pin) { right.appendChild(createBoardPin(pin, 'right', pinInfo, roles)); });
}

function bindPinoutPreview() {
  ['hwDht','hwPir','hwRelay','hwHeartbeat','hwSda','hwScl'].forEach(function(id) {
    document.getElementById(id).onchange = renderBoardPinout;
  });
}

function fillPinSelect(id, pins, value, needsOutput) {
  var select = document.getElementById(id);
  select.innerHTML = '';
  select.add(new Option('Dezactivat / neconfigurat', '-1'));
  pins.forEach(function(p) {
    if (p.reserved || (needsOutput && !p.output)) return;
    var label = 'GPIO' + p.pin;
    if (!p.output) label += ' (doar intrare)';
    if (p.role && p.pin !== Number(value)) label += ' - ocupat: ' + p.role;
    if (p.warning) label += ' - ATENTIE: ' + p.warning;
    select.add(new Option(label, String(p.pin)));
  });
  select.value = String(value);
}

function updateHardwareLabels(d) {
  document.getElementById('dashPirPin').textContent = gpioLabel(d.pir_pin);
  document.getElementById('dashRelayPin').textContent = gpioLabel(d.relay_pin);
  document.getElementById('wireDht').textContent = gpioLabel(d.dht_pin);
  document.getElementById('wirePir').textContent = gpioLabel(d.pir_pin);
  document.getElementById('wireRelay').textContent = gpioLabel(d.relay_pin);
  document.getElementById('wireHeartbeat').textContent = gpioLabel(d.heartbeat_pin);
  document.getElementById('wireSda').textContent = gpioLabel(d.sda_pin);
  document.getElementById('wireScl').textContent = gpioLabel(d.scl_pin);
}

function loadHardwareCfg() {
  fetch('/api/hardware_config')
    .then(r => r.json())
    .then(d => {
      hardwareData = d;
      document.getElementById('hwSummary').textContent =
        'Cip: ' + d.target + ' | Profil build: ' + d.profile + ' | ' + d.gpio_count + ' pozitii GPIO';
      fillPinSelect('hwDht', d.pins, d.dht_pin, true);
      fillPinSelect('hwPir', d.pins, d.pir_pin, false);
      fillPinSelect('hwRelay', d.pins, d.relay_pin, true);
      fillPinSelect('hwHeartbeat', d.pins, d.heartbeat_pin, true);
      fillPinSelect('hwSda', d.pins, d.sda_pin, true);
      fillPinSelect('hwScl', d.pins, d.scl_pin, true);
      document.getElementById('hwRelayLow').checked = !!d.relay_active_low;
      document.getElementById('hwOledAddress').value = String(d.oled_address);
      updateHardwareLabels(d);
      bindPinoutPreview();
      renderBoardPinout();

      var grid = document.getElementById('gpioGrid');
      grid.innerHTML = '';
      d.pins.forEach(function(p) {
        var item = document.createElement('div');
        item.className = 'gpio-item';
        if (p.role) item.classList.add('gpio-used');
        if (p.reserved) item.classList.add('gpio-reserved');
        if (p.warning) item.classList.add('gpio-warning');
        var text = 'GPIO' + p.pin + ': ';
        if (p.reserved) text += p.restriction || 'indisponibil';
        else if (p.role) text += p.role;
        else text += 'liber';
        if (!p.output) text += ', doar IN';
        if (p.warning) text += ' | ' + p.warning;
        item.textContent = text;
        grid.appendChild(item);
      });
    })
    .catch(function() { showHwMsg('Eroare la citirea configuratiei hardware.', 'err'); });
}

function saveHardware() {
  var body =
      'dht_pin=' + encodeURIComponent(document.getElementById('hwDht').value)
    + '&pir_pin=' + encodeURIComponent(document.getElementById('hwPir').value)
    + '&relay_pin=' + encodeURIComponent(document.getElementById('hwRelay').value)
    + '&heartbeat_pin=' + encodeURIComponent(document.getElementById('hwHeartbeat').value)
    + '&sda_pin=' + encodeURIComponent(document.getElementById('hwSda').value)
    + '&scl_pin=' + encodeURIComponent(document.getElementById('hwScl').value)
    + '&relay_active_low=' + (document.getElementById('hwRelayLow').checked ? '1' : '0')
    + '&oled_address=' + encodeURIComponent(document.getElementById('hwOledAddress').value);

  showHwMsg('Se valideaza si se salveaza...', 'info');
  fetch('/api/hardware_config', {
    method: 'POST',
    headers: {'Content-Type': 'application/x-www-form-urlencoded'},
    body: body
  })
  .then(r => r.json().then(d => ({ok:r.ok, data:d})))
  .then(x => showHwMsg(x.data.message || x.data.error || 'Eroare', x.ok ? 'ok' : 'err'))
  .catch(function() { showHwMsg('Conexiunea s-a inchis in timpul restartului.', 'info'); });
}

function resetHardware() {
  if (!confirm('Restaurati pinii impliciti pentru profilul acestei placi?')) return;
  fetch('/api/hardware_reset', {method:'POST'})
    .then(r => r.json())
    .then(d => showHwMsg(d.message || 'Configuratie resetata.', d.ok ? 'ok' : 'err'))
    .catch(function() { showHwMsg('Conexiunea s-a inchis in timpul restartului.', 'info'); });
}

function showHwMsg(txt, type) {
  var m = document.getElementById('hwMsg');
  m.textContent = txt;
  m.style.display = 'block';
  m.style.background = type === 'ok' ? '#d1fae5' : type === 'err' ? '#fee2e2' : '#dbeafe';
  m.style.color = type === 'ok' ? '#065f46' : type === 'err' ? '#991b1b' : '#1e40af';
}

// ---- Board Info ----
function loadBoardInfo() {
  fetch('/api/board_info')
    .then(r => r.json())
    .then(d => {
      document.getElementById('biChipModel').textContent = d.chip_model || '-';
      document.getElementById('biBuildProfile').textContent = d.build_profile || '-';
      document.getElementById('biChipRev').textContent   = d.chip_revision;
      document.getElementById('biCores').textContent     = d.cpu_cores + ' x ' + d.cpu_freq + ' MHz';
      document.getElementById('biFreq').textContent      = d.cpu_freq + ' MHz';
      document.getElementById('biFlash').textContent     = (d.flash_size / 1048576).toFixed(1) + ' MB';
      document.getElementById('biPsram').textContent     = d.psram_size > 0 ? (d.psram_size / 1048576).toFixed(1) + ' MB' : 'Nu';
      document.getElementById('biFreeHeap').textContent  = (d.free_heap / 1024).toFixed(1) + ' KB';
      document.getElementById('biMinHeap').textContent   = (d.min_free_heap / 1024).toFixed(1) + ' KB';
      document.getElementById('biSdk').textContent       = d.sdk_version || '-';
      document.getElementById('biMac').textContent       = d.mac_address || '-';
      document.getElementById('biUptime').textContent    = Math.floor(d.uptime_ms / 1000) + ' s';
      document.getElementById('biReset').textContent     = d.reset_reason || '-';
      document.getElementById('biSketchSize').textContent = (d.sketch_size / 1024).toFixed(1) + ' KB';
      document.getElementById('biSketchFree').textContent = (d.free_sketch_space / 1024).toFixed(1) + ' KB';
      document.getElementById('biMd5').textContent       = d.sketch_md5 || '-';
      document.getElementById('boardContent').style.display = 'block';
    })
    .catch(function() { alert('Eroare la citirea informatiilor placii!'); });
}

loadHardwareCfg();
</script>
</body>
</html>
)rawliteral";


#endif // WEBPAGES_H
