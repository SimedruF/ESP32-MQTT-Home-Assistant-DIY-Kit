Import("env")

import glob
import os
from SCons.Script import COMMAND_LINE_TARGETS


def serial_ports():
    candidates = []

    # Prefer persistent udev links when available.
    for path in sorted(glob.glob("/dev/serial/by-id/*")):
        real_path = os.path.realpath(path)
        if real_path.startswith(("/dev/ttyACM", "/dev/ttyUSB")):
            candidates.append(path)

    for pattern in ("/dev/ttyACM*", "/dev/ttyUSB*"):
        for path in sorted(glob.glob(pattern)):
            if path not in candidates and os.path.realpath(path) not in {
                os.path.realpath(item) for item in candidates
            }:
                candidates.append(path)

    return candidates


ports = serial_ports()
is_upload = any(target == "upload" or target.startswith("upload") for target in COMMAND_LINE_TARGETS)

if ports:
    env.Replace(UPLOAD_PORT=ports[0])
    print("[S3] Port serial selectat automat: %s" % ports[0])
elif is_upload:
    print()
    print("[S3] EROARE: placa nu este detectata ca /dev/ttyACM* sau /dev/ttyUSB*.")
    print("[S3] Nu se va folosi un port intern /dev/ttyS*.")
    print("[S3] Reconecteaza placa direct la PC cu un cablu USB de date,")
    print("[S3] apoi intra in bootloader: tine BOOT, apasa RESET, elibereaza RESET si BOOT.")
    print("[S3] Verifica rezultatul cu: pio device list")
    print()
    env.Exit(1)
