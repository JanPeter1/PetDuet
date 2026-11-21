/**
 * ============================================================================
 * PetDuet ESP32 CYD - Rules Manager Implementation
 * ============================================================================
 * Loads, parses, and hot-reloads rules.json from SPIFFS/Server
 * ============================================================================
 */
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include "esp_log.h"
#include "esp_vfs.h"
#include "esp_timer.h"
#include "cJSON.h"
#include "RulesManager.h"
#include "TamagotchiCoreEngine.h"

static const char* TAG = "RulesMgr";

esp_err_t rules_init(RulesManager* rm, const char* spiffs_path) {
    if (!rm || !spiffs_path) return ESP_ERR_INVALID_ARG;

    memset(rm, 0, sizeof(RulesManager));
    strncpy(rm->rules_path, spiffs_path, sizeof(rm->rules_path)-1);
    rm->loaded = false;
    rm->dirty = false;

    ESP_LOGI(TAG, "Rules manager initialized: %s", spiffs_path);
    return ESP_OK;
}

void rules_deinit(RulesManager* rm) {
    if (!rm) return;
    // Nothing to clean up
}

esp_err_t rules_load(RulesManager* rm) {
    if (!rm) return ESP_ERR_INVALID_ARG;

    return rules_load_from_spiffs(rm);
}

esp_err_t rules_reload(RulesManager* rm) {
    if (!rm) return ESP_ERR_INVALID_ARG;
    rm->dirty = true;
    return rules_load_from_spiffs(rm);
}

esp_err_t rules_load_from_spiffs(RulesManager* rm) {
    if (!rm) return ESP_ERR_INVALID_ARG;

    FILE* f = fopen(rm->rules_path, "r");
    if (!f) {
        ESP_LOGW(TAG, "Rules file not found: %s", rm->rules_path);
        return ESP_ERR_NOT_FOUND;
    }

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);

    if (size > MAX_RULES_JSON_SIZE) {
        ESP_LOGE(TAG, "Rules file too large: %ld bytes", size);
        fclose(f);
        return ESP_ERR_NO_MEM;
    }

    char* json_str = (char*)malloc(size + 1);
    if (!json_str) {
        fclose(f);
        return ESP_ERR_NO_MEM;
    }

    fread(json_str, 1, size, f);
    json_str[size] = '\0';
    fclose(f);

    esp_err_t ret = engine_parse_rules_json(json_str, &rm->rules);
    if (ret == ESP_OK) {
        // Extract version from JSON
        cJSON* root = cJSON_Parse(json_str);
        if (root) {
            cJSON* ver = cJSON_GetObjectItem(root, "version");
            if (ver) strncpy(rm->rules_version, ver->valuestring, MAX_STRING_LEN-1);
            cJSON_Delete(root);
        }
        rm->loaded = true;
        rm->dirty = false;
        rm->last_modified = esp_timer_get_time() / 1000;

        if (rm->on_rules_changed) {
            rm->on_rules_changed(&rm->rules, rm->user_data);
        }

        ESP_LOGI(TAG, "Rules loaded successfully: v%s", rm->rules_version);
    }

    free(json_str);
    return ret;
}

esp_err_t rules_save_to_spiffs(RulesManager* rm, const char* json_data, size_t len) {
    if (!rm || !json_data) return ESP_ERR_INVALID_ARG;

    // Write to temp file first (atomic)
    char temp_path[160];
    snprintf(temp_path, sizeof(temp_path), "%s.tmp", rm->rules_path);

    FILE* f = fopen(temp_path, "w");
    if (!f) return ESP_ERR_INVALID_STATE;

    size_t written = fwrite(json_data, 1, len, f);
    fclose(f);

    if (written != len) {
        unlink(temp_path);
        return ESP_ERR_NO_MEM;
    }

    // Atomic rename
    if (rename(temp_path, rm->rules_path) != 0) {
        unlink(temp_path);
        return ESP_ERR_INVALID_STATE;
    }

    rm->dirty = true; // Trigger reload
    return ESP_OK;
}

bool rules_file_exists(RulesManager* rm) {
    if (!rm) return false;
    struct stat st;
    return stat(rm->rules_path, &st) == 0;
}

uint32_t rules_get_file_size(RulesManager* rm) {
    if (!rm) return 0;
    struct stat st;
    if (stat(rm->rules_path, &st) == 0) {
        return st.st_size;
    }
    return 0;
}

const char* rules_get_version(RulesManager* rm) {
    return rm ? rm->rules_version : "";
}

bool rules_version_matches(RulesManager* rm, const char* version) {
    if (!rm || !version) return false;
    return strcmp(rm->rules_version, version) == 0;
}

void rules_set_callback(RulesManager* rm,
                        void (*callback)(const RulesData*, void*), void* user_data) {
    if (!rm) return;
    rm->on_rules_changed = callback;
    rm->user_data = user_data;
}

void rules_check_for_updates(RulesManager* rm) {
    if (!rm) return;

    // Check if file was modified
    struct stat st;
    if (stat(rm->rules_path, &st) == 0) {
        if (st.st_mtime * 1000 > rm->last_modified) {
            rm->dirty = true;
            rules_load_from_spiffs(rm);
        }
    }
}

const RulesData* rules_get_data(RulesManager* rm) {
    return rm && rm->loaded ? &rm->rules : NULL;
}

bool rules_is_valid(RulesManager* rm) {
    return rm && rm->loaded && rm->rules.valid;
}

char* rules_generate_json(const RulesData* rules) {
    // Not implemented - would serialize rules to JSON
    return NULL;
}

esp_err_t rules_validate_json(const char* json_str) {
    if (!json_str) return ESP_ERR_INVALID_ARG;

    cJSON* root = cJSON_Parse(json_str);
    if (!root) return ESP_ERR_INVALID_ARG;

    cJSON_Delete(root);
    return ESP_OK;
}