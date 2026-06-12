#include <SerialLog.h>

SerialLog serialLog;

void SerialLog::begin()
{
  if (_mutex == nullptr)
    _mutex = xSemaphoreCreateMutex();
}

size_t SerialLog::write(uint8_t value)
{
  return write(&value, 1);
}

size_t SerialLog::write(const uint8_t* buffer, size_t size)
{
  const size_t written = Serial.write(buffer, size);
  if (buffer == nullptr || size == 0)
    return written;

  if (_mutex != nullptr)
    xSemaphoreTake(_mutex, portMAX_DELAY);

  for (size_t i = 0; i < size; ++i)
  {
    _buffer[_nextSequence % BUFFER_SIZE] = static_cast<char>(buffer[i]);
    ++_nextSequence;
    if (_storedBytes < BUFFER_SIZE)
      ++_storedBytes;
  }

  if (_mutex != nullptr)
    xSemaphoreGive(_mutex);

  return written;
}

String SerialLog::readSince(uint32_t requestedSequence,
                            uint32_t& firstSequence,
                            uint32_t& nextSequence,
                            bool& dataWasDropped)
{
  if (_mutex != nullptr)
    xSemaphoreTake(_mutex, portMAX_DELAY);

  const uint32_t oldestSequence = _nextSequence - static_cast<uint32_t>(_storedBytes);
  uint32_t startSequence = requestedSequence;

  if (startSequence < oldestSequence || startSequence > _nextSequence)
    startSequence = oldestSequence;

  dataWasDropped = requestedSequence != 0 && requestedSequence < oldestSequence;
  firstSequence = startSequence;
  nextSequence = _nextSequence;

  String result;
  result.reserve(static_cast<size_t>(nextSequence - startSequence));
  for (uint32_t sequence = startSequence; sequence < nextSequence; ++sequence)
    result += _buffer[sequence % BUFFER_SIZE];

  if (_mutex != nullptr)
    xSemaphoreGive(_mutex);

  return result;
}

void SerialLog::clear()
{
  if (_mutex != nullptr)
    xSemaphoreTake(_mutex, portMAX_DELAY);

  _storedBytes = 0;

  if (_mutex != nullptr)
    xSemaphoreGive(_mutex);
}
