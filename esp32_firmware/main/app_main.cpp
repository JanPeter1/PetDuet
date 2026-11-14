#include "cyd_display.h"
#include "cyd_hw.h"
#include "cyd_touch.h"
#include "cyd_storage.h"
#include "cyd_net.h"
#include "cyd_lvgl_ui.h"
#include "../config.h"

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <stdio.h>
#include <string.h>

static const char* TAG = "PETDUET_MAIN";

static CydDisplay s_display;
static AppRuntimeState s_app = {};

static void on_ota_progress(const char* stage, int percent) {
  cyd_lvgl_show_ota_progress(stage, percent);
}

static void on_p2p_packet(const char* type, const char* text) {
  if (!type) return;
  if (strcmp(type, "TREAT") == 0) {
    cyd_sound_treat_received();
    s_app.engine.receiveRemoteTreat();
    cyd_lvgl_show_banner("Freund hat Snack gesendet!", 3000);
  } else if (strcmp(type, "CHAT") == 0) {
    cyd_sound_treat_received();
    if (text) {
      strncpy(s_app.friendPet.speechBalloon, text, sizeof(s_app.friendPet.speechBalloon) - 1);
      s_app.friendPet.speechBalloon[sizeof(s_app.friendPet.speechBalloon) - 1] = '\0';
      s_app.friendPet.balloonUntilMs = cyd_millis() + 6000;
    }
  } else if (strcmp(type, "MAGIC") == 0) {
    cyd_sound_treat_received();
    s_app.engine.stats.health = (uint8_t)engineMin(100, s_app.engine.stats.health + 30);
    s_app.engine.stats.happiness = (uint8_t)engineMin(100, s_app.engine.stats.happiness + 25);
    cyd_lvgl_show_banner("Freund-Segen erhalten!", 3000);
  } else if (strcmp(type, "QUEST") == 0) {
    cyd_sound_treat_received();
    s_app.engine.coins += 20;
    s_app.engine.addXP(25);
    cyd_lvgl_show_banner("Koop-Quest vom Freund! +20$", 3000);
  } else if (strcmp(type, "TRADE") == 0) {
    cyd_sound_treat_received();
    cyd_lvgl_show_banner("Tausch-Angebot erhalten!", 3000);
  }
  s_app.dirtyUi = true;
}

static void enter_ap_portal_loop(void) {
  cyd_set_rgb(false, false, true);
  cyd_net_start_ap_portal(s_app.config);
  cyd_lvgl_show_ap_screen();
  s_app.apCancelRequested = false;

  while (!s_app.apCancelRequested) {
    lv_timer_handler();
    cyd_delay_ms(15);
  }

  cyd_net_stop_all();
  s_app.offlineMode = true;
  cyd_set_rgb(true, false, false);
  cyd_lvgl_show_main_screen();
  cyd_lvgl_show_banner("Offline Modus aktiv", 2500);
}

extern "C" void app_main(void) {
  ESP_LOGI(TAG, "PetDuet CYD Booting (ESP-IDF + LVGL v8)...");

  // 1. Hardware init (LED, Buzzer, Backlight, SPI Display, SPI Touch)
  cyd_hw_init();
  cyd_set_rgb(true, true, false); // Yellow boot indicator
  cyd_play_tone(880, 60);

  s_display.init();
  cyd_touch_init();
  cyd_lvgl_init(&s_display);

  // 2. Storage (NVS + SPIFFS) & Engine setup
  cyd_storage_init(s_app.config);
  strncpy(s_app.engine.biome, s_app.config.biome, sizeof(s_app.engine.biome) - 1);
  strncpy(s_app.engine.name, s_app.config.trainerName, sizeof(s_app.engine.name) - 1);

  strncpy(s_app.friendPet.deviceId, s_app.config.friendId, sizeof(s_app.friendPet.deviceId) - 1);
  strcpy(s_app.friendPet.name, "Sunscale");
  strcpy(s_app.friendPet.biome, "desert");
  strcpy(s_app.friendPet.phase, "Baby");
  s_app.friendPet.level = 2;
  s_app.friendPet.health = 95;
  s_app.friendPet.port = SOCKET_PORT;
  strcpy(s_app.friendPet.speechBalloon, "Hello Buddy!");

  cyd_storage_load_rules(s_app.engine);
  cyd_storage_load_pet(s_app.engine, s_app.recipesCrafted, s_app.jobsCompleted, s_app.spellsCast, s_app.treatsSent);
  cyd_lvgl_set_app_state(&s_app);

  // Validate current A/B OTA partition after successful init
  cyd_net_mark_ota_valid();

  // 3. BOOT button held -> force SoftAP WiFi config portal
  if (cyd_boot_button_pressed()) {
    ESP_LOGI(TAG, "BOOT button held -> launching AP config portal");
    enter_ap_portal_loop();
  } else {
    cyd_lvgl_show_boot_screen(s_app.config.wifiSsid);
    char ipBuf[20] = "";
    if (cyd_net_connect_sta(s_app.config, ipBuf, sizeof(ipBuf))) {
      cyd_net_start_p2p_server(SOCKET_PORT);
      cyd_set_rgb(false, true, false);
      cyd_sound_level_up();

      if (!cyd_net_test_php_server(s_app.config)) {
        cyd_set_rgb(true, false, true);
        cyd_lvgl_show_server_error_screen(ipBuf, s_app.config.serverUrl);
        s_app.apCancelRequested = false;
        s_app.retryPortalRequested = false;
        while (!s_app.apCancelRequested && !s_app.retryPortalRequested) {
          lv_timer_handler();
          cyd_delay_ms(15);
        }
        if (s_app.retryPortalRequested) {
          enter_ap_portal_loop();
        } else {
          s_app.offlineMode = true;
          cyd_set_rgb(true, false, false);
          cyd_lvgl_show_main_screen();
        }
      } else {
        cyd_net_sync_php_server(s_app.config, s_app.engine, s_app.friendPet);
        char otaMsg[48] = "";
        cyd_net_check_ota(s_app.config, s_app.engine, on_ota_progress, otaMsg, sizeof(otaMsg));
        cyd_lvgl_show_main_screen();
        cyd_lvgl_show_banner("WLAN & Socket bereit!", 2500);
      }
    } else {
      ESP_LOGW(TAG, "WiFi connect failed -> launching AP config portal");
      enter_ap_portal_loop();
    }
  }

  // 4. Main Real-Time Loop
  uint32_t lastTickMs = cyd_millis();
  uint32_t lastSyncMs = cyd_millis();
  uint32_t lastOtaMs  = cyd_millis();
  uint32_t lastSaveMs = cyd_millis();
  uint32_t lastJobUiMs = cyd_millis();

  while (true) {
    uint32_t now = cyd_millis();

    // Incoming LAN socket messages
    if (!s_app.offlineMode) {
      cyd_net_poll_p2p_socket(on_p2p_packet);
    }

    // Active job countdown & completion
    if (s_app.activeJob.running) {
      if ((now - s_app.activeJob.startedMs) >= s_app.activeJob.durationMs) {
        s_app.activeJob.running = false;
        uint8_t jIdx = s_app.activeJob.jobIndex;
        uint16_t cGain = (jIdx == 0) ? 20 : (jIdx == 1 ? 35 : 60);
        uint16_t xGain = (jIdx == 0) ? 25 : (jIdx == 1 ? 45 : 70);
        s_app.engine.coins += cGain;
        s_app.engine.addXP(xGain);
        s_app.jobsCompleted++;
        if (jIdx == 0) {
          s_app.engine.addKnownItemById("mat_herb", 2);
          s_app.engine.addKnownItemById("food_apple", 1);
        } else if (jIdx == 1) {
          s_app.engine.addKnownItemById("food_snack", 1);
          s_app.engine.addKnownItemById("mat_meat", 1);
        } else {
          s_app.engine.addKnownItemById("mat_mushroom", 2);
          s_app.engine.addKnownItemById("mat_sunstone", 1);
        }
        cyd_sound_level_up();
        char bMsg[48];
        snprintf(bMsg, sizeof(bMsg), "Beruf fertig! +%u$ +%uXP", cGain, xGain);
        cyd_lvgl_show_banner(bMsg, 3200);
        if (s_app.activeModal == TAB_JOBS) {
          cyd_lvgl_open_modal(TAB_JOBS);
        }
        s_app.dirtyUi = true;
      } else if ((now - lastJobUiMs) >= 1000) {
        lastJobUiMs = now;
        if (s_app.activeModal == TAB_JOBS || s_app.activeModal == TAB_STATUS) {
          cyd_lvgl_open_modal(s_app.activeModal);
        }
        s_app.dirtyUi = true;
      }
    }

    // Core 6-second stat tick (Decoupled C++ Engine)
    if ((now - lastTickMs) >= PET_TICK_INTERVAL_MS) {
      lastTickMs = now;
      s_app.engine.tick();
      s_app.dirtyUi = true;
    }

    // Manual OTA check triggered via UI dot
    if (s_app.triggerOtaRequested) {
      s_app.triggerOtaRequested = false;
      char otaMsg[48] = "";
      cyd_net_check_ota(s_app.config, s_app.engine, on_ota_progress, otaMsg, sizeof(otaMsg));
      if (strlen(otaMsg) > 0) cyd_lvgl_show_banner(otaMsg, 3000);
      s_app.dirtyUi = true;
    }

    // Manual NVS + Cloud Save triggered via Status tab
    if (s_app.triggerSaveRequested) {
      s_app.triggerSaveRequested = false;
      cyd_storage_save_pet(s_app.engine, s_app.recipesCrafted, s_app.jobsCompleted, s_app.spellsCast, s_app.treatsSent);
      if (!s_app.offlineMode) {
        cyd_net_cloud_save(s_app.config, s_app.engine);
      }
      cyd_lvgl_show_banner("Lokal & Cloud gespeichert!", 2500);
    }

    // Periodic PHP Server sync (10s)
    if (!s_app.offlineMode && (now - lastSyncMs) >= SERVER_SYNC_INTERVAL_MS) {
      lastSyncMs = now;
      if (cyd_net_sync_php_server(s_app.config, s_app.engine, s_app.friendPet)) {
        s_app.dirtyUi = true;
      }
    }

    // Periodic OTA check (15 min)
    if (!s_app.offlineMode && (now - lastOtaMs) >= OTA_CHECK_INTERVAL_MS) {
      lastOtaMs = now;
      char otaMsg[48] = "";
      cyd_net_check_ota(s_app.config, s_app.engine, on_ota_progress, otaMsg, sizeof(otaMsg));
    }

    // Periodic NVS autosave (2 min)
    if ((now - lastSaveMs) >= 120000U) {
      lastSaveMs = now;
      cyd_storage_save_pet(s_app.engine, s_app.recipesCrafted, s_app.jobsCompleted, s_app.spellsCast, s_app.treatsSent);
    }

    // Speech balloon & banner timeouts
    if (s_app.friendPet.balloonUntilMs > 0 && now >= s_app.friendPet.balloonUntilMs) {
      s_app.friendPet.balloonUntilMs = 0;
      s_app.friendPet.speechBalloon[0] = '\0';
      s_app.dirtyUi = true;
    }
    if (s_app.bannerUntilMs > 0 && now >= s_app.bannerUntilMs) {
      s_app.bannerUntilMs = 0;
      s_app.bannerMsg[0] = '\0';
      s_app.dirtyUi = true;
    }

    if (s_app.dirtyUi) {
      s_app.dirtyUi = false;
      cyd_lvgl_refresh_main_screen();
    }

    lv_timer_handler();
    cyd_delay_ms(15);
  }
}
