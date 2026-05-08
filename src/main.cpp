/*
  ESP32 MQTT Home Assistant DIY Kit
  ====================================
  Hardware:
  - ESP-WROOM-32 (30-pin, CH340, Type-C)
  - Senzor temperatura/umiditate DHT11 KY-015  -> GPIO4
  - Senzor PIR miscare HC-SR501                -> GPIO32
  - Releu SSR Low-Level 5V (LOW = ON)          -> GPIO23
  - OLED 0.96" SSD1306 I2C 128x64             -> SDA=GPIO21, SCL=GPIO22
  - LED heartbeat                              -> GPIO18

  Functionalitati:
  - MQTT cu Home Assistant Auto-Discovery
  - Web dashboard in timp real
  - Display OLED cu status complet
  - Configurare WiFi prin browser (AP mode)
  - FreeRTOS tasks pentru citire senzori

  IMPORTANT: Editati sectiunea "Configuratie MQTT" inainte de upload!
*/

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiWebManager.h>
#include <WebPages.h>
#include <PubSubClient.h>
#include <DHT.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Preferences.h>
#include <esp_chip_info.h>
#include <esp_system.h>

// ============================================================
//  Definitii pini hardware
// ============================================================
#define DHT_PIN         4       // DHT11 data line (KY-015)
#define DHT_TYPE        DHT11
#define PIR_PIN         32      // HC-SR501 OUT  (HIGH = miscare detectata)
#define RELAY_PIN       23      // SSR Low-Level (LOW = releu ON, HIGH = releu OFF)
#define HEARTBEAT_PIN   18      // LED status

// OLED SSD1306 128x64 I2C (SDA=GPIO21, SCL=GPIO22 - default ESP32)
#define OLED_WIDTH      128
#define OLED_HEIGHT     64
#define OLED_RESET      -1
#define OLED_ADDR       0x3C

// ============================================================
//  Configuratie MQTT — stocata in NVS, editabila din pagina web
// ============================================================
String g_mqttBroker   = "192.168.1.100";
int    g_mqttPort     = 1883;
String g_mqttUser     = "";
String g_mqttPass     = "";
String g_mqttClientId = "esp32-ha-kit";

// Topice MQTT
static const char* TOPIC_STATE       = "esp32kit/state";
static const char* TOPIC_RELAY_STATE = "esp32kit/relay/state";
static const char* TOPIC_RELAY_CMD   = "esp32kit/relay/command";

// Topice Home Assistant Auto-Discovery
static const char* DISC_TEMP   = "homeassistant/sensor/esp32kit/temperature/config";
static const char* DISC_HUM    = "homeassistant/sensor/esp32kit/humidity/config";
static const char* DISC_MOTION = "homeassistant/binary_sensor/esp32kit/motion/config";
static const char* DISC_RELAY  = "homeassistant/switch/esp32kit/relay/config";

// NVS pentru setarile MQTT
Preferences mqttPrefs;

void loadMqttConfig()
{
  mqttPrefs.begin("mqtt", true);
  g_mqttBroker   = mqttPrefs.getString("broker",   g_mqttBroker);
  g_mqttPort     = mqttPrefs.getInt(   "port",     g_mqttPort);
  g_mqttUser     = mqttPrefs.getString("user",     g_mqttUser);
  g_mqttPass     = mqttPrefs.getString("pass",     g_mqttPass);
  g_mqttClientId = mqttPrefs.getString("client_id",g_mqttClientId);
  mqttPrefs.end();
  Serial.printf("[MQTT] Config NVS: %s:%d  user='%s'  client='%s'\n",
                g_mqttBroker.c_str(), g_mqttPort,
                g_mqttUser.c_str(), g_mqttClientId.c_str());
}

void saveMqttConfig(const String& broker, int port,
                    const String& user,   const String& pass,
                    const String& clientId)
{
  mqttPrefs.begin("mqtt", false);
  mqttPrefs.putString("broker",    broker);
  mqttPrefs.putInt(   "port",      port);
  mqttPrefs.putString("user",      user);
  mqttPrefs.putString("pass",      pass);
  mqttPrefs.putString("client_id", clientId);
  mqttPrefs.end();
  g_mqttBroker   = broker;
  g_mqttPort     = port;
  g_mqttUser     = user;
  g_mqttPass     = pass;
  g_mqttClientId = clientId;
  Serial.println("[MQTT] Configuratie salvata in NVS");
}

// ============================================================
//  Obiecte globale
// ============================================================
WiFiWebManager   wifiManager("ESP32_HAKit", "12345678");
DHT              dht(DHT_PIN, DHT_TYPE);
Adafruit_SSD1306 oled(OLED_WIDTH, OLED_HEIGHT, &Wire, OLED_RESET);
WiFiClient       wifiClient;
PubSubClient     mqttClient(wifiClient);

// ============================================================
//  Variabile partajate (protejate de mutex intre task-uri)
// ============================================================
portMUX_TYPE g_mux = portMUX_INITIALIZER_UNLOCKED;

volatile float g_temperature    = NAN;
volatile float g_humidity       = NAN;
volatile bool  g_motionDetected = false;
volatile bool  g_relayActive    = false;

// Accesat doar din loop() (core 1 - Arduino loop task)
static bool g_mqttConnected      = false;
static bool g_discoveryPublished = false;

// ============================================================
//  Control releu
// ============================================================
void setRelay(bool active)
{
  // SSR Low-Level: LOW = releu pornit, HIGH = releu oprit
  digitalWrite(RELAY_PIN, active ? LOW : HIGH);

  portENTER_CRITICAL(&g_mux);
  g_relayActive = active;
  portEXIT_CRITICAL(&g_mux);

  if (mqttClient.connected())
    mqttClient.publish(TOPIC_RELAY_STATE, active ? "ON" : "OFF", /*retain=*/true);
}

// ============================================================
//  OLED helpers  (apelat doar din loop() - Wire-safe pe core 1)
// ============================================================
void oledDrawStatus()
{
  float temp, hum;
  bool  motion, relay;

  portENTER_CRITICAL(&g_mux);
  temp   = g_temperature;
  hum    = g_humidity;
  motion = g_motionDetected;
  relay  = g_relayActive;
  portEXIT_CRITICAL(&g_mux);

  oled.clearDisplay();
  oled.setTextColor(SSD1306_WHITE);
  oled.setTextSize(1);

  // Titlu
  oled.setCursor(22, 0);
  oled.print(F("ESP32 HA Kit"));
  oled.drawLine(0, 9, OLED_WIDTH - 1, 9, SSD1306_WHITE);

  // Temperatura
  oled.setCursor(0, 13);
  oled.print(F("Temp:  "));
  if (!isnan(temp)) { oled.print(temp, 1); oled.print(F(" C")); }
  else               oled.print(F("--.-"));

  // Umiditate
  oled.setCursor(0, 24);
  oled.print(F("Umid:  "));
  if (!isnan(hum)) { oled.print(hum, 1); oled.print(F(" %")); }
  else              oled.print(F("--.-"));

  // Miscare PIR
  oled.setCursor(0, 35);
  oled.print(F("Miscare: "));
  if (motion)
  {
    oled.fillRect(68, 33, 58, 11, SSD1306_WHITE);
    oled.setTextColor(SSD1306_BLACK);
    oled.setCursor(70, 35);
    oled.print(F("DETECTAT"));
    oled.setTextColor(SSD1306_WHITE);
  }
  else
  {
    oled.print(F("Nu"));
  }

  // Releu
  oled.setCursor(0, 46);
  oled.print(F("Releu:   "));
  oled.print(relay ? F("ON ") : F("OFF"));

  // Bara status
  oled.drawLine(0, 54, OLED_WIDTH - 1, 54, SSD1306_WHITE);
  oled.setCursor(0, 56);
  if (wifiManager.isConnected())
  {
    String ip = wifiManager.getIPAddress();
    int dot1  = ip.indexOf('.', ip.indexOf('.') + 1);
    oled.print(ip.substring(dot1 + 1));         // ultimii 2 octeti
    oled.print(g_mqttConnected ? F(" MQTT:OK") : F(" MQTT:--"));
  }
  else
  {
    oled.print(F("AP: ESP32_HAKit"));
  }

  oled.display();
}

// ============================================================
//  FreeRTOS Tasks
// ============================================================
void taskSensors(void* parameter)
{
  (void)parameter;

  unsigned long lastDhtRead = 0;

  while (true)
  {
    unsigned long now = millis();

    // DHT11: citire la fiecare 2 secunde (max 1 citire/sec conform datasheet)
    if (now - lastDhtRead >= 2000)
    {
      lastDhtRead = now;
      float t = dht.readTemperature();
      float h = dht.readHumidity();

      if (!isnan(t) && !isnan(h))
      {
        portENTER_CRITICAL(&g_mux);
        g_temperature = t;
        g_humidity    = h;
        portEXIT_CRITICAL(&g_mux);
      }
    }

    // PIR HC-SR501: citire continua (HIGH = miscare detectata)
    bool motion = (digitalRead(PIR_PIN) == HIGH);
    portENTER_CRITICAL(&g_mux);
    g_motionDetected = motion;
    portEXIT_CRITICAL(&g_mux);

    vTaskDelay(pdMS_TO_TICKS(100));
  }
}

void taskHeartbeat(void* parameter)
{
  (void)parameter;

  bool state = false;
  while (true)
  {
    state = !state;
    digitalWrite(HEARTBEAT_PIN, state ? HIGH : LOW);
    vTaskDelay(pdMS_TO_TICKS(500));
  }
}

// ============================================================
//  MQTT
// ============================================================
void mqttCallback(char* topic, byte* payload, unsigned int length)
{
  String msg;
  for (unsigned int i = 0; i < length; ++i)
    msg += static_cast<char>(payload[i]);

  if (String(topic) == String(TOPIC_RELAY_CMD))
  {
    if      (msg == "ON")  setRelay(true);
    else if (msg == "OFF") setRelay(false);
  }
}

void publishDiscovery()
{
  // Bloc device comun (inclus in fiecare config)
  const String dev =
    "\"device\":{"
    "\"identifiers\":[\"esp32kit\"],"
    "\"name\":\"ESP32 HA Kit\","
    "\"model\":\"ESP-WROOM-32\","
    "\"manufacturer\":\"Espressif Systems\","
    "\"sw_version\":\"1.0.0\""
    "}";

  // Temperatura
  {
    String cfg = "{\"name\":\"Temperatura\","
                 "\"device_class\":\"temperature\","
                 "\"unit_of_measurement\":\"\xC2\xB0" "C\","
                 "\"state_topic\":\"" + String(TOPIC_STATE) + "\","
                 "\"value_template\":\"{{ value_json.temperature | round(1) }}\","
                 "\"unique_id\":\"esp32kit_temperature\","
                 + dev + "}";
    mqttClient.publish(DISC_TEMP, cfg.c_str(), true);
  }

  // Umiditate
  {
    String cfg = "{\"name\":\"Umiditate\","
                 "\"device_class\":\"humidity\","
                 "\"unit_of_measurement\":\"%\","
                 "\"state_topic\":\"" + String(TOPIC_STATE) + "\","
                 "\"value_template\":\"{{ value_json.humidity | round(1) }}\","
                 "\"unique_id\":\"esp32kit_humidity\","
                 + dev + "}";
    mqttClient.publish(DISC_HUM, cfg.c_str(), true);
  }

  // Senzor PIR miscare
  {
    String cfg = "{\"name\":\"Miscare\","
                 "\"device_class\":\"motion\","
                 "\"state_topic\":\"" + String(TOPIC_STATE) + "\","
                 "\"value_template\":\"{{ value_json.motion }}\","
                 "\"payload_on\":\"true\","
                 "\"payload_off\":\"false\","
                 "\"unique_id\":\"esp32kit_motion\","
                 + dev + "}";
    mqttClient.publish(DISC_MOTION, cfg.c_str(), true);
  }

  // Releu switch
  {
    String cfg = "{\"name\":\"Releu\","
                 "\"state_topic\":\"" + String(TOPIC_RELAY_STATE) + "\","
                 "\"command_topic\":\"" + String(TOPIC_RELAY_CMD) + "\","
                 "\"payload_on\":\"ON\","
                 "\"payload_off\":\"OFF\","
                 "\"unique_id\":\"esp32kit_relay\","
                 + dev + "}";
    mqttClient.publish(DISC_RELAY, cfg.c_str(), true);
  }

  g_discoveryPublished = true;
  Serial.println("[MQTT] Home Assistant discovery publicat");
}

bool mqttReconnect()
{
  mqttClient.setServer(g_mqttBroker.c_str(), g_mqttPort);
  Serial.printf("[MQTT] Conectare la %s:%d ...\n", g_mqttBroker.c_str(), g_mqttPort);

  bool ok = (g_mqttUser.length() > 0)
    ? mqttClient.connect(g_mqttClientId.c_str(), g_mqttUser.c_str(), g_mqttPass.c_str())
    : mqttClient.connect(g_mqttClientId.c_str());

  if (ok)
  {
    Serial.println("[MQTT] Conectat!");
    mqttClient.subscribe(TOPIC_RELAY_CMD);
    g_mqttConnected      = true;
    g_discoveryPublished = false;   // re-publica discovery dupa reconectare
  }
  else
  {
    Serial.printf("[MQTT] Esuat (rc=%d) broker=%s:%d\n", mqttClient.state(), g_mqttBroker.c_str(), g_mqttPort);
    g_mqttConnected = false;
  }
  return ok;
}

void handleMqtt()
{
  if (!wifiManager.isConnected()) return;

  if (!mqttClient.connected())
  {
    static unsigned long lastAttempt = 0;
    if (millis() - lastAttempt >= 5000)
    {
      lastAttempt = millis();
      g_mqttConnected = false;
      mqttReconnect();
    }
    return;
  }

  mqttClient.loop();

  if (!g_discoveryPublished)
    publishDiscovery();
}

void publishState()
{
  if (!mqttClient.connected()) return;

  float temp, hum;
  bool  motion, relay;

  portENTER_CRITICAL(&g_mux);
  temp   = g_temperature;
  hum    = g_humidity;
  motion = g_motionDetected;
  relay  = g_relayActive;
  portEXIT_CRITICAL(&g_mux);

  String json = "{";
  json += "\"temperature\":";
  json += isnan(temp) ? "null" : String(temp, 1);
  json += ",\"humidity\":";
  json += isnan(hum)  ? "null" : String(hum, 1);
  json += ",\"motion\":";
  json += motion ? "true" : "false";
  json += ",\"relay\":";
  json += relay  ? "true" : "false";
  json += ",\"uptime\":";
  json += String(millis() / 1000);
  json += "}";

  mqttClient.publish(TOPIC_STATE, json.c_str());
  mqttClient.publish(TOPIC_RELAY_STATE, relay ? "ON" : "OFF", true);
}

// ============================================================
//  Web Server Handlers
// ============================================================
void handleRoot(WebServer& server)
{
  server.send_P(200, "text/html", htmlPage);
}

void handleData(WebServer& server)
{
  float temp, hum;
  bool  motion, relay;

  portENTER_CRITICAL(&g_mux);
  temp   = g_temperature;
  hum    = g_humidity;
  motion = g_motionDetected;
  relay  = g_relayActive;
  portEXIT_CRITICAL(&g_mux);

  String json = "{";
  json += "\"temperature\":";
  json += isnan(temp) ? "null" : String(temp, 1);
  json += ",\"humidity\":";
  json += isnan(hum)  ? "null" : String(hum, 1);
  json += ",\"motion\":";
  json += motion ? "true" : "false";
  json += ",\"relay\":";
  json += relay  ? "true" : "false";
  json += ",\"mqtt_connected\":";
  json += g_mqttConnected ? "true" : "false";
  json += ",\"uptime_ms\":";
  json += String(millis());
  json += "}";

  server.send(200, "application/json", json);
}

void handleApiRelay(WebServer& server)
{
  if (!server.hasArg("state"))
  {
    server.send(400, "application/json", "{\"error\":\"missing state argument\"}");
    return;
  }
  String s       = server.arg("state");
  bool   newState = (s == "1" || s == "ON" || s == "on");
  setRelay(newState);
  server.send(200, "application/json", "{\"ok\":true}");
}

// GET /api/mqtt_config  → returneaza configuratia curenta (fara parola)
void handleMqttConfigGet(WebServer& server)
{
  String json = "{";
  json += "\"broker\":\"" + g_mqttBroker + "\",";
  json += "\"port\":"      + String(g_mqttPort) + ",";
  json += "\"user\":\""   + g_mqttUser + "\",";
  json += "\"client_id\":\"" + g_mqttClientId + "\",";
  json += "\"connected\":" + String(g_mqttConnected ? "true" : "false");
  json += "}";
  server.send(200, "application/json", json);
}

// POST /api/mqtt_config  → salveaza configuratia si reconnecteaza
void handleMqttConfigPost(WebServer& server)
{
  if (!server.hasArg("broker") || server.arg("broker").length() == 0)
  {
    server.send(400, "application/json", "{\"error\":\"broker obligatoriu\"}");
    return;
  }

  String broker   = server.arg("broker");
  int    port     = server.hasArg("port") ? server.arg("port").toInt() : 1883;
  String user     = server.hasArg("user")      ? server.arg("user")      : "";
  String pass     = server.hasArg("pass")      ? server.arg("pass")      : "";
  String clientId = server.hasArg("client_id") ? server.arg("client_id") : "esp32-ha-kit";

  // Validare port
  if (port <= 0 || port > 65535) port = 1883;

  saveMqttConfig(broker, port, user, pass, clientId);

  // Deconecteaza clientul MQTT pentru a forta reconectare cu noile setari
  if (mqttClient.connected()) mqttClient.disconnect();
  g_mqttConnected      = false;
  g_discoveryPublished = false;

  server.send(200, "application/json", "{\"ok\":true,\"message\":\"Salvat! ESP32 se reconecteaza la broker.\"}");
}

void handleApiStatus(WebServer& server)
{
  float temp, hum;
  bool  motion, relay;

  portENTER_CRITICAL(&g_mux);
  temp   = g_temperature;
  hum    = g_humidity;
  motion = g_motionDetected;
  relay  = g_relayActive;
  portEXIT_CRITICAL(&g_mux);

  String json = "{";
  json += "\"device\":\"esp32-ha-kit\",";
  json += "\"ip\":\"" + wifiManager.getIPAddress() + "\",";
  json += "\"temperature\":";
  json += isnan(temp) ? "null" : String(temp, 1);
  json += ",\"humidity\":";
  json += isnan(hum)  ? "null" : String(hum, 1);
  json += ",\"motion\":";
  json += motion ? "true" : "false";
  json += ",\"relay\":";
  json += relay  ? "true" : "false";
  json += ",\"mqtt_connected\":";
  json += g_mqttConnected ? "true" : "false";
  json += ",\"uptime_ms\":";
  json += String(millis());
  json += "}";

  server.send(200, "application/json", json);
}

void handleBoardInfo(WebServer& server)
{
  esp_chip_info_t chip_info;
  esp_chip_info(&chip_info);

  // Use WiFi MAC (already available, no extra header needed)
  String macStr = WiFi.macAddress();

  String resetReason;
  switch (esp_reset_reason()) {
    case ESP_RST_POWERON:  resetReason = "Power On";          break;
    case ESP_RST_EXT:      resetReason = "External Pin";      break;
    case ESP_RST_SW:       resetReason = "Software Reset";    break;
    case ESP_RST_PANIC:    resetReason = "Exception/Panic";   break;
    case ESP_RST_INT_WDT:  resetReason = "Interrupt Watchdog"; break;
    case ESP_RST_TASK_WDT: resetReason = "Task Watchdog";     break;
    case ESP_RST_DEEPSLEEP: resetReason = "Deep Sleep";       break;
    case ESP_RST_BROWNOUT: resetReason = "Brownout";          break;
    default:               resetReason = "Other";             break;
  }

  String json = "{";
  json += "\"chip_model\":\"ESP32\",";
  json += "\"chip_revision\":" + String(chip_info.revision) + ",";
  json += "\"cpu_cores\":" + String(chip_info.cores) + ",";
  json += "\"cpu_freq\":" + String(ESP.getCpuFreqMHz()) + ",";
  json += "\"flash_size\":" + String(ESP.getFlashChipSize()) + ",";
  json += "\"psram_size\":" + String(ESP.getPsramSize()) + ",";
  json += "\"free_heap\":" + String(ESP.getFreeHeap()) + ",";
  json += "\"min_free_heap\":" + String(ESP.getMinFreeHeap()) + ",";
  json += "\"sdk_version\":\"" + String(ESP.getSdkVersion()) + "\",";
  json += "\"mac_address\":\"" + macStr + "\",";
  json += "\"uptime_ms\":" + String(millis()) + ",";
  json += "\"reset_reason\":\"" + resetReason + "\",";
  json += "\"sketch_size\":" + String(ESP.getSketchSize()) + ",";
  json += "\"free_sketch_space\":" + String(ESP.getFreeSketchSpace()) + ",";
  json += "\"sketch_md5\":\"" + String(ESP.getSketchMD5()) + "\"";
  json += "}";

  server.send(200, "application/json", json);
}

// ============================================================
//  setup() & loop()
// ============================================================
void setup()
{
  Serial.begin(115200);
  delay(300);
  Serial.println("\n=== ESP32 MQTT Home Assistant DIY Kit ===");

  // Configurare pini
  pinMode(PIR_PIN,       INPUT);
  pinMode(RELAY_PIN,     OUTPUT);
  pinMode(HEARTBEAT_PIN, OUTPUT);
  digitalWrite(RELAY_PIN,     HIGH);   // SSR OFF la pornire (Low-Level: HIGH = OFF)
  digitalWrite(HEARTBEAT_PIN, LOW);

  // DHT11
  dht.begin();
  Serial.println("[DHT11] Initializat pe GPIO" + String(DHT_PIN));

  // OLED SSD1306 (I2C pe core 1 - nu folositi Wire din alt task!)
  Wire.begin();
  if (!oled.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR))
  {
    Serial.println("[OLED] EROARE init! Verifica cablajul si adresa I2C (0x3C/0x3D).");
  }
  else
  {
    oled.clearDisplay();
    oled.setTextSize(1);
    oled.setTextColor(SSD1306_WHITE);
    oled.setCursor(16, 20);
    oled.print(F("ESP32 HA Kit"));
    oled.setCursor(10, 35);
    oled.print(F("Initializing..."));
    oled.display();
    Serial.println("[OLED] Initializat (0x3C, SDA=GPIO21, SCL=GPIO22)");
  }

  // WiFiWebManager
  wifiManager.begin();

  // Rute web
  wifiManager.on("/",                    HTTP_GET,  handleRoot);
  wifiManager.on("/data",                HTTP_GET,  handleData);
  wifiManager.on("/api/status",          HTTP_GET,  handleApiStatus);
  wifiManager.on("/api/relay",           HTTP_GET,  handleApiRelay);
  wifiManager.on("/api/board_info",      HTTP_GET,  handleBoardInfo);
  wifiManager.on("/api/mqtt_config",     HTTP_GET,  handleMqttConfigGet);
  wifiManager.on("/api/mqtt_config",     HTTP_POST, handleMqttConfigPost);

  // Incarca si aplica configuratia MQTT din NVS
  loadMqttConfig();
  mqttClient.setServer(g_mqttBroker.c_str(), g_mqttPort);
  mqttClient.setCallback(mqttCallback);
  mqttClient.setBufferSize(512);

  // FreeRTOS tasks (core 0 - lasa loop/WiFi/MQTT pe core 1)
  xTaskCreatePinnedToCore(taskSensors,   "taskSensors",   4096, nullptr, 2, nullptr, 0);
  xTaskCreatePinnedToCore(taskHeartbeat, "taskHeartbeat", 1024, nullptr, 1, nullptr, 0);

  Serial.println("[WiFi] AP: ESP32_HAKit  /  Parola: 12345678");
  Serial.println("[Web]  http://" + wifiManager.getIPAddress());
  Serial.println("[MQTT] Broker: " + g_mqttBroker + ":" + String(g_mqttPort));
  Serial.println("[Pini] DHT11=GPIO4  PIR=GPIO32  Releu=GPIO23  SDA=GPIO21  SCL=GPIO22");
  Serial.println("=========================================\n");
}

void loop()
{
  wifiManager.handleClient();
  handleMqtt();

  // Detectie schimbare stare PIR → publish imediat
  static bool lastMotion = false;
  bool currentMotion;
  portENTER_CRITICAL(&g_mux);
  currentMotion = g_motionDetected;
  portEXIT_CRITICAL(&g_mux);

  if (currentMotion != lastMotion)
  {
    lastMotion = currentMotion;
    publishState();
  }

  // OLED: actualizare la 500 ms (Wire doar din loop - core 1)
  static unsigned long lastOled = 0;
  if (millis() - lastOled >= 500)
  {
    lastOled = millis();
    oledDrawStatus();
  }

  // Publish MQTT periodic la 30 secunde
  static unsigned long lastPublish = 0;
  if (millis() - lastPublish >= 30000)
  {
    lastPublish = millis();
    publishState();
  }

  // Log serial la 5 secunde
  static unsigned long lastLog = 0;
  if (millis() - lastLog >= 5000)
  {
    lastLog = millis();

    float temp, hum;
    bool  motion, relay;
    portENTER_CRITICAL(&g_mux);
    temp   = g_temperature;
    hum    = g_humidity;
    motion = g_motionDetected;
    relay  = g_relayActive;
    portEXIT_CRITICAL(&g_mux);

    Serial.println("--- Status ---");
    if (!isnan(temp))
      Serial.printf("  Temperatura: %.1f C  Umiditate: %.1f%%\n", temp, hum);
    else
      Serial.println("  DHT11: citire invalida (verifica cablaj GPIO4)");
    Serial.printf("  Miscare PIR: %s | Releu: %s\n",
                  motion ? "DETECTATA" : "Nu",
                  relay  ? "ON" : "OFF");
    Serial.printf("  WiFi: %s | MQTT: %s\n",
                  wifiManager.isConnected()
                    ? ("STA " + wifiManager.getIPAddress()).c_str()
                    : "AP mode",
                  g_mqttConnected ? "Conectat" : "Deconectat");
  }
}


