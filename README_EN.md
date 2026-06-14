# ESP32 MQTT Home Assistant DIY Kit

> Developed by **Automatic House Systems**

[Romanian version](README.md)

[English client guide HTML](Client_Configuration_Guide.html) |
[English client guide PDF](Client_Configuration_Guide.pdf)

A complete DIY kit for temperature, humidity and motion monitoring and SSR relay
control, with native **Home Assistant** integration through **MQTT
Auto-Discovery** and a web dashboard configured directly from a browser.

---

## Contents

- [Features](#features)
- [Required hardware](#required-hardware)
- [Wiring](#wiring)
- [Installation and configuration](#installation-and-configuration)
- [Web interface](#web-interface)
- [MQTT and Home Assistant](#mqtt-and-home-assistant)
- [Alternative ESPHome firmware](#alternative-esphome-firmware)
- [Project structure](#project-structure)
- [Build and upload](#build-and-upload)
- [Troubleshooting](#troubleshooting)

---

## Features

- **Temperature/humidity sensor** - DHT11 (KY-015), read every 2 seconds
- **PIR motion sensor** - HC-SR501, real-time detection
- **SSR relay** - ON/OFF control from the web interface or MQTT
- **0.96-inch OLED display** - temperature, humidity, motion, relay, Wi-Fi and MQTT status
- **MQTT Auto-Discovery** - entities appear automatically in Home Assistant
- **Web dashboard** - monitoring, MQTT/Wi-Fi/hardware configuration and board information
- **Browser Wi-Fi configuration** - no firmware rebuild required
- **Browser MQTT configuration** - broker, port, username and password stored in NVS
- **Browser hardware configuration** - GPIO assignments stored in NVS
- **Multiple boards** - PlatformIO profiles for ESP32-WROOM-32, ESP32-C3,
  ESP32-C6, ESP32-C6 Super Mini, ESP32-S3 and LILYGO T-ZIGBEE
- **GPIO inventory** - free, occupied, input-only, reserved, boot and USB pins
- **Visual pinout** - simplified board drawing with selected functions highlighted
- **Portable FreeRTOS implementation** - supports single-core and dual-core ESP32 variants
- **Heartbeat LED** - visual indication that the firmware is running

---

## Required hardware

| Component | Specification | Quantity |
|---|---|---:|
| ESP32 | WROOM-32, C3-DevKitM-1, C6-DevKitC-1, C6 Super Mini, S3-DevKitC-1 or LILYGO T-ZIGBEE | 1 |
| DHT11 sensor | KY-015 module with pull-up resistor | 1 |
| PIR sensor | HC-SR501 | 1 |
| SSR relay | 5 V low-level trigger module (LOW = ON) | 1 |
| OLED display | 0.96-inch SSD1306, I2C, 128x64 px | 1 |
| LED | Any color with a 220 ohm resistor | 1 |
| Power supply | 5 V/2 A USB supply or adapter | 1 |

---

## Wiring

### ESP32 to DHT11 (KY-015)

| ESP32 | DHT11 KY-015 |
|---|---|
| GPIO4 | S (Data) |
| 3.3 V | + (VCC) |
| GND | - (GND) |

> **Warning:** Power the DHT11 from **3.3 V**, not 5 V.

### ESP32 to PIR HC-SR501

| ESP32 | HC-SR501 |
|---|---|
| GPIO32 | OUT |
| 5 V | VCC |
| GND | GND |

The HC-SR501 normally requires a **5 V** supply.

### ESP32 to OLED SSD1306 (I2C)

| ESP32 | OLED SSD1306 |
|---|---|
| GPIO21 (SDA) | SDA |
| GPIO22 (SCL) | SCL |
| 3.3 V | VCC |
| GND | GND |

### ESP32 to SSR relay

| ESP32 | SSR relay |
|---|---|
| GPIO23 | IN (control signal) |
| 5 V | VCC |
| GND | GND |

The relay uses **low-level triggering**: GPIO23 LOW turns it on and GPIO23 HIGH
turns it off.

### Heartbeat LED

| ESP32 | LED |
|---|---|
| GPIO18 | Anode (+) through 220 ohm |
| GND | Cathode (-) |

The values above belong to the default `esp32-wroom` profile. Other environments
use these defaults:

| PlatformIO environment | DHT | PIR | Relay | Heartbeat | SDA | SCL |
|---|---:|---:|---:|---:|---:|---:|
| `esp32-wroom` | 4 | 32 | 23 | 18 | 21 | 22 |
| `esp32-c3` | 4 | 3 | 7 | 10 | 6 | 5 |
| `lilygo-t-zigbee` | 4 | 5 | 6 | 7 | 8 | 1 |
| `esp32-c6` | 2 | 3 | 7 | 18 | 6 | 10 |
| `esp32-c6-supermini` | 0 | 1 | 2 | 3 | 20 | 19 |
| `esp32-s3` | 4 | 5 | 7 | 18 | 8 | 9 |

Pins can later be changed from the **Hardware** tab. A value of `-1`, displayed
as **Disabled / not configured**, disables that peripheral.

---

## Installation and configuration

### Step 1 - Clone and open the project

```bash
git clone https://github.com/SimedruF/ESP32-MQTT-Home-Assistant-DIY-Kit.git
cd "ESP32-MQTT-Home-Assistant-DIY-Kit"
```

Open the folder in **VS Code** with the **PlatformIO IDE** extension installed.

### Step 2 - Build and upload the firmware

```bash
# ESP32-WROOM-32
pio run -e esp32-wroom -t upload

# ESP32-C3-DevKitM-1
pio run -e esp32-c3 -t upload

# LILYGO T-ZIGBEE v1.2
pio run -e lilygo-t-zigbee -t upload

# ESP32-C6-DevKitC-1
pio run -e esp32-c6 -t upload

# ESP32-C6 Super Mini
pio run -e esp32-c6-supermini -t upload

# ESP32-S3-DevKitC-1-N8
pio run -e esp32-s3 -t upload
```

The `esp32-s3` profile targets the N8 version without PSRAM. On S3 modules with
Octal memory, GPIO35, GPIO36 and GPIO37 are used internally and must not be
connected to peripherals.

For `lilygo-t-zigbee`, select the ESP32-C3 side with the DIP switches and use a
T-U2T adapter for upload and serial logging. The profile powers the TLSR8258
through GPIO0 and protects GPIO18/GPIO19, which are used by the internal Zigbee
UART. The primary firmware still uses MQTT over Wi-Fi; the HCI library required
to operate the Zigbee network is not enabled. GPIO8 is used as SDA, and the I2C
pull-up preserves the boot level required by this strapping pin.

The DIP switch configuration for programming the ESP32-C3 is `3=ON`, `4=ON`,
with `1=OFF`, `2=OFF` and `5=OFF`. Use the `ON` marking on the switch block as
the reference. The board USB-C connector does not contain a USB-to-serial
converter and will not enumerate directly. The T-U2T adapter must be installed
between the board and the computer.

The physical connection order is:

```text
PC -> USB data cable -> T-U2T female USB-C connector
   -> T-U2T male USB-C connector -> board USB-C connector
```

For diagnostics, connect the T-U2T to the PC without the board. The adapter must
appear in `lsusb`. If enumeration already fails, the problem is the adapter,
cable, USB-C contact or USB port/hub, not the ESP32-C3 or DIP switch positions.

The `esp32-c6-supermini` connects directly through USB-C and uses the integrated
USB Serial/JTAG interface. On Linux it normally appears as `/dev/ttyACM0`. For
the first upload, or when automatic reset fails, hold **BOOT**, press and
release **RESET**, release **BOOT**, then immediately start the upload. The
profile intentionally avoids RTS/DTR reset commands that fail on some Linux
USB Serial/JTAG combinations. Press **RESET** after upload to start the board.

Boards with a Zigbee radio show the **IoT Communication** tab. The `MQTT over
Wi-Fi`, `Zigbee` or `Thread` selection is stored in NVS and hidden on boards
without Zigbee capability. The interface separately reports the selected mode
and the mode that is actually running. Zigbee and Thread require firmware built
with the corresponding protocol stack; changing the selection cannot transform
an installed MQTT firmware image into Zigbee or Thread firmware.

A Zigbee coordinator profile can also be stored: generic, Home Assistant ZHA,
Zigbee2MQTT or SONOFF ZBBridge-U. The IP address or local hostname is used only
to open the coordinator portal. Zigbee devices do not select a coordinator by
IP address; they join by radio while a network has `permit join` enabled. There
is no separate SONOFF radio pairing mode. The SONOFF option only selects the
instructions and coordinator portal. Actual association uses standard Zigbee
network steering.

The pairing selector provides two modes:

- **Automatic** - rejoins the stored network or searches for a Zigbee network
  with `permit join` enabled.
- **New network** - clears the Zigbee association on the next boot and searches
  for a coordinator again. The request automatically returns to **Automatic**
  after use.

With its original firmware, ZBBridge-U starts pairing through **+ Add Device**
and officially supports SONOFF and eWeLink ecosystem devices. Compatibility
with a custom ESP32-C6 Zigbee endpoint is therefore not guaranteed.

The ESP32-C6 Super Mini Zigbee Router firmware is built and installed separately:

```bash
pio run -e esp32-c6-supermini-zigbee -t upload
```

Before uploading it, save the selection and pairing mode in the web firmware.
A normal upload preserves NVS; do not use `erase`, because it removes the
configuration. Enable **Permit join** in ZHA/Zigbee2MQTT or **+ Add Device** in
the ZBBridge-U portal, then reset the board.

The Zigbee firmware publishes standard attributes without MQTT or an IP address:

- endpoint `10`: DHT11 temperature and humidity, reported every 30 seconds;
- endpoint `11`: PIR occupancy, reported when the state changes;
- endpoint `12`: relay/outlet controlled through the On/Off cluster.

The web page is not available after installing this target. To use browser
configuration again, reinstall the `esp32-c6-supermini` target while preserving
NVS.

Holding **BOOT** for 5 seconds performs a Zigbee factory reset. Do not attach
peripherals to GPIO12 or GPIO13 because they are the USB D-/D+ lines. GPIO8 is
reserved for the onboard RGB LED and GPIO9 is the BOOT button.

The **Build** and **Upload** buttons in the PlatformIO toolbar can be used
instead of the commands.

### Step 3 - Configure Wi-Fi

On first boot, or when no Wi-Fi credentials are stored, the board creates an
access point:

- **SSID:** `ESP32_HAKit`
- **Password:** `12345678`

Connect to this network, open `http://192.168.4.1`, select the **WiFi** tab,
enter the router SSID and password, then save the settings.

### Step 4 - Configure MQTT

After the board joins Wi-Fi, open `http://esp32-ha-kit.local`. Alternatively,
use the IP displayed on the OLED, in Serial Monitor or in the router DHCP client
list, where the hostname is `esp32-ha-kit`.

Open the **MQTT** tab and enter:

- **Broker IP** - address of the MQTT server, usually the Home Assistant host;
- **Port** - `1883` by default;
- **Client ID** - `esp32-ha-kit` by default;
- **Username / Password** - when authentication is enabled on the broker.

Press **Save**. The ESP32 reconnects using the new settings.

---

## Web interface

Open `http://<ESP32_IP>` in a browser:

| Tab | Function |
|---|---|
| **Dashboard** | Live temperature, humidity, motion and relay state; relay ON/OFF control |
| **MQTT** | Broker, port and authentication settings; connection state |
| **WiFi** | Change the network or clear stored credentials |
| **Hardware** | GPIO configuration, visual pinout, relay polarity, OLED address and pin inventory |
| **Board Info** | Chip, build profile, frequency, memory, MAC address, uptime and capabilities |

Data is refreshed every **2 seconds** without reloading the page.

---

## MQTT and Home Assistant

### Automatically created entities

| Home Assistant entity | Type | State topic |
|---|---|---|
| `sensor.esp32kit_temperature` | Temperature sensor (deg C) | `esp32kit/state` |
| `sensor.esp32kit_humidity` | Humidity sensor (%) | `esp32kit/state` |
| `binary_sensor.esp32kit_motion` | Motion sensor | `esp32kit/state` |
| `switch.esp32kit_relay` | Relay switch | `esp32kit/relay/state` |

### MQTT topics

| Topic | Direction | Content |
|---|---|---|
| `esp32kit/state` | ESP32 -> HA | JSON with temperature, humidity and motion |
| `esp32kit/relay/state` | ESP32 -> HA | `ON` / `OFF` |
| `esp32kit/relay/command` | HA -> ESP32 | `ON` / `OFF` |

Example `esp32kit/state` message:

```json
{
  "temperature": 23.5,
  "humidity": 55.0,
  "motion": true
}
```

---

## Alternative ESPHome firmware

The `esphome/` directory contains separate ESPHome firmware for the same
components: DHT11, PIR, active-LOW relay, SSD1306 OLED and heartbeat LED. Home
Assistant integration uses the ESPHome Native API by default, without an MQTT
broker.

> Installing ESPHome replaces the PlatformIO firmware on the board. Both
> variants remain in the project, but they cannot run simultaneously.

Available profiles:

| Script profile | Board |
|---|---|
| `wroom` | ESP32-WROOM-32 / ESP32 Dev Module |
| `c3` | ESP32-C3-DevKitM-1 |
| `t-zigbee` | LILYGO T-ZIGBEE v1.2, ESP32-C3 + TLSR8258 |
| `c6` | ESP32-C6-DevKitC-1 |
| `c6-supermini` | ESP32-C6 Super Mini |
| `s3` | ESP32-S3-DevKitC-1 |

The complete ESPHome procedure for C6 Super Mini is also available in the
[client guide](Client_Configuration_Guide.html#esphome-c6).

Initial setup on Linux/macOS:

```bash
cd esphome
./setup.sh init
```

Initial setup on Windows PowerShell:

```powershell
cd esphome
.\setup.ps1 init
```

From Command Prompt, run `setup.cmd init`. Python 3.11 or newer is required.
The scripts create the virtual environment and install all dependencies.

The setup detects a compatible Python installation, creates `esphome/.venv`,
installs the ESPHome version pinned in `requirements.txt`, and generates the
access point password and API/OTA credentials in `secrets.yaml`. Router Wi-Fi
credentials are not requested or embedded. The secrets file is excluded from
Git. Builds are stored outside the project because ESP-IDF does not support
spaces in the build directory path.

On first boot, the ESPHome firmware starts in access point mode. Its SSID matches
the friendly profile name, for example `HA Kit C6 Super Mini`. Connect to it,
open `http://192.168.4.1`, select the customer's 2.4 GHz Wi-Fi network and enter
its password. ESPHome stores these credentials in flash and joins the router.
The access point remains available as a fallback when the connection fails.

After joining the router, the Native API, OTA service and web page are available
at `http://<device_name>.local` or at the DHCP address. The YAML firmware does
not contain the customer's router credentials.

### ESP32-C6 Super Mini with ESPHome

Default pins:

| Function | GPIO |
|---|---:|
| DHT11 | 0 |
| PIR | 1 |
| Active-LOW relay | 2 |
| External heartbeat LED | 3 |
| OLED SDA | 20 |
| OLED SCL | 19 |

Linux installation:

```bash
cd esphome
./setup.sh check c6-supermini
./setup.sh run c6-supermini /dev/ttyACM0
```

Windows installation:

```powershell
cd esphome
.\setup.ps1 check c6-supermini
.\setup.ps1 run c6-supermini COM5
```

After provisioning Wi-Fi, add the device in Home Assistant from **Settings ->
Devices & services -> Add integration -> ESPHome**. Use
`esp32-ha-kit-c6-supermini.local` or its IP address, then enter the
`api_encryption_key` value from `secrets.yaml`.

The local web page uses `web_username` and `web_password`. OTA updates use
`ota_password`. These values are compiled into the firmware and cannot be
changed through the standard captive portal. To give the customer exclusive
control, they must adopt or rebuild the board in their own ESPHome Device
Builder with their own credentials.

### Complete WROOM AP-only provisioning

Linux:

```bash
cd esphome
./setup-wroom-ap.sh /dev/ttyUSB0
```

Windows PowerShell:

```powershell
.\setup-wroom-ap.ps1 COM3
```

Windows Command Prompt:

```bat
setup-wroom-ap.cmd COM3
```

The port can be omitted when a single USB serial adapter is detected. When
multiple ports are available, specify the correct one to prevent flashing a
different board.

The WROOM script generates new credentials by default for every board. Use
`--reuse-secrets` on Linux or `-ReuseSecrets` on Windows only when retrying the
same board.

It also generates:

```text
esphome/generated/esp32-ha-kit-wroom-setup.pdf
```

The PDF contains the provisioning ID, Wi-Fi access point QR code, web page QR
code, API key, OTA password and web credentials. It contains secrets, is
excluded from Git and must only be given to the board owner. POSIX systems apply
file mode `600`.

The ESPHome captive portal configures Wi-Fi only. API encryption, OTA and web
authentication are compile-time settings. URL query parameters such as
`?username=...&password=...` do not authenticate ESPHome HTTP Basic Auth, and
embedded URL credentials are intentionally not used.

Common commands:

```bash
# Validate all profiles
./setup.sh check all

# Compile a profile
./setup.sh compile wroom

# Initial USB installation
./setup.sh run wroom /dev/ttyUSB0
./setup.sh run c6-supermini /dev/ttyACM0
./setup.sh run s3 /dev/ttyACM0

# OTA update and logs
./setup.sh upload c3 esp32-ha-kit-c3.local
./setup.sh logs c3 esp32-ha-kit-c3.local
```

PowerShell uses the same command structure:

```powershell
.\setup.ps1 check all
.\setup.ps1 compile wroom
.\setup.ps1 run wroom COM3
.\setup.ps1 upload c6-supermini COM5
.\setup.ps1 logs c3 esp32-ha-kit-c3.local
```

Run `./setup.sh --help` on Linux/macOS or `.\setup.ps1 help` on Windows for the
complete command list.

---

## Project structure

```text
ESP32 MQTT Home Assistant DIY Kit/
|-- src/
|   |-- main.cpp
|   |-- HardwareConfig.cpp
|   |-- SerialLog.cpp
|   `-- zigbee_main.cpp
|-- include/
|   |-- WebPages.h
|   |-- HardwareConfig.h
|   `-- SerialLog.h
|-- scripts/
|   `-- select_esp_usb_port.py
|-- esphome/
|   |-- common.yaml
|   |-- esp32-ha-kit-*.yaml
|   |-- secrets.example.yaml
|   |-- requirements.txt
|   |-- setup.sh
|   |-- setup-wroom-ap.sh
|   |-- setup.ps1 / setup.cmd
|   |-- setup-wroom-ap.ps1 / setup-wroom-ap.cmd
|   `-- generate-wroom-setup-pdf.py
|-- lib/
|   `-- WiFiWebManager/
|-- platformio.ini
|-- README_EN.md
|-- Ghid_Configurare_Client.html
|-- Client_Configuration_Guide.html
|-- Ghid_Configurare_Client.pdf
|-- Client_Configuration_Guide.pdf
`-- generate_pdf_advanced.sh
```

---

## Build and upload

### Requirements

- **VS Code** with the **PlatformIO IDE** extension
- **Python 3.12** from the PlatformIO environment
- **CH340** driver when required on Windows
- Serial port such as `/dev/ttyUSB0`, `/dev/ttyACM0` or `COMx`

### Libraries installed by PlatformIO

| Library | Version | Purpose |
|---|---|---|
| `knolleary/PubSubClient` | ^2.8 | MQTT client |
| `adafruit/DHT sensor library` | ^1.4.6 | DHT11 readings |
| `adafruit/Adafruit SSD1306` | ^2.5.9 | OLED driver |
| `adafruit/Adafruit GFX Library` | ^1.11.9 | Graphics |
| `adafruit/Adafruit BusIO` | ^1.16.1 | I2C/SPI communication |

Useful commands:

```bash
# Build the default ESP32-WROOM-32 profile
pio run

# Build all standard profiles
pio run -e esp32-wroom -e esp32-c3 -e lilygo-t-zigbee \
  -e esp32-c6 -e esp32-c6-supermini -e esp32-s3

# Upload a selected profile
pio run -e esp32-c3 -t upload

# Serial monitor at 115200 baud
pio device monitor -b 115200

# Generate the Romanian client PDF
bash generate_pdf_advanced.sh Ghid_Configurare_Client.html -o Ghid_Configurare_Client.pdf

# Generate the English client PDF
bash generate_pdf_advanced.sh Client_Configuration_Guide.html -o Client_Configuration_Guide.pdf
```

---

## Troubleshooting

### Access without Serial Monitor

1. Open `http://esp32-ha-kit.local` from a device on the same network.
2. If the board has not joined the router, connect to `ESP32_HAKit` with password
   `12345678`, then open `http://192.168.4.1`.
3. Find the `esp32-ha-kit` hostname in the router DHCP client list.
4. For ESP32-C3 and ESP32-S3, inspect ports with `pio device list`. The board
   can appear as `/dev/ttyUSB0` through a USB-to-UART bridge or `/dev/ttyACM0`
   through native USB.
5. Native USB Serial/JTAG on ESP32-C3 requires both `ARDUINO_USB_MODE=1` and
   `ARDUINO_USB_CDC_ON_BOOT=1`. Enabling only CDC causes the
   `USBSerial was not declared` build error.

| Problem | Likely cause | Solution |
|---|---|---|
| `DHT11: invalid reading` | Incorrect data pin or supply | Check the selected GPIO and use 3.3 V |
| `MQTT rc=-2` | Broker is not reachable | Enter the correct broker IP in the MQTT tab |
| OLED remains blank | Incorrect I2C address or wiring | Check SDA, SCL, 3.3 V and address `0x3C` |
| Repeated `i2cWrite ESP_ERR_INVALID_STATE` | OLED/I2C bus does not respond | Check wiring, pins and address; firmware disables refresh after a failed probe |
| S3 upload port is missing | Board is not enumerated | Use a data cable, connect without a hub and inspect `pio device list` |
| PIR always reports motion | Sensitivity is too high | Adjust the HC-SR501 sensitivity potentiometer |
| `ESP32_HAKit` AP is missing | Wi-Fi credentials are already stored | Check the router or clear credentials from the WiFi tab |
| Home Assistant entities are missing | MQTT Auto-Discovery is disabled | Enable MQTT discovery in Home Assistant |
| GPIO configuration is rejected | Pin is invalid, reserved or duplicated | Select a different pin using the hardware inventory |
| C3: `USBSerial was not declared` | CDC enabled without USB Serial/JTAG mode | Keep both USB build definitions enabled |
| C6 Super Mini appears as `/dev/ttyACM0` but upload fails | Board is not in download mode | Use the BOOT + RESET sequence, upload, then press RESET |
| ESPHome C6 is not discovered | mDNS or API key issue | Try the DHCP IP and enter the exact `api_encryption_key` |
| T-ZIGBEE has no serial port | Missing T-U2T or incorrect DIP switches | Use T-U2T and set DIP 3/4 ON, 1/2/5 OFF |

---

## License

Developed by **Automatic House Systems**.  
Distributed for private and educational use.
