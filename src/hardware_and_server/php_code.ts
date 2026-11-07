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

export const PHP_SERVER_FILES: Record<string, { filename: string; description: string; code: string }> = {
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
    description: 'Server landing and status diagnostic dashboard.',
    code: indexPhp,
  },
};
