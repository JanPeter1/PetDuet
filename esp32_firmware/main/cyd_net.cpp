#include "cyd_net.h"
#include "cyd_storage.h"
#include "cyd_hw.h"
#include "../config.h"
#include "../TamagotchiCoreEngine.h"

#include "cJSON.h"
#include "esp_event.h"
#include "esp_http_client.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_ota_ops.h"
#include "esp_system.h"
#include "esp_wifi.h"
#ifdef CONFIG_MBEDTLS_CERTIFICATE_BUNDLE
#include "esp_crt_bundle.h"
#endif

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"

#include "lwip/inet.h"
#include "lwip/sockets.h"

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char* TAG = "CYD_NET";

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

static EventGroupHandle_t s_wifi_event_group = NULL;
static esp_netif_t* s_sta_netif = NULL;
static esp_netif_t* s_ap_netif = NULL;
static httpd_handle_t s_httpd = NULL;
static bool s_wifi_connected = false;
static int s_retry_num = 0;
static int s_listen_sock = -1;
static CydConfig* s_portal_cfg = NULL;
static char s_scanned_options[2048] = "";

static void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
  if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
    esp_wifi_connect();
  } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
    s_wifi_connected = false;
    if (s_retry_num < 6 && s_wifi_event_group) {
      esp_wifi_connect();
      s_retry_num++;
    } else if (s_wifi_event_group) {
      xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
    }
  } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
    s_retry_num = 0;
    s_wifi_connected = true;
    if (s_wifi_event_group) {
      xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
  }
}

void cyd_net_init_stack(void) {
  static bool initialized = false;
  if (initialized) return;
  initialized = true;

  s_wifi_event_group = xEventGroupCreate();
  ESP_ERROR_CHECK(esp_netif_init());
  ESP_ERROR_CHECK(esp_event_loop_create_default());
  s_sta_netif = esp_netif_create_default_wifi_sta();
  s_ap_netif  = esp_netif_create_default_wifi_ap();

  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&cfg));
  ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, NULL));
  ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL, NULL));
}

void cyd_net_mark_ota_valid(void) {
  const esp_partition_t* running = esp_ota_get_running_partition();
  esp_ota_img_states_t ota_state;
  if (running && esp_ota_get_state_partition(running, &ota_state) == ESP_OK) {
    if (ota_state == ESP_OTA_IMG_PENDING_VERIFY) {
      ESP_LOGI(TAG, "Marking new A/B OTA partition '%s' as VALID", running->label);
      esp_ota_mark_app_valid_cancel_rollback();
    }
  }
}

bool cyd_net_is_wifi_connected(void) {
  return s_wifi_connected;
}

bool cyd_net_connect_sta(const CydConfig& cfg, char* outIpStr, size_t ipLen) {
  if (outIpStr && ipLen > 0) outIpStr[0] = '\0';
  if (strlen(cfg.wifiSsid) == 0 || strcmp(cfg.wifiSsid, "YOUR_WIFI_SSID") == 0) {
    ESP_LOGW(TAG, "WiFi SSID unconfigured; skipping STA attempt");
    return false;
  }

  cyd_net_init_stack();
  s_wifi_connected = false;
  s_retry_num = 0;
  xEventGroupClearBits(s_wifi_event_group, WIFI_CONNECTED_BIT | WIFI_FAIL_BIT);

  wifi_config_t wifi_cfg;
  memset(&wifi_cfg, 0, sizeof(wifi_cfg));
  strncpy((char*)wifi_cfg.sta.ssid, cfg.wifiSsid, sizeof(wifi_cfg.sta.ssid) - 1);
  strncpy((char*)wifi_cfg.sta.password, cfg.wifiPass, sizeof(wifi_cfg.sta.password) - 1);
  wifi_cfg.sta.threshold.authmode = (strlen(cfg.wifiPass) > 0) ? WIFI_AUTH_WPA_WPA2_PSK : WIFI_AUTH_OPEN;

  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
  ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_cfg));
  ESP_ERROR_CHECK(esp_wifi_start());

  EventBits_t bits = xEventGroupWaitBits(
    s_wifi_event_group,
    WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
    pdFALSE,
    pdFALSE,
    pdMS_TO_TICKS(10000)
  );

  if (bits & WIFI_CONNECTED_BIT) {
    esp_netif_ip_info_t ip_info;
    if (s_sta_netif && esp_netif_get_ip_info(s_sta_netif, &ip_info) == ESP_OK && outIpStr) {
      snprintf(outIpStr, ipLen, IPSTR, IP2STR(&ip_info.ip));
    }
    ESP_LOGI(TAG, "WiFi STA connected, IP=%s", outIpStr ? outIpStr : "");
    return true;
  }
  ESP_LOGW(TAG, "WiFi STA connection failed for SSID=%s", cfg.wifiSsid);
  return false;
}

void cyd_net_stop_all(void) {
  cyd_net_stop_ap_portal();
  if (s_listen_sock >= 0) {
    close(s_listen_sock);
    s_listen_sock = -1;
  }
  s_wifi_connected = false;
  esp_wifi_disconnect();
  esp_wifi_stop();
}

// ============================================================================
// SOFT-AP CONFIG WEB PORTAL (192.168.4.1)
// ============================================================================
static void url_decode(const char* src, char* dst, size_t maxLen) {
  size_t i = 0;
  while (src && *src && i + 1 < maxLen) {
    if (*src == '+') {
      dst[i++] = ' ';
      src++;
    } else if (*src == '%' && isxdigit((unsigned char)src[1]) && isxdigit((unsigned char)src[2])) {
      char hex[3] = {src[1], src[2], '\0'};
      dst[i++] = (char)strtol(hex, NULL, 16);
      src += 3;
    } else {
      dst[i++] = *src++;
    }
  }
  dst[i] = '\0';
}

static bool extract_form_field(const char* body, const char* key, char* out, size_t outLen) {
  if (!body || !key || !out || outLen == 0) return false;
  size_t keyLen = strlen(key);
  const char* p = body;
  while (p && *p) {
    if (strncmp(p, key, keyLen) == 0 && p[keyLen] == '=') {
      const char* valStart = p + keyLen + 1;
      const char* valEnd = strchr(valStart, '&');
      size_t rawLen = valEnd ? (size_t)(valEnd - valStart) : strlen(valStart);
      char encoded[256];
      size_t copyLen = (rawLen < sizeof(encoded) - 1) ? rawLen : (sizeof(encoded) - 1);
      memcpy(encoded, valStart, copyLen);
      encoded[copyLen] = '\0';
      url_decode(encoded, out, outLen);
      return true;
    }
    p = strchr(p, '&');
    if (p) p++;
  }
  return false;
}

static esp_err_t ap_root_get_handler(httpd_req_t* req) {
  const CydConfig* c = s_portal_cfg;
  char* html = (char*)malloc(6144);
  if (!html) return httpd_resp_send_500(req);

  const char* selGrass = (c && strcmp(c->biome, "grassland") == 0) ? " selected" : "";
  const char* selDesert = (c && strcmp(c->biome, "desert") == 0) ? " selected" : "";
  const char* selWater = (c && strcmp(c->biome, "water") == 0) ? " selected" : "";
  const char* selForest = (c && strcmp(c->biome, "forest") == 0) ? " selected" : "";

  snprintf(html, 6144,
    "<!DOCTYPE html><html><head><meta charset='UTF-8'>"
    "<meta name='viewport' content='width=device-width,initial-scale=1'>"
    "<title>CYD Tamagotchi WiFi Setup</title>"
    "<style>"
    "body{font-family:sans-serif;background:#0f172a;color:#f8fafc;padding:1.5rem;margin:0;}"
    ".card{max-width:440px;margin:0 auto;background:#1e293b;border:1px solid #334155;border-radius:12px;padding:1.5rem;}"
    "h2{color:#f59e0b;margin-top:0;}label{font-size:.8rem;font-weight:600;color:#94a3b8;display:block;margin-top:.8rem;}"
    "input,select{width:100%%;box-sizing:border-box;padding:10px;border-radius:8px;border:1px solid #475569;background:#0f172a;color:#f8fafc;margin-top:4px;}"
    ".btn{width:100%%;margin-top:1.4rem;padding:12px;background:#f59e0b;color:#0f172a;border:none;border-radius:8px;font-weight:700;cursor:pointer;}"
    "</style></head><body><div class='card'><h2>CYD WiFi &amp; RPG Config (ESP-IDF)</h2>"
    "<form action='/save' method='POST'>"
    "<label>1. WiFi Network (2.4GHz):</label>"
    "<select name='ssid' onchange=\"document.getElementById('ssid_txt').value=this.value;\">"
    "<option value=''>-- Choose Scanned Network --</option>%s</select>"
    "<input type='text' name='ssid_custom' id='ssid_txt' value='%s' placeholder='SSID...'>"
    "<label>2. WiFi Password:</label><input type='password' name='password' value='%s'>"
    "<label>3. Device ID:</label><input type='text' name='device_id' value='%s'>"
    "<label>4. Trainer Name:</label><input type='text' name='trainer_name' value='%s'>"
    "<label>5. Friend Device ID:</label><input type='text' name='friend_id' value='%s'>"
    "<label>6. Biome:</label><select name='biome'>"
    "<option value='grassland'%s>Grassland</option>"
    "<option value='desert'%s>Desert</option>"
    "<option value='water'%s>Water</option>"
    "<option value='forest'%s>Forest</option></select>"
    "<label>7. PHP Server URL:</label><input type='text' name='server_url' value='%s'>"
    "<button type='submit' class='btn'>Save &amp; Connect</button>"
    "</form></div></body></html>",
    s_scanned_options,
    c ? c->wifiSsid : "",
    c ? c->wifiPass : "",
    c ? c->deviceId : DEVICE_ID,
    c ? c->trainerName : TRAINER_NAME,
    c ? c->friendId : FRIEND_DEV_ID,
    selGrass, selDesert, selWater, selForest,
    c ? c->serverUrl : PHP_SERVER_BASE_URL
  );

  httpd_resp_set_type(req, "text/html");
  httpd_resp_send(req, html, HTTPD_RESP_USE_STRLEN);
  free(html);
  return ESP_OK;
}

static void delayed_restart_task(void* arg) {
  vTaskDelay(pdMS_TO_TICKS(1000));
  esp_restart();
}

static esp_err_t ap_save_post_handler(httpd_req_t* req) {
  int total = req->content_len;
  if (total <= 0 || total > 2048) {
    httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid form size");
    return ESP_FAIL;
  }
  char* buf = (char*)malloc((size_t)total + 1);
  if (!buf) return httpd_resp_send_500(req);

  int received = 0;
  while (received < total) {
    int r = httpd_req_recv(req, buf + received, total - received);
    if (r <= 0) {
      if (r == HTTPD_SOCK_ERR_TIMEOUT) continue;
      free(buf);
      return ESP_FAIL;
    }
    received += r;
  }
  buf[received] = '\0';

  if (s_portal_cfg) {
    char tmp[160];
    if (extract_form_field(buf, "ssid_custom", tmp, sizeof(tmp)) && strlen(tmp) > 0) {
      strncpy(s_portal_cfg->wifiSsid, tmp, sizeof(s_portal_cfg->wifiSsid) - 1);
    } else if (extract_form_field(buf, "ssid", tmp, sizeof(tmp)) && strlen(tmp) > 0) {
      strncpy(s_portal_cfg->wifiSsid, tmp, sizeof(s_portal_cfg->wifiSsid) - 1);
    }
    if (extract_form_field(buf, "password", tmp, sizeof(tmp)) && strlen(tmp) > 0) {
      strncpy(s_portal_cfg->wifiPass, tmp, sizeof(s_portal_cfg->wifiPass) - 1);
    }
    if (extract_form_field(buf, "device_id", tmp, sizeof(tmp)) && strlen(tmp) > 0) {
      strncpy(s_portal_cfg->deviceId, tmp, sizeof(s_portal_cfg->deviceId) - 1);
    }
    if (extract_form_field(buf, "trainer_name", tmp, sizeof(tmp)) && strlen(tmp) > 0) {
      strncpy(s_portal_cfg->trainerName, tmp, sizeof(s_portal_cfg->trainerName) - 1);
    }
    if (extract_form_field(buf, "friend_id", tmp, sizeof(tmp)) && strlen(tmp) > 0) {
      strncpy(s_portal_cfg->friendId, tmp, sizeof(s_portal_cfg->friendId) - 1);
    }
    if (extract_form_field(buf, "biome", tmp, sizeof(tmp)) && strlen(tmp) > 0) {
      strncpy(s_portal_cfg->biome, tmp, sizeof(s_portal_cfg->biome) - 1);
    }
    if (extract_form_field(buf, "server_url", tmp, sizeof(tmp)) && strlen(tmp) > 0) {
      strncpy(s_portal_cfg->serverUrl, tmp, sizeof(s_portal_cfg->serverUrl) - 1);
    }
    cyd_storage_save_config(*s_portal_cfg);
  }
  free(buf);

  const char* resp =
    "<!DOCTYPE html><html><body style='background:#0f172a;color:#10b981;font-family:sans-serif;text-align:center;padding:2rem;'>"
    "<h2>Settings Saved! Rebooting CYD...</h2></body></html>";
  httpd_resp_set_type(req, "text/html");
  httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);

  xTaskCreate(delayed_restart_task, "reboot_task", 2048, NULL, 5, NULL);
  return ESP_OK;
}

void cyd_net_start_ap_portal(CydConfig& cfg) {
  cyd_net_init_stack();
  s_portal_cfg = &cfg;
  s_scanned_options[0] = '\0';

  // Quick 2.4GHz AP scan in STA mode before launching SoftAP
  esp_wifi_stop();
  esp_wifi_set_mode(WIFI_MODE_STA);
  esp_wifi_start();
  wifi_scan_config_t scan_cfg = {};
  if (esp_wifi_scan_start(&scan_cfg, true) == ESP_OK) {
    uint16_t apCount = 0;
    esp_wifi_scan_get_ap_num(&apCount);
    if (apCount > 12) apCount = 12;
    if (apCount > 0) {
      wifi_ap_record_t* list = (wifi_ap_record_t*)calloc(apCount, sizeof(wifi_ap_record_t));
      if (list) {
        if (esp_wifi_scan_get_ap_records(&apCount, list) == ESP_OK) {
          size_t off = 0;
          for (uint16_t i = 0; i < apCount && off + 96 < sizeof(s_scanned_options); i++) {
            int n = snprintf(s_scanned_options + off, sizeof(s_scanned_options) - off,
                             "<option value='%s'>%s (%d dBm)</option>",
                             (const char*)list[i].ssid, (const char*)list[i].ssid, list[i].rssi);
            if (n > 0) off += (size_t)n;
          }
        }
        free(list);
      }
    }
  }
  esp_wifi_stop();

  wifi_config_t ap_cfg = {};
  strcpy((char*)ap_cfg.ap.ssid, "CYD-Tamagotchi-AP");
  ap_cfg.ap.ssid_len = strlen("CYD-Tamagotchi-AP");
  ap_cfg.ap.channel = 1;
  ap_cfg.ap.max_connection = 4;
  ap_cfg.ap.authmode = WIFI_AUTH_OPEN;

  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
  ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &ap_cfg));
  ESP_ERROR_CHECK(esp_wifi_start());

  httpd_config_t hcfg = HTTPD_DEFAULT_CONFIG();
  hcfg.stack_size = 6144;
  if (httpd_start(&s_httpd, &hcfg) == ESP_OK) {
    httpd_uri_t root_uri = { .uri = "/", .method = HTTP_GET, .handler = ap_root_get_handler, .user_ctx = NULL };
    httpd_uri_t save_uri = { .uri = "/save", .method = HTTP_POST, .handler = ap_save_post_handler, .user_ctx = NULL };
    httpd_register_uri_handler(s_httpd, &root_uri);
    httpd_register_uri_handler(s_httpd, &save_uri);
    ESP_LOGI(TAG, "AP Web Portal started at http://192.168.4.1");
  }
}

void cyd_net_stop_ap_portal(void) {
  if (s_httpd) {
    httpd_stop(s_httpd);
    s_httpd = NULL;
  }
}

// ============================================================================
// HTTP CLIENT HELPERS (PHP SERVER & OTA)
// ============================================================================
struct HttpAccum {
  char* buf;
  size_t len;
  size_t cap;
};

static esp_err_t http_accum_handler(esp_http_client_event_t* evt) {
  HttpAccum* acc = (HttpAccum*)evt->user_data;
  if (evt->event_id == HTTP_EVENT_ON_DATA && acc && acc->buf && evt->data_len > 0) {
    size_t avail = (acc->cap - 1) - acc->len;
    size_t toCopy = ((size_t)evt->data_len < avail) ? (size_t)evt->data_len : avail;
    if (toCopy > 0) {
      memcpy(acc->buf + acc->len, evt->data, toCopy);
      acc->len += toCopy;
      acc->buf[acc->len] = '\0';
    }
  }
  return ESP_OK;
}

static int http_do_request(const char* url, esp_http_client_method_t method, const char* postBody, char* outResp, size_t outCap) {
  if (outResp && outCap > 0) outResp[0] = '\0';
  HttpAccum acc = { outResp, 0, outCap };

  esp_http_client_config_t config = {};
  config.url = url;
  config.method = method;
  config.timeout_ms = 6000;
  config.event_handler = outResp ? http_accum_handler : NULL;
  config.user_data = outResp ? &acc : NULL;
#ifdef CONFIG_MBEDTLS_CERTIFICATE_BUNDLE
  if (strncmp(url, "https://", 8) == 0) {
    config.crt_bundle_attach = esp_crt_bundle_attach;
  }
#endif

  esp_http_client_handle_t client = esp_http_client_init(&config);
  if (!client) return -1;

  if (postBody && method == HTTP_METHOD_POST) {
    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_post_field(client, postBody, (int)strlen(postBody));
  }

  esp_err_t err = esp_http_client_perform(client);
  int status = (err == ESP_OK) ? esp_http_client_get_status_code(client) : -1;
  esp_http_client_cleanup(client);
  return status;
}

bool cyd_net_test_php_server(const CydConfig& cfg) {
  if (!s_wifi_connected) return false;
  char url[220];
  snprintf(url, sizeof(url), "%s/register.php", cfg.serverUrl);

  cJSON* doc = cJSON_CreateObject();
  cJSON_AddStringToObject(doc, "deviceId", cfg.deviceId);
  cJSON_AddStringToObject(doc, "deviceName", cfg.trainerName);
  char* body = cJSON_PrintUnformatted(doc);
  cJSON_Delete(doc);

  int code = http_do_request(url, HTTP_METHOD_POST, body, NULL, 0);
  cJSON_free(body);
  ESP_LOGI(TAG, "PHP server connectivity test HTTP status=%d", code);
  return code == 200;
}

bool cyd_net_sync_php_server(const CydConfig& cfg, const TamagotchiCoreEngine& engine, FriendState& friendPet) {
  if (!s_wifi_connected) return false;

  esp_netif_ip_info_t ip_info = {};
  char localIp[20] = "0.0.0.0";
  if (s_sta_netif && esp_netif_get_ip_info(s_sta_netif, &ip_info) == ESP_OK) {
    snprintf(localIp, sizeof(localIp), IPSTR, IP2STR(&ip_info.ip));
  }

  cJSON* doc = cJSON_CreateObject();
  cJSON_AddStringToObject(doc, "deviceId", cfg.deviceId);
  cJSON_AddStringToObject(doc, "deviceName", cfg.trainerName);
  cJSON_AddStringToObject(doc, "petName", engine.name);
  cJSON_AddStringToObject(doc, "biome", engine.biome);
  cJSON_AddNumberToObject(doc, "level", engine.level);
  cJSON_AddStringToObject(doc, "phase", engine.phase);
  cJSON_AddStringToObject(doc, "ipAddress", localIp);
  cJSON_AddNumberToObject(doc, "port", SOCKET_PORT);

  cJSON* st = cJSON_CreateObject();
  cJSON_AddNumberToObject(st, "health", engine.stats.health);
  cJSON_AddNumberToObject(st, "hunger", engine.stats.hunger);
  cJSON_AddNumberToObject(st, "energy", engine.stats.energy);
  cJSON_AddNumberToObject(st, "happiness", engine.stats.happiness);
  cJSON_AddItemToObject(doc, "stats", st);

  char* body = cJSON_PrintUnformatted(doc);
  cJSON_Delete(doc);

  char url[220];
  snprintf(url, sizeof(url), "%s/register.php", cfg.serverUrl);
  int code = http_do_request(url, HTTP_METHOD_POST, body, NULL, 0);
  cJSON_free(body);
  if (code != 200) return false;

  snprintf(url, sizeof(url), "%s/find_friend.php?friendId=%s", cfg.serverUrl, cfg.friendId);
  char resp[768];
  if (http_do_request(url, HTTP_METHOD_GET, NULL, resp, sizeof(resp)) == 200) {
    cJSON* root = cJSON_Parse(resp);
    if (root) {
      cJSON* f = cJSON_GetObjectItemCaseSensitive(root, "friend");
      if (cJSON_IsObject(f)) {
        cJSON* jOn = cJSON_GetObjectItemCaseSensitive(f, "isOnline");
        cJSON* jName = cJSON_GetObjectItemCaseSensitive(f, "petName");
        cJSON* jBio = cJSON_GetObjectItemCaseSensitive(f, "biome");
        cJSON* jLv = cJSON_GetObjectItemCaseSensitive(f, "level");
        cJSON* jIp = cJSON_GetObjectItemCaseSensitive(f, "ipAddress");
        cJSON* jPh = cJSON_GetObjectItemCaseSensitive(f, "phase");
        cJSON* jSt = cJSON_GetObjectItemCaseSensitive(f, "stats");

        if (cJSON_IsBool(jOn)) friendPet.isOnline = cJSON_IsTrue(jOn);
        if (cJSON_IsString(jName)) {
          strncpy(friendPet.name, jName->valuestring, sizeof(friendPet.name) - 1);
        }
        if (cJSON_IsString(jBio)) {
          strncpy(friendPet.biome, jBio->valuestring, sizeof(friendPet.biome) - 1);
        }
        if (cJSON_IsString(jPh)) {
          strncpy(friendPet.phase, jPh->valuestring, sizeof(friendPet.phase) - 1);
        }
        if (cJSON_IsNumber(jLv)) friendPet.level = (uint8_t)jLv->valueint;
        if (cJSON_IsString(jIp)) {
          strncpy(friendPet.ipAddress, jIp->valuestring, sizeof(friendPet.ipAddress) - 1);
        }
        if (cJSON_IsObject(jSt)) {
          cJSON* hp = cJSON_GetObjectItemCaseSensitive(jSt, "health");
          if (cJSON_IsNumber(hp)) friendPet.health = (uint8_t)hp->valueint;
        }
      }
      cJSON_Delete(root);
    }
  }
  return true;
}

bool cyd_net_cloud_save(const CydConfig& cfg, const TamagotchiCoreEngine& engine) {
  if (!s_wifi_connected) return false;
  cJSON* doc = cJSON_CreateObject();
  cJSON_AddStringToObject(doc, "deviceId", cfg.deviceId);
  cJSON* pet = cJSON_CreateObject();
  cJSON_AddStringToObject(pet, "name", engine.name);
  cJSON_AddStringToObject(pet, "trainerName", cfg.trainerName);
  cJSON_AddStringToObject(pet, "biome", engine.biome);
  cJSON_AddStringToObject(pet, "phase", engine.phase);
  cJSON_AddNumberToObject(pet, "level", engine.level);
  cJSON_AddNumberToObject(pet, "xp", engine.xp);
  cJSON_AddNumberToObject(pet, "coins", engine.coins);
  cJSON_AddItemToObject(doc, "petData", pet);

  char* body = cJSON_PrintUnformatted(doc);
  cJSON_Delete(doc);

  char url[220];
  snprintf(url, sizeof(url), "%s" CLOUD_SAVE_ENDPOINT, cfg.serverUrl);
  int code = http_do_request(url, HTTP_METHOD_POST, body, NULL, 0);
  cJSON_free(body);
  return code == 200;
}

// ============================================================================
// NON-BLOCKING DIRECT P2P SOCKETS (Port 8888) + PHP RELAY FALLBACK
// ============================================================================
void cyd_net_start_p2p_server(uint16_t port) {
  if (s_listen_sock >= 0) close(s_listen_sock);
  s_listen_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
  if (s_listen_sock < 0) return;

  int opt = 1;
  setsockopt(s_listen_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
  int flags = fcntl(s_listen_sock, F_GETFL, 0);
  fcntl(s_listen_sock, F_SETFL, flags | O_NONBLOCK);

  struct sockaddr_in addr = {};
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = htonl(INADDR_ANY);
  addr.sin_port = htons(port);

  if (bind(s_listen_sock, (struct sockaddr*)&addr, sizeof(addr)) != 0 || listen(s_listen_sock, 4) != 0) {
    close(s_listen_sock);
    s_listen_sock = -1;
    return;
  }
  ESP_LOGI(TAG, "P2P socket server listening on port %u", (unsigned)port);
}

void cyd_net_poll_p2p_socket(NetPacketCb onPacket) {
  if (s_listen_sock < 0) return;
  struct sockaddr_in sourceAddr;
  socklen_t addrLen = sizeof(sourceAddr);
  int sock = accept(s_listen_sock, (struct sockaddr*)&sourceAddr, &addrLen);
  if (sock < 0) return;

  struct timeval tv = { .tv_sec = 0, .tv_usec = 150000 };
  setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

  char rxBuf[384];
  int len = recv(sock, rxBuf, sizeof(rxBuf) - 1, 0);
  close(sock);
  if (len <= 0) return;
  rxBuf[len] = '\0';

  cJSON* doc = cJSON_Parse(rxBuf);
  if (doc) {
    cJSON* jType = cJSON_GetObjectItemCaseSensitive(doc, "type");
    cJSON* jText = cJSON_GetObjectItemCaseSensitive(doc, "text");
    if (cJSON_IsString(jType) && onPacket) {
      onPacket(jType->valuestring, cJSON_IsString(jText) ? jText->valuestring : "");
    }
    cJSON_Delete(doc);
  }
}

void cyd_net_send_packet(const CydConfig& cfg, const FriendState& friendPet, const char* type, const char* textPayload) {
  if (!s_wifi_connected) return;

  // Attempt direct TCP P2P connection first
  if (strlen(friendPet.ipAddress) > 0) {
    int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
    if (sock >= 0) {
      struct timeval tv = { .tv_sec = 1, .tv_usec = 200000 };
      setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));

      struct sockaddr_in dest = {};
      dest.sin_family = AF_INET;
      dest.sin_port = htons(friendPet.port);
      dest.sin_addr.s_addr = inet_addr(friendPet.ipAddress);

      if (connect(sock, (struct sockaddr*)&dest, sizeof(dest)) == 0) {
        cJSON* doc = cJSON_CreateObject();
        cJSON_AddStringToObject(doc, "from", cfg.deviceId);
        cJSON_AddStringToObject(doc, "type", type);
        cJSON_AddStringToObject(doc, "text", textPayload ? textPayload : "");
        char* out = cJSON_PrintUnformatted(doc);
        cJSON_Delete(doc);
        if (out) {
          send(sock, out, strlen(out), 0);
          send(sock, "\n", 1, 0);
          cJSON_free(out);
        }
        close(sock);
        return;
      }
      close(sock);
    }
  }

  // Fallback: PHP server relay sync.php
  cJSON* doc = cJSON_CreateObject();
  cJSON_AddStringToObject(doc, "fromDeviceId", cfg.deviceId);
  cJSON_AddStringToObject(doc, "toDeviceId", friendPet.deviceId);
  cJSON_AddStringToObject(doc, "type", type);
  cJSON* payload = cJSON_CreateObject();
  cJSON_AddStringToObject(payload, "text", textPayload ? textPayload : "");
  cJSON_AddItemToObject(doc, "payload", payload);

  char* body = cJSON_PrintUnformatted(doc);
  cJSON_Delete(doc);

  char url[220];
  snprintf(url, sizeof(url), "%s/sync.php", cfg.serverUrl);
  http_do_request(url, HTTP_METHOD_POST, body, NULL, 0);
  cJSON_free(body);
}

// ============================================================================
// DUAL-SLOT A/B OTA FIRMWARE & SPIFFS RULES UPDATE
// ============================================================================
static void resolve_ota_url(const CydConfig& cfg, const char* pathOrUrl, char* out, size_t maxLen) {
  if (strncmp(pathOrUrl, "http://", 7) == 0 || strncmp(pathOrUrl, "https://", 8) == 0) {
    strncpy(out, pathOrUrl, maxLen - 1);
    out[maxLen - 1] = '\0';
  } else if (pathOrUrl[0] == '/') {
    snprintf(out, maxLen, "%s%s", cfg.serverUrl, pathOrUrl);
  } else {
    snprintf(out, maxLen, "%s/%s", cfg.serverUrl, pathOrUrl);
  }
}

static bool perform_firmware_ota(const char* downloadUrl, int expectedSize, OtaProgressCb progressCb) {
  ESP_LOGI(TAG, "Starting A/B Firmware OTA from %s", downloadUrl);
  if (progressCb) progressCb("Verbinde mit OTA-Server...", 0);
  cyd_sound_level_up();

  esp_http_client_config_t config = {};
  config.url = downloadUrl;
  config.timeout_ms = 15000;
#ifdef CONFIG_MBEDTLS_CERTIFICATE_BUNDLE
  if (strncmp(downloadUrl, "https://", 8) == 0) {
    config.crt_bundle_attach = esp_crt_bundle_attach;
  }
#endif

  esp_http_client_handle_t client = esp_http_client_init(&config);
  if (!client) return false;

  if (esp_http_client_open(client, 0) != ESP_OK) {
    esp_http_client_cleanup(client);
    return false;
  }

  int contentLength = (int)esp_http_client_fetch_headers(client);
  int status = esp_http_client_get_status_code(client);
  if (status != 200) {
    ESP_LOGE(TAG, "OTA download HTTP %d", status);
    esp_http_client_close(client);
    esp_http_client_cleanup(client);
    return false;
  }
  if (contentLength <= 0 && expectedSize > 0) contentLength = expectedSize;

  const esp_partition_t* updatePart = esp_ota_get_next_update_partition(NULL);
  if (!updatePart) {
    esp_http_client_close(client);
    esp_http_client_cleanup(client);
    return false;
  }

  esp_ota_handle_t otaHandle = 0;
  esp_err_t err = esp_ota_begin(updatePart, OTA_WITH_SEQUENTIAL_WRITES, &otaHandle);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "esp_ota_begin failed: %s", esp_err_to_name(err));
    esp_http_client_close(client);
    esp_http_client_cleanup(client);
    return false;
  }

  uint8_t* chunk = (uint8_t*)malloc(2048);
  if (!chunk) {
    esp_ota_abort(otaHandle);
    esp_http_client_close(client);
    esp_http_client_cleanup(client);
    return false;
  }

  int written = 0;
  int lastPct = -1;
  bool ok = true;
  while (true) {
    int n = esp_http_client_read(client, (char*)chunk, 2048);
    if (n < 0) {
      ok = false;
      break;
    }
    if (n == 0) break;
    if (esp_ota_write(otaHandle, chunk, (size_t)n) != ESP_OK) {
      ok = false;
      break;
    }
    written += n;
    if (contentLength > 0 && progressCb) {
      int pct = (written * 100) / contentLength;
      if (pct > 100) pct = 100;
      if (pct != lastPct && (pct % 5 == 0 || pct == 100)) {
        lastPct = pct;
        progressCb("Flashe A/B OTA Slot...", pct);
      }
    }
  }
  free(chunk);
  esp_http_client_close(client);
  esp_http_client_cleanup(client);

  if (!ok || written <= 0 || esp_ota_end(otaHandle) != ESP_OK) {
    esp_ota_abort(otaHandle);
    return false;
  }

  if (esp_ota_set_boot_partition(updatePart) != ESP_OK) {
    return false;
  }

  if (progressCb) progressCb("OTA OK! Starte neu...", 100);
  cyd_sound_level_up();
  vTaskDelay(pdMS_TO_TICKS(1500));
  esp_restart();
  return true;
}

int cyd_net_check_ota(
  const CydConfig& cfg,
  TamagotchiCoreEngine& engine,
  OtaProgressCb progressCb,
  char* outMessage,
  size_t outMsgLen
) {
  if (!s_wifi_connected) {
    if (outMessage) snprintf(outMessage, outMsgLen, "Kein WiFi!");
    return -1;
  }

  char url[280];
  snprintf(url, sizeof(url),
           "%s" OTA_CHECK_ENDPOINT "?device=%s&fw_version=%s&rules_version=%s",
           cfg.serverUrl, cfg.deviceId, FIRMWARE_VERSION, RULES_VERSION);

  char* resp = (char*)malloc(2048);
  if (!resp) return -1;

  int code = http_do_request(url, HTTP_METHOD_GET, NULL, resp, 2048);
  if (code != 200) {
    if (outMessage) snprintf(outMessage, outMsgLen, "OTA Server Fehler: %d", code);
    free(resp);
    return -1;
  }

  cJSON* doc = cJSON_Parse(resp);
  free(resp);
  if (!doc) {
    if (outMessage) snprintf(outMessage, outMsgLen, "OTA JSON Fehler!");
    return -1;
  }

  bool rulesAvail = cJSON_IsTrue(cJSON_GetObjectItemCaseSensitive(doc, "rules_update_available"));
  bool fwAvail    = cJSON_IsTrue(cJSON_GetObjectItemCaseSensitive(doc, "fw_update_available"));

  int result = 0;
  if (rulesAvail) {
    cJSON* rUrl = cJSON_GetObjectItemCaseSensitive(doc, "rules_url");
    if (cJSON_IsString(rUrl)) {
      char fullUrl[260];
      resolve_ota_url(cfg, rUrl->valuestring, fullUrl, sizeof(fullUrl));
      char* rulesBuf = (char*)malloc(16384);
      if (rulesBuf) {
        if (http_do_request(fullUrl, HTTP_METHOD_GET, NULL, rulesBuf, 16384) == 200) {
          cyd_storage_save_rules(rulesBuf, strlen(rulesBuf), engine);
          cyd_sound_level_up();
          if (outMessage) snprintf(outMessage, outMsgLen, "Regeln per OTA geladen!");
          result = 1;
        }
        free(rulesBuf);
      }
    }
  } else if (fwAvail) {
    cJSON* fUrl = cJSON_GetObjectItemCaseSensitive(doc, "fw_url");
    cJSON* fSz  = cJSON_GetObjectItemCaseSensitive(doc, "fw_size");
    if (cJSON_IsString(fUrl)) {
      char fullUrl[260];
      resolve_ota_url(cfg, fUrl->valuestring, fullUrl, sizeof(fullUrl));
      int fwSize = cJSON_IsNumber(fSz) ? fSz->valueint : 0;
      cJSON_Delete(doc);
      if (!perform_firmware_ota(fullUrl, fwSize, progressCb)) {
        if (outMessage) snprintf(outMessage, outMsgLen, "OTA Download Fehler!");
        return -1;
      }
      return 1;
    }
  } else {
    if (outMessage) snprintf(outMessage, outMsgLen, "Kein Update verfuegbar");
    result = 0;
  }

  cJSON_Delete(doc);
  return result;
}
