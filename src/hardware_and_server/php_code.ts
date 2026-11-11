/**
 * Ready-to-deploy PHP Server Source Code
 * Directly synced from /php_server/
 */

import registerPhp from '../../php_server/register.php?raw';
import findFriendPhp from '../../php_server/find_friend.php?raw';
import syncPhp from '../../php_server/sync.php?raw';
import cloudSavePhp from '../../php_server/cloud_save.php?raw';
import leaderboardPhp from '../../php_server/leaderboard.php?raw';
import indexPhp from '../../php_server/index.php?raw';
import otaCheckPhp from '../../php_server/ota_check.php?raw';
import otaDownloadPhp from '../../php_server/ota_download.php?raw';
import shopPhp from '../../php_server/shop.php?raw';
import otaManifestJson from '../../php_server/ota_manifest.json?raw';

export const PHP_SERVER_FILES: Record<string, { filename: string; description: string; code: string }> = {
  'ota_check.php': {
    filename: 'ota_check.php',
    description: 'OTA Update check handler for A/B firmware binaries and SPIFFS rules.json hot-reloads.',
    code: otaCheckPhp,
  },
  'ota_download.php': {
    filename: 'ota_download.php',
    description: 'High-speed binary streaming pipeline for ESP32 firmware binaries and live rules.json.',
    code: otaDownloadPhp,
  },
  'shop.php': {
    filename: 'shop.php',
    description: 'Shop catalog & purchase validation API for items, crafting recipes, and co-op spells.',
    code: shopPhp,
  },
  'ota_manifest.json': {
    filename: 'ota_manifest.json',
    description: 'Version manifest defining latest firmware binary and dynamic rules metadata.',
    code: otaManifestJson,
  },
  'register.php': {
    filename: 'register.php',
    description: 'Registers CYD device ID, IP address, port, pet name, biome, and heartbeat.',
    code: registerPhp,
  },
  'find_friend.php': {
    filename: 'find_friend.php',
    description: 'Looks up friend CYD device by ID to fetch remote IP/port for direct socket or relay.',
    code: findFriendPhp,
  },
  'sync.php': {
    filename: 'sync.php',
    description: 'Relays real-time treats, chat messages, magic spells, and trade offers between CYDs.',
    code: syncPhp,
  },
  'cloud_save.php': {
    filename: 'cloud_save.php',
    description: 'Persistent cloud backups for pet state, levels, inventory, and RPG progress.',
    code: cloudSavePhp,
  },
  'leaderboard.php': {
    filename: 'leaderboard.php',
    description: 'Community rankings by level, XP, recipe crafting, and social treats.',
    code: leaderboardPhp,
  },
  'index.php': {
    filename: 'index.php',
    description: 'Server landing and status diagnostic dashboard with live OTA & Shop overview.',
    code: indexPhp,
  },
};
