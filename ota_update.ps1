#!/usr/bin/env pwsh
<#
.SYNOPSIS
    PetDuet OTA Update Helper Script
.DESCRIPTION
    Automates firmware version bump, binary copy, manifest update for OTA deployment.
.PARAMETER Version
    New semantic version (e.g., 2.1.3)
.PARAMETER Notes
    Release notes (default: "Firmware update")
.EXAMPLE
    .\ota_update.ps1 2.1.3 "Fixed touch calibration"
#>

param(
    [Parameter(Mandatory=$true, Position=0)]
    [ValidatePattern('^\d+\.\d+\.\d+$')]
    [string]$Version,

    [Parameter(Mandatory=$false, Position=1)]
    [string]$Notes = "Firmware update"
)

$PROJECT_ROOT = Split-Path -Parent $MyInvocation.MyCommand.Definition
$FIRMWARE_BIN = Join-Path $PROJECT_ROOT "esp32_firmware\.pio\build\esp32-2432s028r\firmware.bin"
$SERVER_BIN   = Join-Path $PROJECT_ROOT "php_server\cyd_tamagotchi.bin"
$MANIFEST     = Join-Path $PROJECT_ROOT "php_server\ota_manifest.json"
$CONFIG_H     = Join-Path $PROJECT_ROOT "esp32_firmware\config.h"

Write-Host "=== PetDuet OTA Update: $Version ===" -ForegroundColor Yellow
Write-Host ""

# 1. Check firmware binary exists
if (-not (Test-Path $FIRMWARE_BIN)) {
    Write-Error "Firmware binary not found at $FIRMWARE_BIN"
    Write-Host "Build the firmware first: cd esp32_firmware && pio run"
    exit 1
}

# 2. Copy firmware to server
Write-Host "[1/6] Copying firmware to PHP server..." -ForegroundColor Green
Copy-Item $FIRMWARE_BIN $SERVER_BIN -Force

# 3. Calculate SHA256 and size
Write-Host "[2/6] Calculating SHA256 and size..." -ForegroundColor Green
$SHA256 = (Get-FileHash -Algorithm SHA256 $SERVER_BIN).Hash
$SIZE   = (Get-Item $SERVER_BIN).Length
$DATE   = Get-Date -Format "yyyy-MM-dd"

Write-Host "  SHA256: $SHA256"
Write-Host "  Size:   $SIZE bytes"

# 4. Update config.h
Write-Host "[3/6] Updating config.h FIRMWARE_VERSION..." -ForegroundColor Green
$config = Get-Content $CONFIG_H -Raw
# Use [regex]::Replace for multiline string replacement
$newConfig = [regex]::Replace($config, '#define FIRMWARE_VERSION\s+".*"', '#define FIRMWARE_VERSION        "' + $Version + '"')
if ($newConfig -eq $config) {
    # Try alternate pattern
    $newConfig = [regex]::Replace($config, '(?s)(#define FIRMWARE_VERSION\s+").*?"', '$1' + $Version + '"')
}
Set-Content $CONFIG_H $newConfig

# 5. Update ota_manifest.json
Write-Host "[4/6] Updating ota_manifest.json..." -ForegroundColor Green
$manifest = Get-Content $MANIFEST | ConvertFrom-Json
$manifest.firmware.latest_version = $Version
$manifest.firmware.file_size = $SIZE
$manifest.firmware.sha256 = $SHA256
$manifest.firmware.release_date = $DATE
$manifest.firmware.release_notes_en = $Notes
$manifest.firmware.release_notes_de = $Notes
$manifest | ConvertTo-Json -Depth 5 | Set-Content $MANIFEST

# 6. Verify
Write-Host "[5/6] Verifying updates..." -ForegroundColor Green
Select-String "FIRMWARE_VERSION" $CONFIG_H
$m = Get-Content $MANIFEST | ConvertFrom-Json
Write-Host "Manifest FW: $($m.firmware.latest_version)"
Write-Host "Manifest SHA: $($m.firmware.sha256.Substring(0,16))..."
Write-Host "Manifest Size: $($m.firmware.file_size)"

# 7. Done
Write-Host "[6/6] Done!" -ForegroundColor Green
Write-Host ""
Write-Host "Next steps:"
Write-Host "  1. Build & flash new firmware to CYD:"
Write-Host "     cd esp32_firmware && pio run -t upload"
Write-Host ""
Write-Host "  2. Start PHP server:"
Write-Host "     php -S localhost:8080 -t php_server"
Write-Host ""
Write-Host "  3. Trigger OTA on CYD (tap center bridge dot or wait 15min auto-check)"
Write-Host ""
Write-Host "Server will now offer FW $Version to devices running older versions" -ForegroundColor Green