[CmdletBinding()]
param(
    [Parameter(Position = 0)]
    [string]$Port = "",

    [switch]$ReuseSecrets,
    [switch]$Help
)

$ErrorActionPreference = "Stop"
Set-StrictMode -Version 2.0

$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$SetupScript = Join-Path $ScriptDir "setup.ps1"
$SecretsFile = Join-Path $ScriptDir "secrets.yaml"
$PdfGenerator = Join-Path $ScriptDir "generate-wroom-setup-pdf.py"
$PdfFile = Join-Path $ScriptDir "generated\esp32-ha-kit-wroom-setup.pdf"
$VenvPython = Join-Path $ScriptDir ".venv\Scripts\python.exe"
$Profile = "wroom"
$ApSsid = "ESP32 HA Kit WROOM"
$WebUrl = "http://esp32-ha-kit-wroom.local/"

function Write-Info {
    param([string]$Message)
    Write-Host "[WROOM AP] $Message"
}

function Stop-WithError {
    param([string]$Message)
    throw "[WROOM AP] Eroare: $Message"
}

function Show-Usage {
    @"
Utilizare:
  .\setup-wroom-ap.ps1 [COMx]
  .\setup-wroom-ap.ps1 [COMx] -ReuseSecrets

Exemple:
  .\setup-wroom-ap.ps1
  .\setup-wroom-ap.ps1 COM3
  .\setup-wroom-ap.ps1 COM3 -ReuseSecrets
  `$env:AP_PASSWORD = "parola-minim-8"
  .\setup-wroom-ap.ps1 COM3

Flux:
  1. Creeaza mediul virtual ESPHome.
  2. Genereaza credentiale unice pentru placa.
  3. Valideaza si compileaza profilul ESP32-WROOM.
  4. Programeaza placa prin USB.
  5. Genereaza PDF-ul cu QR-uri, cheia API si datele de acces.

Aceeasi comanda poate fi rulata din Command Prompt prin setup-wroom-ap.cmd.
"@
}

function Invoke-Native {
    param(
        [string]$FilePath,
        [string[]]$Arguments = @()
    )

    & $FilePath @Arguments
    if ($LASTEXITCODE -ne 0) {
        Stop-WithError "Comanda a esuat cu codul $LASTEXITCODE`: $FilePath"
    }
}

function Get-UsbSerialPorts {
    $Ports = @()

    try {
        $Ports = @(Get-CimInstance Win32_SerialPort -ErrorAction Stop |
            Where-Object {
                ($_.PNPDeviceID -like "USB*") -or
                ($_.Description -match "CH340|CH341|CP210|FTDI|USB.Serial|USB-UART|UART")
            } |
            ForEach-Object { $_.DeviceID } |
            Sort-Object -Unique)
    } catch {
        $Ports = @()
    }

    if ($Ports.Count -eq 0) {
        try {
            $Ports = @([System.IO.Ports.SerialPort]::GetPortNames() | Sort-Object -Unique)
        } catch {
            $Ports = @()
        }
    }

    return $Ports
}

function Resolve-Port {
    if ($script:Port) {
        if ($script:Port -notmatch "^COM\d+$") {
            Stop-WithError "Port invalid: $script:Port. Exemplu corect: COM3"
        }
        return
    }

    $DetectedPorts = @(Get-UsbSerialPorts)
    if ($DetectedPorts.Count -eq 0) {
        Stop-WithError "Nu am gasit placa WROOM. Conecteaz-o si indica portul, de exemplu COM3."
    }
    if ($DetectedPorts.Count -gt 1) {
        Write-Host "[WROOM AP] Porturi COM detectate:"
        $DetectedPorts | ForEach-Object { Write-Host "  $_" }
        Stop-WithError "Sunt disponibile mai multe porturi; specifica portul placii WROOM."
    }

    $script:Port = $DetectedPorts[0]
}

function Invoke-Setup {
    param(
        [string[]]$Arguments,
        [switch]$ForceSecrets
    )

    if ($ForceSecrets) {
        & $SetupScript @Arguments -Force
    } else {
        & $SetupScript @Arguments
    }
    if (-not $?) {
        Stop-WithError "setup.ps1 a esuat."
    }
}

function Get-SecretValue {
    param([string]$Name)

    foreach ($Line in Get-Content $SecretsFile) {
        if ($Line -match "^\s*$([regex]::Escape($Name))\s*:\s*(.+?)\s*$") {
            $Value = ConvertFrom-Json -InputObject $Matches[1]
            return $Value
        }
    }
    return ""
}

try {
    if ($Help) {
        Show-Usage
        exit 0
    }

    Resolve-Port
    Write-Info "Port selectat: $Port"

    if ($ReuseSecrets) {
        Write-Info "Reutilizez credentialele existente pentru aceeasi placa."
        Invoke-Setup @("secrets")
    } else {
        Write-Info "Generez un set unic de parole si chei pentru aceasta placa."
        Invoke-Setup -Arguments @("secrets") -ForceSecrets
    }

    Write-Info "Validez configuratia AP-only pentru ESP32-WROOM."
    Invoke-Setup @("check", $Profile)

    Write-Info "Compilez firmware-ul ESPHome."
    Invoke-Setup @("compile", $Profile)

    Write-Info "Programez placa."
    Invoke-Setup @("upload", $Profile, $Port)

    if (-not (Test-Path $VenvPython)) {
        Stop-WithError "Mediul Python ESPHome nu este disponibil."
    }

    Write-Info "Generez fisa PDF cu QR-uri si datele de acces."
    Invoke-Native $VenvPython @(
        $PdfGenerator,
        "--secrets", $SecretsFile,
        "--output", $PdfFile,
        "--ssid", $ApSsid,
        "--web-url", $WebUrl
    )

    $ApPassword = Get-SecretValue "fallback_password"
    Write-Host ""
    Write-Info "Configurare finalizata:"
    Write-Host "  SSID: $ApSsid"
    Write-Host "  Parola AP: $ApPassword"
    Write-Host "  Portal configurare WiFi: http://192.168.4.1"
    Write-Host "  PDF: $PdfFile"
    Write-Host "  Pas urmator: conecteaza-te la AP, alege reteaua casei si salveaza parola."
} catch {
    Write-Error $_.Exception.Message
    exit 1
}
