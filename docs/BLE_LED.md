# Controlul LED-ului prin BLE

Firmware-ul WiFi/MQTT ofera control pentru LED-ul RGB onboard prin Bluetooth LE.
Nu controleaza LED-ul de alimentare sau iesirea separata de heartbeat.

## Activare

1. Incarca firmware-ul potrivit portului folosit:
   - USB–UART: `pio run -e esp32-c6-n16-uart -t upload`
   - USB nativ: `pio run -e esp32-c6-n16 -t upload`
2. Reincarca dashboardul cu Ctrl+F5.
3. In **Bluetooth LE — Control LED**, bifeaza **Activeaza BLE**.
4. Alege un nume de 1–20 caractere: litere ASCII, cifre, `-` sau `_`.
5. Apasa **Salveaza BLE si reporneste**. Setarile se pastreaza in NVS.

BLE este dezactivat implicit. Activarea, dezactivarea si schimbarea numelui
necesita repornirea placii. WiFi si dashboardul raman disponibile dupa repornire.
Controlul de test nu cere PIN sau asociere: un telefon din apropiere poate
controla LED-ul cat timp BLE este activ. Dezactiveaza BLE din acelasi formular.

## Test de pe telefon

Foloseste [nRF Connect for Mobile](https://www.nordicsemi.com/Products/Development-tools/nRF-Connect-for-mobile)
pe Android sau iOS. Conectarea se face din aplicatie, prin BLE.

1. Scaneaza si conecteaza-te la `ESP32-HA-Kit` sau la numele configurat.
2. Deschide serviciul Nordic UART, UUID `6e400001-b5a3-f393-e0a9-e50e24dcca9e`.
3. Optional, activeaza notificarile caracteristicii TX.
4. In RX, selecteaza **Write request**, format **Text / UTF-8**, si trimite `ON`.
5. Trimite `COLOR #FF0000`, apoi `BLINK 500` pentru blink rosu.
6. Trimite `OFF` pentru oprire.

| Caracteristica | UUID | Utilizare |
| --- | --- | --- |
| RX | `6e400002-b5a3-f393-e0a9-e50e24dcca9e` | Write: o comanda text |
| TX | `6e400003-b5a3-f393-e0a9-e50e24dcca9e` | Read / Notify: raspuns scurt |
| State | `b71c0001-14af-4f17-bd8f-626544da8310` | Read: starea LED-ului in JSON |

| Comanda | Efect |
| --- | --- |
| `ON` | Aprinde LED-ul cu culoarea/luminozitatea selectate si opreste blink-ul |
| `OFF` | Stinge LED-ul si opreste blink-ul |
| `COLOR #FF0000` | Selecteaza rosu; nu modifica ON/OFF sau blink |
| `BRIGHT 25` | Luminozitate 0–100%; nu modifica ON/OFF sau blink |
| `BLINK 500` | Porneste blink: 500 ms aprins, 500 ms stins; interval permis 100–60000 ms |
| `BLINK OFF` | Opreste blink-ul si revine la starea ON/OFF anterioara |
| `STATUS` | Actualizeaza State si confirma cu OK; citeste State pentru JSON |

O comanda per scriere, maximum 31 bytes. Comenzile documentate incap in cei
20 bytes disponibili la MTU implicit. Sunt acceptate litere mici si CR/LF la final;
nu trimite mai multe comenzi in acelasi write. La inceput LED-ul este stins,
culoarea este `#0080FF`, luminozitatea 25%. Culoarea neagra sau luminozitatea 0
nu produc lumina, chiar daca starea logica este ON.

TX trimite `OK` dupa executie, `ERR command` pentru format/valoare invalida sau
`ERR busy` daca se umple coada. Asteapta raspunsul inainte de urmatoarea comanda.
`STATE changed` indica o schimbare din dashboard: citeste State pentru detalii.
Notificarile sunt scurte; JSON-ul se citeste separat pentru a evita trunchierea la
MTU implicit. `on` din JSON reprezinta starea constanta, iar `blink` modul de test,
nu faza instantanee de aprindere/stingere.

Dashboardul actualizeaza starea aproximativ la 3 secunde, cand nu editezi cardul
LED. Comenzile BLE sunt executate in loop(), printr-o coada, pentru a evita
scrierile concurente cu comenzile web. Dupa deconectarea telefonului, placa reia
advertising-ul. Starea LED-ului/blink nu se salveaza si revine la implicit la reboot.
BLE nu expune configurarea WiFi, parolele MQTT sau comenzi pentru releu.

## Verificari fara placa

```bash
g++ -std=c++17 -Iinclude test/ble_led_commands.cpp -o /tmp/ble-led-test
/tmp/ble-led-test
pio run -e esp32-c6-n16 -e esp32-c6-n16-uart
```

Testul radio necesita o placa si un telefon: conectare, comenzi valide/invalide,
reconectare, sincronizare cu dashboardul si dezactivare BLE urmata de reboot.
