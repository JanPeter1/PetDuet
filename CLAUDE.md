# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

**PetDuet** is a dual-device Tamagotchi RPG where two ESP32 Cheap Yellow Display (CYD) devices communicate via WebSocket through a PHP server. The game features RPG progression, biome-based pets, cooking, magic, trading, daily missions, and OTA updates.

- **Hardware**: ESP32-2432S028R (CYD) with ILI9341 320x240 LCD, XPT2046 resistive touch
- **Firmware**: ESP-IDF 5.x/6.x with LVGL graphics, ArduinoJson for rules parsing
- **Server**: PHP 8.x + SQLite + Ratchet WebSocket server
- **Rules Engine**: Single `data/rules.json` shared by ESP32, PHP, and web simulator
- **Assets**: SVG sprites (web) + RGB565 PROGMEM headers (ESP32)

## Key Commands

### Web Simulator (React + TypeScript)
```bash
# Start local dev server (strictly port 3000)
npm run dev

# Check compilation and type linting
npm run build
npm run lint
```

### PHP Server (Development)
```bash
# Start local PHP server
cd php_server && php -S localhost:8080

# Deploy to production (SFTP)
# Host: ftp.familie-riegel.de:2222
# Path: /petduet/
```

### ESP32 Firmware (PlatformIO & ESP-IDF)
```bash
# PlatformIO build & flash to CYD
cd esp32_firmware
pio run -t upload

# ESP-IDF build and flash
idf.py -p <CYD_USB_PORT> menuconfig  # Set flash speed, partition table
idf.py build flash monitor

# OTA update check (runs automatically on boot + every 6h)
# GET https://familie-riegel.de/petduet/api/ota/check
```

### Rules Engine (Single Source of Truth)
```bash
# Validate JSON syntax
cat data/rules.json | jq .

# After modifying rules.json:
# 1. ESP32: Rebuild firmware (or use live hot-reload via /api/rules)
# 2. PHP: Restart server (auto-loads on next request)
# 3. Web: Refresh simulator
```

## Architecture Highlights

### Dual-Device Communication
```
ESP32 CYD 1 ←[WebSocket]→ PHP Server (Relay + SQLite) ←[WebSocket]→ ESP32 CYD 2
     │                                              │
     └────── Local SPIFFS/NVS (offline rules/save)  └────── Local SPIFFS/NVS
```

### Game Loop Tick (60s default)
- Stat decay from `rules.json` → `stats.*.decay_per_tick`
- Care Mistake engine: 15-min response window after Food/Happiness hit 0
- Biome modifiers apply (e.g., Desert: `food_decay: 1.15`)
- Seasonal cycle via ISO week modulo 4 (NTP Berlin time)

### OTA System (Two-Tier)
| Tier | Target | Mechanism | Downtime |
|------|--------|-----------|----------|
| **Firmware** | A/B partitions (`ota_0`/`ota_1`, 1.5MB each) | HTTPS stream to inactive slot → SHA256 → boot switch → auto-rollback on crash | Reboot required |
| **Rules** | SPIFFS `/rules.json` | Download to temp → atomic rename → in-memory hot-reload | **Zero** (no reboot) |

### Display Layout (320×240 ILI9341)
```
[TOP BAR: Lv, Jobs, Magic, Cook, Online, Partner]
[LEFT ACTIONS] | [YOUR PET STAGE] | [SYNC] | [PARTNER PET STAGE] | [RIGHT ACTIONS]
  Feed, Play,     122px wide          8px         122px wide           Treat, Chat,
  Bath, RPG,      (your biome,         WiFi        (partner biome,      Magic+, Quest,
  Save            sprite, HP/Food)     Sync        sprite, chat)       Trade
[BOTTOM HUD: Coins, HP%, Energy% | Partner Coins, HP%, Energy% | Season]
```

## Important Files

| File | Purpose |
|------|---------|
| `data/rules.json` | **All game mechanics** - stats, biomes, recipes, jobs, spells, shop, missions |
| `doc/PetDuet.md` | Full technical specification (read this first) |
| `esp32_firmware/CYD_Pins.h` | Hardware pin definitions (TFT, Touch, LEDs, Buzzer) |
| `esp32_firmware/config.h` | WiFi, device IDs, biome, server URL, language |
| `assets/sprites/` | SVG source graphics (pets, items, emotions, UI) |
| `assets/cyd_bitmaps/` | RGB565 PROGMEM headers for ESP32 |
| `php_server/` | PHP REST API + WebSocket hub + SQLite |

## Rules.json Key Sections

```json
{
  "meta": { "tick_interval_seconds": 60, "timezone": "Europe/Berlin" },
  "stats": { "food": {"decay_per_tick": 0.25}, "health": {"starve_decay": 1.5} },
  "lifecycle": { "phases": [...], "care_mistake_threshold_seconds": 900 },
  "biomes": { "desert": {"modifiers": {"food_decay": 1.15, "mining_yield": 1.25}} },
  "recipes": [{ "id": "rcp_salad", "ingredients": {"mat_herb": 2}, "effects": {...} }],
  "jobs": [{ "id": "job_forage", "energy_cost": 20, "rewards": {...} }],
  "spells": [{ "id": "spl_cleanse", "mana_cost": 15, "target": "self", "effects": {...} }],
  "daily_missions": [{ "action_type": "feed", "target_count": 3, "rewards": {...} }],
  "shop": { "items": [...], "recipes": [...], "spells": [...] }
}
```

- All text is bilingual: `name`/`name_de`, `description`/`description_de`
- Keys use camelCase for ArduinoJson + PHP `json_decode` compatibility

## Common Development Tasks

### Modify Stat Decay Rate
1. Edit `data/rules.json` → `stats.food.decay_per_tick`
2. ESP32: `idf.py build flash monitor` (or wait for hot-reload)
3. PHP: Restart server

### Add New Recipe
1. Add to `data/rules.json` → `recipes[]` with `id`, `ingredients`, `effects`, `min_level`
2. Create SVG in `assets/sprites/items/`
3. Add to `assets/cyd_bitmaps/ItemIcons_RGB565.h` (RGB565 PROGMEM array)

### Add New Biome
1. Add to `rules.json` → `biomes` with `modifiers`, `color`, `rgb565`, `trait`
2. Create 5 pet state SVGs in `assets/sprites/pets/` (`{biome}_pet_{idle,happy,eating,sleeping,sick}.svg`)
3. Update ESP32 pet rendering logic for biome-specific sprites

### Test Game Balance
- Open `assets/previews/AssetCatalogPreview.html` for graphics
- Start PHP server: `cd php_server && php -S localhost:8080`
- Use web simulator for tick-based decay debugging

## Hardware Pin Reference (CYD_Pins.h)

| Component | Pins |
|-----------|------|
| **TFT ILI9341 SPI** | CS=15, MISO=12, MOSI=13, SCLK=14, DC=2, BL=21 |
| **XPT2046 Touch** | CS=33, MOSI=32, MISO=39, CLK=25, IRQ=36 |
| **RGB LED (Active Low)** | R=4, G=16, B=17 |
| **Buzzer** | GPIO 26 (LEDC channel 0, 8-bit) |

## WebSocket Message Types

```json
// Treat sent to friend
{ "type": "TREAT_SEND", "from_device": "CYD_DEV1", "to_device": "CYD_DEV2", "payload": {...} }

// Co-op magic cast
{ "type": "MAGIC_CAST", "from_device": "CYD_DEV1", "to_device": "CYD_DEV2", "payload": {...} }

// Trade proposal
{ "type": "TRADE_PROPOSE", "trade_id": "...", "payload": { "offered_item": "...", "requested_item": "..." } }
```

## Server API Endpoints

| Endpoint | Method | Purpose |
|----------|--------|---------|
| `/api/register-device` | POST | Register device, MAC, trainer name, biome |
| `/api/login` | POST | Authenticate, return session token |
| `/api/pet/get-pet` | GET | Full pet state from SQLite |
| `/api/pet/update` | POST | Sync pet state changes |
| `/api/sync` | POST | Full 2-way sync + trade escrow |
| `/api/rules` | GET | Download latest `rules.json` |
| `/api/ota/check` | GET | Firmware + rules update manifest |
| `/api/ota/firmware/:file` | GET | Stream firmware binary |
| `/api/ota/report` | POST | Client reports upgrade result |

## Bilingual Support

- All UI text in `rules.json` has `name`/`name_de`, `description`/`description_de`
- `config.h`: `#define LANG_DE 1` for German, `0` for English
- Button labels adapt: EN=`FEED/PLAY/BATH/RPG` DE=`FÜTTERN/SPIELEN/BADEN/RPG`

## Deployment

- **Production Server**: `https://familie-riegel.de/petduet/` (SFTP: `ftp.familie-riegel.de:2222`)
- **NTP Time**: `pool.ntp.org` with `Europe/Berlin` timezone (drives seasonal cycle)
- **OTA Manifest**: Server returns firmware + rules versions, SHA256, download URLs

## Partition Scheme (4MB Flash)
```csv
nvs,      data, nvs,     0x9000,   0x5000,
otadata,  data, ota,     0xe000,   0x2000,
app0,     app,  ota_0,   0x10000,  0x180000,  # 1.5MB Slot A
app1,     app,  ota_1,   0x190000, 0x180000,  # 1.5MB Slot B
spiffs,   data, spiffs,  0x310000, 0xE0000,   # ~896KB rules.json + assets
```

## References

- **CYD Hardware**: https://github.com/JanPeter1/ESP_CoGo
- **LVGL Graphics**: Used via ESP-IDF components for ILI9341
- **ArduinoJson**: C++ parser for `rules.json` on ESP32
- **Ratchet**: PHP WebSocket library for real-time relay