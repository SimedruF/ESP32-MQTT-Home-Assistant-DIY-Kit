#ifndef SERIAL_LOG_H
#define SERIAL_LOG_H

#include <Arduino.h>

class SerialLog : public Print
{
public:
  static constexpr size_t BUFFER_SIZE = 8192;

  void begin();
  size_t write(uint8_t value) override;
  size_t write(const uint8_t* buffer, size_t size) override;

  String readSince(uint32_t requestedSequence,
                   uint32_t& firstSequence,
                   uint32_t& nextSequence,
                   bool& dataWasDropped);
  void clear();

private:
  char _buffer[BUFFER_SIZE] = {};
  uint32_t _nextSequence = 0;
  size_t _storedBytes = 0;
  SemaphoreHandle_t _mutex = nullptr;
};

extern SerialLog serialLog;

#endif
