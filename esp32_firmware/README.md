# ESP32 CYD Retro Tamagotchi RPG Firmware (ESP-IDF v6.1.0 + LVGL v8.4)

This directory contains the native **ESP-IDF** (compatible with v5.x and **v6.1.0**) firmware for the **ESP32 Cheap Yellow Display (CYD)**, model **ESP32-2432S028R** (2.8" 320x240 ILI9341 LCD + XPT2046 resistive touch + RGB LED + Piezo speaker).

---

## Confirmed target hardware

- Purchased display: [Amazon ASIN B0FWK1YCQJ](https://www.amazon.de/dp/B0FWK1YCQJ)
- Board model: **ESP32-2432S028R** (classic ESP32 CYD, not an ESP32-S3 board)
- Use the pinout in `CYD_Pins.h`; documentation for `ES3C28P` / `ES3N28P` is for a different S3 board and does not apply here.

### Known-good LCD configuration

- Controller: ILI9341, using the `ILI9341_2` initialization profile.
- Display SPI: `SPI2_HOST`, MOSI 13, MISO 12, SCLK 14, CS 15, DC 2, 20 MHz; do not set `SPI_SAMPLING_POINT_PHASE_1` (unsupported on this ESP32).
- Landscape RGB565 setup: `MADCTL=0x28`, `PIXFMT=0x55`, `FRMCTR1=00 1A`, `DFUNCTR=08 82 27`, inversion command `0x21`.
- Power/VCOM values: `C0=10`, `C1=00`, `C5=30 30`, `C7=B7`; use the ILI9341_2 gamma tables in `main/cyd_display.cpp`.
- LVGL uses 16-bit swapped color and a DMA-capable staging buffer for partial flushes. Keep this staging path; passing the LVGL draw buffer directly to SPI DMA caused visual corruption.
- Touch: XPT2046 on SPI3 (MOSI 32, MISO 39, SCLK 25, CS 33) at 2.5 MHz, with DMA disabled for its three-byte polling transfers. Detect contact from Z1/Z2 pressure; do not require GPIO36 IRQ to be low because that input has no internal pull-up. With ILI9341 `MADCTL=0x28`, swap the XPT2046 axes before mapping to LVGL coordinates (`BOARD_TOUCH_SWAP_XY=1`).

## 🎮 Touchscreen Controls & LVGL RPG Interface

- **Top-Left `FEED / FUTT` + Horizontal Food Flyout**:
  - Tapping `FEED / FUTT` toggles an LVGL horizontal food bar displaying all owned foods from your inventory with crisp 16×16 pixel-art sprites and quantities.
  - **Food Tiers & Satiety Shield**: Crisp Apples (`+12 FD`), Pixel Snacks (`+25 FD`, 30% sickness risk), and cooked meals granting multi-tick `SATT` hunger-freeze shields.
- **6-View LVGL Sub-Screens (`STAT | TASCH | BERUFE | MAGIE | QUEST` + `CHAT`)**:
  - **`STAT`**: Full pet & vitals overview (`HP`, `FD`, `EN`, `SAUB`, `GLUECK`, `MP`), live active job timer, NVS & Cloud save button.
  - **`TASCH` (Bag / Cook / Shop)**: Feed directly, use *Healing Salve*, cook ingredients, or purchase shop supplies.
  - **`BERUFE` (Jobs)**: Long-running professions (60s / 120s / 180s) with live progress bar and countdown timer.
  - **`MAGIE` (Magic)**: Selectable spells (*Laeuterung 15MP*, *Heilaura 20MP*, *Astral-Festmahl 30MP*) plus *Magie erforschen* (`-15 EN -> +15 MP +35 XP`).
  - **`QUEST`**: Daily Login Streak & Mission claims plus selectable Co-op Quests.
  - **`CHAT`**: Clickable bilingual quick-chat phrases sent via lwIP P2P sockets / PHP relay.

---

## ⚡ Native ESP-IDF Drivers & Over-The-Air (OTA) System

1. **Direct SPI Drivers + LVGL v8.4**:
   - **ILI9341 Display (`SPI2_HOST` @ 40MHz)**: Byte-swapped LVGL RGB565 DMA transfers with CYD panel color inversion (`0x21`) and backlight on GPIO 21.
   - **XPT2046 Touch (`SPI3_HOST` @ 2.5MHz)**: Pressure-polled, axis-swapped coordinates feeding LVGL `LV_INDEV_TYPE_POINTER`.
2. **Hardware A/B Dual Partition Switching (`esp_ota_ops.h`)**:
   - Uses `partitions.csv` (`ota_0` 1.5MB, `ota_1` 1.5MB, `spiffs` 896KB).
   - Enables `CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE=y`; marks partition valid after boot or rolls back on failure.
   - Live LVGL progress bar during firmware streaming from `ota_download.php`.
3. **Dynamic Rule File Hot-Reloading (`/spiffs/rules.json`)**:
   - Updates balance constants at runtime without rebooting via SPIFFS.

---

## 📁 Source Code Structure

- `main/app_main.cpp`: FreeRTOS `app_main()` entry point, main RPG tick loop, P2P socket polling, NVS autosave, and OTA checks.
- `main/cyd_lvgl_ui.cpp` / `.h`: Complete LVGL v8 UI, 16×16 sprite conversion, `lv_canvas` pet/egg stage, food flyout, and RPG modals.
- `main/cyd_display.cpp` / `.h`: Native ESP-IDF `SPI2_HOST` ILI9341 driver with DMA `flushRgb565Rect`.
- `main/cyd_touch.c` / `.h`: Native ESP-IDF `SPI3_HOST` XPT2046 touch controller driver.
- `main/cyd_hw.c` / `.h`: GPIO RGB status LEDs (active low), BOOT button (GPIO 0), and LEDC PWM piezo buzzer (GPIO 26).
- `main/cyd_storage.cpp` / `.h`: ESP-IDF `nvs_flash` ("cyd_pet") and `esp_spiffs` rules persistence.
- `main/cyd_net.cpp` / `.h`: `esp_wifi` STA + SoftAP portal (`esp_http_server` at `192.168.4.1`), `lwip/sockets` P2P (port 8888), `esp_http_client` PHP sync, PSA Crypto SHA-256 (IDF 6) / MbedTLS fallback (IDF 5), and `esp_ota_ops` updater.
- `main/lv_conf.h` & `main/idf_component.yml`: LVGL v8.4 configuration & IDF Component Manager dependencies (`lvgl/lvgl ^8.4.0`, `espressif/cjson ^1.7.19` for IDF >= 6.0).
- `TamagotchiCoreEngine.h`: Platform-independent, zero-heap C++ game engine shared with host unit tests.
- `PetGraphics.h`: 16-bit RGB565 biome palettes, 16×16 sprites, and procedural pet/egg drawing.
- `CMakeLists.txt`, `sdkconfig.defaults`, `partitions.csv`, `platformio.ini`: Build configurations for `idf.py` (ESP-IDF 6.1.0) and PlatformIO fallback.

---

## 🚀 Building & Flashing

### Method 1: Native ESP-IDF v6.1.0 CLI (`idf.py`) — Recommended
```bash
cd esp32_firmware
# Bei Wechsel von ESP-IDF 5.x oder nach Git-Sync einmalig altes build/sdkconfig bereinigen:
idf.py fullclean
idf.py set-target esp32
idf.py build flash monitor
```

### Method 2: PlatformIO (`framework = espidf`, ESP-IDF 5.1 Fallback)
```bash
cd esp32_firmware
pio run -e esp32-2432s028r -t upload --upload-port COM3
```
