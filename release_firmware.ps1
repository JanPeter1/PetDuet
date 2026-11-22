#!/usr/bin/env pwsh
<#
.SYNOPSIS
    PetDuet Windows Firmware Release, Test, Coverage, OTA Deploy & Flash Pipeline (ESP-IDF + LVGL)
.DESCRIPTION
    1. Erhoeht FIRMWARE_VERSION in esp32_firmware\config.h & PROJECT_VER in esp32_firmware\CMakeLists.txt
    2. Kompiliert die ESP32-Firmware nativ via ESP-IDF (idf.py build) oder PlatformIO (pio run, framework = espidf)
    3. Fuehrt Unit-Tests aus (mit -Coverage auch Code-Coverage getrennt nach Firmware C++/gcov & PHP Server)
    4. Kopiert Binary + OTA-Versionsfile zum Server (php_server\cyd_tamagotchi.bin, php_server\firmware\, ota_manifest.json)
    5. Fragt interaktiv nach, ob die Firmware geflasht werden soll
    6. Flasht die Firmware via idf.py flash oder PlatformIO auf das angeschlossene ESP32 CYD
.EXAMPLE
    .\release_firmware.ps1
    .\release_firmware.ps1 -Coverage -Bump minor -Notes "Migration auf natives ESP-IDF + LVGL"
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
    [switch]$NoFlash,
    [switch]$Coverage,
    [switch]$PreferPio
)

$ErrorActionPreference = "Stop"

$ProjectRoot  = Split-Path -Parent $MyInvocation.MyCommand.Definition
$FirmwareDir  = Join-Path $ProjectRoot "esp32_firmware"
$ConfigH      = Join-Path $FirmwareDir "config.h"
$RootCMake    = Join-Path $FirmwareDir "CMakeLists.txt"

$IdfBin       = Join-Path $FirmwareDir "build\petduet.bin"
$PioBin       = Join-Path $FirmwareDir ".pio\build\esp32-2432s028r\firmware.bin"

$ServerDir        = Join-Path $ProjectRoot "php_server"
$ServerFwDir      = Join-Path $ServerDir "firmware"
$ServerRootBin    = Join-Path $ServerDir "cyd_tamagotchi.bin"
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

function Ensure-IdfEnvironment {
    if (Get-Command "idf.py" -ErrorAction SilentlyContinue) {
        return $true
    }
    $idfCandidates = @()
    if ($env:IDF_PATH) { $idfCandidates += $env:IDF_PATH }
    if (Test-Path "C:\Espressif\frameworks") {
        $dirs = Get-ChildItem "C:\Espressif\frameworks" -Directory -Filter "esp-idf*" -ErrorAction SilentlyContinue | Sort-Object Name -Descending
        foreach ($d in $dirs) { $idfCandidates += $d.FullName }
    }
    if (Test-Path "$env:USERPROFILE\esp") {
        $dirs = Get-ChildItem "$env:USERPROFILE\esp" -Directory -Filter "esp-idf*" -ErrorAction SilentlyContinue | Sort-Object Name -Descending
        foreach ($d in $dirs) { $idfCandidates += $d.FullName }
    }

    foreach ($cand in $idfCandidates) {
        $exportPs1 = Join-Path $cand "export.ps1"
        if (Test-Path $exportPs1) {
            Write-Host "  Lade ESP-IDF Umgebung: $exportPs1" -ForegroundColor Gray
            $env:IDF_PATH = $cand
            & $exportPs1 | Out-Null
            if (Get-Command "idf.py" -ErrorAction SilentlyContinue) {
                return $true
            }
        }
    }
    return $false
}

$ConfigBackup = $null
$CMakeBackup  = $null
$VersionChanged = $false

function Restore-ConfigOnFailure {
    if ($script:VersionChanged) {
        if ($null -ne $script:ConfigBackup) {
            [System.IO.File]::WriteAllText($ConfigH, $script:ConfigBackup, $Utf8NoBom)
        }
        if ($null -ne $script:CMakeBackup -and (Test-Path $RootCMake)) {
            [System.IO.File]::WriteAllText($RootCMake, $script:CMakeBackup, $Utf8NoBom)
        }
        Write-Host "[Rollback] config.h & CMakeLists.txt auf vorherige Version zurueckgesetzt." -ForegroundColor Yellow
    }
}

try {
    if (-not (Test-Path $ConfigH)) {
        throw "config.h nicht gefunden unter: $ConfigH"
    }

    New-Item -ItemType Directory -Path $CoverageDir -Force | Out-Null

    # ========================================================================
    # SCHRITT 1: FIRMWARE-VERSION ERHOEHEN (config.h & CMakeLists.txt)
    # ========================================================================
    Write-Step "[1/6] Firmware-Version in config.h & CMakeLists.txt erhoehen"

    $ConfigBackup = [System.IO.File]::ReadAllText($ConfigH, $Utf8NoBom)
    if (Test-Path $RootCMake) {
        $CMakeBackup = [System.IO.File]::ReadAllText($RootCMake, $Utf8NoBom)
    }

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
        $Notes = "ESP-IDF Release v$NewVersion"
    }

    $updatedConfig = [regex]::Replace(
        $ConfigBackup,
        '(#define\s+FIRMWARE_VERSION\s+")(\d+\.\d+\.\d+)(")',
        ('${1}' + $NewVersion + '${3}')
    )
    [System.IO.File]::WriteAllText($ConfigH, $updatedConfig, $Utf8NoBom)

    if ($null -ne $CMakeBackup) {
        $updatedCMake = [regex]::Replace(
            $CMakeBackup,
            '(set\s*\(\s*PROJECT_VER\s+")(\d+\.\d+\.\d+)("\s*\))',
            ('${1}' + $NewVersion + '${3}')
        )
        [System.IO.File]::WriteAllText($RootCMake, $updatedCMake, $Utf8NoBom)
    }
    $VersionChanged = $true

    Write-Host "  Alte Version : $OldVersion" -ForegroundColor Gray
    Write-Host "  Neue Version : $NewVersion ($Bump)" -ForegroundColor Green
    Write-Host "  Rules Version: $RulesVersion" -ForegroundColor Gray

    # ========================================================================
    # SCHRITT 2: FIRMWARE KOMPILIEREN (ESP-IDF idf.py oder PlatformIO espidf)
    # ========================================================================
    Write-Step "[2/6] ESP32-Firmware kompilieren (ESP-IDF + LVGL)"

    $buildTool = ""
    $FirmwareBin = ""
    $hasIdf = if ($PreferPio) { $false } else { Ensure-IdfEnvironment }

    if ($hasIdf) {
        $buildTool = "idf.py"
        Write-Host "  Compiler: ESP-IDF (idf.py)" -ForegroundColor Gray
        Push-Location $FirmwareDir
        try {
            & idf.py build
            if ($LASTEXITCODE -ne 0) {
                throw "ESP-IDF Kompilierung fehlgeschlagen (Exit Code $LASTEXITCODE). Falls ESP-IDF aktualisiert wurde: 'cd esp32_firmware; idf.py fullclean' ausfuehren."
            }
        } finally {
            Pop-Location
        }
        $FirmwareBin = $IdfBin
    } else {
        $pioExe = Find-Tool "pio" @(
            (Join-Path $env:USERPROFILE ".platformio\penv\Scripts\pio.exe"),
            (Join-Path $env:LOCALAPPDATA "Programs\PlatformIO\penv\Scripts\pio.exe"),
            "C:\Users\$env:USERNAME\.platformio\penv\Scripts\pio.exe"
        )
        if (-not $pioExe) {
            throw "Weder ESP-IDF ('idf.py') noch PlatformIO ('pio') gefunden. Bitte ESP-IDF oder PlatformIO installieren."
        }
        $buildTool = "pio"
        Write-Host "  Compiler: PlatformIO ESP-IDF ($pioExe)" -ForegroundColor Gray
        Push-Location $FirmwareDir
        try {
            & $pioExe run -e esp32-2432s028r
            if ($LASTEXITCODE -ne 0) {
                throw "PlatformIO ESP-IDF Kompilierung fehlgeschlagen (Exit Code $LASTEXITCODE)."
            }
        } finally {
            Pop-Location
        }
        $FirmwareBin = $PioBin
    }

    if (-not (Test-Path $FirmwareBin)) {
        throw "Kompilierte Binaerdatei nicht gefunden: $FirmwareBin"
    }
    $binInfo = Get-Item $FirmwareBin
    $binHash = (Get-FileHash -Algorithm SHA256 $FirmwareBin).Hash.ToLowerInvariant()
    Write-Host ("  Build OK ({0}): {1} ({2} Bytes, SHA256: {3}...)" -f $buildTool, $FirmwareBin, $binInfo.Length, $binHash.Substring(0, 16)) -ForegroundColor Green

    # ========================================================================
    # SCHRITT 3: UNIT-TESTS (+ OPTIONAL -Coverage GETRENNT NACH FW & SERVER)
    # ========================================================================
    Write-Step "[3/6] Unit-Tests (Firmware C++ Engine & PHP Server)"

    $fwAssertionsPassed = $false
    $fwCovLinePct = $null
    $fwExecLines = 0
    $fwTotalLines = 0

    $gppExe = Find-Tool "g++" @(
        "C:\msys64\ucrt64\bin\g++.exe",
        "C:\msys64\mingw64\bin\g++.exe",
        "C:\mingw64\bin\g++.exe",
        "C:\WinLibs\mingw64\bin\g++.exe",
        "C:\ProgramData\chocolatey\bin\g++.exe"
    )

    if ($Coverage) {
        Write-Host "  [Coverage-Modus aktiviert]" -ForegroundColor Yellow
        if (-not $gppExe) {
            throw "Kein Host C++ Compiler (g++) fuer Firmware-Tests/Coverage gefunden."
        }
        $gcovExe = Find-Tool "gcov" @((Join-Path (Split-Path $gppExe -Parent) "gcov.exe"))
        if (-not $gcovExe) {
            throw "gcov.exe nicht gefunden."
        }

        $fwCovWorkDir = Join-Path $env:TEMP ("petduet_fw_cov_" + $PID)
        if (Test-Path $fwCovWorkDir) { Remove-Item $fwCovWorkDir -Recurse -Force }
        New-Item -ItemType Directory -Path $fwCovWorkDir -Force | Out-Null
        $fwTestExe = Join-Path $fwCovWorkDir "test_firmware_engine.exe"

        Push-Location $fwCovWorkDir
        try {
            $oldPath = $env:PATH
            $env:PATH = "$(Split-Path $gppExe -Parent);$oldPath"

            & $gppExe -std=c++17 -O0 -g --coverage "-I$FwMockInc" "-I$FirmwareDir" "$FwTestSrc" -o "$fwTestExe"
            if ($LASTEXITCODE -ne 0) { throw "Kompilierung der Firmware-Unit-Tests fehlgeschlagen." }
            & $fwTestExe
            if ($LASTEXITCODE -ne 0) { throw "Firmware-Unit-Tests fehlgeschlagen!" }
            $fwAssertionsPassed = $true

            $gcdaItem = Get-ChildItem -Path $fwCovWorkDir -Filter "*.gcda" | Select-Object -First 1
            if ($gcdaItem) {
                & $gcovExe -b -c "$($gcdaItem.FullName)" 2>&1 | Out-Null
            }
            $gcovHeaderFile = Get-ChildItem -Path $fwCovWorkDir -Filter "*TamagotchiCoreEngine.h.gcov" | Select-Object -First 1
            if ($gcovHeaderFile) {
                foreach ($ln in (Get-Content $gcovHeaderFile.FullName)) {
                    if ($ln -match '^\s*([^:]+):\s*(\d+):') {
                        $cnt = $matches[1].Trim()
                        if ($cnt -eq '-' -or $cnt -eq '') { continue }
                        $fwTotalLines++
                        if ($cnt -ne '#####' -and $cnt -ne '=====') { $fwExecLines++ }
                    }
                }
                if ($fwTotalLines -gt 0) { $fwCovLinePct = [math]::Round(($fwExecLines / $fwTotalLines) * 100.0, 2) }
            }

            $fwJsonObj = [ordered]@{
                component = "firmware"
                target = "esp32_firmware/TamagotchiCoreEngine.h"
                tests_passed = $fwAssertionsPassed
                line_coverage_pct = $fwCovLinePct
                executed_lines = $fwExecLines
                executable_lines = $fwTotalLines
            } | ConvertTo-Json -Depth 4
            [System.IO.File]::WriteAllText((Join-Path $CoverageDir "firmware_coverage.json"), $fwJsonObj, $Utf8NoBom)
            Write-Host ("  Firmware Coverage (TamagotchiCoreEngine.h): {0}% ({1}/{2} Zeilen)" -f $fwCovLinePct, $fwExecLines, $fwTotalLines) -ForegroundColor Green
        } finally {
            if ($oldPath) { $env:PATH = $oldPath }
            Pop-Location
            Remove-Item $fwCovWorkDir -Recurse -Force -ErrorAction SilentlyContinue
        }
    } else {
        if ($gppExe) {
            $fwTestWorkDir = Join-Path $env:TEMP ("petduet_fw_test_" + $PID)
            New-Item -ItemType Directory -Path $fwTestWorkDir -Force | Out-Null
            $fwTestExe = Join-Path $fwTestWorkDir "test_firmware_engine.exe"
            try {
                $oldPath = $env:PATH
                $env:PATH = "$(Split-Path $gppExe -Parent);$oldPath"
                & $gppExe -std=c++17 -O0 "-I$FwMockInc" "-I$FirmwareDir" "$FwTestSrc" -o "$fwTestExe"
                if ($LASTEXITCODE -ne 0) { throw "Kompilierung der Firmware-Unit-Tests fehlgeschlagen." }
                & $fwTestExe
                if ($LASTEXITCODE -ne 0) { throw "Firmware-Unit-Tests fehlgeschlagen!" }
                Write-Host "  Firmware Tests: PASSED" -ForegroundColor Green
            } finally {
                if ($oldPath) { $env:PATH = $oldPath }
                Remove-Item $fwTestWorkDir -Recurse -Force -ErrorAction SilentlyContinue
            }
        } else {
            Write-Host "  Firmware Tests: UEBERPRUNGEN (kein g++ gefunden)" -ForegroundColor Yellow
        }
    }

    $laragonPhp = Get-ChildItem -Path "C:\laragon\bin\php" -Filter "php.exe" -Recurse -ErrorAction SilentlyContinue | Select-Object -First 1 -ExpandProperty FullName
    $phpExe = Find-Tool "php" @("C:\php\php.exe", "C:\xampp\php\php.exe", $laragonPhp, "C:\tools\php\php.exe")
    if ($phpExe) {
        if ($Coverage) {
            & $phpExe "$SrvTestRunner" "--cov-dir=$CoverageDir"
        } else {
            & $phpExe "$SrvTestRunner"
        }
        if ($LASTEXITCODE -ne 0) { throw "Server-Unit-Tests fehlgeschlagen (Exit Code $LASTEXITCODE)!" }
        Write-Host "  Server Tests: PASSED" -ForegroundColor Green
    } else {
        Write-Host "  Server Tests: UEBERPRUNGEN (kein PHP gefunden)" -ForegroundColor Yellow
    }

    # ========================================================================
    # SCHRITT 4: FIRMWARE MIT VERSIONSFILE OTA-FAEHIG ZUM SERVER KOPIEREN
    # ========================================================================
    Write-Step "[4/6] Firmware & Versionsfile (OTA-faehig) zum Server kopieren"

    New-Item -ItemType Directory -Path $ServerFwDir -Force | Out-Null
    Copy-Item -Path $FirmwareBin -Destination $ServerRootBin -Force
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

    $verMeta = [ordered]@{
        firmware_version = $NewVersion
        rules_version    = $RulesVersion
        binary_filename  = "cyd_tamagotchi.bin"
        framework        = "esp-idf-lvgl"
        file_size        = [int64]$binInfo.Length
        sha256           = $binHash
        release_date     = $today
        notes            = $Notes
    } | ConvertTo-Json -Depth 4
    [System.IO.File]::WriteAllText($ServerVerJson, $verMeta + [Environment]::NewLine, $Utf8NoBom)

    Write-Host "  OTA Binary  : $ServerRootBin" -ForegroundColor Green
    Write-Host "  OTA Manifest: $ManifestFile" -ForegroundColor Green
    Write-Host "  Version-File: $ServerVerJson" -ForegroundColor Green

    if (-not [string]::IsNullOrWhiteSpace($ServerPath)) {
        Write-Host "  Kopiere OTA-Serverpaket nach: $ServerPath" -ForegroundColor Cyan
        $extFwDir = Join-Path $ServerPath "firmware"
        New-Item -ItemType Directory -Path $extFwDir -Force | Out-Null
        Copy-Item -Path $ServerRootBin -Destination (Join-Path $ServerPath "cyd_tamagotchi.bin") -Force
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
    Write-Step "[6/6] Firmware flashen ($buildTool)"

    if ($doFlash) {
        Push-Location $FirmwareDir
        try {
            if ($buildTool -eq "idf.py") {
                $idfArgs = @()
                if (-not [string]::IsNullOrWhiteSpace($ComPort)) {
                    $idfArgs += @("-p", $ComPort)
                }
                $idfArgs += @("flash")
                Write-Host "  Starte Upload: idf.py $($idfArgs -join ' ')" -ForegroundColor Cyan
                & idf.py @idfArgs
            } else {
                $uploadArgs = @("run", "-e", "esp32-2432s028r", "-t", "upload")
                if (-not [string]::IsNullOrWhiteSpace($ComPort)) {
                    $uploadArgs += @("--upload-port", $ComPort)
                }
                Write-Host "  Starte Upload: $pioExe $($uploadArgs -join ' ')" -ForegroundColor Cyan
                & $pioExe @uploadArgs
            }
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

    Write-Host "`nRelease-Pipeline v$NewVersion abgeschlossen." -ForegroundColor Green
    exit 0
} catch {
    Write-Host "`n[FEHLER] $($_.Exception.Message)" -ForegroundColor Red
    Restore-ConfigOnFailure
    exit 1
}
