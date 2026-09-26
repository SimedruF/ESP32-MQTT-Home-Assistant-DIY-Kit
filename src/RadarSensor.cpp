#include <RadarSensor.h>

#include <SerialLog.h>

#if defined(CONFIG_IDF_TARGET_ESP32S3)
#include <HardwareSerial.h>
#include <ld2410.h>
#endif

namespace {

#if defined(CONFIG_IDF_TARGET_ESP32S3)
constexpr bool RADAR_SUPPORTED = true;
constexpr int8_t RADAR_RX_PIN = 10;  // LD2410C TX -> ESP32 RX
constexpr int8_t RADAR_TX_PIN = 11;  // LD2410C RX -> ESP32 TX
constexpr uint32_t RADAR_BAUD_RATE = 256000;

HardwareSerial radarSerial(1);
ld2410 radar;
bool radarStarted = false;
uint32_t lastValidFrameMs = 0;
RadarSnapshot latest = {
  true, false, false, false, false,
  0, 0, 0, 0, 0, UINT32_MAX,
  RADAR_RX_PIN, RADAR_TX_PIN, RADAR_BAUD_RATE
};
#else
constexpr bool RADAR_SUPPORTED = false;
RadarSnapshot latest = {
  false, false, false, false, false,
  0, 0, 0, 0, 0, UINT32_MAX,
  -1, -1, 0
};
#endif

}  // namespace

void radarSensorBegin()
{
#if defined(CONFIG_IDF_TARGET_ESP32S3)
  radarSerial.begin(RADAR_BAUD_RATE, SERIAL_8N1, RADAR_RX_PIN, RADAR_TX_PIN);
  // Do not wait for a command response. Data frames are detected asynchronously,
  // so the sensor may also be connected or powered after the ESP32 has booted.
  radarStarted = radar.begin(radarSerial, false);
  serialLog.printf("[LD2410C] UART initializat: RX=GPIO%d TX=GPIO%d, %lu baud\n",
                   RADAR_RX_PIN, RADAR_TX_PIN,
                   static_cast<unsigned long>(RADAR_BAUD_RATE));
#else
  serialLog.println("[LD2410C] UART disponibil momentan doar in profilul ESP32-S3");
#endif
}

void radarSensorPoll()
{
#if defined(CONFIG_IDF_TARGET_ESP32S3)
  if (!radarStarted) return;

  radar.read();
  if (radar.isConnected())
  {
    lastValidFrameMs = millis();
    latest.presence = radar.presenceDetected();
    latest.moving = radar.movingTargetDetected();
    latest.stationary = radar.stationaryTargetDetected();
    latest.movingDistanceCm = radar.movingTargetDistance();
    latest.movingEnergy = radar.movingTargetEnergy();
    latest.stationaryDistanceCm = radar.stationaryTargetDistance();
    latest.stationaryEnergy = radar.stationaryTargetEnergy();
    latest.detectionDistanceCm = radar.detectionDistance();
  }

  const uint32_t now = millis();
  latest.connected = lastValidFrameMs != 0 && now - lastValidFrameMs <= 2000;
  latest.ageMs = lastValidFrameMs == 0 ? UINT32_MAX : now - lastValidFrameMs;
#endif
}

RadarSnapshot radarSensorSnapshot()
{
  (void)RADAR_SUPPORTED;
  return latest;
}
