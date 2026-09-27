#include <RadarSensor.h>

#include <SerialLog.h>

#if defined(CONFIG_IDF_TARGET_ESP32S3)
#include <HardwareSerial.h>
#include <MyLD2410.h>
#endif

namespace {

#if defined(CONFIG_IDF_TARGET_ESP32S3)
constexpr bool RADAR_SUPPORTED = true;
constexpr int8_t RADAR_RX_PIN = 18;  // LD2410C TX -> ESP32 RX
constexpr int8_t RADAR_TX_PIN = 17;  // LD2410C RX -> ESP32 TX
constexpr uint32_t RADAR_BAUD_RATE = 256000;

class CountingStream : public Stream {
public:
  explicit CountingStream(HardwareSerial& serial) : _serial(serial) {}

  int available() override { return _serial.available(); }

  int read() override
  {
    const int value = _serial.read();
    if (value >= 0)
    {
      ++_receivedBytes;
      _lastByteMs = millis();
    }
    return value;
  }

  int peek() override { return _serial.peek(); }
  void flush() override { _serial.flush(); }
  size_t write(uint8_t value) override { return _serial.write(value); }
  size_t write(const uint8_t* buffer, size_t size) override
  {
    return _serial.write(buffer, size);
  }

  uint32_t receivedBytes() const { return _receivedBytes; }
  uint32_t lastByteMs() const { return _lastByteMs; }

private:
  HardwareSerial& _serial;
  uint32_t _receivedBytes = 0;
  uint32_t _lastByteMs = 0;
};

HardwareSerial radarSerial(1);
CountingStream radarStream(radarSerial);
MyLD2410 radar(radarStream, false);
bool radarStarted = false;
uint32_t lastValidFrameMs = 0;
RadarSnapshot latest = {
  true, false, false, false, false,
  0, 0, 0, 0, 0, UINT32_MAX,
  RADAR_RX_PIN, RADAR_TX_PIN, RADAR_BAUD_RATE,
  0, UINT32_MAX, false
};
#else
constexpr bool RADAR_SUPPORTED = false;
RadarSnapshot latest = {
  false, false, false, false, false,
  0, 0, 0, 0, 0, UINT32_MAX,
  -1, -1, 0, 0, UINT32_MAX, false
};
#endif

}  // namespace

void radarSensorBegin()
{
#if defined(CONFIG_IDF_TARGET_ESP32S3)
  radarSerial.setRxBufferSize(512);
  radarSerial.begin(RADAR_BAUD_RATE, SERIAL_8N1,
                    RADAR_RX_PIN, RADAR_TX_PIN);
  radarStarted = true;
  serialLog.printf("[LD2410C] MyLD2410: RX=GPIO%d TX=GPIO%d, %lu baud\n",
                   RADAR_RX_PIN, RADAR_TX_PIN,
                   static_cast<unsigned long>(RADAR_BAUD_RATE));
  if (radar.begin())
    serialLog.println("[LD2410C] Senzor detectat de MyLD2410");
  else
    serialLog.println("[LD2410C] Fara raspuns initial; ascultarea UART continua");
#else
  serialLog.println("[LD2410C] UART disponibil momentan doar in profilul ESP32-S3");
#endif
}

void radarSensorPoll()
{
#if defined(CONFIG_IDF_TARGET_ESP32S3)
  if (!radarStarted) return;

  const uint32_t now = millis();
  if (radar.check() == MyLD2410::DATA)
  {
    lastValidFrameMs = now;
    latest.presence = radar.presenceDetected();
    latest.moving = radar.movingTargetDetected();
    latest.stationary = radar.stationaryTargetDetected();
    latest.movingDistanceCm = static_cast<uint16_t>(radar.movingTargetDistance());
    latest.movingEnergy = radar.movingTargetSignal();
    latest.stationaryDistanceCm =
      static_cast<uint16_t>(radar.stationaryTargetDistance());
    latest.stationaryEnergy = radar.stationaryTargetSignal();
    latest.detectionDistanceCm = static_cast<uint16_t>(radar.detectedDistance());
  }

  latest.connected = lastValidFrameMs != 0 && now - lastValidFrameMs <= 2000;
  latest.ageMs = lastValidFrameMs == 0 ? UINT32_MAX : now - lastValidFrameMs;
  latest.receivedBytes = radarStream.receivedBytes();
  const uint32_t lastByteMs = radarStream.lastByteMs();
  latest.lastByteAgeMs = lastByteMs == 0 ? UINT32_MAX : now - lastByteMs;
  latest.baudScanning = false;
#endif
}

RadarSnapshot radarSensorSnapshot()
{
  (void)RADAR_SUPPORTED;
  return latest;
}
