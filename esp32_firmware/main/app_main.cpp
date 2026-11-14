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

static void handle_net_packet(const NetPacket& pkt, const char* viaMode) {
  if (strlen(pkt.type) == 0) return;

  if (viaMode && strlen(viaMode) > 0) {
    if (strcmp(viaMode, "LAN") == 0 || strcmp(s_app.friendPet.linkMode, "LAN") != 0) {
      strncpy(s_app.friendPet.linkMode, viaMode, sizeof(s_app.friendPet.linkMode) - 1);
      s_app.friendPet.linkMode[sizeof(s_app.friendPet.linkMode) - 1] = '\0';
    }
  }

  if (strcmp(pkt.type, "TREAT") == 0) {
    cyd_sound_treat_received();
    s_app.engine.receiveRemoteTreat();
    cyd_lvgl_show_banner("Freund hat Snack gesendet!", 3000);
  } else if (strcmp(pkt.type, "CHAT") == 0) {
    cyd_sound_treat_received();
    if (strlen(pkt.text) > 0) {
      strncpy(s_app.friendPet.speechBalloon, pkt.text, sizeof(s_app.friendPet.speechBalloon) - 1);
      s_app.friendPet.speechBalloon[sizeof(s_app.friendPet.speechBalloon) - 1] = '\0';
      s_app.friendPet.balloonUntilMs = cyd_millis() + 6000;
    }
  } else if (strcmp(pkt.type, "MAGIC") == 0) {
    cyd_sound_treat_received();
    s_app.engine.stats.health = (uint8_t)engineMin(100, s_app.engine.stats.health + 30);
    s_app.engine.stats.happiness = (uint8_t)engineMin(100, s_app.engine.stats.happiness + 25);
    cyd_lvgl_show_banner("Freund-Segen erhalten!", 3000);
  } else if (strcmp(pkt.type, "MAGIC_SYNC") == 0) {
    uint32_t now = cyd_millis();
    if (s_app.dualCastUntilMs > 0 && now <= s_app.dualCastUntilMs) {
      s_app.dualCastUntilMs = 0;
      s_app.friendDualCastUntilMs = 0;
      s_app.engine.stats.health = (uint8_t)engineMin(100, s_app.engine.stats.health + 50);
      s_app.engine.stats.happiness = (uint8_t)engineMin(100, s_app.engine.stats.happiness + 40);
      s_app.engine.addXP(100);
      s_app.engine.addKnownItemById("food_berry_pie", 1);
      cyd_sound_level_up();
      cyd_lvgl_show_banner("ASTRALE HARMONIE! +100XP +Tarte!", 3500);
    } else {
      s_app.friendDualCastUntilMs = now + 10000U;
      cyd_sound_treat_received();
      cyd_lvgl_show_banner("Freund ruft Harmonie! Jetzt MGC+!", 3500);
    }
  } else if (strcmp(pkt.type, "QUEST") == 0) {
    cyd_sound_treat_received();
    s_app.engine.coins += 20;
    s_app.engine.addXP(25);
    cyd_lvgl_show_banner("Koop-Quest vom Freund! +20$", 3000);
  } else if (strcmp(pkt.type, "TRADE") == 0) {
    cyd_sound_treat_received();
    cyd_lvgl_show_banner("Tausch-Angebot erhalten!", 3000);
  } else if (strcmp(pkt.type, "TRADE_OFFER") == 0) {
    s_app.incomingTrade.active = true;
    const char* fromSrc = strlen(pkt.from) > 0 ? pkt.from : s_app.friendPet.deviceId;
    strncpy(s_app.incomingTrade.fromDevice, fromSrc, sizeof(s_app.incomingTrade.fromDevice) - 1);
    s_app.incomingTrade.fromDevice[sizeof(s_app.incomingTrade.fromDevice) - 1] = '\0';
    strncpy(s_app.incomingTrade.tradeId, pkt.tradeId, sizeof(s_app.incomingTrade.tradeId) - 1);
    s_app.incomingTrade.tradeId[sizeof(s_app.incomingTrade.tradeId) - 1] = '\0';
    strncpy(s_app.incomingTrade.offeredId, pkt.offeredId, sizeof(s_app.incomingTrade.offeredId) - 1);
    s_app.incomingTrade.offeredId[sizeof(s_app.incomingTrade.offeredId) - 1] = '\0';
    const char* offName = strlen(pkt.offeredName) > 0 ? pkt.offeredName : pkt.offeredId;
    strncpy(s_app.incomingTrade.offeredName, offName, sizeof(s_app.incomingTrade.offeredName) - 1);
    s_app.incomingTrade.offeredName[sizeof(s_app.incomingTrade.offeredName) - 1] = '\0';
    strncpy(s_app.incomingTrade.requestedId, pkt.requestedId, sizeof(s_app.incomingTrade.requestedId) - 1);
    s_app.incomingTrade.requestedId[sizeof(s_app.incomingTrade.requestedId) - 1] = '\0';
    const char* reqName = strlen(pkt.requestedName) > 0 ? pkt.requestedName : "Geschenk";
    strncpy(s_app.incomingTrade.requestedName, reqName, sizeof(s_app.incomingTrade.requestedName) - 1);
    s_app.incomingTrade.requestedName[sizeof(s_app.incomingTrade.requestedName) - 1] = '\0';
    cyd_sound_treat_received();
    cyd_lvgl_show_banner("Tausch-Angebot! TRADE druecken!", 3500);
    if (s_app.activeModal == TAB_TRADE) {
      cyd_lvgl_open_modal(TAB_TRADE);
    }
  } else if (strcmp(pkt.type, "TRADE_ACCEPT") == 0) {
    if (s_app.outgoingTrade.active &&
        (strlen(pkt.tradeId) == 0 || strcmp(pkt.tradeId, s_app.outgoingTrade.tradeId) == 0)) {
      const char* recvId = strlen(pkt.requestedId) > 0 ? pkt.requestedId : s_app.outgoingTrade.requestedId;
      if (recvId && strlen(recvId) > 0) {
        s_app.engine.addKnownItemById(recvId, 1);
      }
      s_app.engine.addXP(35);
      memset(&s_app.outgoingTrade, 0, sizeof(s_app.outgoingTrade));
      cyd_sound_level_up();
      cyd_lvgl_show_banner("Tausch erfolgreich! +35XP", 3200);
      if (s_app.activeModal == TAB_TRADE) {
        cyd_lvgl_open_modal(TAB_TRADE);
      }
    }
  } else if (strcmp(pkt.type, "TRADE_DECLINE") == 0) {
    if (s_app.outgoingTrade.active &&
        (strlen(pkt.tradeId) == 0 || strcmp(pkt.tradeId, s_app.outgoingTrade.tradeId) == 0)) {
      s_app.engine.addKnownItemById(s_app.outgoingTrade.offeredId, 1);
      memset(&s_app.outgoingTrade, 0, sizeof(s_app.outgoingTrade));
      cyd_sound_warning();
      cyd_lvgl_show_banner("Tausch abgelehnt (zurueck)", 3000);
      if (s_app.activeModal == TAB_TRADE) {
        cyd_lvgl_open_modal(TAB_TRADE);
      }
    } else if (s_app.incomingTrade.active &&
               (strlen(pkt.tradeId) == 0 || strcmp(pkt.tradeId, s_app.incomingTrade.tradeId) == 0)) {
      memset(&s_app.incomingTrade, 0, sizeof(s_app.incomingTrade));
      cyd_lvgl_show_banner("Tausch abgebrochen.", 2500);
      if (s_app.activeModal == TAB_TRADE) {
        cyd_lvgl_open_modal(TAB_TRADE);
      }
    }
  }
  s_app.dirtyUi = true;
}

static void on_lan_packet(const NetPacket& pkt) {
  handle_net_packet(pkt, "LAN");
}

static void on_relay_packet(const NetPacket& pkt) {
  handle_net_packet(pkt, "NET");
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
  strcpy(s_app.friendPet.linkMode, "OFF");
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
  s_app.friendPet.hunger = 80;
  s_app.friendPet.port = SOCKET_PORT;
  strcpy(s_app.friendPet.linkMode, "OFF");
  strcpy(s_app.friendPet.speechBalloon, "Hello Buddy!");

  cyd_storage_load_rules(s_app.engine);
  cyd_storage_load_pet(s_app.engine, s_app.recipesCrafted, s_app.jobsCompleted, s_app.spellsCast, s_app.treatsSent);
  cyd_lvgl_set_app_state(&s_app);

  // Validate current A/B OTA partition after successful init
  cyd_net_mark_ota_valid(nullptr);

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
          strcpy(s_app.friendPet.linkMode, "OFF");
          cyd_set_rgb(true, false, false);
          cyd_lvgl_show_main_screen();
        }
      } else {
        cyd_net_mark_ota_valid(&s_app.config);
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
  uint32_t lastTickMs  = cyd_millis();
  uint32_t lastRelayMs = cyd_millis();
  uint32_t lastSyncMs  = cyd_millis();
  uint32_t lastOtaMs   = cyd_millis();
  uint32_t lastSaveMs  = cyd_millis();
  uint32_t lastJobUiMs = cyd_millis();

  while (true) {
    uint32_t now = cyd_millis();

    // Incoming LAN socket messages + PHP relay polling (3s)
    if (!s_app.offlineMode) {
      cyd_net_poll_p2p_socket(on_lan_packet);
      if ((now - lastRelayMs) >= 3000U) {
        lastRelayMs = now;
        cyd_net_poll_relay(s_app.config, on_relay_packet);
      }
    }

    // Active job countdown & completion
    if (s_app.activeJob.running) {
      if ((now - s_app.activeJob.startedMs) >= s_app.activeJob.durationMs) {
        s_app.activeJob.running = false;
        uint8_t jIdx = s_app.activeJob.jobIndex;
        uint16_t cGain = 20;
        uint16_t xGain = 25;
        if (jIdx < s_app.engine.rules.jobCount) {
          const EngineJob& job = s_app.engine.rules.jobs[jIdx];
          cGain = job.coinReward;
          xGain = job.xpReward;
          if (strlen(job.matRewardId) > 0 && job.matRewardQty > 0) {
            s_app.engine.addKnownItemById(job.matRewardId, job.matRewardQty);
          }
          if (strlen(job.matReward2Id) > 0 && job.matReward2Qty > 0) {
            s_app.engine.addKnownItemById(job.matReward2Id, job.matReward2Qty);
          }
          if (strlen(job.foodRewardId) > 0 && job.foodRewardQty > 0) {
            s_app.engine.addKnownItemById(job.foodRewardId, job.foodRewardQty);
          }
        }
        s_app.engine.coins += cGain;
        s_app.engine.addXP(xGain);
        s_app.jobsCompleted++;
        cyd_sound_level_up();
        char bMsg[48];
        snprintf(bMsg, sizeof(bMsg), "Beruf fertig! +%u$ +%uXP", (unsigned)cGain, (unsigned)xGain);
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
        bool ok = cyd_net_cloud_save(
          s_app.config, s_app.engine,
          s_app.recipesCrafted, s_app.jobsCompleted, s_app.spellsCast, s_app.treatsSent
        );
        cyd_lvgl_show_banner(ok ? "Lokal & Cloud gespeichert!" : "Lokal gespeichert (Cloud Err)", 2500);
      } else {
        cyd_lvgl_show_banner("Lokal im Flash gespeichert!", 2500);
      }
    }

    // Manual Cloud Restore triggered via Status tab
    if (s_app.triggerCloudLoadRequested) {
      s_app.triggerCloudLoadRequested = false;
      if (!s_app.offlineMode &&
          cyd_net_cloud_load(
            s_app.config, s_app.engine,
            s_app.recipesCrafted, s_app.jobsCompleted, s_app.spellsCast, s_app.treatsSent)) {
        cyd_storage_save_pet(s_app.engine, s_app.recipesCrafted, s_app.jobsCompleted, s_app.spellsCast, s_app.treatsSent);
        cyd_sound_level_up();
        cyd_lvgl_show_banner("Cloud-Stand geladen!", 2800);
      } else {
        cyd_sound_warning();
        cyd_lvgl_show_banner("Cloud-Stand nicht gefunden!", 2500);
      }
      s_app.dirtyUi = true;
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
