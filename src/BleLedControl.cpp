#include <BleLedControl.h>
#include <SerialLog.h>
#include <soc/soc_caps.h>

#if defined(SOC_BLE_SUPPORTED) && SOC_BLE_SUPPORTED && \
    (defined(CONFIG_BLUEDROID_ENABLED) || defined(CONFIG_NIMBLE_ENABLED))
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLE2902.h>
#include <atomic>
#include <freertos/queue.h>

namespace {
struct Request { led::Command command; bool valid; };
QueueHandle_t requests = nullptr;
std::atomic<bool> connected{false};
std::atomic<bool> disconnected{false};
std::atomic<bool> overflow{false};
BLEServer* server = nullptr;
BLECharacteristic* replyCharacteristic = nullptr;
BLECharacteristic* stateCharacteristic = nullptr;
void (*applyCommand)(const led::Command&) = nullptr;
String (*getState)() = nullptr;
String previousState;
bool started = false;
bool reconnectPending = false;
uint32_t reconnectAt = 0;
uint32_t lastStatePoll = 0;

class ConnectionCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer*) override { connected.store(true); }
  void onDisconnect(BLEServer*) override {
    connected.store(false);
    disconnected.store(true);
  }
} connectionCallbacks;

class CommandCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic* characteristic) override {
    const auto value = characteristic->getValue();
    Request request{};
    request.valid = led::parse(value.c_str(), value.length(), request.command);
    // The BLE task never touches the LED or blocks waiting for loop().
    if (xQueueSend(requests, &request, 0) != pdTRUE) overflow.store(true);
  }
} commandCallbacks;

void reply(const char* text) {
  replyCharacteristic->setValue(text);
  if (connected.load()) replyCharacteristic->notify();
}
}

bool bleLedSupported() { return true; }
bool bleLedConnected() { return connected.load(); }

bool bleLedBegin(const String& name, void (*apply)(const led::Command&), String (*state)())
{
  if (started) return true;
  requests = xQueueCreate(8, sizeof(Request));
  if (!requests) return false;
  applyCommand = apply;
  getState = state;
  BLEDevice::init(name.c_str());
  if (!BLEDevice::getInitialized()) return false;
  server = BLEDevice::createServer();
  if (!server) return false;
  server->setCallbacks(&connectionCallbacks);
  BLEService* service = server->createService(BLEUUID(BLE_LED_SERVICE), 20);
  if (!service) return false;
  auto* rx = service->createCharacteristic(BLE_LED_RX, BLECharacteristic::PROPERTY_WRITE);
  replyCharacteristic = service->createCharacteristic(BLE_LED_TX,
      BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY);
  stateCharacteristic = service->createCharacteristic(BLE_LED_STATE, BLECharacteristic::PROPERTY_READ);
  if (!rx || !replyCharacteristic || !stateCharacteristic) return false;
  rx->setCallbacks(&commandCallbacks);
#if defined(CONFIG_BLUEDROID_ENABLED)
  replyCharacteristic->addDescriptor(new BLE2902());
#endif
  replyCharacteristic->setValue("READY");
  previousState = getState();
  stateCharacteristic->setValue(previousState.c_str());
  if (!service->start()) return false;
  auto* advertising = server->getAdvertising();
  // Keep the service UUID in the advertisement and the name in the scan response.
  BLEAdvertisementData advertisement;
  advertisement.setFlags(0x06);
  advertisement.setCompleteServices(BLEUUID(BLE_LED_SERVICE));
  BLEAdvertisementData scanResponse;
  scanResponse.setName(name.c_str());
  if (!advertising->setAdvertisementData(advertisement) ||
      !advertising->setScanResponseData(scanResponse) || !advertising->start()) return false;
  started = true;
  serialLog.println("[BLE] LED control: " + name);
  return true;
}

void bleLedPoll()
{
  if (!started) return;
  if (disconnected.exchange(false)) {
    reconnectPending = true;
    reconnectAt = millis();
    // Do not apply commands left over from a disconnected phone.
    xQueueReset(requests);
    serialLog.println("[BLE] Telefon deconectat");
  }
  if (reconnectPending && millis() - reconnectAt >= 500) {
    reconnectPending = false;
    if (!connected.load()) server->startAdvertising();
  }
  Request request;
  for (int i = 0; i < 4 && xQueueReceive(requests, &request, 0) == pdTRUE; ++i) {
    if (!request.valid) { reply("ERR command"); continue; }
    applyCommand(request.command);
    // Update readable state before acknowledging command execution.
    previousState = getState();
    stateCharacteristic->setValue(previousState.c_str());
    reply("OK"); // <=20 bytes: fits even the default ATT MTU.
  }
  if (overflow.exchange(false)) reply("ERR busy");
  if (millis() - lastStatePoll < 250) return;
  lastStatePoll = millis();
  const String state = getState();
  if (state != previousState) {
    previousState = state;
    stateCharacteristic->setValue(state.c_str());
    reply("STATE changed");
  }
}
#else
bool bleLedSupported() { return false; }
bool bleLedConnected() { return false; }
bool bleLedBegin(const String&, void (*)(const led::Command&), String (*)()) { return false; }
void bleLedPoll() {}
#endif
