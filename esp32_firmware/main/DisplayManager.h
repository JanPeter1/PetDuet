/**
 * ============================================================================
 * PetDuet ESP32 CYD - Display Manager
 * ============================================================================
 * High-level display management using PetGraphics
 * ============================================================================
 */
#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_io.h"
#include "PetGraphics.h"
#include "TamagotchiCoreEngine.h"
#include "CYD_Pins.h"

// ============================================================================
// Screen States
// ============================================================================
typedef enum {
    SCREEN_BOOT = 0,
    SCREEN_WIFI_CONNECTING,
    SCREEN_MAIN,
    SCREEN_RPG_MENU,
    SCREEN_INVENTORY,
    SCREEN_SHOP,
    SCREEN_SPELLS,
    SCREEN_MISSIONS,
    SCREEN_TRADE,
    SCREEN_SETTINGS,
    SCREEN_CALIBRATION,
    SCREEN_ERROR,
    SCREEN_OTA_UPDATE
} ScreenState;

// ============================================================================
// Display Manager Context
// ============================================================================
typedef struct {
    esp_lcd_panel_handle_t panel;
    esp_lcd_panel_io_handle_t io;

    ScreenState current_screen;
    ScreenState previous_screen;

    // Pet references
    PetState* pet;
    PetState* partner_pet;
    const RulesData* rules;
    const BiomeTheme* biome_theme;
    const BiomeTheme* partner_biome_theme;

    // UI State
    uint8_t selected_left_btn;
    uint8_t selected_right_btn;
    uint8_t selected_menu_item;
    bool needs_redraw;
    bool partial_update;

    // Animation
    uint32_t last_frame_time;
    uint8_t animation_frame;

    // Touch
    int16_t last_touch_x;
    int16_t last_touch_y;
    bool touch_pressed;

    // Backlight
    uint8_t backlight_level;
    uint32_t last_activity_time;
    bool dimmed;

    // Error display
    char error_message[128];
} DisplayManager;

// ============================================================================
// API
// ============================================================================

// Initialization
esp_err_t display_mgr_init(DisplayManager* dm, PetState* pet, PetState* partner_pet,
                           const RulesData* rules);
void display_mgr_deinit(DisplayManager* dm);

// Main loop
void display_mgr_update(DisplayManager* dm, uint32_t delta_ms);
void display_mgr_render(DisplayManager* dm);

// Screen management
void display_mgr_set_screen(DisplayManager* dm, ScreenState screen);
ScreenState display_mgr_get_screen(DisplayManager* dm);

// Touch handling
void display_mgr_handle_touch(DisplayManager* dm, int16_t x, int16_t y, bool pressed);

// Backlight
void display_mgr_set_backlight(DisplayManager* dm, uint8_t level);
void display_mgr_dim(DisplayManager* dm, bool dim);

// Notifications
void display_mgr_show_notification(DisplayManager* dm, const char* message, uint32_t duration_ms);
void display_mgr_show_error(DisplayManager* dm, const char* error);

// Specific screen renderers
void render_boot_screen(DisplayManager* dm);
void render_main_screen(DisplayManager* dm);
void render_rpg_menu(DisplayManager* dm);
void render_inventory(DisplayManager* dm);
void render_shop(DisplayManager* dm);
void render_spells(DisplayManager* dm);
void render_missions(DisplayManager* dm);
void render_trade(DisplayManager* dm);
void render_settings(DisplayManager* dm);
void render_ota_screen(DisplayManager* dm, const char* status, uint8_t progress);

// Helper
const char* get_localized_string(const char* en, const char* de);

#endif // DISPLAY_MANAGER_H