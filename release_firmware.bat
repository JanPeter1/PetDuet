@echo off
REM ============================================================================
REM PetDuet Windows Release Launcher (Version Bump, Build, Tests+Coverage, OTA, Flash)
REM Ruft release_firmware.ps1 mit allen uebergebenen Parametern auf.
REM Beispiele:
REM   release_firmware.bat
REM   release_firmware.bat -Bump minor -Notes "Neues Chat-Menue"
REM   release_firmware.bat -Version 2.2.0 -ServerPath "C:\xampp\htdocs\cyd-tamagotchi" -ComPort COM5
REM ============================================================================
setlocal

set "SCRIPT_DIR=%~dp0"
set "PS_SCRIPT=%SCRIPT_DIR%release_firmware.ps1"

where powershell >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    where pwsh >nul 2>&1
    if %ERRORLEVEL% NEQ 0 (
        echo [FEHLER] Weder powershell.exe noch pwsh.exe gefunden.
        exit /b 1
    )
    pwsh -NoProfile -ExecutionPolicy Bypass -File "%PS_SCRIPT%" %*
) else (
    powershell -NoProfile -ExecutionPolicy Bypass -File "%PS_SCRIPT%" %*
)

set "EXIT_CODE=%ERRORLEVEL%"

REM Fenster offen lassen, falls per Doppelklick aus dem Windows Explorer gestartet
echo %cmdcmdline% | find /i "/c" >nul 2>&1
if %ERRORLEVEL% EQU 0 (
    echo.
    pause
)

exit /b %EXIT_CODE%
