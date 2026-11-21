/**
 * ============================================================================
 * PetDuet ESP32 CYD - OTA Manager
 * ============================================================================
 * Two-tier OTA: Firmware (A/B partitions) + Rules (SPIFFS, zero-downtime)
 * ============================================================================
 */
#ifndef OTA_MANAGER_H
#define OTA_MANAGER_H

#include <stdint.h>
#include <stdbool.h>
#include "config.h"
#include "NetworkManager.h"
#include "RulesManager.h"
#include "esp_https_ota.h"

// ============================================================================
// OTA State
// ============================================================================
typedef enum {
    OTA_STATE_IDLE = 0,
    OTA_STATE_CHECKING,
    OTA_STATE_FIRMWARE_AVAILABLE,
    OTA_STATE_RULES_AVAILABLE,
    OTA_STATE_DOWNLOADING_FIRMWARE,
    OTA_STATE_DOWNLOADING_RULES,
    OTA_STATE_VERIFYING,
    OTA_STATE_APPLYING,
    OTA_STATE_REBOOTING,
    OTA_STATE_ERROR,
    OTA_STATE_ROLLBACK
} OTAState;

// ============================================================================
// OTA Info from Server
// ============================================================================
typedef struct {
    char firmware_version[MAX_STRING_LEN];
    char rules_version[MAX_STRING_LEN];
    char firmware_url[256];
    char rules_url[256];
    char firmware_sha256[65];
    char rules_sha256[65];
    uint32_t firmware_size;
    uint32_t rules_size;
    bool firmware_mandatory;
    bool rules_mandatory;
    char release_notes[512];
    char release_notes_de[512];
} OTAManifest;

// ============================================================================
// OTA Manager Context
// ============================================================================
typedef struct {
    OTAState state;
    OTAManifest manifest;
    NetworkManager* net;

    // Progress tracking
    uint32_t downloaded_bytes;
    uint32_t total_bytes;
    uint8_t progress_percent;
    char status_message[128];

    // Firmware OTA
    esp_https_ota_handle_t ota_handle;
    const esp_partition_t* running_partition;
    const esp_partition_t* next_partition;

    // Rules OTA (SPIFFS)
    char rules_temp_path[128];
    bool rules_downloaded;

    // Timing
    uint32_t last_check_time;
    uint32_t check_interval_ms;

    // Auto-check
    bool auto_check_enabled;

    // Callbacks
    void (*on_progress)(uint8_t percent, const char* message, void* user_data);
    void (*on_complete)(bool success, const char* version, void* user_data);
    void* user_data;
} OTAManager;

// ============================================================================
// API
// ============================================================================

// Initialization
esp_err_t ota_init(OTAManager* om, NetworkManager* net);
void ota_deinit(OTAManager* om);

// Main check
esp_err_t ota_check_for_updates(OTAManager* om);

// Firmware OTA (A/B partition switch)
esp_err_t ota_download_firmware(OTAManager* om);
esp_err_t ota_verify_firmware(OTAManager* om);
esp_err_t ota_apply_firmware(OTAManager* om);
esp_err_t ota_rollback_firmware(OTAManager* om);

// Rules OTA (SPIFFS - zero downtime)
esp_err_t ota_download_rules(OTAManager* om);
esp_err_t ota_verify_rules(OTAManager* om);
esp_err_t ota_apply_rules(OTAManager* om, RulesManager* rm);

// Status
const char* ota_state_to_string(OTAState state);
const esp_partition_t* ota_get_running_partition(void);
const esp_partition_t* ota_get_next_partition(void);
char* ota_get_partition_label(const esp_partition_t* partition);
bool ota_is_rollback_possible(void);

// Utility
esp_err_t ota_report_result(OTAManager* om, bool success, const char* error_msg);
void ota_set_callbacks(OTAManager* om,
                       void (*on_progress)(uint8_t, const char*, void*),
                       void (*on_complete)(bool, const char*, void*),
                       void* user_data);
void ota_set_auto_check(OTAManager* om, bool enabled, uint32_t interval_ms);

// Partition info (for boot screen)
void ota_print_partition_info(void);

#endif // OTA_MANAGER_H