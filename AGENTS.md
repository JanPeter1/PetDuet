# AGENTS.md

This file provides architectural rules, runtime constraints, and implementation guidelines for any AI agent or automated developer working on the **PetDuet** repository.

---

## 🧭 Project Architecture Overview

**PetDuet** is a dual-device connected retro Tamagotchi RPG system.
It consists of three interconnected layers:
1. **Frontend Simulator (Web SPA)**: React 18 + Vite + TypeScript + Tailwind CSS. Emulates two ESP32 CYD devices side-by-side with interactive buttons, touch simulation, Web Audio sound effects, and comprehensive RPG subsystems.
2. **Embedded Firmware (ESP32 CYD)**: C++ firmware for the **ESP32-2432S028R** ("Cheap Yellow Display", 2.8" 320x240 ILI9341 LCD + XPT2046 touch + RGB LED + Piezo buzzer). Includes hardware A/B OTA partition switching, FreeRTOS networking, and dynamic SPIFFS rules hot-reloading.
3. **Backend Server (PHP 8 + SQLite)**: REST API and WebSocket hub providing device registration, pairing, state synchronization, trade escrow, shop purchases, global leaderboards, and OTA update manifests.

---

## 🔒 Critical Invariants & Constraints

### 1. Dev Server Port Constraint
- The Web dev server must **strictly run on port 3000** (`server.ts` uses `const PORT = 3000;`).
- **Never** use `process.env.PORT || 3000` because cloud run environments often inject `PORT=8080`, which conflicts with internal container proxies and causes `EADDRINUSE`.

### 2. Dual-Engine Parity (TypeScript & C++)
The game logic is duplicated in two parallel core engines:
- **TypeScript**: `src/engine/TamagotchiCoreEngine.ts`
- **C++**: `esp32_firmware/TamagotchiCoreEngine.h`

**Rule**: Any changes to game mechanics, stat decay, XP formulas, daily missions, login streak rewards, or shop catalogs **MUST be applied identically to both engines**.
- Keep `TamagotchiCoreEngine` **UI-agnostic and decoupled from hardware/DOM**.
- Zero dynamic heap allocation in the C++ engine (use fixed-size structs and buffers).

### 3. Central Rules Specification (`rules.json`)
- `data/rules.json` / `esp32_firmware/rules.json` / `php_server/rules.json` is the single source of truth for:
  - Stat decay intervals and care mistake thresholds.
  - Evolution level requirements.
  - Recipes, jobs, and co-op spells.
  - Daily mission definitions and reward values.
  - Shop items, recipes, and spell catalogs with costs and level gates.
- Always validate JSON formatting before committing.

### 4. Bilingual Support (EN & DE)
- Every user-facing string must support both **English** and **German**.
- **Web App**: Add translations to both `translations.en` and `translations.de` in `src/utils/i18n.ts`. Missing keys in either language cause TypeScript compilation errors.
- **ESP32 Firmware**: Use conditional macros based on `#if LANG_DE` (defined in `config.h`).
- **Game Rules**: Items, recipes, jobs, and missions must define paired `name`/`name_de` and `description`/`description_de` properties.

### 5. CYD Hardware Pin Invariants (`CYD_Pins.h`)
The ESP32-2432S028R hardware pins must never be arbitrarily altered:
- **TFT SPI**: `MOSI=13`, `MISO=12`, `SCLK=14`, `CS=15`, `DC=2`, `BL=21` (active high)
- **XPT2046 Touch SPI**: `CS=33`, `MOSI=32`, `MISO=39`, `CLK=25`, `IRQ=36`
- **RGB Status LED**: `Red=4`, `Green=16`, `Blue=17` (Active LOW: `LOW` = ON, `HIGH` = OFF)
- **Piezo Speaker**: `GPIO 26` (LEDC channel 0, 8-bit resolution)
- **LDR Light Sensor**: `GPIO 34` (ADC1_CH6)
- **Micro-SD Card**: `CS=5`, `MOSI=23`, `MISO=19`, `SCK=18`

### 6. Dual-Tier OTA Updates
- **Partition Table (`partitions.csv`)**:
  - `ota_0` (1.5MB app partition A)
  - `ota_1` (1.5MB app partition B)
  - `spiffs` (896KB for dynamic `/rules.json` hot-reloading)
- When a firmware change is finalized, ensure `ota_manifest.json` versions and changelogs are updated.

---

## 🛠️ Verification & Quality Assurance

Before completing any task, run the following verification steps:

1. **Applet Compilation**:
   - `compile_applet` must report: `Build succeeded - the applet is compiled`.
2. **Applet Linting**:
   - `lint_applet` must report: `Linting completed successfully` (`$ tsc --noEmit`).
3. **Archive Synchronization**:
   - If firmware or PHP files are touched, regenerate `esp32_firmware.zip` and `php_server.zip` using Python:
     ```bash
     python3 -c "import shutil, os; shutil.make_archive('esp32_firmware', 'zip', 'esp32_firmware'); shutil.make_archive('php_server', 'zip', 'php_server'); os.makedirs('public', exist_ok=True); shutil.copyfile('esp32_firmware.zip', 'public/esp32_firmware.zip'); shutil.copyfile('php_server.zip', 'public/php_server.zip')"
     ```
4. **Git Discipline**:
   - Commit changes with clear, descriptive semantic commit messages (`feat: ...`, `fix: ...`, `docs: ...`).

---

## 📁 Key Directories & Roles

| Path | Purpose |
|------|---------|
| `src/` | React 18 SPA web simulator |
| `src/engine/` | TypeScript game runtime engine and config parser |
| `src/components/` | Dual CYD canvas, UI controls, RPG modals, shop & streak HUD |
| `src/utils/i18n.ts` | Complete bilingual dictionary (English & German) |
| `esp32_firmware/` | C++ firmware for the ESP32-2432S028R CYD |
| `esp32_firmware/ESP32_Tamagotchi_CYD.ino` | Main sketch (rendering, touch, FreeRTOS networking, OTA) |
| `esp32_firmware/TamagotchiCoreEngine.h` | Decoupled C++ game engine |
| `php_server/` | PHP 8 cloud backend (registration, sync, shop, leaderboard, OTA) |
| `input/PetDuet.md` & `doc/PetDuet.md` | Master PRD and technical design specifications |
| `public/` | Pre-packaged downloadable ZIP archives and Git bundles |
