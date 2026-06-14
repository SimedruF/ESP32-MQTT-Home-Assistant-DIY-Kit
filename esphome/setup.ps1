[CmdletBinding()]
param(
    [Parameter(Position = 0)]
    [string]$Command = "help",

    [Parameter(Position = 1)]
    [string]$Profile = "",

    [Parameter(Position = 2)]
    [string]$Target = "",

    [switch]$Force
)

$ErrorActionPreference = "Stop"
Set-StrictMode -Version 2.0

$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$VenvDir = Join-Path $ScriptDir ".venv"
$RequirementsFile = Join-Path $ScriptDir "requirements.txt"
$SecretsFile = Join-Path $ScriptDir "secrets.yaml"
$PythonExe = ""
$PythonPrefixArgs = @()
$VenvPython = Join-Path $VenvDir "Scripts\python.exe"
$EsphomeExe = Join-Path $VenvDir "Scripts\esphome.exe"

$Configs = @{
    "wroom" = "esp32-ha-kit-wroom.yaml"
    "c3" = "esp32-ha-kit-c3.yaml"
    "t-zigbee" = "esp32-ha-kit-t-zigbee.yaml"
    "c6" = "esp32-ha-kit-c6.yaml"
    "c6-supermini" = "esp32-ha-kit-c6-supermini.yaml"
    "s3" = "esp32-ha-kit-s3.yaml"
}

if ($env:ESPHOME_BUILD_PATH) {
    $BuildRoot = $env:ESPHOME_BUILD_PATH
} elseif ($env:LOCALAPPDATA) {
    $BuildRoot = Join-Path $env:LOCALAPPDATA "esp32-ha-kit-esphome"
} else {
    $BuildRoot = Join-Path $env:TEMP "esp32-ha-kit-esphome"
}
$env:ESPHOME_BUILD_PATH = $BuildRoot

function Write-Info {
    param([string]$Message)
    Write-Host "[ESPHome] $Message"
}

function Stop-WithError {
    param([string]$Message)
    throw "[ESPHome] Eroare: $Message"
}

function Show-Usage {
    @"
Utilizare:
  .\setup.ps1 init
  .\setup.ps1 secrets [-Force]
  .\setup.ps1 check [wroom|c3|t-zigbee|c6|c6-supermini|s3|all]
  .\setup.ps1 compile <profil>
  .\setup.ps1 upload <profil> [COMx-sau-adresa]
  .\setup.ps1 run <profil> [COMx-sau-adresa]
  .\setup.ps1 logs <profil> [COMx-sau-adresa]
  .\setup.ps1 clean [profil|all]
  .\setup.ps1 version

Aceleasi comenzi pot fi rulate din Command Prompt prin setup.cmd.

Variabile de mediu optionale:
  AP_PASSWORD, API_ENCRYPTION_KEY, OTA_PASSWORD, WEB_USERNAME, WEB_PASSWORD

Exemple:
  .\setup.ps1 init
  .\setup.ps1 check all
  .\setup.ps1 run wroom COM3
  .\setup.ps1 upload c6-supermini COM5
  .\setup.ps1 logs c3 esp32-ha-kit-c3.local
"@
}

function Invoke-Native {
    param(
        [Parameter(Mandatory = $true)]
        [string]$FilePath,

        [Parameter(Mandatory = $false)]
        [string[]]$Arguments = @()
    )

    & $FilePath @Arguments
    if ($LASTEXITCODE -ne 0) {
        Stop-WithError "Comanda a esuat cu codul $LASTEXITCODE`: $FilePath"
    }
}

function Test-PythonCandidate {
    param(
        [string]$Executable,
        [string[]]$PrefixArgs
    )

    try {
        & $Executable @PrefixArgs -c `
            "import lzma, ssl, sys, venv; raise SystemExit(sys.version_info < (3, 11))" `
            *> $null
        return $LASTEXITCODE -eq 0
    } catch {
        return $false
    }
}

function Find-Python {
    $Candidates = @()
    $PyLauncher = Get-Command "py.exe" -ErrorAction SilentlyContinue
    if ($PyLauncher) {
        $Candidates += @{
            Executable = $PyLauncher.Source
            Prefix = @("-3.13")
        }
        $Candidates += @{
            Executable = $PyLauncher.Source
            Prefix = @("-3.12")
        }
        $Candidates += @{
            Executable = $PyLauncher.Source
            Prefix = @("-3.11")
        }
    }

    foreach ($Name in @("python.exe", "python3.exe")) {
        $Resolved = Get-Command $Name -ErrorAction SilentlyContinue
        if ($Resolved) {
            $Candidates += @{
                Executable = $Resolved.Source
                Prefix = @()
            }
        }
    }

    foreach ($Candidate in $Candidates) {
        if (Test-PythonCandidate $Candidate.Executable $Candidate.Prefix) {
            $script:PythonExe = $Candidate.Executable
            $script:PythonPrefixArgs = $Candidate.Prefix
            return
        }
    }

    Stop-WithError "ESPHome necesita Python 3.11+ pentru Windows, cu modulele venv, ssl si lzma."
}

function Invoke-BootstrapPython {
    param([string[]]$Arguments)
    Invoke-Native $script:PythonExe ($script:PythonPrefixArgs + $Arguments)
}

function Ensure-Environment {
    Find-Python

    if ($BuildRoot.Contains(" ")) {
        Stop-WithError "Calea de build ESPHome nu poate contine spatii: $BuildRoot"
    }

    if (Test-Path $VenvPython) {
        & $VenvPython -c "import lzma, ssl, venv" *> $null
        if ($LASTEXITCODE -ne 0) {
            Write-Info "Mediul virtual existent foloseste un Python incomplet; il recreez."
            Remove-Item -Recurse -Force $VenvDir
        }
    }

    if (-not (Test-Path $VenvPython)) {
        $Version = & $PythonExe @PythonPrefixArgs --version 2>&1
        Write-Info "Creez mediul virtual cu $Version..."
        Invoke-BootstrapPython @("-m", "venv", $VenvDir)
    }

    $ExpectedHash = (Get-FileHash -Algorithm SHA256 $RequirementsFile).Hash.ToLowerInvariant()
    $StampFile = Join-Path $VenvDir ".requirements.sha256"
    $InstalledHash = ""
    if (Test-Path $StampFile) {
        $InstalledHash = (Get-Content -Raw $StampFile).Trim().ToLowerInvariant()
    }

    if ((-not (Test-Path $EsphomeExe)) -or ($InstalledHash -ne $ExpectedHash)) {
        Write-Info "Instalez dependentele ESPHome in $VenvDir..."
        Invoke-Native $VenvPython @("-m", "pip", "install", "--upgrade", "pip", "wheel")
        Invoke-Native $VenvPython @("-m", "pip", "install", "--requirement", $RequirementsFile)
        Set-Content -Path $StampFile -Value $ExpectedHash -Encoding ASCII
    }
}

function New-RandomBytes {
    param([int]$Length)

    $Bytes = New-Object byte[] $Length
    $Generator = [System.Security.Cryptography.RandomNumberGenerator]::Create()
    try {
        $Generator.GetBytes($Bytes)
    } finally {
        $Generator.Dispose()
    }
    return $Bytes
}

function New-UrlSafeToken {
    param([int]$Length)

    $Token = [Convert]::ToBase64String((New-RandomBytes $Length))
    $Token = $Token.TrimEnd([char[]]"=")
    $Token = $Token.Replace("+", "-")
    return $Token.Replace("/", "_")
}

function ConvertTo-JsonString {
    param([string]$Value)
    $Json = ConvertTo-Json -InputObject $Value -Compress
    return $Json
}

function New-Secrets {
    param([bool]$Force)

    if ((Test-Path $SecretsFile) -and (-not $Force)) {
        Write-Info "$SecretsFile exista deja; nu il modific."
        return
    }

    $WebUsername = if ($env:WEB_USERNAME) { $env:WEB_USERNAME } else { "admin" }
    $ApPassword = if ($env:AP_PASSWORD) {
        $env:AP_PASSWORD
    } elseif ($env:FALLBACK_PASSWORD) {
        $env:FALLBACK_PASSWORD
    } else {
        New-UrlSafeToken 18
    }
    $ApiKey = if ($env:API_ENCRYPTION_KEY) {
        $env:API_ENCRYPTION_KEY
    } else {
        [Convert]::ToBase64String((New-RandomBytes 32))
    }
    $OtaPassword = if ($env:OTA_PASSWORD) {
        $env:OTA_PASSWORD
    } else {
        New-UrlSafeToken 24
    }
    $WebPassword = if ($env:WEB_PASSWORD) {
        $env:WEB_PASSWORD
    } else {
        New-UrlSafeToken 24
    }
    $ProvisioningId = -join ((New-RandomBytes 3) | ForEach-Object { $_.ToString("X2") })

    $Lines = @(
        "# Generat de setup.ps1. Nu salva acest fisier in Git.",
        "provisioning_id: $(ConvertTo-JsonString $ProvisioningId)",
        "fallback_password: $(ConvertTo-JsonString $ApPassword)",
        "api_encryption_key: $(ConvertTo-JsonString $ApiKey)",
        "ota_password: $(ConvertTo-JsonString $OtaPassword)",
        "web_username: $(ConvertTo-JsonString $WebUsername)",
        "web_password: $(ConvertTo-JsonString $WebPassword)"
    )
    Set-Content -Path $SecretsFile -Value $Lines -Encoding UTF8
    Write-Info "Am generat $SecretsFile."
}

function Require-Secrets {
    if (-not (Test-Path $SecretsFile)) {
        Stop-WithError "Lipseste secrets.yaml. Ruleaza mai intai: .\setup.ps1 secrets"
    }
}

function Resolve-Config {
    param([string]$RequestedProfile)

    if (-not $Configs.ContainsKey($RequestedProfile)) {
        Stop-WithError "Profil necunoscut: $RequestedProfile. Foloseste wroom, c3, t-zigbee, c6, c6-supermini sau s3."
    }
    $ConfigPath = Join-Path $ScriptDir $Configs[$RequestedProfile]
    return $ConfigPath
}

function Invoke-ForProfiles {
    param(
        [string]$Action,
        [string]$RequestedProfile
    )

    if ((-not $RequestedProfile) -or ($RequestedProfile -eq "all")) {
        foreach ($CurrentProfile in @("wroom", "c3", "t-zigbee", "c6", "c6-supermini", "s3")) {
            Write-Info "$Action`: $CurrentProfile"
            Invoke-Native $EsphomeExe @($Action, (Resolve-Config $CurrentProfile))
        }
        return
    }

    Invoke-Native $EsphomeExe @($Action, (Resolve-Config $RequestedProfile))
}

function Invoke-DeviceCommand {
    param(
        [string]$Action,
        [string]$RequestedProfile,
        [string]$RequestedTarget
    )

    $Arguments = @($Action, (Resolve-Config $RequestedProfile))
    if ($RequestedTarget) {
        $Arguments += @("--device", $RequestedTarget)
    }
    Invoke-Native $EsphomeExe $Arguments
}

try {
    switch ($Command.ToLowerInvariant()) {
        "init" {
            Ensure-Environment
            New-Secrets $false
            Require-Secrets
            Invoke-ForProfiles "config" "all"
            Write-Info "Setup finalizat. Pentru instalare: .\setup.ps1 run <profil> <COMx>"
        }
        "secrets" {
            New-Secrets ($Force -or ($Profile -in @("--force", "-Force")))
        }
        "check" {
            Ensure-Environment
            Require-Secrets
            Invoke-ForProfiles "config" $(if ($Profile) { $Profile } else { "all" })
        }
        "compile" {
            Ensure-Environment
            Require-Secrets
            Invoke-DeviceCommand "compile" $Profile ""
        }
        { $_ -in @("upload", "run", "logs") } {
            Ensure-Environment
            Require-Secrets
            Invoke-DeviceCommand $_ $Profile $Target
        }
        "clean" {
            Ensure-Environment
            Require-Secrets
            Invoke-ForProfiles "clean" $(if ($Profile) { $Profile } else { "all" })
        }
        "version" {
            Ensure-Environment
            Invoke-Native $VenvPython @("--version")
            Invoke-Native $EsphomeExe @("version")
        }
        { $_ -in @("help", "-h", "--help") } {
            Show-Usage
        }
        default {
            Show-Usage
            Stop-WithError "Comanda necunoscuta: $Command"
        }
    }
} catch {
    Write-Error $_.Exception.Message
    exit 1
}
