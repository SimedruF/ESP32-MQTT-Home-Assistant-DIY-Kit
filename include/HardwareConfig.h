#ifndef HARDWARE_CONFIG_H
#define HARDWARE_CONFIG_H

#include <Arduino.h>
#include <Preferences.h>

constexpr int8_t PIN_DISABLED = -1;

struct HardwareConfig {
  int8_t dhtPin;
  int8_t pirPin;
  int8_t relayPin;
  int8_t heartbeatPin;
  int8_t sdaPin;
  int8_t sclPin;
  bool relayActiveLow;
  uint8_t oledAddress;
};

class HardwareConfigStore {
public:
  HardwareConfig defaults() const;
  HardwareConfig load();
  bool save(const HardwareConfig& config);
  void clear();

private:
  Preferences _preferences;
};

const char* targetName();
const char* buildProfileName();
bool isReservedPin(int pin);
const char* pinRestriction(int pin);
const char* pinWarning(int pin);
bool validateHardwareConfig(const HardwareConfig& config, String& error);
String hardwareConfigToJson(const HardwareConfig& config);
String gpioInventoryToJson(const HardwareConfig& config);

#endif
