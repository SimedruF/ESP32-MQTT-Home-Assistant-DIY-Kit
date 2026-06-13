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
- [Firmware ESPHome alternativ](#firmware-esphome-alternativ)
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
- **Dashboard web** — monitorizare, configurare MQTT/WiFi/hardware și informații despre placă
- **Configurare WiFi din browser** — fără a rescrie firmware-ul
- **Configurare MQTT din browser** — broker/port/user/parolă salvate în memorie NVS
- **Configurare hardware din browser** — GPIO pentru fiecare periferic, salvate în NVS
- **Multi-board** — profile PlatformIO pentru ESP32-WROOM-32, ESP32-C3, ESP32-C6 și ESP32-S3
- **Inventar GPIO** — pini liberi, ocupați, doar-input, rezervați și pini de boot/USB
- **Pinout vizual** — desen simplificat al plăcii, cu ordinea pinilor și evidențierea funcțiilor selectate
- **FreeRTOS portabil** — funcționează pe variante ESP32 single-core și dual-core
- **Heartbeat LED** — indicator vizual că firmware-ul rulează

---

## Hardware necesar

| Componentă | Specificații | Cantitate |
|---|---|---|
| ESP32 | WROOM-32, C3-DevKitM-1, C6-DevKitC-1 sau S3-DevKitC-1 | 1 |
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

Valorile de mai sus sunt profilul implicit `esp32-wroom`. Pentru celelalte medii:

| Mediu PlatformIO | DHT | PIR | Releu | Heartbeat | SDA | SCL |
|---|---:|---:|---:|---:|---:|---:|
| `esp32-wroom` | 4 | 32 | 23 | 18 | 21 | 22 |
| `esp32-c3` | 4 | 3 | 7 | 10 | 6 | 5 |
| `esp32-c6` | 2 | 3 | 7 | 18 | 6 | 10 |
| `esp32-s3` | 4 | 5 | 7 | 18 | 8 | 9 |

Pinii pot fi schimbați ulterior din tab-ul **Hardware**. Valoarea `-1`, afișată în interfață ca
**Dezactivat / neconfigurat**, oprește perifericul respectiv.

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
# ESP32-WROOM-32
pio run -e esp32-wroom -t upload

# ESP32-C3-DevKitM-1
pio run -e esp32-c3 -t upload

# ESP32-C6-DevKitC-1
pio run -e esp32-c6 -t upload

# ESP32-S3-DevKitC-1-N8
pio run -e esp32-s3 -t upload
```

Profilul `esp32-s3` este pentru varianta N8 fără PSRAM. La modulele S3 cu memorie
Octal, GPIO35, GPIO36 și GPIO37 sunt folosiți intern și nu trebuie conectați la periferice.

Sau folosește butoanele **Build** / **Upload** din bara PlatformIO în VS Code.

### Pas 3 — Configurare WiFi

La prima pornire (sau dacă credențialele WiFi nu sunt salvate), ESP32 creează un Access Point:

- **SSID:** `ESP32_HAKit`
- **Parolă:** `12345678`

Conectează-te la acest AP și accesează `http://192.168.4.1` → tab **WiFi** → introdu SSID și parola rețelei tale → **Salvează**.

### Pas 4 — Configurare MQTT

După conectarea la WiFi, accesează dashboard-ul la `http://esp32-ha-kit.local`.
Ca alternativă, folosește IP-ul afișat pe OLED, în Serial Monitor sau în lista
de clienți DHCP a routerului, unde placa apare cu hostname-ul `esp32-ha-kit`.

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
| **Hardware** | Configurare GPIO, pinout vizual, polaritate releu, adresă OLED și inventar pini |
| **Board Info** | Informații tehnice: chip, profil build, frecvență, memorie, MAC, uptime |

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

## Firmware ESPHome alternativ

Directorul `esphome/` conține un firmware ESPHome separat pentru aceleași componente:
DHT11, PIR, releu activ LOW, OLED SSD1306 și LED heartbeat. Integrarea cu Home
Assistant se face implicit prin ESPHome Native API, fără broker MQTT.

> Instalarea ESPHome înlocuiește firmware-ul PlatformIO existent pe placă. Cele două
> variante rămân disponibile în proiect, dar nu pot rula simultan pe același ESP32.

Profile disponibile:

| Profil script | Placă |
|---|---|
| `wroom` | ESP32-WROOM-32 / ESP32 Dev Module |
| `c3` | ESP32-C3-DevKitM-1 |
| `c6` | ESP32-C6-DevKitC-1 |
| `s3` | ESP32-S3-DevKitC-1 |

Setup inițial:

```bash
cd esphome
./setup.sh init
```

Scriptul detectează automat Python 3.11 sau mai nou, creează mediul local
`esphome/.venv`, instalează versiunea ESPHome din `requirements.txt`, solicită
datele WiFi și generează cheile API/OTA în `secrets.yaml`. Fișierul cu secrete
este exclus din Git. Build-urile sunt păstrate în
`~/.cache/esp32-ha-kit-esphome`, deoarece ESP-IDF nu acceptă spații în calea
directorului de compilare.

Comenzi uzuale:

```bash
# Validează toate profilele
./setup.sh check all

# Compilează un profil
./setup.sh compile wroom

# Prima instalare prin USB
./setup.sh run wroom /dev/ttyUSB0
./setup.sh run s3 /dev/ttyACM0

# Update OTA și loguri după prima instalare
./setup.sh upload c3 esp32-ha-kit-c3.local
./setup.sh logs c3 esp32-ha-kit-c3.local
```

Lista completă de comenzi este disponibilă cu `./setup.sh --help`.

---

## Structura proiectului

```
ESP32 MQTT Home Assistant DIY Kit/
├── src/
│   ├── main.cpp                 # Logica principală: senzori, MQTT, FreeRTOS, OLED
│   └── HardwareConfig.cpp       # Profiluri GPIO, validare și persistență NVS
├── include/
│   ├── WebPages.h               # Dashboard HTML embedded
│   └── HardwareConfig.h         # Modelul configurației hardware
├── scripts/
│   └── select_esp_usb_port.py   # Selectează și verifică porturile USB pentru C3/S3
├── esphome/
│   ├── common.yaml              # Componente și integrare Home Assistant comune
│   ├── esp32-ha-kit-*.yaml      # Profile WROOM, C3, C6 și S3
│   ├── secrets.example.yaml     # Model pentru credențiale și chei
│   ├── requirements.txt         # Versiunea ESPHome utilizată
│   └── setup.sh                 # Setup, validare, build, upload și loguri
├── lib/
│   └── WiFiWebManager/          # Biblioteca custom: WiFi AP/STA + WebServer
│       ├── WiFiWebManager.h
│       └── WiFiWebManager.cpp
├── platformio.ini               # Configurare build PlatformIO
├── Ghid_Configurare_Client.html # Ghid client în română (printabil)
├── Client_Configuration_Guide.html # Client guide in English (printable)
├── Ghid_Configurare_Client.pdf  # Ghid PDF generat
├── Client_Configuration_Guide.pdf # English client guide PDF
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
# Build profil implicit (ESP32-WROOM-32)
pio run

# Build toate profilele
pio run -e esp32-wroom -e esp32-c3 -e esp32-c6 -e esp32-s3

# Upload profil selectat
pio run -e esp32-c3 -t upload

# Monitor serial (115200 baud)
pio device monitor -b 115200

# Generare PDF ghid client
bash generate_pdf_advanced.sh Ghid_Configurare_Client.html -o Ghid_Configurare_Client.pdf

# Generare PDF ghid client în engleză
bash generate_pdf_advanced.sh Client_Configuration_Guide.html -o Client_Configuration_Guide.pdf
```

---

## Depanare

### Acces fără Serial Monitor

1. Încearcă `http://esp32-ha-kit.local` de pe un dispozitiv conectat la aceeași rețea.
2. Dacă placa nu s-a conectat la router, conectează-te la WiFi-ul `ESP32_HAKit`
   cu parola `12345678`, apoi deschide `http://192.168.4.1`.
3. În lista DHCP a routerului caută hostname-ul `esp32-ha-kit`.
4. Pentru ESP32-C3 și ESP32-S3, verifică porturile cu `pio device list`; placa
   poate apărea ca `/dev/ttyUSB0` printr-un bridge USB-to-UART sau ca
   `/dev/ttyACM0` prin USB nativ. Profilele C3/S3 preferă
   `/dev/serial/by-id/*`, iar scriptul verifică tipul cipului înainte de upload.
5. Pentru USB Serial/JTAG nativ pe ESP32-C3 sunt necesare împreună
   `ARDUINO_USB_MODE=1` și `ARDUINO_USB_CDC_ON_BOOT=1`. Activarea numai a
   `ARDUINO_USB_CDC_ON_BOOT` produce eroarea de build `USBSerial was not declared`.

| Problemă | Cauză probabilă | Soluție |
|---|---|---|
| `DHT11: citire invalida` | Cablaj GPIO4 sau tensiune incorectă | Verifică firul DATA pe GPIO4, VCC = 3.3V |
| `MQTT rc=-2` | Brokerul MQTT nu este accesibil | Setează IP-ul corect în tab-ul MQTT |
| OLED nu afișează nimic | Adresă I2C greșită sau cablaj | Verifică SDA=GPIO21, SCL=GPIO22, VCC=3.3V |
| Mesaje repetate `i2cWrite ESP_ERR_INVALID_STATE` | Magistrala I2C/OLED nu răspunde sau frecvența este schimbată în timpul transferurilor | Firmware-ul fixează I2C la 400 kHz și oprește refresh-ul OLED după primul probe eșuat; verifică și pinii/adresa din tab-ul Hardware |
| S3 nu are port de upload | Placa nu este enumerată prin USB | Scriptul selectează automat un port existent `/dev/ttyACM*` sau `/dev/ttyUSB*` și refuză `/dev/ttyS*`; folosește un cablu de date, conectare directă fără hub și verifică `pio device list` |
| PIR detectează mereu mișcare | Sensibilitate prea ridicată | Reglează potențiometrul de sensibilitate al HC-SR501 |
| Nu apare AP `ESP32_HAKit` | Credențiale WiFi salvate anterior | Apasă reset sau șterge NVS prin tab WiFi → Șterge credențiale |
| Entitățile nu apar în HA | MQTT Auto-Discovery dezactivat | Activează din HA: Setări → Dispozitive → MQTT → Activează Auto-Discovery |
| Configurația GPIO este respinsă | Pin inexistent, rezervat, neexpus sau folosit de două funcții | Consultă inventarul din tab-ul Hardware și selectează alt GPIO |
| C3: `USBSerial was not declared` | CDC activat fără modul USB Serial/JTAG | Păstrează împreună `-DARDUINO_USB_MODE=1` și `-DARDUINO_USB_CDC_ON_BOOT=1` în profilul `esp32-c3` |

---

## Licență

Proiect dezvoltat de **Automatic House Systems**.  
Distribuit pentru uz privat și educațional.
