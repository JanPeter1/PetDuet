/**
 * ============================================================================
 * PetDuet ESP32 CYD - Pet Graphics Implementation
 * ============================================================================
 * Procedural 16x16 pixel-art sprites with biome theming
 * Uses esp_lcd panel interface for rendering
 * ============================================================================
 */
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include "esp_log.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_commands.h"
#include "esp_partition.h"
#include "esp_ota_ops.h"
#include "driver/ledc.h"
#include "driver/spi_master.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "PetGraphics.h"
#include "CYD_Pins.h"
#include "cyd_font_5x7.h"

static const char* TAG = "PetGraphics";
static const size_t LCD_COLOR_CHUNK_ROWS = 8;
static uint16_t s_rgb565_swap_buf[TFT_WIDTH * LCD_COLOR_CHUNK_ROWS];
static SemaphoreHandle_t s_color_transfer_sem = NULL;
static uint16_t s_text_glyph_buf[6 * 3 * 7 * 3];

static bool IRAM_ATTR lcd_color_transfer_done_cb(esp_lcd_panel_io_handle_t /*panel_io*/,
                                                  esp_lcd_panel_io_event_data_t* /*edata*/,
                                                  void* user_ctx) {
    BaseType_t high_task_woken = pdFALSE;
    xSemaphoreGiveFromISR((SemaphoreHandle_t)user_ctx, &high_task_woken);
    return high_task_woken == pdTRUE;
}

static void lcd_wait_for_color_transfer(void) {
    xSemaphoreTake(s_color_transfer_sem, portMAX_DELAY);
}

// ============================================================================
// Biome Themes
// ============================================================================
static const BiomeTheme s_biome_themes[] = {
    {
        .name = "grassland",
        .name_de = "Grasland",
        .bg_color = 0x8F5F,      // Light green
        .accent_color = 0x07E0,  // Green
        .pet_base_color = 0x04A0,  // Dark green
        .pet_accent_color = 0x8F5F, // Light green
        .ui_panel_color = 0x7BEF,  // Dark grey-green
        .ui_border_color = 0x07E0,
        .text_color = 0xFFFF,
        .text_shadow_color = 0x0000,
        .rgb565_r = 0x00,
        .rgb565_g = 0x80,
        .rgb565_b = 0x00,
        .trait = "Balanced",
        .trait_de = "Ausgeglichen"
    },
    {
        .name = "desert",
        .name_de = "Wüste",
        .bg_color = 0xF7BE,      // Sand
        .accent_color = 0xFD20,  // Orange
        .pet_base_color = 0xD680, // Tan
        .pet_accent_color = 0xF7BE, // Light sand
        .ui_panel_color = 0xBE9F, // Dark sand
        .ui_border_color = 0xFD20,
        .text_color = 0xFFFF,
        .text_shadow_color = 0x0000,
        .rgb565_r = 0xE0,
        .rgb565_g = 0x80,
        .rgb565_b = 0x00,
        .trait = "Hardy",
        .trait_de = "Robust"
    },
    {
        .name = "water",
        .name_de = "Wasser",
        .bg_color = 0x051F,      // Dark blue
        .accent_color = 0x07FF,  // Cyan
        .pet_base_color = 0x029F, // Blue
        .pet_accent_color = 0x05FF, // Light blue
        .ui_panel_color = 0x037F, // Dark blue-grey
        .ui_border_color = 0x07FF,
        .text_color = 0xFFFF,
        .text_shadow_color = 0x0000,
        .rgb565_r = 0x00,
        .rgb565_g = 0x00,
        .rgb565_b = 0xE0,
        .trait = "Fluid",
        .trait_de = "Flüssig"
    },
    {
        .name = "forest",
        .name_de = "Wald",
        .bg_color = 0x2444,      // Forest green
        .accent_color = 0x07E0,  // Green
        .pet_base_color = 0x1CC0, // Dark forest
        .pet_accent_color = 0x2CC4, // Moss
        .ui_panel_color = 0x18A3, // Dark forest grey
        .ui_border_color = 0x07E0,
        .text_color = 0xFFFF,
        .text_shadow_color = 0x0000,
        .rgb565_r = 0x00,
        .rgb565_g = 0x60,
        .rgb565_b = 0x00,
        .trait = "Nurturing",
        .trait_de = "Fürsorglich"
    }
};

static const int s_biome_count = sizeof(s_biome_themes) / sizeof(BiomeTheme);

// ============================================================================
// Procedural Sprite Data (16x16 = 256 pixels each)
// Stored as bitmasks for each biome color layer
// ============================================================================

// Base 16x16 pet shape (bitmask: 1 = pet pixel, 0 = transparent)
// 16 rows, each row is 16 bits (2 bytes)
static const uint16_t s_pet_idle_mask[16] = {
    0x07E0,  // .... ...... .... ....
    0x0FF0,  // .... ...... ...... ....
    0x1FF8,  // .... ...... ...... ....
    0x3FFC,  // .... ...... ...... ....
    0x7FFE,  // .... ...... ...... ....
    0xFFFF,  // .... ...... ...... ....
    0xFFFF,  // .... ...... ...... ....
    0xE777,  // .... ...... ...... ....
    0xC333,  // .... ...... ...... ....
    0xC333,  // .... ...... ...... ....
    0xC333,  // .... ...... ...... ....
    0xC333,  // .... ...... ...... ....
    0xC333,  // .... ...... ...... ....
    0xE777,  // .... ...... ...... ....
    0x7FFE,  // .... ...... ...... ....
    0x3FFC   // .... ...... ...... ....
};

static const uint16_t s_pet_happy_mask[16] = {
    0x07E0,
    0x0FF0,
    0x1FF8,
    0x3FFC,
    0x7FFE,
    0xFFFF,
    0xFFFF,
    0xE777,
    0xC333,
    0xC333,
    0xC773,  // Wider smile
    0xC773,
    0xC333,
    0xE777,
    0x7FFE,
    0x3FFC
};

static const uint16_t s_pet_eating_mask[16] = {
    0x07E0,
    0x0FF0,
    0x1FF8,
    0x3FFC,
    0x7FFE,
    0xFFFF,
    0xF77F,  // Mouth open
    0xE337,
    0xC113,
    0xC113,
    0xC113,
    0xC113,
    0xC333,
    0xE777,
    0x7FFE,
    0x3FFC
};

static const uint16_t s_pet_sleeping_mask[16] = {
    0x07E0,
    0x0FF0,
    0x1FF8,
    0x3FFC,
    0x7FFE,
    0xFFFF,
    0xFFFF,
    0xE777,
    0xC333,  // Eyes closed (zzz)
    0xC333,
    0xC333,
    0xC333,
    0xC333,
    0xE777,
    0x7FFE,
    0x3FFC
};

static const uint16_t s_pet_sick_mask[16] = {
    0x07E0,
    0x0FF0,
    0x1FF8,
    0x3FFC,
    0x7FFE,
    0xFFFF,
    0xFFFF,
    0xE777,
    0xC553,  // X eyes
    0xC333,
    0xC333,
    0xC333,
    0xC333,
    0xE777,
    0x7FFE,
    0x3FFC
};

static const uint16_t s_pet_dead_mask[16] = {
    0x07E0,
    0x0FF0,
    0x1FF8,
    0x3FFC,
    0x7FFE,
    0xFFFF,
    0xFFFF,
    0xE777,
    0xC993,  // X_X eyes
    0xC333,
    0xC333,
    0xC333,
    0xC333,
    0xE777,
    0x7FFE,
    0x3FFC
};

static const uint16_t s_pet_playing_mask[16] = {
    0x07E0,
    0x0FF0,
    0x1FF8,
    0x3FFC,
    0x7FFE,
    0xFFFF,
    0xFFFF,
    0xEF7F,  // Arms up
    0xC773,
    0xC773,
    0xC773,
    0xC333,
    0xC333,
    0xE777,
    0x7FFE,
    0x3FFC
};

static const uint16_t s_pet_bathing_mask[16] = {
    0x07E0,
    0x0FF0,
    0x1FF8,
    0x3FFC,
    0x7FFE,
    0xFFFF,
    0xFFFF,
    0xE777,
    0xC333,
    0xC333,
    0xC333,
    0xC333,
    0xE777,
    0xE777,  // Bubbles
    0x7FFE,
    0x3FFC
};

// Get mask for state
static const uint16_t* get_pet_mask(PetAnimState state) {
    switch (state) {
        case PET_ANIM_IDLE:      return s_pet_idle_mask;
        case PET_ANIM_HAPPY:     return s_pet_happy_mask;
        case PET_ANIM_EATING:    return s_pet_eating_mask;
        case PET_ANIM_SLEEPING:  return s_pet_sleeping_mask;
        case PET_ANIM_SICK:      return s_pet_sick_mask;
        case PET_ANIM_DEAD:      return s_pet_dead_mask;
        case PET_ANIM_PLAYING:   return s_pet_playing_mask;
        case PET_ANIM_BATHING:   return s_pet_bathing_mask;
        default:                 return s_pet_idle_mask;
    }
}

// ============================================================================
// Display Driver Handles
// ============================================================================
static esp_lcd_panel_handle_t s_panel = NULL;
static esp_lcd_panel_io_handle_t s_io = NULL;

// ============================================================================
// ILI9341 Commands
// ============================================================================
#define ILI9341_SWRESET         0x01
#define ILI9341_SLPOUT          0x11
#define ILI9341_SLPIN           0x10
#define ILI9341_DISPON          0x29
#define ILI9341_DISPOFF         0x28
#define ILI9341_CASET           0x2A
#define ILI9341_RASET           0x2B
#define ILI9341_RAMWR           0x2C
#define ILI9341_MADCTL          0x36
#define ILI9341_COLMOD          0x3A
#define ILI9341_INVON           0x21
#define ILI9341_INVOFF          0x20

#define ILI9341_PWCTR1          0xC0
#define ILI9341_PWCTR2          0xC1
#define ILI9341_VMCTR1          0xC5
#define ILI9341_VMCTR2          0xC7

#define ILI9341_MADCTL_MY       0x80
#define ILI9341_MADCTL_MX       0x40
#define ILI9341_MADCTL_MV       0x20
#define ILI9341_MADCTL_BGR      0x08

#define ILI9341_MADCTL_DEFAULT  (ILI9341_MADCTL_MY | ILI9341_MADCTL_BGR)

// ============================================================================
// Low-level Drawing Functions (using IO handle directly)
// ============================================================================
static void lcd_draw_bitmap(int16_t x, int16_t y, int16_t w, int16_t h, const uint16_t* data) {
    if (!s_io || !data || w <= 0 || h <= 0) return;
    if (w > TFT_WIDTH) {
        ESP_LOGE(TAG, "RGB565 bitmap width %d exceeds display width", w);
        return;
    }

    for (int16_t row = 0; row < h; row += LCD_COLOR_CHUNK_ROWS) {
        int16_t chunk_rows = h - row;
        if (chunk_rows > LCD_COLOR_CHUNK_ROWS) chunk_rows = LCD_COLOR_CHUNK_ROWS;
        size_t chunk_pixels = (size_t)w * chunk_rows;
        size_t source_offset = (size_t)row * w;
        for (size_t i = 0; i < chunk_pixels; i++) {
            uint16_t color = data[source_offset + i];
            s_rgb565_swap_buf[i] = (uint16_t)((color >> 8) | (color << 8));
        }

        int16_t chunk_y = y + row;
        int16_t chunk_y_end = chunk_y + chunk_rows - 1;
        ESP_ERROR_CHECK(esp_lcd_panel_io_tx_param(s_io, ILI9341_CASET, (uint8_t[]){
            (uint8_t)((x >> 8) & 0xFF), (uint8_t)(x & 0xFF),
            (uint8_t)(((x + w - 1) >> 8) & 0xFF), (uint8_t)((x + w - 1) & 0xFF)
        }, 4));
        ESP_ERROR_CHECK(esp_lcd_panel_io_tx_param(s_io, ILI9341_RASET, (uint8_t[]){
            (uint8_t)((chunk_y >> 8) & 0xFF), (uint8_t)(chunk_y & 0xFF),
            (uint8_t)((chunk_y_end >> 8) & 0xFF), (uint8_t)(chunk_y_end & 0xFF)
        }, 4));
        ESP_ERROR_CHECK(esp_lcd_panel_io_tx_color(s_io, ILI9341_RAMWR,
                                                   s_rgb565_swap_buf,
                                                   chunk_pixels * sizeof(uint16_t)));
        lcd_wait_for_color_transfer();
    }
}

static void lcd_fill_rect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
    if (!s_io || w <= 0 || h <= 0) return;
    if (w > TFT_WIDTH) {
        ESP_LOGE(TAG, "RGB565 fill width %d exceeds display width", w);
        return;
    }

    uint16_t swapped_color = (uint16_t)((color >> 8) | (color << 8));
    for (int16_t row = 0; row < h; row += LCD_COLOR_CHUNK_ROWS) {
        int16_t chunk_rows = h - row;
        if (chunk_rows > LCD_COLOR_CHUNK_ROWS) chunk_rows = LCD_COLOR_CHUNK_ROWS;
        size_t chunk_pixels = (size_t)w * chunk_rows;
        for (size_t i = 0; i < chunk_pixels; i++) {
            s_rgb565_swap_buf[i] = swapped_color;
        }

        int16_t chunk_y = y + row;
        int16_t chunk_y_end = chunk_y + chunk_rows - 1;
        ESP_ERROR_CHECK(esp_lcd_panel_io_tx_param(s_io, ILI9341_CASET, (uint8_t[]){
            (uint8_t)((x >> 8) & 0xFF), (uint8_t)(x & 0xFF),
            (uint8_t)(((x + w - 1) >> 8) & 0xFF), (uint8_t)((x + w - 1) & 0xFF)
        }, 4));
        ESP_ERROR_CHECK(esp_lcd_panel_io_tx_param(s_io, ILI9341_RASET, (uint8_t[]){
            (uint8_t)((chunk_y >> 8) & 0xFF), (uint8_t)(chunk_y & 0xFF),
            (uint8_t)((chunk_y_end >> 8) & 0xFF), (uint8_t)(chunk_y_end & 0xFF)
        }, 4));
        ESP_ERROR_CHECK(esp_lcd_panel_io_tx_color(s_io, ILI9341_RAMWR,
                                                   s_rgb565_swap_buf,
                                                   chunk_pixels * sizeof(uint16_t)));
        lcd_wait_for_color_transfer();
    }
}

// ============================================================================
// Display Initialization (inline ILI9341 driver)
// ============================================================================
esp_err_t display_init(esp_lcd_panel_handle_t* panel_handle, esp_lcd_panel_io_handle_t* io_handle) {
    ESP_LOGI(TAG, "Initializing ILI9341 display...");
    s_color_transfer_sem = xSemaphoreCreateBinary();
    if (!s_color_transfer_sem) return ESP_ERR_NO_MEM;

    // Panel IO configuration - using correct field order for ESP-IDF v5.4
    esp_lcd_panel_io_spi_config_t io_config;
    memset(&io_config, 0, sizeof(io_config));
    io_config.cs_gpio_num = (gpio_num_t)TFT_CS;
    io_config.dc_gpio_num = (gpio_num_t)TFT_DC;
    io_config.spi_mode = 0;
    io_config.pclk_hz = TFT_SPI_FREQ_HZ;
    io_config.trans_queue_depth = 10;
    io_config.on_color_trans_done = lcd_color_transfer_done_cb;
    io_config.user_ctx = s_color_transfer_sem;
    io_config.lcd_cmd_bits = 8;
    io_config.lcd_param_bits = 8;
    io_config.cs_ena_pretrans = 0;
    io_config.cs_ena_posttrans = 0;
    // flags sub-struct fields default to 0/false from memset
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)TFT_SPI_HOST, &io_config, io_handle));

    // Use the IO handle as panel handle (we'll implement ILI9341 commands directly)
    *panel_handle = (esp_lcd_panel_handle_t)*io_handle;

    // Initialize ILI9341 via direct SPI commands
    // Software reset
    ESP_ERROR_CHECK(esp_lcd_panel_io_tx_param(*io_handle, ILI9341_SWRESET, NULL, 0));
    vTaskDelay(pdMS_TO_TICKS(120));

    // Power control A
    ESP_ERROR_CHECK(esp_lcd_panel_io_tx_param(*io_handle, ILI9341_PWCTR1, (uint8_t[]){0x23}, 1));
    // Power control B
    ESP_ERROR_CHECK(esp_lcd_panel_io_tx_param(*io_handle, ILI9341_PWCTR2, (uint8_t[]){0x10}, 1));
    // VCOM control 1
    ESP_ERROR_CHECK(esp_lcd_panel_io_tx_param(*io_handle, ILI9341_VMCTR1, (uint8_t[]){0x3E, 0x28}, 2));
    // VCOM control 2
    ESP_ERROR_CHECK(esp_lcd_panel_io_tx_param(*io_handle, ILI9341_VMCTR2, (uint8_t[]){0x86}, 1));
    // Memory access control
    ESP_ERROR_CHECK(esp_lcd_panel_io_tx_param(*io_handle, ILI9341_MADCTL, (uint8_t[]){ILI9341_MADCTL_DEFAULT}, 1));
    // Pixel format: 16-bit (RGB565)
    ESP_ERROR_CHECK(esp_lcd_panel_io_tx_param(*io_handle, ILI9341_COLMOD, (uint8_t[]){0x55}, 1));

    // Exit sleep
    ESP_ERROR_CHECK(esp_lcd_panel_io_tx_param(*io_handle, ILI9341_SLPOUT, NULL, 0));
    vTaskDelay(pdMS_TO_TICKS(120));

    // Display on
    ESP_ERROR_CHECK(esp_lcd_panel_io_tx_param(*io_handle, ILI9341_DISPON, NULL, 0));

    ESP_ERROR_CHECK(esp_lcd_panel_io_tx_param(*io_handle, ILI9341_INVOFF, NULL, 0));

    // Store handles globally
    s_panel = *panel_handle;
    s_io = *io_handle;

    // Show boot screen
    const esp_partition_t* running = esp_ota_get_running_partition();
    show_boot_screen(*panel_handle, running ? running->label : "unknown");

    ESP_LOGI(TAG, "Display initialized");
    return ESP_OK;
}

void display_deinit(esp_lcd_panel_handle_t panel_handle, esp_lcd_panel_io_handle_t io_handle) {
    // Turn off display
    if (io_handle) {
        esp_lcd_panel_io_tx_param(io_handle, ILI9341_DISPOFF, NULL, 0);
        esp_lcd_panel_io_tx_param(io_handle, ILI9341_SLPIN, NULL, 0);
        esp_lcd_panel_io_del(io_handle);
    }
    if (s_color_transfer_sem) {
        vSemaphoreDelete(s_color_transfer_sem);
        s_color_transfer_sem = NULL;
    }
    // panel_handle is the same as io_handle in our implementation
    s_panel = NULL;
    s_io = NULL;
}

void display_set_backlight(uint8_t brightness) {
    ESP_ERROR_CHECK(ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1, brightness));
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1));
}

// ============================================================================
// Biome Theme Access
// ============================================================================
const BiomeTheme* get_biome_theme(const char* biome_name) {
    if (!biome_name) return &s_biome_themes[0];
    for (int i = 0; i < s_biome_count; i++) {
        if (strcmp(s_biome_themes[i].name, biome_name) == 0) {
            return &s_biome_themes[i];
        }
    }
    return &s_biome_themes[0]; // Default to grassland
}

uint16_t get_biome_rgb565(const char* biome_name) {
    const BiomeTheme* theme = get_biome_theme(biome_name);
    return rgb888_to_rgb565(theme->rgb565_r, theme->rgb565_g, theme->rgb565_b);
}

// ============================================================================
// Sprite Rendering
// ============================================================================
void draw_pet_sprite(esp_lcd_panel_handle_t panel, int16_t x, int16_t y,
                     PetAnimState state, const BiomeTheme* theme, bool is_partner) {
    if (!theme) theme = &s_biome_themes[0];

    const uint16_t* mask = get_pet_mask(state);
    uint16_t base_color = theme->pet_base_color;
    uint16_t accent_color = theme->pet_accent_color;

    // For partner, slightly desaturate
    if (is_partner) {
        base_color = (base_color & 0xF7DE) >> 1;  // Reduce intensity
        accent_color = (accent_color & 0xF7DE) >> 1;
    }

    // Create scaled sprite buffer (32x32 for 2x scale)
    const int scale = 2;
    const int src_w = 16, src_h = 16;
    const int dst_w = src_w * scale, dst_h = src_h * scale;

    uint16_t* buffer = (uint16_t*)malloc(dst_w * dst_h * 2);
    if (!buffer) return;

    for (int sy = 0; sy < src_h; sy++) {
        uint16_t row_mask = mask[sy];
        for (int sx = 0; sx < src_w; sx++) {
            if (row_mask & (0x8000 >> sx)) {
                // Determine color based on position (simple shading)
                uint16_t pixel_color = base_color;
                if (sy < src_h / 3) pixel_color = accent_color; // Head highlight
                if (sx < 3 || sx > 12) pixel_color = accent_color; // Side highlights

                // Draw 2x2 block
                for (int dy = 0; dy < scale; dy++) {
                    for (int dx = 0; dx < scale; dx++) {
                        buffer[(sy * scale + dy) * dst_w + (sx * scale + dx)] = pixel_color;
                    }
                }
            } else {
                // Transparent - fill with 0 (will be skipped)
                for (int dy = 0; dy < scale; dy++) {
                    for (int dx = 0; dx < scale; dx++) {
                        buffer[(sy * scale + dy) * dst_w + (sx * scale + dx)] = 0xFFFF; // placeholder
                    }
                }
            }
        }
    }

    // Draw with transparency (skip 0xFFFF)
    // For simplicity, draw full rect then overlay - in production use proper transparency
    lcd_draw_bitmap(x, y, dst_w, dst_h, buffer);
    free(buffer);
}

void draw_pet_sprite_scaled(esp_lcd_panel_handle_t panel, int16_t x, int16_t y,
                            PetAnimState state, const BiomeTheme* theme,
                            bool is_partner, uint8_t scale) {
    if (scale <= 2) {
        draw_pet_sprite(panel, x, y, state, theme, is_partner);
        return;
    }
    // Custom scale implementation would go here
    draw_pet_sprite(panel, x, y, state, theme, is_partner);
}

// ============================================================================
// UI Drawing Functions
// ============================================================================
void draw_top_bar(esp_lcd_panel_handle_t panel, int level, int jobs, int magic,
                  int cook, bool online, bool partner_online) {
    const int bar_h = TOP_BAR_HEIGHT;
    const BiomeTheme* theme = &s_biome_themes[0];

    // Background
    fill_rect(panel, 0, 0, TFT_WIDTH, bar_h, theme->ui_panel_color);
    draw_fast_hline(panel, 0, bar_h - 1, TFT_WIDTH, theme->ui_border_color);

    // Level
    char buf[32];
    snprintf(buf, sizeof(buf), "Lv.%d", level);
    draw_text(panel, 4, 4, buf, theme->text_color, theme->ui_panel_color, 1);

    // Icons (Jobs, Magic, Cook)
    int icon_x = 50;
    snprintf(buf, sizeof(buf), "J:%d", jobs);
    draw_text(panel, icon_x, 4, buf, theme->text_color, theme->ui_panel_color, 1);
    icon_x += 40;
    snprintf(buf, sizeof(buf), "M:%d", magic);
    draw_text(panel, icon_x, 4, buf, theme->text_color, theme->ui_panel_color, 1);
    icon_x += 40;
    snprintf(buf, sizeof(buf), "C:%d", cook);
    draw_text(panel, icon_x, 4, buf, theme->text_color, theme->ui_panel_color, 1);

    // Online status
    int status_x = TFT_WIDTH - 80;
    draw_text(panel, status_x, 4, online ? "ON" : "OFF",
              online ? TFT_GREEN : TFT_RED, theme->ui_panel_color, 1);
    status_x += 30;
    draw_text(panel, status_x, 4, partner_online ? "P:ON" : "P:OFF",
              partner_online ? TFT_GREEN : TFT_RED, theme->ui_panel_color, 1);
}

void draw_left_actions(esp_lcd_panel_handle_t panel) {
    const BiomeTheme* theme = &s_biome_themes[0];
    const char* labels[5] = {"FEED", "PLAY", "BATH", "RPG", "SAVE"};
    const char* labels_de[5] = {"FUETTR", "SPIELEN", "BADEN", "RPG", "SPEICH"};

    for (int i = 0; i < 5; i++) {
        int16_t x = LEFT_BTN_X;
        int16_t y = LEFT_BTN_Y + i * (LEFT_BTN_H + LEFT_BTN_GAP);

        UIButton btn = {
            .x = x, .y = y, .w = LEFT_BTN_W, .h = LEFT_BTN_H,
            .label = labels[i], .label_de = labels_de[i],
            .bg_color = theme->ui_panel_color,
            .text_color = theme->text_color,
            .border_color = theme->ui_border_color,
            .pressed = false, .enabled = true
        };
        draw_button(panel, &btn);
    }
}

void draw_right_actions(esp_lcd_panel_handle_t panel) {
    const BiomeTheme* theme = &s_biome_themes[0];
    const char* labels[5] = {"TREAT", "CHAT", "MAGIC+", "QUEST", "TRADE"};
    const char* labels_de[5] = {"LECKER", "CHAT", "MAGIE+", "QUEST", "HANDEL"};

    for (int i = 0; i < 5; i++) {
        int16_t x = RIGHT_BTN_X;
        int16_t y = RIGHT_BTN_Y + i * (RIGHT_BTN_H + RIGHT_BTN_GAP);

        UIButton btn = {
            .x = x, .y = y, .w = RIGHT_BTN_W, .h = RIGHT_BTN_H,
            .label = labels[i], .label_de = labels_de[i],
            .bg_color = theme->ui_panel_color,
            .text_color = theme->text_color,
            .border_color = theme->ui_border_color,
            .pressed = false, .enabled = true
        };
        draw_button(panel, &btn);
    }
}

void draw_bottom_hud(esp_lcd_panel_handle_t panel, uint32_t coins, uint8_t hp_pct,
                     uint8_t energy_pct, uint32_t partner_coins,
                     uint8_t partner_hp_pct, uint8_t partner_energy_pct,
                     const char* season) {
    const BiomeTheme* theme = &s_biome_themes[0];
    int16_t y = HUD_Y;

    // Background
    fill_rect(panel, 0, y, TFT_WIDTH, HUD_HEIGHT, theme->ui_panel_color);
    draw_fast_hline(panel, 0, y, TFT_WIDTH, theme->ui_border_color);

    // Left: Your stats
    char buf[64];
    snprintf(buf, sizeof(buf), "$:%" PRIu32 " HP:%d%% EN:%d%%", coins, hp_pct, energy_pct);
    draw_text(panel, 4, y + 4, buf, theme->text_color, theme->ui_panel_color, 1);

    // Right: Partner stats
    snprintf(buf, sizeof(buf), "P:$:%" PRIu32 " HP:%d%% EN:%d%%", partner_coins, partner_hp_pct, partner_energy_pct);
    draw_text(panel, TFT_WIDTH - 120, y + 4, buf, theme->text_color, theme->ui_panel_color, 1);

    // Center: Season
    if (season) {
        draw_centered_text(panel, y + 4, season, theme->accent_color, theme->ui_panel_color, 1);
    }
}

void draw_sync_indicator(esp_lcd_panel_handle_t panel, bool synced) {
    uint16_t color = synced ? TFT_GREEN : TFT_RED;
    // Small circle at top center
    for (int dy = -3; dy <= 3; dy++) {
        for (int dx = -3; dx <= 3; dx++) {
            if (dx*dx + dy*dy <= 9) {
                draw_pixel(panel, SYNC_INDICATOR_X + dx, SYNC_INDICATOR_Y + dy, color);
            }
        }
    }
}

void draw_button(esp_lcd_panel_handle_t panel, const UIButton* btn) {
    if (!btn->enabled) {
        fill_rect(panel, btn->x, btn->y, btn->w, btn->h, TFT_DARKGREY);
        draw_rect(panel, btn->x, btn->y, btn->w, btn->h, TFT_DARKGREY);
        draw_text(panel, btn->x + 2, btn->y + 2,
                  LANG_DE ? btn->label_de : btn->label,
                  TFT_LIGHTGREY, TFT_DARKGREY, 1);
        return;
    }

    uint16_t bg = btn->pressed ? btn->border_color : btn->bg_color;
    uint16_t text = btn->pressed ? btn->bg_color : btn->text_color;

    fill_rect(panel, btn->x, btn->y, btn->w, btn->h, bg);
    draw_rect(panel, btn->x, btn->y, btn->w, btn->h, btn->border_color);

    // Center text
    const char* label = LANG_DE ? btn->label_de : btn->label;
    int text_w = strlen(label) * 6; // Approximate
    int text_x = btn->x + (btn->w - text_w) / 2;
    int text_y = btn->y + (btn->h - 8) / 2;
    draw_text(panel, text_x, text_y, label, text, bg, 1);
}

void draw_text(esp_lcd_panel_handle_t panel, int16_t x, int16_t y,
               const char* text, uint16_t color, uint16_t bg_color, uint8_t size) {
    if (!text || size == 0) return;
    if (size > 3) {
        ESP_LOGE(TAG, "Unsupported 5x7 font scale: %u", size);
        return;
    }

    int glyph_w = 6 * size;
    int glyph_h = 7 * size;
    for (size_t i = 0; text[i] != '\0'; i++) {
        unsigned char c = (unsigned char)text[i];
        if (c < 0x20 || c > 0x7E) c = '?';
        const uint8_t* glyph = CYD_FONT5X7[c - 0x20];
        for (int row = 0; row < 7; row++) {
            for (int sy = 0; sy < size; sy++) {
                for (int col = 0; col < 6; col++) {
                    uint16_t pixel = bg_color;
                    if (col < 5 && (glyph[col] & (1U << row))) pixel = color;
                    for (int sx = 0; sx < size; sx++) {
                        size_t index = (size_t)(row * size + sy) * glyph_w + col * size + sx;
                        s_text_glyph_buf[index] = pixel;
                    }
                }
            }
        }
        lcd_draw_bitmap(x + i * glyph_w, y, glyph_w, glyph_h, s_text_glyph_buf);
    }
}

void draw_centered_text(esp_lcd_panel_handle_t panel, int16_t y,
                        const char* text, uint16_t color, uint16_t bg_color, uint8_t size) {
    int len = strlen(text);
    int16_t x = (TFT_WIDTH - len * 6 * size) / 2;
    draw_text(panel, x, y, text, color, bg_color, size);
}

// ============================================================================
// Utility Functions
// ============================================================================
uint16_t rgb888_to_rgb565(uint8_t r, uint8_t g, uint8_t b) {
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}

void fill_rect(esp_lcd_panel_handle_t panel, int16_t x, int16_t y,
               int16_t w, int16_t h, uint16_t color) {
    if (w <= 0 || h <= 0) return;
    lcd_fill_rect(x, y, w, h, color);
}

void draw_rect(esp_lcd_panel_handle_t panel, int16_t x, int16_t y,
               int16_t w, int16_t h, uint16_t color) {
    draw_fast_hline(panel, x, y, w, color);
    draw_fast_hline(panel, x, y + h - 1, w, color);
    draw_fast_vline(panel, x, y, h, color);
    draw_fast_vline(panel, x + w - 1, y, h, color);
}

void draw_fast_hline(esp_lcd_panel_handle_t panel, int16_t x, int16_t y,
                     int16_t w, uint16_t color) {
    if (w <= 0) return;
    lcd_fill_rect(x, y, w, 1, color);
}

void draw_fast_vline(esp_lcd_panel_handle_t panel, int16_t x, int16_t y,
                     int16_t h, uint16_t color) {
    if (h <= 0) return;
    lcd_fill_rect(x, y, 1, h, color);
}

void draw_pixel(esp_lcd_panel_handle_t panel, int16_t x, int16_t y, uint16_t color) {
    lcd_fill_rect(x, y, 1, 1, color);
}

// ============================================================================
// Screen Helpers
// ============================================================================
void clear_screen(esp_lcd_panel_handle_t panel, uint16_t color) {
    fill_rect(panel, 0, 0, TFT_WIDTH, TFT_HEIGHT, color);
}

// Touch calibration screen
void draw_calibration_screen(esp_lcd_panel_handle_t panel) {
    clear_screen(panel, TFT_BLACK);
    draw_centered_text(panel, 50, "Touch Calibration", TFT_YELLOW, TFT_BLACK, 2);
    draw_centered_text(panel, 90, "Touch the crosshairs", TFT_WHITE, TFT_BLACK, 1);
    draw_centered_text(panel, 110, "at each corner", TFT_WHITE, TFT_BLACK, 1);
    draw_centered_text(panel, 130, "and center", TFT_WHITE, TFT_BLACK, 1);
    draw_centered_text(panel, 200, "Timeout: 10s per point", TFT_LIGHTGREY, TFT_BLACK, 1);
}

void show_boot_screen(esp_lcd_panel_handle_t panel, const char* partition_label) {
    clear_screen(panel, TFT_BLACK);

    const char* title = "PetDuet";
    draw_centered_text(panel, 50, title, TFT_YELLOW, TFT_BLACK, 3);

    char buf[64];
    snprintf(buf, sizeof(buf), "Version %s", FIRMWARE_VERSION);
    draw_centered_text(panel, 90, buf, TFT_WHITE, TFT_BLACK, 1);

    // Partition info
    char part_slot = '?';
    if (strcmp(partition_label, "ota_0") == 0) part_slot = 'A';
    else if (strcmp(partition_label, "ota_1") == 0) part_slot = 'B';

    snprintf(buf, sizeof(buf), "Boot Slot: %c (%s)", part_slot, partition_label);
    draw_centered_text(panel, 120, buf, TFT_CYAN, TFT_BLACK, 1);

    draw_centered_text(panel, 160, "Initializing...", TFT_GREEN, TFT_BLACK, 1);
}

void show_wifi_connecting(esp_lcd_panel_handle_t panel, const char* ssid) {
    clear_screen(panel, TFT_NAVY);
    draw_centered_text(panel, 100, "Connecting to WiFi...", TFT_WHITE, TFT_NAVY, 1);
    char buf[64];
    snprintf(buf, sizeof(buf), "SSID: %s", ssid);
    draw_centered_text(panel, 120, buf, TFT_CYAN, TFT_NAVY, 1);
}

void show_error_screen(esp_lcd_panel_handle_t panel, const char* error) {
    clear_screen(panel, TFT_MAROON);
    draw_centered_text(panel, 80, "ERROR", TFT_YELLOW, TFT_MAROON, 2);
    draw_centered_text(panel, 120, error, TFT_WHITE, TFT_MAROON, 1);
    draw_centered_text(panel, 160, "Restart required", TFT_LIGHTGREY, TFT_MAROON, 1);
}