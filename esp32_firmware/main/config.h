/**
 * ============================================================================
 * PetDuet ESP32 CYD - Configuration
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
// 4. PHP Server URL (Cloud Save, Peer Discovery, OTA & Shop)
// Point this to your PHP server deployment or your local IP:
// e.g., "http://192.168.1.100/petduet" or "https://yourdomain.com/petduet"
// ----------------------------------------------------------------------------
#define PHP_SERVER_BASE_URL "http://192.168.1.100/petduet"

// Versioning and OTA Updates (A/B Dual Partition Switching)
#define FIRMWARE_VERSION        "2.1.11"
#define RULES_VERSION           "1.0.4"
#define OTA_CHECK_INTERVAL_MS   (15 * 60 * 1000) // Check for updates every 15 minutes

// Endpoint Paths
#define OTA_CHECK_ENDPOINT      "/api/ota/check"
#define OTA_FIRMWARE_ENDPOINT   "/api/ota/firmware"
#define OTA_RULES_ENDPOINT      "/api/rules"
#define SYNC_ENDPOINT           "/api/sync"
#define PET_GET_ENDPOINT        "/api/pet/get-pet"
#define PET_UPDATE_ENDPOINT     "/api/pet/update"
#define REGISTER_ENDPOINT       "/api/register-device"
#define LOGIN_ENDPOINT          "/api/login"

// Synchronization intervals (milliseconds)
#define SERVER_SYNC_INTERVAL_MS 10000 // Heartbeat every 10 seconds
#define PET_TICK_INTERVAL_MS    60000 // Hunger/energy decay step (60 seconds)

// ----------------------------------------------------------------------------
// 5. Language Selection (Bilingual: English & Deutsch)
// Set LANG_DE to 1 for Deutsch, or 0 for English
// ----------------------------------------------------------------------------
#define LANG_DE 1

#endif // CONFIG_H