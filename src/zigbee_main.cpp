#include <Arduino.h>
#include <DHT.h>
#include <Preferences.h>
#include <Zigbee.h>

#include <HardwareConfig.h>
#include <SerialLog.h>

#ifndef ZIGBEE_MODE_ZCZR
#error "Profilul Zigbee necesita ZIGBEE_MODE_ZCZR"
#endif

namespace {

constexpr uint8_t TEMP_ENDPOINT = 10;
constexpr uint8_t OCCUPANCY_ENDPOINT = 11;
constexpr uint8_t OUTLET_ENDPOINT = 12;
constexpr uint32_t SENSOR_INTERVAL_MS = 30000;
constexpr uint32_t PAIRING_LOG_INTERVAL_MS = 2000;
constexpr uint32_t FACTORY_RESET_HOLD_MS = 5000;

HardwareConfigStore hardwareStore;
HardwareConfig hardware;
DHT* dht = nullptr;

ZigbeeTempSensor temperatureEndpoint(TEMP_ENDPOINT);
ZigbeeOccupancySensor occupancyEndpoint(OCCUPANCY_ENDPOINT);
ZigbeePowerOutlet outletEndpoint(OUTLET_ENDPOINT);

bool lastOccupancy = false;
bool occupancyReported = false;
bool forceNewNetwork = false;

void applyRelayState(bool active)
{
  if (hardware.relayPin == PIN_DISABLED) return;
  const uint8_t activeLevel = hardware.relayActiveLow ? LOW : HIGH;
  digitalWrite(hardware.relayPin, active ? activeLevel : !activeLevel);
  serialLog.printf("[Zigbee] Releu comandat: %s\n", active ? "ON" : "OFF");
}

void loadPairingRequest()
{
  Preferences preferences;
  if (!preferences.begin("iot_radio", false)) return;
  forceNewNetwork = preferences.getString("zb_pair", "auto") == "force_new";
  if (forceNewNetwork)
    preferences.putString("zb_pair", "auto");
  preferences.end();
}

void configureHardware()
{
  hardware = hardwareStore.load();

  if (hardware.pirPin != PIN_DISABLED)
    pinMode(hardware.pirPin, INPUT);

  if (hardware.relayPin != PIN_DISABLED)
  {
    pinMode(hardware.relayPin, OUTPUT);
    applyRelayState(false);
  }

  if (hardware.heartbeatPin != PIN_DISABLED)
  {
    pinMode(hardware.heartbeatPin, OUTPUT);
    digitalWrite(hardware.heartbeatPin, LOW);
  }

  if (hardware.dhtPin != PIN_DISABLED)
  {
    dht = new DHT(hardware.dhtPin, DHT11);
    dht->begin();
  }
}

void configureEndpoints()
{
  temperatureEndpoint.setManufacturerAndModel(
    "AutomaticHouse", "ESP32C6-HA-Kit-TH");
  temperatureEndpoint.setMinMaxValue(-40, 85);
  temperatureEndpoint.setTolerance(0.5);
  temperatureEndpoint.addHumiditySensor(0, 100, 1, 50);

  occupancyEndpoint.setManufacturerAndModel(
    "AutomaticHouse", "ESP32C6-HA-Kit-PIR");

  outletEndpoint.setManufacturerAndModel(
    "AutomaticHouse", "ESP32C6-HA-Kit-Relay");
  outletEndpoint.onPowerOutletChange(applyRelayState);

  Zigbee.allowMultiEndpointBinding(true);
  Zigbee.addEndpoint(&temperatureEndpoint);
  Zigbee.addEndpoint(&occupancyEndpoint);
  Zigbee.addEndpoint(&outletEndpoint);
}

void reportEnvironment()
{
  if (!dht || !Zigbee.connected()) return;

  const float temperature = dht->readTemperature();
  const float humidity = dht->readHumidity();
  if (isnan(temperature) || isnan(humidity))
  {
    serialLog.println("[Zigbee] Citire DHT11 invalida");
    return;
  }

  temperatureEndpoint.setTemperature(temperature);
  temperatureEndpoint.setHumidity(humidity);
  temperatureEndpoint.report();
  serialLog.printf("[Zigbee] Raport: temperatura %.1f C, umiditate %.1f%%\n",
                   temperature, humidity);
}

void reportOccupancyIfChanged()
{
  if (hardware.pirPin == PIN_DISABLED || !Zigbee.connected()) return;

  const bool occupied = digitalRead(hardware.pirPin) == HIGH;
  if (occupancyReported && occupied == lastOccupancy) return;

  lastOccupancy = occupied;
  occupancyReported = true;
  occupancyEndpoint.setOccupancy(occupied);
  occupancyEndpoint.report();
  serialLog.printf("[Zigbee] Raport PIR: %s\n",
                   occupied ? "ocupat" : "liber");
}

void handleFactoryResetButton()
{
  static uint32_t pressedAt = 0;
  const bool pressed = digitalRead(BOOT_PIN) == LOW;

  if (!pressed)
  {
    pressedAt = 0;
    return;
  }

  if (pressedAt == 0)
  {
    pressedAt = millis();
    return;
  }

  if (millis() - pressedAt >= FACTORY_RESET_HOLD_MS)
  {
    serialLog.println("[Zigbee] Factory reset si repornire");
    delay(100);
    Zigbee.factoryReset();
  }
}

}  // namespace

void setup()
{
  Serial.begin(115200);
  serialLog.begin();
  delay(300);
  serialLog.println("\n=== ESP32-C6 HA Kit - Zigbee Router ===");

  pinMode(BOOT_PIN, INPUT_PULLUP);
  loadPairingRequest();
  configureHardware();
  configureEndpoints();

  Zigbee.setTimeout(60000);
  esp_zb_cfg_t config = ZIGBEE_DEFAULT_ROUTER_CONFIG();
  if (!Zigbee.begin(&config, forceNewNetwork))
  {
    serialLog.println("[Zigbee] Stack-ul nu a pornit; restart");
    delay(1000);
    ESP.restart();
  }

  serialLog.println(
    forceNewNetwork
      ? "[Zigbee] Asocierea veche a fost stearsa; caut retea in permit join"
      : "[Zigbee] Rejoin la reteaua salvata sau caut retea in permit join");

  uint32_t lastPairingLog = 0;
  while (!Zigbee.connected())
  {
    if (millis() - lastPairingLog >= PAIRING_LOG_INTERVAL_MS)
    {
      lastPairingLog = millis();
      serialLog.println("[Zigbee] Se asteapta coordinatorul...");
    }
    handleFactoryResetButton();
    delay(50);
  }

  serialLog.println("[Zigbee] Conectat la retea");
  temperatureEndpoint.setReporting(5, 300, 0.5);
  temperatureEndpoint.setHumidityReporting(5, 300, 1.0);
  outletEndpoint.setState(false);
  reportEnvironment();
  reportOccupancyIfChanged();
}

void loop()
{
  static uint32_t lastSensorReport = 0;
  static uint32_t lastHeartbeat = 0;

  handleFactoryResetButton();
  reportOccupancyIfChanged();

  if (millis() - lastSensorReport >= SENSOR_INTERVAL_MS)
  {
    lastSensorReport = millis();
    reportEnvironment();
  }

  if (hardware.heartbeatPin != PIN_DISABLED &&
      millis() - lastHeartbeat >= 1000)
  {
    lastHeartbeat = millis();
    digitalWrite(hardware.heartbeatPin,
                 !digitalRead(hardware.heartbeatPin));
  }

  delay(50);
}
