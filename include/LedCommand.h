#ifndef LED_COMMAND_H
#define LED_COMMAND_H

#include <cstdint>
#include <cstddef>
#include <cstring>

namespace led {
enum class Action { On, Off, Color, Brightness, Blink, StopBlink, Status };
struct Command {
  Action action = Action::Status;
  uint32_t value = 0;
};

inline bool number(const char* text, uint32_t minimum, uint32_t maximum, uint32_t& result)
{
  if (!*text) return false;
  uint32_t value = 0;
  for (; *text; ++text) {
    if (*text < '0' || *text > '9') return false;
    const uint32_t digit = *text - '0';
    if (value > maximum / 10 ||
        (value == maximum / 10 && digit > maximum % 10)) return false;
    value = value * 10 + digit;
  }
  if (value < minimum) return false;
  result = value;
  return true;
}

// One ASCII command per GATT write; reject embedded NULs and truncated input.
inline bool parse(const char* data, size_t size, Command& result)
{
  if (!data || size == 0 || size > 31) return false;
  while (size && (data[size-1] == '\r' || data[size-1] == '\n' || data[size-1] == ' ')) --size;
  while (size && *data == ' ') { ++data; --size; }
  if (!size) return false;
  char text[32] = {};
  for (size_t i = 0; i < size; ++i) {
    const unsigned char c = data[i];
    if (c < 32 || c > 126) return false;
    text[i] = c >= 'a' && c <= 'z' ? c - 'a' + 'A' : c;
  }
  Command command;
  if (!strcmp(text, "ON")) command.action = Action::On;
  else if (!strcmp(text, "OFF")) command.action = Action::Off;
  else if (!strcmp(text, "STATUS")) command.action = Action::Status;
  else if (!strcmp(text, "BLINK OFF")) command.action = Action::StopBlink;
  else if (!strncmp(text, "BLINK ", 6)) {
    command.action = Action::Blink;
    if (!number(text + 6, 100, 60000, command.value)) return false;
  } else if (!strncmp(text, "BRIGHT ", 7)) {
    command.action = Action::Brightness;
    if (!number(text + 7, 0, 100, command.value)) return false;
  } else if (!strncmp(text, "COLOR #", 7) && size == 13) {
    command.action = Action::Color;
    for (size_t i = 7; i < 13; ++i) {
      const char c = text[i];
      if (!((c >= '0' && c <= '9') || (c >= 'A' && c <= 'F'))) return false;
      command.value = (command.value << 4) | (c <= '9' ? c - '0' : c - 'A' + 10);
    }
  } else return false;
  result = command;
  return true;
}
}
#endif
