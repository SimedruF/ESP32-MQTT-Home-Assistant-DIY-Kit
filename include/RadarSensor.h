#ifndef RADAR_SENSOR_H
#define RADAR_SENSOR_H

#include <Arduino.h>

struct RadarSnapshot {
  bool supported;
  bool connected;
  bool presence;
  bool moving;
  bool stationary;
  uint16_t movingDistanceCm;
  uint8_t movingEnergy;
  uint16_t stationaryDistanceCm;
  uint8_t stationaryEnergy;
  uint16_t detectionDistanceCm;
  uint32_t ageMs;
  int8_t rxPin;
  int8_t txPin;
  uint32_t baudRate;
};

void radarSensorBegin();
void radarSensorPoll();
RadarSnapshot radarSensorSnapshot();

#endif
