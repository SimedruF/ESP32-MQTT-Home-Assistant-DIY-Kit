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
- **Multi-board** — profile PlatformIO pentru ESP32-WROOM-32, ESP32-C3, ESP32-C6, ESP32-C6 Super Mini, ESP32-S3 și LILYGO T-ZIGBEE
- **Inventar GPIO** — pini liberi, ocupați, doar-input, rezervați și pini de boot/USB
- **Pinout vizual** — desen simplificat al plăcii, cu ordinea pinilor și evidențierea funcțiilor selectate
- **FreeRTOS portabil** — funcționează pe variante ESP32 single-core și dual-core
- **Heartbeat LED** — indicator vizual că firmware-ul rulează

---

## Hardware necesar

| Componentă | Specificații | Cantitate |
|---|---|---|
| ESP32 | WROOM-32, C3-DevKitM-1, C6-DevKitC-1, C6 Super Mini, S3-DevKitC-1 sau LILYGO T-ZIGBEE | 1 |
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
| `lilygo-t-zigbee` | 4 | 5 | 6 | 7 | 8 | 1 |
| `esp32-c6` | 2 | 3 | 7 | 18 | 6 | 10 |
| `esp32-c6-supermini` | 0 | 1 | 2 | 3 | 20 | 19 |
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

# LILYGO T-ZIGBEE v1.2
pio run -e lilygo-t-zigbee -t upload

# ESP32-C6-DevKitC-1
pio run -e esp32-c6 -t upload

# ESP32-C6 Super Mini
pio run -e esp32-c6-supermini -t upload

# ESP32-S3-DevKitC-1-N8
pio run -e esp32-s3 -t upload
```

Profilul `esp32-s3` este pentru varianta N8 fără PSRAM. La modulele S3 cu memorie
Octal, GPIO35, GPIO36 și GPIO37 sunt folosiți intern și nu trebuie conectați la periferice.

Pentru `lilygo-t-zigbee`, selectează modul ESP32-C3 din comutatoarele DIP și folosește
adaptorul T-U2T pentru upload și log serial. Profilul pornește TLSR8258 prin GPIO0 și
protejează GPIO18/19, folosiți de UART-ul intern Zigbee. Firmware-ul principal rămâne
MQTT prin WiFi; biblioteca HCI pentru operarea rețelei Zigbee nu este încă activată.
GPIO8 este folosit ca SDA; rezistența pull-up I2C păstrează nivelul de boot necesar
acestui pin de strapping.

Configurația DIP pentru programarea ESP32-C3 este `3=ON`, `4=ON`, iar
`1=OFF`, `2=OFF`, `5=OFF`, cu marcajul `ON` al blocului DIP ca reper. Portul USB-C
al plăcii nu include un convertor USB-serial și nu se va enumera direct în sistem;
T-U2T trebuie conectat între placă și portul USB al calculatorului.

Ordinea fizică este `PC -> cablu USB de date -> mufa USB-C mamă T-U2T ->
conectorul USB-C tată T-U2T -> mufa USB-C a plăcii`. Pentru diagnostic, T-U2T
poate fi conectat la PC fără placă: adaptorul trebuie să apară în `lsusb`. Dacă
enumerarea eșuează deja în acest test, problema este adaptorul, cablul, contactul
USB-C sau portul/hub-ul USB, nu ESP32-C3 și nici poziția DIP.

Pentru `esp32-c6-supermini`, conectarea se face direct prin USB-C; placa folosește
interfața USB Serial/JTAG integrată și apare de regulă ca `/dev/ttyACM0`. La prima
programare, sau dacă upload-ul nu poate reseta automat placa, ține apăsat **BOOT**,
apasă și eliberează **RESET**, apoi eliberează **BOOT** și pornește imediat upload-ul.
Profilul evită intenționat comenzile RTS/DTR care eșuează pe unele combinații
Linux + USB Serial/JTAG. După terminarea upload-ului, apasă **RESET** pentru pornire.

Pe plăcile cu radio Zigbee, dashboard-ul afișează tab-ul **Comunicare IoT**.
Selecția `MQTT prin WiFi`, `Zigbee` sau `Thread` este salvată în NVS și este ascunsă
pe plăcile fără capabilități Zigbee. Interfața afișează separat modul selectat și
modul activ real. Zigbee și Thread necesită în continuare un firmware construit cu
stack-ul corespunzător; schimbarea selecției nu poate transforma firmware-ul MQTT
deja instalat într-un firmware Zigbee sau Thread.

Pentru Zigbee poate fi salvat și un profil de coordinator: generic, Home Assistant
ZHA, Zigbee2MQTT sau SONOFF ZBBridge-U. Adresa IP/numele local este folosit numai
pentru deschiderea portalului coordinatorului. Dispozitivele Zigbee nu aleg
coordinatorul prin IP; ele se alătură prin radio unei rețele aflate în `permit join`.
Nu există un mod radio separat de pairing „SONOFF”: opțiunea SONOFF selectează doar
instrucțiunile și portalul coordinatorului. Asocierea efectivă folosește procedura
Zigbee standard de network steering.

Selectorul de pairing are două moduri:

- **Automat** — încearcă rejoin la rețeaua salvată; dacă nu există una, caută o
  rețea Zigbee aflată în `permit join`.
- **Rețea nouă** — șterge asocierea Zigbee la următoarea pornire și caută din nou
  un coordinator. Cererea este resetată automat la **Automat** după utilizare.

În firmware-ul original, ZBBridge-U pornește asocierea din **+ Add Device** și
acceptă oficial numai dispozitive SONOFF și din ecosistemul eWeLink, deci un
endpoint Zigbee personalizat pe ESP32-C6 nu are compatibilitate garantată.

Firmware-ul Zigbee Router pentru ESP32-C6 SuperMini se compilează și se instalează
separat:

```bash
pio run -e esp32-c6-supermini-zigbee -t upload
```

Înainte de upload, salvează selecția și modul de pairing din firmware-ul web.
Upload-ul obișnuit păstrează NVS; nu folosi `erase`, deoarece acesta ar șterge
configurația. Activează apoi **Permit join** în ZHA/Zigbee2MQTT sau **+ Add Device**
în portalul ZBBridge-U și resetează placa.

Firmware-ul Zigbee publică atribute standard, fără MQTT și fără adresă IP:

- endpoint `10`: temperatură și umiditate DHT11, raportate la 30 de secunde;
- endpoint `11`: ocupare PIR, raportată la schimbarea stării;
- endpoint `12`: releu/priză, comandabil de coordinator prin clusterul On/Off.

După instalarea acestui target, pagina web nu mai este disponibilă. Pentru a
schimba din nou configurația din browser trebuie reinstalat targetul
`esp32-c6-supermini`, păstrând NVS.

Ținerea butonului **BOOT** apăsat timp de 5 secunde face factory reset Zigbee.
Nu conecta periferice pe GPIO12 și GPIO13, deoarece acestea sunt liniile USB D-/D+.
GPIO8 este rezervat LED-ului RGB onboard, iar GPIO9 este butonul BOOT.

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
| `t-zigbee` | LILYGO T-ZIGBEE v1.2, ESP32-C3 + TLSR8258 |
| `c6` | ESP32-C6-DevKitC-1 |
| `c6-supermini` | ESP32-C6 Super Mini |
| `s3` | ESP32-S3-DevKitC-1 |

Setup inițial:

```bash
cd esphome
./setup.sh init
```

Pe Windows, deschide PowerShell în directorul `esphome` și rulează:

```powershell
.\setup.ps1 init
```

Din Command Prompt poate fi folosit direct `setup.cmd init`. Este necesar Python
3.11 sau mai nou; scriptul creează automat mediul virtual și instalează
dependențele.

Scriptul detectează automat Python 3.11 sau mai nou, creează mediul local
`esphome/.venv`, instalează versiunea ESPHome din `requirements.txt` și generează
parola access point-ului și cheile API/OTA în `secrets.yaml`. Nu sunt solicitate
și nu sunt incluse credențiale pentru o rețea WiFi externă. Fișierul cu secrete
este exclus din Git. Build-urile sunt păstrate în
`~/.cache/esp32-ha-kit-esphome`, deoarece ESP-IDF nu acceptă spații în calea
directorului de compilare.

La prima pornire, firmware-ul ESPHome pornește numai în mod access point, cu
SSID-ul egal cu numele profilului, de exemplu `HA Kit C6 Super Mini`. După
conectarea la această rețea, deschide `http://192.168.4.1`, selectează rețeaua
WiFi a casei și introdu parola. ESPHome salvează credențialele în flash și se
conectează la router; access point-ul rămâne disponibil ca fallback dacă
legătura nu mai poate fi stabilită.

După conectarea la router, Native API, OTA și pagina web devin accesibile la
`http://<device_name>.local` sau la adresa IP atribuită prin DHCP. Firmware-ul
YAML nu conține credențialele rețelei casei.

Pentru configurarea completă AP-only a unei plăci ESP32-WROOM există scriptul
dedicat:

```bash
cd esphome
./setup-wroom-ap.sh /dev/ttyUSB0
```

Echivalentul Windows este:

```powershell
.\setup-wroom-ap.ps1 COM3
```

Din Command Prompt:

```bat
setup-wroom-ap.cmd COM3
```

Portul poate fi omis dacă este detectat un singur adaptor USB-serial. Când sunt
mai multe porturi COM, scriptul solicită alegerea explicită pentru a evita
programarea altei plăci.

Scriptul creează mediul ESPHome, generează `secrets.yaml`, validează, compilează
și programează profilul WROOM. După upload, conectează-te la
`ESP32 HA Kit WROOM` și configurează routerul din `http://192.168.4.1`. Dacă
există o singură placă `/dev/ttyUSB*`, portul poate fi omis. Scriptul generează
implicit parole și chei noi la fiecare rulare, astfel încât două plăci livrate
unor clienți diferiți să nu aibă aceleași credențiale. Opțiunea
`--reuse-secrets` pe Linux sau `-ReuseSecrets` pe Windows se folosește numai
când se repetă programarea aceleiași plăci.

La final este generat
`esphome/generated/esp32-ha-kit-wroom-setup.pdf`. Documentul conține ID-ul unic
al configurării, un QR
WiFi pentru conectarea la access point, un QR către
`http://esp32-ha-kit-wroom.local/`, cheia API, parola OTA și datele de
autentificare tipărite. PDF-ul conține secrete, are permisiuni `600`, este exclus
din Git și trebuie predat numai proprietarului plăcii. Permisiunea `600` este
aplicată pe sistemele care implementează permisiuni POSIX.

Portalul captiv ESPHome permite clientului să introducă singur SSID-ul și parola
rețelei sale; aceste date nu sunt cunoscute producătorului. Cheia API, parola OTA
și autentificarea `web_server` sunt însă valori incluse la compilare și nu pot fi
schimbate din portalul captiv standard. Pentru ca producătorul să nu mai cunoască
niciuna dintre credențialele finale, clientul trebuie să adopte sau să
recompileze dispozitivul în propriul ESPHome Device Builder. După instalarea
firmware-ului clientului, credențialele inițiale din PDF nu mai trebuie
considerate valide.

ESPHome folosește HTTP Basic Auth pentru pagina web. Parametrii
`?username=...&password=...` nu autentifică cererea. Forma
`http://username:password@host/` nu este folosită deoarece expune parola în URL
și nu funcționează consecvent în browserele moderne.

Comenzi uzuale:

```bash
# Validează toate profilele
./setup.sh check all

# Compilează un profil
./setup.sh compile wroom

# Prima instalare prin USB
./setup.sh run wroom /dev/ttyUSB0
./setup.sh run c6-supermini /dev/ttyACM0
./setup.sh run s3 /dev/ttyACM0

# Update OTA și loguri după prima instalare
./setup.sh upload c3 esp32-ha-kit-c3.local
./setup.sh logs c3 esp32-ha-kit-c3.local
```

Comenzile PowerShell au aceeași structură:

```powershell
.\setup.ps1 check all
.\setup.ps1 compile wroom
.\setup.ps1 run wroom COM3
.\setup.ps1 upload c6-supermini COM5
.\setup.ps1 logs c3 esp32-ha-kit-c3.local
```

Lista completă de comenzi este disponibilă cu `./setup.sh --help` pe
Linux/macOS sau `.\setup.ps1 help` pe Windows.

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
│   └── select_esp_usb_port.py   # Selectează porturile USB pentru C3/C6/T-ZIGBEE/S3
├── esphome/
│   ├── common.yaml              # Componente și integrare Home Assistant comune
│   ├── esp32-ha-kit-*.yaml      # Profile WROOM, C3, T-ZIGBEE, C6, C6 Super Mini și S3
│   ├── secrets.example.yaml     # Model pentru credențiale și chei
│   ├── requirements.txt         # Versiunea ESPHome utilizată
│   ├── setup.sh                 # Setup, validare, build, upload și loguri
│   ├── setup-wroom-ap.sh        # Instalare completă WROOM în mod AP-only
│   ├── setup.ps1 / setup.cmd    # Echivalent Windows pentru setup.sh
│   ├── setup-wroom-ap.ps1/.cmd  # Instalare completă WROOM pe Windows
│   └── generate-wroom-setup-pdf.py # Fisa PDF cu QR-uri si credentiale
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
pio run -e esp32-wroom -e esp32-c3 -e lilygo-t-zigbee -e esp32-c6 -e esp32-c6-supermini -e esp32-s3

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
| C6 Super Mini apare ca `/dev/ttyACM0`, dar upload-ul nu pornește | Placa nu a intrat manual în bootloader | Ține BOOT, apasă și eliberează RESET, eliberează BOOT, rulează `pio run -e esp32-c6-supermini -t upload`, apoi apasă RESET după scriere |
| T-ZIGBEE nu apare ca port serial | USB-C-ul plăcii este conectat direct, DIP-urile sunt greșite sau lipsește T-U2T | Folosește T-U2T și setează DIP 3/4 ON, DIP 1/2/5 OFF, apoi reconectează și apasă RESET |

---

## Licență

Proiect dezvoltat de **Automatic House Systems**.  
Distribuit pentru uz privat și educațional.
