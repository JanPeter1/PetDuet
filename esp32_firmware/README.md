# ESP32 CYD Retro Tamagotchi RPG Firmware

This repository contains the complete firmware for the **ESP32 Cheap Yellow Display (CYD)**, model **ESP32-2432S028R** (2.8" 320x240 ILI9341 LCD with XPT2046 resistive touch, RGB LED, and piezo speaker).

Hardware Reference: [https://github.com/JanPeter1/ESP_CoGo](https://github.com/JanPeter1/ESP_CoGo)

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

- `ESP32_Tamagotchi_CYD.ino`: Main Arduino sketch containing the WebServer AP portal, 320x240 LCD rendering, XPT2046 touch, FreeRTOS WiFi socket server, and pet RPG logic.
- `CYD_Pins.h`: Complete hardware pinout definitions for display, touch, RGB LED, speaker, LDR, and SD card.
- `config.h`: Default fallbacks for WiFi credentials, device IDs, biome selection, and PHP server URL.
- `PetGraphics.h`: 16-bit 565 color palettes for Desert, Grassland, Water, and Forest biomes, plus egg, baby, adult, and elder sprite rendering.
- `platformio.ini`: 1-click build configuration for PlatformIO.

---

## ⚡ Method 1: Flashing with PlatformIO (Recommended)

1. Open VS Code with the **PlatformIO IDE** extension installed.
2. Open the `esp32_firmware` folder.
3. Plug your CYD into your computer using a Micro-USB cable.
4. Click the **Upload** arrow button in the bottom status bar.
5. The display will boot, launch the `CYD-Tamagotchi-AP` web portal, and guide you through setup!

---

## 🛠️ Method 2: Flashing with Arduino IDE

1. **Board Manager**:
   - Select Board: **ESP32 Dev Module**.
   - Partition Scheme: **Huge APP (3MB No OTA / 1MB SPIFFS)**.
2. **Libraries**:
   - Install `TFT_eSPI` by Bodmer.
   - Install `XPT2046_Touchscreen` by Paul Stoffregen.
   - Install `ArduinoJson` (v6 or v7) by Benoit Blanchon.
3. **Configure `TFT_eSPI`**:
   - In `libraries/TFT_eSPI/User_Setup.h`, ensure ILI9341 is selected with CYD pins (MOSI 13, MISO 12, SCLK 14, CS 15, DC 2, BL 21).
4. **Upload**:
   - Open `ESP32_Tamagotchi_CYD.ino` and click **Upload**!
