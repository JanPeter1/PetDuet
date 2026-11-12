@echo off
REM PetDuet OTA Update Helper Script (Windows Batch)
REM Usage: ota_update.bat <new_version> [notes]
REM Example: ota_update.bat 2.1.2 "Fixed touch calibration"

setlocal enabledelayedexpansion

set PROJECT_ROOT=%~dp0
set FIRMWARE_BIN=%PROJECT_ROOT%esp32_firmware\.pio\build\esp32-2432s028r\firmware.bin
set SERVER_BIN=%PROJECT_ROOT%php_server\cyd_tamagotchi.bin
set MANIFEST=%PROJECT_ROOT%php_server\ota_manifest.json
set CONFIG_H=%PROJECT_ROOT%esp32_firmware\config.h

set NEW_VERSION=%1
set "NOTES=%~2"
if "%NOTES%"=="" (
    set NOTES=Firmware update
)

if "%NEW_VERSION%"=="" (
    echo Error: Version required
    echo Usage: ota_update.bat ^<version^> [notes]
    echo Example: ota_update.bat 2.1.2 "Fixed touch calibration"
    exit /b 1
)

echo === PetDuet OTA Update: %NEW_VERSION% ===
echo.

REM 1. Check firmware binary exists
if not exist "%FIRMWARE_BIN%" (
    echo Error: Firmware binary not found at %FIRMWARE_BIN%
    echo Build the firmware first: cd esp32_firmware && pio run
    exit /b 1
)

REM 2. Copy firmware to server
echo [1/6] Copying firmware to PHP server...
copy /Y "%FIRMWARE_BIN%" "%SERVER_BIN%"

REM 3. Calculate SHA256 and size (requires certutil or PowerShell)
echo [2/6] Calculating SHA256 and size...
for /f "delims=" %%a in ('powershell -Command "Get-FileHash -Algorithm SHA256 '%SERVER_BIN%' | Select-Object -ExpandProperty Hash"') do set SHA256=%%a
for /f "delims=" %%a in ('powershell -Command "(Get-Item '%SERVER_BIN%').Length"') do set SIZE=%%a

for /f "tokens=2 delims==" %%a in ('wmic os get localdatetime /value') do set DT=%%a
set DATE=%DT:~0,4%-%DT:~4,2%-%DT:~6,2%

echo SHA256: %SHA256%
echo Size:   %SIZE% bytes

REM 3. Update config.h
echo [3/6] Updating config.h FIRMWARE_VERSION...
powershell -Command "(Get-Content '%CONFIG_H%') -replace '#define FIRMWARE_VERSION\s+\".*\"', '#define FIRMWARE_VERSION        \"%NEW_VERSION%\"' | Set-Content '%CONFIG_H%'"

REM 4. Update ota_manifest.json using PowerShell
echo [4/6] Updating ota_manifest.json...
powershell -Command ^
    "$m = Get-Content '%MANIFEST%' | ConvertFrom-Json; " ^
    "$m.firmware.latest_version = '%NEW_VERSION%'; " ^
    "$m.firmware.file_size = %SIZE%; " ^
    "$m.firmware.sha256 = '%SHA256%'; " ^
    "$m.firmware.release_date = '%DATE%'; " ^
    "$m.firmware.release_notes_en = '%NOTES%'; " ^
    "$m.firmware.release_notes_de = '%NOTES%'; " ^
    "$m | ConvertTo-Json -Depth 5 | Set-Content '%MANIFEST%'"

REM 5. Verify
echo [5/6] Verifying updates...
findstr "FIRMWARE_VERSION" "%CONFIG_H%"
powershell -Command "$m = Get-Content '%MANIFEST%' | ConvertFrom-Json; Write-Host \"Manifest FW: $($m.firmware.latest_version)\"; Write-Host \"Manifest SHA: $($m.firmware.sha256.Substring(0,16))...\"; Write-Host \"Manifest Size: $($m.firmware.file_size)\""

REM 6. Done
echo [6/6] Done!
echo.
echo Next steps:
echo   1. Build ^& flash new firmware to CYD:
echo      cd esp32_firmware ^&^& pio run -t upload
echo.
echo   2. Start PHP server:
echo      php -S localhost:8080 -t php_server
echo.
echo   3. Trigger OTA on CYD (tap center bridge dot or wait 15min auto-check)
echo.
echo Server will now offer FW %NEW_VERSION% to devices running older versions