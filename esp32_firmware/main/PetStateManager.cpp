/**
 * ============================================================================
 * PetDuet ESP32 CYD - Pet State Manager Implementation
 * ============================================================================
 * Manages pet state persistence (NVS), partner state, and synchronization
 * ============================================================================
 */
#include <string.h>
#include <stdlib.h>
#include "esp_log.h"
#include "esp_timer.h"
#include "nvs.h"
#include "cJSON.h"
#include "PetStateManager.h"
#include "TamagotchiCoreEngine.h"
#include "NetworkManager.h"

static const char* TAG = "PetStateMgr";

esp_err_t pet_mgr_init(PetStateManager* pm, NetworkManager* net,
                       const RulesData* rules, const char* device_id,
                       const char* nvs_namespace) {
    if (!pm || !net || !rules || !device_id || !nvs_namespace) return ESP_ERR_INVALID_ARG;

    memset(pm, 0, sizeof(PetStateManager));
    pm->net = net;
    pm->rules = rules;
    pm->conflict_strategy = CONFLICT_MERGE;

    strncpy(pm->nvs_namespace, nvs_namespace, sizeof(pm->nvs_namespace)-1);

    // Open NVS namespace
    esp_err_t err = nvs_open(nvs_namespace, NVS_READWRITE, &pm->nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open NVS: %s", esp_err_to_name(err));
        return err;
    }

    // Initialize pet state
    ESP_ERROR_CHECK(engine_init(&pm->pet, pm->rules));
    strncpy(pm->pet.device_id, device_id, MAX_STRING_LEN-1);

    ESP_LOGI(TAG, "Pet state manager initialized: %s", device_id);
    return ESP_OK;
}

void pet_mgr_deinit(PetStateManager* pm) {
    if (!pm) return;

    if (pm->nvs_handle) {
        nvs_close(pm->nvs_handle);
    }
}

PetState* pet_mgr_get_pet(PetStateManager* pm) {
    return pm ? &pm->pet : NULL;
}

PetState* pet_mgr_get_partner(PetStateManager* pm) {
    return pm && pm->partner_valid ? &pm->partner_pet : NULL;
}

bool pet_mgr_has_partner(PetStateManager* pm) {
    return pm && pm->partner_valid;
}

esp_err_t pet_mgr_save_local(PetStateManager* pm) {
    if (!pm) return ESP_ERR_INVALID_ARG;

    // Serialize pet state to JSON
    char* json = engine_generate_pet_json(&pm->pet);
    if (!json) return ESP_ERR_NO_MEM;

    // Save to NVS
    esp_err_t err = nvs_set_str(pm->nvs_handle, "pet_state", json);
    if (err == ESP_OK) {
        err = nvs_commit(pm->nvs_handle);
    }

    if (err == ESP_OK) {
        pm->local_dirty = false;
        pm->last_local_save = esp_timer_get_time() / 1000;
        ESP_LOGD(TAG, "Pet state saved locally");
    }

    free(json);
    return err;
}

esp_err_t pet_mgr_load_local(PetStateManager* pm) {
    if (!pm) return ESP_ERR_INVALID_ARG;

    // Get required size
    size_t required_size = 0;
    esp_err_t err = nvs_get_str(pm->nvs_handle, "pet_state", NULL, &required_size);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGI(TAG, "No saved pet state found, starting fresh");
        return ESP_ERR_NOT_FOUND;
    }
    if (err != ESP_OK) return err;

    // Read JSON
    char* json = (char*)malloc(required_size);
    if (!json) return ESP_ERR_NO_MEM;

    err = nvs_get_str(pm->nvs_handle, "pet_state", json, &required_size);
    if (err == ESP_OK) {
        err = engine_parse_pet_json(json, &pm->pet);
        if (err == ESP_OK) {
            pm->pet.initialized = true;
            ESP_LOGI(TAG, "Pet state loaded: %s (Lv.%d)", pm->pet.trainer_name, pm->pet.level);
        }
    }

    free(json);
    return err;
}

esp_err_t pet_mgr_save_to_nvs(PetStateManager* pm, const PetState* pet, const char* key) {
    if (!pm || !pet || !key) return ESP_ERR_INVALID_ARG;

    char* json = engine_generate_pet_json(pet);
    if (!json) return ESP_ERR_NO_MEM;

    esp_err_t err = nvs_set_str(pm->nvs_handle, key, json);
    if (err == ESP_OK) err = nvs_commit(pm->nvs_handle);

    free(json);
    return err;
}

esp_err_t pet_mgr_load_from_nvs(PetStateManager* pm, PetState* pet, const char* key) {
    if (!pm || !pet || !key) return ESP_ERR_INVALID_ARG;

    size_t required_size = 0;
    esp_err_t err = nvs_get_str(pm->nvs_handle, key, NULL, &required_size);
    if (err != ESP_OK) return err;

    char* json = (char*)malloc(required_size);
    if (!json) return ESP_ERR_NO_MEM;

    err = nvs_get_str(pm->nvs_handle, key, json, &required_size);
    if (err == ESP_OK) {
        err = engine_parse_pet_json(json, pet);
    }

    free(json);
    return err;
}

esp_err_t pet_mgr_sync_with_server(PetStateManager* pm) {
    if (!pm) return ESP_ERR_INVALID_ARG;

    pm->server_sync_pending = true;
    esp_err_t err = net_full_sync(pm->net, &pm->pet, &pm->partner_pet);

    if (err == ESP_OK) {
        pm->last_server_sync = esp_timer_get_time() / 1000;
        pm->server_sync_pending = false;

        if (pm->on_sync_complete) {
            pm->on_sync_complete(true, pm->user_data);
        }
    } else {
        pm->server_sync_pending = false;
        if (pm->on_sync_complete) {
            pm->on_sync_complete(false, pm->user_data);
        }
    }

    return err;
}

esp_err_t pet_mgr_push_to_server(PetStateManager* pm) {
    if (!pm) return ESP_ERR_INVALID_ARG;
    return net_update_pet(pm->net, &pm->pet);
}

esp_err_t pet_mgr_pull_from_server(PetStateManager* pm) {
    if (!pm) return ESP_ERR_INVALID_ARG;
    return net_get_pet(pm->net, &pm->pet);
}

esp_err_t pet_mgr_request_partner_sync(PetStateManager* pm) {
    if (!pm) return ESP_ERR_INVALID_ARG;

    // Send sync request via WebSocket
    WSMessage msg;
    memset(&msg, 0, sizeof(msg));
    msg.type = WS_MSG_TYPE_SYNC_REQUEST;
    msg.timestamp = (uint32_t)(esp_timer_get_time() / 1000);
    strncpy(msg.from_device, pm->pet.device_id, MAX_STRING_LEN-1);
    strncpy(msg.to_device, pm->pet.partner_device_id, MAX_STRING_LEN-1);

    pm->partner_sync_pending = true;
    return net_ws_send(pm->net, &msg);
}

esp_err_t pet_mgr_handle_partner_data(PetStateManager* pm, const char* json_data) {
    if (!pm || !json_data) return ESP_ERR_INVALID_ARG;

    esp_err_t err = engine_parse_pet_json(json_data, &pm->partner_pet);
    if (err == ESP_OK) {
        pm->partner_valid = true;
        pm->partner_pet.initialized = true;
        pm->last_partner_sync = esp_timer_get_time() / 1000;
        pm->partner_sync_pending = false;

        if (pm->on_partner_updated) {
            pm->on_partner_updated(&pm->partner_pet, pm->user_data);
        }
    }

    return err;
}

void pet_mgr_mark_dirty(PetStateManager* pm) {
    if (pm) pm->local_dirty = true;
}

void pet_mgr_mark_partner_dirty(PetStateManager* pm) {
    if (pm) pm->partner_valid = true; // Mark as needing sync
}

void pet_mgr_set_partner_info(PetStateManager* pm, const char* device_id,
                              const char* trainer_name, const char* biome_id) {
    if (!pm) return;

    strncpy(pm->pet.partner_device_id, device_id, MAX_STRING_LEN-1);
    strncpy(pm->pet.partner_trainer_name, trainer_name, MAX_STRING_LEN-1);
    strncpy(pm->pet.partner_biome_id, biome_id, MAX_STRING_LEN-1);
}

esp_err_t pet_mgr_update_partner_state(PetStateManager* pm, const PetState* partner) {
    if (!pm || !partner) return ESP_ERR_INVALID_ARG;

    memcpy(&pm->partner_pet, partner, sizeof(PetState));
    pm->partner_valid = true;

    if (pm->on_partner_updated) {
        pm->on_partner_updated(&pm->partner_pet, pm->user_data);
    }
    return ESP_OK;
}

esp_err_t pet_mgr_resolve_conflict(PetStateManager* pm, const PetState* server_pet) {
    if (!pm || !server_pet) return ESP_ERR_INVALID_ARG;

    switch (pm->conflict_strategy) {
        case CONFLICT_LOCAL_WINS:
            // Keep local, push to server
            return pet_mgr_push_to_server(pm);

        case CONFLICT_SERVER_WINS:
            // Overwrite local with server
            memcpy(&pm->pet, server_pet, sizeof(PetState));
            pm->local_dirty = false;
            return pet_mgr_save_local(pm);

        case CONFLICT_MERGE:
            // Merge: take newer timestamps for each field
            // Simplified: just take server for now
            memcpy(&pm->pet, server_pet, sizeof(PetState));
            pm->local_dirty = false;
            return pet_mgr_save_local(pm);

        case CONFLICT_MANUAL:
        default:
            // Would show conflict resolution UI
            return ESP_ERR_INVALID_STATE;
    }
}

bool pet_mgr_is_initialized(PetStateManager* pm) {
    return pm && pm->pet.initialized;
}

uint32_t pet_mgr_get_last_save_time(PetStateManager* pm) {
    return pm ? pm->last_local_save : 0;
}

void pet_mgr_set_callbacks(PetStateManager* pm,
                           void (*on_pet_changed)(const PetState*, void*),
                           void (*on_partner_updated)(const PetState*, void*),
                           void (*on_sync_complete)(bool, void*),
                           void* user_data) {
    if (!pm) return;
    pm->on_pet_changed = on_pet_changed;
    pm->on_partner_updated = on_partner_updated;
    pm->on_sync_complete = on_sync_complete;
    pm->user_data = user_data;
}

esp_err_t pet_mgr_factory_reset(PetStateManager* pm) {
    if (!pm) return ESP_ERR_INVALID_ARG;

    // Erase NVS namespace
    esp_err_t err = nvs_erase_all(pm->nvs_handle);
    if (err == ESP_OK) {
        err = nvs_commit(pm->nvs_handle);
    }

    // Reinitialize pet state
    if (err == ESP_OK && pm->rules) {
        memset(&pm->pet, 0, sizeof(PetState));
        engine_init(&pm->pet, pm->rules);
        strncpy(pm->pet.device_id, DEVICE_ID, MAX_STRING_LEN-1);
        strncpy(pm->pet.trainer_name, TRAINER_NAME, MAX_STRING_LEN-1);
        strncpy(pm->pet.biome_id, DEFAULT_BIOME, MAX_STRING_LEN-1);
        pm->local_dirty = true;
        pet_mgr_save_local(pm);
    }

    return err;
}