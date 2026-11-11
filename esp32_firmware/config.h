/**
 * ============================================================================
 * ESP32 CYD Retro Tamagotchi Companion Configuration
 * ============================================================================
 */
#ifndef CONFIG_H
#define CONFIG_H

// ----------------------------------------------------------------------------
// 1. WiFi Network Credentials (Must be 2.4GHz)
// ----------------------------------------------------------------------------
#define WIFI_SSID     "YOUR_WIFI_SSID"
#define WIFI_PASSWORD "YOUR_WIFI_PASSWORD"

// ----------------------------------------------------------------------------
// 2. Unique Device & Peer Identification
// Set DEV1 on your first CYD, and DEV2 on your friend's CYD!
// ----------------------------------------------------------------------------
#define DEVICE_ID     "CYD-DEV1"      // e.g. "CYD-DEV1" or "CYD-DEV2"
#define TRAINER_NAME  "PixelTrainer"  // Name shown on friend's display
#define FRIEND_DEV_ID "CYD-DEV2"      // Device ID of your buddy's CYD

// ----------------------------------------------------------------------------
// 3. Chosen Biome at Startup
// Options: "desert", "grassland", "water", "forest"
// ----------------------------------------------------------------------------
#define DEFAULT_BIOME "grassland"

// ----------------------------------------------------------------------------
// 4. Direct Socket Port for CYD-to-CYD Peer Communication
// ----------------------------------------------------------------------------
#define SOCKET_PORT   8888

// ----------------------------------------------------------------------------
// 5. PHP Server URL (Cloud Save, Peer Discovery, OTA & Shop)
// Point this to your PHP server deployment or your local IP:
// e.g., "http://192.168.1.100/cyd-tamagotchi" or "https://yourdomain.com/cyd"
// ----------------------------------------------------------------------------
#define PHP_SERVER_BASE_URL "http://192.168.1.100/cyd-tamagotchi"

// Versioning and OTA Updates (A/B Dual Partition Switching)
#define FIRMWARE_VERSION        "2.1.0"
#define RULES_VERSION           "1.0.4"
#define OTA_CHECK_INTERVAL_MS   (15 * 60 * 1000) // Check for updates every 15 minutes

// Endpoint Paths
#define OTA_CHECK_ENDPOINT      "/ota_check.php"
#define OTA_DOWNLOAD_ENDPOINT   "/ota_download.php"
#define SHOP_ENDPOINT           "/shop.php"
#define CLOUD_SAVE_ENDPOINT     "/cloud_save.php"

// Synchronization intervals (milliseconds)
#define SERVER_SYNC_INTERVAL_MS 10000 // Heartbeat every 10 seconds
#define PET_TICK_INTERVAL_MS     6000 // Hunger/energy decay step

// ----------------------------------------------------------------------------
// 6. Language Selection (Bilingual: English & Deutsch)
// Set LANG_DE to 1 for Deutsch, or 0 for English
// ----------------------------------------------------------------------------
#define LANG_DE 1

#endif // CONFIG_H

