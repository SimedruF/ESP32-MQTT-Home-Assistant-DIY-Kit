#!/usr/bin/env python3

import argparse
import io
import json
import os
from pathlib import Path

import qrcode
from reportlab.lib import colors
from reportlab.lib.pagesizes import A4
from reportlab.lib.styles import getSampleStyleSheet
from reportlab.lib.units import mm
from reportlab.platypus import (
    Image,
    KeepTogether,
    Paragraph,
    SimpleDocTemplate,
    Spacer,
    Table,
    TableStyle,
)


def parse_arguments():
    parser = argparse.ArgumentParser(
        description="Genereaza fisa PDF de configurare ESPHome WROOM."
    )
    parser.add_argument("--secrets", required=True, type=Path)
    parser.add_argument("--output", required=True, type=Path)
    parser.add_argument("--ssid", default="ESP32 HA Kit WROOM")
    parser.add_argument(
        "--web-url", default="http://esp32-ha-kit-wroom.local/"
    )
    return parser.parse_args()


def load_secrets(path):
    values = {}
    for line in path.read_text(encoding="utf-8").splitlines():
        key, separator, raw_value = line.partition(":")
        if not separator or key.lstrip().startswith("#"):
            continue
        values[key.strip()] = json.loads(raw_value.strip())

    required = (
        "fallback_password",
        "api_encryption_key",
        "ota_password",
        "web_username",
        "web_password",
    )
    missing = [key for key in required if not values.get(key)]
    if missing:
        raise ValueError(
            "Lipsesc valorile din secrets.yaml: " + ", ".join(missing)
        )
    return values


def wifi_escape(value):
    escaped = str(value).replace("\\", "\\\\")
    for character in (";", ",", ":", '"'):
        escaped = escaped.replace(character, "\\" + character)
    return escaped


def qr_image(payload, size=42 * mm):
    qr = qrcode.QRCode(
        version=None,
        error_correction=qrcode.constants.ERROR_CORRECT_M,
        box_size=10,
        border=4,
    )
    qr.add_data(payload)
    qr.make(fit=True)
    bitmap = qr.make_image(fill_color="black", back_color="white")
    buffer = io.BytesIO()
    bitmap.save(buffer, format="PNG")
    buffer.seek(0)
    return Image(buffer, width=size, height=size)


def qr_section(title, description, payload, styles):
    return KeepTogether(
        [
            Paragraph(title, styles["Heading2"]),
            Spacer(1, 1 * mm),
            qr_image(payload),
            Spacer(1, 1 * mm),
            Paragraph(description, styles["BodyText"]),
            Spacer(1, 3 * mm),
        ]
    )


def build_pdf(args, secrets):
    args.output.parent.mkdir(parents=True, exist_ok=True)
    styles = getSampleStyleSheet()
    styles["Title"].textColor = colors.HexColor("#1E40AF")
    styles["Heading2"].textColor = colors.HexColor("#1E293B")

    wifi_payload = "WIFI:T:WPA;S:{};P:{};;".format(
        wifi_escape(args.ssid),
        wifi_escape(secrets["fallback_password"]),
    )

    document = SimpleDocTemplate(
        str(args.output),
        pagesize=A4,
        rightMargin=18 * mm,
        leftMargin=18 * mm,
        topMargin=12 * mm,
        bottomMargin=12 * mm,
        title="Configurare ESPHome ESP32-WROOM",
        author="Automatic House Systems",
    )

    credentials = Table(
        [
            ["ID configurare", secrets.get("provisioning_id", "LEGACY")],
            ["Access point", args.ssid],
            ["Parola access point", secrets["fallback_password"]],
            ["Pagina web", args.web_url],
            ["Username web", secrets["web_username"]],
            ["Parola web", secrets["web_password"]],
            ["Cheie API Home Assistant", secrets["api_encryption_key"]],
            ["Parola OTA initiala", secrets["ota_password"]],
        ],
        colWidths=[52 * mm, 118 * mm],
        hAlign="LEFT",
    )
    credentials.setStyle(
        TableStyle(
            [
                ("BACKGROUND", (0, 0), (0, -1), colors.HexColor("#E2E8F0")),
                ("GRID", (0, 0), (-1, -1), 0.5, colors.HexColor("#94A3B8")),
                ("FONTNAME", (0, 0), (0, -1), "Helvetica-Bold"),
                ("FONTNAME", (1, 0), (1, -1), "Helvetica"),
                ("FONTSIZE", (0, 0), (-1, -1), 9),
                ("VALIGN", (0, 0), (-1, -1), "TOP"),
                ("LEFTPADDING", (0, 0), (-1, -1), 6),
                ("RIGHTPADDING", (0, 0), (-1, -1), 6),
                ("TOPPADDING", (0, 0), (-1, -1), 5),
                ("BOTTOMPADDING", (0, 0), (-1, -1), 5),
            ]
        )
    )

    story = [
        Paragraph("Configurare ESPHome - ESP32-WROOM", styles["Title"]),
        Spacer(1, 2 * mm),
        Paragraph(
            "Aceasta fisa contine credentialele initiale unice ale placii. "
            "Preda documentul numai proprietarului si pastreaza-l intr-un loc sigur.",
            styles["BodyText"],
        ),
        Spacer(1, 3 * mm),
        qr_section(
            "1. Conectare la access point",
            "Scaneaza codul cu telefonul pentru conectare automata la reteaua "
            "creata de placa.",
            wifi_payload,
            styles,
        ),
        qr_section(
            "2. Deschidere pagina ESPHome",
            "Scaneaza dupa ce placa a fost configurata pe reteaua locala. "
            "Browserul va solicita username-ul si parola web.",
            args.web_url,
            styles,
        ),
        Paragraph("Date de acces", styles["Heading2"]),
        Spacer(1, 2 * mm),
        credentials,
        Spacer(1, 5 * mm),
        Paragraph(
            "Configurare initiala WiFi: conecteaza-te la access point si "
            "deschide http://192.168.4.1. Selecteaza reteaua casei si salveaza "
            "parola. Credentialele web nu sunt incluse in QR-ul URL. Cheia API "
            "este necesara la adaugarea dispozitivului in Home Assistant. Pentru "
            "control exclusiv, proprietarul trebuie sa recompila firmware-ul in "
            "propriul ESPHome Device Builder cu chei API/OTA proprii.",
            styles["BodyText"],
        ),
    ]
    document.build(story)
    os.chmod(args.output, 0o600)


def main():
    args = parse_arguments()
    build_pdf(args, load_secrets(args.secrets))
    print(args.output)


if __name__ == "__main__":
    main()
