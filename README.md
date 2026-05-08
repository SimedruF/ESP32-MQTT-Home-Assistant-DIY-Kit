# ESP32 MQTT Home Assistant DIY Kit

> Developed by **Automatic House Systems**

Kit DIY complet pentru monitorizarea temperaturii, umidității, mișcării și controlul unui releu SSR, cu integrare nativă în **Home Assistant** prin **MQTT Auto-Discovery** și dashboard web configurat direct din browser.

---

## Cuprins

- [Caracteristici](#caracteristici)
- [Hardware necesar](#hardware-necesar)
- [Schema de cablaj](#schema-de-cablaj)
- [Instalare și configurare](#instalare-și-configurare)
- [Interfața web](#interfața-web)
- [MQTT și Home Assistant](#mqtt-și-home-assistant)
- [Structura proiectului](#structura-proiectului)
- [Build și upload](#build-și-upload)
- [Depanare](#depanare)

---

## Caracteristici

- **Senzor temperatură/umiditate** — DHT11 (KY-015), citire la fiecare 2 secunde
- **Senzor de mișcare PIR** — HC-SR501, detecție în timp real
- **Releu SSR** — control ON/OFF din interfața web sau prin MQTT
- **Display OLED 0.96"** — afișare status complet (temp, umiditate, mișcare, releu, WiFi, MQTT)
- **MQTT Auto-Discovery** — entitățile apar automat în Home Assistant fără configurare manuală
- **Dashboard web** — 4 tab-uri: monitorizare, configurare MQTT, configurare WiFi, informații board
- **Configurare WiFi din browser** — fără a rescrie firmware-ul
- **Configurare MQTT din browser** — broker/port/user/parolă salvate în memorie NVS
- **FreeRTOS** — citire senzori pe core 0, WiFi/MQTT/OLED pe core 1
- **Heartbeat LED** — indicator vizual că firmware-ul rulează

---

## Hardware necesar

| Componentă | Specificații | Cantitate |
|---|---|---|
| ESP32 | ESP-WROOM-32, 30 pini, USB Type-C, CH340 | 1 |
| Senzor DHT11 | Modul KY-015 cu rezistență pull-up inclusă | 1 |
| Senzor PIR | HC-SR501 | 1 |
| Releu SSR | Low-level trigger 5V (LOW = ON) | 1 |
| Display OLED | 0.96" SSD1306, I2C, 128×64 px | 1 |
| LED | Orice culoare + rezistență 220Ω | 1 |
| Sursă de alimentare | 5V/2A USB sau adaptor | 1 |

---

## Schema de cablaj

### ESP32 → DHT11 (KY-015)

| ESP32 | DHT11 KY-015 |
|---|---|
| GPIO4 | S (Data) |
| 3.3V | + (VCC) |
| GND | - (GND) |

> ⚠️ **Atenție:** Folosiți **3.3V** pentru alimentarea DHT11, nu 5V!

### ESP32 → PIR HC-SR501

| ESP32 | HC-SR501 |
|---|---|
| GPIO32 | OUT |
| 5V | VCC |
| GND | GND |

> PIR HC-SR501 necesită alimentare la **5V** pentru funcționare corectă.

### ESP32 → OLED SSD1306 (I2C)

| ESP32 | OLED SSD1306 |
|---|---|
| GPIO21 (SDA) | SDA |
| GPIO22 (SCL) | SCL |
| 3.3V | VCC |
| GND | GND |

### ESP32 → Releu SSR

| ESP32 | Releu SSR |
|---|---|
| GPIO23 | IN (semnal control) |
| 5V | VCC |
| GND | GND |

> Releul este **Low-Level trigger**: GPIO23 = LOW → releu ON; GPIO23 = HIGH → releu OFF.

### LED Heartbeat

| ESP32 | LED |
|---|---|
| GPIO18 | Anod (+) prin 220Ω |
| GND | Catod (-) |

---

## Instalare și configurare

### Pas 1 — Clonează și deschide proiectul

```bash
git clone https://github.com/SimedruF/ESP32-MQTT-Home-Assistant-DIY-Kit.git
cd "ESP32-MQTT-Home-Assistant-DIY-Kit"
```

Deschide folderul în **VS Code** cu extensia **PlatformIO IDE** instalată.

### Pas 2 — Build și upload firmware

```bash
# Build
~/.platformio/penv/bin/python3.12 ~/.platformio/penv/bin/pio run

# Upload (asigură-te că ESP32 este conectat pe /dev/ttyUSB0)
~/.platformio/penv/bin/python3.12 ~/.platformio/penv/bin/pio run --target upload
```

Sau folosește butoanele **Build** / **Upload** din bara PlatformIO în VS Code.

### Pas 3 — Configurare WiFi

La prima pornire (sau dacă credențialele WiFi nu sunt salvate), ESP32 creează un Access Point:

- **SSID:** `ESP32_HAKit`
- **Parolă:** `12345678`

Conectează-te la acest AP și accesează `http://192.168.4.1` → tab **WiFi** → introdu SSID și parola rețelei tale → **Salvează**.

### Pas 4 — Configurare MQTT

După conectarea la WiFi, accesează dashboard-ul ESP32 în browser (IP-ul afișat pe OLED sau în Serial Monitor).

Mergi la tab-ul **MQTT** și completează:
- **Broker IP** — adresa IP a serverului MQTT (ex: IP-ul Home Assistant)
- **Port** — implicit `1883`
- **Client ID** — implicit `esp32-ha-kit`
- **Username / Parolă** — dacă brokerul necesită autentificare

Apasă **Salvează** — ESP32 se reconectează automat cu noile setări.

---

## Interfața web

Accesează `http://<IP_ESP32>` din browser:

| Tab | Funcționalitate |
|---|---|
| **Dashboard** | Monitorizare live: temperatură, umiditate, mișcare, releu; control ON/OFF releu |
| **MQTT** | Configurare broker, port, autentificare; status conexiune |
| **WiFi** | Schimbare rețea WiFi; ștergere credențiale |
| **Board Info** | Informații tehnice: chip, frecvență, memorie, MAC, uptime |

Date actualizate automat la **2 secunde** fără reîncărcare pagină.

---

## MQTT și Home Assistant

### Entități create automat (Auto-Discovery)

| Entitate HA | Tip | Topic stare |
|---|---|---|
| `sensor.esp32kit_temperature` | Senzor temperatură (°C) | `esp32kit/state` |
| `sensor.esp32kit_humidity` | Senzor umiditate (%) | `esp32kit/state` |
| `binary_sensor.esp32kit_motion` | Senzor mișcare | `esp32kit/state` |
| `switch.esp32kit_relay` | Comutator releu | `esp32kit/relay/state` |

### Topice MQTT

| Topic | Direcție | Conținut |
|---|---|---|
| `esp32kit/state` | ESP32 → HA | JSON cu temperatură, umiditate, mișcare |
| `esp32kit/relay/state` | ESP32 → HA | `ON` / `OFF` |
| `esp32kit/relay/command` | HA → ESP32 | `ON` / `OFF` |

### Exemplu mesaj `esp32kit/state`

```json
{
  "temperature": 23.5,
  "humidity": 55.0,
  "motion": true
}
```

---

## Structura proiectului

```
ESP32 MQTT Home Assistant DIY Kit/
├── src/
│   └── main.cpp                 # Logica principală: senzori, MQTT, FreeRTOS, OLED
├── include/
│   └── WebPages.h               # Dashboard HTML embedded (4 tab-uri)
├── lib/
│   └── WiFiWebManager/          # Biblioteca custom: WiFi AP/STA + WebServer
│       ├── WiFiWebManager.h
│       └── WiFiWebManager.cpp
├── platformio.ini               # Configurare build PlatformIO
├── Ghid_Configurare_Client.html # Ghid HTML pentru client (printabil)
├── Ghid_Configurare_Client.pdf  # Ghid PDF generat
└── generate_pdf_advanced.sh     # Script generare PDF din HTML
```

---

## Build și upload

### Cerințe

- **VS Code** cu extensia **PlatformIO IDE**
- **Python 3.12** (inclus în PlatformIO venv)
- Driver **CH340** instalat (pentru Windows)
- Port serial: `/dev/ttyUSB0` (Linux) sau `COMx` (Windows)

### Librării (instalate automat de PlatformIO)

| Librărie | Versiune | Utilizare |
|---|---|---|
| `knolleary/PubSubClient` | ^2.8 | Client MQTT |
| `adafruit/DHT sensor library` | ^1.4.6 | Citire DHT11 |
| `adafruit/Adafruit SSD1306` | ^2.5.9 | Driver OLED |
| `adafruit/Adafruit GFX Library` | ^1.11.9 | Grafică OLED |
| `adafruit/Adafruit BusIO` | ^1.16.1 | Comunicație I2C/SPI |

### Comenzi utile

```bash
# Build
~/.platformio/penv/bin/python3.12 ~/.platformio/penv/bin/pio run

# Upload
~/.platformio/penv/bin/python3.12 ~/.platformio/penv/bin/pio run --target upload

# Monitor serial (115200 baud)
~/.platformio/penv/bin/python3.12 ~/.platformio/penv/bin/pio device monitor

# Generare PDF ghid client
bash generate_pdf_advanced.sh Ghid_Configurare_Client.html -o Ghid_Configurare_Client.pdf
```

---

## Depanare

| Problemă | Cauză probabilă | Soluție |
|---|---|---|
| `DHT11: citire invalida` | Cablaj GPIO4 sau tensiune incorectă | Verifică firul DATA pe GPIO4, VCC = 3.3V |
| `MQTT rc=-2` | Brokerul MQTT nu este accesibil | Setează IP-ul corect în tab-ul MQTT |
| OLED nu afișează nimic | Adresă I2C greșită sau cablaj | Verifică SDA=GPIO21, SCL=GPIO22, VCC=3.3V |
| PIR detectează mereu mișcare | Sensibilitate prea ridicată | Reglează potențiometrul de sensibilitate al HC-SR501 |
| Nu apare AP `ESP32_HAKit` | Credențiale WiFi salvate anterior | Apasă reset sau șterge NVS prin tab WiFi → Șterge credențiale |
| Entitățile nu apar în HA | MQTT Auto-Discovery dezactivat | Activează din HA: Setări → Dispozitive → MQTT → Activează Auto-Discovery |

---

## Licență

Proiect dezvoltat de **Automatic House Systems**.  
Distribuit pentru uz privat și educațional.
