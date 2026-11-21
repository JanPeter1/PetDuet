/**
 * ============================================================================
 * PetDuet ESP32 CYD - Network Manager
 * ============================================================================
 * WiFi, HTTP/HTTPS, WebSocket client for PHP server communication
 * ============================================================================
 */
#ifndef NETWORK_MANAGER_H
#define NETWORK_MANAGER_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_http_client.h"
#include "config.h"
#include "TamagotchiCoreEngine.h"

// ============================================================================
// Network State
// ============================================================================
typedef enum {
    NET_STATE_DISCONNECTED = 0,
    NET_STATE_CONNECTING,
    NET_STATE_CONNECTED,
    NET_STATE_WS_CONNECTING,
    NET_STATE_WS_CONNECTED,
    NET_STATE_ERROR
} NetworkState;

typedef enum {
    WS_MSG_TYPE_NONE = 0,
    WS_MSG_TYPE_TREAT_SEND,
    WS_MSG_TYPE_TREAT_RECEIVE,
    WS_MSG_TYPE_MAGIC_CAST,
    WS_MSG_TYPE_TRADE_PROPOSE,
    WS_MSG_TYPE_TRADE_ACCEPT,
    WS_MSG_TYPE_TRADE_REJECT,
    WS_MSG_TYPE_SYNC_REQUEST,
    WS_MSG_TYPE_SYNC_RESPONSE,
    WS_MSG_TYPE_PING,
    WS_MSG_TYPE_PONG,
    WS_MSG_TYPE_SERVER_COMMAND
} WSMessageType;

// ============================================================================
// WebSocket Message
// ============================================================================
typedef struct {
    WSMessageType type;
    char from_device[MAX_STRING_LEN];
    char to_device[MAX_STRING_LEN];
    char payload[1024];
    uint32_t timestamp;
} WSMessage;

// ============================================================================
// Network Manager Context
// ============================================================================
typedef struct {
    NetworkState state;
    char wifi_ssid[MAX_STRING_LEN];
    char wifi_password[MAX_STRING_LEN];
    char server_url[MAX_STRING_LEN];
    char device_id[MAX_STRING_LEN];
    char auth_token[256];

    // HTTP Client
    esp_http_client_handle_t http_client;

    // WebSocket
    // Note: ESP-IDF doesn't have built-in WS client, we'll use a simple implementation
    // or connect via raw socket
    int ws_socket;
    bool ws_connected;
    uint32_t ws_last_ping;

    // Callbacks
    void (*on_ws_message)(const WSMessage* msg, void* user_data);
    void* ws_user_data;

    // Sync
    uint32_t last_sync_time;
    bool sync_pending;

    // Error tracking
    int consecutive_failures;
    char last_error[128];
} NetworkManager;

// ============================================================================
// API
// ============================================================================

// Initialization
esp_err_t net_init(NetworkManager* net, const char* ssid, const char* password,
                   const char* server_url, const char* device_id);
void net_deinit(NetworkManager* net);

// WiFi
esp_err_t net_wifi_connect(NetworkManager* net);
esp_err_t net_wifi_disconnect(NetworkManager* net);
bool net_wifi_is_connected(NetworkManager* net);

// HTTP Requests
esp_err_t net_http_get(NetworkManager* net, const char* endpoint,
                       char* response, size_t response_size);
esp_err_t net_http_post(NetworkManager* net, const char* endpoint,
                        const char* json_body, char* response, size_t response_size);
esp_err_t net_http_post_file(NetworkManager* net, const char* endpoint,
                             const char* file_path, char* response, size_t response_size);

// Authentication
esp_err_t net_register_device(NetworkManager* net, const char* trainer_name,
                              const char* biome_id);
esp_err_t net_login(NetworkManager* net);

// Pet Sync
esp_err_t net_get_pet(NetworkManager* net, PetState* pet);
esp_err_t net_update_pet(NetworkManager* net, const PetState* pet);
esp_err_t net_full_sync(NetworkManager* net, PetState* pet, PetState* partner_pet);

// WebSocket
esp_err_t net_ws_connect(NetworkManager* net);
esp_err_t net_ws_disconnect(NetworkManager* net);
esp_err_t net_ws_send(NetworkManager* net, const WSMessage* msg);
void net_ws_set_callback(NetworkManager* net,
                         void (*callback)(const WSMessage*, void*), void* user_data);
void net_ws_poll(NetworkManager* net);

// OTA
esp_err_t net_ota_check(NetworkManager* net, char* firmware_version,
                        char* rules_version, char* firmware_url,
                        char* rules_url, char* firmware_sha256, char* rules_sha256);
esp_err_t net_ota_download_firmware(NetworkManager* net, const char* url,
                                    const char* expected_sha256);
esp_err_t net_ota_download_rules(NetworkManager* net, const char* url,
                                 const char* expected_sha256);
esp_err_t net_ota_report(NetworkManager* net, bool success, const char* version,
                         const char* error_msg);

// Utility
const char* net_state_to_string(NetworkState state);
uint32_t net_get_uptime_ms(void);

#endif // NETWORK_MANAGER_H