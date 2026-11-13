# 🐾 PetDuet: Connected Dual-Device Retro Tamagotchi RPG

[![React](https://img.shields.io/badge/Frontend-React%20%2B%20TypeScript-blue.svg)](https://react.dev/)
[![Tailwind CSS](https://img.shields.io/badge/Style-Tailwind%20CSS-38bdf8.svg)](https://tailwindcss.com/)
[![ESP32](https://img.shields.io/badge/Hardware-ESP32--2432S028R%20CYD-red.svg)](https://github.com/JanPeter1/ESP_CoGo)
[![PHP](https://img.shields.io/badge/Backend-PHP%208%20%2B%20SQLite-777bb4.svg)](https://www.php.net/)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

**PetDuet** is an interactive, multi-platform Tamagotchi RPG ecosystem. Two players nurture, evolve, and connect their digital companions using dedicated **ESP32 Cheap Yellow Display (CYD)** microcontrollers or via the responsive browser-based **Dual CYD Simulator**.

---

## 🌟 Key Features

### 1. 🐣 Tamagotchi Care Mechanics, Inventory-Gated Feeding & Evolution
- **Core Vitals**: Real-time management of Hunger, Happiness, Energy, Hygiene, Health, and Mana.
- **Inventory-Gated Feeding & Horizontal Feed Flyout**: You can only feed food items currently stored in your bag. Pressing the top-left **FEED / FUTT** button opens a **horizontal food flyout bar** right beside it displaying available dishes, quantities, hunger boost, and satiety/risk badges (or a quick jump to the Shop/Jobs when empty).
- **Food Tiers, Satiety Shield & Snack Sickness (`rules.json` v1.1.0)**:
  - **Starting Rations**: Begin with `2x Crisp Apple`, `1x Pixel Snack`, and `2x Meadow Herb`.
  - **🍎 Crisp Apple**: Light, safe snack (`+12 FD`, `+5 Happy`, `+2 HP`).
  - **🥨 Pixel Snack**: Rich quick bite (`+25 FD`, `+15 Happy`, `+5 EN`), but carries a **30% tummyache risk** (`isSick`, `-15 HP` & gradual health drain until cured via *Healing Salve* or *Purify* spell).
  - **🍲 Cooked Dishes & Satiety Shield (`satietyTicks`)**: High-tier meals (*Herb Salad +35 FD / 4 ticks*, *Berry Tart +45 FD / 5 ticks*, *Sun Jerky +55 FD / 6 ticks*, *Kelp Stew +75 FD / 8 ticks*, *Golden Ambrosia*) grant a **Satiety Shield** that freezes hunger decay for several simulation ticks.
- **5 Evolution Stages**: *Egg* ➔ *Baby* ➔ *Youth* ➔ *Adult* ➔ *Elder* driven by XP milestones and care accuracy.
- **Biome Specialization**: 4 distinct elemental biomes (*Desert, Grassland, Water, Forest*) with custom color palettes and procedural pixel-art sprites.
- **Full Stats Screen & Auto-Save**: Bottom-left **STAT** button opens the 5-tab sub-menu (`STAT | BAG | JOBS | MAGIC | QUEST`); state automatically saves every few minutes.

### 2. ⚔️ RPG Subsystems, Jobs, Cooking & Magic Research
- **Professions & Jobs**: *Herb & Apple Forager*, *Village Scout*, *Crystal & Mushroom Miner*, and *Deep Diver* award Coins, XP, cooking ingredients, and **direct food drops** (`bonusFoodDrop`).
- **Culinary Workshop**: Combine gathered ingredients (*Meadow Herb, Forest Mushroom, Raw Meat, Sun Salt, Lagoon Kelp*) to craft hearty meals stored directly in your inventory.
- **Selectable Magic Spells & Magic Research**: Tap **MAGIC / ZAUB** (Button 4 Left on CYD) to open selectable spells (*Purify / Läuterung* to cure sickness, *Vitality Aura*, *Astral Feast* with partner blessing) and **Research Magic** (converts Energy into Mana & XP).
- **Equipment & Bag**: Manage food, curative salves/potions, cooking ingredients, and decorative accessories (*Blossom Crown, Starry Cowl, Fancy Top Hat*).

### 3. 🎯 Quests, Daily Missions & 7-Day Login Streak Bonus
- **Co-op Quests & Daily Missions**: Tap **QUEST** (Button 4 Right on CYD) or open the Quests panel to launch collaborative quests (*Scout Patrol*, *Treasure Hunt*) and claim rotating care missions (Feeding, Minigames, Bathing, Magic, Sending Treats).
- **7-Day Login Streak**: Escalating daily reward ladder:
  - **Day 1**: +15 🪙 / 10 XP
  - **Day 2**: +25 🪙 / 15 XP
  - **Day 3**: +40 🪙 / 25 XP
  - **Day 4**: +60 🪙 / 35 XP
  - **Day 5**: +85 🪙 / 50 XP
  - **Day 6**: +115 🪙 / 65 XP
  - **Day 7+**: +150 🪙 / 100 XP (Weekly Jackpot)
- **Claim & Fanfare**: In-app claim triggers audio jingles, confetti particle effects, and cloud save synchronization.

### 4. 🛒 In-Game Shop & Marketplace (Bazar)
- Purchase apples, snacks, prepared herb salads, healing salves, energy tonics, cooking recipes, and spells with level gates and coin costs. All purchased consumables go straight into your inventory.
- Fully validated client-side in `TamagotchiCoreEngine` (`v1.1.0`) and server-side in `php_server/shop.php`.

### 5. 🤝 Dual-Device Real-Time Sync & Trading
- **Direct Socket & WebSocket Relay**: Under 20ms sync latency between devices.
- **Social Interactions**: Send hearty treats, quick chats, and collaborative quest invitations.
- **Peer Trading Post**: Propose trades with escrow locking and atomic exchanges across devices.

### 6. ⚡ Dual-Tier Over-The-Air (OTA) Updates
- **Tier 1 (Firmware)**: Hardware A/B dual partition switching (`ota_0` and `ota_1`, 1.5MB each) with auto-rollback protection.
- **Tier 2 (Rules Hot-Reload)**: Live hot-reloading of game balance and shop rules (`/rules.json`) directly into the SPIFFS flash partition **without rebooting**.

---

## 🖥️ Repository Structure

```
├── CLAUDE.md                    # Guidance for Claude Code CLI
├── AGENTS.md                    # Universal architectural rules for AI coding agents
├── README.md                    # Project documentation & setup guide (this file)
├── input/
│   └── PetDuet.md               # Master Technical PRD & Game Design Document
├── doc/
│   ├── PetDuet.md               # Synchronized technical specification
│   ├── graphics_and_animation_research.md
│   └── tamagotchi_and_rpg_rules_research.md
├── src/                         # Web Simulator (React + TypeScript + Vite)
│   ├── engine/                  # TypeScript TamagotchiCoreEngine & rule parser
│   ├── components/              # Dual CYD Canvas, RPG Modals, Controls, HUD
│   ├── utils/                   # i18n bilingual dictionaries (EN & DE), Web Audio
│   └── hardware_and_server/     # Embedded firmware & PHP code viewer modules
├── esp32_firmware/              # ESP32 CYD (ESP32-2432S028R) C++ Source Code
│   ├── ESP32_Tamagotchi_CYD.ino # Main sketch (ILI9341 TFT, Touch, WiFi, FreeRTOS, OTA)
│   ├── TamagotchiCoreEngine.h   # UI-agnostic C++ game runtime engine
│   ├── CYD_Pins.h               # Hardware pinouts (ILI9341, XPT2046, RGB LED, Buzzer)
│   ├── config.h                 # WiFi, Server URL, Language (LANG_DE), Biome config
│   ├── cyd_config.h             # Display driver & hardware dimensions
│   ├── PetGraphics.h            # 16-bit RGB565 sprites and color palettes
│   ├── partitions.csv           # Dual 1.5MB A/B OTA + 896KB SPIFFS partition table
│   └── platformio.ini           # 1-click PlatformIO build configuration
├── php_server/                  # Cloud Backend (PHP 8 + SQLite)
│   ├── index.php                # Status dashboard & health check
│   ├── register.php             # Device registration & pairing
│   ├── sync.php                 # 2-way state synchronization & trade escrow
│   ├── shop.php                 # Shop catalog query & purchase validation
│   ├── cloud_save.php           # Remote state backup
│   ├── leaderboard.php          # Community top trainer ranking
│   ├── ota_check.php            # Firmware & rules update checker
│   ├── ota_download.php         # Streaming binary & JSON rule file updates
│   └── ota_manifest.json        # Release versioning, SHA256 hashes & changelogs
└── public/                      # Static assets & downloadable ZIP packages
    ├── esp32_firmware.zip       # Pre-packaged firmware ready for flashing
    ├── php_server.zip           # Pre-packaged backend server scripts
    └── petduet-repo.bundle      # Full Git repository bundle
```

---

## 🚀 Quick Start: Web Simulator

The web simulator provides a pixel-accurate recreation of two ESP32 CYD devices side-by-side:

```bash
# 1. Install dependencies
npm install

# 2. Start the development server (runs strictly on port 3000)
npm run dev

# 3. Open in your browser
http://localhost:3000
```

To build production bundles:
```bash
npm run build
npm run lint
```

---

## 📟 ESP32 CYD Hardware Flashing

### Hardware Target
- **Board**: ESP32-2432S028R ("Cheap Yellow Display")
- **Display**: 2.8-inch TFT LCD (ILI9341 320x240 pixels)
- **Touch**: XPT2046 Resistive Touch Controller
- **Sound**: Onboard Piezo Buzzer (GPIO 26)
- **Status**: Tri-color RGB LED (GPIO 4, 16, 17 - active low)

### Method A: PlatformIO (Recommended)
1. Install [VS Code](https://code.visualstudio.com/) and the **PlatformIO IDE** extension.
2. Open the `esp32_firmware` directory in VS Code.
3. Connect your CYD device via a Micro-USB cable.
4. Click **PlatformIO: Upload** in the bottom status bar.
5. (Optional) Run `PlatformIO: Upload Filesystem Image` to flash the default `rules.json` to SPIFFS.

### Method B: Arduino IDE
1. Install the **esp32** board package (by Espressif) via Board Manager.
2. Select **ESP32 Dev Module**.
3. Install required libraries:
   - `TFT_eSPI` by Bodmer (configure `User_Setup.h` for ILI9341 on CYD pins: MOSI 13, MISO 12, SCLK 14, CS 15, DC 2, BL 21)
   - `XPT2046_Touchscreen` by Paul Stoffregen
   - `ArduinoJson` (v6 or v7) by Benoit Blanchon
4. Copy `partitions.csv` into your sketch folder or choose **Minimal SPIFFS (1.9MB with OTA)**.
5. Open `ESP32_Tamagotchi_CYD.ino` and click **Upload**.

---

## 📶 WiFi Setup (No Code Modification Needed!)

On first power-on (or if WiFi credentials are missing or unreachable):
1. The CYD boots into **SoftAP Mode** and broadcasts SSID: `CYD-Tamagotchi-AP`.
2. Connect your smartphone, tablet, or PC to `CYD-Tamagotchi-AP`.
3. Open your browser and visit: `http://192.168.4.1`.
4. Scan networks, enter your WiFi password, set your **Device ID** (`CYD-DEV1`), **Trainer Name**, **Friend's ID** (`CYD-DEV2`), **Startup Biome**, and **Server URL**.
5. Tap **Save & Reboot** — configuration is saved permanently in **NVS Flash**.

---

## 🌐 PHP Backend Server Deployment

```bash
# Local development testing
cd php_server
php -S localhost:8080

# Production deployment
# Upload php_server/ contents to your web server (e.g. Apache/Nginx with PHP 8.1+)
# Ensure the database directory is writable for SQLite:
chmod 775 data/
```

Production Server Target:
`https://familie-riegel.de/petduet/` (SFTP: `ftp.familie-riegel.de:2222`)

---

## 🌐 Bilingual Support (English & Deutsch)

- **Web Simulator**: Toggle instantly between English and German via the top bar language selector.
- **ESP32 Firmware**: Set `#define LANG_DE 1` in `config.h` for German button labels and notifications, or `0` for English.
- **Rules Engine**: All items, recipes, jobs, and missions contain paired `name`/`name_de` and `description`/`description_de` attributes.

---

## 📄 License

This project is licensed under the MIT License. See [LICENSE](LICENSE) for details.
