#ifndef BLE_LED_CONTROL_H
#define BLE_LED_CONTROL_H
#include <Arduino.h>
#include <LedCommand.h>

// Nordic UART Service: RX accepts one text command, TX returns short acknowledgments.
constexpr const char* BLE_LED_SERVICE = "6e400001-b5a3-f393-e0a9-e50e24dcca9e";
constexpr const char* BLE_LED_RX = "6e400002-b5a3-f393-e0a9-e50e24dcca9e";
constexpr const char* BLE_LED_TX = "6e400003-b5a3-f393-e0a9-e50e24dcca9e";
constexpr const char* BLE_LED_STATE = "b71c0001-14af-4f17-bd8f-626544da8310";

bool bleLedSupported();
bool bleLedBegin(const String& name, void (*apply)(const led::Command&), String (*state)());
void bleLedPoll();
bool bleLedConnected();
#endif
