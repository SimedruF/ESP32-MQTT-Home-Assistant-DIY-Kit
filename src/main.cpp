/*
  ESP32 MQTT Home Assistant DIY Kit
  ====================================
  Tintele suportate:
  - ESP32-WROOM-32 / ESP32 Dev Module
  - ESP32-C3-DevKitM-1
  - ESP32-C6-DevKitC-1
  - ESP32-S3-DevKitC-1

  Pinii pentru DHT11, PIR, releu, OLED si heartbeat sunt configurabili
  din interfata web si persistati in NVS.

  Functionalitati:
  - MQTT cu Home Assistant Auto-Discovery
  - Web dashboard in timp real
  - Display OLED cu status complet
  - Configurare WiFi prin browser (AP mode)
  - FreeRTOS tasks pentru citire senzori

*/

#include <Arduino.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <WiFiWebManager.h>
#include <WebPages.h>
#include <HardwareConfig.h>
#include <PubSubClient.h>
#include <DHT.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Preferences.h>
#include <esp_chip_info.h>
#include <esp_system.h>
#include <soc/soc_caps.h>
#include <SerialLog.h>

#define DHT_TYPE        DHT11

// OLED SSD1306 128x64 I2C
#define OLED_WIDTH      128
#define OLED_HEIGHT     64
#define OLED_RESET      -1
#define OLED_I2C_FREQ   400000UL

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
Preferences uiPrefs;
String g_uiLanguage = "ro";

void loadUiConfig()
{
  if (!uiPrefs.begin("ui", false))
  {
    serialLog.println("[UI] NVS indisponibil; se foloseste limba romana");
    return;
  }
  g_uiLanguage = uiPrefs.getString("language", "ro");
  if (g_uiLanguage != "ro" && g_uiLanguage != "en")
    g_uiLanguage = "ro";
  uiPrefs.end();
  serialLog.println("[UI] Limba interfetei: " + g_uiLanguage);
}

bool saveUiLanguage(const String& language)
{
  if (language != "ro" && language != "en") return false;
  if (!uiPrefs.begin("ui", false)) return false;
  const bool saved = uiPrefs.putString("language", language) > 0;
  uiPrefs.end();
  if (saved) g_uiLanguage = language;
  return saved;
}

void loadMqttConfig()
{
  // Read-write creates the namespace on first boot. Opening a missing namespace
  // read-only makes Preferences emit ESP_ERR_NVS_NOT_FOUND even though defaults work.
  if (!mqttPrefs.begin("mqtt", false))
  {
    serialLog.println("[MQTT] NVS indisponibil; se foloseste configuratia implicita");
    return;
  }
  g_mqttBroker   = mqttPrefs.getString("broker",   g_mqttBroker);
  g_mqttPort     = mqttPrefs.getInt(   "port",     g_mqttPort);
  g_mqttUser     = mqttPrefs.getString("user",     g_mqttUser);
  g_mqttPass     = mqttPrefs.getString("pass",     g_mqttPass);
  g_mqttClientId = mqttPrefs.getString("client_id",g_mqttClientId);
  mqttPrefs.end();
  serialLog.printf("[MQTT] Config NVS: %s:%d  user='%s'  client='%s'\n",
                   g_mqttBroker.c_str(), g_mqttPort,
                   g_mqttUser.c_str(), g_mqttClientId.c_str());
}

void saveMqttConfig(const String& broker, int port,
                    const String& user,   const String& pass,
                    const String& clientId)
{
  if (!mqttPrefs.begin("mqtt", false))
  {
    serialLog.println("[MQTT] Eroare la deschiderea NVS pentru scriere");
    return;
  }
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
  serialLog.println("[MQTT] Configuratie salvata in NVS");
}

// ============================================================
//  Obiecte globale
// ============================================================
WiFiWebManager   wifiManager("ESP32_HAKit", "12345678");
DHT*             dht = nullptr;
// Aceeasi frecventa in timpul si dupa transfer evita recrearea repetata a
// handle-ului I2C in Arduino-ESP32 3.x.
Adafruit_SSD1306 oled(OLED_WIDTH, OLED_HEIGHT, &Wire, OLED_RESET,
                     OLED_I2C_FREQ, OLED_I2C_FREQ);
WiFiClient       wifiClient;
PubSubClient     mqttClient(wifiClient);
HardwareConfigStore hardwareStore;
HardwareConfig      g_hardware;

static bool g_oledReady = false;
static bool g_oledFailureReported = false;
static bool g_mdnsReady = false;
static unsigned long g_restartAt = 0;

#if defined(RGB_BUILTIN)
static constexpr bool RGB_LED_SUPPORTED = true;
#else
static constexpr bool RGB_LED_SUPPORTED = false;
#endif

static bool g_rgbLedOn = false;
static uint8_t g_rgbLedRed = 0;
static uint8_t g_rgbLedGreen = 128;
static uint8_t g_rgbLedBlue = 255;
static uint8_t g_rgbLedBrightness = 25;

// ============================================================
//  Variabile partajate (protejate de mutex intre task-uri)
// ============================================================
volatile float g_temperature    = NAN;
volatile float g_humidity       = NAN;
volatile bool  g_dhtPresent     = false;
volatile bool  g_motionDetected = false;
volatile bool  g_relayActive    = false;

// Accesat doar din loop() (core 1 - Arduino loop task)
static bool g_mqttConnected      = false;
static bool g_discoveryPublished = false;

// Mutex FreeRTOS pentru date partajate intre task-uri
// (portENTER_CRITICAL dezactiveaza intreruperile -> provoaca Interrupt WDT)
SemaphoreHandle_t g_mutex = nullptr;

// ============================================================
//  Control releu
// ============================================================
void setRelay(bool active)
{
  if (g_hardware.relayPin == PIN_DISABLED) return;

  const uint8_t activeLevel = g_hardware.relayActiveLow ? LOW : HIGH;
  digitalWrite(g_hardware.relayPin, active ? activeLevel : !activeLevel);

  xSemaphoreTake(g_mutex, portMAX_DELAY);
  g_relayActive = active;
  xSemaphoreGive(g_mutex);

  if (mqttClient.connected())
    mqttClient.publish(TOPIC_RELAY_STATE, active ? "ON" : "OFF", /*retain=*/true);
}

// ============================================================
//  Control LED RGB onboard
// ============================================================
void applyRgbLed()
{
#if defined(RGB_BUILTIN)
  if (!g_rgbLedOn)
  {
    rgbLedWrite(RGB_BUILTIN, 0, 0, 0);
    return;
  }

  const uint8_t red = static_cast<uint8_t>(
    (static_cast<uint16_t>(g_rgbLedRed) * g_rgbLedBrightness) / 100);
  const uint8_t green = static_cast<uint8_t>(
    (static_cast<uint16_t>(g_rgbLedGreen) * g_rgbLedBrightness) / 100);
  const uint8_t blue = static_cast<uint8_t>(
    (static_cast<uint16_t>(g_rgbLedBlue) * g_rgbLedBrightness) / 100);
  rgbLedWrite(RGB_BUILTIN, red, green, blue);
#endif
}

String rgbLedStateJson()
{
  String json;
  json.reserve(120);
  json = F("{\"supported\":");
  json += RGB_LED_SUPPORTED ? F("true") : F("false");
  json += F(",\"on\":");
  json += g_rgbLedOn ? F("true") : F("false");
  json += F(",\"red\":");
  json += g_rgbLedRed;
  json += F(",\"green\":");
  json += g_rgbLedGreen;
  json += F(",\"blue\":");
  json += g_rgbLedBlue;
  json += F(",\"brightness\":");
  json += g_rgbLedBrightness;
#if defined(PIN_RGB_LED)
  json += F(",\"pin\":");
  json += PIN_RGB_LED;
#else
  json += F(",\"pin\":-1");
#endif
  json += '}';
  return json;
}

// ============================================================
//  OLED helpers  (apelat doar din loop() - Wire-safe pe core 1)
// ============================================================
bool isI2cDevicePresent(uint8_t address)
{
  Wire.beginTransmission(address);
  return Wire.endTransmission(true) == 0;
}

void disableOled(const __FlashStringHelper* reason)
{
  g_oledReady = false;
  if (!g_oledFailureReported)
  {
    serialLog.print(F("[OLED] Dezactivat: "));
    serialLog.println(reason);
    g_oledFailureReported = true;
  }
}

void oledDrawStatus()
{
  if (!g_oledReady) return;
  if (!isI2cDevicePresent(g_hardware.oledAddress))
  {
    disableOled(F("display-ul nu mai raspunde pe magistrala I2C"));
    return;
  }

  float temp, hum;
  bool  motion, relay;

  xSemaphoreTake(g_mutex, portMAX_DELAY);
  temp   = g_temperature;
  hum    = g_humidity;
  motion = g_motionDetected;
  relay  = g_relayActive;
  xSemaphoreGive(g_mutex);

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
  uint8_t consecutiveDhtFailures = 0;

  while (true)
  {
    unsigned long now = millis();

    // DHT11: citire la fiecare 2 secunde (max 1 citire/sec conform datasheet)
    if (dht != nullptr && now - lastDhtRead >= 2000)
    {
      lastDhtRead = now;
      float t = dht->readTemperature();
      float h = dht->readHumidity();

      if (!isnan(t) && !isnan(h))
      {
        consecutiveDhtFailures = 0;
        xSemaphoreTake(g_mutex, portMAX_DELAY);
        g_temperature = t;
        g_humidity    = h;
        g_dhtPresent  = true;
        xSemaphoreGive(g_mutex);
      }
      else if (++consecutiveDhtFailures >= 3)
      {
        xSemaphoreTake(g_mutex, portMAX_DELAY);
        g_temperature = NAN;
        g_humidity    = NAN;
        g_dhtPresent  = false;
        xSemaphoreGive(g_mutex);
      }
    }

    // PIR HC-SR501: citire continua (HIGH = miscare detectata)
    if (g_hardware.pirPin != PIN_DISABLED)
    {
      bool motion = (digitalRead(g_hardware.pirPin) == HIGH);
      xSemaphoreTake(g_mutex, portMAX_DELAY);
      g_motionDetected = motion;
      xSemaphoreGive(g_mutex);
    }

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
    digitalWrite(g_hardware.heartbeatPin, state ? HIGH : LOW);
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
    "\"model\":\"" + String(ESP.getChipModel()) + "\","
    "\"manufacturer\":\"Espressif Systems\","
    "\"sw_version\":\"1.0.0\""
    "}";

  // Temperatura si umiditate
  if (g_hardware.dhtPin != PIN_DISABLED)
  {
    String cfg = "{\"name\":\"Temperatura\","
                 "\"device_class\":\"temperature\","
                 "\"unit_of_measurement\":\"\xC2\xB0" "C\","
                 "\"state_topic\":\"" + String(TOPIC_STATE) + "\","
                 "\"value_template\":\"{{ value_json.temperature | round(1) }}\","
                 "\"unique_id\":\"esp32kit_temperature\","
                 + dev + "}";
    mqttClient.publish(DISC_TEMP, cfg.c_str(), true);

    cfg = "{\"name\":\"Umiditate\","
          "\"device_class\":\"humidity\","
          "\"unit_of_measurement\":\"%\","
          "\"state_topic\":\"" + String(TOPIC_STATE) + "\","
          "\"value_template\":\"{{ value_json.humidity | round(1) }}\","
          "\"unique_id\":\"esp32kit_humidity\","
          + dev + "}";
    mqttClient.publish(DISC_HUM, cfg.c_str(), true);
  }
  else
  {
    mqttClient.publish(DISC_TEMP, "", true);
    mqttClient.publish(DISC_HUM, "", true);
  }

  // Senzor PIR miscare
  if (g_hardware.pirPin != PIN_DISABLED)
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
  else
  {
    mqttClient.publish(DISC_MOTION, "", true);
  }

  // Releu switch
  if (g_hardware.relayPin != PIN_DISABLED)
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
  else
  {
    mqttClient.publish(DISC_RELAY, "", true);
  }

  g_discoveryPublished = true;
  serialLog.println("[MQTT] Home Assistant discovery publicat");
}

bool mqttReconnect()
{
  mqttClient.setServer(g_mqttBroker.c_str(), g_mqttPort);
  serialLog.printf("[MQTT] Conectare la %s:%d ...\n", g_mqttBroker.c_str(), g_mqttPort);

  bool ok = (g_mqttUser.length() > 0)
    ? mqttClient.connect(g_mqttClientId.c_str(), g_mqttUser.c_str(), g_mqttPass.c_str())
    : mqttClient.connect(g_mqttClientId.c_str());

  if (ok)
  {
    serialLog.println("[MQTT] Conectat!");
    mqttClient.subscribe(TOPIC_RELAY_CMD);
    g_mqttConnected      = true;
    g_discoveryPublished = false;   // re-publica discovery dupa reconectare
  }
  else
  {
    serialLog.printf("[MQTT] Esuat (rc=%d) broker=%s:%d\n", mqttClient.state(), g_mqttBroker.c_str(), g_mqttPort);
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

  xSemaphoreTake(g_mutex, portMAX_DELAY);
  temp   = g_temperature;
  hum    = g_humidity;
  motion = g_motionDetected;
  relay  = g_relayActive;
  xSemaphoreGive(g_mutex);

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

void handleUiConfigGet(WebServer& server)
{
  server.sendHeader("Cache-Control", "no-store");
  server.send(200, "application/json",
              String(F("{\"language\":\"")) + g_uiLanguage + F("\"}"));
}

void handleUiConfigPost(WebServer& server)
{
  const String language = server.arg("language");
  if (language != "ro" && language != "en")
  {
    server.send(400, "application/json",
                "{\"ok\":false,\"error\":\"language must be ro or en\"}");
    return;
  }
  if (!saveUiLanguage(language))
  {
    server.send(500, "application/json",
                "{\"ok\":false,\"error\":\"failed to save language\"}");
    return;
  }
  server.send(200, "application/json",
              String(F("{\"ok\":true,\"language\":\"")) + g_uiLanguage + F("\"}"));
}

void handleData(WebServer& server)
{
  float temp, hum;
  bool  dhtPresent, motion, relay;

  xSemaphoreTake(g_mutex, portMAX_DELAY);
  temp       = g_temperature;
  hum        = g_humidity;
  dhtPresent = g_dhtPresent;
  motion     = g_motionDetected;
  relay      = g_relayActive;
  xSemaphoreGive(g_mutex);

  String json = "{";
  json += "\"dht_available\":";
  json += dhtPresent ? "true" : "false";
  json += ",\"pir_available\":";
  json += g_hardware.pirPin != PIN_DISABLED ? "true" : "false";
  json += ",\"relay_available\":";
  json += g_hardware.relayPin != PIN_DISABLED ? "true" : "false";
  json += ",\"oled_available\":";
  json += g_oledReady ? "true" : "false";
  json += ",\"heartbeat_available\":";
  json += g_hardware.heartbeatPin != PIN_DISABLED ? "true" : "false";
  json += ",\"temperature\":";
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

void handleSerialLog(WebServer& server)
{
  uint32_t requestedSequence = 0;
  if (server.hasArg("since"))
    requestedSequence = static_cast<uint32_t>(strtoul(server.arg("since").c_str(), nullptr, 10));

  uint32_t firstSequence;
  uint32_t nextSequence;
  bool dataWasDropped;
  String logData = serialLog.readSince(requestedSequence, firstSequence,
                                       nextSequence, dataWasDropped);

  server.sendHeader("Cache-Control", "no-store");
  server.sendHeader("X-Log-Start", String(firstSequence));
  server.sendHeader("X-Log-Next", String(nextSequence));
  server.sendHeader("X-Log-Dropped", dataWasDropped ? "1" : "0");
  server.send(200, "text/plain; charset=utf-8", logData);
}

void handleSerialLogClear(WebServer& server)
{
  serialLog.clear();
  server.sendHeader("Cache-Control", "no-store");
  server.send(200, "application/json", "{\"ok\":true}");
}

void handleApiRelay(WebServer& server)
{
  if (g_hardware.relayPin == PIN_DISABLED)
  {
    server.send(409, "application/json", "{\"error\":\"releul nu este configurat\"}");
    return;
  }

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

void handleApiRgbLedGet(WebServer& server)
{
  server.sendHeader("Cache-Control", "no-store");
  server.send(200, "application/json", rgbLedStateJson());
}

void handleApiRgbLedPost(WebServer& server)
{
  if (!RGB_LED_SUPPORTED)
  {
    server.send(409, "application/json",
                "{\"ok\":false,\"error\":\"placa nu expune un LED RGB onboard\"}");
    return;
  }

  if (server.hasArg("red") && server.hasArg("green") && server.hasArg("blue"))
  {
    const long red = server.arg("red").toInt();
    const long green = server.arg("green").toInt();
    const long blue = server.arg("blue").toInt();
    if (red < 0 || red > 255 || green < 0 || green > 255 || blue < 0 || blue > 255)
    {
      server.send(400, "application/json",
                  "{\"ok\":false,\"error\":\"valorile RGB trebuie sa fie intre 0 si 255\"}");
      return;
    }
    g_rgbLedRed = static_cast<uint8_t>(red);
    g_rgbLedGreen = static_cast<uint8_t>(green);
    g_rgbLedBlue = static_cast<uint8_t>(blue);
  }

  if (server.hasArg("brightness"))
  {
    const long brightness = server.arg("brightness").toInt();
    if (brightness < 0 || brightness > 100)
    {
      server.send(400, "application/json",
                  "{\"ok\":false,\"error\":\"luminozitatea trebuie sa fie intre 0 si 100\"}");
      return;
    }
    g_rgbLedBrightness = static_cast<uint8_t>(brightness);
  }

  if (server.hasArg("state"))
  {
    const String state = server.arg("state");
    g_rgbLedOn = state == "1" || state == "ON" || state == "on" || state == "true";
  }

  applyRgbLed();
  serialLog.printf("[RGB] %s #%02X%02X%02X, luminozitate %u%%\n",
                   g_rgbLedOn ? "ON" : "OFF",
                   g_rgbLedRed, g_rgbLedGreen, g_rgbLedBlue, g_rgbLedBrightness);

  String json = F("{\"ok\":true,\"led\":");
  json += rgbLedStateJson();
  json += '}';
  server.send(200, "application/json", json);
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

  xSemaphoreTake(g_mutex, portMAX_DELAY);
  temp   = g_temperature;
  hum    = g_humidity;
  motion = g_motionDetected;
  relay  = g_relayActive;
  xSemaphoreGive(g_mutex);

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

#if defined(SOC_WIFI_SUPPORTED) && SOC_WIFI_SUPPORTED
  const bool wifiCapable = true;
#else
  const bool wifiCapable = false;
#endif

#if defined(SOC_IEEE802154_SUPPORTED) && SOC_IEEE802154_SUPPORTED
  const bool ieee802154Capable = true;
#else
  const bool ieee802154Capable = false;
#endif

#if defined(ZIGBEE_MODE_ED)
  const char* zigbeeFirmwareMode = "End Device";
#elif defined(ZIGBEE_MODE_ZCZR)
  const char* zigbeeFirmwareMode = "Coordinator / Router";
#else
  const char* zigbeeFirmwareMode = "Inactiv in firmware-ul curent";
#endif

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
  json += "\"chip_model\":\"" + String(ESP.getChipModel()) + "\",";
  json += "\"build_profile\":\"" + String(buildProfileName()) + "\",";
  json += "\"chip_revision\":" + String(chip_info.revision) + ",";
  json += "\"cpu_cores\":" + String(chip_info.cores) + ",";
  json += "\"cpu_freq\":" + String(ESP.getCpuFreqMHz()) + ",";
  json += "\"wifi_capable\":";
  json += wifiCapable ? "true" : "false";
  json += ",\"ieee802154_capable\":";
  json += ieee802154Capable ? "true" : "false";
  json += ",\"zigbee_capable\":";
  json += ieee802154Capable ? "true" : "false";
  json += ",\"zigbee_firmware_mode\":\"" + String(zigbeeFirmwareMode) + "\",";
  json += "\"thread_capable\":";
  json += ieee802154Capable ? "true" : "false";
  json += ",\"matter_capable\":";
  json += (wifiCapable || ieee802154Capable) ? "true" : "false";
  json += ",\"matter_wifi_capable\":";
  json += wifiCapable ? "true" : "false";
  json += ",\"matter_thread_capable\":";
  json += ieee802154Capable ? "true" : "false";
  json += ",";
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

int parsePinArgument(WebServer& server, const char* name, bool& ok)
{
  if (!server.hasArg(name))
  {
    ok = false;
    return PIN_DISABLED;
  }

  String value = server.arg(name);
  value.trim();
  if (value == "-1")
  {
    ok = true;
    return PIN_DISABLED;
  }

  if (value.length() == 0)
  {
    ok = false;
    return PIN_DISABLED;
  }

  for (size_t i = 0; i < value.length(); ++i)
  {
    if (!isDigit(value[i]))
    {
      ok = false;
      return PIN_DISABLED;
    }
  }

  long pin = value.toInt();
  ok = pin >= 0 && pin <= 127;
  return static_cast<int>(pin);
}

void scheduleRestart()
{
  g_restartAt = millis() + 1500;
}

void handleHardwareConfigGet(WebServer& server)
{
  String json = hardwareConfigToJson(g_hardware);
  json.remove(json.length() - 1);
  json += F(",\"pins\":");
  json += gpioInventoryToJson(g_hardware);
  json += '}';
  server.send(200, "application/json", json);
}

void handleHardwareConfigPost(WebServer& server)
{
  bool ok = true;
  HardwareConfig config = g_hardware;
  config.dhtPin = parsePinArgument(server, "dht_pin", ok);
  if (!ok) { server.send(400, "application/json", "{\"ok\":false,\"error\":\"dht_pin invalid\"}"); return; }
  config.pirPin = parsePinArgument(server, "pir_pin", ok);
  if (!ok) { server.send(400, "application/json", "{\"ok\":false,\"error\":\"pir_pin invalid\"}"); return; }
  config.relayPin = parsePinArgument(server, "relay_pin", ok);
  if (!ok) { server.send(400, "application/json", "{\"ok\":false,\"error\":\"relay_pin invalid\"}"); return; }
  config.heartbeatPin = parsePinArgument(server, "heartbeat_pin", ok);
  if (!ok) { server.send(400, "application/json", "{\"ok\":false,\"error\":\"heartbeat_pin invalid\"}"); return; }
  config.sdaPin = parsePinArgument(server, "sda_pin", ok);
  if (!ok) { server.send(400, "application/json", "{\"ok\":false,\"error\":\"sda_pin invalid\"}"); return; }
  config.sclPin = parsePinArgument(server, "scl_pin", ok);
  if (!ok) { server.send(400, "application/json", "{\"ok\":false,\"error\":\"scl_pin invalid\"}"); return; }

  config.relayActiveLow = !server.hasArg("relay_active_low") ||
                          server.arg("relay_active_low") == "1" ||
                          server.arg("relay_active_low") == "true";

  int oledAddress = server.hasArg("oled_address") ? server.arg("oled_address").toInt() : 0x3C;
  config.oledAddress = static_cast<uint8_t>(oledAddress);

  String error;
  if (!validateHardwareConfig(config, error))
  {
    String response = F("{\"ok\":false,\"error\":\"");
    response += error;
    response += F("\"}");
    server.send(400, "application/json", response);
    return;
  }

  if (!hardwareStore.save(config))
  {
    server.send(500, "application/json", "{\"ok\":false,\"error\":\"scrierea in NVS a esuat\"}");
    return;
  }

  server.send(200, "application/json",
              "{\"ok\":true,\"message\":\"Configuratia a fost salvata. Dispozitivul reporneste.\"}");
  scheduleRestart();
}

void handleHardwareConfigReset(WebServer& server)
{
  hardwareStore.clear();
  server.send(200, "application/json",
              "{\"ok\":true,\"message\":\"Profilul implicit a fost restaurat. Dispozitivul reporneste.\"}");
  scheduleRestart();
}

// ============================================================
//  setup() & loop()
// ============================================================
void setup()
{
  // Creeaza mutex-ul inainte de orice altceva (inclusiv task-uri)
  g_mutex = xSemaphoreCreateMutex();

  Serial.begin(115200);
  serialLog.begin();
  delay(300);
  serialLog.println("\n=== ESP32 MQTT Home Assistant DIY Kit ===");

  loadUiConfig();
  g_hardware = hardwareStore.load();
  serialLog.println("[HW] Target: " + String(targetName()) + " / profil: " + buildProfileName());

  applyRgbLed();
#if defined(PIN_RGB_LED)
  serialLog.printf("[RGB] LED onboard disponibil pe GPIO%d\n", PIN_RGB_LED);
#else
  serialLog.println("[RGB] LED onboard indisponibil pentru acest profil");
#endif

  if (g_hardware.pirPin != PIN_DISABLED)
    pinMode(g_hardware.pirPin, INPUT);

  if (g_hardware.relayPin != PIN_DISABLED)
  {
    pinMode(g_hardware.relayPin, OUTPUT);
    const uint8_t activeLevel = g_hardware.relayActiveLow ? LOW : HIGH;
    digitalWrite(g_hardware.relayPin, !activeLevel);
  }

  if (g_hardware.heartbeatPin != PIN_DISABLED)
  {
    pinMode(g_hardware.heartbeatPin, OUTPUT);
    digitalWrite(g_hardware.heartbeatPin, LOW);
  }

  // DHT11
  if (g_hardware.dhtPin != PIN_DISABLED)
  {
    dht = new DHT(g_hardware.dhtPin, DHT_TYPE);
    dht->begin();
    serialLog.println("[DHT11] Initializat pe GPIO" + String(g_hardware.dhtPin));
  }
  else
  {
    serialLog.println("[DHT11] Dezactivat");
  }

  // OLED SSD1306 (Wire este folosit doar din loop)
  if (g_hardware.sdaPin != PIN_DISABLED && g_hardware.sclPin != PIN_DISABLED)
  {
    Wire.setTimeOut(25);
    bool wireReady = Wire.begin(g_hardware.sdaPin, g_hardware.sclPin, OLED_I2C_FREQ);
    if (!wireReady)
    {
      disableOled(F("initializarea magistralei I2C a esuat"));
    }
    else if (!isI2cDevicePresent(g_hardware.oledAddress))
    {
      disableOled(F("adresa configurata nu raspunde; verifica SDA, SCL si 0x3C/0x3D"));
    }
    else
    {
      // Wire a fost deja initializat cu pinii configurati; periphBegin=false.
      g_oledReady = oled.begin(SSD1306_SWITCHCAPVCC, g_hardware.oledAddress,
                               true, false);
    }

    if (!g_oledReady && !g_oledFailureReported)
    {
      disableOled(F("initializarea controlerului SSD1306 a esuat"));
    }
    else if (g_oledReady)
    {
      oled.clearDisplay();
      oled.setTextSize(1);
      oled.setTextColor(SSD1306_WHITE);
      oled.setCursor(16, 20);
      oled.print(F("ESP32 HA Kit"));
      oled.setCursor(10, 35);
      oled.print(F("Initializing..."));
      oled.display();
      serialLog.printf("[OLED] Initializat (0x%02X, SDA=GPIO%d, SCL=GPIO%d)\n",
                       g_hardware.oledAddress, g_hardware.sdaPin, g_hardware.sclPin);
    }
  }
  else
  {
    serialLog.println("[OLED] Dezactivat");
  }

  // WiFiWebManager
  wifiManager.begin();

  // Pastreaza dashboard-ul accesibil chiar daca DHCP schimba adresa IP.
  g_mdnsReady = MDNS.begin("esp32-ha-kit");
  if (g_mdnsReady)
  {
    MDNS.addService("http", "tcp", 80);
    serialLog.println("[mDNS] http://esp32-ha-kit.local");
  }
  else
  {
    serialLog.println("[mDNS] Initializarea a esuat; foloseste adresa IP");
  }

  // Rute web
  wifiManager.on("/",                    HTTP_GET,  handleRoot);
  wifiManager.on("/api/ui_config",       HTTP_GET,  handleUiConfigGet);
  wifiManager.on("/api/ui_config",       HTTP_POST, handleUiConfigPost);
  wifiManager.on("/data",                HTTP_GET,  handleData);
  wifiManager.on("/api/status",          HTTP_GET,  handleApiStatus);
  wifiManager.on("/api/relay",           HTTP_GET,  handleApiRelay);
  wifiManager.on("/api/rgb_led",         HTTP_GET,  handleApiRgbLedGet);
  wifiManager.on("/api/rgb_led",         HTTP_POST, handleApiRgbLedPost);
  wifiManager.on("/api/serial_log",      HTTP_GET,  handleSerialLog);
  wifiManager.on("/api/serial_log/clear",HTTP_POST, handleSerialLogClear);
  wifiManager.on("/api/board_info",      HTTP_GET,  handleBoardInfo);
  wifiManager.on("/api/mqtt_config",     HTTP_GET,  handleMqttConfigGet);
  wifiManager.on("/api/mqtt_config",     HTTP_POST, handleMqttConfigPost);
  wifiManager.on("/api/hardware_config", HTTP_GET,  handleHardwareConfigGet);
  wifiManager.on("/api/hardware_config", HTTP_POST, handleHardwareConfigPost);
  wifiManager.on("/api/hardware_reset",  HTTP_POST, handleHardwareConfigReset);

  // Incarca si aplica configuratia MQTT din NVS
  loadMqttConfig();
  mqttClient.setServer(g_mqttBroker.c_str(), g_mqttPort);
  mqttClient.setCallback(mqttCallback);
  mqttClient.setBufferSize(512);

  // xTaskCreate este portabil pe tintele single-core si dual-core.
  xTaskCreate(taskSensors, "taskSensors", 4096, nullptr, 2, nullptr);
  if (g_hardware.heartbeatPin != PIN_DISABLED)
    xTaskCreate(taskHeartbeat, "taskHeartbeat", 1024, nullptr, 1, nullptr);

  serialLog.println("[WiFi] AP: ESP32_HAKit  /  Parola: 12345678");
  serialLog.println("[Web]  http://" + wifiManager.getIPAddress());
  if (g_mdnsReady)
    serialLog.println("[Web]  http://esp32-ha-kit.local");
  serialLog.println("[MQTT] Broker: " + g_mqttBroker + ":" + String(g_mqttPort));
  serialLog.printf("[Pini] DHT=%d PIR=%d Releu=%d LED=%d SDA=%d SCL=%d\n",
                   g_hardware.dhtPin, g_hardware.pirPin, g_hardware.relayPin,
                   g_hardware.heartbeatPin, g_hardware.sdaPin, g_hardware.sclPin);
  serialLog.println("=========================================\n");
}

void loop()
{
  wifiManager.handleClient();
  handleMqtt();

  if (g_restartAt != 0 && static_cast<long>(millis() - g_restartAt) >= 0)
  {
    ESP.restart();
  }

  // Detectie schimbare stare PIR → publish imediat
  static bool lastMotion = false;
  bool currentMotion;
  xSemaphoreTake(g_mutex, portMAX_DELAY);
  currentMotion = g_motionDetected;
  xSemaphoreGive(g_mutex);

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
    if (g_oledReady) oledDrawStatus();
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
    xSemaphoreTake(g_mutex, portMAX_DELAY);
    temp   = g_temperature;
    hum    = g_humidity;
    motion = g_motionDetected;
    relay  = g_relayActive;
    xSemaphoreGive(g_mutex);

    serialLog.println("--- Status ---");
    if (!isnan(temp))
      serialLog.printf("  Temperatura: %.1f C  Umiditate: %.1f%%\n", temp, hum);
    else
      serialLog.println("  DHT11: dezactivat sau citire invalida");
    serialLog.printf("  Miscare PIR: %s | Releu: %s\n",
                     motion ? "DETECTATA" : "Nu",
                     relay  ? "ON" : "OFF");
    serialLog.printf("  WiFi: %s | MQTT: %s\n",
                     wifiManager.isConnected()
                       ? ("STA " + wifiManager.getIPAddress()).c_str()
                       : "AP mode",
                     g_mqttConnected ? "Conectat" : "Deconectat");
  }
}
