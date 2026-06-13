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
profile = env.subst("$PIOENV")
label = "[%s]" % profile

if is_upload and ports:
    expected_chip = {
        "esp32-c3": "ESP32-C3",
        "esp32-s3": "ESP32-S3",
    }.get(profile)
    esptool_dir = env.PioPlatform().get_package_dir("tool-esptoolpy")
    esptool = os.path.join(esptool_dir, "esptool.py") if esptool_dir else ""
    matching_ports = []

    if expected_chip and os.path.isfile(esptool):
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
        ports = matching_ports

if ports:
    env.Replace(UPLOAD_PORT=ports[0], MONITOR_PORT=ports[0])
    print("%s Port upload si monitor selectat automat: %s" % (label, ports[0]))
elif is_upload:
    print()
    print("%s EROARE: placa nu este detectata ca /dev/ttyACM* sau /dev/ttyUSB*." % label)
    print("%s Nu se va folosi un port intern /dev/ttyS*." % label)
    print("%s Reconecteaza placa direct la PC cu un cablu USB de date." % label)
    print("%s Verifica daca mediul PlatformIO corespunde cipului conectat." % label)
    print("%s Pentru bootloader: tine BOOT, apasa RESET, apoi elibereaza RESET si BOOT." % label)
    print("%s Verifica rezultatul cu: pio device list" % label)
    print()
    env.Exit(1)
