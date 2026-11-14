#include "cyd_storage.h"
#include "../config.h"
#include "../TamagotchiCoreEngine.h"

#include "esp_log.h"
#include "esp_spiffs.h"
#include "nvs.h"
#include "nvs_flash.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

static const char* TAG = "CYD_STOR";
static const char* NVS_NS = "cyd_pet";
static const char* RULES_PATH = "/spiffs/rules.json";
static bool s_spiffs_mounted = false;

static void copy_default(char* dst, size_t dstSize, const char* fallback) {
  strncpy(dst, fallback ? fallback : "", dstSize - 1);
  dst[dstSize - 1] = '\0';
}

static void nvs_get_str_or_default(nvs_handle_t h, const char* key, char* out, size_t maxLen, const char* defVal) {
  size_t required = maxLen;
  esp_err_t err = nvs_get_str(h, key, out, &required);
  if (err != ESP_OK) {
    copy_default(out, maxLen, defVal);
  }
}

bool cyd_storage_init(CydConfig& cfg) {
  esp_err_t err = nvs_flash_init();
  if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_LOGW(TAG, "NVS init needs erase, resetting NVS...");
    ESP_ERROR_CHECK(nvs_flash_erase());
    err = nvs_flash_init();
  }
  ESP_ERROR_CHECK(err);

  esp_vfs_spiffs_conf_t spiffs_conf = {
    .base_path = "/spiffs",
    .partition_label = "spiffs",
    .max_files = 5,
    .format_if_mount_failed = true
  };
  err = esp_vfs_spiffs_register(&spiffs_conf);
  if (err == ESP_OK) {
    s_spiffs_mounted = true;
    ESP_LOGI(TAG, "SPIFFS mounted successfully");
  } else {
    ESP_LOGW(TAG, "SPIFFS mount failed (%s); using compiled defaults", esp_err_to_name(err));
  }

  cyd_storage_load_config(cfg);
  return true;
}

void cyd_storage_load_config(CydConfig& cfg) {
  copy_default(cfg.wifiSsid, sizeof(cfg.wifiSsid), WIFI_SSID);
  copy_default(cfg.wifiPass, sizeof(cfg.wifiPass), WIFI_PASSWORD);
  copy_default(cfg.deviceId, sizeof(cfg.deviceId), DEVICE_ID);
  copy_default(cfg.trainerName, sizeof(cfg.trainerName), TRAINER_NAME);
  copy_default(cfg.friendId, sizeof(cfg.friendId), FRIEND_DEV_ID);
  copy_default(cfg.biome, sizeof(cfg.biome), DEFAULT_BIOME);
  copy_default(cfg.serverUrl, sizeof(cfg.serverUrl), PHP_SERVER_BASE_URL);

  nvs_handle_t h;
  if (nvs_open(NVS_NS, NVS_READONLY, &h) == ESP_OK) {
    nvs_get_str_or_default(h, "ssid", cfg.wifiSsid, sizeof(cfg.wifiSsid), WIFI_SSID);
    nvs_get_str_or_default(h, "pass", cfg.wifiPass, sizeof(cfg.wifiPass), WIFI_PASSWORD);
    nvs_get_str_or_default(h, "dev_id", cfg.deviceId, sizeof(cfg.deviceId), DEVICE_ID);
    nvs_get_str_or_default(h, "trainer", cfg.trainerName, sizeof(cfg.trainerName), TRAINER_NAME);
    nvs_get_str_or_default(h, "friend", cfg.friendId, sizeof(cfg.friendId), FRIEND_DEV_ID);
    nvs_get_str_or_default(h, "biome", cfg.biome, sizeof(cfg.biome), DEFAULT_BIOME);
    nvs_get_str_or_default(h, "srv_url", cfg.serverUrl, sizeof(cfg.serverUrl), PHP_SERVER_BASE_URL);
    nvs_close(h);
  }
  ESP_LOGI(TAG, "NVS Config: SSID=%s Device=%s Biome=%s", cfg.wifiSsid, cfg.deviceId, cfg.biome);
}

void cyd_storage_save_config(const CydConfig& cfg) {
  nvs_handle_t h;
  if (nvs_open(NVS_NS, NVS_READWRITE, &h) == ESP_OK) {
    nvs_set_str(h, "ssid", cfg.wifiSsid);
    nvs_set_str(h, "pass", cfg.wifiPass);
    nvs_set_str(h, "dev_id", cfg.deviceId);
    nvs_set_str(h, "trainer", cfg.trainerName);
    nvs_set_str(h, "friend", cfg.friendId);
    nvs_set_str(h, "biome", cfg.biome);
    nvs_set_str(h, "srv_url", cfg.serverUrl);
    nvs_commit(h);
    nvs_close(h);
    ESP_LOGI(TAG, "NVS config saved.");
  }
}

void cyd_storage_load_pet(TamagotchiCoreEngine& engine, uint16_t& recipesCrafted, uint16_t& jobsCompleted, uint16_t& spellsCast, uint16_t& treatsSent) {
  nvs_handle_t h;
  if (nvs_open(NVS_NS, NVS_READONLY, &h) != ESP_OK) return;

  uint8_t u8 = 0;
  uint16_t u16 = 0;
  size_t len = sizeof(engine.phase);
  nvs_get_str(h, "pet_phase", engine.phase, &len);
  if (nvs_get_u8(h, "pet_lv", &u8) == ESP_OK && u8 >= 1) engine.level = u8;
  if (nvs_get_u16(h, "pet_xp", &u16) == ESP_OK) engine.xp = u16;
  if (nvs_get_u16(h, "pet_xp_n", &u16) == ESP_OK && u16 >= 50) engine.xpNext = u16;
  if (nvs_get_u16(h, "pet_coins", &u16) == ESP_OK) engine.coins = u16;
  if (nvs_get_u8(h, "pet_warmth", &u8) == ESP_OK) engine.eggWarmth = u8;
  if (nvs_get_u8(h, "pet_hp", &u8) == ESP_OK) engine.stats.health = u8;
  if (nvs_get_u8(h, "pet_fd", &u8) == ESP_OK) engine.stats.hunger = u8;
  if (nvs_get_u8(h, "pet_en", &u8) == ESP_OK) engine.stats.energy = u8;
  if (nvs_get_u8(h, "pet_hy", &u8) == ESP_OK) engine.stats.hygiene = u8;
  if (nvs_get_u8(h, "pet_ha", &u8) == ESP_OK) engine.stats.happiness = u8;
  if (nvs_get_u8(h, "pet_mp", &u8) == ESP_OK) engine.stats.mana = u8;
  if (nvs_get_u8(h, "pet_satiety", &u8) == ESP_OK) engine.satietyTicks = u8;
  if (nvs_get_u8(h, "pet_sick", &u8) == ESP_OK) engine.isSick = (u8 != 0);

  nvs_get_u16(h, "cnt_rcp", &recipesCrafted);
  nvs_get_u16(h, "cnt_job", &jobsCompleted);
  nvs_get_u16(h, "cnt_spl", &spellsCast);
  nvs_get_u16(h, "cnt_trt", &treatsSent);
  if (nvs_get_u16(h, "stk_days", &u16) == ESP_OK && u16 >= 1) engine.consecutiveLoginDays = u16;
  if (nvs_get_u8(h, "stk_clm", &u8) == ESP_OK) engine.streakClaimedToday = (u8 != 0);

  // Restore core food & material counts
  uint8_t qty = 0;
  if (nvs_get_u8(h, "inv_apple", &qty) == ESP_OK) {
    engine.inventoryCount = 0;
    if (qty > 0) engine.addKnownItemById("food_apple", qty);
    if (nvs_get_u8(h, "inv_snack", &qty) == ESP_OK && qty > 0) engine.addKnownItemById("food_snack", qty);
    if (nvs_get_u8(h, "inv_salad", &qty) == ESP_OK && qty > 0) engine.addKnownItemById("food_herb_salad", qty);
    if (nvs_get_u8(h, "inv_pie", &qty) == ESP_OK && qty > 0) engine.addKnownItemById("food_berry_pie", qty);
    if (nvs_get_u8(h, "inv_jerky", &qty) == ESP_OK && qty > 0) engine.addKnownItemById("food_sun_jerky", qty);
    if (nvs_get_u8(h, "inv_stew", &qty) == ESP_OK && qty > 0) engine.addKnownItemById("food_kelp_stew", qty);
    if (nvs_get_u8(h, "inv_ambrosia", &qty) == ESP_OK && qty > 0) engine.addKnownItemById("food_ambrosia", qty);
    if (nvs_get_u8(h, "inv_salve", &qty) == ESP_OK && qty > 0) engine.addKnownItemById("item_salve", qty);
    if (nvs_get_u8(h, "inv_elixir", &qty) == ESP_OK && qty > 0) engine.addKnownItemById("item_energy_elixir", qty);
    if (nvs_get_u8(h, "inv_herb", &qty) == ESP_OK && qty > 0) engine.addKnownItemById("mat_herb", qty);
    if (nvs_get_u8(h, "inv_mush", &qty) == ESP_OK && qty > 0) engine.addKnownItemById("mat_mushroom", qty);
    if (nvs_get_u8(h, "inv_meat", &qty) == ESP_OK && qty > 0) engine.addKnownItemById("mat_meat", qty);
    if (nvs_get_u8(h, "inv_salt", &qty) == ESP_OK && qty > 0) engine.addKnownItemById("mat_sunstone", qty);
    if (nvs_get_u8(h, "inv_kelp", &qty) == ESP_OK && qty > 0) engine.addKnownItemById("mat_kelp", qty);
  }
  nvs_close(h);
}

void cyd_storage_save_pet(const TamagotchiCoreEngine& engine, uint16_t recipesCrafted, uint16_t jobsCompleted, uint16_t spellsCast, uint16_t treatsSent) {
  nvs_handle_t h;
  if (nvs_open(NVS_NS, NVS_READWRITE, &h) != ESP_OK) return;

  nvs_set_str(h, "pet_phase", engine.phase);
  nvs_set_u8(h, "pet_lv", engine.level);
  nvs_set_u16(h, "pet_xp", engine.xp);
  nvs_set_u16(h, "pet_xp_n", engine.xpNext);
  nvs_set_u16(h, "pet_coins", engine.coins);
  nvs_set_u8(h, "pet_warmth", engine.eggWarmth);
  nvs_set_u8(h, "pet_hp", engine.stats.health);
  nvs_set_u8(h, "pet_fd", engine.stats.hunger);
  nvs_set_u8(h, "pet_en", engine.stats.energy);
  nvs_set_u8(h, "pet_hy", engine.stats.hygiene);
  nvs_set_u8(h, "pet_ha", engine.stats.happiness);
  nvs_set_u8(h, "pet_mp", engine.stats.mana);
  nvs_set_u8(h, "pet_satiety", engine.satietyTicks);
  nvs_set_u8(h, "pet_sick", engine.isSick ? 1 : 0);

  nvs_set_u16(h, "cnt_rcp", recipesCrafted);
  nvs_set_u16(h, "cnt_job", jobsCompleted);
  nvs_set_u16(h, "cnt_spl", spellsCast);
  nvs_set_u16(h, "cnt_trt", treatsSent);
  nvs_set_u16(h, "stk_days", engine.consecutiveLoginDays);
  nvs_set_u8(h, "stk_clm", engine.streakClaimedToday ? 1 : 0);

  nvs_set_u8(h, "inv_apple", engine.getItemQuantity("food_apple"));
  nvs_set_u8(h, "inv_snack", engine.getItemQuantity("food_snack"));
  nvs_set_u8(h, "inv_salad", engine.getItemQuantity("food_herb_salad"));
  nvs_set_u8(h, "inv_pie", engine.getItemQuantity("food_berry_pie"));
  nvs_set_u8(h, "inv_jerky", engine.getItemQuantity("food_sun_jerky"));
  nvs_set_u8(h, "inv_stew", engine.getItemQuantity("food_kelp_stew"));
  nvs_set_u8(h, "inv_ambrosia", engine.getItemQuantity("food_ambrosia"));
  nvs_set_u8(h, "inv_salve", engine.getItemQuantity("item_salve"));
  nvs_set_u8(h, "inv_elixir", engine.getItemQuantity("item_energy_elixir"));
  nvs_set_u8(h, "inv_herb", engine.getItemQuantity("mat_herb"));
  nvs_set_u8(h, "inv_mush", engine.getItemQuantity("mat_mushroom"));
  nvs_set_u8(h, "inv_meat", engine.getItemQuantity("mat_meat"));
  nvs_set_u8(h, "inv_salt", engine.getItemQuantity("mat_sunstone"));
  nvs_set_u8(h, "inv_kelp", engine.getItemQuantity("mat_kelp"));

  nvs_commit(h);
  nvs_close(h);
}

bool cyd_storage_load_rules(TamagotchiCoreEngine& engine) {
  if (!s_spiffs_mounted) return false;
  struct stat st;
  if (stat(RULES_PATH, &st) != 0) {
    ESP_LOGI(TAG, "No /spiffs/rules.json found, writing default rules.");
    const char* defRules =
      "{\n"
      "  \"version\": \"" RULES_VERSION "\",\n"
      "  \"hunger_decay\": 1,\n"
      "  \"energy_decay\": 1,\n"
      "  \"happiness_decay\": 1,\n"
      "  \"starve_threshold\": 20\n"
      "}\n";
    return cyd_storage_save_rules(defRules, strlen(defRules), engine);
  }

  FILE* f = fopen(RULES_PATH, "r");
  if (!f) return false;
  size_t len = (size_t)st.st_size;
  if (len > 40960) len = 40960;
  char* buf = (char*)malloc(len + 1);
  if (!buf) {
    fclose(f);
    return false;
  }
  size_t readLen = fread(buf, 1, len, f);
  fclose(f);
  buf[readLen] = '\0';

  ESP_LOGI(TAG, "Loaded /spiffs/rules.json (%u bytes)", (unsigned)readLen);
  engine.loadConfigFromText(buf);
  free(buf);
  return true;
}

bool cyd_storage_save_rules(const char* data, size_t len, TamagotchiCoreEngine& engine) {
  if (!s_spiffs_mounted || !data || len == 0) return false;
  const char* tmpPath = "/spiffs/rules_new.json";
  FILE* f = fopen(tmpPath, "w");
  if (!f) return false;
  size_t written = fwrite(data, 1, len, f);
  fclose(f);
  if (written != len) {
    remove(tmpPath);
    return false;
  }
  remove(RULES_PATH);
  if (rename(tmpPath, RULES_PATH) != 0) {
    remove(tmpPath);
    return false;
  }

  char* copy = (char*)malloc(len + 1);
  if (copy) {
    memcpy(copy, data, len);
    copy[len] = '\0';
    engine.loadConfigFromText(copy);
    free(copy);
  }
  return true;
}
