#ifndef CYD_LVGL_UI_H
#define CYD_LVGL_UI_H

#include "lvgl.h"
#include "cyd_display.h"
#include "cyd_net.h"
#include "cyd_storage.h"
#include "../TamagotchiCoreEngine.h"

enum ActiveScreenMode {
  UI_SCREEN_BOOT = 0,
  UI_SCREEN_MAIN,
  UI_SCREEN_AP_PORTAL,
  UI_SCREEN_SERVER_ERR,
  UI_SCREEN_OTA
};

enum SubModalTab {
  TAB_NONE = 0,
  TAB_STATUS,
  TAB_BAG,
  TAB_JOBS,
  TAB_MAGIC,
  TAB_QUEST,
  TAB_CHAT,
  TAB_TRADE
};

struct ActiveJobState {
  bool running;
  uint8_t jobIndex; // 0 = Forager (60s), 1 = Scout (120s), 2 = Miner (180s)
  uint32_t startedMs;
  uint32_t durationMs;
};

struct OutgoingTradeState {
  bool active;
  char tradeId[24];
  char offeredId[20];
  char offeredName[28];
  char requestedId[20];
  char requestedName[28];
};

struct IncomingTradeState {
  bool active;
  char fromDevice[24];
  char tradeId[24];
  char offeredId[20];
  char offeredName[28];
  char requestedId[20];
  char requestedName[28];
};

struct AppRuntimeState {
  CydConfig config;
  TamagotchiCoreEngine engine;
  FriendState friendPet;
  ActiveJobState activeJob;

  OutgoingTradeState outgoingTrade;
  IncomingTradeState incomingTrade;
  uint8_t tradeSelectedWantIdx;

  uint32_t dualCastUntilMs;       // Local device pressed MGC+ waiting up to 10s
  uint32_t friendDualCastUntilMs; // Partner device pressed MGC+ within last 10s

  uint16_t recipesCrafted;
  uint16_t jobsCompleted;
  uint16_t spellsCast;
  uint16_t treatsSent;

  bool offlineMode;
  bool apCancelRequested;
  bool retryPortalRequested;
  bool triggerOtaRequested;
  bool triggerSaveRequested;
  bool triggerCloudLoadRequested;
  bool dirtyUi;

  SubModalTab activeModal;
  bool feedFlyoutOpen;

  char bannerMsg[64];
  uint32_t bannerUntilMs;
};

bool cyd_lvgl_init(CydDisplay* display);
void cyd_lvgl_set_app_state(AppRuntimeState* state);

void cyd_lvgl_show_boot_screen(const char* ssid);
void cyd_lvgl_show_ap_screen(void);
void cyd_lvgl_show_server_error_screen(const char* ipStr, const char* serverUrl);
void cyd_lvgl_show_ota_progress(const char* stage, int percent);
void cyd_lvgl_show_main_screen(void);
void cyd_lvgl_refresh_main_screen(void);
void cyd_lvgl_show_banner(const char* msg, uint32_t durationMs);

void cyd_lvgl_open_modal(SubModalTab tab);
void cyd_lvgl_close_modal(void);

#endif // CYD_LVGL_UI_H
