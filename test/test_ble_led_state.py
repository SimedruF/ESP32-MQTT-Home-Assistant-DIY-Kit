"""Exercise the firmware's actual LED state transitions on the host, without a radio.
Run: python3 test/test_ble_led_state.py
"""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
source = (root / 'src/main.cpp').read_text()
state = source[source.index('static bool g_rgbLedOn'):source.index('static bool g_bleEnabled')]
commands = source[source.index('void setRgbLedMode'):source.index('bool validBleName')]
timer = source[source.index('void handleRgbLedBlink()'):source.index('String rgbLedStateJson()')]
prelude = r'''
#include <LedCommand.h>
#include <cassert>
#include <iostream>
uint32_t now = 0;
uint32_t millis() { return now; }
bool RGB_LED_SUPPORTED = true;
unsigned writes = 0;
void applyRgbLed() { ++writes; }
struct Log { template<typename... T> void printf(const char*, T...) {} } serialLog;
'''
tests = r'''
void send(const char* text) {
  led::Command command;
  assert(led::parse(text, std::strlen(text), command));
  applyBleLedCommand(command);
}
int main() {
  send("ON"); assert(g_rgbLedOn && !g_rgbLedBlink);
  send("COLOR #ff0010");
  assert(g_rgbLedRed == 255 && g_rgbLedGreen == 0 && g_rgbLedBlue == 16);
  send("BRIGHT 40"); assert(g_rgbLedBrightness == 40 && g_rgbLedOn);
  send("BLINK 500"); assert(g_rgbLedBlink && g_rgbLedBlinkPhase);
  now = 499; handleRgbLedBlink(); assert(g_rgbLedBlinkPhase);
  now = 500; handleRgbLedBlink(); assert(!g_rgbLedBlinkPhase);
  send("BRIGHT 0"); assert(g_rgbLedBlink && !g_rgbLedBlinkPhase);
  send("COLOR #0080ff"); assert(g_rgbLedBlink && !g_rgbLedBlinkPhase);
  send("BLINK OFF"); assert(!g_rgbLedBlink && g_rgbLedOn);
  send("OFF"); send("BLINK 100");
  now += 100; handleRgbLedBlink(); assert(!g_rgbLedBlinkPhase);
  send("OFF"); assert(!g_rgbLedOn && !g_rgbLedBlink);
  auto before = writes; now += 100; handleRgbLedBlink(); assert(writes == before);
  send("BLINK 100"); send("ON"); assert(g_rgbLedOn && !g_rgbLedBlink);
  send("STATUS"); assert(writes > before && g_rgbLedOn);
  now = UINT32_MAX - 49; send("BLINK 100");
  now = 50; handleRgbLedBlink(); assert(!g_rgbLedBlinkPhase);
  // Simulate a dashboard change, then a BLE command against the same state.
  setRgbLedMode(false, false, 1000);
  send("BLINK 60000"); assert(g_rgbLedBlink && g_rgbLedBlinkIntervalMs == 60000);
  send("BLINK OFF"); assert(!g_rgbLedBlink && !g_rgbLedOn);
  std::cout << "BLE LED state and timing tests passed\n";
}
'''
with tempfile.TemporaryDirectory() as tmp:
    cpp = Path(tmp) / 'test.cpp'
    exe = Path(tmp) / 'test'
    cpp.write_text(prelude + state + commands + timer + tests)
    subprocess.run(['g++', '-std=c++17', '-Wall', '-Wextra', '-I' + str(root / 'include'), str(cpp), '-o', str(exe)], check=True)
    subprocess.run([str(exe)], check=True)
