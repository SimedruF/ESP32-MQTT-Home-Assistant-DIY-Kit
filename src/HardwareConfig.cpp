#include "HardwareConfig.h"
#include <SerialLog.h>

#include <esp32-hal-gpio.h>
#include <soc/soc_caps.h>

#ifndef DEVICE_PROFILE_NAME
#define DEVICE_PROFILE_NAME "Generic ESP32"
#endif

namespace {

struct PinRole {
  const char* name;
  int pin;
  bool requiresOutput;
};

bool isAssigned(int pin)
{
  return pin != PIN_DISABLED;
}

bool validPinNumber(int pin)
{
  return pin >= 0 && pin < SOC_GPIO_PIN_COUNT && digitalPinIsValid(pin);
}

String jsonEscape(const String& value)
{
  String escaped;
  escaped.reserve(value.length() + 8);
  for (size_t i = 0; i < value.length(); ++i) {
    const char c = value[i];
    if (c == '"' || c == '\\') {
      escaped += '\\';
      escaped += c;
    } else if (c == '\n') {
      escaped += F("\\n");
    } else if (c == '\r') {
      escaped += F("\\r");
    } else {
      escaped += c;
    }
  }
  return escaped;
}

const char* assignedRole(const HardwareConfig& config, int pin)
{
  if (config.dhtPin == pin) return "DHT";
  if (config.pirPin == pin) return "PIR";
  if (config.relayPin == pin) return "relay";
  if (config.heartbeatPin == pin) return "heartbeat";
  if (config.sdaPin == pin) return "I2C SDA";
  if (config.sclPin == pin) return "I2C SCL";
  return "";
}

}  // namespace

const char* targetName()
{
#if defined(CONFIG_IDF_TARGET_ESP32S3)
  return "ESP32-S3";
#elif defined(CONFIG_IDF_TARGET_ESP32C6)
  return "ESP32-C6";
#elif defined(CONFIG_IDF_TARGET_ESP32C3)
  return "ESP32-C3";
#elif defined(CONFIG_IDF_TARGET_ESP32)
  return "ESP32";
#else
  return "ESP32 family";
#endif
}

const char* buildProfileName()
{
  return DEVICE_PROFILE_NAME;
}

HardwareConfig HardwareConfigStore::defaults() const
{
#if defined(CONFIG_IDF_TARGET_ESP32S3)
  return {4, 5, 7, 18, 8, 9, true, 0x3C};
#elif defined(CONFIG_IDF_TARGET_ESP32C6)
  return {2, 3, 7, 18, 6, 10, true, 0x3C};
#elif defined(CONFIG_IDF_TARGET_ESP32C3)
  return {4, 3, 7, 10, 6, 5, true, 0x3C};
#else
  return {4, 32, 23, 18, 21, 22, true, 0x3C};
#endif
}

HardwareConfig HardwareConfigStore::load()
{
  HardwareConfig config = defaults();
  _preferences.begin("hardware", true);
  config.dhtPin = _preferences.getChar("dht", config.dhtPin);
  config.pirPin = _preferences.getChar("pir", config.pirPin);
  config.relayPin = _preferences.getChar("relay", config.relayPin);
  config.heartbeatPin = _preferences.getChar("heart", config.heartbeatPin);
  config.sdaPin = _preferences.getChar("sda", config.sdaPin);
  config.sclPin = _preferences.getChar("scl", config.sclPin);
  config.relayActiveLow = _preferences.getBool("relay_low", config.relayActiveLow);
  config.oledAddress = _preferences.getUChar("oled_addr", config.oledAddress);
  _preferences.end();

  String error;
  if (!validateHardwareConfig(config, error)) {
    serialLog.println("[HW] Configuratie NVS invalida: " + error);
    serialLog.println("[HW] Se folosesc valorile implicite pentru " + String(buildProfileName()));
    config = defaults();
  }
  return config;
}

bool HardwareConfigStore::save(const HardwareConfig& config)
{
  String error;
  if (!validateHardwareConfig(config, error)) return false;

  _preferences.begin("hardware", false);
  bool ok = true;
  ok &= _preferences.putChar("dht", config.dhtPin) > 0;
  ok &= _preferences.putChar("pir", config.pirPin) > 0;
  ok &= _preferences.putChar("relay", config.relayPin) > 0;
  ok &= _preferences.putChar("heart", config.heartbeatPin) > 0;
  ok &= _preferences.putChar("sda", config.sdaPin) > 0;
  ok &= _preferences.putChar("scl", config.sclPin) > 0;
  ok &= _preferences.putBool("relay_low", config.relayActiveLow) > 0;
  ok &= _preferences.putUChar("oled_addr", config.oledAddress) > 0;
  _preferences.end();
  return ok;
}

void HardwareConfigStore::clear()
{
  _preferences.begin("hardware", false);
  _preferences.clear();
  _preferences.end();
}

bool isReservedPin(int pin)
{
#if defined(CONFIG_IDF_TARGET_ESP32S3)
  return pin >= 26 && pin <= 34;
#elif defined(CONFIG_IDF_TARGET_ESP32C6)
  return pin == 14 || (pin >= 24 && pin <= 30);
#elif defined(CONFIG_IDF_TARGET_ESP32C3)
  return pin >= 11 && pin <= 17;
#elif defined(CONFIG_IDF_TARGET_ESP32)
  return pin >= 6 && pin <= 11;
#else
  return false;
#endif
}

const char* pinRestriction(int pin)
{
#if defined(CONFIG_IDF_TARGET_ESP32S3)
  if (pin >= 26 && pin <= 32) return "rezervat pentru memoria flash";
  if (pin == 33 || pin == 34) return "neexpus pe ESP32-S3-DevKitC-1";
#elif defined(CONFIG_IDF_TARGET_ESP32C6)
  if (pin == 14) return "neexpus pe ESP32-C6-DevKitC-1";
  if (pin >= 24 && pin <= 30) return "rezervat pentru memoria flash";
#elif defined(CONFIG_IDF_TARGET_ESP32C3)
  if (pin == 11) return "neexpus pe ESP32-C3-DevKitM-1";
  if (pin >= 12 && pin <= 17) return "rezervat pentru memoria flash";
#elif defined(CONFIG_IDF_TARGET_ESP32)
  if (pin >= 6 && pin <= 11) return "rezervat pentru memoria flash";
#endif
  return "";
}

const char* pinWarning(int pin)
{
#if defined(CONFIG_IDF_TARGET_ESP32S3)
  if (pin == 19 || pin == 20) return "USB OTG/JTAG implicit";
  if (pin == 43 || pin == 44) return "UART0 folosit pentru programare/log";
  if (pin >= 35 && pin <= 37) return "indisponibil pe modulele cu flash/PSRAM Octal";
  if (pin == 38 || pin == 48) return "posibil LED RGB onboard, in functie de revizie";
  if (pin == 0 || pin == 3 || pin == 45 || pin == 46) return "pin de boot/strapping";
#elif defined(CONFIG_IDF_TARGET_ESP32C6)
  if (pin == 12 || pin == 13) return "USB-JTAG implicit";
  if (pin == 4 || pin == 5 || pin == 8 || pin == 9 || pin == 15) return "pin de boot/strapping";
#elif defined(CONFIG_IDF_TARGET_ESP32C3)
  if (pin == 18 || pin == 19) return "USB-JTAG implicit";
  if (pin == 2 || pin == 8 || pin == 9) return "pin de boot/strapping";
#elif defined(CONFIG_IDF_TARGET_ESP32)
  if (pin >= 34 && pin <= 39) return "doar intrare";
  if (pin == 0 || pin == 2 || pin == 5 || pin == 12 || pin == 15) return "pin de boot/strapping";
#endif
  return "";
}

bool validateHardwareConfig(const HardwareConfig& config, String& error)
{
  const PinRole roles[] = {
    {"DHT", config.dhtPin, true},
    {"PIR", config.pirPin, false},
    {"releu", config.relayPin, true},
    {"heartbeat", config.heartbeatPin, true},
    {"I2C SDA", config.sdaPin, true},
    {"I2C SCL", config.sclPin, true},
  };

  if (isAssigned(config.sdaPin) != isAssigned(config.sclPin)) {
    error = F("SDA si SCL trebuie configurati impreuna sau dezactivati impreuna");
    return false;
  }

  if (config.oledAddress != 0x3C && config.oledAddress != 0x3D) {
    error = F("adresa OLED trebuie sa fie 0x3C sau 0x3D");
    return false;
  }

  for (const PinRole& role : roles) {
    if (!isAssigned(role.pin)) continue;
    if (!validPinNumber(role.pin)) {
      error = String(role.name) + F(": GPIO invalid pentru acest cip");
      return false;
    }
    if (isReservedPin(role.pin)) {
      error = String(role.name) + F(": GPIO") + role.pin + F(" este ") + pinRestriction(role.pin);
      return false;
    }
    if (role.requiresOutput && !digitalPinCanOutput(role.pin)) {
      error = String(role.name) + F(": GPIO") + role.pin + F(" nu poate fi iesire");
      return false;
    }
  }

  for (size_t i = 0; i < sizeof(roles) / sizeof(roles[0]); ++i) {
    if (!isAssigned(roles[i].pin)) continue;
    for (size_t j = i + 1; j < sizeof(roles) / sizeof(roles[0]); ++j) {
      if (roles[i].pin == roles[j].pin) {
        error = String(F("GPIO")) + roles[i].pin + F(" este atribuit simultan pentru ") +
                roles[i].name + F(" si ") + roles[j].name;
        return false;
      }
    }
  }

  error = "";
  return true;
}

String hardwareConfigToJson(const HardwareConfig& config)
{
  String json;
  json.reserve(320);
  json = F("{\"target\":\"");
  json += jsonEscape(targetName());
  json += F("\",\"profile\":\"");
  json += jsonEscape(buildProfileName());
  json += F("\",\"gpio_count\":");
  json += SOC_GPIO_PIN_COUNT;
  json += F(",\"dht_pin\":");
  json += config.dhtPin;
  json += F(",\"pir_pin\":");
  json += config.pirPin;
  json += F(",\"relay_pin\":");
  json += config.relayPin;
  json += F(",\"heartbeat_pin\":");
  json += config.heartbeatPin;
  json += F(",\"sda_pin\":");
  json += config.sdaPin;
  json += F(",\"scl_pin\":");
  json += config.sclPin;
  json += F(",\"relay_active_low\":");
  json += config.relayActiveLow ? F("true") : F("false");
  json += F(",\"oled_address\":");
  json += config.oledAddress;
  json += '}';
  return json;
}

String gpioInventoryToJson(const HardwareConfig& config)
{
  String json;
  json.reserve(1800);
  json = '[';
  bool first = true;
  for (int pin = 0; pin < SOC_GPIO_PIN_COUNT; ++pin) {
    if (!digitalPinIsValid(pin)) continue;
    if (!first) json += ',';
    first = false;

    json += F("{\"pin\":");
    json += pin;
    json += F(",\"output\":");
    json += digitalPinCanOutput(pin) ? F("true") : F("false");
    json += F(",\"reserved\":");
    json += isReservedPin(pin) ? F("true") : F("false");
    json += F(",\"restriction\":\"");
    json += pinRestriction(pin);
    json += '"';
    json += F(",\"role\":\"");
    json += assignedRole(config, pin);
    json += F("\",\"warning\":\"");
    json += pinWarning(pin);
    json += F("\"}");
  }
  json += ']';
  return json;
}
