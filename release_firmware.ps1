#!/usr/bin/env pwsh
<#
.SYNOPSIS
    PetDuet Windows Firmware Release, Test, Coverage, OTA Deploy & Flash Pipeline
.DESCRIPTION
    1. Erhoeht die FIRMWARE_VERSION in esp32_firmware\config.h (patch/minor/major oder explizit)
    2. Kompiliert die ESP32-Firmware via PlatformIO (pio run)
    3. Fuehrt Unit-Tests aus und ermittelt Code-Coverage getrennt nach Firmware (C++/gcov) und Server (PHP)
    4. Kopiert die kompilierte Firmware + OTA-Versionsfile (ota_manifest.json) zum Server (php_server\firmware\)
    5. Fragt interaktiv nach, ob die Firmware geflasht werden soll
    6. Flasht die Firmware via PlatformIO auf das angeschlossene ESP32 CYD
.EXAMPLE
    .\release_firmware.ps1
    .\release_firmware.ps1 -Bump minor -Notes "Neues Chat-Menue & Bugfixes"
    .\release_firmware.ps1 -Version 2.2.0 -ServerPath "C:\xampp\htdocs\cyd-tamagotchi" -ComPort COM5
#>

[CmdletBinding()]
param(
    [ValidateSet("patch", "minor", "major")]
    [string]$Bump = "patch",

    [Parameter(Mandatory = $false)]
    [ValidatePattern('^\d+\.\d+\.\d+$')]
    [string]$Version = "",

    [Parameter(Mandatory = $false)]
    [string]$Notes = "",

    [Parameter(Mandatory = $false)]
    [string]$ServerPath = $env:PETDUET_SERVER_PATH,

    [Parameter(Mandatory = $false)]
    [string]$ComPort = "",

    [switch]$Flash,
    [switch]$NoFlash
)

$ErrorActionPreference = "Stop"

$ProjectRoot  = Split-Path -Parent $MyInvocation.MyCommand.Definition
$FirmwareDir  = Join-Path $ProjectRoot "esp32_firmware"
$ConfigH      = Join-Path $FirmwareDir "config.h"
$FirmwareBin  = Join-Path $FirmwareDir ".pio\build\esp32-2432s028r\firmware.bin"

$ServerDir        = Join-Path $ProjectRoot "php_server"
$ServerFwDir      = Join-Path $ServerDir "firmware"
$ServerBin        = Join-Path $ServerFwDir "cyd_tamagotchi.bin"
$ServerVerJson    = Join-Path $ServerFwDir "version.json"
$ManifestFile     = Join-Path $ServerDir "ota_manifest.json"
$ServerRulesFile  = Join-Path $ServerDir "rules.json"

$CoverageDir      = Join-Path $ProjectRoot "reports\coverage"
$FwTestSrc        = Join-Path $ProjectRoot "tests\firmware\test_firmware_engine.cpp"
$FwMockInc        = Join-Path $ProjectRoot "tests\firmware\mock_arduino"
$SrvTestRunner    = Join-Path $ProjectRoot "tests\server\run_tests.php"

$Utf8NoBom = New-Object System.Text.UTF8Encoding($false)

function Write-Step([string]$title) {
    Write-Host ""
    Write-Host "======================================================================" -ForegroundColor DarkCyan
    Write-Host " $title" -ForegroundColor Cyan
    Write-Host "======================================================================" -ForegroundColor DarkCyan
}

function Find-Tool([string]$exeName, [string[]]$extraCandidates) {
    $cmd = Get-Command $exeName -ErrorAction SilentlyContinue
    if ($cmd) { return $cmd.Source }
    foreach ($cand in $extraCandidates) {
        if ($cand -and (Test-Path $cand)) { return $cand }
    }
    return $null
}

$ConfigBackup = $null
$VersionChanged = $false

function Restore-ConfigOnFailure {
    if ($script:VersionChanged -and $null -ne $script:ConfigBackup) {
        [System.IO.File]::WriteAllText($ConfigH, $script:ConfigBackup, $Utf8NoBom)
        Write-Host "[Rollback] config.h wurde wegen Fehler auf die vorherige Version zurueckgesetzt." -ForegroundColor Yellow
    }
}

try {
    if (-not (Test-Path $ConfigH)) {
        throw "config.h nicht gefunden unter: $ConfigH"
    }

    New-Item -ItemType Directory -Path $CoverageDir -Force | Out-Null

    # ========================================================================
    # SCHRITT 1: FIRMWARE-VERSION ERHOEHEN
    # ========================================================================
    Write-Step "[1/6] Firmware-Version in config.h erhoehen"

    $ConfigBackup = [System.IO.File]::ReadAllText($ConfigH, $Utf8NoBom)
    $verMatch = [regex]::Match($ConfigBackup, '#define\s+FIRMWARE_VERSION\s+"(\d+\.\d+\.\d+)"')
    if (-not $verMatch.Success) {
        throw "Konnte #define FIRMWARE_VERSION `"x.y.z`" in $ConfigH nicht finden."
    }
    $OldVersion = $verMatch.Groups[1].Value

    $rulesMatch = [regex]::Match($ConfigBackup, '#define\s+RULES_VERSION\s+"([^"]+)"')
    $RulesVersion = if ($rulesMatch.Success) { $rulesMatch.Groups[1].Value } else { "1.0.4" }

    if (-not [string]::IsNullOrWhiteSpace($Version)) {
        $NewVersion = $Version
    } else {
        $parts = $OldVersion.Split('.') | ForEach-Object { [int]$_ }
        switch ($Bump) {
            "major" { $parts[0]++; $parts[1] = 0; $parts[2] = 0 }
            "minor" { $parts[1]++; $parts[2] = 0 }
            default { $parts[2]++ }
        }
        $NewVersion = "$($parts[0]).$($parts[1]).$($parts[2])"
    }

    if ([string]::IsNullOrWhiteSpace($Notes)) {
        $Notes = "Firmware Release v$NewVersion"
    }

    $updatedConfig = [regex]::Replace(
        $ConfigBackup,
        '(#define\s+FIRMWARE_VERSION\s+")(\d+\.\d+\.\d+)(")',
        ('${1}' + $NewVersion + '${3}')
    )
    [System.IO.File]::WriteAllText($ConfigH, $updatedConfig, $Utf8NoBom)
    $VersionChanged = $true

    Write-Host "  Alte Version : $OldVersion" -ForegroundColor Gray
    Write-Host "  Neue Version : $NewVersion ($Bump)" -ForegroundColor Green
    Write-Host "  Rules Version: $RulesVersion" -ForegroundColor Gray

    # ========================================================================
    # SCHRITT 2: FIRMWARE KOMPILIEREN (PlatformIO)
    # ========================================================================
    Write-Step "[2/6] ESP32-Firmware kompilieren (PlatformIO)"

    $pioExe = Find-Tool "pio" @(
        (Join-Path $env:USERPROFILE ".platformio\penv\Scripts\pio.exe"),
        (Join-Path $env:LOCALAPPDATA "Programs\PlatformIO\penv\Scripts\pio.exe"),
        "C:\Users\$env:USERNAME\.platformio\penv\Scripts\pio.exe"
    )
    if (-not $pioExe) {
        throw "PlatformIO CLI ('pio') nicht gefunden. Bitte PlatformIO Core installieren oder PATH ergaenzen."
    }
    Write-Host "  PlatformIO: $pioExe" -ForegroundColor Gray

    Push-Location $FirmwareDir
    try {
        & $pioExe run -e esp32-2432s028r
        if ($LASTEXITCODE -ne 0) {
            throw "Firmware-Kompilierung fehlgeschlagen (Exit Code $LASTEXITCODE)."
        }
    } finally {
        Pop-Location
    }

    if (-not (Test-Path $FirmwareBin)) {
        throw "Kompilierte Binaerdatei nicht gefunden: $FirmwareBin"
    }
    $binInfo = Get-Item $FirmwareBin
    $binHash = (Get-FileHash -Algorithm SHA256 $FirmwareBin).Hash.ToLowerInvariant()
    Write-Host ("  Build OK: {0} ({1} Bytes, SHA256: {2}...)" -f $FirmwareBin, $binInfo.Length, $binHash.Substring(0, 16)) -ForegroundColor Green

    # ========================================================================
    # SCHRITT 3: UNIT-TESTS & GETRENNTE CODE-COVERAGE (FIRMWARE & SERVER)
    # ========================================================================
    Write-Step "[3/6] Unit-Tests & Code-Coverage (getrennt nach Firmware & Server)"

    # --- 3A: Firmware Unit Tests + gcov Coverage ---
    Write-Host "`n--- [3A] Firmware Unit-Tests & Code-Coverage (C++ / gcov) ---" -ForegroundColor Yellow

    $gppExe = Find-Tool "g++" @(
        "C:\msys64\ucrt64\bin\g++.exe",
        "C:\msys64\mingw64\bin\g++.exe",
        "C:\mingw64\bin\g++.exe",
        "C:\WinLibs\mingw64\bin\g++.exe",
        "C:\ProgramData\chocolatey\bin\g++.exe"
    )
    if (-not $gppExe) {
        throw "Kein Host C++ Compiler (g++) fuer Firmware-Tests/Coverage gefunden (z.B. MSYS2/MinGW/WinLibs g++.exe)."
    }

    $gcovExe = Find-Tool "gcov" @(
        (Join-Path (Split-Path $gppExe -Parent) "gcov.exe")
    )
    if (-not $gcovExe) {
        throw "gcov.exe nicht gefunden (wird fuer Firmware Code-Coverage benoetigt)."
    }

    $fwCovWorkDir = Join-Path $env:TEMP ("petduet_fw_cov_" + $PID)
    if (Test-Path $fwCovWorkDir) { Remove-Item $fwCovWorkDir -Recurse -Force }
    New-Item -ItemType Directory -Path $fwCovWorkDir -Force | Out-Null

    $fwTestExe = Join-Path $fwCovWorkDir "test_firmware_engine.exe"
    $fwCovLinePct = $null
    $fwExecLines = 0
    $fwTotalLines = 0
    $fwAssertionsPassed = $false

    Push-Location $fwCovWorkDir
    try {
        # Ensure g++ DLLs (e.g. libstdc++-6.dll) are resolvable at runtime
        $oldPath = $env:PATH
        $gppDir = Split-Path $gppExe -Parent
        $env:PATH = "$gppDir;$oldPath"

        & $gppExe -std=c++17 -O0 -g --coverage "-I$FwMockInc" "-I$FirmwareDir" "$FwTestSrc" -o "$fwTestExe"
        if ($LASTEXITCODE -ne 0) {
            throw "Kompilierung der Firmware-Unit-Tests fehlgeschlagen."
        }

        & $fwTestExe
        if ($LASTEXITCODE -ne 0) {
            throw "Firmware-Unit-Tests sind fehlgeschlagen!"
        }
        $fwAssertionsPassed = $true

        # Run gcov to collect coverage strictly for TamagotchiCoreEngine.h
        $gcdaItem = Get-ChildItem -Path $fwCovWorkDir -Filter "*.gcda" | Select-Object -First 1
        if (-not $gcdaItem) {
            throw "Keine .gcda Coverage-Daten von g++ erzeugt."
        }
        $gcovOut = & $gcovExe -b -c "$($gcdaItem.FullName)" 2>&1

        $gcovHeaderFile = Get-ChildItem -Path $fwCovWorkDir -Filter "*TamagotchiCoreEngine.h.gcov" | Select-Object -First 1
        if ($gcovHeaderFile) {
            $lines = Get-Content $gcovHeaderFile.FullName
            foreach ($ln in $lines) {
                if ($ln -match '^\s*([^:]+):\s*(\d+):') {
                    $cnt = $matches[1].Trim()
                    if ($cnt -eq '-' -or $cnt -eq '') { continue }
                    $fwTotalLines++
                    if ($cnt -ne '#####' -and $cnt -ne '=====') {
                        $fwExecLines++
                    }
                }
            }
            if ($fwTotalLines -gt 0) {
                $fwCovLinePct = [math]::Round(($fwExecLines / $fwTotalLines) * 100.0, 2)
            }
        }

        $fwReportText = @(
            "PetDuet Firmware Unit-Test & Code-Coverage Report"
            "================================================"
            "Compiler       : $gppExe"
            "Target Header  : esp32_firmware\TamagotchiCoreEngine.h"
            "Tests Passed   : $fwAssertionsPassed"
            "Line Coverage  : $fwCovLinePct% ($fwExecLines / $fwTotalLines ausfuehrbare Zeilen)"
        ) -join [Environment]::NewLine

        $fwTxtOut  = Join-Path $CoverageDir "firmware_coverage.txt"
        $fwJsonOut = Join-Path $CoverageDir "firmware_coverage.json"
        [System.IO.File]::WriteAllText($fwTxtOut, $fwReportText + [Environment]::NewLine, $Utf8NoBom)

        $fwJsonObj = [ordered]@{
            component = "firmware"
            target = "esp32_firmware/TamagotchiCoreEngine.h"
            tests_passed = $fwAssertionsPassed
            line_coverage_pct = $fwCovLinePct
            executed_lines = $fwExecLines
            executable_lines = $fwTotalLines
        } | ConvertTo-Json -Depth 4
        [System.IO.File]::WriteAllText($fwJsonOut, $fwJsonObj, $Utf8NoBom)

        Write-Host ("  Firmware Coverage (TamagotchiCoreEngine.h): {0}% ({1}/{2} Zeilen)" -f $fwCovLinePct, $fwExecLines, $fwTotalLines) -ForegroundColor Green
    } finally {
        if ($oldPath) { $env:PATH = $oldPath }
        Pop-Location
        if (Test-Path $fwCovWorkDir) { Remove-Item $fwCovWorkDir -Recurse -Force -ErrorAction SilentlyContinue }
    }

    # --- 3B: Server Unit Tests + PHP Coverage ---
    Write-Host "`n--- [3B] Server Unit-Tests & Code-Coverage (PHP Server Endpoints) ---" -ForegroundColor Yellow

    $laragonPhp = Get-ChildItem -Path "C:\laragon\bin\php" -Filter "php.exe" -Recurse -ErrorAction SilentlyContinue | Select-Object -First 1 -ExpandProperty FullName
    $phpExe = Find-Tool "php" @(
        "C:\php\php.exe",
        "C:\xampp\php\php.exe",
        $laragonPhp,
        "C:\tools\php\php.exe"
    )
    if (-not $phpExe) {
        throw "PHP CLI ('php.exe') nicht gefunden. Bitte PHP installieren oder in PATH aufnehmen."
    }

    & $phpExe "$SrvTestRunner" "--cov-dir=$CoverageDir"
    if ($LASTEXITCODE -ne 0) {
        throw "Server-Unit-Tests sind fehlgeschlagen (Exit Code $LASTEXITCODE)!"
    }

    $srvJsonPath = Join-Path $CoverageDir "server_coverage.json"
    $srvCov = Get-Content $srvJsonPath -Raw | ConvertFrom-Json

    # Getrennte Zusammenfassung ausgeben
    Write-Host "`n=== GETRENNTE CODE-COVERAGE ZUSAMMENFASSUNG ===" -ForegroundColor Cyan
    Write-Host ("  1. FIRMWARE (TamagotchiCoreEngine.h) : {0}% ({1}/{2} Zeilen)" -f $fwCovLinePct, $fwExecLines, $fwTotalLines) -ForegroundColor Green
    if ($null -ne $srvCov.line_coverage_pct) {
        Write-Host ("  2. SERVER   (php_server/*.php)       : {0}% ({1}/{2} Zeilen, Treiber: {3}) | Endpoints: {4}%" -f $srvCov.line_coverage_pct, $srvCov.executed_lines, $srvCov.executable_lines, $srvCov.coverage_driver, $srvCov.endpoint_coverage_pct) -ForegroundColor Green
    } else {
        Write-Host ("  2. SERVER   (php_server/*.php)       : Endpoint-Coverage {0}% ({1}/{2} PHP-Skripte) | Zeilen-Coverage: PCOV/Xdebug nicht aktiv" -f $srvCov.endpoint_coverage_pct, $srvCov.tested_endpoints, $srvCov.total_endpoints) -ForegroundColor Yellow
    }
    Write-Host "  Reports gespeichert in: $CoverageDir" -ForegroundColor Gray

    # ========================================================================
    # SCHRITT 4: FIRMWARE MIT VERSIONSFILE OTA-FAEHIG ZUM SERVER KOPIEREN
    # ========================================================================
    Write-Step "[4/6] Firmware & Versionsfile (OTA-faehig) zum Server kopieren"

    New-Item -ItemType Directory -Path $ServerFwDir -Force | Out-Null
    Copy-Item -Path $FirmwareBin -Destination $ServerBin -Force

    $today = Get-Date -Format "yyyy-MM-dd"
    $manifest = Get-Content $ManifestFile -Raw | ConvertFrom-Json

    $manifest.firmware.latest_version   = $NewVersion
    $manifest.firmware.binary_filename  = "cyd_tamagotchi.bin"
    $manifest.firmware.download_url     = "ota_download.php?type=firmware"
    $manifest.firmware.file_size        = [int64]$binInfo.Length
    $manifest.firmware.sha256           = $binHash
    $manifest.firmware.release_date     = $today
    $manifest.firmware.release_notes_en = $Notes
    $manifest.firmware.release_notes_de = $Notes

    if (Test-Path $ServerRulesFile) {
        $rulesInfo = Get-Item $ServerRulesFile
        $rulesHash = (Get-FileHash -Algorithm SHA256 $ServerRulesFile).Hash.ToLowerInvariant()
        $manifest.rules.latest_version   = $RulesVersion
        $manifest.rules.file_size        = [int64]$rulesInfo.Length
        $manifest.rules.sha256           = $rulesHash
        $manifest.rules.release_date     = $today
    }

    $manifestJson = $manifest | ConvertTo-Json -Depth 6
    [System.IO.File]::WriteAllText($ManifestFile, $manifestJson + [Environment]::NewLine, $Utf8NoBom)

    # Begleitendes Versionsfile direkt neben dem Binary im Server-Firmware-Ordner
    $verMeta = [ordered]@{
        firmware_version = $NewVersion
        rules_version    = $RulesVersion
        binary_filename  = "cyd_tamagotchi.bin"
        file_size        = [int64]$binInfo.Length
        sha256           = $binHash
        release_date     = $today
        notes            = $Notes
    } | ConvertTo-Json -Depth 4
    [System.IO.File]::WriteAllText($ServerVerJson, $verMeta + [Environment]::NewLine, $Utf8NoBom)

    Write-Host "  OTA Binary  : $ServerBin" -ForegroundColor Green
    Write-Host "  OTA Manifest: $ManifestFile" -ForegroundColor Green
    Write-Host "  Version-File: $ServerVerJson" -ForegroundColor Green

    # Optional: Auf externes Server-Zielverzeichnis (z.B. XAMPP/htdocs oder Netzlaufwerk) deployen
    if (-not [string]::IsNullOrWhiteSpace($ServerPath)) {
        Write-Host "  Kopiere OTA-Serverpaket nach: $ServerPath" -ForegroundColor Cyan
        $extFwDir = Join-Path $ServerPath "firmware"
        New-Item -ItemType Directory -Path $extFwDir -Force | Out-Null
        Copy-Item -Path $ServerBin     -Destination (Join-Path $extFwDir "cyd_tamagotchi.bin") -Force
        Copy-Item -Path $ServerVerJson -Destination (Join-Path $extFwDir "version.json")       -Force
        Copy-Item -Path $ManifestFile  -Destination (Join-Path $ServerPath "ota_manifest.json") -Force
        if (Test-Path $ServerRulesFile) {
            Copy-Item -Path $ServerRulesFile -Destination (Join-Path $ServerPath "rules.json") -Force
        }
        foreach ($phpFile in @("ota_check.php", "ota_download.php", "index.php")) {
            $srcPhp = Join-Path $ServerDir $phpFile
            if (Test-Path $srcPhp) {
                Copy-Item -Path $srcPhp -Destination (Join-Path $ServerPath $phpFile) -Force
            }
        }
        Write-Host "  Externes Server-Deployment abgeschlossen." -ForegroundColor Green
    }

    # Build & Tests waren erfolgreich -> kein Rollback mehr noetig
    $VersionChanged = $false

    # ========================================================================
    # SCHRITT 5: FRAGEN, OB DIE FIRMWARE GEFLASHT WERDEN SOLL
    # ========================================================================
    Write-Step "[5/6] Flashen auf ESP32 CYD bestaetigen"

    $doFlash = $false
    if ($Flash) {
        $doFlash = $true
        Write-Host "  Parameter -Flash gesetzt: Starte Flash-Vorgang..." -ForegroundColor Green
    } elseif ($NoFlash) {
        $doFlash = $false
        Write-Host "  Parameter -NoFlash gesetzt: Flash wird uebersprungen." -ForegroundColor Yellow
    } else {
        $answer = Read-Host "  Soll die Firmware v$NewVersion jetzt per USB auf das ESP32 CYD geflasht werden? [j/N]"
        if ($answer -match '^(j|ja|y|yes)$') {
            $doFlash = $true
        }
    }

    # ========================================================================
    # SCHRITT 6: FIRMWARE FLASHEN
    # ========================================================================
    Write-Step "[6/6] Firmware flashen"

    if ($doFlash) {
        Push-Location $FirmwareDir
        try {
            $uploadArgs = @("run", "-e", "esp32-2432s028r", "-t", "upload")
            if (-not [string]::IsNullOrWhiteSpace($ComPort)) {
                $uploadArgs += @("--upload-port", $ComPort)
            }
            Write-Host "  Starte Upload: $pioExe $($uploadArgs -join ' ')" -ForegroundColor Cyan
            & $pioExe @uploadArgs
            if ($LASTEXITCODE -ne 0) {
                throw "Flashen fehlgeschlagen (Exit Code $LASTEXITCODE)."
            }
            Write-Host "`n  [ERFOLG] Firmware v$NewVersion erfolgreich auf ESP32 CYD geflasht!" -ForegroundColor Green
        } finally {
            Pop-Location
        }
    } else {
        Write-Host "  USB-Flash uebersprungen. Die OTA-Dateien stehen auf dem Server bereit." -ForegroundColor Yellow
    }

    Write-Host "`nRelease-Pipeline v$NewVersion erfolgreich abgeschlossen." -ForegroundColor Green
    exit 0
} catch {
    Write-Host "`n[FEHLER] $($_.Exception.Message)" -ForegroundColor Red
    Restore-ConfigOnFailure
    exit 1
}
