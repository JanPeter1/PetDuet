/**
 * ============================================================================
 * PetDuet ESP32 CYD - Pet State Manager
 * ============================================================================
 * Manages pet state persistence (NVS), partner state, and synchronization
 * ============================================================================
 */
#ifndef PET_STATE_MANAGER_H
#define PET_STATE_MANAGER_H

#include <stdint.h>
#include <stdbool.h>
#include "TamagotchiCoreEngine.h"
#include "NetworkManager.h"

// ============================================================================
// Conflict Resolution Strategy
// ============================================================================
typedef enum {
    CONFLICT_LOCAL_WINS = 0,
    CONFLICT_SERVER_WINS,
    CONFLICT_MERGE,
    CONFLICT_MANUAL
} ConflictStrategy;

// ============================================================================
// Pet State Manager Context
// ============================================================================
typedef struct {
    PetState pet;
    PetState partner_pet;
    bool partner_valid;

    // NVS
    nvs_handle_t nvs_handle;
    char nvs_namespace[32];

    // Network
    NetworkManager* net;

    // Sync state
    uint32_t last_local_save;
    uint32_t last_server_sync;
    uint32_t last_partner_sync;
    bool local_dirty;
    bool server_sync_pending;
    bool partner_sync_pending;

    // Conflict resolution
    ConflictStrategy conflict_strategy;

    // Callbacks
    void (*on_pet_changed)(const PetState* pet, void* user_data);
    void (*on_partner_updated)(const PetState* partner, void* user_data);
    void (*on_sync_complete)(bool success, void* user_data);
    void* user_data;
} PetStateManager;

// ============================================================================
// API
// ============================================================================

// Initialization
esp_err_t pet_mgr_init(PetStateManager* pm, NetworkManager* net,
                       const char* device_id, const char* nvs_namespace);
void pet_mgr_deinit(PetStateManager* pm);

// State access
PetState* pet_mgr_get_pet(PetStateManager* pm);
PetState* pet_mgr_get_partner(PetStateManager* pm);
bool pet_mgr_has_partner(PetStateManager* pm);

// Persistence
esp_err_t pet_mgr_save_local(PetStateManager* pm);
esp_err_t pet_mgr_load_local(PetStateManager* pm);
esp_err_t pet_mgr_save_to_nvs(PetStateManager* pm, const PetState* pet, const char* key);
esp_err_t pet_mgr_load_from_nvs(PetStateManager* pm, PetState* pet, const char* key);

// Server sync
esp_err_t pet_mgr_sync_with_server(PetStateManager* pm);
esp_err_t pet_mgr_push_to_server(PetStateManager* pm);
esp_err_t pet_mgr_pull_from_server(PetStateManager* pm);

// Partner sync (via WebSocket)
esp_err_t pet_mgr_request_partner_sync(PetStateManager* pm);
esp_err_t pet_mgr_handle_partner_data(PetStateManager* pm, const char* json_data);

// State modification (marks dirty)
void pet_mgr_mark_dirty(PetStateManager* pm);
void pet_mgr_mark_partner_dirty(PetStateManager* pm);

// Partner state
void pet_mgr_set_partner_info(PetStateManager* pm, const char* device_id,
                              const char* trainer_name, const char* biome_id);
esp_err_t pet_mgr_update_partner_state(PetStateManager* pm, const PetState* partner);

// Conflict resolution
esp_err_t pet_mgr_resolve_conflict(PetStateManager* pm, const PetState* server_pet);

// Utility
bool pet_mgr_is_initialized(PetStateManager* pm);
uint32_t pet_mgr_get_last_save_time(PetStateManager* pm);
void pet_mgr_set_callbacks(PetStateManager* pm,
                           void (*on_pet_changed)(const PetState*, void*),
                           void (*on_partner_updated)(const PetState*, void*),
                           void (*on_sync_complete)(bool, void*),
                           void* user_data);

// Factory reset
esp_err_t pet_mgr_factory_reset(PetStateManager* pm);

#endif // PET_STATE_MANAGER_H