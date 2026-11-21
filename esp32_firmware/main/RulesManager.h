/**
 * ============================================================================
 * PetDuet ESP32 CYD - Rules Manager
 * ============================================================================
 * Loads, parses, and hot-reloads rules.json from SPIFFS/Server
 * ============================================================================
 */
#ifndef RULES_MANAGER_H
#define RULES_MANAGER_H

#include <stdint.h>
#include <stdbool.h>
#include "TamagotchiCoreEngine.h"

// ============================================================================
// Rules Manager Context
// ============================================================================
typedef struct {
    RulesData rules;
    char rules_path[128];
    char rules_version[MAX_STRING_LEN];
    uint32_t last_modified;
    bool loaded;
    bool dirty;  // Needs reload

    // Hot-reload callback
    void (*on_rules_changed)(const RulesData* new_rules, void* user_data);
    void* user_data;
} RulesManager;

// ============================================================================
// API
// ============================================================================

// Initialization
esp_err_t rules_init(RulesManager* rm, const char* spiffs_path);
void rules_deinit(RulesManager* rm);

// Loading
esp_err_t rules_load(RulesManager* rm);
esp_err_t rules_reload(RulesManager* rm);

// File operations
esp_err_t rules_save_to_spiffs(RulesManager* rm, const char* json_data, size_t len);
esp_err_t rules_load_from_spiffs(RulesManager* rm);
bool rules_file_exists(RulesManager* rm);
uint32_t rules_get_file_size(RulesManager* rm);

// Version management
const char* rules_get_version(RulesManager* rm);
bool rules_version_matches(RulesManager* rm, const char* version);

// Hot-reload
void rules_set_callback(RulesManager* rm,
                        void (*callback)(const RulesData*, void*), void* user_data);
void rules_check_for_updates(RulesManager* rm);

// Accessors
const RulesData* rules_get_data(RulesManager* rm);
bool rules_is_valid(RulesManager* rm);

// JSON helpers
char* rules_generate_json(const RulesData* rules);
esp_err_t rules_validate_json(const char* json_str);

#endif // RULES_MANAGER_H