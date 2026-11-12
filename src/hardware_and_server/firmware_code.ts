/**
 * ESP32 Cheap Yellow Display (CYD - ESP32-2432S028R) Tamagotchi RPG Firmware
 * Based on JanPeter1/ESP_CoGo hardware initialization and setup.
 */

import cydIno from '../../esp32_firmware/ESP32_Tamagotchi_CYD.ino?raw';
import cydPins from '../../esp32_firmware/CYD_Pins.h?raw';
import cydConfigH from '../../esp32_firmware/cyd_config.h?raw';
import cydConfig from '../../esp32_firmware/config.h?raw';
import cydGraphics from '../../esp32_firmware/PetGraphics.h?raw';
import cydEngine from '../../esp32_firmware/TamagotchiCoreEngine.h?raw';
import cydRules from '../../esp32_firmware/tamagotchi_rules.cfg?raw';
import cydRulesJson from '../../esp32_firmware/rules.json?raw';
import cydPartitions from '../../esp32_firmware/partitions.csv?raw';
import cydPio from '../../esp32_firmware/platformio.ini?raw';
import cydReadme from '../../esp32_firmware/README.md?raw';

export const FIRMWARE_FILES: Record<string, { filename: string; description: string; code: string }> = {
  'ESP32_Tamagotchi_CYD.ino': {
    filename: 'ESP32_Tamagotchi_CYD.ino',
    description: 'Main Arduino sketch with ILI9341 320x240 LCD rendering, XPT2046 touch, RGB LED, buzzer audio & WiFi socket loop.',
    code: cydIno,
  },
  'cyd_config.h': {
    filename: 'cyd_config.h',
    description: 'Hardware pinout & display/touch config for ESP32-2432S028R (ILI9341 320x240 + XPT2046 touch + RGB LED + Audio).',
    code: cydConfigH,
  },
  'partitions.csv': {
    filename: 'partitions.csv',
    description: 'ESP32 4MB Dual A/B Partition Table (ota_0 & ota_1 1.5MB each, otadata, spiffs 896KB for dynamic rules.json).',
    code: cydPartitions,
  },
  'rules.json': {
    filename: 'rules.json',
    description: 'Unified JSON Game Mechanics & RPG specification (care mistakes, biomes, bilingual recipes, jobs, spells).',
    code: cydRulesJson,
  },
  'TamagotchiCoreEngine.h': {
    filename: 'TamagotchiCoreEngine.h',
    description: 'UI-independent, hardware-decoupled C++ runtime engine that interprets tamagotchi_rules.cfg & rules.json at runtime.',
    code: cydEngine,
  },
  'tamagotchi_rules.cfg': {
    filename: 'tamagotchi_rules.cfg',
    description: 'Human-readable game mechanics configuration file for ESP32 flash / SD card (legacy INI/CFG format).',
    code: cydRules,
  },
  'CYD_Pins.h': {
    filename: 'CYD_Pins.h',
    description: 'Hardware pin definitions matching ESP_CoGo for the ESP32-2432S028R display.',
    code: cydPins,
  },
  'PetGraphics.h': {
    filename: 'PetGraphics.h',
    description: '16-bit 565 color palettes for 4 biomes, procedural pixel art pet sprites, egg hatching, and touch buttons.',
    code: cydGraphics,
  },
  'config.h': {
    filename: 'config.h',
    description: 'User configuration: 2.4GHz WiFi credentials, Device IDs (DEV1 / DEV2), and Server URL.',
    code: cydConfig,
  },
  'platformio.ini': {
    filename: 'platformio.ini',
    description: 'Preconfigured PlatformIO project with TFT_eSPI build flags, dual A/B partition table, and dependencies.',
    code: cydPio,
  },
  'README.md': {
    filename: 'README.md',
    description: 'Step-by-step flashing guide for Arduino IDE, PlatformIO, and A/B OTA update setup.',
    code: cydReadme,
  },
};

