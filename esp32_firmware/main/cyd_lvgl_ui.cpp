#include "cyd_lvgl_ui.h"
#include "cyd_hw.h"
#include "cyd_touch.h"
#include "../config.h"
#include "../cyd_config.h"
#include "../PetGraphics.h"

#include "esp_timer.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ============================================================================
// BILINGUAL RULE NAME SELECTION & LVGL ASCII/UMLAUT SANITIZER
// ============================================================================
#if LANG_DE
  #define PICK_RULE_NAME(obj) (strlen((obj).name_de) > 0 ? (obj).name_de : (obj).name)
#else
  #define PICK_RULE_NAME(obj) (strlen((obj).name) > 0 ? (obj).name : (obj).name_de)
#endif

static void sanitize_lvgl_text(const char* src, char* dst, size_t maxLen) {
  if (!dst || maxLen == 0) return;
  if (!src) { dst[0] = '\0'; return; }
  size_t out = 0;
  const unsigned char* p = (const unsigned char*)src;
  while (*p && out + 1 < maxLen) {
    unsigned char c = *p;
    if (c == 0xC3 && p[1] != '\0') {
      unsigned char c2 = p[1];
      const char* rep = nullptr;
      if (c2 == 0xA4) rep = "ae";
      else if (c2 == 0xB6) rep = "oe";
      else if (c2 == 0xBC) rep = "ue";
      else if (c2 == 0x84) rep = "Ae";
      else if (c2 == 0x96) rep = "Oe";
      else if (c2 == 0x9C) rep = "Ue";
      else if (c2 == 0x9F) rep = "ss";
      if (rep) {
        if (out + 2 < maxLen) {
          dst[out++] = rep[0];
          dst[out++] = rep[1];
        }
        p += 2;
        continue;
      }
      p += 2;
      continue;
    }
    if (c >= 0xC0) {
      // Skip multi-byte UTF-8 glyphs/emojis unsupported by ASCII Montserrat font
      p++;
      while (*p && ((*p & 0xC0) == 0x80)) p++;
      continue;
    }
    if (c == '\n' || c == '\t' || (c >= 0x20 && c <= 0x7E)) {
      dst[out++] = (char)c;
    }
    p++;
  }
  dst[out] = '\0';
}

// ============================================================================
// BILINGUAL UI TEXT MACROS (EN / DE)
// ============================================================================
#if LANG_DE
  #define TXT_FEED       "FUTT"
  #define TXT_PLAY       "SPIEL"
  #define TXT_BATH       "BAD"
  #define TXT_MAGIC      "ZAUB"
  #define TXT_SAVE       "STATUS"
  #define TXT_TREAT      "SNACK"
  #define TXT_CHAT       "CHAT"
  #define TXT_MAGIC_PLUS "MGC+"
  #define TXT_QUEST      "QUEST"
  #define TXT_TRADE      "TAUSCH"
  #define TXT_ONLINE     "ONLINE"
  #define TXT_ASLEEP     "SCHLAFT"
  #define TXT_TAP_EGG    "TIPPEN!"
  #define TXT_SICK_BADGE "KRANK!"
  #define TXT_BANNER_PLAY            "Gespielt! +20 Freude"
  #define TXT_BANNER_BATH            "Sauber gebadet!"
  #define TXT_BANNER_AURA            "Heilaura gewirkt!"
  #define TXT_BANNER_NO_MANA         "Zu wenig Mana!"
  #define TXT_BANNER_SAVED           "Gespeichert & Cloud!"
  #define TXT_BANNER_TREAT_SENT      "Snack gesendet!"
  #define TXT_BANNER_CHAT_SENT       "Chat gesendet!"
  #define TXT_BANNER_BLESSING        "Segen gesendet!"
  #define TXT_BANNER_TRADE           "Tausch gesendet!"
  #define TXT_BANNER_EGG_HATCHED     "EI GESCHLUEPFT! Baby da!"
  #define TXT_BANNER_EGG_WARMED      "Ei gewaermt! Tippen!"
  #define TXT_BANNER_CUDDLED         "Gestreichelt! +5"
  #define L_TAB_STAT   "Stat"
  #define L_TAB_BAG    "Tasche"
  #define L_TAB_JOBS   "Berufe"
  #define L_TAB_MAGIC  "Magie"
  #define L_TAB_QUEST  "Quest"
  #define L_CLOSE            "X"
  #define L_BAG_EMPTY        "Leer! Shop/Job nutzen"
  #define L_BTN_CLOUD_SAVE   "In Flash & Cloud speichern"
  #define L_BTN_CLOUD_LOAD   "Stand aus Cloud laden"
  #define L_CLD_OFFLINE      "Cloud: Offline-Modus"
  #define L_CLD_SYNCED       "Cloud: Synchron (vor %lus)"
  #define L_CLD_SAVED_NVS    "Cloud: Backup # %lu bereit"
  #define L_CLD_NONE         "Cloud: Noch kein Backup"
  #define L_BTN_SLEEP_ON     "Schlaf-Modus aktivieren (+EN)"
  #define L_BTN_SLEEP_OFF    "Pet aufwecken (Wach)"
  #define L_TAG_FOOD         "[Essen]"
  #define L_TAG_USE          "[Nutzen]"
  #define L_TAG_MAT          "[Zutat]"
  #define L_TAG_EQUIP        "[Anlegen]"
  #define L_TAG_EQUIPPED     "[Angelegt]"
#else
  #define TXT_FEED       "FEED"
  #define TXT_PLAY       "PLAY"
  #define TXT_BATH       "BATH"
  #define TXT_MAGIC      "MAGIC"
  #define TXT_SAVE       "STATUS"
  #define TXT_TREAT      "TREAT"
  #define TXT_CHAT       "CHAT"
  #define TXT_MAGIC_PLUS "MGC+"
  #define TXT_QUEST      "QUEST"
  #define TXT_TRADE      "TRADE"
  #define TXT_ONLINE     "ONLINE"
  #define TXT_ASLEEP     "ASLEEP"
  #define TXT_TAP_EGG    "TAP EGG!"
  #define TXT_SICK_BADGE "SICK!"
  #define TXT_BANNER_PLAY            "Played! +20 Happy"
  #define TXT_BANNER_BATH            "Clean Bath! +10 Happy"
  #define TXT_BANNER_AURA            "Vitality Aura Cast!"
  #define TXT_BANNER_NO_MANA         "Not enough Mana!"
  #define TXT_BANNER_SAVED           "Saved locally & Cloud!"
  #define TXT_BANNER_TREAT_SENT      "Treat Sent to Friend!"
  #define TXT_BANNER_CHAT_SENT       "Chat Beamed!"
  #define TXT_BANNER_BLESSING        "Blessing Sent!"
  #define TXT_BANNER_TRADE           "Trade Proposal Sent!"
  #define TXT_BANNER_EGG_HATCHED     "EGG HATCHED! Baby Born!"
  #define TXT_BANNER_EGG_WARMED      "Egg Warmed! Keep Tapping"
  #define TXT_BANNER_CUDDLED         "Pet Cuddled! +5"
  #define L_TAB_STAT   "Status"
  #define L_TAB_BAG    "Bag"
  #define L_TAB_JOBS   "Jobs"
  #define L_TAB_MAGIC  "Magic"
  #define L_TAB_QUEST  "Quest"
  #define L_CLOSE            "X"
  #define L_BAG_EMPTY        "Empty! Try Shop/Jobs"
  #define L_BTN_CLOUD_SAVE   "Save Flash & Cloud"
  #define L_BTN_CLOUD_LOAD   "Restore from Cloud"
  #define L_CLD_OFFLINE      "Cloud: Offline mode"
  #define L_CLD_SYNCED       "Cloud: Synced (%lus ago)"
  #define L_CLD_SAVED_NVS    "Cloud: Backup #%lu ready"
  #define L_CLD_NONE         "Cloud: No backup yet"
  #define L_BTN_SLEEP_ON     "Sleep Mode (+Energy regen)"
  #define L_BTN_SLEEP_OFF    "Wake Pet Up"
  #define L_TAG_FOOD         "[Eat]"
  #define L_TAG_USE          "[Use]"
  #define L_TAG_MAT          "[Mat]"
  #define L_TAG_EQUIP        "[Equip]"
  #define L_TAG_EQUIPPED     "[Equipped]"
#endif

// ============================================================================
// LVGL DISPLAY & TOUCH HAL STATE
// ============================================================================
static CydDisplay* s_disp = nullptr;
static AppRuntimeState* s_app = nullptr;

static lv_disp_draw_buf_t s_draw_buf;
static lv_color_t s_buf1[BOARD_DISPLAY_WIDTH * BOARD_DISPLAY_BUFFER_ROWS];
static lv_disp_drv_t s_disp_drv;
static lv_indev_drv_t s_indev_drv;
static esp_timer_handle_t s_lvgl_tick_timer = nullptr;

static inline lv_color_t rgb565_to_lv(uint16_t c) {
  lv_color_t out;
  out.full = (uint16_t)((c >> 8) | (c << 8));
  return out;
}

static void lvgl_tick_cb(void* /*arg*/) {
  lv_tick_inc(5);
}

static void lvgl_flush_cb(lv_disp_drv_t* drv, const lv_area_t* area, lv_color_t* color_p) {
  if (s_disp) {
    s_disp->flushRgb565Rect(area->x1, area->y1, area->x2, area->y2, (const uint16_t*)color_p, true);
  }
  lv_disp_flush_ready(drv);
}

static void lvgl_touch_read_cb(lv_indev_drv_t* /*drv*/, lv_indev_data_t* data) {
  uint16_t x = 0, y = 0;
  if (cyd_touch_read(&x, &y)) {
    data->state = LV_INDEV_STATE_PR;
    data->point.x = (lv_coord_t)x;
    data->point.y = (lv_coord_t)y;
  } else {
    data->state = LV_INDEV_STATE_REL;
  }
}

// ============================================================================
// 16x16 PIXEL SPRITE -> LVGL IMAGE DESCRIPTOR TABLE
// ============================================================================
enum SpriteIconId {
  SPR_FEED = 0, SPR_PLAY, SPR_BATH, SPR_MAGIC, SPR_STATS,
  SPR_TREAT, SPR_CHAT, SPR_MGC_PLUS, SPR_QUEST, SPR_TRADE,
  SPR_APPLE, SPR_SNACK, SPR_SALAD, SPR_PIE, SPR_JERKY, SPR_STEW,
  SPR_COUNT
};

static uint16_t s_sprite_pixels[SPR_COUNT][16 * 16];
static lv_img_dsc_t s_sprite_dsc[SPR_COUNT];
static bool s_sprites_built = false;

static void build_sprite_dsc(SpriteIconId id, const char* const rows[16]) {
  for (int y = 0; y < 16; y++) {
    const char* row = rows[y];
    for (int x = 0; x < 16; x++) {
      char ch = row[x];
      if (ch == '\0') ch = '.';
      uint16_t col = (ch == '.') ? 0x0000 : mapSpriteColor(ch);
      s_sprite_pixels[id][y * 16 + x] = (uint16_t)((col >> 8) | (col << 8));
    }
  }
  s_sprite_dsc[id].header.always_zero = 0;
  s_sprite_dsc[id].header.w = 16;
  s_sprite_dsc[id].header.h = 16;
  s_sprite_dsc[id].header.cf = LV_IMG_CF_TRUE_COLOR_CHROMA_KEYED;
  s_sprite_dsc[id].data_size = 16 * 16 * 2;
  s_sprite_dsc[id].data = (const uint8_t*)s_sprite_pixels[id];
}

static void init_all_sprites(void) {
  if (s_sprites_built) return;
  s_sprites_built = true;
  build_sprite_dsc(SPR_FEED,     SPR_BTN_FEED);
  build_sprite_dsc(SPR_PLAY,     SPR_BTN_PLAY);
  build_sprite_dsc(SPR_BATH,     SPR_BTN_BATH);
  build_sprite_dsc(SPR_MAGIC,    SPR_BTN_MAGIC);
  build_sprite_dsc(SPR_STATS,    SPR_BTN_STATS);
  build_sprite_dsc(SPR_TREAT,    SPR_BTN_TREAT);
  build_sprite_dsc(SPR_CHAT,     SPR_BTN_CHAT);
  build_sprite_dsc(SPR_MGC_PLUS, SPR_BTN_MGC_PLUS);
  build_sprite_dsc(SPR_QUEST,    SPR_BTN_QUEST);
  build_sprite_dsc(SPR_TRADE,    SPR_BTN_TRADE);
  build_sprite_dsc(SPR_APPLE,    SPR_FOOD_APPLE);
  build_sprite_dsc(SPR_SNACK,    SPR_FOOD_SNACK);
  build_sprite_dsc(SPR_SALAD,    SPR_FOOD_SALAD);
  build_sprite_dsc(SPR_PIE,      SPR_FOOD_PIE);
  build_sprite_dsc(SPR_JERKY,    SPR_FOOD_JERKY);
  build_sprite_dsc(SPR_STEW,     SPR_FOOD_STEW);
}

static const lv_img_dsc_t* get_food_sprite_dsc(const char* foodId) {
  if (strcmp(foodId, "food_apple") == 0) return &s_sprite_dsc[SPR_APPLE];
  if (strcmp(foodId, "food_snack") == 0) return &s_sprite_dsc[SPR_SNACK];
  if (strcmp(foodId, "food_herb_salad") == 0) return &s_sprite_dsc[SPR_SALAD];
  if (strcmp(foodId, "food_berry_pie") == 0) return &s_sprite_dsc[SPR_PIE];
  if (strcmp(foodId, "food_sun_jerky") == 0) return &s_sprite_dsc[SPR_JERKY];
  return &s_sprite_dsc[SPR_STEW];
}

// ============================================================================
// FAST DIRECT CANVAS ADAPTER FOR PetGraphics.h (Egg & Critter Art)
// ============================================================================
#define PET_CANVAS_W 76
#define PET_CANVAS_H 84

static uint16_t s_my_pet_buf[PET_CANVAS_W * PET_CANVAS_H];
static uint16_t s_fr_pet_buf[PET_CANVAS_W * PET_CANVAS_H];

class LvCanvasDrawAdapter {
public:
  LvCanvasDrawAdapter(uint16_t* buf, int w, int h) : _buf(buf), _w(w), _h(h) {}

  void clear(uint16_t bg565) {
    uint16_t be = (uint16_t)((bg565 >> 8) | (bg565 << 8));
    for (int i = 0; i < _w * _h; i++) _buf[i] = be;
  }

  void drawPixel(int x, int y, uint16_t color) {
    if (x < 0 || y < 0 || x >= _w || y >= _h) return;
    _buf[y * _w + x] = (uint16_t)((color >> 8) | (color << 8));
  }

  void drawFastHLine(int x, int y, int w, uint16_t color) {
    if (y < 0 || y >= _h || w <= 0) return;
    int x0 = (x < 0) ? 0 : x;
    int x1 = (x + w > _w) ? _w : (x + w);
    uint16_t be = (uint16_t)((color >> 8) | (color << 8));
    for (int xx = x0; xx < x1; xx++) _buf[y * _w + xx] = be;
  }

  void drawFastVLine(int x, int y, int h, uint16_t color) {
    if (x < 0 || x >= _w || h <= 0) return;
    int y0 = (y < 0) ? 0 : y;
    int y1 = (y + h > _h) ? _h : (y + h);
    uint16_t be = (uint16_t)((color >> 8) | (color << 8));
    for (int yy = y0; yy < y1; yy++) _buf[yy * _w + x] = be;
  }

  void fillRect(int x, int y, int w, int h, uint16_t color) {
    for (int yy = 0; yy < h; yy++) drawFastHLine(x, y + yy, w, color);
  }

  void drawRect(int x, int y, int w, int h, uint16_t color) {
    drawFastHLine(x, y, w, color);
    drawFastHLine(x, y + h - 1, w, color);
    drawFastVLine(x, y, h, color);
    drawFastVLine(x + w - 1, y, h, color);
  }

  void drawLine(int x0, int y0, int x1, int y1, uint16_t color) {
    int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;
    while (true) {
      drawPixel(x0, y0, color);
      if (x0 == x1 && y0 == y1) break;
      int e2 = 2 * err;
      if (e2 >= dy) { err += dy; x0 += sx; }
      if (e2 <= dx) { err += dx; y0 += sy; }
    }
  }

  void fillCircle(int x0, int y0, int r, uint16_t color) {
    for (int dy = -r; dy <= r; dy++) {
      for (int dx = -r; dx <= r; dx++) {
        if (dx * dx + dy * dy <= r * r) drawPixel(x0 + dx, y0 + dy, color);
      }
    }
  }

  void drawCircle(int x0, int y0, int r, uint16_t color) {
    int f = 1 - r, ddF_x = 1, ddF_y = -2 * r, x = 0, y = r;
    drawPixel(x0, y0 + r, color); drawPixel(x0, y0 - r, color);
    drawPixel(x0 + r, y0, color); drawPixel(x0 - r, y0, color);
    while (x < y) {
      if (f >= 0) { y--; ddF_y += 2; f += ddF_y; }
      x++; ddF_x += 2; f += ddF_x;
      drawPixel(x0 + x, y0 + y, color); drawPixel(x0 - x, y0 + y, color);
      drawPixel(x0 + x, y0 - y, color); drawPixel(x0 - x, y0 - y, color);
      drawPixel(x0 + y, y0 + x, color); drawPixel(x0 - y, y0 + x, color);
      drawPixel(x0 + y, y0 - x, color); drawPixel(x0 - y, y0 - x, color);
    }
  }

  void fillEllipse(int x0, int y0, int rx, int ry, uint16_t color) {
    if (rx < 1 || ry < 1) return;
    int32_t rx2 = rx * rx, ry2 = ry * ry;
    for (int dy = -ry; dy <= ry; dy++) {
      int32_t num = rx2 * (ry2 - dy * dy);
      int dx = 0;
      while ((dx + 1) * (dx + 1) * ry2 <= num) dx++;
      drawFastHLine(x0 - dx, y0 + dy, 2 * dx + 1, color);
    }
  }

  void drawEllipse(int x0, int y0, int rx, int ry, uint16_t color) {
    if (rx < 1 || ry < 1) return;
    for (int a = 0; a < 90; a++) {
      float rad = a * (3.14159f / 180.0f);
      int dx = (int)(cosf(rad) * rx + 0.5f);
      int dy = (int)(sinf(rad) * ry + 0.5f);
      drawPixel(x0 + dx, y0 + dy, color); drawPixel(x0 - dx, y0 + dy, color);
      drawPixel(x0 + dx, y0 - dy, color); drawPixel(x0 - dx, y0 - dy, color);
    }
  }

  void fillRoundRect(int x, int y, int w, int h, int r, uint16_t color) {
    fillRect(x, y, w, h, color);
    fillCircle(x + r, y + r, r, color);
    fillCircle(x + w - r - 1, y + r, r, color);
  }

  void drawRoundRect(int x, int y, int w, int h, int r, uint16_t color) {
    drawRect(x, y, w, h, color);
  }

  void fillTriangle(int x0, int y0, int x1, int y1, int x2, int y2, uint16_t color) {
    int minX = (x0 < x1 ? (x0 < x2 ? x0 : x2) : (x1 < x2 ? x1 : x2));
    int maxX = (x0 > x1 ? (x0 > x2 ? x0 : x2) : (x1 > x2 ? x1 : x2));
    int minY = (y0 < y1 ? (y0 < y2 ? y0 : y2) : (y1 < y2 ? y1 : y2));
    int maxY = (y0 > y1 ? (y0 > y2 ? y0 : y2) : (y1 > y2 ? y1 : y2));
    for (int py = minY; py <= maxY; py++) {
      for (int px = minX; px <= maxX; px++) {
        int d1 = (px - x1) * (y0 - y1) - (x0 - x1) * (py - y1);
        int d2 = (px - x2) * (y1 - y2) - (x1 - x2) * (py - y2);
        int d3 = (px - x0) * (y2 - y0) - (x2 - x0) * (py - y0);
        if (!(((d1 < 0) || (d2 < 0) || (d3 < 0)) && ((d1 > 0) || (d2 > 0) || (d3 > 0)))) {
          drawPixel(px, py, color);
        }
      }
    }
  }

  void drawTriangle(int x0, int y0, int x1, int y1, int x2, int y2, uint16_t color) {
    drawLine(x0, y0, x1, y1, color);
    drawLine(x1, y1, x2, y2, color);
    drawLine(x2, y2, x0, y0, color);
  }

  void setTextDatum(uint8_t) {}
  void setTextColor(uint16_t, uint16_t) {}
  void drawString(const char*, int, int, uint8_t = 1) {}

private:
  uint16_t* _buf;
  int _w, _h;
};

// ============================================================================
// MAIN SCREEN WIDGET REFERENCES
// ============================================================================
static lv_obj_t* s_main_scr = nullptr;
static lv_obj_t* s_left_stage = nullptr;
static lv_obj_t* s_right_stage = nullptr;
static lv_obj_t* s_dot_btn = nullptr;

static lv_obj_t* s_my_name_lbl = nullptr;
static lv_obj_t* s_my_lv_lbl = nullptr;
static lv_obj_t* s_my_phase_lbl = nullptr;
static lv_obj_t* s_my_badge_lbl = nullptr;
static lv_obj_t* s_my_hp_bar = nullptr;
static lv_obj_t* s_my_fd_bar = nullptr;
static lv_obj_t* s_my_xp_bar = nullptr;
static lv_obj_t* s_my_canvas = nullptr;
static lv_obj_t* s_my_egg_lbl = nullptr;
static lv_obj_t* s_my_footer_lbl = nullptr;

static lv_obj_t* s_fr_name_lbl = nullptr;
static lv_obj_t* s_fr_lv_lbl = nullptr;
static lv_obj_t* s_fr_status_lbl = nullptr;
static lv_obj_t* s_fr_hp_bar = nullptr;
static lv_obj_t* s_fr_canvas = nullptr;
static lv_obj_t* s_fr_balloon_box = nullptr;
static lv_obj_t* s_fr_balloon_lbl = nullptr;
static lv_obj_t* s_fr_footer_lbl = nullptr;

static lv_obj_t* s_banner_box = nullptr;
static lv_obj_t* s_banner_lbl = nullptr;
static lv_obj_t* s_flyout_bar = nullptr;
static lv_obj_t* s_modal_overlay = nullptr;
static lv_obj_t* s_ota_bar = nullptr;
static lv_obj_t* s_ota_stage_lbl = nullptr;
static lv_obj_t* s_ota_pct_lbl = nullptr;

// ============================================================================
// GAME ACTIONS & CALLBACKS
// ============================================================================
static void check_level_up_notify(uint8_t prevLv) {
  if (!s_app) return;
  if (s_app->engine.level > prevLv) {
    cyd_sound_level_up();
    cyd_lvgl_show_banner(LANG_DE ? "LEVEL UP! Pet entwickelt!" : "LEVEL UP! Pet evolved!", 3000);
  }
}

static void rebuild_feed_flyout(void);

static void on_flyout_food_clicked(lv_event_t* e) {
  if (!s_app) return;
  int slotIdx = (int)(intptr_t)lv_event_get_user_data(e);
  if (slotIdx == -1) {
    s_app->feedFlyoutOpen = false;
    rebuild_feed_flyout();
    cyd_lvgl_open_modal(TAB_BAG);
    return;
  }
  if (slotIdx < 0 || slotIdx >= s_app->engine.inventoryCount) return;

  cyd_sound_click();
  uint8_t prevLv = s_app->engine.level;
  char itemId[20];
  strncpy(itemId, s_app->engine.inventory[slotIdx].id, sizeof(itemId) - 1);
  itemId[sizeof(itemId) - 1] = '\0';

  FeedResult res = s_app->engine.feedWithItem(itemId);
  if (res.success) {
    cyd_sound_munch();
    char msg[64];
    if (res.becameSick) {
      cyd_sound_warning();
      snprintf(msg, sizeof(msg), LANG_DE ? "KRANK! Snack -15HP (+%u FD)" : "SICK! Snack -15HP (+%u FD)", res.hungerGain);
    } else if (res.satietyGained > 0) {
      snprintf(msg, sizeof(msg), LANG_DE ? "+%u FD | Satt %uT!" : "+%u FD | Full %uT!", res.hungerGain, res.satietyGained);
    } else {
      snprintf(msg, sizeof(msg), LANG_DE ? "Gefuettert! +%u FD" : "Fed Pet! +%u FD", res.hungerGain);
    }
    cyd_lvgl_show_banner(msg, 2500);
    check_level_up_notify(prevLv);
  }
  s_app->feedFlyoutOpen = false;
  rebuild_feed_flyout();
  cyd_lvgl_refresh_main_screen();
}

static void rebuild_feed_flyout(void) {
  if (!s_flyout_bar || !s_app) return;
  lv_obj_clean(s_flyout_bar);
  if (!s_app->feedFlyoutOpen) {
    lv_obj_add_flag(s_flyout_bar, LV_OBJ_FLAG_HIDDEN);
    return;
  }
  lv_obj_clear_flag(s_flyout_bar, LV_OBJ_FLAG_HIDDEN);
  lv_obj_move_foreground(s_flyout_bar);

  uint8_t foodIdx[8];
  uint8_t cnt = s_app->engine.getAvailableFoods(foodIdx, 5);
  if (cnt == 0) {
    lv_obj_t* btn = lv_btn_create(s_flyout_bar);
    lv_obj_set_size(btn, 148, 34);
    lv_obj_set_style_bg_color(btn, lv_color_hex(0x7C2D12), 0);
    lv_obj_add_event_cb(btn, on_flyout_food_clicked, LV_EVENT_CLICKED, (void*)(intptr_t)-1);
    lv_obj_t* lbl = lv_label_create(btn);
    lv_label_set_text(lbl, L_BAG_EMPTY);
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_10, 0);
    lv_obj_center(lbl);
    return;
  }

  for (uint8_t i = 0; i < cnt; i++) {
    uint8_t idx = foodIdx[i];
    const EngineInventorySlot& slot = s_app->engine.inventory[idx];
    lv_obj_t* btn = lv_btn_create(s_flyout_bar);
    lv_obj_set_size(btn, 42, 36);
    lv_obj_set_style_bg_color(btn, slot.sickChancePct > 0 ? lv_color_hex(0x451A03) : lv_color_hex(0x064E3B), 0);
    lv_obj_set_style_border_color(btn, lv_color_hex(0xF59E0B), 0);
    lv_obj_set_style_border_width(btn, 1, 0);
    lv_obj_set_style_pad_all(btn, 1, 0);
    lv_obj_add_event_cb(btn, on_flyout_food_clicked, LV_EVENT_CLICKED, (void*)(intptr_t)idx);

    lv_obj_t* img = lv_img_create(btn);
    lv_img_set_src(img, get_food_sprite_dsc(slot.id));
    lv_obj_align(img, LV_ALIGN_TOP_MID, 0, 1);

    lv_obj_t* qLbl = lv_label_create(btn);
    lv_label_set_text_fmt(qLbl, "x%u", slot.quantity);
    lv_obj_set_style_text_font(qLbl, &lv_font_montserrat_8, 0);
    lv_obj_align(qLbl, LV_ALIGN_BOTTOM_MID, 0, -1);
  }
}

static void on_left_btn_clicked(lv_event_t* e) {
  if (!s_app) return;
  int btnId = (int)(intptr_t)lv_event_get_user_data(e);
  cyd_sound_click();
  uint8_t prevLv = s_app->engine.level;

  switch (btnId) {
    case 1: // FEED flyout toggle
      s_app->feedFlyoutOpen = !s_app->feedFlyoutOpen;
      rebuild_feed_flyout();
      return;
    case 2: // PLAY
      s_app->feedFlyoutOpen = false;
      rebuild_feed_flyout();
      s_app->engine.play(20, 10);
      cyd_lvgl_show_banner(TXT_BANNER_PLAY, 2200);
      break;
    case 3: // BATH
      s_app->feedFlyoutOpen = false;
      rebuild_feed_flyout();
      s_app->engine.clean();
      cyd_lvgl_show_banner(TXT_BANNER_BATH, 2200);
      break;
    case 4: // MAGIC Modal
      s_app->feedFlyoutOpen = false;
      rebuild_feed_flyout();
      cyd_lvgl_open_modal(TAB_MAGIC);
      return;
    case 5: // STATUS Modal (5-Tab)
      s_app->feedFlyoutOpen = false;
      rebuild_feed_flyout();
      cyd_lvgl_open_modal(TAB_STATUS);
      return;
  }
  check_level_up_notify(prevLv);
  cyd_lvgl_refresh_main_screen();
}

static void on_right_btn_clicked(lv_event_t* e) {
  if (!s_app) return;
  int btnId = (int)(intptr_t)lv_event_get_user_data(e);
  cyd_sound_click();
  s_app->feedFlyoutOpen = false;
  rebuild_feed_flyout();
  uint8_t prevLv = s_app->engine.level;

  switch (btnId) {
    case 6: // TREAT
      cyd_sound_treat_sent();
      cyd_net_send_packet(s_app->config, s_app->friendPet, "TREAT", "Heart Donut");
      s_app->treatsSent++;
      s_app->engine.addXP(25);
      s_app->engine.recordAction("send_treat", 1);
      cyd_lvgl_show_banner(TXT_BANNER_TREAT_SENT, 2500);
      break;
    case 7: // CHAT Modal
      cyd_lvgl_open_modal(TAB_CHAT);
      return;
    case 8: { // MGC+ (Friend Blessing + 10s Dual Astral Harmony Co-op Window)
      if (s_app->engine.stats.mana >= 30) {
        s_app->engine.stats.mana -= 30;
        uint32_t now = cyd_millis();
        NetPacket pkt = {};
        strncpy(pkt.from, s_app->config.deviceId, sizeof(pkt.from) - 1);
        if (s_app->friendDualCastUntilMs > 0 && now <= s_app->friendDualCastUntilMs) {
          // Both devices pressed MGC+ within 10s -> Dual Astral Harmony!
          s_app->friendDualCastUntilMs = 0;
          s_app->dualCastUntilMs = 0;
          s_app->auroraUntilMs = now + 4000U;
          strncpy(pkt.type, "MAGIC_SYNC", sizeof(pkt.type) - 1);
          strncpy(pkt.text, "Astral Harmony!", sizeof(pkt.text) - 1);
          cyd_net_send_packet_ex(s_app->config, s_app->friendPet, pkt);
          s_app->engine.stats.health = (uint8_t)engineMin(100, s_app->engine.stats.health + 50);
          s_app->engine.stats.happiness = (uint8_t)engineMin(100, s_app->engine.stats.happiness + 40);
          s_app->engine.addXP(100);
          s_app->engine.addKnownItemById("food_berry_pie", 1);
          cyd_sound_level_up();
          cyd_lvgl_show_banner(LANG_DE ? "ASTRALE HARMONIE! +100XP +Tarte!" : "ASTRAL HARMONY! +100XP +Tart!", 3500);
        } else {
          s_app->dualCastUntilMs = now + 10000U;
          strncpy(pkt.type, "MAGIC_SYNC", sizeof(pkt.type) - 1);
          strncpy(pkt.text, "Friend Blessing", sizeof(pkt.text) - 1);
          cyd_net_send_packet_ex(s_app->config, s_app->friendPet, pkt);
          s_app->engine.addXP(40);
          cyd_sound_treat_sent();
          cyd_lvgl_show_banner(LANG_DE ? "Segen! Dual-Cast 10s offen!" : "Blessing! 10s Dual-Cast open!", 2800);
        }
        s_app->spellsCast++;
        s_app->engine.recordAction("magic", 1);
      } else {
        cyd_lvgl_show_banner(TXT_BANNER_NO_MANA, 2000);
      }
      break;
    }
    case 9: // QUEST Modal
      cyd_lvgl_open_modal(TAB_QUEST);
      return;
    case 10: // TRADE Modal
      cyd_lvgl_open_modal(TAB_TRADE);
      return;
  }
  check_level_up_notify(prevLv);
  cyd_lvgl_refresh_main_screen();
}

static void on_pet_stage_tapped(lv_event_t* /*e*/) {
  if (!s_app) return;
  cyd_sound_click();
  if (strcmp(s_app->engine.phase, "Egg") == 0) {
    if (s_app->engine.warmEgg(25)) {
      cyd_sound_hatch();
      cyd_lvgl_show_banner(TXT_BANNER_EGG_HATCHED, 3200);
    } else {
      cyd_lvgl_show_banner(TXT_BANNER_EGG_WARMED, 1800);
    }
  } else {
    s_app->engine.stats.happiness = (uint8_t)engineMin(100, s_app->engine.stats.happiness + 5);
    cyd_lvgl_show_banner(TXT_BANNER_CUDDLED, 1800);
  }
  cyd_lvgl_refresh_main_screen();
}

static void on_dot_ota_clicked(lv_event_t* /*e*/) {
  if (!s_app) return;
  cyd_sound_click();
  s_app->triggerOtaRequested = true;
}

static void on_footer_clicked(lv_event_t* /*e*/) {
  cyd_sound_click();
  cyd_lvgl_open_modal(TAB_QUEST);
}

// ============================================================================
// MODAL SUBSCREEN ACTIONS (Status, Bag/Cook/Shop, Jobs, Magic, Quest, Chat)
// ============================================================================
static void on_modal_tab_switch(lv_event_t* e) {
  cyd_sound_click();
  SubModalTab tab = (SubModalTab)(intptr_t)lv_event_get_user_data(e);
  if (tab == TAB_NONE) {
    cyd_lvgl_close_modal();
  } else {
    cyd_lvgl_open_modal(tab);
  }
}

static void on_bag_action_cb(lv_event_t* e) {
  if (!s_app) return;
  int code = (int)(intptr_t)lv_event_get_user_data(e);
  cyd_sound_click();
  uint8_t prevLv = s_app->engine.level;

  if (code >= 100 && code < 100 + (int)s_app->engine.rules.recipeCount) {
    int rIdx = code - 100;
    const EngineRecipe& r = s_app->engine.rules.recipes[rIdx];
    char rName[28];
    sanitize_lvgl_text(PICK_RULE_NAME(r), rName, sizeof(rName));
    if (s_app->engine.craftRecipe((uint8_t)rIdx)) {
      s_app->recipesCrafted++;
      cyd_sound_munch();
      char msg[56];
      snprintf(msg, sizeof(msg), LANG_DE ? "Gekocht: %s!" : "Cooked: %s!", rName);
      cyd_lvgl_show_banner(msg, 2200);
    } else if (s_app->engine.level < r.minLevel) {
      char msg[40];
      snprintf(msg, sizeof(msg), LANG_DE ? "Braucht Level %u!" : "Needs Level %u!", (unsigned)r.minLevel);
      cyd_lvgl_show_banner(msg, 2000);
    } else {
      cyd_lvgl_show_banner(LANG_DE ? "Zutaten fehlen!" : "Missing ingredients!", 2000);
    }
  } else if (code >= 200 && code < 200 + (int)s_app->engine.rules.shopItemCount) {
    int sIdx = code - 200;
    const EngineShopItem& sh = s_app->engine.rules.shopItems[sIdx];
    char sName[28];
    sanitize_lvgl_text(PICK_RULE_NAME(sh), sName, sizeof(sName));
    bool allowLocal = true;
    if (!s_app->offlineMode) {
      char srvErr[64] = "";
      int srvRes = cyd_net_shop_purchase(
        s_app->config, "item", sh.id,
        s_app->engine.coins, s_app->engine.level,
        srvErr, sizeof(srvErr)
      );
      if (srvRes == 0) {
        allowLocal = false;
        cyd_sound_warning();
        cyd_lvgl_show_banner(strlen(srvErr) > 0 ? srvErr : (LANG_DE ? "Shop abgelehnt!" : "Shop declined!"), 2200);
      }
    }
    if (allowLocal) {
      if (s_app->engine.purchaseItem(sh.id)) {
        cyd_sound_treat_sent();
        char msg[56];
        snprintf(msg, sizeof(msg), "%s (-%u$)", sName, (unsigned)sh.cost);
        cyd_lvgl_show_banner(msg, 1800);
      } else if (s_app->engine.level < sh.minLevel) {
        char msg[40];
        snprintf(msg, sizeof(msg), LANG_DE ? "Braucht Level %u!" : "Needs Level %u!", (unsigned)sh.minLevel);
        cyd_lvgl_show_banner(msg, 1800);
      } else {
        cyd_lvgl_show_banner(LANG_DE ? "Zu wenig Coins!" : "Not enough Coins!", 1800);
      }
    }
  } else if (code >= 300 && code < 300 + (int)s_app->engine.rules.shopRecipeCount) {
    int srIdx = code - 300;
    const EngineShopRecipe& sr = s_app->engine.rules.shopRecipes[srIdx];
    char rName[28];
    sanitize_lvgl_text(PICK_RULE_NAME(sr), rName, sizeof(rName));
    if (s_app->engine.isRecipePurchased(sr.id)) {
      cyd_lvgl_show_banner(LANG_DE ? "Rezept bereits gekauft!" : "Recipe already owned!", 1800);
    } else {
      bool allowLocal = true;
      if (!s_app->offlineMode) {
        char srvErr[64] = "";
        int srvRes = cyd_net_shop_purchase(
          s_app->config, "recipe", sr.id,
          s_app->engine.coins, s_app->engine.level,
          srvErr, sizeof(srvErr)
        );
        if (srvRes == 0) {
          allowLocal = false;
          cyd_sound_warning();
          cyd_lvgl_show_banner(strlen(srvErr) > 0 ? srvErr : (LANG_DE ? "Shop abgelehnt!" : "Shop declined!"), 2200);
        }
      }
      if (allowLocal) {
        if (s_app->engine.purchaseRecipe(sr.id)) {
          cyd_sound_level_up();
          char msg[56];
          snprintf(msg, sizeof(msg), "%s (+30XP)", rName);
          cyd_lvgl_show_banner(msg, 2000);
        } else if (s_app->engine.level < sr.minLevel) {
          char msg[40];
          snprintf(msg, sizeof(msg), LANG_DE ? "Braucht Level %u!" : "Needs Level %u!", (unsigned)sr.minLevel);
          cyd_lvgl_show_banner(msg, 1800);
        } else {
          cyd_lvgl_show_banner(LANG_DE ? "Zu wenig Coins!" : "Not enough Coins!", 1800);
        }
      }
    }
  } else if (code >= 400 && code < 400 + (int)s_app->engine.rules.shopSpellCount) {
    int ssIdx = code - 400;
    const EngineShopSpell& ss = s_app->engine.rules.shopSpells[ssIdx];
    char spName[28];
    sanitize_lvgl_text(PICK_RULE_NAME(ss), spName, sizeof(spName));
    if (s_app->engine.isSpellPurchased(ss.id)) {
      cyd_lvgl_show_banner(LANG_DE ? "Zauberbuch bereits im Besitz!" : "Tome already owned!", 1800);
    } else {
      bool allowLocal = true;
      if (!s_app->offlineMode) {
        char srvErr[64] = "";
        int srvRes = cyd_net_shop_purchase(
          s_app->config, "spell", ss.id,
          s_app->engine.coins, s_app->engine.level,
          srvErr, sizeof(srvErr)
        );
        if (srvRes == 0) {
          allowLocal = false;
          cyd_sound_warning();
          cyd_lvgl_show_banner(strlen(srvErr) > 0 ? srvErr : (LANG_DE ? "Shop abgelehnt!" : "Shop declined!"), 2200);
        }
      }
      if (allowLocal) {
        if (s_app->engine.purchaseSpell(ss.id)) {
          cyd_sound_level_up();
          char msg[56];
          snprintf(msg, sizeof(msg), "%s (+45XP)", spName);
          cyd_lvgl_show_banner(msg, 2000);
        } else if (s_app->engine.level < ss.minLevel) {
          char msg[40];
          snprintf(msg, sizeof(msg), LANG_DE ? "Braucht Level %u!" : "Needs Level %u!", (unsigned)ss.minLevel);
          cyd_lvgl_show_banner(msg, 1800);
        } else {
          cyd_lvgl_show_banner(LANG_DE ? "Zu wenig Coins!" : "Not enough Coins!", 1800);
        }
      }
    }
  } else if (code >= 0 && code < s_app->engine.inventoryCount) {
    char id[20];
    strncpy(id, s_app->engine.inventory[code].id, sizeof(id) - 1);
    id[sizeof(id) - 1] = '\0';
    if (s_app->engine.inventory[code].category == 0) {
      FeedResult res = s_app->engine.feedWithItem(id);
      if (res.success) {
        cyd_sound_munch();
        cyd_lvgl_show_banner(res.becameSick ? (LANG_DE ? "KRANK vom Snack!" : "SICK from Snack!") : (LANG_DE ? "Gefuettert!" : "Fed Pet!"), 2000);
      }
    } else if (s_app->engine.inventory[code].category == 2) {
      if (s_app->engine.usePotionItem(id)) {
        cyd_sound_level_up();
        cyd_lvgl_show_banner(LANG_DE ? "Trank/Salbe angewendet!" : "Used Potion/Salve!", 2200);
      }
    } else if (s_app->engine.inventory[code].category == 3) {
      bool wasEquipped = (strcmp(s_app->engine.equippedHead, id) == 0);
      if (s_app->engine.equipAccessory(id)) {
        cyd_sound_treat_sent();
        cyd_lvgl_show_banner(
          wasEquipped ? (LANG_DE ? "Accessoire abgelegt!" : "Accessory unequipped!")
                      : (LANG_DE ? "Accessoire angelegt!" : "Accessory equipped!"),
          2000
        );
      }
    }
  }
  check_level_up_notify(prevLv);
  cyd_lvgl_open_modal(TAB_BAG);
  cyd_lvgl_refresh_main_screen();
}

static void on_job_start_cb(lv_event_t* e) {
  if (!s_app) return;
  int jIdx = (int)(intptr_t)lv_event_get_user_data(e);
  if (jIdx < 0 || jIdx >= (int)s_app->engine.rules.jobCount) return;
  cyd_sound_click();
  if (s_app->activeJob.running) {
    cyd_lvgl_show_banner(LANG_DE ? "Beruf laeuft bereits!" : "Job already running!", 1800);
    return;
  }
  const EngineJob& job = s_app->engine.rules.jobs[jIdx];
  if (s_app->engine.level < job.minLevel) {
    char msg[40];
    snprintf(msg, sizeof(msg), LANG_DE ? "Braucht Level %u!" : "Needs Level %u!", (unsigned)job.minLevel);
    cyd_lvgl_show_banner(msg, 1800);
    return;
  }
  if (s_app->engine.stats.energy < job.energyCost) {
    cyd_lvgl_show_banner(LANG_DE ? "Zu wenig Energie!" : "Not enough Energy!", 1800);
    return;
  }
  s_app->engine.stats.energy -= job.energyCost;
  s_app->activeJob.running = true;
  s_app->activeJob.jobIndex = (uint8_t)jIdx;
  s_app->activeJob.startedMs = cyd_millis();
  s_app->activeJob.durationMs = (uint32_t)(job.durationSec > 0 ? job.durationSec : 60) * 1000U;
  char jName[32], msg[64];
  sanitize_lvgl_text(PICK_RULE_NAME(job), jName, sizeof(jName));
  snprintf(msg, sizeof(msg), LANG_DE ? "Gestartet: %s" : "Started: %s", jName);
  cyd_lvgl_show_banner(msg, 2000);
  cyd_lvgl_open_modal(TAB_JOBS);
  cyd_lvgl_refresh_main_screen();
}

static void on_magic_action_cb(lv_event_t* e) {
  if (!s_app) return;
  int mIdx = (int)(intptr_t)lv_event_get_user_data(e);
  cyd_sound_click();
  uint8_t prevLv = s_app->engine.level;

  if (mIdx == 99) { // Research Magic (-15 EN -> +15 MP, +35 XP)
    if (s_app->engine.stats.energy >= 15) {
      s_app->engine.stats.energy -= 15;
      uint8_t maxMana = s_app->engine.getEffectiveMaxMana();
      s_app->engine.stats.mana = (uint8_t)engineMin((int)maxMana, (int)s_app->engine.stats.mana + 15);
      s_app->engine.addXP(35);
      s_app->engine.recordAction("magic", 1);
      cyd_sound_level_up();
      cyd_lvgl_show_banner(LANG_DE ? "Erforscht! +15MP +35XP" : "Researched! +15MP +35XP", 2500);
    } else {
      cyd_lvgl_show_banner(LANG_DE ? "Braucht 15 Energie!" : "Needs 15 Energy!", 1800);
    }
  } else if (mIdx >= 0 && mIdx < (int)s_app->engine.rules.spellCount) {
    const EngineSpell& sp = s_app->engine.rules.spells[mIdx];
    char spName[28];
    sanitize_lvgl_text(PICK_RULE_NAME(sp), spName, sizeof(spName));
    if (s_app->engine.level < sp.minLevel) {
      char msg[40];
      snprintf(msg, sizeof(msg), LANG_DE ? "Braucht Level %u!" : "Needs Level %u!", (unsigned)sp.minLevel);
      cyd_lvgl_show_banner(msg, 1800);
    } else if (s_app->engine.stats.mana < sp.manaCost) {
      cyd_lvgl_show_banner(TXT_BANNER_NO_MANA, 1800);
    } else if (s_app->engine.castSpell((uint8_t)mIdx)) {
      s_app->spellsCast++;
      if (sp.target == 2) {
        uint32_t now = cyd_millis();
        NetPacket pkt = {};
        strncpy(pkt.type, "MAGIC_SYNC", sizeof(pkt.type) - 1);
        strncpy(pkt.from, s_app->config.deviceId, sizeof(pkt.from) - 1);
        strncpy(pkt.text, sp.name, sizeof(pkt.text) - 1);
        cyd_net_send_packet_ex(s_app->config, s_app->friendPet, pkt);
        if (s_app->friendDualCastUntilMs > 0 && now <= s_app->friendDualCastUntilMs) {
          s_app->friendDualCastUntilMs = 0;
          s_app->dualCastUntilMs = 0;
          s_app->auroraUntilMs = now + 4000U;
          cyd_sound_level_up();
          cyd_lvgl_show_banner(LANG_DE ? "ASTRALE HARMONIE AKTIV!" : "ASTRAL HARMONY ACTIVE!", 3000);
        } else {
          s_app->dualCastUntilMs = now + 10000U;
          cyd_sound_treat_sent();
          char msg[56];
          snprintf(msg, sizeof(msg), LANG_DE ? "%s! Dual-Window 10s!" : "%s! 10s Dual-Window!", spName);
          cyd_lvgl_show_banner(msg, 2800);
        }
      } else if (sp.target == 1) {
        cyd_net_send_packet(s_app->config, s_app->friendPet, "MAGIC", sp.name);
        cyd_sound_treat_sent();
        char msg[56];
        snprintf(msg, sizeof(msg), LANG_DE ? "Segen gesendet: %s!" : "Blessing sent: %s!", spName);
        cyd_lvgl_show_banner(msg, 2500);
      } else {
        cyd_sound_treat_sent();
        char msg[56];
        snprintf(msg, sizeof(msg), LANG_DE ? "Zauber: %s!" : "Cast: %s!", spName);
        cyd_lvgl_show_banner(msg, 2500);
      }
    }
  }
  check_level_up_notify(prevLv);
  cyd_lvgl_open_modal(TAB_MAGIC);
  cyd_lvgl_refresh_main_screen();
}

static void on_quest_action_cb(lv_event_t* e) {
  if (!s_app) return;
  int qCode = (int)(intptr_t)lv_event_get_user_data(e);
  cyd_sound_click();
  uint8_t prevLv = s_app->engine.level;

  if (qCode == 0) { // Claim streak
    if (s_app->engine.claimDailyLoginStreakBonus()) {
      cyd_sound_level_up();
      cyd_lvgl_show_banner(LANG_DE ? "Login-Streak eingeloest!" : "Login Streak claimed!", 2500);
    }
  } else if (qCode == 1) { // Claim first completed mission
    for (uint8_t i = 0; i < s_app->engine.missionCount; i++) {
      if (s_app->engine.missions[i].completed && !s_app->engine.missions[i].claimed) {
        s_app->engine.claimMission(s_app->engine.missions[i].id);
        cyd_sound_level_up();
        cyd_lvgl_show_banner(LANG_DE ? "Mission eingeloest!" : "Mission claimed!", 2500);
        break;
      }
    }
  } else if (qCode == 2) { // Co-op Scout Patrol
    cyd_net_send_packet(s_app->config, s_app->friendPet, "QUEST", "Scout Patrol");
    s_app->engine.coins += 35;
    s_app->engine.addXP(45);
    s_app->jobsCompleted++;
    cyd_sound_treat_sent();
    cyd_lvgl_show_banner(LANG_DE ? "Spaeher-Quest! +35$ +45XP" : "Scout Patrol! +35$ +45XP", 2500);
  } else if (qCode == 3) { // Treasure Hunt
    cyd_net_send_packet(s_app->config, s_app->friendPet, "QUEST", "Treasure Hunt");
    s_app->engine.coins += 50;
    s_app->engine.addKnownItemById("food_apple", 1);
    s_app->engine.addXP(60);
    s_app->jobsCompleted++;
    cyd_sound_level_up();
    cyd_lvgl_show_banner(LANG_DE ? "Schatzsuche! +50$ +Apfel" : "Treasure Hunt! +50$ +Apple", 2500);
  }
  check_level_up_notify(prevLv);
  cyd_lvgl_open_modal(TAB_QUEST);
  cyd_lvgl_refresh_main_screen();
}

static const char* const CHAT_PHRASES[6] = {
#if LANG_DE
  "Hallo Buddy!",
  "Snack unterwegs!",
  "Lass questen!",
  "Magie-Segen!",
  "GG mein Freund!",
  "Bis spaeter!"
#else
  "Hello Buddy!",
  "Treat incoming!",
  "Co-op Quest?",
  "Magic Blessing!",
  "GG my Friend!",
  "See you soon!"
#endif
};

static void on_chat_phrase_cb(lv_event_t* e) {
  if (!s_app) return;
  int idx = (int)(intptr_t)lv_event_get_user_data(e);
  if (idx < 0 || idx >= 6) return;
  cyd_sound_treat_sent();
  cyd_net_send_packet(s_app->config, s_app->friendPet, "CHAT", CHAT_PHRASES[idx]);
  char msg[64];
  snprintf(msg, sizeof(msg), "Chat: %s", CHAT_PHRASES[idx]);
  cyd_lvgl_show_banner(msg, 2500);
  cyd_lvgl_close_modal();
}

static void on_toggle_sleep_cb(lv_event_t* /*e*/) {
  if (!s_app) return;
  cyd_sound_click();
  s_app->engine.toggleSleep();
  cyd_lvgl_show_banner(
    s_app->engine.isSleeping
      ? (LANG_DE ? "Gute Nacht! Energie laedt..." : "Sleeping! Regen Energy...")
      : (LANG_DE ? "Guten Morgen! Pet wach!" : "Good morning! Pet awake!"),
    2200
  );
  cyd_lvgl_open_modal(TAB_STATUS);
  cyd_lvgl_refresh_main_screen();
}

static void on_save_cloud_cb(lv_event_t* /*e*/) {
  if (!s_app) return;
  cyd_sound_click();
  s_app->triggerSaveRequested = true;
  cyd_lvgl_close_modal();
}

static void on_load_cloud_cb(lv_event_t* /*e*/) {
  if (!s_app) return;
  cyd_sound_click();
  s_app->triggerCloudLoadRequested = true;
  cyd_lvgl_close_modal();
}

static const char* const TRADE_WANT_IDS[5] = { "", "food_apple", "mat_herb", "mat_mushroom", "item_salve" };
static const char* const TRADE_WANT_NAMES[5] = {
#if LANG_DE
  "Freies Geschenk", "Frischer Apfel", "Wiesenkraeuter", "Waldpilz", "Heilsalbe"
#else
  "Free Gift", "Crisp Apple", "Meadow Herb", "Forest Mushroom", "Healing Salve"
#endif
};

static void on_trade_want_cycle_cb(lv_event_t* /*e*/) {
  if (!s_app) return;
  cyd_sound_click();
  s_app->tradeSelectedWantIdx = (uint8_t)((s_app->tradeSelectedWantIdx + 1) % 5);
  cyd_lvgl_open_modal(TAB_TRADE);
}

static void on_trade_send_offer_cb(lv_event_t* e) {
  if (!s_app) return;
  int slotIdx = (int)(intptr_t)lv_event_get_user_data(e);
  if (s_app->outgoingTrade.active) {
    cyd_lvgl_show_banner(LANG_DE ? "Tausch laeuft noch!" : "Trade still active!", 1800);
    return;
  }
  if (slotIdx < 0 || slotIdx >= s_app->engine.inventoryCount) return;

  cyd_sound_click();
  char offerId[20];
  char offerName[28];
  strncpy(offerId, s_app->engine.inventory[slotIdx].id, sizeof(offerId) - 1);
  offerId[sizeof(offerId) - 1] = '\0';
  sanitize_lvgl_text(PICK_RULE_NAME(s_app->engine.inventory[slotIdx]), offerName, sizeof(offerName));

  if (!s_app->engine.consumeInventoryItem(offerId, 1)) return;

  uint8_t wIdx = s_app->tradeSelectedWantIdx % 5;
  s_app->outgoingTrade.active = true;
  snprintf(s_app->outgoingTrade.tradeId, sizeof(s_app->outgoingTrade.tradeId), "tr_%lu", (unsigned long)cyd_millis());
  strncpy(s_app->outgoingTrade.offeredId, offerId, sizeof(s_app->outgoingTrade.offeredId) - 1);
  strncpy(s_app->outgoingTrade.offeredName, offerName, sizeof(s_app->outgoingTrade.offeredName) - 1);
  strncpy(s_app->outgoingTrade.requestedId, TRADE_WANT_IDS[wIdx], sizeof(s_app->outgoingTrade.requestedId) - 1);
  strncpy(s_app->outgoingTrade.requestedName, TRADE_WANT_NAMES[wIdx], sizeof(s_app->outgoingTrade.requestedName) - 1);

  NetPacket pkt = {};
  strncpy(pkt.type, "TRADE_OFFER", sizeof(pkt.type) - 1);
  strncpy(pkt.from, s_app->config.deviceId, sizeof(pkt.from) - 1);
  snprintf(pkt.text, sizeof(pkt.text), "%s -> %s", offerName, TRADE_WANT_NAMES[wIdx]);
  strncpy(pkt.tradeId, s_app->outgoingTrade.tradeId, sizeof(pkt.tradeId) - 1);
  strncpy(pkt.offeredId, offerId, sizeof(pkt.offeredId) - 1);
  strncpy(pkt.offeredName, offerName, sizeof(pkt.offeredName) - 1);
  strncpy(pkt.requestedId, TRADE_WANT_IDS[wIdx], sizeof(pkt.requestedId) - 1);
  strncpy(pkt.requestedName, TRADE_WANT_NAMES[wIdx], sizeof(pkt.requestedName) - 1);

  cyd_net_send_packet_ex(s_app->config, s_app->friendPet, pkt);
  cyd_sound_treat_sent();
  cyd_lvgl_show_banner(LANG_DE ? "Tausch in Escrow gesendet!" : "Trade sent to Escrow!", 2500);
  cyd_lvgl_open_modal(TAB_TRADE);
  cyd_lvgl_refresh_main_screen();
}

static void on_trade_cancel_outgoing_cb(lv_event_t* /*e*/) {
  if (!s_app || !s_app->outgoingTrade.active) return;
  cyd_sound_click();
  s_app->engine.addKnownItemById(s_app->outgoingTrade.offeredId, 1);

  NetPacket pkt = {};
  strncpy(pkt.type, "TRADE_DECLINE", sizeof(pkt.type) - 1);
  strncpy(pkt.from, s_app->config.deviceId, sizeof(pkt.from) - 1);
  strncpy(pkt.tradeId, s_app->outgoingTrade.tradeId, sizeof(pkt.tradeId) - 1);
  cyd_net_send_packet_ex(s_app->config, s_app->friendPet, pkt);

  memset(&s_app->outgoingTrade, 0, sizeof(s_app->outgoingTrade));
  cyd_lvgl_show_banner(LANG_DE ? "Escrow abgebrochen (zurueck)" : "Escrow cancelled (returned)", 2200);
  cyd_lvgl_open_modal(TAB_TRADE);
  cyd_lvgl_refresh_main_screen();
}

static void on_trade_accept_cb(lv_event_t* /*e*/) {
  if (!s_app || !s_app->incomingTrade.active) return;
  cyd_sound_click();

  if (strlen(s_app->incomingTrade.requestedId) > 0) {
    if (s_app->engine.getItemQuantity(s_app->incomingTrade.requestedId) == 0) {
      cyd_sound_warning();
      cyd_lvgl_show_banner(LANG_DE ? "Gegenstand fehlt im Beutel!" : "Missing requested item!", 2200);
      return;
    }
    s_app->engine.consumeInventoryItem(s_app->incomingTrade.requestedId, 1);
  }

  s_app->engine.addKnownItemById(s_app->incomingTrade.offeredId, 1);
  s_app->engine.addXP(35);

  NetPacket pkt = {};
  strncpy(pkt.type, "TRADE_ACCEPT", sizeof(pkt.type) - 1);
  strncpy(pkt.from, s_app->config.deviceId, sizeof(pkt.from) - 1);
  strncpy(pkt.tradeId, s_app->incomingTrade.tradeId, sizeof(pkt.tradeId) - 1);
  strncpy(pkt.offeredId, s_app->incomingTrade.offeredId, sizeof(pkt.offeredId) - 1);
  strncpy(pkt.requestedId, s_app->incomingTrade.requestedId, sizeof(pkt.requestedId) - 1);
  cyd_net_send_packet_ex(s_app->config, s_app->friendPet, pkt);

  memset(&s_app->incomingTrade, 0, sizeof(s_app->incomingTrade));
  cyd_sound_level_up();
  cyd_lvgl_show_banner(LANG_DE ? "Tausch abgeschlossen! +35XP" : "Trade complete! +35XP", 2800);
  cyd_lvgl_close_modal();
  cyd_lvgl_refresh_main_screen();
}

static void on_trade_decline_cb(lv_event_t* /*e*/) {
  if (!s_app || !s_app->incomingTrade.active) return;
  cyd_sound_click();
  NetPacket pkt = {};
  strncpy(pkt.type, "TRADE_DECLINE", sizeof(pkt.type) - 1);
  strncpy(pkt.from, s_app->config.deviceId, sizeof(pkt.from) - 1);
  strncpy(pkt.tradeId, s_app->incomingTrade.tradeId, sizeof(pkt.tradeId) - 1);
  cyd_net_send_packet_ex(s_app->config, s_app->friendPet, pkt);

  memset(&s_app->incomingTrade, 0, sizeof(s_app->incomingTrade));
  cyd_lvgl_show_banner(LANG_DE ? "Tausch abgelehnt." : "Trade declined.", 2000);
  cyd_lvgl_open_modal(TAB_TRADE);
}

static void on_cancel_ap_cb(lv_event_t* /*e*/) {
  if (!s_app) return;
  cyd_sound_click();
  s_app->apCancelRequested = true;
}

static void on_open_ap_cb(lv_event_t* /*e*/) {
  if (!s_app) return;
  cyd_sound_click();
  s_app->retryPortalRequested = true;
}

// ============================================================================
// LVGL CORE INIT
// ============================================================================
bool cyd_lvgl_init(CydDisplay* display) {
  s_disp = display;
  lv_init();
  init_all_sprites();

  lv_disp_draw_buf_init(&s_draw_buf, s_buf1, NULL, BOARD_DISPLAY_WIDTH * BOARD_DISPLAY_BUFFER_ROWS);
  lv_disp_drv_init(&s_disp_drv);
  s_disp_drv.hor_res = BOARD_DISPLAY_WIDTH;
  s_disp_drv.ver_res = BOARD_DISPLAY_HEIGHT;
  s_disp_drv.flush_cb = lvgl_flush_cb;
  s_disp_drv.draw_buf = &s_draw_buf;
  lv_disp_drv_register(&s_disp_drv);

  lv_indev_drv_init(&s_indev_drv);
  s_indev_drv.type = LV_INDEV_TYPE_POINTER;
  s_indev_drv.read_cb = lvgl_touch_read_cb;
  lv_indev_drv_register(&s_indev_drv);

  esp_timer_create_args_t targs = {};
  targs.callback = lvgl_tick_cb;
  targs.name = "lvgl_tick";
  if (esp_timer_create(&targs, &s_lvgl_tick_timer) == ESP_OK) {
    esp_timer_start_periodic(s_lvgl_tick_timer, 5000); // 5ms
  }
  return true;
}

void cyd_lvgl_set_app_state(AppRuntimeState* state) {
  s_app = state;
}

// ============================================================================
// SYSTEM SCREENS (BOOT, AP PORTAL, SERVER ERROR, OTA)
// ============================================================================
void cyd_lvgl_show_boot_screen(const char* ssid) {
  lv_obj_t* scr = lv_obj_create(NULL);
  lv_obj_set_style_bg_color(scr, lv_color_hex(0x090D16), 0);

  lv_obj_t* title = lv_label_create(scr);
  lv_label_set_text(title, "PetDuet CYD (ESP-IDF + LVGL)");
  lv_obj_set_style_text_font(title, &lv_font_montserrat_16, 0);
  lv_obj_set_style_text_color(title, lv_color_hex(0xF59E0B), 0);
  lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 52);

  lv_obj_t* sub = lv_label_create(scr);
  lv_label_set_text_fmt(sub, LANG_DE ? "Verbinde mit WLAN: %s..." : "Connecting WiFi: %s...", ssid ? ssid : "");
  lv_obj_set_style_text_color(sub, lv_color_hex(0xE2E8F0), 0);
  lv_obj_align(sub, LV_ALIGN_CENTER, 0, -4);

  lv_obj_t* ver = lv_label_create(scr);
  const char* rulesVer = (s_app && strlen(s_app->engine.rules.version) > 0) ? s_app->engine.rules.version : RULES_VERSION;
  lv_label_set_text_fmt(ver, "FW v%s  |  Rules v%s", FIRMWARE_VERSION, rulesVer);
  lv_obj_set_style_text_color(ver, lv_color_hex(0x22D3EE), 0);
  lv_obj_align(ver, LV_ALIGN_CENTER, 0, 28);

  lv_scr_load(scr);
  lv_timer_handler();
}

void cyd_lvgl_show_ap_screen(void) {
  lv_obj_t* scr = lv_obj_create(NULL);
  lv_obj_set_style_bg_color(scr, lv_color_hex(0x0F172A), 0);

  lv_obj_t* card = lv_obj_create(scr);
  lv_obj_set_size(card, 300, 220);
  lv_obj_center(card);
  lv_obj_set_style_bg_color(card, lv_color_hex(0x1E293B), 0);
  lv_obj_set_style_border_color(card, lv_color_hex(0xF59E0B), 0);

  lv_obj_t* lbl = lv_label_create(card);
  lv_label_set_text(lbl, LANG_DE
    ? "WLAN SETUP MODUS\n\n1. Mit AP verbinden:\n   CYD-Tamagotchi-AP\n\n2. Browser oeffnen:\n   http://192.168.4.1"
    : "WIFI SETUP MODE\n\n1. Connect to AP:\n   CYD-Tamagotchi-AP\n\n2. Open Browser:\n   http://192.168.4.1");
  lv_obj_set_style_text_color(lbl, lv_color_hex(0xF8FAFC), 0);
  lv_obj_align(lbl, LV_ALIGN_TOP_LEFT, 6, 4);

  lv_obj_t* btn = lv_btn_create(card);
  lv_obj_set_size(btn, 220, 34);
  lv_obj_align(btn, LV_ALIGN_BOTTOM_MID, 0, -4);
  lv_obj_set_style_bg_color(btn, lv_color_hex(0x334155), 0);
  lv_obj_add_event_cb(btn, on_cancel_ap_cb, LV_EVENT_CLICKED, NULL);
  lv_obj_t* bLbl = lv_label_create(btn);
  lv_label_set_text(bLbl, LANG_DE ? "Offline Modus starten" : "Start Offline Mode");
  lv_obj_center(bLbl);

  lv_scr_load(scr);
  lv_timer_handler();
}

void cyd_lvgl_show_server_error_screen(const char* ipStr, const char* serverUrl) {
  lv_obj_t* scr = lv_obj_create(NULL);
  lv_obj_set_style_bg_color(scr, lv_color_hex(0x111827), 0);

  lv_obj_t* card = lv_obj_create(scr);
  lv_obj_set_size(card, 300, 220);
  lv_obj_center(card);
  lv_obj_set_style_bg_color(card, lv_color_hex(0x1F2937), 0);
  lv_obj_set_style_border_color(card, lv_color_hex(0xEF4444), 0);

  lv_obj_t* lbl = lv_label_create(card);
  lv_label_set_text_fmt(lbl,
    LANG_DE ? "SERVER NICHT ERREICHBAR\nWiFi IP: %s\nURL:\n%s"
            : "SERVER UNREACHABLE\nWiFi IP: %s\nURL:\n%s",
    ipStr ? ipStr : "?", serverUrl ? serverUrl : "");
  lv_obj_set_style_text_color(lbl, lv_color_hex(0xFCA5A5), 0);
  lv_obj_set_width(lbl, 270);
  lv_obj_align(lbl, LV_ALIGN_TOP_LEFT, 4, 4);

  lv_obj_t* b1 = lv_btn_create(card);
  lv_obj_set_size(b1, 130, 34);
  lv_obj_align(b1, LV_ALIGN_BOTTOM_LEFT, 0, -2);
  lv_obj_set_style_bg_color(b1, lv_color_hex(0xD97706), 0);
  lv_obj_add_event_cb(b1, on_open_ap_cb, LV_EVENT_CLICKED, NULL);
  lv_obj_t* l1 = lv_label_create(b1);
  lv_label_set_text(l1, "AP Setup");
  lv_obj_center(l1);

  lv_obj_t* b2 = lv_btn_create(card);
  lv_obj_set_size(b2, 130, 34);
  lv_obj_align(b2, LV_ALIGN_BOTTOM_RIGHT, 0, -2);
  lv_obj_set_style_bg_color(b2, lv_color_hex(0x334155), 0);
  lv_obj_add_event_cb(b2, on_cancel_ap_cb, LV_EVENT_CLICKED, NULL);
  lv_obj_t* l2 = lv_label_create(b2);
  lv_label_set_text(l2, "Offline");
  lv_obj_center(l2);

  lv_scr_load(scr);
  lv_timer_handler();
}

void cyd_lvgl_show_ota_progress(const char* stage, int percent) {
  if (!s_ota_bar) {
    lv_obj_t* scr = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x0B1120), 0);

    lv_obj_t* t = lv_label_create(scr);
    lv_label_set_text(t, "FIRMWARE UPDATE (A/B OTA)");
    lv_obj_set_style_text_font(t, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(t, lv_color_hex(0xFACC15), 0);
    lv_obj_align(t, LV_ALIGN_TOP_MID, 0, 26);

    s_ota_stage_lbl = lv_label_create(scr);
    lv_obj_set_style_text_color(s_ota_stage_lbl, lv_color_hex(0x38BDF8), 0);
    lv_obj_align(s_ota_stage_lbl, LV_ALIGN_CENTER, 0, -24);

    s_ota_bar = lv_bar_create(scr);
    lv_obj_set_size(s_ota_bar, 260, 22);
    lv_obj_align(s_ota_bar, LV_ALIGN_CENTER, 0, 8);
    lv_bar_set_range(s_ota_bar, 0, 100);
    lv_obj_set_style_bg_color(s_ota_bar, lv_color_hex(0x10B981), LV_PART_INDICATOR);

    s_ota_pct_lbl = lv_label_create(scr);
    lv_obj_set_style_text_color(s_ota_pct_lbl, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(s_ota_pct_lbl, LV_ALIGN_CENTER, 0, 42);

    lv_obj_t* warn = lv_label_create(scr);
    lv_label_set_text(warn, "NICHT AUSSCHALTEN! / DO NOT POWER OFF!");
    lv_obj_set_style_text_color(warn, lv_color_hex(0xF87171), 0);
    lv_obj_align(warn, LV_ALIGN_BOTTOM_MID, 0, -24);

    lv_scr_load(scr);
  }
  lv_label_set_text(s_ota_stage_lbl, stage ? stage : "");
  lv_bar_set_value(s_ota_bar, percent, LV_ANIM_OFF);
  lv_label_set_text_fmt(s_ota_pct_lbl, "%d%%", percent);
  lv_timer_handler();
}

// ============================================================================
// MAIN DUAL-PET LVGL SCREEN
// ============================================================================
static lv_obj_t* create_rail_button(lv_obj_t* parent, int x, int y, const char* label, uint32_t bgHex, SpriteIconId spr, lv_event_cb_t cb, int tagId) {
  lv_obj_t* btn = lv_btn_create(parent);
  lv_obj_set_pos(btn, x, y);
  lv_obj_set_size(btn, 36, 44);
  lv_obj_set_style_bg_color(btn, lv_color_hex(bgHex), 0);
  lv_obj_set_style_radius(btn, 5, 0);
  lv_obj_set_style_border_color(btn, lv_color_hex(0x64748B), 0);
  lv_obj_set_style_border_width(btn, 1, 0);
  lv_obj_set_style_pad_all(btn, 1, 0);
  lv_obj_add_event_cb(btn, cb, LV_EVENT_CLICKED, (void*)(intptr_t)tagId);

  lv_obj_t* img = lv_img_create(btn);
  lv_img_set_src(img, &s_sprite_dsc[spr]);
  lv_obj_align(img, LV_ALIGN_TOP_MID, 0, 4);

  lv_obj_t* lbl = lv_label_create(btn);
  lv_label_set_text(lbl, label);
  lv_obj_set_style_text_font(lbl, &lv_font_montserrat_8, 0);
  lv_obj_set_style_text_color(lbl, lv_color_hex(0xFFFFFF), 0);
  lv_obj_align(lbl, LV_ALIGN_BOTTOM_MID, 0, -2);
  return btn;
}

static lv_obj_t* create_mini_stat_bar(lv_obj_t* parent, int x, int y, const char* tag, uint32_t colHex) {
  lv_obj_t* l = lv_label_create(parent);
  lv_label_set_text(l, tag);
  lv_obj_set_style_text_font(l, &lv_font_montserrat_8, 0);
  lv_obj_set_style_text_color(l, lv_color_hex(colHex), 0);
  lv_obj_set_pos(l, x, y);

  lv_obj_t* bar = lv_bar_create(parent);
  lv_obj_set_pos(bar, x + 18, y + 1);
  lv_obj_set_size(bar, 92, 6);
  lv_bar_set_range(bar, 0, 100);
  lv_obj_set_style_bg_color(bar, lv_color_hex(0x0F172A), 0);
  lv_obj_set_style_bg_color(bar, lv_color_hex(colHex), LV_PART_INDICATOR);
  return bar;
}

void cyd_lvgl_show_main_screen(void) {
  s_ota_bar = nullptr;
  s_main_scr = lv_obj_create(NULL);
  lv_obj_set_style_bg_color(s_main_scr, lv_color_hex(0x0B0F1A), 0);
  lv_obj_clear_flag(s_main_scr, LV_OBJ_FLAG_SCROLLABLE);

  // Left Stage (x=40..159) & Right Stage (x=161..279)
  s_left_stage = lv_obj_create(s_main_scr);
  lv_obj_set_pos(s_left_stage, 40, 0);
  lv_obj_set_size(s_left_stage, 119, 240);
  lv_obj_set_style_border_width(s_left_stage, 0, 0);
  lv_obj_set_style_radius(s_left_stage, 0, 0);
  lv_obj_set_style_pad_all(s_left_stage, 0, 0);
  lv_obj_clear_flag(s_left_stage, LV_OBJ_FLAG_SCROLLABLE);

  s_right_stage = lv_obj_create(s_main_scr);
  lv_obj_set_pos(s_right_stage, 161, 0);
  lv_obj_set_size(s_right_stage, 119, 240);
  lv_obj_set_style_border_width(s_right_stage, 0, 0);
  lv_obj_set_style_radius(s_right_stage, 0, 0);
  lv_obj_set_style_pad_all(s_right_stage, 0, 0);
  lv_obj_clear_flag(s_right_stage, LV_OBJ_FLAG_SCROLLABLE);

  // Side Toolbars (dark panels)
  lv_obj_t* leftRail = lv_obj_create(s_main_scr);
  lv_obj_set_pos(leftRail, 0, 0);
  lv_obj_set_size(leftRail, 40, 240);
  lv_obj_set_style_bg_color(leftRail, lv_color_hex(0x111827), 0);
  lv_obj_set_style_border_width(leftRail, 0, 0);
  lv_obj_set_style_radius(leftRail, 0, 0);
  lv_obj_set_style_pad_all(leftRail, 0, 0);
  lv_obj_clear_flag(leftRail, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t* rightRail = lv_obj_create(s_main_scr);
  lv_obj_set_pos(rightRail, 280, 0);
  lv_obj_set_size(rightRail, 40, 240);
  lv_obj_set_style_bg_color(rightRail, lv_color_hex(0x111827), 0);
  lv_obj_set_style_border_width(rightRail, 0, 0);
  lv_obj_set_style_radius(rightRail, 0, 0);
  lv_obj_set_style_pad_all(rightRail, 0, 0);
  lv_obj_clear_flag(rightRail, LV_OBJ_FLAG_SCROLLABLE);

  // Left Control Buttons (16x16 pixel sprites)
  create_rail_button(leftRail, 2, 2,   TXT_FEED,  0x15803D, SPR_FEED,  on_left_btn_clicked, 1);
  create_rail_button(leftRail, 2, 50,  TXT_PLAY,  0x1D4ED8, SPR_PLAY,  on_left_btn_clicked, 2);
  create_rail_button(leftRail, 2, 98,  TXT_BATH,  0x0369A1, SPR_BATH,  on_left_btn_clicked, 3);
  create_rail_button(leftRail, 2, 146, TXT_MAGIC, 0x6D28D9, SPR_MAGIC, on_left_btn_clicked, 4);
  create_rail_button(leftRail, 2, 194, TXT_SAVE,  0xB45309, SPR_STATS, on_left_btn_clicked, 5);

  // Right Control Buttons
  create_rail_button(rightRail, 2, 2,   TXT_TREAT,      0xBE185D, SPR_TREAT,    on_right_btn_clicked, 6);
  create_rail_button(rightRail, 2, 50,  TXT_CHAT,       0x0E7490, SPR_CHAT,     on_right_btn_clicked, 7);
  create_rail_button(rightRail, 2, 98,  TXT_MAGIC_PLUS, 0x6D28D9, SPR_MGC_PLUS, on_right_btn_clicked, 8);
  create_rail_button(rightRail, 2, 146, TXT_QUEST,      0x15803D, SPR_QUEST,    on_right_btn_clicked, 9);
  create_rail_button(rightRail, 2, 194, TXT_TRADE,      0xCA8A04, SPR_TRADE,    on_right_btn_clicked, 10);

  // Center Connection / OTA Trigger Dot
  s_dot_btn = lv_btn_create(s_main_scr);
  lv_obj_set_pos(s_dot_btn, 154, 114);
  lv_obj_set_size(s_dot_btn, 12, 12);
  lv_obj_set_style_radius(s_dot_btn, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_style_border_color(s_dot_btn, lv_color_hex(0xFFFFFF), 0);
  lv_obj_set_style_border_width(s_dot_btn, 1, 0);
  lv_obj_add_event_cb(s_dot_btn, on_dot_ota_clicked, LV_EVENT_CLICKED, NULL);

  // Left Pet HUD Widgets
  s_my_name_lbl = lv_label_create(s_left_stage);
  lv_obj_set_pos(s_my_name_lbl, 4, 4);
  lv_obj_set_style_text_color(s_my_name_lbl, lv_color_hex(0xFFFFFF), 0);

  s_my_lv_lbl = lv_label_create(s_left_stage);
  lv_obj_set_pos(s_my_lv_lbl, 82, 4);
  lv_obj_set_style_text_color(s_my_lv_lbl, lv_color_hex(0xFACC15), 0);

  s_my_phase_lbl = lv_label_create(s_left_stage);
  lv_obj_set_pos(s_my_phase_lbl, 4, 20);
  lv_obj_set_style_text_font(s_my_phase_lbl, &lv_font_montserrat_10, 0);

  s_my_badge_lbl = lv_label_create(s_left_stage);
  lv_obj_set_pos(s_my_badge_lbl, 56, 20);
  lv_obj_set_style_text_font(s_my_badge_lbl, &lv_font_montserrat_10, 0);

  s_my_hp_bar = create_mini_stat_bar(s_left_stage, 4, 34, "HP", 0xEF4444);
  s_my_fd_bar = create_mini_stat_bar(s_left_stage, 4, 44, "FD", 0xF97316);
  s_my_xp_bar = create_mini_stat_bar(s_left_stage, 4, 54, "XP", 0xA855F7);

  s_my_canvas = lv_canvas_create(s_left_stage);
  lv_canvas_set_buffer(s_my_canvas, s_my_pet_buf, PET_CANVAS_W, PET_CANVAS_H, LV_IMG_CF_TRUE_COLOR);
  lv_obj_set_pos(s_my_canvas, 21, 84);
  lv_obj_add_flag(s_my_canvas, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_add_event_cb(s_my_canvas, on_pet_stage_tapped, LV_EVENT_CLICKED, NULL);

  s_my_egg_lbl = lv_label_create(s_left_stage);
  lv_label_set_text(s_my_egg_lbl, TXT_TAP_EGG);
  lv_obj_set_style_text_font(s_my_egg_lbl, &lv_font_montserrat_10, 0);
  lv_obj_set_style_text_color(s_my_egg_lbl, lv_color_hex(0xFACC15), 0);
  lv_obj_set_pos(s_my_egg_lbl, 34, 174);

  lv_obj_t* ftrBtn = lv_btn_create(s_left_stage);
  lv_obj_set_pos(ftrBtn, 2, 212);
  lv_obj_set_size(ftrBtn, 115, 24);
  lv_obj_set_style_bg_opa(ftrBtn, LV_OPA_40, 0);
  lv_obj_set_style_bg_color(ftrBtn, lv_color_hex(0x000000), 0);
  lv_obj_add_event_cb(ftrBtn, on_footer_clicked, LV_EVENT_CLICKED, NULL);
  s_my_footer_lbl = lv_label_create(ftrBtn);
  lv_obj_set_style_text_font(s_my_footer_lbl, &lv_font_montserrat_10, 0);
  lv_obj_set_style_text_color(s_my_footer_lbl, lv_color_hex(0xFDE047), 0);
  lv_obj_center(s_my_footer_lbl);

  // Right Friend HUD Widgets
  s_fr_name_lbl = lv_label_create(s_right_stage);
  lv_obj_set_pos(s_fr_name_lbl, 6, 4);
  lv_obj_set_style_text_color(s_fr_name_lbl, lv_color_hex(0xFFFFFF), 0);

  s_fr_lv_lbl = lv_label_create(s_right_stage);
  lv_obj_set_pos(s_fr_lv_lbl, 84, 4);
  lv_obj_set_style_text_color(s_fr_lv_lbl, lv_color_hex(0x38BDF8), 0);

  s_fr_status_lbl = lv_label_create(s_right_stage);
  lv_obj_set_pos(s_fr_status_lbl, 6, 20);
  lv_obj_set_style_text_font(s_fr_status_lbl, &lv_font_montserrat_10, 0);

  s_fr_hp_bar = create_mini_stat_bar(s_right_stage, 6, 34, "HP", 0xEF4444);

  s_fr_canvas = lv_canvas_create(s_right_stage);
  lv_canvas_set_buffer(s_fr_canvas, s_fr_pet_buf, PET_CANVAS_W, PET_CANVAS_H, LV_IMG_CF_TRUE_COLOR);
  lv_obj_set_pos(s_fr_canvas, 21, 80);

  s_fr_balloon_box = lv_obj_create(s_right_stage);
  lv_obj_set_pos(s_fr_balloon_box, 4, 168);
  lv_obj_set_size(s_fr_balloon_box, 110, 26);
  lv_obj_set_style_bg_color(s_fr_balloon_box, lv_color_hex(0xFFFFFF), 0);
  lv_obj_set_style_border_color(s_fr_balloon_box, lv_color_hex(0x0F172A), 0);
  lv_obj_set_style_radius(s_fr_balloon_box, 4, 0);
  lv_obj_set_style_pad_all(s_fr_balloon_box, 2, 0);
  lv_obj_clear_flag(s_fr_balloon_box, LV_OBJ_FLAG_SCROLLABLE);

  s_fr_balloon_lbl = lv_label_create(s_fr_balloon_box);
  lv_obj_set_style_text_font(s_fr_balloon_lbl, &lv_font_montserrat_10, 0);
  lv_obj_set_style_text_color(s_fr_balloon_lbl, lv_color_hex(0x000000), 0);
  lv_obj_center(s_fr_balloon_lbl);

  s_fr_footer_lbl = lv_label_create(s_right_stage);
  lv_obj_set_pos(s_fr_footer_lbl, 6, 218);
  lv_obj_set_style_text_font(s_fr_footer_lbl, &lv_font_montserrat_10, 0);
  lv_obj_set_style_text_color(s_fr_footer_lbl, lv_color_hex(0x67E8F9), 0);

  // Horizontal Feed Flyout Bar (Right next to Feed Button!)
  s_flyout_bar = lv_obj_create(s_main_scr);
  lv_obj_set_pos(s_flyout_bar, 42, 2);
  lv_obj_set_size(s_flyout_bar, 232, 44);
  lv_obj_set_style_bg_color(s_flyout_bar, lv_color_hex(0x0F172A), 0);
  lv_obj_set_style_border_color(s_flyout_bar, lv_color_hex(0xF59E0B), 0);
  lv_obj_set_style_pad_all(s_flyout_bar, 3, 0);
  lv_obj_set_flex_flow(s_flyout_bar, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(s_flyout_bar, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_add_flag(s_flyout_bar, LV_OBJ_FLAG_HIDDEN);

  // Notification Toast Banner
  s_banner_box = lv_obj_create(s_main_scr);
  lv_obj_set_pos(s_banner_box, 48, 4);
  lv_obj_set_size(s_banner_box, 224, 24);
  lv_obj_set_style_bg_color(s_banner_box, lv_color_hex(0x1E293B), 0);
  lv_obj_set_style_border_color(s_banner_box, lv_color_hex(0xFACC15), 0);
  lv_obj_set_style_radius(s_banner_box, 6, 0);
  lv_obj_set_style_pad_all(s_banner_box, 2, 0);
  lv_obj_add_flag(s_banner_box, LV_OBJ_FLAG_HIDDEN);
  s_banner_lbl = lv_label_create(s_banner_box);
  lv_obj_set_style_text_font(s_banner_lbl, &lv_font_montserrat_10, 0);
  lv_obj_set_style_text_color(s_banner_lbl, lv_color_hex(0xFDE047), 0);
  lv_obj_center(s_banner_lbl);

  lv_scr_load(s_main_scr);
  cyd_lvgl_refresh_main_screen();
}

void cyd_lvgl_refresh_main_screen(void) {
  if (!s_app || !s_main_scr) return;
  const TamagotchiCoreEngine& eng = s_app->engine;
  const FriendState& fr = s_app->friendPet;

  BiomeTheme myBiome = getBiomeTheme(eng.biome);
  BiomeTheme frBiome = getBiomeTheme(fr.biome);

  uint32_t nowMs = cyd_millis();
  bool auroraOn = (s_app->auroraUntilMs > nowMs);
  bool coopReady = (s_app->friendDualCastUntilMs > nowMs);
  bool coopWait = (s_app->dualCastUntilMs > nowMs);

  lv_obj_set_style_bg_color(s_left_stage, rgb565_to_lv(myBiome.bgColor), 0);
  lv_obj_set_style_bg_color(s_right_stage, rgb565_to_lv(frBiome.bgColor), 0);
  if (auroraOn) {
    uint32_t glowCol = (((nowMs / 350U) % 2U) == 0) ? 0x22D3EE : 0xFACC15;
    lv_obj_set_style_border_width(s_left_stage, 2, 0);
    lv_obj_set_style_border_color(s_left_stage, lv_color_hex(glowCol), 0);
    lv_obj_set_style_border_width(s_right_stage, 2, 0);
    lv_obj_set_style_border_color(s_right_stage, lv_color_hex(glowCol), 0);
  } else {
    lv_obj_set_style_border_width(s_left_stage, 0, 0);
    lv_obj_set_style_border_width(s_right_stage, 0, 0);
  }
  lv_obj_set_style_bg_color(s_dot_btn, lv_color_hex(fr.isOnline ? 0x22C55E : 0xEF4444), 0);

  lv_label_set_text(s_my_name_lbl, eng.name);
  lv_label_set_text_fmt(s_my_lv_lbl, "Lv%u", eng.level);
  lv_label_set_text(s_my_phase_lbl, eng.phase);
  lv_obj_set_style_text_color(s_my_phase_lbl, rgb565_to_lv(myBiome.textColor), 0);

  if (eng.isSick) {
    lv_label_set_text(s_my_badge_lbl, TXT_SICK_BADGE);
    lv_obj_set_style_text_color(s_my_badge_lbl, lv_color_hex(0xEF4444), 0);
  } else if (eng.attentionNeeded) {
    lv_label_set_text(s_my_badge_lbl, LANG_DE ? "!PFLEGE" : "!CARE");
    lv_obj_set_style_text_color(s_my_badge_lbl, lv_color_hex(0xF87171), 0);
  } else if (eng.isSleeping) {
    lv_label_set_text(s_my_badge_lbl, TXT_ASLEEP);
    lv_obj_set_style_text_color(s_my_badge_lbl, lv_color_hex(0x38BDF8), 0);
  } else if (auroraOn) {
    lv_label_set_text(s_my_badge_lbl, "AURORA!");
    lv_obj_set_style_text_color(s_my_badge_lbl, lv_color_hex(0x22D3EE), 0);
  } else if (coopReady) {
    lv_label_set_text_fmt(s_my_badge_lbl, "MGC!%lus", (unsigned long)((s_app->friendDualCastUntilMs - nowMs + 999U) / 1000U));
    lv_obj_set_style_text_color(s_my_badge_lbl, lv_color_hex(0xF472B6), 0);
  } else if (coopWait) {
    lv_label_set_text_fmt(s_my_badge_lbl, "SYNC:%lus", (unsigned long)((s_app->dualCastUntilMs - nowMs + 999U) / 1000U));
    lv_obj_set_style_text_color(s_my_badge_lbl, lv_color_hex(0xFACC15), 0);
  } else if (eng.satietyTicks > 0) {
    lv_label_set_text_fmt(s_my_badge_lbl, LANG_DE ? "SATT:%uT" : "FULL:%uT", eng.satietyTicks);
    lv_obj_set_style_text_color(s_my_badge_lbl, lv_color_hex(0x4ADE80), 0);
  } else if (s_app->activeJob.running) {
    uint32_t elapsed = cyd_millis() - s_app->activeJob.startedMs;
    uint32_t remS = (elapsed < s_app->activeJob.durationMs) ? ((s_app->activeJob.durationMs - elapsed) / 1000U) : 0;
    lv_label_set_text_fmt(s_my_badge_lbl, "JOB:%lus", (unsigned long)remS);
    lv_obj_set_style_text_color(s_my_badge_lbl, lv_color_hex(0xFACC15), 0);
  } else {
    lv_label_set_text(s_my_badge_lbl, "");
  }

  lv_bar_set_value(s_my_hp_bar, eng.stats.health, LV_ANIM_OFF);
  lv_bar_set_value(s_my_fd_bar, eng.stats.hunger, LV_ANIM_OFF);
  uint16_t xpNext = (eng.xpNext > 0) ? eng.xpNext : 100;
  int xpPct = (int)((eng.xp * 100U) / xpNext);
  if (xpPct > 100) xpPct = 100;
  lv_bar_set_value(s_my_xp_bar, xpPct, LV_ANIM_OFF);

  LvCanvasDrawAdapter myCanv(s_my_pet_buf, PET_CANVAS_W, PET_CANVAS_H);
  myCanv.clear(myBiome.bgColor);
  if (strcmp(eng.phase, "Egg") == 0) {
    drawPixelEgg(myCanv, PET_CANVAS_W / 2, PET_CANVAS_H / 2 - 2, myBiome.petColor, eng.eggWarmth, eng.biome);
    lv_obj_clear_flag(s_my_egg_lbl, LV_OBJ_FLAG_HIDDEN);
  } else {
    drawPixelPet(myCanv, PET_CANVAS_W / 2, PET_CANVAS_H / 2, eng.biome, eng.phase, auroraOn);
    drawPetOverlays(myCanv, PET_CANVAS_W / 2, PET_CANVAS_H / 2,
                    eng.phase, eng.evoBranch, eng.equippedHead,
                    eng.poopCount, eng.isSick, eng.isSleeping, eng.attentionNeeded);
    lv_obj_add_flag(s_my_egg_lbl, LV_OBJ_FLAG_HIDDEN);
  }
  lv_obj_invalidate(s_my_canvas);

  lv_label_set_text_fmt(s_my_footer_lbl, "$%u S%u%s EN:%u",
                        eng.coins, eng.consecutiveLoginDays,
                        eng.canClaimStreakBonus() ? "!" : "", eng.stats.energy);

  // Right Friend Viewport
  lv_label_set_text(s_fr_name_lbl, fr.name);
  lv_label_set_text_fmt(s_fr_lv_lbl, "Lv%u", fr.level);
  lv_label_set_text(s_fr_status_lbl, fr.isOnline ? TXT_ONLINE : TXT_ASLEEP);
  lv_obj_set_style_text_color(s_fr_status_lbl, lv_color_hex(fr.isOnline ? 0x4ADE80 : 0xCBD5E1), 0);
  lv_bar_set_value(s_fr_hp_bar, fr.health, LV_ANIM_OFF);

  LvCanvasDrawAdapter frCanv(s_fr_pet_buf, PET_CANVAS_W, PET_CANVAS_H);
  frCanv.clear(frBiome.bgColor);
  drawPixelPet(frCanv, PET_CANVAS_W / 2, PET_CANVAS_H / 2, fr.biome, fr.phase, true);
  lv_obj_invalidate(s_fr_canvas);

  if (strlen(fr.speechBalloon) > 0) {
    lv_label_set_text(s_fr_balloon_lbl, fr.speechBalloon);
    lv_obj_clear_flag(s_fr_balloon_box, LV_OBJ_FLAG_HIDDEN);
  } else {
    lv_obj_add_flag(s_fr_balloon_box, LV_OBJ_FLAG_HIDDEN);
  }
  lv_label_set_text_fmt(s_fr_footer_lbl, "%s [%s]", fr.deviceId, strlen(fr.linkMode) > 0 ? fr.linkMode : "OFF");

  // Notification banner visibility
  if (strlen(s_app->bannerMsg) > 0 && !s_app->feedFlyoutOpen) {
    lv_label_set_text(s_banner_lbl, s_app->bannerMsg);
    lv_obj_clear_flag(s_banner_box, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(s_banner_box);
  } else {
    lv_obj_add_flag(s_banner_box, LV_OBJ_FLAG_HIDDEN);
  }
}

void cyd_lvgl_show_banner(const char* msg, uint32_t durationMs) {
  if (!s_app) return;
  strncpy(s_app->bannerMsg, msg ? msg : "", sizeof(s_app->bannerMsg) - 1);
  s_app->bannerMsg[sizeof(s_app->bannerMsg) - 1] = '\0';
  s_app->bannerUntilMs = (durationMs > 0) ? (cyd_millis() + durationMs) : 0;
  s_app->dirtyUi = true;
}

// ============================================================================
// 5-TAB RPG SUB-MODAL + CHAT PICKER
// ============================================================================
static lv_obj_t* add_modal_btn(lv_obj_t* parent, const char* txt, uint32_t colHex, lv_event_cb_t cb, int userInt) {
  lv_obj_t* btn = lv_btn_create(parent);
  lv_obj_set_size(btn, 276, 30);
  lv_obj_set_style_bg_color(btn, lv_color_hex(colHex), 0);
  lv_obj_add_event_cb(btn, cb, LV_EVENT_CLICKED, (void*)(intptr_t)userInt);
  lv_obj_t* l = lv_label_create(btn);
  lv_label_set_text(l, txt);
  lv_obj_set_style_text_font(l, &lv_font_montserrat_12, 0);
  lv_obj_align(l, LV_ALIGN_LEFT_MID, 4, 0);
  return btn;
}

void cyd_lvgl_close_modal(void) {
  if (s_modal_overlay) {
    lv_obj_del(s_modal_overlay);
    s_modal_overlay = nullptr;
  }
  if (s_app) s_app->activeModal = TAB_NONE;
}

void cyd_lvgl_open_modal(SubModalTab tab) {
  if (!s_app || !s_main_scr) return;
  if (s_modal_overlay) {
    lv_obj_del(s_modal_overlay);
    s_modal_overlay = nullptr;
  }
  s_app->activeModal = tab;

  s_modal_overlay = lv_obj_create(s_main_scr);
  lv_obj_set_pos(s_modal_overlay, 6, 6);
  lv_obj_set_size(s_modal_overlay, 308, 228);
  lv_obj_set_style_bg_color(s_modal_overlay, lv_color_hex(0x0F172A), 0);
  lv_obj_set_style_border_color(s_modal_overlay, lv_color_hex(0xF59E0B), 0);
  lv_obj_set_style_border_width(s_modal_overlay, 2, 0);
  lv_obj_set_style_pad_all(s_modal_overlay, 4, 0);
  lv_obj_clear_flag(s_modal_overlay, LV_OBJ_FLAG_SCROLLABLE);

  // Top Navigation Bar
  lv_obj_t* nav = lv_obj_create(s_modal_overlay);
  lv_obj_set_pos(nav, 2, 2);
  lv_obj_set_size(nav, 294, 28);
  lv_obj_set_style_bg_color(nav, lv_color_hex(0x1E293B), 0);
  lv_obj_set_style_border_width(nav, 0, 0);
  lv_obj_set_style_pad_all(nav, 2, 0);
  lv_obj_set_flex_flow(nav, LV_FLEX_FLOW_ROW);
  lv_obj_clear_flag(nav, LV_OBJ_FLAG_SCROLLABLE);

  struct NavItem { const char* lbl; SubModalTab t; };
  const NavItem items[6] = {
    {L_TAB_STAT, TAB_STATUS}, {L_TAB_BAG, TAB_BAG}, {L_TAB_JOBS, TAB_JOBS},
    {L_TAB_MAGIC, TAB_MAGIC}, {L_TAB_QUEST, TAB_QUEST}, {L_CLOSE, TAB_NONE}
  };
  for (int i = 0; i < 6; i++) {
    lv_obj_t* b = lv_btn_create(nav);
    lv_obj_set_size(b, (i == 5) ? 28 : 49, 22);
    uint32_t bg = (i == 5) ? 0x991B1B : ((tab == items[i].t) ? 0xD97706 : 0x334155);
    lv_obj_set_style_bg_color(b, lv_color_hex(bg), 0);
    lv_obj_set_style_pad_all(b, 0, 0);
    lv_obj_add_event_cb(b, on_modal_tab_switch, LV_EVENT_CLICKED, (void*)(intptr_t)items[i].t);
    lv_obj_t* l = lv_label_create(b);
    lv_label_set_text(l, items[i].lbl);
    lv_obj_set_style_text_font(l, &lv_font_montserrat_10, 0);
    lv_obj_center(l);
  }

  // Scrollable Body Area
  lv_obj_t* body = lv_obj_create(s_modal_overlay);
  lv_obj_set_pos(body, 2, 34);
  lv_obj_set_size(body, 294, 182);
  lv_obj_set_style_bg_color(body, lv_color_hex(0x0B1120), 0);
  lv_obj_set_style_border_width(body, 0, 0);
  lv_obj_set_style_pad_all(body, 6, 0);
  lv_obj_set_flex_flow(body, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_style_pad_row(body, 5, 0);

  const TamagotchiCoreEngine& eng = s_app->engine;

  if (tab == TAB_STATUS) {
    char jobTxt[56];
    snprintf(jobTxt, sizeof(jobTxt), LANG_DE ? "Beruf: keiner" : "Job: idle");
    if (s_app->activeJob.running) {
      uint32_t elapsed = cyd_millis() - s_app->activeJob.startedMs;
      uint32_t remS = (elapsed < s_app->activeJob.durationMs) ? ((s_app->activeJob.durationMs - elapsed) / 1000U) : 0;
      char jName[28] = "Job";
      if (s_app->activeJob.jobIndex < eng.rules.jobCount) {
        sanitize_lvgl_text(PICK_RULE_NAME(eng.rules.jobs[s_app->activeJob.jobIndex]), jName, sizeof(jName));
      }
      snprintf(jobTxt, sizeof(jobTxt), "%s: %lus", jName, (unsigned long)remS);
    }
    const char* linkLbl = s_app->offlineMode ? "Offline" : (strlen(s_app->friendPet.linkMode) > 0 ? s_app->friendPet.linkMode : "NET");
    const char* rulesVer = (strlen(eng.rules.version) > 0) ? eng.rules.version : RULES_VERSION;
    char petSafeName[20];
    sanitize_lvgl_text(eng.name, petSafeName, sizeof(petSafeName));
    const char* seasonLbl = eng.season;
    if (LANG_DE) {
      if (strcmp(eng.season, "spring") == 0) seasonLbl = "Fruehling";
      else if (strcmp(eng.season, "summer") == 0) seasonLbl = "Sommer";
      else if (strcmp(eng.season, "autumn") == 0) seasonLbl = "Herbst";
      else if (strcmp(eng.season, "winter") == 0) seasonLbl = "Winter";
    }
    char cloudTxt[48];
    if (s_app->offlineMode) {
      snprintf(cloudTxt, sizeof(cloudTxt), L_CLD_OFFLINE);
    } else if (s_app->cloudSyncOk && s_app->lastCloudSaveMs > 0) {
      uint32_t agoSec = (cyd_millis() - s_app->lastCloudSaveMs) / 1000U;
      snprintf(cloudTxt, sizeof(cloudTxt), L_CLD_SYNCED, (unsigned long)agoSec);
    } else if (s_app->lastCloudSaveEpoch > 0) {
      snprintf(cloudTxt, sizeof(cloudTxt), L_CLD_SAVED_NVS, (unsigned long)s_app->lastCloudSaveEpoch);
    } else {
      snprintf(cloudTxt, sizeof(cloudTxt), L_CLD_NONE);
    }

    lv_obj_t* lbl = lv_label_create(body);
    lv_label_set_text_fmt(lbl,
      "%s (Lv.%u %s/%s) | $%u | %s\n"
      "HP:%u%% %s:%u%% EN:%u%% SA:%u%%\n"
      "%s:%u%% MP:%u/%u | CM:%u Poop:%u\n"
      "%s: %s | %s: %uT | LDR:%u\n"
      "Head:%s\n%s\n%s\nRules:v%s FW:v%s",
      petSafeName, (unsigned)eng.level, eng.phase, eng.evoBranch, (unsigned)eng.coins, linkLbl,
      (unsigned)eng.stats.health, LANG_DE ? "Satt" : "Food", (unsigned)eng.stats.hunger,
      (unsigned)eng.stats.energy, (unsigned)eng.stats.hygiene,
      LANG_DE ? "Fr" : "Hap", (unsigned)eng.stats.happiness, (unsigned)eng.stats.mana,
      (unsigned)eng.getEffectiveMaxMana(), (unsigned)eng.careMistakes, (unsigned)eng.poopCount,
      LANG_DE ? "Saison" : "Season", seasonLbl, LANG_DE ? "Schutz" : "Shield",
      (unsigned)eng.satietyTicks, (unsigned)s_app->ambientLdr,
      strlen(eng.equippedHead) > 0 ? eng.equippedHead : "-",
      jobTxt, cloudTxt, rulesVer, FIRMWARE_VERSION);
    lv_obj_set_style_text_color(lbl, lv_color_hex(0xE2E8F0), 0);

    if (strcmp(eng.phase, "Egg") != 0) {
      add_modal_btn(body, eng.isSleeping ? L_BTN_SLEEP_OFF : L_BTN_SLEEP_ON,
                    eng.isSleeping ? 0x0E7490 : 0x4338CA, on_toggle_sleep_cb, 0);
    }
    add_modal_btn(body, L_BTN_CLOUD_SAVE, 0x0369A1, on_save_cloud_cb, 0);
    if (!s_app->offlineMode) {
      add_modal_btn(body, L_BTN_CLOUD_LOAD, 0x1E40AF, on_load_cloud_cb, 0);
    }
  } else if (tab == TAB_BAG) {
    for (uint8_t i = 0; i < eng.inventoryCount; i++) {
      const EngineInventorySlot& sl = eng.inventory[i];
      char safeName[28], rowTxt[64];
      sanitize_lvgl_text(PICK_RULE_NAME(sl), safeName, sizeof(safeName));
      const char* tag = L_TAG_MAT;
      uint32_t col = 0x334155;
      if (sl.category == 0) {
        tag = L_TAG_FOOD;
        col = 0x065F46;
      } else if (sl.category == 2) {
        tag = L_TAG_USE;
        col = 0x9F1239;
      } else if (sl.category == 3) {
        bool equipped = (strcmp(eng.equippedHead, sl.id) == 0);
        tag = equipped ? L_TAG_EQUIPPED : L_TAG_EQUIP;
        col = equipped ? 0x6D28D9 : 0x4338CA;
      }
      snprintf(rowTxt, sizeof(rowTxt), "%s x%u %s", safeName, sl.quantity, tag);
      add_modal_btn(body, rowTxt, col, on_bag_action_cb, (int)i);
    }
    for (uint8_t r = 0; r < eng.rules.recipeCount; r++) {
      const EngineRecipe& rec = eng.rules.recipes[r];
      char rName[28], rowTxt[64];
      sanitize_lvgl_text(PICK_RULE_NAME(rec), rName, sizeof(rName));
      snprintf(rowTxt, sizeof(rowTxt), "%s: %s (Lv%u)", LANG_DE ? "Kochen" : "Cook", rName, (unsigned)rec.minLevel);
      uint32_t col = (eng.level >= rec.minLevel) ? 0x7C2D12 : 0x451A03;
      add_modal_btn(body, rowTxt, col, on_bag_action_cb, 100 + (int)r);
    }
    for (uint8_t s = 0; s < eng.rules.shopItemCount; s++) {
      const EngineShopItem& sh = eng.rules.shopItems[s];
      if (sh.category > 3) continue; // Food, Material, Potion & Accessories
      char sName[28], rowTxt[64];
      sanitize_lvgl_text(PICK_RULE_NAME(sh), sName, sizeof(sName));
      snprintf(rowTxt, sizeof(rowTxt), "Shop: %s ($%u, Lv%u)", sName, (unsigned)sh.cost, (unsigned)sh.minLevel);
      uint32_t col = (eng.coins >= sh.cost && eng.level >= sh.minLevel) ? 0x854D0E : 0x422006;
      add_modal_btn(body, rowTxt, col, on_bag_action_cb, 200 + (int)s);
    }
    for (uint8_t sr = 0; sr < eng.rules.shopRecipeCount; sr++) {
      const EngineShopRecipe& rec = eng.rules.shopRecipes[sr];
      char rName[24], rowTxt[64];
      sanitize_lvgl_text(PICK_RULE_NAME(rec), rName, sizeof(rName));
      bool owned = eng.isRecipePurchased(rec.id);
      snprintf(rowTxt, sizeof(rowTxt), "%s: %s %s",
               LANG_DE ? "Rezept" : "Blueprint", rName, owned ? "[OK]" : "");
      if (!owned) {
        snprintf(rowTxt, sizeof(rowTxt), "%s: %s ($%u, Lv%u)",
                 LANG_DE ? "Rezept" : "Blueprint", rName, (unsigned)rec.cost, (unsigned)rec.minLevel);
      }
      uint32_t col = owned ? 0x334155 : ((eng.coins >= rec.cost && eng.level >= rec.minLevel) ? 0x9A3412 : 0x451A03);
      add_modal_btn(body, rowTxt, col, on_bag_action_cb, 300 + (int)sr);
    }
    for (uint8_t ss = 0; ss < eng.rules.shopSpellCount; ss++) {
      const EngineShopSpell& sp = eng.rules.shopSpells[ss];
      char spName[24], rowTxt[64];
      sanitize_lvgl_text(PICK_RULE_NAME(sp), spName, sizeof(spName));
      bool owned = eng.isSpellPurchased(sp.id);
      if (owned) {
        snprintf(rowTxt, sizeof(rowTxt), "%s: %s [OK]", LANG_DE ? "Buch" : "Tome", spName);
      } else {
        snprintf(rowTxt, sizeof(rowTxt), "%s: %s ($%u, Lv%u)",
                 LANG_DE ? "Buch" : "Tome", spName, (unsigned)sp.cost, (unsigned)sp.minLevel);
      }
      uint32_t col = owned ? 0x334155 : ((eng.coins >= sp.cost && eng.level >= sp.minLevel) ? 0x581C87 : 0x2E1065);
      add_modal_btn(body, rowTxt, col, on_bag_action_cb, 400 + (int)ss);
    }
  } else if (tab == TAB_JOBS) {
    if (s_app->activeJob.running) {
      uint32_t elapsed = cyd_millis() - s_app->activeJob.startedMs;
      uint32_t remS = (elapsed < s_app->activeJob.durationMs) ? ((s_app->activeJob.durationMs - elapsed) / 1000U) : 0;
      lv_obj_t* l = lv_label_create(body);
      lv_label_set_text_fmt(l, LANG_DE ? "Beruf aktiv - Restzeit: %lus" : "Job active - Left: %lus", (unsigned long)remS);
      lv_obj_set_style_text_color(l, lv_color_hex(0xFACC15), 0);

      lv_obj_t* jb = lv_bar_create(body);
      lv_obj_set_size(jb, 268, 12);
      lv_bar_set_range(jb, 0, 100);
      int pct = (s_app->activeJob.durationMs > 0) ? (int)((elapsed * 100U) / s_app->activeJob.durationMs) : 100;
      if (pct > 100) pct = 100;
      lv_bar_set_value(jb, pct, LV_ANIM_OFF);
    }
    static const uint32_t JOB_COLS[4] = { 0x166534, 0x1E40AF, 0x6B21A8, 0x9A3412 };
    for (uint8_t j = 0; j < eng.rules.jobCount; j++) {
      const EngineJob& job = eng.rules.jobs[j];
      char jName[28], rowTxt[64];
      sanitize_lvgl_text(PICK_RULE_NAME(job), jName, sizeof(jName));
      snprintf(rowTxt, sizeof(rowTxt), "%s (%us -%uEN +%u$)",
               jName, (unsigned)job.durationSec, (unsigned)job.energyCost, (unsigned)job.coinReward);
      uint32_t col = (eng.level >= job.minLevel) ? JOB_COLS[j % 4] : 0x334155;
      add_modal_btn(body, rowTxt, col, on_job_start_cb, (int)j);
    }
  } else if (tab == TAB_MAGIC) {
    lv_obj_t* mpL = lv_label_create(body);
    lv_label_set_text_fmt(mpL,
                          LANG_DE ? "Mana: %u / %u | Energie: %u / 100" : "Mana: %u / %u | Energy: %u / 100",
                          (unsigned)eng.stats.mana, (unsigned)eng.getEffectiveMaxMana(), (unsigned)eng.stats.energy);
    lv_obj_set_style_text_color(mpL, lv_color_hex(0xC084FC), 0);
    static const uint32_t SPELL_COLS[5] = { 0x047857, 0x6D28D9, 0x9333EA, 0x0369A1, 0xB45309 };
    for (uint8_t m = 0; m < eng.rules.spellCount; m++) {
      const EngineSpell& sp = eng.rules.spells[m];
      char spName[28], rowTxt[64];
      sanitize_lvgl_text(PICK_RULE_NAME(sp), spName, sizeof(spName));
      snprintf(rowTxt, sizeof(rowTxt), "%u. %s (%uMP, Lv%u)",
               (unsigned)(m + 1), spName, (unsigned)sp.manaCost, (unsigned)sp.minLevel);
      uint32_t col = (eng.level >= sp.minLevel) ? SPELL_COLS[m % 5] : 0x334155;
      add_modal_btn(body, rowTxt, col, on_magic_action_cb, (int)m);
    }
    add_modal_btn(body,
                  LANG_DE ? "Magie erforschen (-15EN -> +15MP +35XP)" : "Research Magic (-15EN -> +15MP +35XP)",
                  0x1D4ED8, on_magic_action_cb, 99);
  } else if (tab == TAB_QUEST) {
    if (eng.canClaimStreakBonus()) {
      char sTxt[48];
      snprintf(sTxt, sizeof(sTxt),
               LANG_DE ? "Streak Tag %u abholen! (+Coins/+XP)" : "Claim Streak Day %u! (+Coins/+XP)",
               eng.consecutiveLoginDays);
      add_modal_btn(body, sTxt, 0xD97706, on_quest_action_cb, 0);
    }
    bool hasClaimable = false;
    for (uint8_t m = 0; m < eng.missionCount; m++) {
      if (eng.missions[m].completed && !eng.missions[m].claimed) {
        hasClaimable = true;
        break;
      }
    }
    if (hasClaimable) {
      add_modal_btn(body, LANG_DE ? "Tages-Mission einloesen!" : "Claim Daily Mission!", 0x15803D, on_quest_action_cb, 1);
    }
    for (uint8_t m = 0; m < eng.missionCount; m++) {
      const EngineDailyMission& dm = eng.missions[m];
      char mName[28];
      sanitize_lvgl_text(PICK_RULE_NAME(dm), mName, sizeof(mName));
      lv_obj_t* ml = lv_label_create(body);
      const char* statusTag = dm.claimed ? "[OK]" : (dm.completed ? (LANG_DE ? "[FERTIG!]" : "[READY!]") : "");
      lv_label_set_text_fmt(ml, "%s (%u/%u) %s", mName, (unsigned)dm.currentCount, (unsigned)dm.targetCount, statusTag);
      lv_obj_set_style_text_font(ml, &lv_font_montserrat_10, 0);
      lv_obj_set_style_text_color(ml, lv_color_hex(dm.claimed ? 0x94A3B8 : (dm.completed ? 0x4ADE80 : 0xE2E8F0)), 0);
    }
    add_modal_btn(body,
                  LANG_DE ? "Koop-Quest: Spaeher-Patrouille (+35$)" : "Co-op Quest: Scout Patrol (+35$)",
                  0x1E40AF, on_quest_action_cb, 2);
    add_modal_btn(body,
                  LANG_DE ? "Koop-Quest: Schatzsuche (+50$ Apfel)" : "Co-op Quest: Treasure Hunt (+50$ Apple)",
                  0x065F46, on_quest_action_cb, 3);
  } else if (tab == TAB_CHAT) {
    lv_obj_t* hdr = lv_label_create(body);
    lv_label_set_text(hdr, LANG_DE ? "Nachricht an Freund senden:" : "Send message to friend:");
    lv_obj_set_style_text_color(hdr, lv_color_hex(0x38BDF8), 0);
    for (int c = 0; c < 6; c++) {
      add_modal_btn(body, CHAT_PHRASES[c], 0x0E7490, on_chat_phrase_cb, c);
    }
  } else if (tab == TAB_TRADE) {
    if (s_app->incomingTrade.active) {
      lv_obj_t* inL = lv_label_create(body);
      lv_label_set_text_fmt(inL,
                            LANG_DE ? "Angebot von %s:\nBietet: %s\nGegen: %s" : "Offer from %s:\nOffers: %s\nFor: %s",
                            s_app->incomingTrade.fromDevice,
                            s_app->incomingTrade.offeredName,
                            strlen(s_app->incomingTrade.requestedName) > 0
                              ? s_app->incomingTrade.requestedName
                              : (LANG_DE ? "Geschenk" : "Gift"));
      lv_obj_set_style_text_color(inL, lv_color_hex(0xFACC15), 0);
      add_modal_btn(body, LANG_DE ? "Tausch ANNEHMEN (+35XP)" : "ACCEPT Trade (+35XP)", 0x15803D, on_trade_accept_cb, 0);
      add_modal_btn(body, LANG_DE ? "Tausch ABLEHNEN" : "DECLINE Trade", 0x991B1B, on_trade_decline_cb, 0);
    } else if (s_app->outgoingTrade.active) {
      lv_obj_t* outL = lv_label_create(body);
      lv_label_set_text_fmt(outL,
                            LANG_DE ? "Escrow aktiv:\nDu bietest: %s\nWunsch: %s" : "Escrow active:\nOffering: %s\nSeeking: %s",
                            s_app->outgoingTrade.offeredName,
                            s_app->outgoingTrade.requestedName);
      lv_obj_set_style_text_color(outL, lv_color_hex(0x38BDF8), 0);
      add_modal_btn(body, LANG_DE ? "Angebot zurueckziehen" : "Cancel Offer", 0x991B1B, on_trade_cancel_outgoing_cb, 0);
    } else {
      uint8_t wIdx = s_app->tradeSelectedWantIdx % 5;
      char wantBtn[48];
      snprintf(wantBtn, sizeof(wantBtn), LANG_DE ? "Wunsch: %s (Tippen)" : "Want: %s (Tap)", TRADE_WANT_NAMES[wIdx]);
      add_modal_btn(body, wantBtn, 0x334155, on_trade_want_cycle_cb, 0);

      for (uint8_t i = 0; i < eng.inventoryCount; i++) {
        char iName[28], rowTxt[56];
        sanitize_lvgl_text(PICK_RULE_NAME(eng.inventory[i]), iName, sizeof(iName));
        snprintf(rowTxt, sizeof(rowTxt), LANG_DE ? "Bieten: %s (x%u)" : "Offer: %s (x%u)",
                 iName, (unsigned)eng.inventory[i].quantity);
        add_modal_btn(body, rowTxt, 0xB45309, on_trade_send_offer_cb, (int)i);
      }
    }
  }
}
