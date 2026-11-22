/**
 * ============================================================================
 * PetDuet ESP32 CYD - Network Manager Implementation
 * ============================================================================
 * WiFi, HTTP/HTTPS, WebSocket client for PHP server communication
 * ============================================================================
 */
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "esp_log.h"
#include "esp_err.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include "esp_http_client.h"
#include "esp_tls.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "cJSON.h"
#include "NetworkManager.h"
#include "TamagotchiCoreEngine.h"

static const char* TAG = "NetworkMgr";

// ============================================================================
// WiFi Event Handler
// ============================================================================
static EventGroupHandle_t s_wifi_event_group;
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                               int32_t event_id, void* event_data) {
    NetworkManager* net = (NetworkManager*)arg;
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        net->state = NET_STATE_DISCONNECTED;
        esp_wifi_connect();
        xEventGroupClearBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
        xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*)event_data;
        ESP_LOGI(TAG, "Got IP: " IPSTR, IP2STR(&event->ip_info.ip));
        net->state = NET_STATE_CONNECTED;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
        xEventGroupClearBits(s_wifi_event_group, WIFI_FAIL_BIT);
    }
}

// ============================================================================
// HTTP Event Handler
// ============================================================================
static esp_err_t http_event_handler(esp_http_client_event_t* evt) {
    switch (evt->event_id) {
        case HTTP_EVENT_ERROR:
            ESP_LOGD(TAG, "HTTP_EVENT_ERROR");
            break;
        case HTTP_EVENT_ON_CONNECTED:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_CONNECTED");
            break;
        case HTTP_EVENT_HEADER_SENT:
            ESP_LOGD(TAG, "HTTP_EVENT_HEADER_SENT");
            break;
        case HTTP_EVENT_ON_HEADER:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
            break;
        case HTTP_EVENT_ON_DATA:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
            break;
        case HTTP_EVENT_ON_FINISH:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_FINISH");
            break;
        case HTTP_EVENT_DISCONNECTED:
            ESP_LOGD(TAG, "HTTP_EVENT_DISCONNECTED");
            break;
        case HTTP_EVENT_REDIRECT:
            ESP_LOGD(TAG, "HTTP_EVENT_REDIRECT");
            break;
        default:
            break;
    }
    return ESP_OK;
}

// ============================================================================
// Network Manager Implementation
// ============================================================================

esp_err_t net_init(NetworkManager* net, const char* ssid, const char* password,
                   const char* server_url, const char* device_id) {
    if (!net) return ESP_ERR_INVALID_ARG;

    memset(net, 0, sizeof(NetworkManager));
    net->state = NET_STATE_DISCONNECTED;

    strncpy(net->wifi_ssid, ssid, MAX_STRING_LEN-1);
    strncpy(net->wifi_password, password, MAX_STRING_LEN-1);
    strncpy(net->server_url, server_url, MAX_STRING_LEN-1);
    strncpy(net->device_id, device_id, MAX_STRING_LEN-1);

    // Initialize WiFi
    s_wifi_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_event_handler,
                                                        net,
                                                        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &wifi_event_handler,
                                                        net,
                                                        &instance_got_ip));

    wifi_config_t wifi_config;
    memset(&wifi_config, 0, sizeof(wifi_config));
    strncpy((char*)wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid)-1);
    strncpy((char*)wifi_config.sta.password, password, sizeof(wifi_config.sta.password)-1);
    wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
    wifi_config.sta.pmf_cfg.capable = true;
    wifi_config.sta.pmf_cfg.required = false;

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    // Initialize HTTP client
    esp_http_client_config_t http_cfg;
    memset(&http_cfg, 0, sizeof(http_cfg));
    http_cfg.url = server_url;
    http_cfg.event_handler = http_event_handler;
    http_cfg.timeout_ms = 10000;
    http_cfg.buffer_size = 4096;
    http_cfg.buffer_size_tx = 2048;
    net->http_client = esp_http_client_init(&http_cfg);

    ESP_LOGI(TAG, "Network manager initialized");
    return ESP_OK;
}

void net_deinit(NetworkManager* net) {
    if (!net) return;

    net_wifi_disconnect(net);
    net_ws_disconnect(net);

    if (net->http_client) {
        esp_http_client_cleanup(net->http_client);
    }

    if (s_wifi_event_group) {
        vEventGroupDelete(s_wifi_event_group);
    }

    esp_wifi_stop();
    esp_wifi_deinit();
}

esp_err_t net_wifi_connect(NetworkManager* net) {
    if (!net) return ESP_ERR_INVALID_ARG;
    if (strlen(net->wifi_ssid) == 0 ||
        strcmp(net->wifi_ssid, "YOUR_WIFI_SSID") == 0) {
        net->state = NET_STATE_DISCONNECTED;
        return ESP_ERR_INVALID_STATE;
    }

    net->state = NET_STATE_CONNECTING;
    esp_err_t ret = esp_wifi_connect();

    if (ret == ESP_OK) {
        EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
                                               WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
                                               pdFALSE, pdFALSE, pdMS_TO_TICKS(15000));
        if (bits & WIFI_CONNECTED_BIT) {
            return ESP_OK;
        }
    }
    return ESP_FAIL;
}

esp_err_t net_wifi_disconnect(NetworkManager* net) {
    if (!net) return ESP_ERR_INVALID_ARG;
    net->state = NET_STATE_DISCONNECTED;
    return esp_wifi_disconnect();
}

bool net_wifi_is_connected(NetworkManager* net) {
    return net && net->state == NET_STATE_CONNECTED;
}

// ============================================================================
// HTTP Requests
// ============================================================================

static esp_err_t net_http_request(NetworkManager* net, const char* endpoint,
                                   esp_http_client_method_t method,
                                   const char* json_body,
                                   char* response, size_t response_size) {
    if (!net || !net->http_client || !endpoint) return ESP_ERR_INVALID_ARG;

    char url[512];
    snprintf(url, sizeof(url), "%s%s", net->server_url, endpoint);

    esp_http_client_set_url(net->http_client, url);
    esp_http_client_set_method(net->http_client, method);

    if (net->auth_token[0]) {
        char auth_header[384];
        snprintf(auth_header, sizeof(auth_header), "Bearer %s", net->auth_token);
        esp_http_client_set_header(net->http_client, "Authorization", auth_header);
    }
    esp_http_client_set_header(net->http_client, "Content-Type", "application/json");
    esp_http_client_set_header(net->http_client, "X-Device-ID", net->device_id);

    if (json_body) {
        esp_http_client_set_post_field(net->http_client, json_body, strlen(json_body));
    }

    esp_err_t err = esp_http_client_perform(net->http_client);
    if (err == ESP_OK) {
        int status = esp_http_client_get_status_code(net->http_client);
        if (status == 200) {
            int len = esp_http_client_read_response(net->http_client, response, response_size - 1);
            if (len > 0) {
                response[len] = '\0';
            }
            return ESP_OK;
        } else {
            snprintf(net->last_error, sizeof(net->last_error), "HTTP %d", status);
            net->consecutive_failures++;
        }
    } else {
        snprintf(net->last_error, sizeof(net->last_error), "%s", esp_err_to_name(err));
        net->consecutive_failures++;
    }
    return err;
}

esp_err_t net_http_get(NetworkManager* net, const char* endpoint,
                       char* response, size_t response_size) {
    return net_http_request(net, endpoint, HTTP_METHOD_GET, NULL, response, response_size);
}

esp_err_t net_http_post(NetworkManager* net, const char* endpoint,
                        const char* json_body, char* response, size_t response_size) {
    return net_http_request(net, endpoint, HTTP_METHOD_POST, json_body, response, response_size);
}

esp_err_t net_http_post_file(NetworkManager* net, const char* endpoint,
                             const char* file_path, char* response, size_t response_size) {
    // Not implemented for now
    return ESP_ERR_NOT_SUPPORTED;
}

// ============================================================================
// Authentication
// ============================================================================

esp_err_t net_register_device(NetworkManager* net, const char* trainer_name,
                              const char* biome_id) {
    if (!net) return ESP_ERR_INVALID_ARG;

    cJSON* json = cJSON_CreateObject();
    cJSON_AddStringToObject(json, "device_id", net->device_id);
    cJSON_AddStringToObject(json, "trainer_name", trainer_name);
    cJSON_AddStringToObject(json, "biome", biome_id);
    cJSON_AddStringToObject(json, "firmware_version", FIRMWARE_VERSION);
    cJSON_AddStringToObject(json, "rules_version", RULES_VERSION);

    char* json_str = cJSON_Print(json);
    cJSON_Delete(json);

    char response[1024];
    esp_err_t err = net_http_post(net, REGISTER_ENDPOINT, json_str, response, sizeof(response));
    free(json_str);

    if (err == ESP_OK) {
        cJSON* resp = cJSON_Parse(response);
        if (resp) {
            cJSON* token = cJSON_GetObjectItem(resp, "token");
            if (token) {
                strncpy(net->auth_token, token->valuestring, sizeof(net->auth_token)-1);
            }
            cJSON_Delete(resp);
        }
    }
    return err;
}

esp_err_t net_login(NetworkManager* net) {
    if (!net) return ESP_ERR_INVALID_ARG;

    cJSON* json = cJSON_CreateObject();
    cJSON_AddStringToObject(json, "device_id", net->device_id);

    char* json_str = cJSON_Print(json);
    cJSON_Delete(json);

    char response[1024];
    esp_err_t err = net_http_post(net, LOGIN_ENDPOINT, json_str, response, sizeof(response));
    free(json_str);

    if (err == ESP_OK) {
        cJSON* resp = cJSON_Parse(response);
        if (resp) {
            cJSON* token = cJSON_GetObjectItem(resp, "token");
            if (token) {
                strncpy(net->auth_token, token->valuestring, sizeof(net->auth_token)-1);
            }
            cJSON_Delete(resp);
        }
    }
    return err;
}

// ============================================================================
// Pet Sync
// ============================================================================

esp_err_t net_get_pet(NetworkManager* net, PetState* pet) {
    if (!net || !pet) return ESP_ERR_INVALID_ARG;

    char response[4096];
    esp_err_t err = net_http_get(net, PET_GET_ENDPOINT, response, sizeof(response));
    if (err == ESP_OK) {
        engine_parse_pet_json(response, pet);
    }
    return err;
}

esp_err_t net_update_pet(NetworkManager* net, const PetState* pet) {
    if (!net || !pet) return ESP_ERR_INVALID_ARG;

    char* json_str = engine_generate_pet_json(pet);
    if (!json_str) return ESP_ERR_NO_MEM;

    char response[1024];
    esp_err_t err = net_http_post(net, PET_UPDATE_ENDPOINT, json_str, response, sizeof(response));
    free(json_str);
    return err;
}

esp_err_t net_full_sync(NetworkManager* net, PetState* pet, PetState* partner_pet) {
    if (!net) return ESP_ERR_INVALID_ARG;

    // Push local pet state
    if (pet) {
        net_update_pet(net, pet);
    }

    // Pull from server (includes partner state)
    char response[8192];
    esp_err_t err = net_http_post(net, SYNC_ENDPOINT, "{}", response, sizeof(response));
    if (err == ESP_OK) {
        cJSON* resp = cJSON_Parse(response);
        if (resp) {
            cJSON* pet_json = cJSON_GetObjectItem(resp, "pet");
            if (pet_json && pet) {
                char* pet_str = cJSON_Print(pet_json);
                engine_parse_pet_json(pet_str, pet);
                free(pet_str);
            }
            cJSON* partner_json = cJSON_GetObjectItem(resp, "partner_pet");
            if (partner_json && partner_pet) {
                char* partner_str = cJSON_Print(partner_json);
                engine_parse_pet_json(partner_str, partner_pet);
                free(partner_str);
            }
            cJSON_Delete(resp);
        }
    }
    return err;
}

// ============================================================================
// WebSocket (Simplified - using HTTP polling for now)
// ============================================================================

esp_err_t net_ws_connect(NetworkManager* net) {
    // In a full implementation, this would establish a WebSocket connection
    // For now, we use HTTP polling via net_ws_poll
    net->ws_connected = true;
    net->state = NET_STATE_WS_CONNECTED;
    return ESP_OK;
}

esp_err_t net_ws_disconnect(NetworkManager* net) {
    net->ws_connected = false;
    return ESP_OK;
}

esp_err_t net_ws_send(NetworkManager* net, const WSMessage* msg) {
    // Would send via WebSocket
    return ESP_OK;
}

void net_ws_set_callback(NetworkManager* net,
                         void (*callback)(const WSMessage*, void*), void* user_data) {
    net->on_ws_message = callback;
    net->ws_user_data = user_data;
}

void net_ws_poll(NetworkManager* net) {
    // HTTP long-polling fallback
    if (!net->ws_connected) return;

    // Check for pending messages via HTTP
    // This is a simplified implementation
}

// ============================================================================
// OTA
// ============================================================================

esp_err_t net_ota_check(NetworkManager* net, char* firmware_version,
                        char* rules_version, char* firmware_url,
                        char* rules_url, char* firmware_sha256, char* rules_sha256) {
    if (!net) return ESP_ERR_INVALID_ARG;

    char response[2048];
    esp_err_t err = net_http_get(net, OTA_CHECK_ENDPOINT, response, sizeof(response));
    if (err == ESP_OK) {
        cJSON* resp = cJSON_Parse(response);
        if (resp) {
            cJSON* fw_ver = cJSON_GetObjectItem(resp, "firmware_version");
            cJSON* rl_ver = cJSON_GetObjectItem(resp, "rules_version");
            cJSON* fw_url = cJSON_GetObjectItem(resp, "firmware_url");
            cJSON* rl_url = cJSON_GetObjectItem(resp, "rules_url");
            cJSON* fw_sha = cJSON_GetObjectItem(resp, "firmware_sha256");
            cJSON* rl_sha = cJSON_GetObjectItem(resp, "rules_sha256");

            if (fw_ver) strncpy(firmware_version, fw_ver->valuestring, MAX_STRING_LEN-1);
            if (rl_ver) strncpy(rules_version, rl_ver->valuestring, MAX_STRING_LEN-1);
            if (fw_url) strncpy(firmware_url, fw_url->valuestring, 255);
            if (rl_url) strncpy(rules_url, rl_url->valuestring, 255);
            if (fw_sha) strncpy(firmware_sha256, fw_sha->valuestring, 64);
            if (rl_sha) strncpy(rules_sha256, rl_sha->valuestring, 64);

            cJSON_Delete(resp);
        }
    }
    return err;
}

esp_err_t net_ota_download_firmware(NetworkManager* net, const char* url,
                                    const char* expected_sha256) {
    // Handled by esp_https_ota directly
    return ESP_OK;
}

esp_err_t net_ota_download_rules(NetworkManager* net, const char* url,
                                 const char* expected_sha256) {
    // Download rules.json to SPIFFS
    return ESP_OK;
}

esp_err_t net_ota_report(NetworkManager* net, bool success, const char* version,
                         const char* error_msg) {
    if (!net) return ESP_ERR_INVALID_ARG;

    cJSON* json = cJSON_CreateObject();
    cJSON_AddBoolToObject(json, "success", success);
    cJSON_AddStringToObject(json, "version", version);
    if (error_msg) cJSON_AddStringToObject(json, "error", error_msg);

    char* json_str = cJSON_Print(json);
    cJSON_Delete(json);

    char response[512];
    esp_err_t err = net_http_post(net, "/api/ota/report", json_str, response, sizeof(response));
    free(json_str);
    return err;
}

// ============================================================================
// Utility
// ============================================================================

const char* net_state_to_string(NetworkState state) {
    switch (state) {
        case NET_STATE_DISCONNECTED: return "DISCONNECTED";
        case NET_STATE_CONNECTING: return "CONNECTING";
        case NET_STATE_CONNECTED: return "CONNECTED";
        case NET_STATE_WS_CONNECTING: return "WS_CONNECTING";
        case NET_STATE_WS_CONNECTED: return "WS_CONNECTED";
        case NET_STATE_ERROR: return "ERROR";
        default: return "UNKNOWN";
    }
}

uint32_t net_get_uptime_ms(void) {
    return esp_timer_get_time() / 1000;
}