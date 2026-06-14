#!/usr/bin/env bash

set -Eeuo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
SETUP_SCRIPT="${SCRIPT_DIR}/setup.sh"
SECRETS_FILE="${SCRIPT_DIR}/secrets.yaml"
PDF_GENERATOR="${SCRIPT_DIR}/generate-wroom-setup-pdf.py"
PDF_FILE="${SCRIPT_DIR}/generated/esp32-ha-kit-wroom-setup.pdf"
PROFILE="wroom"
AP_SSID="ESP32 HA Kit WROOM"
WEB_URL="http://esp32-ha-kit-wroom.local/"
PORT=""
REUSE_SECRETS=false

info() {
  printf '[WROOM AP] %s\n' "$*"
}

die() {
  printf '[WROOM AP] Eroare: %s\n' "$*" >&2
  exit 1
}

usage() {
  cat <<'EOF'
Utilizare:
  ./setup-wroom-ap.sh [port]
  ./setup-wroom-ap.sh [port] --reuse-secrets

Exemple:
  ./setup-wroom-ap.sh
  ./setup-wroom-ap.sh /dev/ttyUSB0
  ./setup-wroom-ap.sh /dev/ttyUSB0 --reuse-secrets
  AP_PASSWORD='parola-minim-8' ./setup-wroom-ap.sh /dev/ttyUSB0

Flux:
  1. Creeaza mediul virtual ESPHome, daca lipseste.
  2. Genereaza credentiale unice pentru placa programata.
  3. Valideaza si compileaza profilul ESP32-WROOM.
  4. Programeaza placa prin USB.
  5. Genereaza PDF-ul cu QR-uri si datele de acces.

Implicit, fiecare rulare inlocuieste secrets.yaml pentru a nu reutiliza aceleasi
chei la clienti diferiti. Foloseste --reuse-secrets numai pentru a reincerca
programarea aceleiasi placi.

Autodetectarea foloseste numai porturi /dev/ttyUSB*. Un port /dev/ttyACM*
trebuie transmis explicit pentru a evita programarea altei placi.
EOF
}

parse_arguments() {
  local argument

  for argument in "$@"; do
    case "${argument}" in
      --reuse-secrets)
        REUSE_SECRETS=true
        ;;
      --force-secrets)
        REUSE_SECRETS=false
        ;;
      -h|--help)
        usage
        exit 0
        ;;
      -*)
        die "Optiune necunoscuta: ${argument}"
        ;;
      *)
        [[ -z "${PORT}" ]] || die "Poate fi specificat un singur port USB."
        PORT="${argument}"
        ;;
    esac
  done
}

detect_port() {
  local candidate
  local real_port
  local -a ports=()

  if [[ -n "${PORT}" ]]; then
    [[ -c "${PORT}" ]] || die "Portul nu exista sau nu este serial: ${PORT}"
    return
  fi

  for candidate in /dev/serial/by-id/*; do
    [[ -e "${candidate}" ]] || continue
    real_port="$(readlink -f -- "${candidate}")"
    if [[ "${real_port}" == /dev/ttyUSB* ]]; then
      ports+=("${candidate}")
    fi
  done

  if [[ "${#ports[@]}" -eq 0 ]]; then
    for candidate in /dev/ttyUSB*; do
      [[ -c "${candidate}" ]] && ports+=("${candidate}")
    done
  fi

  case "${#ports[@]}" in
    0)
      die "Nu am gasit placa WROOM. Conecteaz-o si indica portul, de exemplu /dev/ttyUSB0."
      ;;
    1)
      PORT="${ports[0]}"
      ;;
    *)
      printf '[WROOM AP] Porturi USB detectate:\n' >&2
      printf '  %s\n' "${ports[@]}" >&2
      die "Sunt conectate mai multe placi; specifica portul dorit."
      ;;
  esac
}

show_connection_info() {
  local python_bin="${SCRIPT_DIR}/.venv/bin/python"

  [[ -x "${python_bin}" ]] || return
  "${python_bin}" - "${SECRETS_FILE}" "${AP_SSID}" <<'PY'
import json
import pathlib
import sys

secrets_file = pathlib.Path(sys.argv[1])
ssid = sys.argv[2]
password = ""

for line in secrets_file.read_text(encoding="utf-8").splitlines():
    key, separator, raw_value = line.partition(":")
    if separator and key.strip() == "fallback_password":
        password = json.loads(raw_value.strip())
        break

print()
print("[WROOM AP] Configurare finalizata:")
print(f"  SSID: {ssid}")
print(f"  Parola AP: {password}")
print("  Portal configurare WiFi: http://192.168.4.1")
print("  Pas urmator: conecteaza-te la AP, alege reteaua casei si salveaza parola.")
PY
}

generate_setup_pdf() {
  local python_bin="${SCRIPT_DIR}/.venv/bin/python"

  [[ -x "${python_bin}" ]] || die "Mediul Python ESPHome nu este disponibil."
  info "Generez fisa PDF cu QR-uri si datele de acces."
  "${python_bin}" "${PDF_GENERATOR}" \
    --secrets "${SECRETS_FILE}" \
    --output "${PDF_FILE}" \
    --ssid "${AP_SSID}" \
    --web-url "${WEB_URL}"
  info "PDF generat: ${PDF_FILE}"
}

main() {
  parse_arguments "$@"
  detect_port

  info "Port selectat: ${PORT}"

  if [[ "${REUSE_SECRETS}" == "true" ]]; then
    info "Reutilizez credentialele existente pentru aceeasi placa."
    "${SETUP_SCRIPT}" secrets
  else
    info "Generez un set unic de parole si chei pentru aceasta placa."
    "${SETUP_SCRIPT}" secrets --force
  fi

  info "Validez configuratia AP-only pentru ESP32-WROOM."
  "${SETUP_SCRIPT}" check "${PROFILE}"

  info "Compilez firmware-ul ESPHome."
  "${SETUP_SCRIPT}" compile "${PROFILE}"

  info "Programez placa."
  "${SETUP_SCRIPT}" upload "${PROFILE}" "${PORT}"

  generate_setup_pdf
  show_connection_info
}

main "$@"
