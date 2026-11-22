/**
 * ESP32 Cheap Yellow Display (CYD - ESP32-2432S028R) Tamagotchi RPG Firmware
 * Native ESP-IDF (v5 / v6.1) + LVGL v8.4 implementation.
 */

import appMainCpp from '../../esp32_firmware/main/app_main.cpp?raw';
import lvglUiCpp from '../../esp32_firmware/main/cyd_lvgl_ui.cpp?raw';
import cydDisplayCpp from '../../esp32_firmware/main/cyd_display.cpp?raw';
import cydNetCpp from '../../esp32_firmware/main/cyd_net.cpp?raw';
import cydStorageCpp from '../../esp32_firmware/main/cyd_storage.cpp?raw';
import cydTouchC from '../../esp32_firmware/main/cyd_touch.c?raw';
import cydHwC from '../../esp32_firmware/main/cyd_hw.c?raw';
import mainCmake from '../../esp32_firmware/main/CMakeLists.txt?raw';
import rootCmake from '../../esp32_firmware/CMakeLists.txt?raw';
import lvConfH from '../../esp32_firmware/main/lv_conf.h?raw';
import idfCompYml from '../../esp32_firmware/main/idf_component.yml?raw';
import sdkDefaults from '../../esp32_firmware/sdkconfig.defaults?raw';
import cydPins from '../../esp32_firmware/CYD_Pins.h?raw';
import cydConfigH from '../../esp32_firmware/cyd_config.h?raw';
import cydConfig from '../../esp32_firmware/config.h?raw';
import cydGraphics from '../../esp32_firmware/PetGraphics.h?raw';
import cydEngine from '../../esp32_firmware/TamagotchiCoreEngine.h?raw';
import cydRulesJson from '../../esp32_firmware/rules.json?raw';
import cydPartitions from '../../esp32_firmware/partitions.csv?raw';
import cydPio from '../../esp32_firmware/platformio.ini?raw';
import cydReadme from '../../esp32_firmware/README.md?raw';

export const FIRMWARE_FILES: Record<string, { filename: string; description: string; code: string }> = {
  'main/app_main.cpp': {
    filename: 'main/app_main.cpp',
    description: 'Native ESP-IDF app_main() FreeRTOS loop with LVGL UI, SoftAP portal, P2P sockets, and A/B OTA.',
    code: appMainCpp,
  },
  'main/cyd_lvgl_ui.cpp': {
    filename: 'main/cyd_lvgl_ui.cpp',
    description: 'LVGL v8.4 Dual-Pet UI with 16x16 pixel-art sprites, horizontal feed flyout, and 5-tab + chat modals.',
    code: lvglUiCpp,
  },
  'main/cyd_display.cpp': {
    filename: 'main/cyd_display.cpp',
    description: 'Native ESP-IDF SPI2_HOST ILI9341 320x240 DMA display driver with 0x21 CYD color inversion fix.',
    code: cydDisplayCpp,
  },
  'main/cyd_touch.c': {
    filename: 'main/cyd_touch.c',
    description: 'Native ESP-IDF SPI3_HOST XPT2046 resistive touch driver calibrated for CYD landscape.',
    code: cydTouchC,
  },
  'main/cyd_net.cpp': {
    filename: 'main/cyd_net.cpp',
    description: 'ESP-IDF WiFi STA/SoftAP web portal, lwIP P2P sockets (port 8888), PHP HTTP client, and esp_ota_ops A/B updater.',
    code: cydNetCpp,
  },
  'main/cyd_storage.cpp': {
    filename: 'main/cyd_storage.cpp',
    description: 'NVS flash persistence ("cyd_pet") and SPIFFS /spiffs/rules.json loader.',
    code: cydStorageCpp,
  },
  'main/cyd_hw.c': {
    filename: 'main/cyd_hw.c',
    description: 'ESP-IDF GPIO RGB status LEDs, BOOT button (GPIO 0), and LEDC piezo speaker driver.',
    code: cydHwC,
  },
  'TamagotchiCoreEngine.h': {
    filename: 'TamagotchiCoreEngine.h',
    description: 'Zero-heap UI-agnostic C++ game runtime engine shared across ESP-IDF and host unit tests.',
    code: cydEngine,
  },
  'PetGraphics.h': {
    filename: 'PetGraphics.h',
    description: '16-bit RGB565 biome palettes, 16x16 pixel-art sprites, and procedural pet/egg artwork.',
    code: cydGraphics,
  },
  'main/lv_conf.h': {
    filename: 'main/lv_conf.h',
    description: 'LVGL v8.4 configuration (byte-swapped RGB565, chroma-key transparency, 32KB heap).',
    code: lvConfH,
  },
  'main/idf_component.yml': {
    filename: 'main/idf_component.yml',
    description: 'ESP-IDF Component Manager manifest declaring lvgl/lvgl ^8.4.0 and espressif/cjson (IDF >= 6.0).',
    code: idfCompYml,
  },
  'main/CMakeLists.txt': {
    filename: 'main/CMakeLists.txt',
    description: 'ESP-IDF main component source registration.',
    code: mainCmake,
  },
  'CMakeLists.txt': {
    filename: 'CMakeLists.txt',
    description: 'Root ESP-IDF v5/v6.1 project CMake configuration (with GCC 15 C17 flags for third-party libs).',
    code: rootCmake,
  },
  'sdkconfig.defaults': {
    filename: 'sdkconfig.defaults',
    description: 'ESP-IDF default sdkconfig: 4MB flash, custom A/B partitions.csv, rollback, 1kHz FreeRTOS.',
    code: sdkDefaults,
  },
  'partitions.csv': {
    filename: 'partitions.csv',
    description: 'ESP32 4MB Dual A/B Partition Table (ota_0 & ota_1 1.5MB each, otadata, spiffs 896KB).',
    code: cydPartitions,
  },
  'cyd_config.h': {
    filename: 'cyd_config.h',
    description: 'Hardware pinout & SPI host configuration for ESP32-2432S028R.',
    code: cydConfigH,
  },
  'CYD_Pins.h': {
    filename: 'CYD_Pins.h',
    description: 'Hardware pin constants for display, touch, RGB LED, buzzer, LDR, and SD.',
    code: cydPins,
  },
  'config.h': {
    filename: 'config.h',
    description: 'Default WiFi, Device IDs, Language (LANG_DE), Biome, and PHP server settings.',
    code: cydConfig,
  },
  'rules.json': {
    filename: 'rules.json',
    description: 'Unified JSON Game Mechanics & RPG specification.',
    code: cydRulesJson,
  },
  'platformio.ini': {
    filename: 'platformio.ini',
    description: 'PlatformIO legacy fallback config using framework = espidf + LVGL v8.4.',
    code: cydPio,
  },
  'README.md': {
    filename: 'README.md',
    description: 'ESP-IDF & PlatformIO build, flashing, and OTA documentation.',
    code: cydReadme,
  },
};
