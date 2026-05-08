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
        <div style="font-size:.8em;opacity:.8;margin-top:4px">HC-SR501 &bull; GPIO32</div>
      </div>

      <!-- Relay -->
      <div class="card card-relay">
        <div class="card-label">Releu SSR</div>
        <div class="relay-state" id="relayVal">OFF</div>
        <div style="font-size:.8em;opacity:.8;margin-bottom:6px">GPIO23 &bull; Low-Level</div>
        <div class="relay-btns">
          <button onclick="setRelay(1)">&#9889; ON</button>
          <button onclick="setRelay(0)">&#9898; OFF</button>
        </div>
      </div>
    </div>

    <div class="info-box">
      <strong>&#128268; Cablaj hardware:</strong>
      <ul>
        <li>DHT11 (KY-015) DATA &rarr; GPIO4 &nbsp;|&nbsp; VCC=3.3V, GND=GND</li>
        <li>PIR HC-SR501 OUT &rarr; GPIO32 &nbsp;|&nbsp; VCC=5V, GND=GND</li>
        <li>Releu SSR IN &rarr; GPIO23 &nbsp;|&nbsp; VCC=5V, GND=GND &nbsp;(LOW=ON)</li>
        <li>OLED SSD1306 SDA &rarr; GPIO21 &nbsp;|&nbsp; SCL &rarr; GPIO22 &nbsp;|&nbsp; VCC=3.3V</li>
        <li>LED Heartbeat &rarr; GPIO18</li>
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

  <!-- BOARD INFO TAB -->
  <div id="board-tab" class="tab">
    <button class="bi-btn" onclick="loadBoardInfo()">&#128260; Incarca informatii placa</button>
    <div id="boardContent" style="display:none">
      <div class="bi-section bi-chip">
        <div class="bi-title">&#128295; Chip</div>
        <div class="bi-grid">
          <div><div class="bi-item-label">Model</div><div class="bi-item-value" id="biChipModel">-</div></div>
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

// ---- Board Info ----
function loadBoardInfo() {
  fetch('/api/board_info')
    .then(r => r.json())
    .then(d => {
      document.getElementById('biChipModel').textContent = d.chip_model || '-';
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
</script>
</body>
</html>
)rawliteral";


#endif // WEBPAGES_H
