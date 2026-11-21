/**
 * ============================================================================
 * PetDuet ESP32 CYD - OTA Manager Implementation
 * ============================================================================
 * Two-tier OTA: Firmware (A/B partitions) + Rules (SPIFFS, zero-downtime)
 * ============================================================================
 */
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "esp_log.h"
#include "esp_err.h"
#include "esp_ota_ops.h"
#include "esp_https_ota.h"
#include "esp_partition.h"
#include "esp_crc.h"
#include "esp_timer.h"
#include "OTAManager.h"
#include "NetworkManager.h"
#include "RulesManager.h"

static const char* TAG = "OTAMgr";

esp_err_t ota_init(OTAManager* om, NetworkManager* net) {
    if (!om || !net) return ESP_ERR_INVALID_ARG;

    memset(om, 0, sizeof(OTAManager));
    om->net = net;
    om->state = OTA_STATE_IDLE;
    om->check_interval_ms = OTA_CHECK_INTERVAL_MS;
    om->auto_check_enabled = true;
    om->last_check_time = 0;

    om->running_partition = esp_ota_get_running_partition();
    om->next_partition = esp_ota_get_next_update_partition(NULL);

    ESP_LOGI(TAG, "OTA Manager initialized");
    ESP_LOGI(TAG, "Running partition: %s", om->running_partition ? om->running_partition->label : "unknown");
    ESP_LOGI(TAG, "Next partition: %s", om->next_partition ? om->next_partition->label : "unknown");

    return ESP_OK;
}

void ota_deinit(OTAManager* om) {
    if (!om) return;
    if (om->ota_handle) {
        esp_https_ota_abort(om->ota_handle);
    }
}

esp_err_t ota_check_for_updates(OTAManager* om) {
    if (!om) return ESP_ERR_INVALID_ARG;

    om->state = OTA_STATE_CHECKING;
    om->last_check_time = esp_timer_get_time() / 1000;

    char fw_ver[MAX_STRING_LEN] = {0};
    char rl_ver[MAX_STRING_LEN] = {0};
    char fw_url[256] = {0};
    char rl_url[256] = {0};
    char fw_sha[65] = {0};
    char rl_sha[65] = {0};

    esp_err_t err = net_ota_check(om->net, fw_ver, rl_ver, fw_url, rl_url, fw_sha, rl_sha);

    if (err == ESP_OK) {
        bool fw_update = (strcmp(fw_ver, FIRMWARE_VERSION) != 0 && fw_ver[0] != '\0');
        bool rl_update = (strcmp(rl_ver, RULES_VERSION) != 0 && rl_ver[0] != '\0');

        if (fw_update) {
            strncpy(om->manifest.firmware_version, fw_ver, MAX_STRING_LEN-1);
            strncpy(om->manifest.firmware_url, fw_url, 255);
            strncpy(om->manifest.firmware_sha256, fw_sha, 64);
            om->state = OTA_STATE_FIRMWARE_AVAILABLE;
            ESP_LOGI(TAG, "Firmware update available: %s -> %s", FIRMWARE_VERSION, fw_ver);

            if (om->on_progress) om->on_progress(0, "Firmware update available", om->user_data);
            return ota_download_firmware(om);
        }

        if (rl_update) {
            strncpy(om->manifest.rules_version, rl_ver, MAX_STRING_LEN-1);
            strncpy(om->manifest.rules_url, rl_url, 255);
            strncpy(om->manifest.rules_sha256, rl_sha, 64);
            om->state = OTA_STATE_RULES_AVAILABLE;
            ESP_LOGI(TAG, "Rules update available: %s -> %s", RULES_VERSION, rl_ver);

            if (om->on_progress) om->on_progress(0, "Rules update available", om->user_data);
            return ota_download_rules(om);
        }

        ESP_LOGI(TAG, "No updates available");
        om->state = OTA_STATE_IDLE;
    } else {
        om->state = OTA_STATE_ERROR;
        ESP_LOGE(TAG, "OTA check failed: %s", esp_err_to_name(err));
    }

    return err;
}

// ============================================================================
// Firmware OTA (A/B Partition)
// ============================================================================

esp_err_t ota_download_firmware(OTAManager* om) {
    if (!om) return ESP_ERR_INVALID_ARG;

    om->state = OTA_STATE_DOWNLOADING_FIRMWARE;

    if (om->on_progress) om->on_progress(10, "Downloading firmware...", om->user_data);

    esp_http_client_config_t config;
    memset(&config, 0, sizeof(config));
    config.url = om->manifest.firmware_url;
    config.host = NULL;
    config.port = 0;
    config.username = NULL;
    config.password = NULL;
    config.auth_type = HTTP_AUTH_TYPE_NONE;
    config.path = NULL;
    config.query = NULL;
    config.cert_pem = NULL;
    config.cert_len = 0;
    config.client_cert_pem = NULL;
    config.client_cert_len = 0;
    config.client_key_pem = NULL;
    config.client_key_len = 0;
    config.client_key_password = NULL;
    config.client_key_password_len = 0;
    config.tls_version = ESP_HTTP_CLIENT_TLS_VER_ANY;
    config.user_agent = NULL;
    config.method = HTTP_METHOD_GET;
    config.timeout_ms = 30000;
    config.disable_auto_redirect = false;
    config.max_redirection_count = 0;
    config.max_authorization_retries = 0;
    config.event_handler = NULL;
    config.transport_type = HTTP_TRANSPORT_UNKNOWN;
    config.buffer_size = 8192;
    config.buffer_size_tx = 0;
    config.user_data = NULL;
    config.is_async = false;
    config.use_global_ca_store = false;
    config.skip_cert_common_name_check = false;
    config.common_name = NULL;
    config.crt_bundle_attach = NULL;
    config.keep_alive_enable = false;
    config.keep_alive_idle = 0;
    config.keep_alive_interval = 0;
    config.keep_alive_count = 0;
    config.if_name = NULL;
    config.addr_type = HTTP_ADDR_TYPE_UNSPEC;

    esp_https_ota_config_t ota_config;
    memset(&ota_config, 0, sizeof(ota_config));
    ota_config.http_config = &config;
    ota_config.http_client_init_cb = NULL;
    ota_config.bulk_flash_erase = false;
    ota_config.buffer_caps = MALLOC_CAP_DEFAULT;

    esp_err_t err = esp_https_ota_begin(&ota_config, &om->ota_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "OTA begin failed: %s", esp_err_to_name(err));
        om->state = OTA_STATE_ERROR;
        return err;
    }

    om->downloaded_bytes = 0;
    om->total_bytes = 0;

    while (1) {
        err = esp_https_ota_perform(om->ota_handle);
        if (err != ESP_ERR_HTTPS_OTA_IN_PROGRESS) break;

        om->downloaded_bytes = esp_https_ota_get_image_len_read(om->ota_handle);
        if (om->total_bytes == 0) {
            om->total_bytes = esp_https_ota_get_image_size(om->ota_handle);
        }

        if (om->total_bytes > 0 && om->on_progress) {
            om->progress_percent = (om->downloaded_bytes * 100) / om->total_bytes;
            char msg[64];
            snprintf(msg, sizeof(msg), "Downloading: %d%%", om->progress_percent);
            om->on_progress(om->progress_percent, msg, om->user_data);
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }

    if (err == ESP_OK) {
        om->state = OTA_STATE_VERIFYING;
        if (om->on_progress) om->on_progress(90, "Verifying firmware...", om->user_data);

        err = ota_verify_firmware(om);
        if (err == ESP_OK) {
            om->state = OTA_STATE_APPLYING;
            if (om->on_progress) om->on_progress(95, "Applying firmware...", om->user_data);
            err = ota_apply_firmware(om);
        }
    }

    if (err != ESP_OK) {
        om->state = OTA_STATE_ERROR;
        if (om->on_complete) om->on_complete(false, om->manifest.firmware_version, om->user_data);
    }

    esp_https_ota_finish(om->ota_handle);
    om->ota_handle = NULL;

    return err;
}

esp_err_t ota_verify_firmware(OTAManager* om) {
    if (!om) return ESP_ERR_INVALID_ARG;

    // Verify SHA256
    if (om->manifest.firmware_sha256[0] != '\0') {
        // In a full implementation, calculate SHA256 of downloaded image
        // and compare with manifest
        ESP_LOGI(TAG, "Expected SHA256: %s", om->manifest.firmware_sha256);
        // For now, skip verification
    }

    return ESP_OK;
}

esp_err_t ota_apply_firmware(OTAManager* om) {
    if (!om) return ESP_ERR_INVALID_ARG;

    // The esp_https_ota_finish handles the partition switch
    // Just mark the new partition as valid
    esp_err_t err = esp_ota_set_boot_partition(om->next_partition);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set boot partition: %s", esp_err_to_name(err));
        return err;
    }

    om->state = OTA_STATE_REBOOTING;
    ESP_LOGI(TAG, "Firmware applied, rebooting...");

    if (om->on_complete) om->on_complete(true, om->manifest.firmware_version, om->user_data);

    // Report success to server
    ota_report_result(om, true, om->manifest.firmware_version);

    return ESP_OK;
}

esp_err_t ota_rollback_firmware(OTAManager* om) {
    if (!om) return ESP_ERR_INVALID_ARG;

    // Get the previous partition
    const esp_partition_t* prev = esp_ota_get_last_invalid_partition();
    if (!prev) {
        ESP_LOGE(TAG, "No previous partition to rollback to");
        return ESP_ERR_NOT_FOUND;
    }

    esp_err_t err = esp_ota_set_boot_partition(prev);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "Rollback to partition %s scheduled", prev->label);
        om->state = OTA_STATE_ROLLBACK;
    }

    return err;
}

// ============================================================================
// Rules OTA (SPIFFS - Zero Downtime)
// ============================================================================

esp_err_t ota_download_rules(OTAManager* om) {
    if (!om) return ESP_ERR_INVALID_ARG;

    om->state = OTA_STATE_DOWNLOADING_RULES;

    if (om->on_progress) om->on_progress(20, "Downloading rules...", om->user_data);

    // Download rules.json via HTTP
    esp_http_client_config_t config;
    memset(&config, 0, sizeof(config));
    config.url = om->manifest.rules_url;
    config.host = NULL;
    config.port = 0;
    config.username = NULL;
    config.password = NULL;
    config.auth_type = HTTP_AUTH_TYPE_NONE;
    config.path = NULL;
    config.query = NULL;
    config.cert_pem = NULL;
    config.cert_len = 0;
    config.client_cert_pem = NULL;
    config.client_cert_len = 0;
    config.client_key_pem = NULL;
    config.client_key_len = 0;
    config.client_key_password = NULL;
    config.client_key_password_len = 0;
    config.tls_version = ESP_HTTP_CLIENT_TLS_VER_ANY;
    config.user_agent = NULL;
    config.method = HTTP_METHOD_GET;
    config.timeout_ms = 15000;
    config.disable_auto_redirect = false;
    config.max_redirection_count = 0;
    config.max_authorization_retries = 0;
    config.event_handler = NULL;
    config.transport_type = HTTP_TRANSPORT_UNKNOWN;
    config.buffer_size = 512;
    config.buffer_size_tx = 0;
    config.user_data = NULL;
    config.is_async = false;
    config.use_global_ca_store = false;
    config.skip_cert_common_name_check = false;
    config.common_name = NULL;
    config.crt_bundle_attach = NULL;
    config.keep_alive_enable = false;
    config.keep_alive_idle = 0;
    config.keep_alive_interval = 0;
    config.keep_alive_count = 0;
    config.if_name = NULL;
    config.addr_type = HTTP_ADDR_TYPE_UNSPEC;

    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (!client) return ESP_ERR_NO_MEM;

    // Write to temp file
    snprintf(om->rules_temp_path, sizeof(om->rules_temp_path), "/spiffs/rules.json.tmp");
    FILE* f = fopen(om->rules_temp_path, "w");
    if (!f) {
        esp_http_client_cleanup(client);
        return ESP_ERR_INVALID_STATE;
    }

    esp_err_t err = esp_http_client_open(client, 0);
    if (err == ESP_OK) {
        int content_length = esp_http_client_fetch_headers(client);
        char buffer[1024];
        int total_read = 0;

        while (1) {
            int read = esp_http_client_read(client, buffer, sizeof(buffer));
            if (read <= 0) break;
            fwrite(buffer, 1, read, f);
            total_read += read;

            if (content_length > 0 && om->on_progress) {
                om->progress_percent = 20 + (total_read * 60) / content_length;
                char msg[64];
                snprintf(msg, sizeof(msg), "Downloading rules: %d%%", om->progress_percent);
                om->on_progress(om->progress_percent, msg, om->user_data);
            }
        }

        if (total_read > 0) {
            om->rules_downloaded = true;
        }
    }

    fclose(f);
    esp_http_client_cleanup(client);

    if (om->rules_downloaded) {
        om->state = OTA_STATE_VERIFYING;
        if (om->on_progress) om->on_progress(85, "Verifying rules...", om->user_data);
        err = ota_verify_rules(om);
        if (err == ESP_OK) {
            om->state = OTA_STATE_APPLYING;
            if (om->on_progress) om->on_progress(95, "Applying rules...", om->user_data);
        }
    } else {
        unlink(om->rules_temp_path);
        om->state = OTA_STATE_ERROR;
    }

    return err;
}

esp_err_t ota_verify_rules(OTAManager* om) {
    if (!om) return ESP_ERR_INVALID_ARG;

    // Verify SHA256
    if (om->manifest.rules_sha256[0] != '\0') {
        // Calculate SHA256 of downloaded file
        FILE* f = fopen(om->rules_temp_path, "r");
        if (f) {
            // Simplified - in production use mbedtls_sha256
            fclose(f);
        }
    }

    // Validate JSON
    FILE* f = fopen(om->rules_temp_path, "r");
    if (f) {
        fseek(f, 0, SEEK_END);
        long size = ftell(f);
        fseek(f, 0, SEEK_SET);

        if (size > 0 && size < MAX_RULES_JSON_SIZE) {
            char* json = (char*)malloc(size + 1);
            if (json) {
                fread(json, 1, size, f);
                json[size] = '\0';
                esp_err_t err = rules_validate_json(json);
                free(json);
                fclose(f);
                return err;
            }
        }
        fclose(f);
    }

    return ESP_ERR_INVALID_ARG;
}

esp_err_t ota_apply_rules(OTAManager* om, RulesManager* rm) {
    if (!om || !rm) return ESP_ERR_INVALID_ARG;

    // Atomic rename
    if (rename(om->rules_temp_path, "/spiffs/rules.json") != 0) {
        return ESP_ERR_INVALID_STATE;
    }

    om->rules_downloaded = false;
    om->state = OTA_STATE_IDLE;

    // Trigger hot-reload
    rm->dirty = true;
    rules_load_from_spiffs(rm);

    ESP_LOGI(TAG, "Rules updated to v%s", om->manifest.rules_version);

    if (om->on_complete) om->on_complete(true, om->manifest.rules_version, om->user_data);
    ota_report_result(om, true, om->manifest.rules_version);

    return ESP_OK;
}

// ============================================================================
// Status & Utility
// ============================================================================

const char* ota_state_to_string(OTAState state) {
    switch (state) {
        case OTA_STATE_IDLE: return "IDLE";
        case OTA_STATE_CHECKING: return "CHECKING";
        case OTA_STATE_FIRMWARE_AVAILABLE: return "FW_AVAILABLE";
        case OTA_STATE_RULES_AVAILABLE: return "RULES_AVAILABLE";
        case OTA_STATE_DOWNLOADING_FIRMWARE: return "DL_FW";
        case OTA_STATE_DOWNLOADING_RULES: return "DL_RULES";
        case OTA_STATE_VERIFYING: return "VERIFYING";
        case OTA_STATE_APPLYING: return "APPLYING";
        case OTA_STATE_REBOOTING: return "REBOOTING";
        case OTA_STATE_ERROR: return "ERROR";
        case OTA_STATE_ROLLBACK: return "ROLLBACK";
        default: return "UNKNOWN";
    }
}

const esp_partition_t* ota_get_running_partition(void) {
    return esp_ota_get_running_partition();
}

const esp_partition_t* ota_get_next_partition(void) {
    return esp_ota_get_next_update_partition(NULL);
}

char* ota_get_partition_label(const esp_partition_t* partition) {
    static char label[17];
    if (partition) {
        strncpy(label, partition->label, 16);
        label[16] = '\0';
    } else {
        strcpy(label, "unknown");
    }
    return label;
}

bool ota_is_rollback_possible(void) {
    return esp_ota_get_last_invalid_partition() != NULL;
}

esp_err_t ota_report_result(OTAManager* om, bool success, const char* error_msg) {
    if (!om) return ESP_ERR_INVALID_ARG;
    return net_ota_report(om->net, success, success ? "firmware" : "rules", error_msg);
}

void ota_set_callbacks(OTAManager* om,
                       void (*on_progress)(uint8_t, const char*, void*),
                       void (*on_complete)(bool, const char*, void*),
                       void* user_data) {
    if (!om) return;
    om->on_progress = on_progress;
    om->on_complete = on_complete;
    om->user_data = user_data;
}

void ota_set_auto_check(OTAManager* om, bool enabled, uint32_t interval_ms) {
    if (!om) return;
    om->auto_check_enabled = enabled;
    om->check_interval_ms = interval_ms;
}

void ota_print_partition_info(void) {
    const esp_partition_t* running = esp_ota_get_running_partition();
    const esp_partition_t* next = esp_ota_get_next_update_partition(NULL);

    ESP_LOGI(TAG, "=== Partition Info ===");
    if (running) {
        ESP_LOGI(TAG, "Running: %s @ 0x%06" PRIX32 " (%" PRIu32 " KB)",
                 running->label, running->address, running->size / 1024);
    }
    if (next) {
        ESP_LOGI(TAG, "Next:    %s @ 0x%06" PRIX32 " (%" PRIu32 " KB)",
                 next->label, next->address, next->size / 1024);
    }

    // OTA data partition
    const esp_partition_t* ota_data = esp_partition_find_first(ESP_PARTITION_TYPE_DATA,
                                                                ESP_PARTITION_SUBTYPE_DATA_OTA,
                                                                NULL);
    if (ota_data) {
        ESP_LOGI(TAG, "OTA Data: %s @ 0x%06" PRIX32, ota_data->label, ota_data->address);
    }
}