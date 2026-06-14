#!/usr/bin/env bash

set -Eeuo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
VENV_DIR="${SCRIPT_DIR}/.venv"
REQUIREMENTS_FILE="${SCRIPT_DIR}/requirements.txt"
SECRETS_FILE="${SCRIPT_DIR}/secrets.yaml"
BUILD_ROOT="${ESPHOME_BUILD_PATH:-${XDG_CACHE_HOME:-${HOME}/.cache}/esp32-ha-kit-esphome}"
PYTHON_BIN=""
ESPHOME_BIN="${VENV_DIR}/bin/esphome"

# ESP-IDF/PlatformIO nu accepta spatii in calea proiectului de build.
export ESPHOME_BUILD_PATH="${BUILD_ROOT}"

readonly -A CONFIGS=(
  [wroom]="esp32-ha-kit-wroom.yaml"
  [c3]="esp32-ha-kit-c3.yaml"
  [t-zigbee]="esp32-ha-kit-t-zigbee.yaml"
  [c6]="esp32-ha-kit-c6.yaml"
  [c6-supermini]="esp32-ha-kit-c6-supermini.yaml"
  [s3]="esp32-ha-kit-s3.yaml"
)

info() {
  printf '[ESPHome] %s\n' "$*"
}

error() {
  printf '[ESPHome] Eroare: %s\n' "$*" >&2
}

die() {
  error "$*"
  exit 1
}

usage() {
  cat <<'EOF'
Utilizare:
  ./setup.sh init
  ./setup.sh secrets [--force]
  ./setup.sh check [wroom|c3|t-zigbee|c6|c6-supermini|s3|all]
  ./setup.sh compile <wroom|c3|t-zigbee|c6|c6-supermini|s3>
  ./setup.sh upload <wroom|c3|t-zigbee|c6|c6-supermini|s3> [port-sau-adresa]
  ./setup.sh run <wroom|c3|t-zigbee|c6|c6-supermini|s3> [port-sau-adresa]
  ./setup.sh logs <wroom|c3|t-zigbee|c6|c6-supermini|s3> [port-sau-adresa]
  ./setup.sh clean [wroom|c3|t-zigbee|c6|c6-supermini|s3|all]
  ./setup.sh version

Comenzi:
  init      Creeaza mediul Python local si secrets.yaml.
  secrets   Genereaza numai secrets.yaml. Foloseste --force pentru rescriere.
  check     Valideaza una sau toate configuratiile.
  compile   Compileaza firmware-ul pentru profilul selectat.
  upload    Incarca firmware-ul deja compilat, prin USB sau OTA.
  run       Valideaza, compileaza, incarca si deschide logurile.
  logs      Afiseaza logurile prin USB sau API.
  clean     Sterge fisierele de build generate de ESPHome.
  version   Afiseaza versiunile Python si ESPHome.

Variabile optionale pentru rulare neinteractiva:
  AP_PASSWORD, API_ENCRYPTION_KEY, OTA_PASSWORD, WEB_USERNAME, WEB_PASSWORD

Exemple:
  ./setup.sh init
  ./setup.sh check all
  ./setup.sh run wroom /dev/ttyUSB0
  ./setup.sh run t-zigbee /dev/ttyUSB0
  ./setup.sh upload s3 /dev/ttyACM0
  ./setup.sh logs c3 esp32-ha-kit-c3.local
EOF
}

find_python() {
  local candidate
  local resolved
  local candidates=(
    /usr/bin/python3.13
    /usr/bin/python3.12
    /usr/bin/python3.11
    "${HOME}/.platformio/penv/bin/python"
    python3.13
    python3.12
    python3.11
    python3
  )

  for candidate in "${candidates[@]}"; do
    if [[ "${candidate}" == */* ]]; then
      [[ -x "${candidate}" ]] || continue
      resolved="${candidate}"
    else
      resolved="$(command -v "${candidate}" 2>/dev/null || true)"
      [[ -n "${resolved}" ]] || continue
    fi

    if "${resolved}" -c \
      'import lzma, ssl, sys, venv; raise SystemExit(sys.version_info < (3, 11))'
    then
      PYTHON_BIN="${resolved}"
      return
    fi
  done

  die "ESPHome necesita Python 3.11+ cu modulele venv, ssl si lzma."
}

requirements_hash() {
  if command -v sha256sum >/dev/null 2>&1; then
    sha256sum "${REQUIREMENTS_FILE}" | awk '{print $1}'
  else
    shasum -a 256 "${REQUIREMENTS_FILE}" | awk '{print $1}'
  fi
}

ensure_environment() {
  local expected_hash
  local installed_hash=""
  local stamp_file="${VENV_DIR}/.requirements.sha256"

  find_python

  [[ "${BUILD_ROOT}" != *" "* ]] ||
    die "Calea de build ESPHome nu poate contine spatii: ${BUILD_ROOT}"

  if [[ -x "${VENV_DIR}/bin/python" ]] &&
     ! "${VENV_DIR}/bin/python" -c 'import lzma, ssl, venv' >/dev/null 2>&1
  then
    info "Mediul virtual existent foloseste un Python incomplet; il recreez."
    rm -rf "${VENV_DIR}"
  fi

  if [[ ! -x "${VENV_DIR}/bin/python" ]]; then
    info "Creez mediul virtual cu $(${PYTHON_BIN} --version 2>&1)..."
    "${PYTHON_BIN}" -m venv "${VENV_DIR}"
  fi

  expected_hash="$(requirements_hash)"
  if [[ -f "${stamp_file}" ]]; then
    installed_hash="$(<"${stamp_file}")"
  fi

  if [[ ! -x "${ESPHOME_BIN}" || "${installed_hash}" != "${expected_hash}" ]]; then
    info "Instalez dependentele ESPHome in ${VENV_DIR}..."
    "${VENV_DIR}/bin/python" -m pip install --upgrade pip wheel
    "${VENV_DIR}/bin/python" -m pip install --requirement "${REQUIREMENTS_FILE}"
    printf '%s\n' "${expected_hash}" > "${stamp_file}"
  fi
}

prompt_value() {
  local variable_name="$1"
  local prompt="$2"
  local secret="${3:-false}"
  local current_value="${!variable_name:-}"
  local entered=""

  if [[ -n "${current_value}" ]]; then
    return
  fi

  [[ -t 0 ]] || die "${variable_name} nu este setata si terminalul nu este interactiv."

  if [[ "${secret}" == "true" ]]; then
    read -r -s -p "${prompt}: " entered
    printf '\n'
  else
    read -r -p "${prompt}: " entered
  fi

  printf -v "${variable_name}" '%s' "${entered}"
}

generate_secrets() {
  local force="${1:-false}"

  find_python

  if [[ -f "${SECRETS_FILE}" && "${force}" != "true" ]]; then
    info "${SECRETS_FILE} exista deja; nu il modific."
    return
  fi

  WEB_USERNAME="${WEB_USERNAME:-admin}"

  export WEB_USERNAME
  export AP_PASSWORD="${AP_PASSWORD:-${FALLBACK_PASSWORD:-}}"
  export API_ENCRYPTION_KEY="${API_ENCRYPTION_KEY:-}"
  export OTA_PASSWORD="${OTA_PASSWORD:-}"
  export WEB_PASSWORD="${WEB_PASSWORD:-}"
  export SECRETS_FILE

  "${PYTHON_BIN}" <<'PY'
import base64
import json
import os
import secrets

values = {
    "provisioning_id": secrets.token_hex(3).upper(),
    "fallback_password": (
        os.environ["AP_PASSWORD"] or secrets.token_urlsafe(18)
    ),
    "api_encryption_key": (
        os.environ["API_ENCRYPTION_KEY"]
        or base64.b64encode(secrets.token_bytes(32)).decode("ascii")
    ),
    "ota_password": os.environ["OTA_PASSWORD"] or secrets.token_urlsafe(24),
    "web_username": os.environ["WEB_USERNAME"],
    "web_password": os.environ["WEB_PASSWORD"] or secrets.token_urlsafe(24),
}

with open(os.environ["SECRETS_FILE"], "w", encoding="utf-8") as stream:
    stream.write("# Generat de setup.sh. Nu salva acest fisier in Git.\n")
    for key, value in values.items():
        stream.write("{}: {}\n".format(key, json.dumps(value)))
PY

  chmod 600 "${SECRETS_FILE}"
  info "Am generat ${SECRETS_FILE} cu permisiuni 600."
}

require_secrets() {
  [[ -f "${SECRETS_FILE}" ]] ||
    die "Lipseste secrets.yaml. Ruleaza mai intai: ./setup.sh secrets"
}

resolve_config() {
  local profile="${1:-}"

  [[ -n "${profile}" ]] || die "Lipseste profilul placii."
  [[ -n "${CONFIGS[${profile}]:-}" ]] ||
    die "Profil necunoscut: ${profile}. Foloseste wroom, c3, t-zigbee, c6, c6-supermini sau s3."

  printf '%s/%s\n' "${SCRIPT_DIR}" "${CONFIGS[${profile}]}"
}

run_for_profiles() {
  local action="$1"
  local requested="${2:-all}"
  local profile
  local config

  if [[ "${requested}" == "all" ]]; then
    for profile in wroom c3 t-zigbee c6 c6-supermini s3; do
      config="$(resolve_config "${profile}")"
      info "${action}: ${profile}"
      "${ESPHOME_BIN}" "${action}" "${config}"
    done
    return
  fi

  config="$(resolve_config "${requested}")"
  "${ESPHOME_BIN}" "${action}" "${config}"
}

run_device_command() {
  local action="$1"
  local profile="${2:-}"
  local target="${3:-}"
  local config
  local args=()

  config="$(resolve_config "${profile}")"
  if [[ -n "${target}" ]]; then
    args+=(--device "${target}")
  fi

  "${ESPHOME_BIN}" "${action}" "${config}" "${args[@]}"
}

main() {
  local command="${1:-help}"

  case "${command}" in
    init)
      ensure_environment
      generate_secrets false
      require_secrets
      run_for_profiles config all
      info "Setup finalizat. Pentru instalare: ./setup.sh run <profil> <port>"
      ;;
    secrets)
      if [[ "${2:-}" == "--force" ]]; then
        generate_secrets true
      else
        generate_secrets false
      fi
      ;;
    check)
      ensure_environment
      require_secrets
      run_for_profiles config "${2:-all}"
      ;;
    compile)
      ensure_environment
      require_secrets
      run_device_command compile "${2:-}"
      ;;
    upload|run|logs)
      ensure_environment
      require_secrets
      run_device_command "${command}" "${2:-}" "${3:-}"
      ;;
    clean)
      ensure_environment
      require_secrets
      run_for_profiles clean "${2:-all}"
      ;;
    version)
      ensure_environment
      "${VENV_DIR}/bin/python" --version
      "${ESPHOME_BIN}" version
      ;;
    help|-h|--help)
      usage
      ;;
    *)
      usage
      die "Comanda necunoscuta: ${command}"
      ;;
  esac
}

main "$@"
