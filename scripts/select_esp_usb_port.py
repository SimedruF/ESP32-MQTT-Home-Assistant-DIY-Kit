Import("env")

import glob
import os
import subprocess
import sys
from SCons.Script import COMMAND_LINE_TARGETS


def serial_ports():
    candidates = []

    # Prefer persistent links for Espressif native USB Serial/JTAG.
    for path in sorted(glob.glob("/dev/serial/by-id/*")):
        real_path = os.path.realpath(path)
        name = os.path.basename(path).lower()
        if real_path.startswith("/dev/ttyACM") and (
            "espressif" in name or "usb_jtag" in name
        ):
            candidates.append(path)

    # Fall back to other USB serial devices only when no persistent Espressif
    # link exists. Internal /dev/ttyS* ports are never considered.
    if candidates:
        return candidates

    for pattern in ("/dev/ttyACM*", "/dev/ttyUSB*"):
        for path in sorted(glob.glob(pattern)):
            candidates.append(path)

    return candidates


ports = serial_ports()
is_upload = any(
    target == "upload" or target.startswith("upload")
    for target in COMMAND_LINE_TARGETS
)
is_serial_upload = is_upload and env.subst("$UPLOAD_PROTOCOL") == "esptool"
profile = env.subst("$PIOENV")
label = "[%s]" % profile
is_c6_supermini = profile in (
    "esp32-c6-supermini",
    "esp32-c6-supermini-zigbee",
)

if is_serial_upload and ports:
    expected_chip = {
        "esp32-c3": "ESP32-C3",
        "lilygo-t-zigbee": "ESP32-C3",
        "esp32-c6-supermini": "ESP32-C6",
        "esp32-c6-supermini-zigbee": "ESP32-C6",
        "esp32-s3": "ESP32-S3",
    }.get(profile)
    esptool_dir = env.PioPlatform().get_package_dir("tool-esptoolpy")
    esptool = os.path.join(esptool_dir, "esptool.py") if esptool_dir else ""
    matching_ports = []

    if (
        expected_chip
        and not is_c6_supermini
        and os.path.isfile(esptool)
    ):
        for port in ports:
            try:
                result = subprocess.run(
                    [sys.executable, esptool, "--port", port, "chip-id"],
                    stdout=subprocess.PIPE,
                    stderr=subprocess.STDOUT,
                    text=True,
                    timeout=20,
                    check=False,
                )
                if expected_chip in result.stdout:
                    matching_ports.append(port)
                elif result.returncode != 0:
                    last_line = result.stdout.strip().splitlines()[-1:]
                    if last_line:
                        print("%s Detectia cipului a esuat: %s" % (label, last_line[0]))
            except (OSError, subprocess.TimeoutExpired):
                pass
        if matching_ports:
            ports = matching_ports
        else:
            ports = []
    elif is_c6_supermini:
        print(
            "%s Se foloseste resetarea USB Serial/JTAG pentru intrarea automata "
            "in bootloader." % label
        )

if ports:
    env.Replace(MONITOR_PORT=ports[0])
    if is_serial_upload:
        env.Replace(UPLOAD_PORT=ports[0])
        print("%s Port upload si monitor selectat automat: %s" % (label, ports[0]))
    else:
        print("%s Port monitor selectat automat: %s" % (label, ports[0]))
elif is_serial_upload:
    print()
    print("%s EROARE: placa nu este detectata ca /dev/ttyACM* sau /dev/ttyUSB*." % label)
    print("%s Nu se va folosi un port intern /dev/ttyS*." % label)
    if profile == "lilygo-t-zigbee":
        print("%s USB-C-ul placii nu este un convertor USB-serial." % label)
        print("%s Conectare: PC -> cablu -> T-U2T mama -> T-U2T tata -> placa." % label)
        print("%s Mod ESP32-C3: DIP 3 si 4 = ON; DIP 1, 2 si 5 = OFF." % label)
        print("%s Dupa schimbarea DIP-urilor, reconecteaza T-U2T si apasa RESET." % label)
    else:
        print("%s Reconecteaza placa direct la PC cu un cablu USB de date." % label)
        print("%s Verifica daca mediul PlatformIO corespunde cipului conectat." % label)
        print("%s Pentru bootloader: tine BOOT, apasa RESET, apoi elibereaza RESET si BOOT." % label)
    print("%s Verifica rezultatul cu: pio device list" % label)
    print()
    env.Exit(1)
