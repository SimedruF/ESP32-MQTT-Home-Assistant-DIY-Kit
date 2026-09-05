// Run on the host: g++ -std=c++17 -Iinclude test/ble_led_commands.cpp -o /tmp/ble-led-test && /tmp/ble-led-test
#include <LedCommand.h>
#include <cassert>
#include <iostream>
#include <string>

int main()
{
  struct Example { const char* text; led::Action action; uint32_t value; };
  const Example examples[] = {
    {"ON", led::Action::On, 0}, {"off", led::Action::Off, 0},
    {"  on\r\n", led::Action::On, 0}, {"STATUS", led::Action::Status, 0},
    {"BLINK OFF", led::Action::StopBlink, 0},
    {"BLINK 100", led::Action::Blink, 100}, {"BLINK 60000", led::Action::Blink, 60000},
    {"BRIGHT 0", led::Action::Brightness, 0}, {"BRIGHT 100", led::Action::Brightness, 100},
    {"color #00aBfF", led::Action::Color, 0x00abff}, {"COLOR #000000", led::Action::Color, 0}
  };
  for (const auto& example : examples) {
    led::Command command;
    assert(led::parse(example.text, std::strlen(example.text), command));
    assert(command.action == example.action && command.value == example.value);
  }
  for (const char* invalid : {"", " ", "ON OFF", "ON\nOFF", "BLINK", "BLINK 99",
       "BLINK 60001", "BLINK -100", "BLINK 500ms", "BLINK 4294967796",
       "BRIGHT 101", "BRIGHT 1.5", "COLOR #12345", "COLOR #1234567",
       "COLOR #gg0000", "STATUS extra", "RELAY ON"}) {
    led::Command command{led::Action::Color, 0x123456};
    assert(!led::parse(invalid, std::strlen(invalid), command));
    assert(command.action == led::Action::Color && command.value == 0x123456);
  }
  led::Command command;
  const char embedded[] = {'O','N','\0','O','F','F'};
  assert(!led::parse(embedded, sizeof(embedded), command));
  const std::string oversized(32, ' ');
  assert(!led::parse(oversized.data(), oversized.size(), command));
  assert(!led::parse(nullptr, 2, command));
  // Arbitrary single-byte input must not be accepted as an LED command.
  for (int c = 0; c < 256; ++c) {
    char byte = static_cast<char>(c);
    assert(!led::parse(&byte, 1, command));
  }
  std::cout << "BLE command tests passed\n";
}
