# ESP32 CYD Retro Tamagotchi RPG Firmware (v2.2.0)

This repository contains the complete firmware for the **ESP32 Cheap Yellow Display (CYD)**, model **ESP32-2432S028R** (2.8" 320x240 ILI9341 LCD with XPT2046 resistive touch, RGB LED, and piezo speaker).

Hardware Reference: [https://github.com/JanPeter1/ESP_CoGo](https://github.com/JanPeter1/ESP_CoGo)

---

## 🎮 Touchscreen Controls & 5-Tab RPG Interface

- **Top-Left `FEED / FUTT` + Horizontal Food Flyout**:
  - Tapping `FEED / FUTT` toggles a horizontal bar directly beside the button displaying all owned foods from your inventory (`food_apple`, `food_snack`, cooked meals like *Herb Salad*, *Berry Tart*, *Sun Jerky*).
  - **Food Tiers & Satiety Shield**: Apples provide safe light nutrition (`+12 FD`); Pixel Snacks give `+25 FD / +15 Happy` with a `30%` sickness risk (`KRANK!/SICK!` HUD badge); cooked dishes grant a `SATT:xT` hunger freeze shield for multiple ticks.
  - If your bag has no food, the flyout offers a direct shortcut to the **Shop** (`Bag -> Shop`) or **Jobs**.
- **5-Tab Sub-Screen (`STAT | TASCH/BAG | JOBS | MAGIE/MAGIC | QUEST`)**:
  - **Button 5 Left (`STAT`)**: Full pet & vitals overview (`HP`, `FD`, `EN`, `SAUB`, `GLUECK`, `MP`), XP bar, Login Streak, Satiety Shield (`SATT`), Health Condition (`Gesund/KRANK!`), and automatic NVS + Cloud save every 2 minutes.
  - **`TASCH / BAG` (Bag / Cook / Shop)**: Feed directly from inventory, apply *Healing Salve* (`Heilsalbe`) to cure snack sickness, cook gathered ingredients into dishes, or buy supplies in the Shop.
  - **`JOBS`**: Work professions (*Herb & Apple Forager*, *Village Scout*, *Crystal & Mushroom Mine*) for Coins, XP, cooking materials, and direct food drops.
  - **Button 4 Left (`ZAUB / MGC`) ➔ `MAGIE`**: Selectable spells (*1. Purify / Läuterung* 15 MP to cure sickness, *2. Vitality Aura* 20 MP, *3. Astral Feast* 30 MP with friend blessing) plus **Research Magic** (`-15 EN -> +35 XP +15 MP`).
  - **Button 4 Right (`QUEST`) ➔ `QUEST`**: Claim Login Streak & Daily Missions or launch selectable Co-op Quests (*Scout Patrol*, *Treasure Hunt*).

---

## ⚡ Over-The-Air (OTA) Updates & Dynamic Rules System

The firmware features an industrial-grade update pipeline:

1. **Hardware A/B Dual Partition Switching**:
   - Uses `partitions.csv` with dual 1.5MB app partitions (`ota_0` and `ota_1`).
   - Binary updates stream directly to the inactive slot via `Update.h`.
   - On completion, the ESP32 bootloader switches the active boot slot and reboots.
   - Built-in hardware watchdog and boot rollback prevent bricking if a new image fails to boot.
2. **Dynamic Rule File Hot-Reloading (`rules.json` v1.1.0)**:
   - Game rules, balance parameters, food items, shop catalogs, and daily missions are stored in the 896KB SPIFFS flash partition (`/rules.json`).
   - When new balance rules are published to the PHP server, the CYD downloads and applies them **at runtime without rebooting or interrupting the player**!
3. **Dedicated On-Screen OTA Progress Feedback**:
   - The 2.8" ILI9341 LCD shows a live visual progress bar during firmware downloads with percentage, slot information, and power-off warning.

---

## 🛍️ Daily Missions, Login Streak & Shop Marketplace (Bazar)

- **Daily Login Streak Engine**:
   - Consecutive daily logins increment the streak count (1 to 7+ days).
   - Escalating daily bonus rewards: Day 1 (+15🪙, +10XP) up to Day 7+ (+150🪙, +100XP).
   - Streak claim status is tracked locally and synchronized with the cloud save.
- **Daily Missions Engine**:
   - Built into `TamagotchiCoreEngine.h` (zero dynamic heap allocation).
   - Tracks daily care tasks: Feeding, Playing minigames, Bathing, Magic casting/research, and Sending Treats.
   - Tap `QUEST` (Button 4 Right) or the Coins display (`$XX`) in the footer to claim completed daily missions!
- **Shop & Bazar System**:
   - Purchase Apples, Pixel Snacks, Herb Salads, Healing Salves, Energy Tonics, Recipes, and Co-op Spells; all purchased consumables are added directly to the 16-slot inventory.
   - Validates level requirements and coin balance locally and with `php_server/shop.php`.

---

## 📶 WiFi Setup via Access Point & Web Interface (No Hardcoding Required!)

You do not need to hardcode your WiFi password into `config.h`! The CYD includes an automatic **Captive Web Portal**:

1. **First Boot or Connection Failure**:
   - If WiFi is unconfigured or unreachable, the CYD automatically launches an Access Point:
     - **AP SSID**: `CYD-Tamagotchi-AP`
     - **Default Web IP**: `http://192.168.4.1`
     - The RGB LED glows blue and the 2.8" LCD displays setup instructions.
2. **Connect & Configure**:
   - Connect your smartphone, tablet, or laptop to the `CYD-Tamagotchi-AP` network.
   - Open your web browser and navigate to `http://192.168.4.1`.
   - The web page automatically scans available 2.4GHz WiFi networks.
   - Select your network, enter the password, customize your **Device ID** (`CYD-DEV1`), **Trainer Name**, **Friend's Device ID** (`CYD-DEV2`), **Startup Biome** (Desert, Grassland, Water, Forest), and **PHP Server URL**.
   - Tap **"Save & Connect"**.
3. **Automatic Reboot & Persistence**:
   - Credentials and RPG settings are saved permanently into ESP32 **NVS (Non-Volatile Storage)**.
   - The CYD reboots and joins your WiFi automatically!
4. **Re-enter Setup Mode Anytime**:
   - Hold the `BOOT` button (GPIO 0) during power-on to immediately trigger the AP Web Portal.

---

## 📁 Source Code Structure

- `ESP32_Tamagotchi_CYD.ino`: Main sketch with LCD rendering, XPT2046 touch, FreeRTOS WiFi socket server, A/B OTA updater, and SPIFFS loader.
- `TamagotchiCoreEngine.h`: UI-agnostic C++ game engine with Daily Missions, Level progression, and Shop purchase validation.
- `CYD_Pins.h`: Hardware pinout definitions for display, touch, RGB LED, speaker, LDR, and SD card.
- `config.h`: Default fallbacks for WiFi credentials, device IDs, biome selection, version numbers, and server URLs.
- `PetGraphics.h`: 16-bit 565 color palettes and pixel sprites for Desert, Grassland, Water, and Forest biomes.
- `partitions.csv`: A/B OTA partition scheme (dual 1.5MB app partitions + 896KB SPIFFS).
- `platformio.ini`: 1-click build configuration for PlatformIO.
- `rules.json`: Dynamic balancing rules, shop items, recipes, spells, and mission goals.

---

## ⚡ Method 1: Flashing with PlatformIO (Recommended)

1. Open VS Code with the **PlatformIO IDE** extension installed.
2. Open the `esp32_firmware` folder.
3. Plug your CYD into your computer using a Micro-USB cable.
4. Click the **Upload** arrow button in the bottom status bar (automatically uses `partitions.csv`).
5. (Optional) Run `Upload Filesystem Image` to flash the default `rules.json` to SPIFFS.
6. The display will boot, launch the `CYD-Tamagotchi-AP` web portal, and guide you through setup!

---

## 🛠️ Method 2: Flashing with Arduino IDE

1. **Board Manager**:
   - Select Board: **ESP32 Dev Module**.
   - Partition Scheme: Copy `partitions.csv` into your sketch folder or select **Minimal SPIFFS (1.9MB with OTA)**.
2. **Libraries**:
   - Install `TFT_eSPI` by Bodmer.
   - Install `XPT2046_Touchscreen` by Paul Stoffregen.
   - Install `ArduinoJson` (v6 or v7) by Benoit Blanchon.
3. **Configure `TFT_eSPI`**:
   - In `libraries/TFT_eSPI/User_Setup.h`, ensure ILI9341 is selected with CYD pins (MOSI 13, MISO 12, SCLK 14, CS 15, DC 2, BL 21).
4. **Upload**:
   - Open `ESP32_Tamagotchi_CYD.ino` and click **Upload**!
