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
.language-control{display:flex;justify-content:flex-end;align-items:center;gap:8px;margin:-12px 0 14px;color:#475569;font-size:.86em;font-weight:700}
.language-control select{padding:6px 9px;border:1px solid #cbd5e1;border-radius:7px;background:#fff;color:#1e293b;font-weight:700}
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
/* RGB LED card */
.card-rgb{display:none;background:linear-gradient(135deg,#0f172a,#334155)}
.rgb-preview{width:58px;height:58px;border-radius:50%;margin:8px auto;border:3px solid rgba(255,255,255,.7);background:#000;box-shadow:0 0 18px rgba(255,255,255,.3)}
.rgb-controls{display:grid;grid-template-columns:54px 1fr;gap:8px;align-items:center;margin-top:10px;text-align:left;font-size:.8em}
.rgb-controls input[type=color]{width:54px;height:36px;padding:2px;border:2px solid rgba(255,255,255,.6);border-radius:6px;background:transparent;cursor:pointer}
.rgb-controls input[type=range]{width:100%}
.rgb-meta{font-size:.78em;opacity:.85;margin-top:7px}
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
.bi-iot{background:linear-gradient(135deg,#0f766e,#0891b2)}
.cap-grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(210px,1fr));gap:12px}
.cap-card{background:rgba(255,255,255,.14);border:1px solid rgba(255,255,255,.24);border-radius:10px;padding:14px}
.cap-head{display:flex;align-items:center;justify-content:space-between;gap:10px;margin-bottom:8px}
.cap-name{font-weight:700;font-size:1.05em}
.cap-status{padding:4px 9px;border-radius:999px;font-size:.76em;font-weight:800}
.cap-yes{background:#d1fae5;color:#065f46}
.cap-no{background:#fee2e2;color:#991b1b}
.cap-detail{font-size:.82em;line-height:1.45;opacity:.92}
.cap-note{margin-top:12px;padding:10px 12px;border-radius:8px;background:rgba(15,23,42,.24);font-size:.82em;line-height:1.5}
.bi-btn{padding:11px 28px;background:#3b82f6;color:#fff;border:none;border-radius:8px;font-weight:700;cursor:pointer;font-size:.95em;display:block;margin:0 auto 20px}
/* Serial log */
.serial-toolbar{display:flex;flex-wrap:wrap;gap:9px;align-items:center;margin-bottom:12px}
.serial-toolbar .btn{flex:0 0 auto;padding:9px 16px}
.serial-check{display:flex;align-items:center;gap:7px;color:#475569;font-size:.9em;font-weight:600}
.serial-status{margin-left:auto;color:#64748b;font-size:.85em}
.serial-terminal{height:430px;overflow:auto;background:#07111f;color:#d1fae5;border:1px solid #334155;border-radius:10px;padding:14px;font:13px/1.5 Consolas,'Courier New',monospace;white-space:pre-wrap;overflow-wrap:anywhere;tab-size:2}
/* SPIFFS files */
.fs-summary{display:grid;grid-template-columns:repeat(3,1fr);gap:10px;margin-bottom:18px}
.fs-stat{padding:13px;border-radius:9px;background:#eff6ff;color:#1e3a8a;text-align:center}
.fs-stat strong{display:block;font-size:1.15em;margin-top:3px}
.fs-upload{padding:16px;border:2px dashed #93c5fd;border-radius:10px;background:#f8fafc;margin-bottom:18px}
.fs-upload-row{display:flex;flex-wrap:wrap;gap:10px;align-items:center}
.fs-upload-row input[type=file]{flex:1;min-width:220px}
.fs-table{width:100%;border-collapse:collapse}
.fs-table th,.fs-table td{padding:10px;border-bottom:1px solid #e2e8f0;text-align:left}
.fs-table th{color:#475569;font-size:.82em;text-transform:uppercase}
.fs-table td:nth-child(2),.fs-table th:nth-child(2){text-align:right;white-space:nowrap}
.fs-table td:nth-child(3),.fs-table th:nth-child(3){text-align:center;white-space:nowrap}
.fs-download{display:inline-block;padding:6px 12px;border-radius:6px;background:#2563eb;color:#fff;text-decoration:none;font-weight:700;font-size:.85em}
.fs-empty{text-align:center;padding:24px;color:#64748b}
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
  .language-control{justify-content:center;margin-top:-10px}
  .pinout-board{grid-template-columns:145px 135px 145px;gap:5px}.board-body{min-height:450px}
  .serial-status{width:100%;margin-left:0}.serial-terminal{height:360px}
  .fs-summary{grid-template-columns:1fr}.fs-table th:first-child,.fs-table td:first-child{word-break:break-all}
}
</style>
</head>
<body>
<div class="container">
  <h1>&#127968; ESP32 HA Kit</h1>
  <p class="subtitle">MQTT &bull; Home Assistant &bull; DHT11 &bull; PIR &bull; Releu &bull; OLED</p>
  <div class="language-control">
    <label for="uiLanguage">Limba interfeței</label>
    <select id="uiLanguage" onchange="saveUiLanguage(this.value)">
      <option value="ro">Română</option>
      <option value="en">English</option>
    </select>
  </div>

  <div class="tabs">
    <button class="tab-btn active" onclick="showTab('monitor',this)">&#128202; Dashboard</button>
    <button class="tab-btn" onclick="showTab('mqtt',this)">&#128268; MQTT</button>
    <button class="tab-btn" onclick="showTab('wifi',this)">&#128225; WiFi</button>
    <button class="tab-btn" id="communicationTabButton" style="display:none"
            onclick="showTab('communication',this)">&#128225; Comunicare IoT</button>
    <button class="tab-btn" onclick="showTab('hardware',this)">&#128295; Hardware</button>
    <button class="tab-btn" onclick="showTab('files',this)">&#128193; Fisiere</button>
    <button class="tab-btn" onclick="showTab('serial',this)">&#9000; Serial Log</button>
    <button class="tab-btn" onclick="showTab('board',this)">&#128203; Info Placa</button>
  </div>

  <!-- DASHBOARD TAB -->
  <div id="monitor-tab" class="tab active">
    <div class="mqtt-bar mqtt-unknown" id="mqttBar">MQTT: se verifica...</div>

    <div class="cards">
      <!-- Temperature -->
      <div class="card card-temp" id="tempCard" style="display:none">
        <div class="card-label">Temperatura</div>
        <div class="card-value" id="tempVal">--</div>
        <div class="card-unit">&deg;C &nbsp;|&nbsp; DHT11</div>
      </div>

      <!-- Humidity -->
      <div class="card card-hum" id="humCard" style="display:none">
        <div class="card-label">Umiditate</div>
        <div class="card-value" id="humVal">--</div>
        <div class="card-unit">% &nbsp;|&nbsp; DHT11</div>
      </div>

      <!-- Motion -->
      <div class="card card-motion" id="motionCard" style="display:none">
        <div class="card-label">Senzor Miscare</div>
        <div class="motion-circle" id="motionCircle">&#10003;</div>
        <div id="motionText" style="font-size:1.1em;font-weight:700">Fara miscare</div>
        <div style="font-size:.8em;opacity:.8;margin-top:4px">HC-SR501 &bull; <span id="dashPirPin">GPIO--</span></div>
      </div>

      <!-- Relay -->
      <div class="card card-relay" id="relayCard" style="display:none">
        <div class="card-label">Releu SSR</div>
        <div class="relay-state" id="relayVal">OFF</div>
        <div style="font-size:.8em;opacity:.8;margin-bottom:6px"><span id="dashRelayPin">GPIO--</span> &bull; configurabil</div>
        <div class="relay-btns">
          <button onclick="setRelay(1)">&#9889; ON</button>
          <button onclick="setRelay(0)">&#9898; OFF</button>
        </div>
      </div>

      <!-- RGB LED onboard -->
      <div class="card card-rgb" id="rgbCard">
        <div class="card-label">LED RGB Onboard</div>
        <div class="rgb-preview" id="rgbPreview"></div>
        <div class="relay-state" id="rgbState">OFF</div>
        <div class="rgb-controls">
          <input type="color" id="rgbColor" value="#0080ff" aria-label="Culoare LED">
          <input type="range" id="rgbBrightness" min="0" max="100" value="25" aria-label="Luminozitate LED">
        </div>
        <div class="rgb-meta"><span id="rgbHex">#0080FF</span> &bull; <span id="rgbBrightnessValue">25%</span> &bull; <span id="rgbPin">GPIO--</span></div>
        <div class="relay-btns">
          <button onclick="setRgbLed(1)">ON</button>
          <button onclick="setRgbLed(0)">OFF</button>
        </div>
      </div>
    </div>

    <div class="info-box" id="hardwareInfo">
      <strong>&#128268; Cablaj hardware:</strong>
      <ul>
        <li id="wireDhtRow" style="display:none">DHT11 DATA &rarr; <span id="wireDht">neconfigurat</span> &nbsp;|&nbsp; VCC=3.3V, GND=GND</li>
        <li id="wirePirRow" style="display:none">PIR HC-SR501 OUT &rarr; <span id="wirePir">neconfigurat</span></li>
        <li id="wireRelayRow" style="display:none">Releu SSR IN &rarr; <span id="wireRelay">neconfigurat</span></li>
        <li id="wireOledRow" style="display:none">OLED SSD1306 SDA &rarr; <span id="wireSda">neconfigurat</span> &nbsp;|&nbsp; SCL &rarr; <span id="wireScl">neconfigurat</span></li>
        <li id="wireHeartbeatRow" style="display:none">LED Heartbeat &rarr; <span id="wireHeartbeat">neconfigurat</span></li>
        <li id="noExternalHardware">Niciun periferic extern detectat sau configurat.</li>
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

  <!-- IOT COMMUNICATION TAB: enabled only for Zigbee-capable boards -->
  <div id="communication-tab" class="tab">
    <div class="mqtt-bar mqtt-unknown" id="communicationStatus">
      Se citeste configuratia radio...
    </div>

    <h3 style="margin-bottom:14px;color:#1e293b">Protocol de comunicare IoT</h3>
    <p style="margin-bottom:16px;color:#64748b;font-size:.9em">
      Selectia este salvata in memoria NVS. Modurile Zigbee si Thread necesita un firmware
      construit explicit cu stack-ul protocolului ales; starea de mai sus indica modul activ real.
    </p>

    <div class="form-group">
      <label class="form-label" for="communicationProtocol">Protocol selectat</label>
      <select id="communicationProtocol" class="form-input" onchange="updateCoordinatorVisibility()">
        <option value="wifi_mqtt">MQTT prin WiFi</option>
        <option value="zigbee">Zigbee</option>
        <option value="thread">Thread</option>
      </select>
    </div>

    <div id="zigbeeCoordinatorConfig" style="display:none">
      <div class="form-group">
        <label class="form-label" for="zigbeeCoordinator">Coordinator / gateway Zigbee</label>
        <select id="zigbeeCoordinator" class="form-input" onchange="renderCoordinatorHelp()">
          <option value="generic">Coordinator Zigbee generic</option>
          <option value="zha">Home Assistant ZHA</option>
          <option value="zigbee2mqtt">Zigbee2MQTT</option>
          <option value="zbbridge_u">SONOFF ZBBridge-U (firmware original)</option>
        </select>
      </div>
      <div class="form-group">
        <label class="form-label" for="zigbeeCoordinatorHost">Adresa portalului coordinatorului</label>
        <input type="text" id="zigbeeCoordinatorHost" class="form-input"
               placeholder="ex. 192.168.1.6 sau zbbridgeu.local">
        <small style="color:#64748b;font-size:.82em">
          Adresa este folosita numai pentru deschiderea portalului de administrare, nu pentru conexiunea radio Zigbee.
        </small>
      </div>
      <div class="form-group">
        <label class="form-label" for="zigbeePairingMode">Mod pairing Zigbee</label>
        <select id="zigbeePairingMode" class="form-input">
          <option value="auto">Automat: conectare sau rejoin la reteaua salvata</option>
          <option value="force_new">Retea noua: sterge asocierea Zigbee la urmatoarea pornire</option>
        </select>
      </div>
      <div class="btn-row" style="margin-top:10px">
        <button class="btn" style="background:#0f766e;color:#fff" onclick="openCoordinatorPortal()">
          Deschide portalul coordinatorului
        </button>
      </div>
      <div class="info-box" id="zigbeeCoordinatorHelp" style="margin-top:14px"></div>
    </div>

    <div class="btn-row">
      <button class="btn btn-primary" onclick="saveCommunicationConfig()">&#128190; Salveaza selectia</button>
      <button class="btn" style="background:#64748b;color:#fff;flex:0 0 auto"
              onclick="loadCommunicationConfig()">&#8635;</button>
    </div>
    <div class="msg-box" id="communicationMsg"></div>

    <div class="info-box" style="margin-top:18px">
      <strong>&#8505; Cerinte:</strong>
      <ul>
        <li><strong>Zigbee:</strong> necesita un coordinator/gateway Zigbee aflat in modul de asociere.</li>
        <li><strong>Thread:</strong> necesita o retea Thread, credencialele ei si un Thread Border Router.</li>
        <li>Pentru control direct din Home Assistant prin Thread este necesar Matter over Thread.</li>
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

  <!-- SPIFFS FILES TAB -->
  <div id="files-tab" class="tab">
    <div class="fs-summary">
      <div class="fs-stat">Capacitate<strong id="fsTotal">-</strong></div>
      <div class="fs-stat">Utilizat<strong id="fsUsed">-</strong></div>
      <div class="fs-stat">Liber<strong id="fsFree">-</strong></div>
    </div>

    <div class="fs-upload">
      <div class="fs-upload-row">
        <input type="file" id="fsUploadFile">
        <button class="btn btn-primary" style="flex:0 0 auto" onclick="uploadSpiffsFile()">
          Incarca fisier
        </button>
        <button class="btn" style="background:#64748b;color:#fff;flex:0 0 auto" onclick="loadSpiffsFiles()">
          Actualizeaza
        </button>
      </div>
      <div class="msg-box" id="fsMsg"></div>
      <p style="margin-top:10px;color:#64748b;font-size:.82em">
        Fisierul este salvat in SPIFFS. Un fisier existent cu acelasi nume va fi suprascris.
      </p>
    </div>

    <div style="overflow-x:auto">
      <table class="fs-table">
        <thead><tr><th>Nume fisier</th><th>Dimensiune</th><th>Actiune</th></tr></thead>
        <tbody id="fsFileList"><tr><td colspan="3" class="fs-empty">Se citeste continutul SPIFFS...</td></tr></tbody>
      </table>
    </div>
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
      <div class="bi-section bi-iot">
        <div class="bi-title">&#128225; Capabilitati IoT</div>
        <div class="cap-grid">
          <div class="cap-card">
            <div class="cap-head">
              <span class="cap-name">Zigbee</span>
              <span class="cap-status" id="biZigbeeStatus">-</span>
            </div>
            <div class="cap-detail" id="biZigbeeDetail">-</div>
          </div>
          <div class="cap-card">
            <div class="cap-head">
              <span class="cap-name">Thread</span>
              <span class="cap-status" id="biThreadStatus">-</span>
            </div>
            <div class="cap-detail" id="biThreadDetail">-</div>
          </div>
          <div class="cap-card">
            <div class="cap-head">
              <span class="cap-name">Matter</span>
              <span class="cap-status" id="biMatterStatus">-</span>
            </div>
            <div class="cap-detail" id="biMatterDetail">-</div>
          </div>
        </div>
        <div class="cap-note">
          <div id="biCommunicationSummary" style="font-weight:700;margin-bottom:5px"></div>
          Capabilitatea indica suportul placii si al platformei. Firmware-ul curent foloseste MQTT prin WiFi;
          Zigbee, Thread sau Matter necesita un firmware configurat explicit pentru protocolul ales.
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
var currentLanguage = 'ro';
var originalTextNodes = [];

var englishText = {
  'Limba interfeței':'Interface language',
  'Română':'Romanian',
  'MQTT • Home Assistant • DHT11 • PIR • Releu • OLED':'MQTT • Home Assistant • DHT11 • PIR • Relay • OLED',
  'Info Placa':'Board Info',
  'Fisiere':'Files',
  'Capacitate':'Capacity',
  'Utilizat':'Used',
  'Liber':'Free',
  'Incarca fisier':'Upload file',
  'Actualizeaza':'Refresh',
  'Fisierul este salvat in SPIFFS. Un fisier existent cu acelasi nume va fi suprascris.':'The file is stored in SPIFFS. An existing file with the same name will be overwritten.',
  'Nume fisier':'File name',
  'Dimensiune':'Size',
  'Actiune':'Action',
  'Se citeste continutul SPIFFS...':'Reading SPIFFS contents...',
  'Comunicare IoT':'IoT Communication',
  'Se citeste configuratia radio...':'Reading radio configuration...',
  'Protocol de comunicare IoT':'IoT communication protocol',
  'Selectia este salvata in memoria NVS. Modurile Zigbee si Thread necesita un firmware construit explicit cu stack-ul protocolului ales; starea de mai sus indica modul activ real.':'The selection is stored in NVS. Zigbee and Thread require firmware built explicitly with the selected protocol stack; the status above shows the actual active mode.',
  'Protocol selectat':'Selected protocol',
  'Coordinator / gateway Zigbee':'Zigbee coordinator / gateway',
  'Coordinator Zigbee generic':'Generic Zigbee coordinator',
  'Adresa portalului coordinatorului':'Coordinator portal address',
  'Mod pairing Zigbee':'Zigbee pairing mode',
  'Automat: conectare sau rejoin la reteaua salvata':'Automatic: join or rejoin the saved network',
  'Retea noua: sterge asocierea Zigbee la urmatoarea pornire':'New network: erase Zigbee pairing on the next boot',
  'Adresa este folosita numai pentru deschiderea portalului de administrare, nu pentru conexiunea radio Zigbee.':'The address is used only to open the administration portal, not for the Zigbee radio connection.',
  'Deschide portalul coordinatorului':'Open coordinator portal',
  'MQTT prin WiFi':'MQTT over Wi-Fi',
  'Salveaza selectia':'Save selection',
  'Cerinte:':'Requirements:',
  'necesita un coordinator/gateway Zigbee aflat in modul de asociere.':'requires a Zigbee coordinator/gateway in pairing mode.',
  'necesita o retea Thread, credencialele ei si un Thread Border Router.':'requires a Thread network, its credentials, and a Thread Border Router.',
  'Pentru control direct din Home Assistant prin Thread este necesar Matter over Thread.':'Direct control from Home Assistant over Thread requires Matter over Thread.',
  'Temperatura':'Temperature',
  'Umiditate':'Humidity',
  'Senzor Miscare':'Motion Sensor',
  'Fara miscare':'No motion',
  'Releu SSR':'SSR Relay',
  'Releu SSR IN':'SSR relay IN',
  'configurabil':'configurable',
  'Culoare LED':'LED color',
  'Luminozitate LED':'LED brightness',
  'Cablaj hardware:':'Hardware wiring:',
  'neconfigurat':'not configured',
  'Niciun periferic extern detectat sau configurat.':'No external peripheral detected or configured.',
  'MQTT publish la fiecare 30s sau la detectie miscare':'MQTT publishes every 30s or when motion is detected',
  'Configuratie broker MQTT':'MQTT broker configuration',
  'Verificare status MQTT...':'Checking MQTT status...',
  'Setarile sunt salvate in memoria flash a ESP32 si se aplica imediat dupa salvare. Home Assistant trebuie sa aiba un broker MQTT activ (ex. Mosquitto add-on).':'Settings are stored in ESP32 flash memory and applied immediately. Home Assistant must have an active MQTT broker, such as the Mosquitto add-on.',
  'Adresa IP broker':'Broker IP address',
  'Utilizator':'Username',
  '(optional)':'(optional)',
  'Parola':'Password',
  'Parola nu este afisata dupa salvare din motive de securitate.':'The password is not displayed after saving for security reasons.',
  'Salveaza si aplica':'Save and apply',
  'Topice MQTT folosite:':'MQTT topics:',
  'JSON: temperatura, umiditate, miscare, releu':'JSON: temperature, humidity, motion, relay',
  'starea releului (ON / OFF)':'relay state (ON / OFF)',
  'comanda releu din HA (ON / OFF)':'relay command from Home Assistant (ON / OFF)',
  'Auto-discovery Home Assistant publicat la fiecare conectare la broker':'Home Assistant auto-discovery is published on every broker connection',
  'Verificare status WiFi...':'Checking Wi-Fi status...',
  'Conectare la reteaua WiFi':'Connect to the Wi-Fi network',
  'Salveaza credentialele WiFi – ESP32 se va reconecta automat la pornire. Dupa salvare, brokerul MQTT devine accesibil.':'Save the Wi-Fi credentials. The ESP32 reconnects automatically at startup and can then reach the MQTT broker.',
  'SSID (Nume retea)':'SSID (network name)',
  'Parola WiFi':'Wi-Fi password',
  'Salveaza si conecteaza':'Save and connect',
  'Info:':'Information:',
  'Configuratia este salvata in memoria flash (NVS)':'The configuration is stored in flash memory (NVS)',
  'Daca conectarea esueaza, ESP32 revine in mod Access Point':'If the connection fails, the ESP32 returns to Access Point mode',
  'Adresa IP AP:':'AP IP address:',
  'Se citeste configuratia hardware...':'Reading hardware configuration...',
  'Selecteaza GPIO pentru fiecare functie. Optiunea':'Select a GPIO for each function. The',
  'Dezactivat':'Disabled',
  'marcheaza perifericul ca neconfigurat. Modificarile sunt validate, salvate in NVS si aplicate dupa restart.':'option marks the peripheral as not configured. Changes are validated, stored in NVS and applied after restart.',
  'Releu IN':'Relay IN',
  'Adresa OLED':'OLED address',
  'Releu activ pe LOW':'Relay active on LOW',
  'Salveaza si reporneste':'Save and restart',
  'Valori implicite':'Default values',
  'Dispunere pini':'Pin layout',
  'Vedere de sus. Conectorul USB este reprezentat in partea de jos.':'Top view. The USB connector is shown at the bottom.',
  'Profil placa':'Board profile',
  'Configurat':'Configured',
  'Liber':'Free',
  'Indisponibil':'Unavailable',
  'Avertisment':'Warning',
  'Alimentare':'Power',
  'Inventar GPIO':'GPIO inventory',
  'Pinii de flash si cei neexpusi pe placa sunt blocati. Pinii de boot sau USB sunt permisi, dar marcati cu avertisment.':'Flash pins and pins not exposed by the board are blocked. Boot and USB pins are allowed but marked with a warning.',
  'Pauza':'Pause',
  'Descarca':'Download',
  'Sterge buffer':'Clear buffer',
  'Inactiv':'Inactive',
  'Deschide tab-ul pentru a citi jurnalul serial...':'Open this tab to read the serial log...',
  'Afisarea este o copie a mesajelor aplicatiei trimise pe USB serial la 115200 baud. Placa pastreaza circular ultimii 8 KB in RAM; mesajele vechi sunt suprascrise automat.':'This is a copy of application messages sent over USB serial at 115200 baud. The board keeps the latest 8 KB in a circular RAM buffer; older messages are overwritten automatically.',
  'Incarca informatii placa':'Load board information',
  'Profil build':'Build profile',
  'Revizie':'Revision',
  'Nuclee CPU':'CPU cores',
  'Frecventa':'Frequency',
  'Capabilitati IoT':'IoT capabilities',
  'Capabilitatea indica suportul placii si al platformei. Firmware-ul curent foloseste MQTT prin WiFi; Zigbee, Thread sau Matter necesita un firmware configurat explicit pentru protocolul ales.':'Capability indicates board and platform support. The current firmware uses MQTT over Wi-Fi; Zigbee, Thread or Matter require firmware explicitly configured for that protocol.',
  'Memorie':'Memory',
  'Heap liber':'Free heap',
  'Heap min liber':'Minimum free heap',
  'Sistem':'System',
  'Program':'Firmware',
  'Spatiu liber':'Free space'
  ,'las gol daca nu e necesar':'leave empty if not required'
  ,'las gol daca nu e necesara':'leave empty if not required'
  ,'Nume retea WiFi':'Wi-Fi network name'
};

function normalizeUiText(value) {
  return value.replace(/\s+/g, ' ').trim();
}

function translateStaticText() {
  if (!originalTextNodes.length) {
    var walker = document.createTreeWalker(document.body, NodeFilter.SHOW_TEXT);
    var node;
    while ((node = walker.nextNode())) {
      if (node.parentElement && !['SCRIPT','STYLE'].includes(node.parentElement.tagName))
        originalTextNodes.push({node:node, value:node.nodeValue});
    }
  }

  originalTextNodes.forEach(function(entry) {
    if (currentLanguage === 'ro') {
      entry.node.nodeValue = entry.value;
      return;
    }
    var source = normalizeUiText(entry.value);
    if (!source) return;
    var plain = source.replace(/^[^A-Za-z0-9]+/, '');
    var prefix = source.slice(0, source.length - plain.length);
    var translated = englishText[source] || englishText[plain];
    if (translated) entry.node.nodeValue = prefix + translated;
  });

  document.querySelectorAll('input[placeholder]').forEach(function(input) {
    if (!input.dataset.roPlaceholder) input.dataset.roPlaceholder = input.placeholder;
    var source = input.dataset.roPlaceholder;
    input.placeholder = currentLanguage === 'en' ? (englishText[source] || source) : source;
  });
  document.documentElement.lang = currentLanguage;
  document.getElementById('uiLanguage').value = currentLanguage;
}

function tr(ro, en) {
  return currentLanguage === 'en' ? (en || englishText[ro] || ro) : ro;
}

function applyUiLanguage(language) {
  currentLanguage = language === 'en' ? 'en' : 'ro';
  translateStaticText();
  updateData();
  if (hardwareData) loadHardwareCfg();
  loadCommunicationConfig();
  var boardContent = document.getElementById('boardContent');
  if (boardContent && boardContent.style.display === 'block') loadBoardInfo();
}

function loadUiLanguage() {
  fetch('/api/ui_config', {cache:'no-store'})
    .then(function(r) { return r.json(); })
    .then(function(d) { applyUiLanguage(d.language); })
    .catch(function() { applyUiLanguage('ro'); });
}

function saveUiLanguage(language) {
  applyUiLanguage(language);
  fetch('/api/ui_config', {
    method:'POST',
    headers:{'Content-Type':'application/x-www-form-urlencoded'},
    body:'language=' + encodeURIComponent(currentLanguage)
  }).catch(function(){});
}

function showTab(name, btn) {
  document.querySelectorAll('.tab').forEach(t => t.classList.remove('active'));
  document.querySelectorAll('.tab-btn').forEach(b => b.classList.remove('active'));
  document.getElementById(name + '-tab').classList.add('active');
  btn.classList.add('active');
  if (name === 'wifi')  loadWifiStatus();
  if (name === 'mqtt')  loadMqttCfg();
  if (name === 'communication') loadCommunicationConfig();
  if (name === 'hardware') loadHardwareCfg();
  if (name === 'files') loadSpiffsFiles();
  serialTabActive = name === 'serial';
  if (serialTabActive) loadSerialLog();
}

// ---- Dashboard ----
function updateData() {
  fetch('/data')
    .then(r => r.json())
    .then(d => {
      document.getElementById('tempCard').style.display = d.dht_available ? 'block' : 'none';
      document.getElementById('humCard').style.display = d.dht_available ? 'block' : 'none';
      document.getElementById('motionCard').style.display = d.pir_available ? 'block' : 'none';
      document.getElementById('relayCard').style.display = d.relay_available ? 'block' : 'none';
      document.getElementById('wireDhtRow').style.display = d.dht_available ? 'list-item' : 'none';
      document.getElementById('wirePirRow').style.display = d.pir_available ? 'list-item' : 'none';
      document.getElementById('wireRelayRow').style.display = d.relay_available ? 'list-item' : 'none';
      document.getElementById('wireOledRow').style.display = d.oled_available ? 'list-item' : 'none';
      document.getElementById('wireHeartbeatRow').style.display = d.heartbeat_available ? 'list-item' : 'none';
      document.getElementById('noExternalHardware').style.display =
        d.dht_available || d.pir_available || d.relay_available ||
        d.oled_available || d.heartbeat_available ? 'none' : 'list-item';

      document.getElementById('tempVal').textContent = d.temperature !== null ? d.temperature : '--';
      document.getElementById('humVal').textContent  = d.humidity    !== null ? d.humidity    : '--';

      var mc = document.getElementById('motionCard');
      var ci = document.getElementById('motionCircle');
      var mt = document.getElementById('motionText');
      if (d.motion) {
        mc.classList.add('alert');
        ci.classList.add('pulse');
        ci.textContent = '!';
        mt.textContent = tr('MISCARE DETECTATA!', 'MOTION DETECTED!');
      } else {
        mc.classList.remove('alert');
        ci.classList.remove('pulse');
        ci.textContent = '\u2713';
        mt.textContent = tr('Fara miscare', 'No motion');
      }

      document.getElementById('relayVal').textContent = d.relay ? 'ON' : 'OFF';

      var bar = document.getElementById('mqttBar');
      if (d.mqtt_connected) {
        bar.className = 'mqtt-bar mqtt-ok';
        bar.textContent = tr('MQTT: Conectat la broker \u2714', 'MQTT: Connected to broker \u2714');
      } else {
        bar.className = 'mqtt-bar mqtt-err';
        bar.textContent = tr('MQTT: Deconectat \u2014 verifica IP broker si WiFi',
                             'MQTT: Disconnected \u2014 check broker IP and Wi-Fi');
      }
    })
    .catch(function() {
      document.getElementById('mqttBar').className = 'mqtt-bar mqtt-unknown';
      document.getElementById('mqttBar').textContent =
        tr('Dashboard: eroare citire date...', 'Dashboard: data read error...');
    });
}
setInterval(updateData, 2000);
updateData();

function setRelay(s) {
  fetch('/api/relay?state=' + s)
    .catch(function(){});
}

var rgbLedOn = false;
var rgbUpdateTimer = null;

function rgbHexToValues(hex) {
  return {
    red: parseInt(hex.slice(1, 3), 16),
    green: parseInt(hex.slice(3, 5), 16),
    blue: parseInt(hex.slice(5, 7), 16)
  };
}

function renderRgbLed(d) {
  var card = document.getElementById('rgbCard');
  card.style.display = d.supported ? 'block' : 'none';
  if (!d.supported) return;

  rgbLedOn = !!d.on;
  var hex = '#' + [d.red, d.green, d.blue].map(function(value) {
    return Number(value).toString(16).padStart(2, '0');
  }).join('').toUpperCase();
  document.getElementById('rgbColor').value = hex;
  document.getElementById('rgbBrightness').value = d.brightness;
  document.getElementById('rgbBrightnessValue').textContent = d.brightness + '%';
  document.getElementById('rgbHex').textContent = hex;
  document.getElementById('rgbPin').textContent = d.pin >= 0 ? 'GPIO' + d.pin : 'onboard';
  document.getElementById('rgbState').textContent = d.on ? 'ON' : 'OFF';
  var intensity = d.on ? Number(d.brightness) / 100 : 0;
  document.getElementById('rgbPreview').style.background = d.on ? hex : '#000000';
  document.getElementById('rgbPreview').style.boxShadow =
    '0 0 ' + (10 + Math.round(24 * intensity)) + 'px ' + (d.on ? hex : 'rgba(255,255,255,.2)');
}

function loadRgbLed() {
  fetch('/api/rgb_led', {cache:'no-store'})
    .then(function(r) { return r.json(); })
    .then(renderRgbLed)
    .catch(function(){});
}

function sendRgbLed(state) {
  var color = rgbHexToValues(document.getElementById('rgbColor').value);
  var brightness = document.getElementById('rgbBrightness').value;
  var body = 'red=' + color.red + '&green=' + color.green + '&blue=' + color.blue
    + '&brightness=' + encodeURIComponent(brightness);
  if (state !== undefined) body += '&state=' + state;

  fetch('/api/rgb_led', {
    method:'POST',
    headers:{'Content-Type':'application/x-www-form-urlencoded'},
    body:body
  })
    .then(function(r) { return r.json().then(function(d) { return {ok:r.ok, data:d}; }); })
    .then(function(result) {
      if (result.ok && result.data.led) renderRgbLed(result.data.led);
    })
    .catch(function(){});
}

function setRgbLed(state) {
  rgbLedOn = !!state;
  sendRgbLed(state ? 1 : 0);
}

function scheduleRgbUpdate() {
  var color = document.getElementById('rgbColor').value.toUpperCase();
  var brightness = document.getElementById('rgbBrightness').value;
  document.getElementById('rgbHex').textContent = color;
  document.getElementById('rgbBrightnessValue').textContent = brightness + '%';
  document.getElementById('rgbPreview').style.background = rgbLedOn ? color : '#000000';
  clearTimeout(rgbUpdateTimer);
  rgbUpdateTimer = setTimeout(function() { sendRgbLed(); }, 120);
}

document.getElementById('rgbColor').addEventListener('input', scheduleRgbUpdate);
document.getElementById('rgbBrightness').addEventListener('input', scheduleRgbUpdate);
loadRgbLed();

// ---- IoT Communication ----
function communicationLabel(protocol) {
  if (protocol === 'zigbee') return 'Zigbee';
  if (protocol === 'thread') return 'Thread';
  return tr('MQTT prin WiFi', 'MQTT over Wi-Fi');
}

function renderCommunicationConfig(d) {
  var tabButton = document.getElementById('communicationTabButton');
  tabButton.style.display = d.visible ? '' : 'none';
  if (!d.visible) return;

  var select = document.getElementById('communicationProtocol');
  select.querySelector('option[value="zigbee"]').disabled = !d.zigbee_capable;
  select.querySelector('option[value="thread"]').disabled = !d.thread_capable;
  select.value = d.selected_protocol || 'wifi_mqtt';
  document.getElementById('zigbeeCoordinator').value =
    d.zigbee_coordinator || 'generic';
  document.getElementById('zigbeeCoordinatorHost').value =
    d.zigbee_coordinator_host || '';
  document.getElementById('zigbeePairingMode').value =
    d.zigbee_pairing_mode || 'auto';
  updateCoordinatorVisibility();

  var status = document.getElementById('communicationStatus');
  var selected = communicationLabel(d.selected_protocol);
  var active = communicationLabel(d.active_protocol);
  if (d.firmware_change_required) {
    status.className = 'mqtt-bar mqtt-unknown';
    status.textContent = tr('Selectat: ', 'Selected: ') + selected + ' | ' +
      tr('Activ: ', 'Active: ') + active + ' | ' +
      tr('este necesar firmware compatibil', 'compatible firmware required');
  } else {
    status.className = 'mqtt-bar mqtt-ok';
    status.textContent = tr('Mod activ: ', 'Active mode: ') + active;
  }
}

function updateCoordinatorVisibility() {
  var zigbeeSelected =
    document.getElementById('communicationProtocol').value === 'zigbee';
  document.getElementById('zigbeeCoordinatorConfig').style.display =
    zigbeeSelected ? 'block' : 'none';
  if (zigbeeSelected) renderCoordinatorHelp();
}

function renderCoordinatorHelp() {
  var coordinator = document.getElementById('zigbeeCoordinator').value;
  var help = document.getElementById('zigbeeCoordinatorHelp');
  if (coordinator === 'zbbridge_u') {
    help.style.background = '#fff7ed';
    help.style.borderLeftColor = '#f97316';
    help.style.color = '#9a3412';
    help.innerHTML = '<strong>' +
      tr('Limitare ZBBridge-U:', 'ZBBridge-U limitation:') + '</strong> ' +
      tr('firmware-ul original accepta oficial numai dispozitive SONOFF si din ecosistemul eWeLink. Un dispozitiv Zigbee personalizat pe ESP32-C6 poate sa nu fie acceptat sau recunoscut. Asocierea se porneste din portal cu „+ Add Device”.',
         'the stock firmware officially accepts only SONOFF and eWeLink ecosystem devices. A custom ESP32-C6 Zigbee device may not be accepted or recognized. Start pairing from the portal using \"+ Add Device\".');
  } else if (coordinator === 'zha') {
    help.style.background = '#f0f9ff';
    help.style.borderLeftColor = '#3b82f6';
    help.style.color = '#1e3a5f';
    help.textContent = tr(
      'In Home Assistant deschide ZHA, selecteaza Add device, apoi porneste firmware-ul Zigbee al placii. Placa va cauta automat reteaua aflata in permit join.',
      'In Home Assistant open ZHA, select Add device, then start the board Zigbee firmware. The board automatically searches for the network in permit-join mode.');
  } else if (coordinator === 'zigbee2mqtt') {
    help.style.background = '#f0f9ff';
    help.style.borderLeftColor = '#3b82f6';
    help.style.color = '#1e3a5f';
    help.textContent = tr(
      'In Zigbee2MQTT activeaza Permit join, apoi porneste firmware-ul Zigbee al placii. Nu este necesara adresa IP a coordinatorului in firmware-ul ESP32.',
      'In Zigbee2MQTT enable Permit join, then start the board Zigbee firmware. The coordinator IP address is not required by the ESP32 firmware.');
  } else {
    help.style.background = '#f0f9ff';
    help.style.borderLeftColor = '#3b82f6';
    help.style.color = '#1e3a5f';
    help.textContent = tr(
      'Activeaza permit join pe coordinator si porneste firmware-ul Zigbee al placii. Selectarea profilului nu alege coordinatorul prin IP; asocierea se face direct prin radio.',
      'Enable permit join on the coordinator and start the board Zigbee firmware. Selecting this profile does not choose the coordinator by IP; pairing happens directly over radio.');
  }
}

function openCoordinatorPortal() {
  var host = document.getElementById('zigbeeCoordinatorHost').value.trim();
  if (!host || !/^[A-Za-z0-9.:[\]-]+$/.test(host)) {
    showCommunicationMsg(
      tr('Introdu o adresa IP sau un nume local valid pentru coordinator.',
         'Enter a valid coordinator IP address or local hostname.'),
      'err');
    return;
  }
  window.open('http://' + host, '_blank', 'noopener');
}

function loadCommunicationConfig() {
  fetch('/api/communication_config', {cache:'no-store'})
    .then(function(r) { return r.json(); })
    .then(renderCommunicationConfig)
    .catch(function() {
      document.getElementById('communicationTabButton').style.display = 'none';
    });
}

function saveCommunicationConfig() {
  var protocol = document.getElementById('communicationProtocol').value;
  var coordinator = document.getElementById('zigbeeCoordinator').value;
  var coordinatorHost =
    document.getElementById('zigbeeCoordinatorHost').value.trim();
  var pairingMode = document.getElementById('zigbeePairingMode').value;
  showCommunicationMsg(tr('Se salveaza...', 'Saving...'), 'info');
  fetch('/api/communication_config', {
    method:'POST',
    headers:{'Content-Type':'application/x-www-form-urlencoded'},
    body:'protocol=' + encodeURIComponent(protocol) +
      '&zigbee_coordinator=' + encodeURIComponent(coordinator) +
      '&zigbee_coordinator_host=' + encodeURIComponent(coordinatorHost) +
      '&zigbee_pairing_mode=' + encodeURIComponent(pairingMode)
  })
    .then(function(r) {
      return r.json().then(function(d) { return {ok:r.ok, data:d}; });
    })
    .then(function(result) {
      if (!result.ok) {
        showCommunicationMsg(result.data.error || tr('Eroare la salvare!', 'Error while saving!'), 'err');
        return;
      }
      renderCommunicationConfig({
        visible:true,
        zigbee_capable:true,
        thread_capable:!document.querySelector('option[value="thread"]').disabled,
        selected_protocol:result.data.selected_protocol,
        active_protocol:result.data.active_protocol,
        zigbee_coordinator:result.data.zigbee_coordinator,
        zigbee_coordinator_host:result.data.zigbee_coordinator_host,
        zigbee_pairing_mode:result.data.zigbee_pairing_mode,
        firmware_change_required:result.data.firmware_change_required
      });
      showCommunicationMsg(
        result.data.firmware_change_required
          ? tr('Selectia a fost salvata. Pentru activare trebuie instalat firmware-ul protocolului ales.',
               'Selection saved. Firmware for the selected protocol must be installed to activate it.')
          : tr('Selectia a fost salvata si corespunde modului activ.',
               'Selection saved and it matches the active mode.'),
        result.data.firmware_change_required ? 'info' : 'ok');
    })
    .catch(function() {
      showCommunicationMsg(tr('Eroare la salvare!', 'Error while saving!'), 'err');
    });
}

function showCommunicationMsg(txt, type) {
  var message = document.getElementById('communicationMsg');
  message.textContent = txt;
  message.style.display = 'block';
  message.style.background = type === 'ok' ? '#d1fae5' : type === 'err' ? '#fee2e2' : '#dbeafe';
  message.style.color = type === 'ok' ? '#065f46' : type === 'err' ? '#991b1b' : '#1e40af';
}

loadCommunicationConfig();

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
        output.textContent += tr(
          '\n[Web] O parte din jurnal a fost suprascrisa in buffer.\n',
          '\n[Web] Part of the log was overwritten in the buffer.\n');
      output.textContent += data.text;
      if (output.textContent.length > 50000)
        output.textContent = output.textContent.slice(-50000);
      if (Number.isFinite(data.next)) serialCursor = data.next;
      document.getElementById('serialStatus').textContent =
        tr('Conectat', 'Connected') + ' | cursor ' + serialCursor + ' | ' +
        tr('buffer placa 8 KB', 'board buffer 8 KB');
      if (document.getElementById('serialAutoScroll').checked)
        output.scrollTop = output.scrollHeight;
    })
    .catch(function() {
      document.getElementById('serialStatus').textContent =
        tr('Eroare la citirea jurnalului', 'Error reading the log');
    })
    .finally(function() { serialLoading = false; });
}

function toggleSerialPause() {
  serialPaused = !serialPaused;
  document.getElementById('serialPauseBtn').textContent = serialPaused
    ? tr('Continua', 'Resume') : tr('Pauza', 'Pause');
  document.getElementById('serialStatus').textContent =
    serialPaused ? tr('Actualizare oprita', 'Updates paused')
                 : tr('Se reconecteaza...', 'Reconnecting...');
  if (!serialPaused) loadSerialLog();
}

function clearSerialLog() {
  fetch('/api/serial_log/clear', {method:'POST'})
    .then(function(r) {
      if (!r.ok) throw new Error('HTTP ' + r.status);
      serialCursor = 0;
      document.getElementById('serialOutput').textContent = '';
      document.getElementById('serialStatus').textContent = tr('Buffer sters', 'Buffer cleared');
      loadSerialLog();
    })
    .catch(function() {
      document.getElementById('serialStatus').textContent =
        tr('Stergerea bufferului a esuat', 'Failed to clear the buffer');
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

// ---- SPIFFS Files ----
function formatFileSize(bytes) {
  if (bytes < 1024) return bytes + ' B';
  if (bytes < 1048576) return (bytes / 1024).toFixed(1) + ' KB';
  return (bytes / 1048576).toFixed(2) + ' MB';
}

function showSpiffsMessage(message, ok) {
  var box = document.getElementById('fsMsg');
  box.style.display = 'block';
  box.style.background = ok ? '#d1fae5' : '#fee2e2';
  box.style.color = ok ? '#065f46' : '#991b1b';
  box.textContent = message;
}

function loadSpiffsFiles() {
  var list = document.getElementById('fsFileList');
  list.innerHTML = '<tr><td colspan="3" class="fs-empty">' +
    tr('Se citeste continutul SPIFFS...', 'Reading SPIFFS contents...') + '</td></tr>';

  fetch('/api/files', {cache:'no-store'})
    .then(function(r) {
      return r.json().then(function(data) {
        if (!r.ok) throw new Error(data.error || ('HTTP ' + r.status));
        return data;
      });
    })
    .then(function(data) {
      document.getElementById('fsTotal').textContent = formatFileSize(data.total);
      document.getElementById('fsUsed').textContent = formatFileSize(data.used);
      document.getElementById('fsFree').textContent = formatFileSize(data.total - data.used);
      list.textContent = '';

      if (!data.files.length) {
        var emptyRow = list.insertRow();
        var emptyCell = emptyRow.insertCell();
        emptyCell.colSpan = 3;
        emptyCell.className = 'fs-empty';
        emptyCell.textContent = tr('Nu exista fisiere in SPIFFS.', 'There are no files in SPIFFS.');
        return;
      }

      data.files.sort(function(a, b) { return a.name.localeCompare(b.name); });
      data.files.forEach(function(file) {
        var row = list.insertRow();
        row.insertCell().textContent = file.name;
        row.insertCell().textContent = formatFileSize(file.size);
        var actionCell = row.insertCell();
        var link = document.createElement('a');
        link.className = 'fs-download';
        link.href = '/api/files/download?path=' + encodeURIComponent(file.name);
        link.textContent = tr('Descarca', 'Download');
        actionCell.appendChild(link);
      });
    })
    .catch(function(error) {
      list.innerHTML = '';
      var row = list.insertRow();
      var cell = row.insertCell();
      cell.colSpan = 3;
      cell.className = 'fs-empty';
      cell.textContent = tr('Eroare SPIFFS: ', 'SPIFFS error: ') + error.message;
    });
}

function uploadSpiffsFile() {
  var input = document.getElementById('fsUploadFile');
  if (!input.files.length) {
    showSpiffsMessage(tr('Selecteaza mai intai un fisier.', 'Select a file first.'), false);
    return;
  }

  var data = new FormData();
  data.append('file', input.files[0]);
  showSpiffsMessage(tr('Fisierul se incarca...', 'Uploading file...'), true);

  fetch('/api/files/upload', {method:'POST', body:data})
    .then(function(r) {
      return r.json().then(function(response) {
        if (!r.ok) throw new Error(response.error || ('HTTP ' + r.status));
        return response;
      });
    })
    .then(function(response) {
      showSpiffsMessage(
        tr('Fisier incarcat: ', 'File uploaded: ') + response.name +
        ' (' + formatFileSize(response.size) + ')', true);
      input.value = '';
      loadSpiffsFiles();
    })
    .catch(function(error) {
      showSpiffsMessage(tr('Upload esuat: ', 'Upload failed: ') + error.message, false);
    });
}

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
        bar.textContent = tr('MQTT: Conectat la ', 'MQTT: Connected to ') +
                          d.broker + ':' + d.port + ' \u2714';
      } else {
        bar.className = 'mqtt-bar mqtt-err';
        bar.textContent = tr('MQTT: Deconectat de la ', 'MQTT: Disconnected from ') +
                          d.broker + ':' + d.port;
      }
    })
    .catch(function() {
      document.getElementById('mqttCfgBar').className = 'mqtt-bar mqtt-unknown';
      document.getElementById('mqttCfgBar').textContent =
        tr('Eroare citire configuratie MQTT', 'Error reading MQTT configuration');
    });
}

function saveMqttCfg() {
  var broker   = document.getElementById('mqttBroker').value.trim();
  var port     = document.getElementById('mqttPort').value.trim() || '1883';
  var clientId = document.getElementById('mqttClientId').value.trim() || 'esp32-ha-kit';
  var user     = document.getElementById('mqttUser').value.trim();
  var pass     = document.getElementById('mqttPass').value;

  if (!broker) {
    showMqttMsg(tr('Adresa IP broker este obligatorie!', 'Broker IP address is required!'), 'err');
    return;
  }

  showMqttMsg(tr('Se salveaza...', 'Saving...'), 'info');

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
    showMqttMsg(d.ok ? tr('Salvat! ESP32 se reconecteaza la broker.',
                          'Saved! The ESP32 is reconnecting to the broker.')
                     : (d.error || tr('Eroare!', 'Error!')),
                d.ok ? 'ok' : 'err');
    if (d.ok) setTimeout(loadMqttCfg, 3000);
  })
  .catch(function() {
    showMqttMsg(tr('Eroare la salvare!', 'Error while saving!'), 'err');
  });
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
        box.innerHTML = '&#10003; ' + tr('Conectat la', 'Connected to') +
                        ' <strong>' + d.ssid + '</strong> &nbsp;|&nbsp; IP: ' + d.ip;
      } else {
        box.className = 'wifi-status wifi-ap';
        box.innerHTML = '&#128225; ' + tr('Mod Access Point', 'Access Point mode') +
                        ' &nbsp;|&nbsp; IP: ' + d.ip;
      }
      if (d.saved_ssid) document.getElementById('wifiSSID').value = d.saved_ssid;
    })
    .catch(function(){});
}

function saveWifi() {
  var ssid = document.getElementById('wifiSSID').value;
  var pass = document.getElementById('wifiPass').value;
  if (!ssid) {
    showMsg(tr('Introduceti SSID!', 'Enter the SSID!'), 'err');
    return;
  }
  showMsg(tr('Se salveaza...', 'Saving...'), 'info');
  fetch('/wifi_config', {
    method: 'POST',
    headers: {'Content-Type': 'application/x-www-form-urlencoded'},
    body: 'ssid=' + encodeURIComponent(ssid) + '&password=' + encodeURIComponent(pass)
  })
  .then(r => r.json())
  .then(d => {
    showMsg(d.success
      ? tr('Configuratie salvata! ESP32 va reporni...', 'Configuration saved! The ESP32 will restart...')
      : tr('SSID invalid!', 'Invalid SSID!'), d.success ? 'ok' : 'err');
  })
  .catch(function() { showMsg(tr('Eroare la salvare!', 'Error while saving!'), 'err'); });
}

function clearWifi() {
  if (!confirm(tr('Stergeti configuratia WiFi salvata?',
                  'Delete the saved Wi-Fi configuration?'))) return;
  fetch('/wifi_clear')
    .then(r => r.json())
    .then(d => {
      showMsg(tr('Configuratia WiFi a fost stearsa!',
                 'Wi-Fi configuration cleared!'), 'ok');
      document.getElementById('wifiSSID').value = '';
      document.getElementById('wifiPass').value = '';
      setTimeout(loadWifiStatus, 800);
    })
    .catch(function() { showMsg(tr('Eroare!', 'Error!'), 'err'); });
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
  },
  c6supermini: {
    title: 'ESP32-C6 Super Mini - 20 pini',
    chip: 'ESP32-C6<br>Super Mini',
    note: 'Vedere de sus, USB sus. GPIO12/13 sunt USB D-/D+, GPIO9 este BOOT, GPIO8 este LED RGB, iar GPIO15 este LED albastru. GPIO21/22/23 si GPIO12/13 sunt pad-uri inferioare.',
    left: [
      {label:'TX / GPIO16',gpio:16}, {label:'RX / GPIO17',gpio:17},
      {label:'GPIO0 / A0',gpio:0}, {label:'GPIO1 / A1',gpio:1},
      {label:'GPIO2 / A2',gpio:2}, {label:'GPIO3 / A3',gpio:3},
      {label:'GPIO4 / SS',gpio:4}, {label:'GPIO5 / MOSI',gpio:5},
      {label:'GPIO6 / MISO',gpio:6}, {label:'GPIO7 / SCK',gpio:7}
    ],
    right: [
      {label:'5V',type:'power'}, {label:'GND',type:'power'}, {label:'3V3',type:'power'},
      {label:'GPIO20 / SDA',gpio:20}, {label:'GPIO19 / SCL',gpio:19},
      {label:'GPIO18',gpio:18}, {label:'GPIO15 / LED',gpio:15},
      {label:'GPIO14',gpio:14}, {label:'GPIO9 / BOOT',gpio:9},
      {label:'GPIO8 / RGB',gpio:8}
    ]
  },
  tzigbee: {
    title: 'LILYGO T-ZIGBEE v1.2 - headere ESP32-C3 / TLSR8258',
    chip: 'ESP32-C3<br>+ TLSR8258',
    note: 'Vedere de sus, USB jos. GPIO0 alimenteaza TLSR8258, iar GPIO18/19 formeaza UART-ul intern Zigbee.',
    left: [
      {label:'3V3',type:'power'}, {label:'RST',type:'control'}, {label:'MTMS / GPIO4',gpio:4},
      {label:'MTDI / GPIO5',gpio:5}, {label:'MTCK / GPIO6',gpio:6},
      {label:'MTDO / GPIO7',gpio:7}, {label:'GPIO8',gpio:8},
      {label:'NC',type:'control'}, {label:'NC',type:'control'}, {label:'NC',type:'control'},
      {label:'TLSR PB4',type:'control'}, {label:'TLSR PB5',type:'control'},
      {label:'TLSR PC0',type:'control'}, {label:'GND',type:'power'},
      {label:'TLSR PC1',type:'control'}, {label:'TLSR PC2',type:'control'},
      {label:'TLSR PC3',type:'control'}, {label:'TLSR PC4',type:'control'},
      {label:'5V',type:'power'}, {label:'NC',type:'control'}, {label:'NC',type:'control'}
    ],
    right: [
      {label:'GND',type:'power'}, {label:'GPIO1',gpio:1}, {label:'USER / GPIO2',gpio:2},
      {label:'TX0 / GPIO21',gpio:21}, {label:'RX0 / GPIO20',gpio:20},
      {label:'NC',type:'control'}, {label:'GND',type:'power'},
      {label:'NC',type:'control'}, {label:'NC',type:'control'}, {label:'NC',type:'control'},
      {label:'NC',type:'control'}, {label:'NC',type:'control'},
      {label:'TLSR PD7',type:'control'}, {label:'TLSR PA1',type:'control'},
      {label:'TLSR PD2',type:'control'}, {label:'TLSR PD3',type:'control'},
      {label:'NC',type:'control'}, {label:'NC',type:'control'}, {label:'NC',type:'control'},
      {label:'NC',type:'control'}, {label:'NC',type:'control'}
    ]
  }
};

function translateHardwareText(value) {
  if (currentLanguage !== 'en' || !value) return value;
  var exact = {
    'LED RGB onboard':'onboard RGB LED',
    'rezervat pentru memoria flash':'reserved for flash memory',
    'neexpus pe ESP32-S3-DevKitC-1':'not exposed on ESP32-S3-DevKitC-1',
    'neexpus pe ESP32-C6-DevKitC-1':'not exposed on ESP32-C6-DevKitC-1',
    'neexpus pe ESP32-C3-DevKitM-1':'not exposed on ESP32-C3-DevKitM-1',
    'neexpus pe LILYGO T-ZIGBEE':'not exposed on LILYGO T-ZIGBEE',
    'alimentare coprocesor Zigbee TLSR8258':'TLSR8258 Zigbee coprocessor power',
    'LED albastru onboard':'onboard blue LED',
    'UART intern catre TLSR8258':'internal UART to TLSR8258',
    'buton USER onboard si pin de boot/strapping':'onboard USER button and boot/strapping pin',
    'buton BOOT onboard si pin de boot/strapping':'onboard BOOT button and boot/strapping pin',
    'LED albastru onboard si pin de boot/strapping':'onboard blue LED and boot/strapping pin',
    'USB OTG/JTAG implicit':'default USB OTG/JTAG',
    'UART0 folosit pentru programare/log':'UART0 used for programming/logging',
    'indisponibil pe modulele cu flash/PSRAM Octal':'unavailable on modules with Octal flash/PSRAM',
    'LED RGB onboard pe unele revizii':'onboard RGB LED on some revisions',
    'pin de boot/strapping':'boot/strapping pin',
    'USB-JTAG implicit':'default USB-JTAG',
    'doar intrare':'input only',
    'indisponibil':'unavailable',
    'liber':'free',
    'releu':'relay'
  };
  if (exact[value]) return exact[value];
  return value
    .replace('GPIO invalid pentru acest cip', 'GPIO is invalid for this chip')
    .replace(' este atribuit simultan pentru ', ' is assigned to both ')
    .replace(' si ', ' and ')
    .replace(' nu poate fi iesire', ' cannot be used as an output')
    .replace(' este rezervat pentru memoria flash', ' is reserved for flash memory')
    .replace('scrierea in NVS a esuat', 'failed to write to NVS')
    .replace('SDA si SCL trebuie configurati impreuna sau dezactivati impreuna',
             'SDA and SCL must be configured or disabled together')
    .replace('adresa OLED trebuie sa fie', 'OLED address must be');
}

function gpioLabel(pin) {
  return Number(pin) >= 0 ? 'GPIO' + pin : tr('neconfigurat', 'not configured');
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
  if (profile.indexOf('T-ZIGBEE') >= 0) return boardLayouts.tzigbee;
  if (profile.indexOf('C6-SuperMini') >= 0) return boardLayouts.c6supermini;
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
    roleText.textContent = ' | ' + translateHardwareText(role);
    text.appendChild(roleText);
  }

  if (info) {
    var details = [];
    if (info.restriction) details.push(translateHardwareText(info.restriction));
    if (info.warning) details.push(translateHardwareText(info.warning));
    if (!info.output) details.push(tr('doar intrare', 'input only'));
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

  if (currentLanguage === 'en') {
    document.getElementById('pinoutTitle').textContent = layout.title
      .replace('pini', 'pins').replace('headere', 'headers');
    document.getElementById('pinoutNote').textContent = layout.note
      .replace('Vedere de sus, USB jos.', 'Top view, USB at the bottom.')
      .replace('Profilul generic WROOM foloseste aranjamentul comun al placii cu 30 de pini.',
               'The generic WROOM profile uses the common 30-pin board layout.')
      .replace('Ordinea pinilor urmeaza headerele', 'Pin order follows headers')
      .replace('Ordinea celor 44 de pini urmeaza headerele', 'The 44-pin order follows headers')
      .replace('ale ', 'of ');
  } else {
    document.getElementById('pinoutTitle').textContent = layout.title;
    document.getElementById('pinoutNote').textContent = layout.note;
  }
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
  select.add(new Option(tr('Dezactivat / neconfigurat', 'Disabled / not configured'), '-1'));
  pins.forEach(function(p) {
    if (p.reserved || (needsOutput && !p.output)) return;
    var label = 'GPIO' + p.pin;
    if (!p.output) label += tr(' (doar intrare)', ' (input only)');
    if (p.role && p.pin !== Number(value))
      label += tr(' - ocupat: ', ' - assigned: ') + translateHardwareText(p.role);
    if (p.warning)
      label += tr(' - ATENTIE: ', ' - WARNING: ') + translateHardwareText(p.warning);
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
        tr('Cip: ', 'Chip: ') + d.target + ' | ' +
        tr('Profil build: ', 'Build profile: ') + d.profile + ' | ' +
        d.gpio_count + tr(' pozitii GPIO', ' GPIO positions');
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
        if (p.reserved) text += translateHardwareText(p.restriction || 'indisponibil');
        else if (p.role) text += translateHardwareText(p.role);
        else text += tr('liber', 'free');
        if (!p.output) text += tr(', doar IN', ', input only');
        if (p.warning) text += ' | ' + translateHardwareText(p.warning);
        item.textContent = text;
        grid.appendChild(item);
      });
    })
    .catch(function() {
      showHwMsg(tr('Eroare la citirea configuratiei hardware.',
                   'Error reading the hardware configuration.'), 'err');
    });
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

  showHwMsg(tr('Se valideaza si se salveaza...', 'Validating and saving...'), 'info');
  fetch('/api/hardware_config', {
    method: 'POST',
    headers: {'Content-Type': 'application/x-www-form-urlencoded'},
    body: body
  })
  .then(r => r.json().then(d => ({ok:r.ok, data:d})))
  .then(x => showHwMsg(
    x.ok ? tr('Configuratia a fost salvata. Dispozitivul reporneste.',
              'Configuration saved. The device is restarting.')
         : translateHardwareText(x.data.error || tr('Eroare', 'Error')),
    x.ok ? 'ok' : 'err'))
  .catch(function() {
    showHwMsg(tr('Conexiunea s-a inchis in timpul restartului.',
                 'The connection closed during restart.'), 'info');
  });
}

function resetHardware() {
  if (!confirm(tr('Restaurati pinii impliciti pentru profilul acestei placi?',
                  'Restore the default pins for this board profile?'))) return;
  fetch('/api/hardware_reset', {method:'POST'})
    .then(r => r.json())
    .then(d => showHwMsg(
      d.ok ? tr('Profilul implicit a fost restaurat. Dispozitivul reporneste.',
                'The default profile was restored. The device is restarting.')
           : tr('Resetarea configuratiei a esuat.', 'Failed to reset the configuration.'),
      d.ok ? 'ok' : 'err'))
    .catch(function() {
      showHwMsg(tr('Conexiunea s-a inchis in timpul restartului.',
                   'The connection closed during restart.'), 'info');
    });
}

function showHwMsg(txt, type) {
  var m = document.getElementById('hwMsg');
  m.textContent = txt;
  m.style.display = 'block';
  m.style.background = type === 'ok' ? '#d1fae5' : type === 'err' ? '#fee2e2' : '#dbeafe';
  m.style.color = type === 'ok' ? '#065f46' : type === 'err' ? '#991b1b' : '#1e40af';
}

// ---- Board Info ----
function setCapability(statusId, detailId, supported, detail) {
  var status = document.getElementById(statusId);
  status.textContent = supported ? tr('DA', 'YES') : tr('NU', 'NO');
  status.className = 'cap-status ' + (supported ? 'cap-yes' : 'cap-no');
  document.getElementById(detailId).textContent = detail;
}

function loadBoardInfo() {
  fetch('/api/board_info')
    .then(r => r.json())
    .then(d => {
      document.getElementById('biChipModel').textContent = d.chip_model || '-';
      document.getElementById('biBuildProfile').textContent = d.build_profile || '-';
      document.getElementById('biChipRev').textContent   = d.chip_revision;
      document.getElementById('biCores').textContent     = d.cpu_cores + ' x ' + d.cpu_freq + ' MHz';
      document.getElementById('biFreq').textContent      = d.cpu_freq + ' MHz';

      var zigbeeDetail;
      if (d.zigbee_external) {
        zigbeeDetail = tr('Coprocesor TLSR8258 disponibil prin UART intern. Mod firmware: ',
                          'TLSR8258 coprocessor available over internal UART. Firmware mode: ') +
          (d.zigbee_firmware_mode || tr('necunoscut', 'unknown')) + '.';
      } else if (d.zigbee_capable) {
        zigbeeDetail = tr('Radio IEEE 802.15.4 disponibil. Mod firmware: ',
                          'IEEE 802.15.4 radio available. Firmware mode: ') +
          (d.zigbee_firmware_mode || tr('necunoscut', 'unknown')) + '.';
      } else {
        zigbeeDetail = tr('Lipseste radioul IEEE 802.15.4 necesar pentru Zigbee nativ.',
                          'The IEEE 802.15.4 radio required for native Zigbee is not available.');
      }
      setCapability('biZigbeeStatus', 'biZigbeeDetail', d.zigbee_capable, zigbeeDetail);

      var threadDetail = d.thread_capable
        ? tr('Radio IEEE 802.15.4 disponibil pentru OpenThread si Matter over Thread.',
             'IEEE 802.15.4 radio available for OpenThread and Matter over Thread.')
        : tr('Thread nativ nu este disponibil fara radio IEEE 802.15.4.',
             'Native Thread is unavailable without an IEEE 802.15.4 radio.');
      setCapability('biThreadStatus', 'biThreadDetail', d.thread_capable, threadDetail);

      var matterTransports = [];
      if (d.matter_wifi_capable) matterTransports.push('WiFi');
      if (d.matter_thread_capable) matterTransports.push('Thread');
      var matterDetail = d.matter_capable
        ? tr('Transporturi hardware posibile: ', 'Available hardware transports: ') +
          matterTransports.join(' + ') + '.'
        : tr('Placa nu ofera un transport IP compatibil Matter in aceasta configuratie.',
             'The board does not provide a Matter-compatible IP transport in this configuration.');
      setCapability('biMatterStatus', 'biMatterDetail', d.matter_capable, matterDetail);
      document.getElementById('biCommunicationSummary').textContent =
        tr('Comunicare selectata: ', 'Selected communication: ') +
        communicationLabel(d.communication_selected) + ' | ' +
        tr('activa: ', 'active: ') + communicationLabel(d.communication_active);

      document.getElementById('biFlash').textContent     = (d.flash_size / 1048576).toFixed(1) + ' MB';
      document.getElementById('biPsram').textContent     =
        d.psram_size > 0 ? (d.psram_size / 1048576).toFixed(1) + ' MB' : tr('Nu', 'No');
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
    .catch(function() {
      alert(tr('Eroare la citirea informatiilor placii!',
               'Error reading board information!'));
    });
}

loadHardwareCfg();
loadUiLanguage();
</script>
</body>
</html>
)rawliteral";


#endif // WEBPAGES_H
