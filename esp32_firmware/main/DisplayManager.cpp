/**
 * ============================================================================
 * PetDuet ESP32 CYD - Display Manager Implementation
 * ============================================================================
 * High-level display management using PetGraphics
 * ============================================================================
 */
#include <string.h>
#include <stdlib.h>
#include "esp_log.h"
#include "esp_timer.h"
#include "DisplayManager.h"
#include "PetGraphics.h"
#include "TamagotchiCoreEngine.h"

static const char* TAG = "DisplayMgr";

// Forward declarations for static functions used in display_mgr_handle_touch
static void handle_left_action(DisplayManager* dm, int btn);
static void handle_right_action(DisplayManager* dm, int btn);

esp_err_t display_mgr_init(DisplayManager* dm, PetState* pet, PetState* partner_pet,
                           const RulesData* rules) {
    if (!dm) return ESP_ERR_INVALID_ARG;

    memset(dm, 0, sizeof(DisplayManager));
    dm->pet = pet;
    dm->partner_pet = partner_pet;
    dm->rules = rules;
    dm->current_screen = SCREEN_BOOT;
    dm->needs_redraw = true;
    dm->backlight_level = 200;
    dm->last_activity_time = esp_timer_get_time() / 1000;

    // Get biome themes
    if (pet && pet->biome_id[0]) {
        dm->biome_theme = get_biome_theme(pet->biome_id);
    } else {
        dm->biome_theme = get_biome_theme(NULL); // Returns default grassland
    }
    dm->partner_biome_theme = dm->biome_theme;

    ESP_LOGI(TAG, "Display manager initialized");
    return ESP_OK;
}

void display_mgr_deinit(DisplayManager* dm) {
    if (!dm) return;
    // Display deinit handled by PetGraphics
}

void display_mgr_update(DisplayManager* dm, uint32_t delta_ms) {
    if (!dm) return;

    dm->last_frame_time += delta_ms;
    dm->animation_frame = (dm->animation_frame + 1) % 60;

    // Auto-dim after 30 seconds inactivity
    uint32_t now = esp_timer_get_time() / 1000;
    if (now - dm->last_activity_time > 30000 && !dm->dimmed) {
        display_mgr_dim(dm, true);
    }
}

void display_mgr_render(DisplayManager* dm) {
    if (!dm || !dm->needs_redraw) return;

    switch (dm->current_screen) {
        case SCREEN_BOOT:
            render_boot_screen(dm);
            break;
        case SCREEN_WIFI_CONNECTING:
            render_main_screen(dm); // Will show connecting overlay
            break;
        case SCREEN_MAIN:
            render_main_screen(dm);
            break;
        case SCREEN_RPG_MENU:
            render_rpg_menu(dm);
            break;
        case SCREEN_INVENTORY:
            render_inventory(dm);
            break;
        case SCREEN_SHOP:
            render_shop(dm);
            break;
        case SCREEN_SPELLS:
            render_spells(dm);
            break;
        case SCREEN_MISSIONS:
            render_missions(dm);
            break;
        case SCREEN_TRADE:
            render_trade(dm);
            break;
        case SCREEN_SETTINGS:
            render_settings(dm);
            break;
        case SCREEN_OTA_UPDATE:
            render_ota_screen(dm, dm->error_message, 0);
            break;
        default:
            render_main_screen(dm);
            break;
    }

    dm->needs_redraw = false;
}

void display_mgr_set_screen(DisplayManager* dm, ScreenState screen) {
    if (!dm) return;
    dm->previous_screen = dm->current_screen;
    dm->current_screen = screen;
    dm->needs_redraw = true;
    dm->last_activity_time = esp_timer_get_time() / 1000;
}

ScreenState display_mgr_get_screen(DisplayManager* dm) {
    return dm ? dm->current_screen : SCREEN_BOOT;
}

void display_mgr_handle_touch(DisplayManager* dm, int16_t x, int16_t y, bool pressed) {
    if (!dm) return;

    dm->last_touch_x = x;
    dm->last_touch_y = y;
    dm->touch_pressed = pressed;
    dm->last_activity_time = esp_timer_get_time() / 1000;

    if (dm->dimmed) {
        display_mgr_dim(dm, false);
        return;
    }

    // Handle button presses based on current screen
    if (pressed && dm->current_screen == SCREEN_MAIN) {
        // Left buttons
        for (int i = 0; i < 5; i++) {
            int16_t bx = LEFT_BTN_X;
            int16_t by = LEFT_BTN_Y + i * (LEFT_BTN_H + LEFT_BTN_GAP);
            if (x >= bx && x < bx + LEFT_BTN_W && y >= by && y < by + LEFT_BTN_H) {
                dm->selected_left_btn = i;
                // Handle action
                handle_left_action(dm, i);
                dm->needs_redraw = true;
                return;
            }
        }

        // Right buttons
        for (int i = 0; i < 5; i++) {
            int16_t bx = RIGHT_BTN_X;
            int16_t by = RIGHT_BTN_Y + i * (RIGHT_BTN_H + RIGHT_BTN_GAP);
            if (x >= bx && x < bx + RIGHT_BTN_W && y >= by && y < by + RIGHT_BTN_H) {
                dm->selected_right_btn = i;
                handle_right_action(dm, i);
                dm->needs_redraw = true;
                return;
            }
        }
    }
}

void display_mgr_set_backlight(DisplayManager* dm, uint8_t level) {
    if (!dm) return;
    dm->backlight_level = level;
    display_set_backlight(level);
}

void display_mgr_dim(DisplayManager* dm, bool dim) {
    if (!dm) return;
    dm->dimmed = dim;
    display_set_backlight(dim ? 30 : dm->backlight_level);
}

void display_mgr_show_notification(DisplayManager* dm, const char* message, uint32_t duration_ms) {
    if (!dm || !message) return;
    strncpy(dm->error_message, message, sizeof(dm->error_message)-1);
    dm->needs_redraw = true;
}

void display_mgr_show_error(DisplayManager* dm, const char* error) {
    display_mgr_show_notification(dm, error, 5000);
}

// ============================================================================
// Screen Renderers
// ============================================================================

static void handle_left_action(DisplayManager* dm, int btn) {
    if (!dm || !dm->pet) return;

    switch (btn) {
        case 0: // FEED
            display_mgr_set_screen(dm, SCREEN_INVENTORY); // Show food items
            break;
        case 1: // PLAY
            display_mgr_set_screen(dm, SCREEN_INVENTORY); // Show toys
            break;
        case 2: // BATH
            engine_bathe(dm->pet, dm->rules);
            break;
        case 3: // RPG
            display_mgr_set_screen(dm, SCREEN_RPG_MENU);
            break;
        case 4: // SAVE
            // Trigger save
            break;
    }
}

static void handle_right_action(DisplayManager* dm, int btn) {
    if (!dm || !dm->pet) return;

    switch (btn) {
        case 0: // TREAT
            display_mgr_set_screen(dm, SCREEN_TRADE);
            break;
        case 1: // CHAT
            // Send chat emote
            break;
        case 2: // MAGIC+
            display_mgr_set_screen(dm, SCREEN_SPELLS);
            break;
        case 3: // QUEST
            display_mgr_set_screen(dm, SCREEN_MISSIONS);
            break;
        case 4: // TRADE
            display_mgr_set_screen(dm, SCREEN_TRADE);
            break;
    }
}

void render_boot_screen(DisplayManager* dm) {
    if (!dm) return;
    show_boot_screen(dm->panel, "ota_0"); // Will be updated with actual partition
}

void render_main_screen(DisplayManager* dm) {
    if (!dm || !dm->pet || !dm->panel) return;

    const BiomeTheme* theme = dm->biome_theme;

    // Clear with biome background
    clear_screen(dm->panel, theme->bg_color);

    // Top bar
    draw_top_bar(dm->panel, dm->pet->level, 0, 0, 0, true, dm->pet->partner_online);

    // Your pet area
    draw_pet_sprite(dm->panel, YOUR_PET_X, YOUR_PET_Y, PET_ANIM_IDLE, theme, false);

    // Pet stats overlay
    char buf[32];
    snprintf(buf, sizeof(buf), "HP:%d%%", (int)(dm->pet->stats[STAT_HEALTH] / dm->pet->stat_max[STAT_HEALTH] * 100));
    draw_text(dm->panel, YOUR_PET_X, YOUR_PET_Y + PET_AREA_H + 4, buf, theme->text_color, theme->bg_color, 1);

    // Partner pet area
    if (dm->partner_pet && dm->partner_pet->initialized) {
        draw_pet_sprite(dm->panel, PARTNER_PET_X, PARTNER_PET_Y, PET_ANIM_IDLE, dm->partner_biome_theme, true);
        snprintf(buf, sizeof(buf), "HP:%d%%", (int)(dm->partner_pet->stats[STAT_HEALTH] / dm->partner_pet->stat_max[STAT_HEALTH] * 100));
        draw_text(dm->panel, PARTNER_PET_X, PARTNER_PET_Y + PET_AREA_H + 4, buf, theme->text_color, theme->bg_color, 1);
    }

    // Sync indicator
    draw_sync_indicator(dm->panel, dm->pet->partner_online);

    // Left action buttons
    draw_left_actions(dm->panel);

    // Right action buttons
    draw_right_actions(dm->panel);

    // Bottom HUD
    draw_bottom_hud(dm->panel, dm->pet->coins,
                    (int)(dm->pet->stats[STAT_HEALTH] / dm->pet->stat_max[STAT_HEALTH] * 100),
                    (int)(dm->pet->stats[STAT_ENERGY] / dm->pet->stat_max[STAT_ENERGY] * 100),
                    dm->partner_pet ? dm->partner_pet->coins : 0,
                    dm->partner_pet ? (int)(dm->partner_pet->stats[STAT_HEALTH] / dm->partner_pet->stat_max[STAT_HEALTH] * 100) : 0,
                    dm->partner_pet ? (int)(dm->partner_pet->stats[STAT_ENERGY] / dm->partner_pet->stat_max[STAT_ENERGY] * 100) : 0,
                    "Spring");
}

void render_rpg_menu(DisplayManager* dm) {
    if (!dm || !dm->panel) return;
    clear_screen(dm->panel, TFT_NAVY);
    draw_centered_text(dm->panel, 50, "RPG MENU", TFT_YELLOW, TFT_NAVY, 2);
    draw_centered_text(dm->panel, 100, "Jobs, Dungeons, Crafting", TFT_WHITE, TFT_NAVY, 1);
    draw_centered_text(dm->panel, 150, "Touch to go back", TFT_CYAN, TFT_NAVY, 1);
}

void render_inventory(DisplayManager* dm) {
    if (!dm || !dm->panel || !dm->pet) return;
    clear_screen(dm->panel, TFT_DARKGREY);
    draw_centered_text(dm->panel, 20, "INVENTORY", TFT_YELLOW, TFT_DARKGREY, 2);

    int y = 50;
    for (int i = 0; i < MAX_INVENTORY_SLOTS && y < TFT_HEIGHT - 30; i++) {
        if (dm->pet->inventory[i].valid) {
            char buf[64];
            snprintf(buf, sizeof(buf), "%s x%d", dm->pet->inventory[i].item_id, dm->pet->inventory[i].count);
            draw_text(dm->panel, 20, y, buf, TFT_WHITE, TFT_DARKGREY, 1);
            y += 20;
        }
    }
    draw_centered_text(dm->panel, TFT_HEIGHT - 20, "Touch to go back", TFT_CYAN, TFT_DARKGREY, 1);
}

void render_shop(DisplayManager* dm) {
    if (!dm || !dm->panel) return;
    clear_screen(dm->panel, TFT_MAROON);
    draw_centered_text(dm->panel, 50, "SHOP", TFT_YELLOW, TFT_MAROON, 2);
    draw_centered_text(dm->panel, 100, "Items, Recipes, Spells", TFT_WHITE, TFT_MAROON, 1);
    draw_centered_text(dm->panel, 150, "Touch to go back", TFT_CYAN, TFT_MAROON, 1);
}

void render_spells(DisplayManager* dm) {
    if (!dm || !dm->panel) return;
    clear_screen(dm->panel, TFT_PURPLE);
    draw_centered_text(dm->panel, 50, "SPELLS", TFT_YELLOW, TFT_PURPLE, 2);
    draw_centered_text(dm->panel, 100, "Magic abilities", TFT_WHITE, TFT_PURPLE, 1);
    draw_centered_text(dm->panel, 150, "Touch to go back", TFT_CYAN, TFT_PURPLE, 1);
}

void render_missions(DisplayManager* dm) {
    if (!dm || !dm->panel || !dm->pet) return;
    clear_screen(dm->panel, TFT_OLIVE);
    draw_centered_text(dm->panel, 20, "DAILY MISSIONS", TFT_YELLOW, TFT_OLIVE, 2);

    int y = 50;
    for (int i = 0; i < dm->pet->mission_count && y < TFT_HEIGHT - 30; i++) {
        ActiveMission* m = &dm->pet->active_missions[i];
        char buf[64];
        snprintf(buf, sizeof(buf), "%s: %d/%d %s", m->def.name, m->current_progress, m->def.target_count,
                 m->completed ? "[DONE]" : "");
        uint16_t color = m->completed ? TFT_GREEN : TFT_WHITE;
        draw_text(dm->panel, 20, y, buf, color, TFT_OLIVE, 1);
        y += 20;
    }
    draw_centered_text(dm->panel, TFT_HEIGHT - 20, "Touch to go back", TFT_CYAN, TFT_OLIVE, 1);
}

void render_trade(DisplayManager* dm) {
    if (!dm || !dm->panel) return;
    clear_screen(dm->panel, TFT_TEAL);
    draw_centered_text(dm->panel, 50, "TRADE", TFT_YELLOW, TFT_TEAL, 2);
    draw_centered_text(dm->panel, 100, "Propose/accept trades", TFT_WHITE, TFT_TEAL, 1);
    draw_centered_text(dm->panel, 150, "Touch to go back", TFT_CYAN, TFT_TEAL, 1);
}

void render_settings(DisplayManager* dm) {
    if (!dm || !dm->panel) return;
    clear_screen(dm->panel, TFT_DARKCYAN);
    draw_centered_text(dm->panel, 50, "SETTINGS", TFT_YELLOW, TFT_DARKCYAN, 2);
    draw_centered_text(dm->panel, 100, "WiFi, Display, Language", TFT_WHITE, TFT_DARKCYAN, 1);
    draw_centered_text(dm->panel, 150, "Touch to go back", TFT_CYAN, TFT_DARKCYAN, 1);
}

void render_ota_screen(DisplayManager* dm, const char* status, uint8_t progress) {
    if (!dm || !dm->panel) return;
    clear_screen(dm->panel, TFT_NAVY);
    draw_centered_text(dm->panel, 50, "OTA UPDATE", TFT_YELLOW, TFT_NAVY, 2);
    if (status) draw_centered_text(dm->panel, 100, status, TFT_WHITE, TFT_NAVY, 1);

    // Progress bar
    int bar_w = 200;
    int bar_x = (TFT_WIDTH - bar_w) / 2;
    int bar_y = 130;
    draw_rect(dm->panel, bar_x, bar_y, bar_w, 20, TFT_WHITE);
    fill_rect(dm->panel, bar_x + 2, bar_y + 2, (bar_w - 4) * progress / 100, 16, TFT_GREEN);

    char buf[32];
    snprintf(buf, sizeof(buf), "%d%%", progress);
    draw_centered_text(dm->panel, 160, buf, TFT_CYAN, TFT_NAVY, 1);
}

// ============================================================================
// Helper
// ============================================================================

const char* get_localized_string(const char* en, const char* de) {
    return LANG_DE ? de : en;
}