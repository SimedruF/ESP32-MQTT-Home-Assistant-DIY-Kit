/*
  ESP32 MQTT Home Assistant DIY Kit
  ====================================
  Tintele suportate:
  - ESP32-WROOM-32 / ESP32 Dev Module
  - ESP32-C3-DevKitM-1
  - ESP32-C6-DevKitC-1
  - ESP32-C6 Super Mini
  - ESP32-S3-DevKitC-1
  - LILYGO T-ZIGBEE v1.2 (ESP32-C3 + TLSR8258)

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
#include <SPIFFS.h>
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
#include <esp_core_dump.h>
#include <esp_partition.h>
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
String g_topicState       = "esp32kit/state";
String g_topicRelayState  = "esp32kit/relay/state";
String g_topicRelayCommand = "esp32kit/relay/command";
String g_topicDigitalInput = "esp32kit/input/button";
String g_digitalInputPayloadActive = "PRESSED";
String g_digitalInputPayloadInactive = "RELEASED";
bool g_digitalInputRetain = true;

// Topice Home Assistant Auto-Discovery
static const char* DISC_TEMP   = "homeassistant/sensor/esp32kit/temperature/config";
static const char* DISC_HUM    = "homeassistant/sensor/esp32kit/humidity/config";
static const char* DISC_MOTION = "homeassistant/binary_sensor/esp32kit/motion/config";
static const char* DISC_RELAY  = "homeassistant/switch/esp32kit/relay/config";

// NVS pentru setarile MQTT
Preferences mqttPrefs;
Preferences uiPrefs;
Preferences communicationPrefs;
String g_uiLanguage = "ro";
String g_selectedCommunication = "wifi_mqtt";
String g_zigbeeCoordinator = "generic";
String g_zigbeeCoordinatorHost = "";
String g_zigbeePairingMode = "auto";

String jsonEscape(const String& value);

bool isValidMqttTopic(const String& topic)
{
  if (topic.length() == 0 || topic.length() > 96 ||
      topic.indexOf('+') >= 0 || topic.indexOf('#') >= 0)
    return false;

  for (size_t i = 0; i < topic.length(); ++i)
  {
    const uint8_t c = static_cast<uint8_t>(topic[i]);
    if (c == 0 || c < 0x20 || c == 0x7f) return false;
  }
  return true;
}

bool isValidMqttPayload(const String& payload)
{
  if (payload.length() == 0 || payload.length() > 48) return false;
  for (size_t i = 0; i < payload.length(); ++i)
  {
    const uint8_t c = static_cast<uint8_t>(payload[i]);
    if (c == 0 || c < 0x20 || c == 0x7f) return false;
  }
  return true;
}

bool hasNativeIeee802154()
{
#if defined(SOC_IEEE802154_SUPPORTED) && SOC_IEEE802154_SUPPORTED
  return true;
#else
  return false;
#endif
}

bool hasExternalZigbeeRadio()
{
#if defined(ZIGBEE_COPROCESSOR_TLSR8258)
  return true;
#else
  return false;
#endif
}

bool hasZigbeeCapability()
{
  return hasNativeIeee802154() || hasExternalZigbeeRadio();
}

const char* activeCommunicationMode()
{
#if defined(IOT_PROTOCOL_ACTIVE_THREAD)
  return "thread";
#elif defined(ZIGBEE_MODE_ED) || defined(ZIGBEE_MODE_ZCZR)
  return "zigbee";
#else
  return "wifi_mqtt";
#endif
}

bool isValidCommunicationMode(const String& mode)
{
  if (mode == "wifi_mqtt") return true;
  if (mode == "zigbee") return hasZigbeeCapability();
  if (mode == "thread") return hasNativeIeee802154();
  return false;
}

bool isValidZigbeeCoordinator(const String& coordinator)
{
  return coordinator == "generic" ||
         coordinator == "zha" ||
         coordinator == "zigbee2mqtt" ||
         coordinator == "zbbridge_u";
}

bool isValidZigbeePairingMode(const String& pairingMode)
{
  return pairingMode == "auto" || pairingMode == "force_new";
}

bool isValidCoordinatorHost(const String& host)
{
  if (host.length() > 96) return false;
  for (size_t i = 0; i < host.length(); ++i)
  {
    const char c = host[i];
    if (!isAlphaNumeric(c) && c != '.' && c != '-' && c != ':' &&
        c != '[' && c != ']')
      return false;
  }
  return true;
}

void loadCommunicationConfig()
{
  if (!communicationPrefs.begin("iot_radio", false))
  {
    serialLog.println("[IoT] NVS indisponibil; se foloseste MQTT prin WiFi");
    return;
  }

  const String storedMode = communicationPrefs.getString("protocol", "wifi_mqtt");
  const String storedCoordinator =
    communicationPrefs.getString("zb_coord", "generic");
  const String storedCoordinatorHost =
    communicationPrefs.getString("zb_host", "");
  const String storedPairingMode =
    communicationPrefs.getString("zb_pair", "auto");
  communicationPrefs.end();
  g_selectedCommunication = isValidCommunicationMode(storedMode)
    ? storedMode
    : "wifi_mqtt";
  g_zigbeeCoordinator = isValidZigbeeCoordinator(storedCoordinator)
    ? storedCoordinator
    : "generic";
  g_zigbeeCoordinatorHost = isValidCoordinatorHost(storedCoordinatorHost)
    ? storedCoordinatorHost
    : "";
  g_zigbeePairingMode = isValidZigbeePairingMode(storedPairingMode)
    ? storedPairingMode
    : "auto";

  serialLog.printf("[IoT] Mod selectat: %s | activ: %s | coordinator: %s%s%s | pairing: %s\n",
                   g_selectedCommunication.c_str(), activeCommunicationMode(),
                   g_zigbeeCoordinator.c_str(),
                   g_zigbeeCoordinatorHost.length() ? " @ " : "",
                   g_zigbeeCoordinatorHost.c_str(),
                   g_zigbeePairingMode.c_str());
}

bool saveCommunicationConfig(const String& mode, const String& coordinator,
                             const String& coordinatorHost,
                             const String& pairingMode)
{
  if (!isValidCommunicationMode(mode) ||
      !isValidZigbeeCoordinator(coordinator) ||
      !isValidCoordinatorHost(coordinatorHost) ||
      !isValidZigbeePairingMode(pairingMode))
    return false;

  if (!communicationPrefs.begin("iot_radio", false)) return false;
  const bool protocolSaved = communicationPrefs.putString("protocol", mode) > 0;
  const bool coordinatorSaved =
    communicationPrefs.putString("zb_coord", coordinator) > 0;
  const bool hostSaved = coordinatorHost.length() == 0
    ? communicationPrefs.remove("zb_host") || !communicationPrefs.isKey("zb_host")
    : communicationPrefs.putString("zb_host", coordinatorHost) > 0;
  const bool pairingSaved =
    communicationPrefs.putString("zb_pair", pairingMode) > 0;
  communicationPrefs.end();
  if (!protocolSaved || !coordinatorSaved || !hostSaved || !pairingSaved)
    return false;

  g_selectedCommunication = mode;
  g_zigbeeCoordinator = coordinator;
  g_zigbeeCoordinatorHost = coordinatorHost;
  g_zigbeePairingMode = pairingMode;
  return true;
}

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
  const String storedTopicState =
    mqttPrefs.getString("topic_state", g_topicState);
  const String storedRelayState =
    mqttPrefs.getString("relay_state", g_topicRelayState);
  const String storedRelayCommand =
    mqttPrefs.getString("relay_cmd", g_topicRelayCommand);
  const String storedDigitalInputTopic =
    mqttPrefs.getString("din_topic", g_topicDigitalInput);
  const String storedDigitalInputActive =
    mqttPrefs.getString("din_on", g_digitalInputPayloadActive);
  const String storedDigitalInputInactive =
    mqttPrefs.getString("din_off", g_digitalInputPayloadInactive);
  const bool storedDigitalInputRetain =
    mqttPrefs.getBool("din_retain", g_digitalInputRetain);
  mqttPrefs.end();

  if (isValidMqttTopic(storedTopicState) &&
      isValidMqttTopic(storedRelayState) &&
      isValidMqttTopic(storedRelayCommand) &&
      storedTopicState != storedRelayState &&
      storedTopicState != storedRelayCommand &&
      storedRelayState != storedRelayCommand &&
      isValidMqttTopic(storedDigitalInputTopic) &&
      storedDigitalInputTopic != storedTopicState &&
      storedDigitalInputTopic != storedRelayState &&
      storedDigitalInputTopic != storedRelayCommand)
  {
    g_topicState = storedTopicState;
    g_topicRelayState = storedRelayState;
    g_topicRelayCommand = storedRelayCommand;
    g_topicDigitalInput = storedDigitalInputTopic;
  }
  if (isValidMqttPayload(storedDigitalInputActive) &&
      isValidMqttPayload(storedDigitalInputInactive) &&
      storedDigitalInputActive != storedDigitalInputInactive)
  {
    g_digitalInputPayloadActive = storedDigitalInputActive;
    g_digitalInputPayloadInactive = storedDigitalInputInactive;
  }
  g_digitalInputRetain = storedDigitalInputRetain;

  serialLog.printf("[MQTT] Config NVS: %s:%d user='%s' client='%s'\n",
                   g_mqttBroker.c_str(), g_mqttPort,
                   g_mqttUser.c_str(), g_mqttClientId.c_str());
  serialLog.printf("[MQTT] Topice: state='%s' relay_state='%s' relay_command='%s'\n",
                   g_topicState.c_str(), g_topicRelayState.c_str(),
                   g_topicRelayCommand.c_str());
}

bool saveMqttConfig(const String& broker, int port,
                    const String& user, const String& pass,
                    const String& clientId, const String& topicState,
                    const String& topicRelayState,
                    const String& topicRelayCommand,
                    const String& topicDigitalInput,
                    const String& digitalInputPayloadActive,
                    const String& digitalInputPayloadInactive,
                    bool digitalInputRetain)
{
  if (!mqttPrefs.begin("mqtt", false))
  {
    serialLog.println("[MQTT] Eroare la deschiderea NVS pentru scriere");
    return false;
  }
  mqttPrefs.putString("broker",    broker);
  mqttPrefs.putInt(   "port",      port);
  mqttPrefs.putString("user",      user);
  mqttPrefs.putString("pass",      pass);
  mqttPrefs.putString("client_id", clientId);
  mqttPrefs.putString("topic_state", topicState);
  mqttPrefs.putString("relay_state", topicRelayState);
  mqttPrefs.putString("relay_cmd", topicRelayCommand);
  mqttPrefs.putString("din_topic", topicDigitalInput);
  mqttPrefs.putString("din_on", digitalInputPayloadActive);
  mqttPrefs.putString("din_off", digitalInputPayloadInactive);
  mqttPrefs.putBool("din_retain", digitalInputRetain);
  mqttPrefs.end();
  g_mqttBroker   = broker;
  g_mqttPort     = port;
  g_mqttUser     = user;
  g_mqttPass     = pass;
  g_mqttClientId = clientId;
  g_topicState = topicState;
  g_topicRelayState = topicRelayState;
  g_topicRelayCommand = topicRelayCommand;
  g_topicDigitalInput = topicDigitalInput;
  g_digitalInputPayloadActive = digitalInputPayloadActive;
  g_digitalInputPayloadInactive = digitalInputPayloadInactive;
  g_digitalInputRetain = digitalInputRetain;
  serialLog.println("[MQTT] Configuratie salvata in NVS");
  return true;
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
static bool g_spiffsReady = false;
static unsigned long g_restartAt = 0;
static File g_spiffsUploadFile;
static String g_spiffsUploadPath;
static String g_spiffsUploadError;
static size_t g_spiffsUploadBytes = 0;
static bool g_spiffsUploadStarted = false;

static const char* RESET_LOG_PATH = "/reset-log.txt";
static const char* LAST_COREDUMP_PATH = "/last-coredump.bin";
static const size_t RESET_LOG_MAX_SIZE = 64 * 1024;

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
static bool g_digitalInputActive = false;
static bool g_digitalInputReady = false;

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
    mqttClient.publish(g_topicRelayState.c_str(), active ? "ON" : "OFF",
                       /*retain=*/true);
}

const char* resetReasonName(esp_reset_reason_t reason)
{
  switch (reason)
  {
    case ESP_RST_POWERON:    return "Power On";
    case ESP_RST_EXT:        return "External Pin";
    case ESP_RST_SW:         return "Software Reset";
    case ESP_RST_PANIC:      return "Exception/Panic";
    case ESP_RST_INT_WDT:    return "Interrupt Watchdog";
    case ESP_RST_TASK_WDT:   return "Task Watchdog";
    case ESP_RST_WDT:        return "Other Watchdog";
    case ESP_RST_DEEPSLEEP:  return "Deep Sleep";
    case ESP_RST_BROWNOUT:   return "Brownout";
    case ESP_RST_SDIO:       return "SDIO";
    case ESP_RST_USB:        return "USB";
    case ESP_RST_JTAG:       return "JTAG";
    case ESP_RST_EFUSE:      return "eFuse Error";
    case ESP_RST_PWR_GLITCH: return "Power Glitch";
    case ESP_RST_CPU_LOCKUP: return "CPU Lockup";
    default:                 return "Unknown";
  }
}

bool copyLastCoreDump(size_t imageAddress, size_t imageSize)
{
  const esp_partition_t* partition =
    esp_partition_find_first(ESP_PARTITION_TYPE_DATA,
                             ESP_PARTITION_SUBTYPE_DATA_COREDUMP, nullptr);
  if (!partition || imageAddress < partition->address ||
      imageAddress - partition->address + imageSize > partition->size)
    return false;

  File output = SPIFFS.open(LAST_COREDUMP_PATH, FILE_WRITE);
  if (!output) return false;

  uint8_t buffer[512];
  size_t copied = 0;
  const size_t partitionOffset = imageAddress - partition->address;
  while (copied < imageSize)
  {
    const size_t chunk = min(sizeof(buffer), imageSize - copied);
    if (esp_partition_read(partition, partitionOffset + copied, buffer, chunk) != ESP_OK ||
        output.write(buffer, chunk) != chunk)
    {
      output.close();
      SPIFFS.remove(LAST_COREDUMP_PATH);
      return false;
    }
    copied += chunk;
  }

  output.close();
  return true;
}

void writeCoreDumpSummary(File& logFile, bool& coreDumpCopied)
{
  size_t imageAddress = 0;
  size_t imageSize = 0;
  const esp_err_t imageResult = esp_core_dump_image_get(&imageAddress, &imageSize);
  if (imageResult != ESP_OK)
  {
    logFile.println(F("Coredump: indisponibil"));
    return;
  }

  logFile.printf("Coredump: prezent, %u bytes, flash=0x%08lx\n",
                 static_cast<unsigned>(imageSize),
                 static_cast<unsigned long>(imageAddress));
  logFile.printf("Coredump integritate: %s\n",
                 esp_err_to_name(esp_core_dump_image_check()));

#if CONFIG_ESP_COREDUMP_ENABLE_TO_FLASH && CONFIG_ESP_COREDUMP_DATA_FORMAT_ELF
  char panicReason[256] = {};
  if (esp_core_dump_get_panic_reason(panicReason, sizeof(panicReason)) == ESP_OK)
    logFile.printf("Panic reason: %s\n", panicReason);

  esp_core_dump_summary_t* summary =
    static_cast<esp_core_dump_summary_t*>(malloc(sizeof(esp_core_dump_summary_t)));
  if (summary && esp_core_dump_get_summary(summary) == ESP_OK)
  {
    logFile.printf("Task: %s | TCB=0x%08lx | PC=0x%08lx\n",
                   summary->exc_task,
                   static_cast<unsigned long>(summary->exc_tcb),
                   static_cast<unsigned long>(summary->exc_pc));
    logFile.printf("Coredump version: 0x%08lx | App ELF SHA256: %s\n",
                   static_cast<unsigned long>(summary->core_dump_version),
                   reinterpret_cast<const char*>(summary->app_elf_sha256));

#if CONFIG_IDF_TARGET_ARCH_XTENSA
    logFile.printf("EXCCAUSE=0x%08lx EXCVADDR=0x%08lx\n",
                   static_cast<unsigned long>(summary->ex_info.exc_cause),
                   static_cast<unsigned long>(summary->ex_info.exc_vaddr));
    for (size_t i = 0; i < 16; ++i)
    {
      logFile.printf("A%u=0x%08lx%s", static_cast<unsigned>(i),
                     static_cast<unsigned long>(summary->ex_info.exc_a[i]),
                     (i % 4 == 3) ? "\n" : " ");
    }
    logFile.print(F("Backtrace PC:"));
    for (size_t i = 0; i < summary->exc_bt_info.depth &&
                       i < sizeof(summary->exc_bt_info.bt) / sizeof(summary->exc_bt_info.bt[0]);
         ++i)
      logFile.printf(" 0x%08lx",
                     static_cast<unsigned long>(summary->exc_bt_info.bt[i]));
    logFile.printf("\nBacktrace corupt: %s\n",
                   summary->exc_bt_info.corrupted ? "da" : "nu");
#elif CONFIG_IDF_TARGET_ARCH_RISCV
    logFile.printf("MCAUSE=0x%08lx MTVAL=0x%08lx MSTATUS=0x%08lx MTVEC=0x%08lx\n",
                   static_cast<unsigned long>(summary->ex_info.mcause),
                   static_cast<unsigned long>(summary->ex_info.mtval),
                   static_cast<unsigned long>(summary->ex_info.mstatus),
                   static_cast<unsigned long>(summary->ex_info.mtvec));
    logFile.printf("RA=0x%08lx SP=0x%08lx\n",
                   static_cast<unsigned long>(summary->ex_info.ra),
                   static_cast<unsigned long>(summary->ex_info.sp));
    for (size_t i = 0; i < 8; ++i)
    {
      logFile.printf("A%u=0x%08lx%s", static_cast<unsigned>(i),
                     static_cast<unsigned long>(summary->ex_info.exc_a[i]),
                     (i % 4 == 3) ? "\n" : " ");
    }
    logFile.printf("Stack dump disponibil: %u bytes\n",
                   static_cast<unsigned>(summary->exc_bt_info.dump_size));
#endif
  }
  else
  {
    logFile.println(F("Rezumat coredump: nu a putut fi citit"));
  }
  free(summary);
#endif

  coreDumpCopied = copyLastCoreDump(imageAddress, imageSize);
  logFile.printf("Coredump brut: %s%s\n",
                 coreDumpCopied ? "salvat in " : "copiere esuata pentru ",
                 LAST_COREDUMP_PATH);
}

void persistResetLog()
{
  if (!g_spiffsReady) return;

  Preferences resetPrefs;
  uint32_t bootSequence = 1;
  if (resetPrefs.begin("reset_log", false))
  {
    bootSequence = resetPrefs.getUInt("sequence", 0) + 1;
    resetPrefs.putUInt("sequence", bootSequence);
    resetPrefs.end();
  }

  File existing = SPIFFS.open(RESET_LOG_PATH, FILE_READ);
  const bool rotateLog = existing && existing.size() >= RESET_LOG_MAX_SIZE;
  if (existing) existing.close();
  if (rotateLog) SPIFFS.remove(RESET_LOG_PATH);

  File logFile = SPIFFS.open(RESET_LOG_PATH, FILE_APPEND);
  if (!logFile)
  {
    serialLog.println("[ResetLog] Nu pot deschide /reset-log.txt");
    return;
  }

  const esp_reset_reason_t reason = esp_reset_reason();
  logFile.println(F("============================================================"));
  logFile.printf("Boot #%lu | reset=%s (%d)\n",
                 static_cast<unsigned long>(bootSequence),
                 resetReasonName(reason), static_cast<int>(reason));
  logFile.printf("Chip=%s | profil=%s | SDK=%s\n",
                 ESP.getChipModel(), buildProfileName(), ESP.getSdkVersion());
  logFile.printf("Firmware MD5=%s | sketch=%u bytes | flash=%u bytes\n",
                 ESP.getSketchMD5().c_str(),
                 static_cast<unsigned>(ESP.getSketchSize()),
                 static_cast<unsigned>(ESP.getFlashChipSize()));

  bool coreDumpCopied = false;
  writeCoreDumpSummary(logFile, coreDumpCopied);
  logFile.println();
  logFile.close();

  if (coreDumpCopied)
  {
    const esp_err_t eraseResult = esp_core_dump_image_erase();
    if (eraseResult != ESP_OK)
      serialLog.printf("[ResetLog] Coredump copiat, dar stergerea a esuat: %s\n",
                       esp_err_to_name(eraseResult));
  }

  serialLog.printf("[ResetLog] Boot #%lu salvat in %s (%s)\n",
                   static_cast<unsigned long>(bootSequence), RESET_LOG_PATH,
                   resetReasonName(reason));
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

  if (String(topic) == g_topicRelayCommand)
  {
    if      (msg == "ON")  setRelay(true);
    else if (msg == "OFF") setRelay(false);
  }
}

void publishDigitalInputState()
{
  if (!mqttClient.connected() || !g_digitalInputReady) return;
  mqttClient.publish(g_topicDigitalInput.c_str(),
                     g_digitalInputActive
                       ? g_digitalInputPayloadActive.c_str()
                       : g_digitalInputPayloadInactive.c_str(),
                     g_digitalInputRetain);
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
                 "\"state_topic\":\"" + jsonEscape(g_topicState) + "\","
                 "\"value_template\":\"{{ value_json.temperature | round(1) }}\","
                 "\"unique_id\":\"esp32kit_temperature\","
                 + dev + "}";
    mqttClient.publish(DISC_TEMP, cfg.c_str(), true);

    cfg = "{\"name\":\"Umiditate\","
          "\"device_class\":\"humidity\","
          "\"unit_of_measurement\":\"%\","
          "\"state_topic\":\"" + jsonEscape(g_topicState) + "\","
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
                 "\"state_topic\":\"" + jsonEscape(g_topicState) + "\","
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
                 "\"state_topic\":\"" + jsonEscape(g_topicRelayState) + "\","
                 "\"command_topic\":\"" + jsonEscape(g_topicRelayCommand) + "\","
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
    mqttClient.subscribe(g_topicRelayCommand.c_str());
    g_mqttConnected      = true;
    g_discoveryPublished = false;   // re-publica discovery dupa reconectare
    publishDigitalInputState();
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
  json += ",\"digital_input\":";
  json += g_digitalInputReady
    ? (g_digitalInputActive ? "true" : "false")
    : "null";
  json += ",\"uptime\":";
  json += String(millis() / 1000);
  json += "}";

  mqttClient.publish(g_topicState.c_str(), json.c_str());
  mqttClient.publish(g_topicRelayState.c_str(), relay ? "ON" : "OFF", true);
  publishDigitalInputState();
}

// ============================================================
//  Web Server Handlers
// ============================================================
void handleRoot(WebServer& server)
{
  server.send_P(200, "text/html", htmlPage);
}

String jsonEscape(const String& value)
{
  String escaped;
  escaped.reserve(value.length() + 8);
  for (size_t i = 0; i < value.length(); ++i)
  {
    const char c = value[i];
    switch (c)
    {
      case '"':  escaped += F("\\\""); break;
      case '\\': escaped += F("\\\\"); break;
      case '\b': escaped += F("\\b"); break;
      case '\f': escaped += F("\\f"); break;
      case '\n': escaped += F("\\n"); break;
      case '\r': escaped += F("\\r"); break;
      case '\t': escaped += F("\\t"); break;
      default:
        if (static_cast<uint8_t>(c) >= 0x20) escaped += c;
        break;
    }
  }
  return escaped;
}

bool isValidSpiffsPath(const String& path)
{
  if (path.length() < 2 || path.length() > 128 || path[0] != '/') return false;
  if (path.indexOf(F("..")) >= 0 || path.indexOf('\\') >= 0) return false;

  for (size_t i = 0; i < path.length(); ++i)
  {
    if (static_cast<uint8_t>(path[i]) < 0x20) return false;
  }
  return true;
}

String safeSpiffsUploadPath(const String& filename)
{
  int separator = filename.lastIndexOf('/');
  const int backslash = filename.lastIndexOf('\\');
  if (backslash > separator) separator = backslash;

  String basename = filename.substring(separator + 1);
  basename.trim();
  if (basename.length() == 0 || basename == "." || basename == "..") return "";

  String safeName;
  safeName.reserve(min(static_cast<size_t>(64), basename.length()) + 1);
  for (size_t i = 0; i < basename.length() && safeName.length() < 64; ++i)
  {
    const char c = basename[i];
    safeName += isAlphaNumeric(c) || c == '.' || c == '-' || c == '_' ? c : '_';
  }
  if (safeName.length() == 0 || safeName == "." || safeName == "..") return "";
  return "/" + safeName;
}

String safeDownloadFilename(const String& path)
{
  String filename = path.substring(path.lastIndexOf('/') + 1);
  String safeName;
  safeName.reserve(filename.length());
  for (size_t i = 0; i < filename.length(); ++i)
  {
    const char c = filename[i];
    safeName += isAlphaNumeric(c) || c == '.' || c == '-' || c == '_' ? c : '_';
  }
  return safeName.length() ? safeName : "download.bin";
}

void handleSpiffsList(WebServer& server)
{
  server.sendHeader("Cache-Control", "no-store");
  if (!g_spiffsReady)
  {
    server.send(503, "application/json",
                "{\"mounted\":false,\"error\":\"SPIFFS nu este disponibil\"}");
    return;
  }

  File root = SPIFFS.open("/");
  if (!root || !root.isDirectory())
  {
    server.send(500, "application/json",
                "{\"mounted\":true,\"error\":\"directorul SPIFFS nu poate fi citit\"}");
    return;
  }

  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200, "application/json", "");
  server.sendContent(String(F("{\"mounted\":true,\"total\":")) +
                     SPIFFS.totalBytes() + F(",\"used\":") +
                     SPIFFS.usedBytes() + F(",\"files\":["));

  bool first = true;
  File file = root.openNextFile();
  while (file)
  {
    if (!file.isDirectory())
    {
      String fileName = file.name();
      if (!fileName.startsWith("/")) fileName = "/" + fileName;
      String entry;
      entry.reserve(fileName.length() + 40);
      if (!first) entry += ',';
      entry += F("{\"name\":\"");
      entry += jsonEscape(fileName);
      entry += F("\",\"size\":");
      entry += file.size();
      entry += '}';
      server.sendContent(entry);
      first = false;
    }
    file = root.openNextFile();
  }
  server.sendContent("]}");
  server.sendContent("");
}

void handleSpiffsDownload(WebServer& server)
{
  if (!g_spiffsReady)
  {
    server.send(503, "application/json", "{\"error\":\"SPIFFS nu este disponibil\"}");
    return;
  }

  const String path = server.arg("path");
  if (!isValidSpiffsPath(path))
  {
    server.send(400, "application/json", "{\"error\":\"cale SPIFFS invalida\"}");
    return;
  }

  File file = SPIFFS.open(path, FILE_READ);
  if (!file || file.isDirectory())
  {
    server.send(404, "application/json", "{\"error\":\"fisierul nu exista\"}");
    return;
  }

  server.sendHeader("Cache-Control", "no-store");
  server.sendHeader("Content-Disposition",
                    "attachment; filename=\"" + safeDownloadFilename(path) + "\"");
  server.streamFile(file, "application/octet-stream");
  file.close();
}

void handleSpiffsUploadData()
{
  WebServer& server = wifiManager.getServer();
  HTTPUpload& upload = server.upload();

  if (upload.status == UPLOAD_FILE_START)
  {
    if (g_spiffsUploadFile) g_spiffsUploadFile.close();
    g_spiffsUploadPath = "";
    g_spiffsUploadError = "";
    g_spiffsUploadBytes = 0;
    g_spiffsUploadStarted = true;

    if (!g_spiffsReady)
    {
      g_spiffsUploadError = F("SPIFFS nu este disponibil");
      return;
    }

    g_spiffsUploadPath = safeSpiffsUploadPath(upload.filename);
    if (g_spiffsUploadPath.length() == 0)
    {
      g_spiffsUploadError = F("nume de fisier invalid");
      return;
    }

    g_spiffsUploadFile = SPIFFS.open(g_spiffsUploadPath, FILE_WRITE);
    if (!g_spiffsUploadFile)
    {
      g_spiffsUploadError = F("fisierul nu poate fi creat");
      return;
    }
    serialLog.println("[SPIFFS] Upload inceput: " + g_spiffsUploadPath);
  }
  else if (upload.status == UPLOAD_FILE_WRITE)
  {
    if (g_spiffsUploadError.length() == 0 && g_spiffsUploadFile)
    {
      const size_t written = g_spiffsUploadFile.write(upload.buf, upload.currentSize);
      g_spiffsUploadBytes += written;
      if (written != upload.currentSize)
      {
        g_spiffsUploadError = F("spatiu insuficient sau eroare la scriere");
        g_spiffsUploadFile.close();
        SPIFFS.remove(g_spiffsUploadPath);
      }
    }
  }
  else if (upload.status == UPLOAD_FILE_END)
  {
    if (g_spiffsUploadFile) g_spiffsUploadFile.close();
    if (g_spiffsUploadError.length() == 0)
      serialLog.printf("[SPIFFS] Upload finalizat: %s (%u bytes)\n",
                       g_spiffsUploadPath.c_str(),
                       static_cast<unsigned>(g_spiffsUploadBytes));
    else if (g_spiffsUploadPath.length())
      SPIFFS.remove(g_spiffsUploadPath);
  }
  else if (upload.status == UPLOAD_FILE_ABORTED)
  {
    if (g_spiffsUploadFile) g_spiffsUploadFile.close();
    if (g_spiffsUploadPath.length()) SPIFFS.remove(g_spiffsUploadPath);
    g_spiffsUploadError = F("upload intrerupt");
  }
}

void handleSpiffsUploadComplete()
{
  WebServer& server = wifiManager.getServer();
  server.sendHeader("Cache-Control", "no-store");

  if (!g_spiffsUploadStarted)
  {
    server.send(400, "application/json",
                "{\"ok\":false,\"error\":\"nu a fost primit niciun fisier\"}");
    return;
  }
  g_spiffsUploadStarted = false;

  if (g_spiffsUploadError.length())
  {
    String response = F("{\"ok\":false,\"error\":\"");
    response += jsonEscape(g_spiffsUploadError);
    response += F("\"}");
    server.send(g_spiffsReady ? 500 : 503, "application/json", response);
    return;
  }

  if (g_spiffsUploadPath.length() == 0)
  {
    server.send(400, "application/json",
                "{\"ok\":false,\"error\":\"nu a fost primit niciun fisier\"}");
    return;
  }

  String response = F("{\"ok\":true,\"name\":\"");
  response += jsonEscape(g_spiffsUploadPath);
  response += F("\",\"size\":");
  response += g_spiffsUploadBytes;
  response += '}';
  server.send(201, "application/json", response);
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

void handleCommunicationConfigGet(WebServer& server)
{
  const bool zigbeeCapable = hasZigbeeCapability();
  const bool threadCapable = hasNativeIeee802154();
  const char* activeMode = activeCommunicationMode();

  String json;
  json.reserve(240);
  json = F("{\"visible\":");
  json += zigbeeCapable ? F("true") : F("false");
  json += F(",\"zigbee_capable\":");
  json += zigbeeCapable ? F("true") : F("false");
  json += F(",\"thread_capable\":");
  json += threadCapable ? F("true") : F("false");
  json += F(",\"selected_protocol\":\"");
  json += g_selectedCommunication;
  json += F("\",\"active_protocol\":\"");
  json += activeMode;
  json += F("\",\"zigbee_coordinator\":\"");
  json += g_zigbeeCoordinator;
  json += F("\",\"zigbee_coordinator_host\":\"");
  json += g_zigbeeCoordinatorHost;
  json += F("\",\"zigbee_pairing_mode\":\"");
  json += g_zigbeePairingMode;
  json += F("\",\"zbbridge_custom_device_supported\":false");
  json += F(",\"firmware_change_required\":");
  json += g_selectedCommunication == activeMode ? F("false") : F("true");
  json += '}';

  server.sendHeader("Cache-Control", "no-store");
  server.send(200, "application/json", json);
}

void handleCommunicationConfigPost(WebServer& server)
{
  if (!hasZigbeeCapability())
  {
    server.send(404, "application/json",
                "{\"ok\":false,\"error\":\"configuratia radio nu este disponibila pentru aceasta placa\"}");
    return;
  }

  const String mode = server.arg("protocol");
  const String coordinator = server.hasArg("zigbee_coordinator")
    ? server.arg("zigbee_coordinator")
    : g_zigbeeCoordinator;
  String coordinatorHost = server.hasArg("zigbee_coordinator_host")
    ? server.arg("zigbee_coordinator_host")
    : g_zigbeeCoordinatorHost;
  const String pairingMode = server.hasArg("zigbee_pairing_mode")
    ? server.arg("zigbee_pairing_mode")
    : g_zigbeePairingMode;
  coordinatorHost.trim();

  if (!isValidCommunicationMode(mode) ||
      !isValidZigbeeCoordinator(coordinator) ||
      !isValidCoordinatorHost(coordinatorHost) ||
      !isValidZigbeePairingMode(pairingMode))
  {
    server.send(400, "application/json",
                "{\"ok\":false,\"error\":\"configuratie Zigbee invalida sau indisponibila\"}");
    return;
  }

  if (!saveCommunicationConfig(mode, coordinator, coordinatorHost, pairingMode))
  {
    server.send(500, "application/json",
                "{\"ok\":false,\"error\":\"salvarea configuratiei in NVS a esuat\"}");
    return;
  }

  const bool firmwareChangeRequired = mode != activeCommunicationMode();
  String json = F("{\"ok\":true,\"selected_protocol\":\"");
  json += g_selectedCommunication;
  json += F("\",\"active_protocol\":\"");
  json += activeCommunicationMode();
  json += F("\",\"zigbee_coordinator\":\"");
  json += g_zigbeeCoordinator;
  json += F("\",\"zigbee_coordinator_host\":\"");
  json += g_zigbeeCoordinatorHost;
  json += F("\",\"zigbee_pairing_mode\":\"");
  json += g_zigbeePairingMode;
  json += F("\",\"firmware_change_required\":");
  json += firmwareChangeRequired ? F("true") : F("false");
  json += '}';
  serialLog.printf("[IoT] Configuratie salvata: %s, coordinator %s%s%s, pairing %s%s\n",
                   g_selectedCommunication.c_str(),
                   g_zigbeeCoordinator.c_str(),
                   g_zigbeeCoordinatorHost.length() ? " @ " : "",
                   g_zigbeeCoordinatorHost.c_str(),
                   g_zigbeePairingMode.c_str(),
                   firmwareChangeRequired ? " (necesita firmware compatibil)" : "");
  server.send(200, "application/json", json);
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
  json += ",\"digital_input_available\":";
  json += g_hardware.digitalInputPin != PIN_DISABLED ? "true" : "false";
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
  json += ",\"digital_input\":";
  json += g_digitalInputReady
    ? (g_digitalInputActive ? "true" : "false")
    : "null";
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
  String json;
  json.reserve(420);
  json = "{\"broker\":\"" + jsonEscape(g_mqttBroker) + "\",";
  json += "\"port\":"      + String(g_mqttPort) + ",";
  json += "\"user\":\""   + jsonEscape(g_mqttUser) + "\",";
  json += "\"client_id\":\"" + jsonEscape(g_mqttClientId) + "\",";
  json += "\"topic_state\":\"" + jsonEscape(g_topicState) + "\",";
  json += "\"topic_relay_state\":\"" + jsonEscape(g_topicRelayState) + "\",";
  json += "\"topic_relay_command\":\"" + jsonEscape(g_topicRelayCommand) + "\",";
  json += "\"topic_digital_input\":\"" + jsonEscape(g_topicDigitalInput) + "\",";
  json += "\"digital_input_payload_active\":\"" +
          jsonEscape(g_digitalInputPayloadActive) + "\",";
  json += "\"digital_input_payload_inactive\":\"" +
          jsonEscape(g_digitalInputPayloadInactive) + "\",";
  json += "\"digital_input_retain\":" +
          String(g_digitalInputRetain ? "true" : "false") + ",";
  json += "\"connected\":" + String(g_mqttConnected ? "true" : "false");
  json += "}";
  server.sendHeader("Cache-Control", "no-store");
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
  String pass = server.hasArg("pass") && server.arg("pass").length()
    ? server.arg("pass") : g_mqttPass;
  String clientId = server.hasArg("client_id") ? server.arg("client_id") : "esp32-ha-kit";
  String topicState = server.hasArg("topic_state")
    ? server.arg("topic_state") : g_topicState;
  String topicRelayState = server.hasArg("topic_relay_state")
    ? server.arg("topic_relay_state") : g_topicRelayState;
  String topicRelayCommand = server.hasArg("topic_relay_command")
    ? server.arg("topic_relay_command") : g_topicRelayCommand;
  String topicDigitalInput = server.hasArg("topic_digital_input")
    ? server.arg("topic_digital_input") : g_topicDigitalInput;
  String digitalInputPayloadActive =
    server.hasArg("digital_input_payload_active")
      ? server.arg("digital_input_payload_active")
      : g_digitalInputPayloadActive;
  String digitalInputPayloadInactive =
    server.hasArg("digital_input_payload_inactive")
      ? server.arg("digital_input_payload_inactive")
      : g_digitalInputPayloadInactive;
  const bool digitalInputRetain =
    server.hasArg("digital_input_retain") &&
    (server.arg("digital_input_retain") == "1" ||
     server.arg("digital_input_retain") == "true");

  broker.trim();
  clientId.trim();
  topicState.trim();
  topicRelayState.trim();
  topicRelayCommand.trim();
  topicDigitalInput.trim();
  digitalInputPayloadActive.trim();
  digitalInputPayloadInactive.trim();

  // Validare port
  if (port <= 0 || port > 65535) port = 1883;

  if (!isValidMqttTopic(topicState) ||
      !isValidMqttTopic(topicRelayState) ||
      !isValidMqttTopic(topicRelayCommand) ||
      !isValidMqttTopic(topicDigitalInput))
  {
    server.send(400, "application/json",
                "{\"ok\":false,\"error\":\"topic MQTT invalid: maxim 96 caractere, fara +, # sau caractere de control\"}");
    return;
  }
  if (topicState == topicRelayState ||
      topicState == topicRelayCommand ||
      topicState == topicDigitalInput ||
      topicRelayState == topicRelayCommand ||
      topicRelayState == topicDigitalInput ||
      topicRelayCommand == topicDigitalInput)
  {
    server.send(400, "application/json",
                "{\"ok\":false,\"error\":\"topicele MQTT trebuie sa fie distincte\"}");
    return;
  }
  if (!isValidMqttPayload(digitalInputPayloadActive) ||
      !isValidMqttPayload(digitalInputPayloadInactive) ||
      digitalInputPayloadActive == digitalInputPayloadInactive)
  {
    server.send(400, "application/json",
                "{\"ok\":false,\"error\":\"payload buton invalid\"}");
    return;
  }

  if (!saveMqttConfig(broker, port, user, pass, clientId, topicState,
                      topicRelayState, topicRelayCommand, topicDigitalInput,
                      digitalInputPayloadActive, digitalInputPayloadInactive,
                      digitalInputRetain))
  {
    server.send(500, "application/json",
                "{\"ok\":false,\"error\":\"salvarea configuratiei in NVS a esuat\"}");
    return;
  }

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

  const bool ieee802154Capable = hasNativeIeee802154();
  const bool externalZigbeeCapable = hasExternalZigbeeRadio();

#if defined(ZIGBEE_COPROCESSOR_TLSR8258)
  const char* zigbeeFirmwareMode = "TLSR8258 extern; interfata HCI inactiva";
#elif defined(ZIGBEE_MODE_ED)
  const char* zigbeeFirmwareMode = "End Device";
#elif defined(ZIGBEE_MODE_ZCZR)
  const char* zigbeeFirmwareMode = "Coordinator / Router";
#else
  const char* zigbeeFirmwareMode = "Inactiv in firmware-ul curent";
#endif

  // Use WiFi MAC (already available, no extra header needed)
  String macStr = WiFi.macAddress();

  const char* resetReason = resetReasonName(esp_reset_reason());

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
  json += ",\"zigbee_external\":";
  json += externalZigbeeCapable ? "true" : "false";
  json += ",\"zigbee_capable\":";
  json += (ieee802154Capable || externalZigbeeCapable) ? "true" : "false";
  json += ",\"zigbee_firmware_mode\":\"" + String(zigbeeFirmwareMode) + "\",";
  json += "\"communication_selected\":\"" + g_selectedCommunication + "\",";
  json += "\"communication_active\":\"" + String(activeCommunicationMode()) + "\",";
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
  json += "\"reset_reason\":\"" + String(resetReason) + "\",";
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
  config.digitalInputPin = parsePinArgument(server, "digital_input_pin", ok);
  if (!ok) { server.send(400, "application/json", "{\"ok\":false,\"error\":\"input pin invalid\"}"); return; }
  config.heartbeatPin = parsePinArgument(server, "heartbeat_pin", ok);
  if (!ok) { server.send(400, "application/json", "{\"ok\":false,\"error\":\"heartbeat_pin invalid\"}"); return; }
  config.sdaPin = parsePinArgument(server, "sda_pin", ok);
  if (!ok) { server.send(400, "application/json", "{\"ok\":false,\"error\":\"sda_pin invalid\"}"); return; }
  config.sclPin = parsePinArgument(server, "scl_pin", ok);
  if (!ok) { server.send(400, "application/json", "{\"ok\":false,\"error\":\"scl_pin invalid\"}"); return; }

  config.relayActiveLow = !server.hasArg("relay_active_low") ||
                          server.arg("relay_active_low") == "1" ||
                          server.arg("relay_active_low") == "true";
  config.digitalInputActiveLow =
    !server.hasArg("digital_input_active_low") ||
    server.arg("digital_input_active_low") == "1" ||
    server.arg("digital_input_active_low") == "true";
  const int digitalInputMode = server.hasArg("digital_input_mode")
    ? server.arg("digital_input_mode").toInt()
    : DIGITAL_INPUT_PULLUP;
  config.digitalInputMode = static_cast<uint8_t>(digitalInputMode);

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

#if defined(ZIGBEE_COPROCESSOR_POWER_PIN)
  pinMode(ZIGBEE_COPROCESSOR_POWER_PIN, OUTPUT);
  digitalWrite(ZIGBEE_COPROCESSOR_POWER_PIN, HIGH);
  serialLog.printf("[Zigbee] TLSR8258 alimentat prin GPIO%d; HCI nu este activ in acest firmware\n",
                   ZIGBEE_COPROCESSOR_POWER_PIN);
#endif

  loadUiConfig();
  loadCommunicationConfig();
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

  if (g_hardware.digitalInputPin != PIN_DISABLED)
  {
    uint8_t inputMode = INPUT;
    if (g_hardware.digitalInputMode == DIGITAL_INPUT_PULLUP)
      inputMode = INPUT_PULLUP;
    else if (g_hardware.digitalInputMode == DIGITAL_INPUT_PULLDOWN)
      inputMode = INPUT_PULLDOWN;
    pinMode(g_hardware.digitalInputPin, inputMode);
    const bool rawHigh = digitalRead(g_hardware.digitalInputPin) == HIGH;
    g_digitalInputActive =
      g_hardware.digitalInputActiveLow ? !rawHigh : rawHigh;
    g_digitalInputReady = true;
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

  g_spiffsReady = SPIFFS.begin(true);
  if (g_spiffsReady)
  {
    serialLog.printf("[SPIFFS] Montat: %u / %u bytes folositi\n",
                     static_cast<unsigned>(SPIFFS.usedBytes()),
                     static_cast<unsigned>(SPIFFS.totalBytes()));
    persistResetLog();
  }
  else
  {
    serialLog.println("[SPIFFS] Montarea sau formatarea partitiei a esuat");
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
  wifiManager.on("/api/communication_config", HTTP_GET,  handleCommunicationConfigGet);
  wifiManager.on("/api/communication_config", HTTP_POST, handleCommunicationConfigPost);
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
  wifiManager.on("/api/files",           HTTP_GET,  handleSpiffsList);
  wifiManager.on("/api/files/download",  HTTP_GET,  handleSpiffsDownload);
  wifiManager.getServer().on("/api/files/upload", HTTP_POST,
                             handleSpiffsUploadComplete, handleSpiffsUploadData);

  // Incarca si aplica configuratia MQTT din NVS
  loadMqttConfig();
  mqttClient.setServer(g_mqttBroker.c_str(), g_mqttPort);
  mqttClient.setCallback(mqttCallback);
  mqttClient.setBufferSize(768);

  // xTaskCreate este portabil pe tintele single-core si dual-core.
  xTaskCreate(taskSensors, "taskSensors", 4096, nullptr, 2, nullptr);
  if (g_hardware.heartbeatPin != PIN_DISABLED)
    xTaskCreate(taskHeartbeat, "taskHeartbeat", 1024, nullptr, 1, nullptr);

  serialLog.println("[WiFi] AP: ESP32_HAKit  /  Parola: 12345678");
  serialLog.println("[Web]  http://" + wifiManager.getIPAddress());
  if (g_mdnsReady)
    serialLog.println("[Web]  http://esp32-ha-kit.local");
  serialLog.println("[MQTT] Broker: " + g_mqttBroker + ":" + String(g_mqttPort));
  serialLog.printf("[Pini] DHT=%d PIR=%d Releu=%d Input=%d LED=%d SDA=%d SCL=%d\n",
                   g_hardware.dhtPin, g_hardware.pirPin, g_hardware.relayPin,
                   g_hardware.digitalInputPin, g_hardware.heartbeatPin,
                   g_hardware.sdaPin, g_hardware.sclPin);
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

  // Intrare digitala cu debounce pentru butoane si contacte mecanice.
  static bool inputInitialized = false;
  static bool lastRawInput = false;
  static bool stableInput = false;
  static unsigned long inputChangedAt = 0;
  if (g_digitalInputReady)
  {
    const bool rawHigh = digitalRead(g_hardware.digitalInputPin) == HIGH;
    const bool active =
      g_hardware.digitalInputActiveLow ? !rawHigh : rawHigh;
    if (!inputInitialized)
    {
      inputInitialized = true;
      lastRawInput = active;
      stableInput = active;
      inputChangedAt = millis();
    }
    else
    {
      if (active != lastRawInput)
      {
        lastRawInput = active;
        inputChangedAt = millis();
      }
      if (stableInput != lastRawInput && millis() - inputChangedAt >= 40)
      {
        stableInput = lastRawInput;
        g_digitalInputActive = stableInput;
        publishState();
      }
    }
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
