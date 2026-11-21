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
#include "esp_sntp.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "mbedtls/sha256.h"
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
#include <time.h>

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

static void safe_copy(char* dst, size_t maxLen, const char* src) {
  if (!dst || maxLen == 0) return;
  if (!src) {
    dst[0] = '\0';
    return;
  }
  strncpy(dst, src, maxLen - 1);
  dst[maxLen - 1] = '\0';
}

static void sha256_bytes_to_hex(const uint8_t hash[32], char outHex[65]) {
  static const char HEX[] = "0123456789abcdef";
  for (int i = 0; i < 32; i++) {
    outHex[i * 2]     = HEX[(hash[i] >> 4) & 0x0F];
    outHex[i * 2 + 1] = HEX[hash[i] & 0x0F];
  }
  outHex[64] = '\0';
}

static bool sha256_hex_matches(const char* actualHex, const char* expectedHex) {
  if (!expectedHex || strlen(expectedHex) != 64 || !actualHex || strlen(actualHex) != 64) {
    return true; // Skip check if server omitted a valid 64-char SHA256
  }
  for (int i = 0; i < 64; i++) {
    if (tolower((unsigned char)actualHex[i]) != tolower((unsigned char)expectedHex[i])) {
      return false;
    }
  }
  return true;
}

static void wifi_event_handler(void* /*arg*/, esp_event_base_t event_base, int32_t event_id, void* /*event_data*/) {
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

static bool s_ota_valid_report_pending = false;

void cyd_net_mark_ota_valid(const CydConfig* cfg) {
  const esp_partition_t* running = esp_ota_get_running_partition();
  esp_ota_img_states_t ota_state;
  if (running && esp_ota_get_state_partition(running, &ota_state) == ESP_OK) {
    if (ota_state == ESP_OTA_IMG_PENDING_VERIFY) {
      ESP_LOGI(TAG, "Marking new A/B OTA partition '%s' as VALID", running->label);
      esp_ota_mark_app_valid_cancel_rollback();
      s_ota_valid_report_pending = true;
    }
  }
  if (s_ota_valid_report_pending && cfg && s_wifi_connected) {
    s_ota_valid_report_pending = false;
    cyd_net_report_ota(*cfg, "ota_valid", running ? running->label : "unknown");
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

static bool s_sntp_started = false;

void cyd_net_init_sntp(void) {
  setenv("TZ", "CET-1CEST,M3.5.0,M10.5.0/3", 1);
  tzset();
  if (s_sntp_started) return;
  s_sntp_started = true;
  esp_sntp_setoperatingmode(ESP_SNTP_OPMODE_POLL);
  esp_sntp_setservername(0, "pool.ntp.org");
  esp_sntp_init();
  ESP_LOGI(TAG, "SNTP initialized (pool.ntp.org, Europe/Berlin TZ)");
}

static bool is_leap_year(int year) {
  return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}

bool cyd_net_get_berlin_time(int* outIsoWeek, uint32_t* outYyyymmdd, int* outHour) {
  time_t now = 0;
  time(&now);
  if (now < 1704067200) { // Before 2024-01-01 -> SNTP not yet synced
    return false;
  }
  struct tm tmInfo = {};
  localtime_r(&now, &tmInfo);
  int year = tmInfo.tm_year + 1900;
  int month = tmInfo.tm_mon + 1;
  int day = tmInfo.tm_mday;

  if (outYyyymmdd) {
    *outYyyymmdd = (uint32_t)(year * 10000 + month * 100 + day);
  }
  if (outHour) {
    *outHour = tmInfo.tm_hour;
  }
  if (outIsoWeek) {
    int wday = (tmInfo.tm_wday == 0) ? 7 : tmInfo.tm_wday; // 1=Mon..7=Sun
    int thursYday = tmInfo.tm_yday + (4 - wday);
    int daysInYear = is_leap_year(year) ? 366 : 365;
    if (thursYday < 0) {
      int prevDays = is_leap_year(year - 1) ? 366 : 365;
      *outIsoWeek = ((thursYday + prevDays) / 7) + 1;
    } else if (thursYday >= daysInYear) {
      *outIsoWeek = 1;
    } else {
      *outIsoWeek = (thursYday / 7) + 1;
    }
  }
  return true;
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

static void delayed_restart_task(void* /*arg*/) {
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
      safe_copy(s_portal_cfg->wifiSsid, sizeof(s_portal_cfg->wifiSsid), tmp);
    } else if (extract_form_field(buf, "ssid", tmp, sizeof(tmp)) && strlen(tmp) > 0) {
      safe_copy(s_portal_cfg->wifiSsid, sizeof(s_portal_cfg->wifiSsid), tmp);
    }
    if (extract_form_field(buf, "password", tmp, sizeof(tmp)) && strlen(tmp) > 0) {
      safe_copy(s_portal_cfg->wifiPass, sizeof(s_portal_cfg->wifiPass), tmp);
    }
    if (extract_form_field(buf, "device_id", tmp, sizeof(tmp)) && strlen(tmp) > 0) {
      safe_copy(s_portal_cfg->deviceId, sizeof(s_portal_cfg->deviceId), tmp);
    }
    if (extract_form_field(buf, "trainer_name", tmp, sizeof(tmp)) && strlen(tmp) > 0) {
      safe_copy(s_portal_cfg->trainerName, sizeof(s_portal_cfg->trainerName), tmp);
    }
    if (extract_form_field(buf, "friend_id", tmp, sizeof(tmp)) && strlen(tmp) > 0) {
      safe_copy(s_portal_cfg->friendId, sizeof(s_portal_cfg->friendId), tmp);
    }
    if (extract_form_field(buf, "biome", tmp, sizeof(tmp)) && strlen(tmp) > 0) {
      safe_copy(s_portal_cfg->biome, sizeof(s_portal_cfg->biome), tmp);
    }
    if (extract_form_field(buf, "server_url", tmp, sizeof(tmp)) && strlen(tmp) > 0) {
      safe_copy(s_portal_cfg->serverUrl, sizeof(s_portal_cfg->serverUrl), tmp);
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
// HTTP CLIENT HELPERS
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
  config.timeout_ms = 5000;
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
  if (code != 200) {
    safe_copy(friendPet.linkMode, sizeof(friendPet.linkMode), "OFF");
    return false;
  }

  snprintf(url, sizeof(url), "%s/find_friend.php?friendId=%s", cfg.serverUrl, cfg.friendId);
  char resp[768];
  if (http_do_request(url, HTTP_METHOD_GET, NULL, resp, sizeof(resp)) == 200) {
    cJSON* root = cJSON_Parse(resp);
    if (root) {
      cJSON* f = cJSON_GetObjectItemCaseSensitive(root, "friend");
      if (cJSON_IsObject(f)) {
        cJSON* jOn   = cJSON_GetObjectItemCaseSensitive(f, "isOnline");
        cJSON* jName = cJSON_GetObjectItemCaseSensitive(f, "petName");
        cJSON* jBio  = cJSON_GetObjectItemCaseSensitive(f, "biome");
        cJSON* jLv   = cJSON_GetObjectItemCaseSensitive(f, "level");
        cJSON* jIp   = cJSON_GetObjectItemCaseSensitive(f, "ipAddress");
        cJSON* jPort = cJSON_GetObjectItemCaseSensitive(f, "port");
        cJSON* jPh   = cJSON_GetObjectItemCaseSensitive(f, "phase");
        cJSON* jSt   = cJSON_GetObjectItemCaseSensitive(f, "stats");

        if (cJSON_IsBool(jOn)) friendPet.isOnline = cJSON_IsTrue(jOn);
        if (cJSON_IsString(jName)) safe_copy(friendPet.name, sizeof(friendPet.name), jName->valuestring);
        if (cJSON_IsString(jBio)) safe_copy(friendPet.biome, sizeof(friendPet.biome), jBio->valuestring);
        if (cJSON_IsString(jPh)) safe_copy(friendPet.phase, sizeof(friendPet.phase), jPh->valuestring);
        if (cJSON_IsNumber(jLv)) friendPet.level = (uint8_t)jLv->valueint;
        if (cJSON_IsString(jIp)) safe_copy(friendPet.ipAddress, sizeof(friendPet.ipAddress), jIp->valuestring);
        if (cJSON_IsNumber(jPort) && jPort->valueint > 0) friendPet.port = (uint16_t)jPort->valueint;
        if (cJSON_IsObject(jSt)) {
          cJSON* hp = cJSON_GetObjectItemCaseSensitive(jSt, "health");
          cJSON* fd = cJSON_GetObjectItemCaseSensitive(jSt, "hunger");
          if (cJSON_IsNumber(hp)) friendPet.health = (uint8_t)hp->valueint;
          if (cJSON_IsNumber(fd)) friendPet.hunger = (uint8_t)fd->valueint;
        }
        if (strcmp(friendPet.linkMode, "LAN") != 0) {
          safe_copy(friendPet.linkMode, sizeof(friendPet.linkMode), friendPet.isOnline ? "NET" : "OFF");
        }
      }
      cJSON_Delete(root);
    }
  }
  return true;
}

static void format_server_url(const char* baseUrl, const char* path, char* outUrl, size_t maxLen) {
  if (!outUrl || maxLen == 0) return;
  outUrl[0] = '\0';
  if (!baseUrl) return;
  size_t bLen = strlen(baseUrl);
  while (bLen > 0 && baseUrl[bLen - 1] == '/') {
    bLen--;
  }
  snprintf(outUrl, maxLen, "%.*s%s", (int)bLen, baseUrl, path ? path : "");
}

static void url_encode_component(const char* src, char* dst, size_t maxLen) {
  if (!dst || maxLen == 0) return;
  static const char HEX[] = "0123456789ABCDEF";
  size_t pos = 0;
  while (src && *src && pos + 1 < maxLen) {
    unsigned char c = (unsigned char)*src++;
    if (isalnum(c) || c == '-' || c == '_' || c == '.') {
      dst[pos++] = (char)c;
    } else if (pos + 4 <= maxLen) {
      dst[pos++] = '%';
      dst[pos++] = HEX[(c >> 4) & 0x0F];
      dst[pos++] = HEX[c & 0x0F];
    } else {
      break;
    }
  }
  dst[pos] = '\0';
}

// ============================================================================
// FULL CLOUD SAVE & RESTORE (cloud_save.php / /api/cloud_save)
// ============================================================================
bool cyd_net_cloud_save(
  const CydConfig& cfg,
  const TamagotchiCoreEngine& engine,
  uint16_t recipesCrafted,
  uint16_t jobsCompleted,
  uint16_t spellsCast,
  uint16_t treatsSent,
  uint32_t* outUpdatedAt
) {
  if (!s_wifi_connected || strlen(cfg.serverUrl) == 0 || strlen(cfg.deviceId) == 0) return false;
  char* body = (char*)malloc(4096);
  if (!body) return false;

  size_t len = engine.serializeCloudSaveJson(
    body, 4096,
    cfg.deviceId, cfg.trainerName,
    recipesCrafted, jobsCompleted, spellsCast, treatsSent,
    FIRMWARE_VERSION
  );
  if (len == 0) {
    free(body);
    return false;
  }

  char url[256];
  format_server_url(cfg.serverUrl, CLOUD_SAVE_ENDPOINT, url, sizeof(url));

  char resp[512];
  int code = http_do_request(url, HTTP_METHOD_POST, body, resp, sizeof(resp));
  free(body);

  if (code == 200 && outUpdatedAt) {
    cJSON* rDoc = cJSON_Parse(resp);
    if (rDoc) {
      cJSON* jTs = cJSON_GetObjectItemCaseSensitive(rDoc, "updatedAt");
      if (cJSON_IsNumber(jTs) && jTs->valuedouble > 0) {
        double dv = jTs->valuedouble;
        *outUpdatedAt = (dv > 20000000000.0) ? (uint32_t)(dv / 1000.0) : (uint32_t)dv;
      }
      cJSON_Delete(rDoc);
    }
  }
  ESP_LOGI(TAG, "Cloud Save POST %s status=%d", url, code);
  return code == 200;
}

bool cyd_net_cloud_load(
  const CydConfig& cfg,
  TamagotchiCoreEngine& engine,
  uint16_t& recipesCrafted,
  uint16_t& jobsCompleted,
  uint16_t& spellsCast,
  uint16_t& treatsSent,
  char* outTrainerName,
  size_t trainerNameLen,
  uint32_t* outUpdatedAt
) {
  if (!s_wifi_connected || strlen(cfg.serverUrl) == 0 || strlen(cfg.deviceId) == 0) return false;

  char encodedDev[72];
  url_encode_component(cfg.deviceId, encodedDev, sizeof(encodedDev));
  char path[128];
  snprintf(path, sizeof(path), CLOUD_SAVE_ENDPOINT "?deviceId=%s", encodedDev);
  char url[256];
  format_server_url(cfg.serverUrl, path, url, sizeof(url));

  const size_t RESP_CAP = 12288;
  char* resp = (char*)malloc(RESP_CAP);
  if (!resp) return false;

  int code = http_do_request(url, HTTP_METHOD_GET, NULL, resp, RESP_CAP);
  ESP_LOGI(TAG, "Cloud Restore GET %s status=%d bytes=%u", url, code, (unsigned)strlen(resp));
  if (code != 200) {
    free(resp);
    return false;
  }

  bool ok = engine.parseCloudRestoreJson(
    resp,
    recipesCrafted, jobsCompleted, spellsCast, treatsSent,
    outTrainerName, trainerNameLen, outUpdatedAt
  );
  free(resp);
  return ok;
}

// ============================================================================
// SERVER SHOP VALIDATION (shop.php)
// ============================================================================
int cyd_net_shop_purchase(
  const CydConfig& cfg,
  const char* itemType,
  const char* itemId,
  uint16_t currentCoins,
  uint8_t currentLevel,
  char* outErrMsg,
  size_t errLen
) {
  if (outErrMsg && errLen > 0) outErrMsg[0] = '\0';
  if (!s_wifi_connected) return -1;

  cJSON* doc = cJSON_CreateObject();
  cJSON_AddStringToObject(doc, "deviceId", cfg.deviceId);
  cJSON_AddStringToObject(doc, "itemType", itemType ? itemType : "item");
  cJSON_AddStringToObject(doc, "itemId", itemId ? itemId : "");
  cJSON_AddNumberToObject(doc, "currentCoins", currentCoins);
  cJSON_AddNumberToObject(doc, "currentLevel", currentLevel);
  char* body = cJSON_PrintUnformatted(doc);
  cJSON_Delete(doc);
  if (!body) return -1;

  char url[220];
  snprintf(url, sizeof(url), "%s" SHOP_ENDPOINT, cfg.serverUrl);
  char resp[512];
  int code = http_do_request(url, HTTP_METHOD_POST, body, resp, sizeof(resp));
  cJSON_free(body);

  if (code == 200) return 1;
  if (code < 0 || code >= 500) return -1; // Offline / unreachable -> allow local engine purchase

  cJSON* errJson = cJSON_Parse(resp);
  if (errJson) {
    cJSON* msg = cJSON_GetObjectItemCaseSensitive(errJson, "message");
    if (cJSON_IsString(msg) && outErrMsg) {
      safe_copy(outErrMsg, errLen, msg->valuestring);
    }
    cJSON_Delete(errJson);
  }
  return 0;
}

// ============================================================================
// NON-BLOCKING P2P SOCKETS (Port 8888) + PHP RELAY (GET / POST sync.php)
// ============================================================================
static void extract_packet_fields(cJSON* obj, NetPacket& pkt) {
  if (!cJSON_IsObject(obj)) return;
  cJSON* v = NULL;
  if ((v = cJSON_GetObjectItemCaseSensitive(obj, "type")) && cJSON_IsString(v)) safe_copy(pkt.type, sizeof(pkt.type), v->valuestring);
  if ((v = cJSON_GetObjectItemCaseSensitive(obj, "from")) && cJSON_IsString(v)) safe_copy(pkt.from, sizeof(pkt.from), v->valuestring);
  if ((v = cJSON_GetObjectItemCaseSensitive(obj, "fromDeviceId")) && cJSON_IsString(v)) safe_copy(pkt.from, sizeof(pkt.from), v->valuestring);
  if ((v = cJSON_GetObjectItemCaseSensitive(obj, "text")) && cJSON_IsString(v)) safe_copy(pkt.text, sizeof(pkt.text), v->valuestring);
  if ((v = cJSON_GetObjectItemCaseSensitive(obj, "tradeId")) && cJSON_IsString(v)) safe_copy(pkt.tradeId, sizeof(pkt.tradeId), v->valuestring);
  if ((v = cJSON_GetObjectItemCaseSensitive(obj, "offeredId")) && cJSON_IsString(v)) safe_copy(pkt.offeredId, sizeof(pkt.offeredId), v->valuestring);
  if ((v = cJSON_GetObjectItemCaseSensitive(obj, "offeredName")) && cJSON_IsString(v)) safe_copy(pkt.offeredName, sizeof(pkt.offeredName), v->valuestring);
  if ((v = cJSON_GetObjectItemCaseSensitive(obj, "requestedId")) && cJSON_IsString(v)) safe_copy(pkt.requestedId, sizeof(pkt.requestedId), v->valuestring);
  if ((v = cJSON_GetObjectItemCaseSensitive(obj, "requestedName")) && cJSON_IsString(v)) safe_copy(pkt.requestedName, sizeof(pkt.requestedName), v->valuestring);
}

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

  char rxBuf[512];
  int len = recv(sock, rxBuf, sizeof(rxBuf) - 1, 0);
  close(sock);
  if (len <= 0) return;
  rxBuf[len] = '\0';

  cJSON* doc = cJSON_Parse(rxBuf);
  if (doc) {
    NetPacket pkt = {};
    extract_packet_fields(doc, pkt);
    if (strlen(pkt.type) > 0 && onPacket) {
      onPacket(pkt);
    }
    cJSON_Delete(doc);
  }
}

void cyd_net_poll_relay(const CydConfig& cfg, NetPacketCb onPacket) {
  if (!s_wifi_connected || !onPacket) return;
  char url[220];
  snprintf(url, sizeof(url), "%s/sync.php?deviceId=%s", cfg.serverUrl, cfg.deviceId);
  char* resp = (char*)malloc(2048);
  if (!resp) return;

  if (http_do_request(url, HTTP_METHOD_GET, NULL, resp, 2048) == 200) {
    cJSON* root = cJSON_Parse(resp);
    if (root) {
      cJSON* msgs = cJSON_GetObjectItemCaseSensitive(root, "messages");
      if (cJSON_IsArray(msgs)) {
        cJSON* m = NULL;
        cJSON_ArrayForEach(m, msgs) {
          NetPacket pkt = {};
          extract_packet_fields(m, pkt);
          cJSON* payload = cJSON_GetObjectItemCaseSensitive(m, "payload");
          if (cJSON_IsObject(payload)) {
            extract_packet_fields(payload, pkt);
          }
          if (strlen(pkt.type) > 0) {
            onPacket(pkt);
          }
        }
      }
      cJSON_Delete(root);
    }
  }
  free(resp);
}

bool cyd_net_send_packet_ex(const CydConfig& cfg, FriendState& friendPet, const NetPacket& pkt) {
  if (!s_wifi_connected) return false;

  // 1. Attempt direct TCP P2P connection first
  if (strlen(friendPet.ipAddress) > 0 && strcmp(friendPet.ipAddress, "0.0.0.0") != 0) {
    int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
    if (sock >= 0) {
      struct timeval tv = { .tv_sec = 0, .tv_usec = 800000 };
      setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));

      struct sockaddr_in dest = {};
      dest.sin_family = AF_INET;
      dest.sin_port = htons(friendPet.port > 0 ? friendPet.port : SOCKET_PORT);
      dest.sin_addr.s_addr = inet_addr(friendPet.ipAddress);

      if (connect(sock, (struct sockaddr*)&dest, sizeof(dest)) == 0) {
        cJSON* doc = cJSON_CreateObject();
        cJSON_AddStringToObject(doc, "from", cfg.deviceId);
        cJSON_AddStringToObject(doc, "type", pkt.type);
        cJSON_AddStringToObject(doc, "text", pkt.text);
        if (strlen(pkt.tradeId) > 0)       cJSON_AddStringToObject(doc, "tradeId", pkt.tradeId);
        if (strlen(pkt.offeredId) > 0)     cJSON_AddStringToObject(doc, "offeredId", pkt.offeredId);
        if (strlen(pkt.offeredName) > 0)   cJSON_AddStringToObject(doc, "offeredName", pkt.offeredName);
        if (strlen(pkt.requestedId) > 0)   cJSON_AddStringToObject(doc, "requestedId", pkt.requestedId);
        if (strlen(pkt.requestedName) > 0) cJSON_AddStringToObject(doc, "requestedName", pkt.requestedName);

        char* out = cJSON_PrintUnformatted(doc);
        cJSON_Delete(doc);
        if (out) {
          send(sock, out, strlen(out), 0);
          send(sock, "\n", 1, 0);
          cJSON_free(out);
        }
        close(sock);
        safe_copy(friendPet.linkMode, sizeof(friendPet.linkMode), "LAN");
        return true;
      }
      close(sock);
    }
  }

  // 2. Fallback: PHP Server Mailbox Relay (POST /sync.php)
  cJSON* doc = cJSON_CreateObject();
  cJSON_AddStringToObject(doc, "fromDeviceId", cfg.deviceId);
  cJSON_AddStringToObject(doc, "toDeviceId", strlen(friendPet.deviceId) > 0 ? friendPet.deviceId : cfg.friendId);
  cJSON_AddStringToObject(doc, "type", pkt.type);

  cJSON* payload = cJSON_CreateObject();
  cJSON_AddStringToObject(payload, "text", pkt.text);
  if (strlen(pkt.tradeId) > 0)       cJSON_AddStringToObject(payload, "tradeId", pkt.tradeId);
  if (strlen(pkt.offeredId) > 0)     cJSON_AddStringToObject(payload, "offeredId", pkt.offeredId);
  if (strlen(pkt.offeredName) > 0)   cJSON_AddStringToObject(payload, "offeredName", pkt.offeredName);
  if (strlen(pkt.requestedId) > 0)   cJSON_AddStringToObject(payload, "requestedId", pkt.requestedId);
  if (strlen(pkt.requestedName) > 0) cJSON_AddStringToObject(payload, "requestedName", pkt.requestedName);
  cJSON_AddItemToObject(doc, "payload", payload);

  char* body = cJSON_PrintUnformatted(doc);
  cJSON_Delete(doc);
  if (!body) return false;

  char url[220];
  snprintf(url, sizeof(url), "%s/sync.php", cfg.serverUrl);
  int code = http_do_request(url, HTTP_METHOD_POST, body, NULL, 0);
  cJSON_free(body);
  if (code == 200) {
    safe_copy(friendPet.linkMode, sizeof(friendPet.linkMode), "NET");
    return true;
  }
  return false;
}

void cyd_net_send_packet(const CydConfig& cfg, FriendState& friendPet, const char* type, const char* textPayload) {
  NetPacket pkt = {};
  safe_copy(pkt.type, sizeof(pkt.type), type);
  safe_copy(pkt.from, sizeof(pkt.from), cfg.deviceId);
  safe_copy(pkt.text, sizeof(pkt.text), textPayload);
  cyd_net_send_packet_ex(cfg, friendPet, pkt);
}

// ============================================================================
// DUAL-SLOT A/B OTA FIRMWARE & SPIFFS RULES UPDATE (WITH SHA256 CHECKSUM)
// ============================================================================
static void resolve_ota_url(const CydConfig& cfg, const char* pathOrUrl, char* out, size_t maxLen) {
  if (strncmp(pathOrUrl, "http://", 7) == 0 || strncmp(pathOrUrl, "https://", 8) == 0) {
    safe_copy(out, maxLen, pathOrUrl);
  } else if (pathOrUrl[0] == '/') {
    snprintf(out, maxLen, "%s%s", cfg.serverUrl, pathOrUrl);
  } else {
    snprintf(out, maxLen, "%s/%s", cfg.serverUrl, pathOrUrl);
  }
}

void cyd_net_report_ota(const CydConfig& cfg, const char* event, const char* detail) {
  if (!s_wifi_connected) return;
  const esp_partition_t* running = esp_ota_get_running_partition();

  cJSON* doc = cJSON_CreateObject();
  cJSON_AddStringToObject(doc, "deviceId", cfg.deviceId);
  cJSON_AddStringToObject(doc, "event", event ? event : "status");
  cJSON_AddStringToObject(doc, "version", FIRMWARE_VERSION);
  cJSON_AddStringToObject(doc, "partition", running ? running->label : "unknown");
  cJSON_AddStringToObject(doc, "detail", detail ? detail : "");
  char* body = cJSON_PrintUnformatted(doc);
  cJSON_Delete(doc);
  if (!body) return;

  char url[220];
  snprintf(url, sizeof(url), "%s/ota_report.php", cfg.serverUrl);
  http_do_request(url, HTTP_METHOD_POST, body, NULL, 0);
  cJSON_free(body);
}

static bool perform_firmware_ota(
  const CydConfig& cfg,
  const char* downloadUrl,
  int expectedSize,
  const char* expectedSha256,
  OtaProgressCb progressCb
) {
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

  mbedtls_sha256_context shaCtx;
  mbedtls_sha256_init(&shaCtx);
  mbedtls_sha256_starts(&shaCtx, 0);

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
    mbedtls_sha256_update(&shaCtx, chunk, (size_t)n);
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

  uint8_t shaBin[32];
  char actualHex[65];
  mbedtls_sha256_finish(&shaCtx, shaBin);
  mbedtls_sha256_free(&shaCtx);
  sha256_bytes_to_hex(shaBin, actualHex);

  if (ok && !sha256_hex_matches(actualHex, expectedSha256)) {
    ESP_LOGE(TAG, "OTA SHA256 mismatch! actual=%s expected=%s", actualHex, expectedSha256);
    if (progressCb) progressCb("SHA256 Pruefung fehlgeschlagen!", 0);
    cyd_net_report_ota(cfg, "sha256_mismatch", actualHex);
    esp_ota_abort(otaHandle);
    return false;
  }

  if (!ok || written <= 0 || esp_ota_end(otaHandle) != ESP_OK) {
    esp_ota_abort(otaHandle);
    cyd_net_report_ota(cfg, "ota_write_failed", "esp_ota_end");
    return false;
  }

  if (esp_ota_set_boot_partition(updatePart) != ESP_OK) {
    return false;
  }

  cyd_net_report_ota(cfg, "ota_flashed", updatePart->label);
  if (progressCb) progressCb("SHA256 OK! Starte neu...", 100);
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

  const char* activeRulesVer = (strlen(engine.rules.version) > 0) ? engine.rules.version : RULES_VERSION;
  char url[280];
  snprintf(url, sizeof(url),
           "%s" OTA_CHECK_ENDPOINT "?device=%s&fw_version=%s&rules_version=%s",
           cfg.serverUrl, cfg.deviceId, FIRMWARE_VERSION, activeRulesVer);

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
    cJSON* rSha = cJSON_GetObjectItemCaseSensitive(doc, "rules_sha256");
    const char* expSha = cJSON_IsString(rSha) ? rSha->valuestring : "";

    if (cJSON_IsString(rUrl)) {
      char fullUrl[260];
      resolve_ota_url(cfg, rUrl->valuestring, fullUrl, sizeof(fullUrl));
      char* rulesBuf = (char*)malloc(36864);
      if (rulesBuf) {
        if (http_do_request(fullUrl, HTTP_METHOD_GET, NULL, rulesBuf, 36864) == 200) {
          size_t rLen = strlen(rulesBuf);
          uint8_t shaBin[32];
          char actualHex[65];
          mbedtls_sha256_context rShaCtx;
          mbedtls_sha256_init(&rShaCtx);
          mbedtls_sha256_starts(&rShaCtx, 0);
          mbedtls_sha256_update(&rShaCtx, (const unsigned char*)rulesBuf, rLen);
          mbedtls_sha256_finish(&rShaCtx, shaBin);
          mbedtls_sha256_free(&rShaCtx);
          sha256_bytes_to_hex(shaBin, actualHex);

          cJSON* parsedRules = cJSON_Parse(rulesBuf);
          if (parsedRules && sha256_hex_matches(actualHex, expSha)) {
            cJSON_Delete(parsedRules);
            if (cyd_storage_save_rules(rulesBuf, rLen, engine)) {
              cyd_sound_level_up();
              if (outMessage) snprintf(outMessage, outMsgLen, "Regeln (SHA256 OK) geladen!");
              cyd_net_report_ota(cfg, "rules_hot_reload", "ok");
              result = 1;
            }
          } else {
            if (parsedRules) cJSON_Delete(parsedRules);
            if (outMessage) snprintf(outMessage, outMsgLen, "Regeln SHA256/JSON Fehler!");
            result = -1;
          }
        }
        free(rulesBuf);
      }
    }
  } else if (fwAvail) {
    cJSON* fUrl = cJSON_GetObjectItemCaseSensitive(doc, "fw_url");
    cJSON* fSz  = cJSON_GetObjectItemCaseSensitive(doc, "fw_size");
    cJSON* fSha = cJSON_GetObjectItemCaseSensitive(doc, "fw_sha256");
    if (cJSON_IsString(fUrl)) {
      char fullUrl[260];
      resolve_ota_url(cfg, fUrl->valuestring, fullUrl, sizeof(fullUrl));
      int fwSize = cJSON_IsNumber(fSz) ? fSz->valueint : 0;
      char expSha[65] = "";
      if (cJSON_IsString(fSha)) safe_copy(expSha, sizeof(expSha), fSha->valuestring);
      cJSON_Delete(doc);
      if (!perform_firmware_ota(cfg, fullUrl, fwSize, expSha, progressCb)) {
        if (outMessage) snprintf(outMessage, outMsgLen, "OTA SHA256/Download Fehler!");
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
