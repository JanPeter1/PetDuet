/**
 * ============================================================================
 * PetDuet ESP32 CYD - Pet Graphics Rendering
 * ============================================================================
 * Procedural 16x16 pixel-art sprites with biome theming
 * TFT_eSPI style API adapted for ESP-IDF esp_lcd
 * ============================================================================
 */
#ifndef PET_GRAPHICS_H
#define PET_GRAPHICS_H

#include <stdint.h>
#include "CYD_Pins.h"
#include "config.h"

// Include ESP-IDF LCD types (contains esp_lcd_panel_io_handle_t, esp_lcd_panel_handle_t)
#include "esp_lcd_types.h"

// ============================================================================
// Color Definitions (RGB565)
// ============================================================================
#define TFT_BLACK       0x0000
#define TFT_NAVY        0x000F
#define TFT_DARKGREEN   0x03E0
#define TFT_DARKCYAN    0x03EF
#define TFT_MAROON      0x7800
#define TFT_PURPLE      0x780F
#define TFT_OLIVE       0x7BE0
#define TFT_LIGHTGREY   0xC618
#define TFT_DARKGREY    0x7BEF
#define TFT_BLUE        0x001F
#define TFT_GREEN       0x07E0
#define TFT_CYAN        0x07FF
#define TFT_RED         0xF800
#define TFT_MAGENTA     0xF81F
#define TFT_YELLOW      0xFFE0
#define TFT_WHITE       0xFFFF
#define TFT_ORANGE      0xFD20
#define TFT_GREENYELLOW 0xAFE5
#define TFT_PINK        0xF81F
#define TFT_TEAL        0x0410

// ============================================================================
// Biome Theme Structure
// ============================================================================
typedef struct {
    const char* name;
    const char* name_de;
    uint16_t bg_color;
    uint16_t accent_color;
    uint16_t pet_base_color;
    uint16_t pet_accent_color;
    uint16_t ui_panel_color;
    uint16_t ui_border_color;
    uint16_t text_color;
    uint16_t text_shadow_color;
    uint8_t  rgb565_r;
    uint8_t  rgb565_g;
    uint8_t  rgb565_b;
    const char* trait;
    const char* trait_de;
} BiomeTheme;

// ============================================================================
// Pet Animation State Enum (for sprite rendering)
// ============================================================================
typedef enum {
    PET_ANIM_IDLE = 0,
    PET_ANIM_HAPPY,
    PET_ANIM_EATING,
    PET_ANIM_SLEEPING,
    PET_ANIM_SICK,
    PET_ANIM_DEAD,
    PET_ANIM_PLAYING,
    PET_ANIM_BATHING
} PetAnimState;

// ============================================================================
// Sprite Structure (16x16 procedural pixel art)
// ============================================================================
typedef struct {
    uint16_t width;
    uint16_t height;
    const uint16_t* pixels;  // RGB565 pixel data
} Sprite16;

// ============================================================================
// UI Button Structure
// ============================================================================
typedef struct {
    int16_t x, y, w, h;
    const char* label;
    const char* label_de;
    uint16_t bg_color;
    uint16_t text_color;
    uint16_t border_color;
    bool pressed;
    bool enabled;
} UIButton;

// ============================================================================
// Function Declarations
// ============================================================================

// Display initialization
esp_err_t display_init(esp_lcd_panel_handle_t* panel_handle, esp_lcd_panel_io_handle_t* io_handle);
void display_deinit(esp_lcd_panel_handle_t panel_handle, esp_lcd_panel_io_handle_t io_handle);
void display_set_backlight(uint8_t brightness);

// Biome themes
const BiomeTheme* get_biome_theme(const char* biome_name);
uint16_t get_biome_rgb565(const char* biome_name);

// Sprite rendering (procedural 16x16)
void draw_pet_sprite(esp_lcd_panel_handle_t panel, int16_t x, int16_t y,
                     PetAnimState state, const BiomeTheme* theme, bool is_partner);
void draw_pet_sprite_scaled(esp_lcd_panel_handle_t panel, int16_t x, int16_t y,
                            PetAnimState state, const BiomeTheme* theme,
                            bool is_partner, uint8_t scale);

// UI Drawing
void draw_top_bar(esp_lcd_panel_handle_t panel, int level, int jobs, int magic,
                  int cook, bool online, bool partner_online);
void draw_left_actions(esp_lcd_panel_handle_t panel);
void draw_right_actions(esp_lcd_panel_handle_t panel);
void draw_bottom_hud(esp_lcd_panel_handle_t panel, uint32_t coins, uint8_t hp_pct,
                     uint8_t energy_pct, uint32_t partner_coins,
                     uint8_t partner_hp_pct, uint8_t partner_energy_pct,
                     const char* season);
void draw_sync_indicator(esp_lcd_panel_handle_t panel, bool synced);
void draw_button(esp_lcd_panel_handle_t panel, const UIButton* btn);
void draw_text(esp_lcd_panel_handle_t panel, int16_t x, int16_t y,
               const char* text, uint16_t color, uint16_t bg_color, uint8_t size);
void draw_centered_text(esp_lcd_panel_handle_t panel, int16_t y,
                        const char* text, uint16_t color, uint16_t bg_color, uint8_t size);

// Utility
uint16_t rgb888_to_rgb565(uint8_t r, uint8_t g, uint8_t b);
void fill_rect(esp_lcd_panel_handle_t panel, int16_t x, int16_t y,
               int16_t w, int16_t h, uint16_t color);
void draw_rect(esp_lcd_panel_handle_t panel, int16_t x, int16_t y,
               int16_t w, int16_t h, uint16_t color);
void draw_fast_hline(esp_lcd_panel_handle_t panel, int16_t x, int16_t y,
                     int16_t w, uint16_t color);
void draw_fast_vline(esp_lcd_panel_handle_t panel, int16_t x, int16_t y,
                     int16_t h, uint16_t color);
void draw_pixel(esp_lcd_panel_handle_t panel, int16_t x, int16_t y, uint16_t color);

// Touch calibration
void draw_calibration_screen(esp_lcd_panel_handle_t panel);
bool touch_calibrate(esp_lcd_panel_handle_t panel);

// Screen helpers
void clear_screen(esp_lcd_panel_handle_t panel, uint16_t color);
void show_boot_screen(esp_lcd_panel_handle_t panel, const char* partition_label);
void show_wifi_connecting(esp_lcd_panel_handle_t panel, const char* ssid);
void show_error_screen(esp_lcd_panel_handle_t panel, const char* error);

#endif // PET_GRAPHICS_H