/**
 * ============================================================================
 * PetDuet ESP32 CYD - Main Entry Point (ESP-IDF)
 * ============================================================================
 * app_main() replaces Arduino setup()/loop()
 * ============================================================================
 */
#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_timer.h"
#include "esp_sleep.h"
#include "esp_ota_ops.h"
#include "esp_partition.h"
#include "nvs_flash.h"
#include "esp_spiffs.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "driver/ledc.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"
#include "cJSON.h"

#include "config.h"
#include "CYD_Pins.h"
#include "PetGraphics.h"
#include "TamagotchiCoreEngine.h"
#include "NetworkManager.h"
#include "DisplayManager.h"
#include "TouchManager.h"
#include "RulesManager.h"
#include "OTAManager.h"
#include "PetStateManager.h"

static const char* TAG = "PetDuet";

// ============================================================================
// Global Instances
// ============================================================================
static RulesManager g_rules_mgr;
static NetworkManager g_net_mgr;
static DisplayManager g_disp_mgr;
static TouchManager g_touch_mgr;
static OTAManager g_ota_mgr;
static PetStateManager g_pet_mgr;

// ============================================================================
// Task Handles
// ============================================================================
static TaskHandle_t s_game_task_handle = NULL;
static TaskHandle_t s_network_task_handle = NULL;
static TaskHandle_t s_display_task_handle = NULL;
static TaskHandle_t s_ota_task_handle = NULL;

// ============================================================================
// Event Groups
// ============================================================================
static EventGroupHandle_t s_sync_events;
#define EVT_WIFI_CONNECTED      BIT0
#define EVT_WS_CONNECTED        BIT1
#define EVT_RULES_LOADED        BIT2
#define EVT_PET_LOADED          BIT3
#define EVT_SYNC_COMPLETE       BIT4
#define EVT_OTA_COMPLETE        BIT5
#define EVT_SHUTDOWN            BIT6

// ============================================================================
// Forward Declarations
// ============================================================================
static void game_task(void* arg);
static void network_task(void* arg);
static void display_task(void* arg);
static void ota_task(void* arg);
static esp_err_t initialize_hardware(void);
static esp_err_t initialize_storage(void);
static void print_boot_info(void);
static void on_rules_changed(const RulesData* rules, void* user_data);
static void on_pet_changed(const PetState* pet, void* user_data);
static void on_partner_updated(const PetState* partner, void* user_data);
static void on_sync_complete(bool success, void* user_data);
static void on_ota_progress(uint8_t percent, const char* msg, void* user_data);
static void on_ota_complete(bool success, const char* version, void* user_data);
static void on_touch_event(const TouchPoint* point, void* user_data);
static void on_ws_message(const WSMessage* msg, void* user_data);

// ============================================================================
// app_main - Entry Point
// ============================================================================
extern "C" void app_main(void)
{
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "PetDuet ESP32 CYD Starting...");
    ESP_LOGI(TAG, "Firmware Version: %s", FIRMWARE_VERSION);
    ESP_LOGI(TAG, "Rules Version: %s", RULES_VERSION);
    ESP_LOGI(TAG, "========================================");

    // Print boot partition info (for A/B OTA debugging)
    print_boot_info();

    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Initialize storage (SPIFFS for rules.json)
    ESP_ERROR_CHECK(initialize_storage());

    // Initialize hardware (display, touch, SPI)
    ESP_ERROR_CHECK(initialize_hardware());

    // Create event group
    s_sync_events = xEventGroupCreate();

    // Initialize Rules Manager
    ESP_ERROR_CHECK(rules_init(&g_rules_mgr, "/spiffs/rules.json"));
    rules_set_callback(&g_rules_mgr, on_rules_changed, NULL);
    ESP_ERROR_CHECK(rules_load(&g_rules_mgr));

    // Wait for rules to load
    xEventGroupWaitBits(s_sync_events, EVT_RULES_LOADED, pdTRUE, pdFALSE, pdMS_TO_TICKS(5000));

    // Initialize Network Manager
    ESP_ERROR_CHECK(net_init(&g_net_mgr, WIFI_SSID, WIFI_PASSWORD,
                             PHP_SERVER_BASE_URL, DEVICE_ID));
    net_ws_set_callback(&g_net_mgr, on_ws_message, NULL);

    // Initialize Pet State Manager
    ESP_ERROR_CHECK(pet_mgr_init(&g_pet_mgr, &g_net_mgr,
                                 rules_get_data(&g_rules_mgr), DEVICE_ID,
                                 "petduet"));
    pet_mgr_set_callbacks(&g_pet_mgr, on_pet_changed, on_partner_updated,
                          on_sync_complete, NULL);
    esp_err_t load_err = pet_mgr_load_local(&g_pet_mgr);
    if (load_err != ESP_OK && load_err != ESP_ERR_NOT_FOUND) {
        ESP_LOGE(TAG, "Failed to load local pet state: %s", esp_err_to_name(load_err));
        ESP_ERROR_CHECK(load_err);
    }
    xEventGroupSetBits(s_sync_events, EVT_PET_LOADED);

    // Initialize Display Manager
    ESP_ERROR_CHECK(display_mgr_init(&g_disp_mgr, pet_mgr_get_pet(&g_pet_mgr),
                                     pet_mgr_get_partner(&g_pet_mgr),
                                     rules_get_data(&g_rules_mgr)));

    // Initialize Touch Manager
    ESP_ERROR_CHECK(touch_init(&g_touch_mgr));
    touch_set_callback(&g_touch_mgr, on_touch_event, NULL);

    // Initialize OTA Manager
    ESP_ERROR_CHECK(ota_init(&g_ota_mgr, &g_net_mgr));
    ota_set_callbacks(&g_ota_mgr, on_ota_progress, on_ota_complete, NULL);
    ota_set_auto_check(&g_ota_mgr, true, OTA_CHECK_INTERVAL_MS);

    // Set partner info
    pet_mgr_set_partner_info(&g_pet_mgr, FRIEND_DEV_ID, TRAINER_NAME, DEFAULT_BIOME);

    // Create tasks
    xTaskCreatePinnedToCore(game_task, "game_task", 8192, NULL, 5, &s_game_task_handle, 1);
    xTaskCreatePinnedToCore(network_task, "network_task", 8192, NULL, 4, &s_network_task_handle, 0);
    xTaskCreatePinnedToCore(display_task, "display_task", 8192, NULL, 3, &s_display_task_handle, 1);
    xTaskCreatePinnedToCore(ota_task, "ota_task", 8192, NULL, 2, &s_ota_task_handle, 0);

    ESP_LOGI(TAG, "All tasks created. Entering main loop...");

    // Main loop - monitor tasks and handle shutdown
    while (1) {
        EventBits_t bits = xEventGroupWaitBits(s_sync_events, EVT_SHUTDOWN,
                                               pdTRUE, pdFALSE, pdMS_TO_TICKS(10000));

        if (bits & EVT_SHUTDOWN) {
            ESP_LOGI(TAG, "Shutdown requested");
            break;
        }

        // Periodic health check
        ESP_LOGD(TAG, "System running - Free heap: %" PRIu32 ", Min free: %" PRIu32,
                 esp_get_free_heap_size(), esp_get_minimum_free_heap_size());
    }

    // Cleanup
    vTaskDelete(s_game_task_handle);
    vTaskDelete(s_network_task_handle);
    vTaskDelete(s_display_task_handle);
    vTaskDelete(s_ota_task_handle);

    display_mgr_deinit(&g_disp_mgr);
    touch_deinit(&g_touch_mgr);
    ota_deinit(&g_ota_mgr);
    pet_mgr_deinit(&g_pet_mgr);
    net_deinit(&g_net_mgr);
    rules_deinit(&g_rules_mgr);

    vEventGroupDelete(s_sync_events);

    ESP_LOGI(TAG, "PetDuet shutdown complete");
}

// ============================================================================
// Hardware Initialization
// ============================================================================
static esp_err_t initialize_hardware(void)
{
    ESP_LOGI(TAG, "Initializing hardware...");

    // Initialize SPI bus for TFT (HSPI)
    spi_bus_config_t tft_bus_cfg;
    memset(&tft_bus_cfg, 0, sizeof(tft_bus_cfg));
    tft_bus_cfg.mosi_io_num = TFT_MOSI;
    tft_bus_cfg.miso_io_num = TFT_MISO;
    tft_bus_cfg.sclk_io_num = TFT_SCLK;
    tft_bus_cfg.quadwp_io_num = -1;
    tft_bus_cfg.quadhd_io_num = -1;
    tft_bus_cfg.data4_io_num = -1;
    tft_bus_cfg.data5_io_num = -1;
    tft_bus_cfg.data6_io_num = -1;
    tft_bus_cfg.data7_io_num = -1;
    tft_bus_cfg.max_transfer_sz = TFT_WIDTH * TFT_HEIGHT * 2 + 8;
    tft_bus_cfg.flags = SPICOMMON_BUSFLAG_MASTER | SPICOMMON_BUSFLAG_GPIO_PINS;
    tft_bus_cfg.isr_cpu_id = ESP_INTR_CPU_AFFINITY_AUTO;
    tft_bus_cfg.intr_flags = ESP_INTR_FLAG_IRAM;
    ESP_ERROR_CHECK(spi_bus_initialize(TFT_SPI_HOST, &tft_bus_cfg, SPI_DMA_CH_AUTO));

    // Initialize SPI bus for Touch (VSPI) - shared with SD if used
    spi_bus_config_t touch_bus_cfg;
    memset(&touch_bus_cfg, 0, sizeof(touch_bus_cfg));
    touch_bus_cfg.mosi_io_num = TOUCH_MOSI;
    touch_bus_cfg.miso_io_num = TOUCH_MISO;
    touch_bus_cfg.sclk_io_num = TOUCH_CLK;
    touch_bus_cfg.quadwp_io_num = -1;
    touch_bus_cfg.quadhd_io_num = -1;
    touch_bus_cfg.data4_io_num = -1;
    touch_bus_cfg.data5_io_num = -1;
    touch_bus_cfg.data6_io_num = -1;
    touch_bus_cfg.data7_io_num = -1;
    touch_bus_cfg.max_transfer_sz = 32;
    touch_bus_cfg.flags = SPICOMMON_BUSFLAG_MASTER | SPICOMMON_BUSFLAG_GPIO_PINS;
    touch_bus_cfg.isr_cpu_id = ESP_INTR_CPU_AFFINITY_AUTO;
    touch_bus_cfg.intr_flags = 0;
    ESP_ERROR_CHECK(spi_bus_initialize(TOUCH_SPI_HOST, &touch_bus_cfg, SPI_DMA_CH_AUTO));

    // Initialize RGB LED GPIOs
    gpio_config_t led_cfg = {
        .pin_bit_mask = (1ULL << LED_R) | (1ULL << LED_G) | (1ULL << LED_B),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&led_cfg));
    // Active low - turn off initially
    gpio_set_level((gpio_num_t)LED_R, 1);
    gpio_set_level((gpio_num_t)LED_G, 1);
    gpio_set_level((gpio_num_t)LED_B, 1);

    // Initialize Buzzer (LEDC)
    ledc_timer_config_t buzzer_timer;
    memset(&buzzer_timer, 0, sizeof(buzzer_timer));
    buzzer_timer.speed_mode = LEDC_LOW_SPEED_MODE;
    buzzer_timer.duty_resolution = (ledc_timer_bit_t)BUZZER_RESOLUTION;
    buzzer_timer.timer_num = (ledc_timer_t)BUZZER_LEDC_TIMER;
    buzzer_timer.freq_hz = BUZZER_FREQ_HZ;
    buzzer_timer.clk_cfg = LEDC_AUTO_CLK;
    buzzer_timer.deconfigure = false;
    ESP_ERROR_CHECK(ledc_timer_config(&buzzer_timer));

    ledc_channel_config_t buzzer_channel;
    memset(&buzzer_channel, 0, sizeof(buzzer_channel));
    buzzer_channel.gpio_num = BUZZER_GPIO;
    buzzer_channel.speed_mode = LEDC_LOW_SPEED_MODE;
    buzzer_channel.channel = (ledc_channel_t)BUZZER_LEDC_CHANNEL;
    buzzer_channel.timer_sel = (ledc_timer_t)BUZZER_LEDC_TIMER;
    buzzer_channel.duty = 0;
    buzzer_channel.hpoint = 0;
    // flags sub-struct defaults from memset
    ESP_ERROR_CHECK(ledc_channel_config(&buzzer_channel));

    // Initialize Backlight (LEDC)
    ledc_timer_config_t bl_timer;
    memset(&bl_timer, 0, sizeof(bl_timer));
    bl_timer.speed_mode = LEDC_LOW_SPEED_MODE;
    bl_timer.duty_resolution = LEDC_TIMER_8_BIT;
    bl_timer.timer_num = LEDC_TIMER_1;
    bl_timer.freq_hz = 5000;
    bl_timer.clk_cfg = LEDC_AUTO_CLK;
    bl_timer.deconfigure = false;
    ESP_ERROR_CHECK(ledc_timer_config(&bl_timer));

    ledc_channel_config_t bl_channel;
    memset(&bl_channel, 0, sizeof(bl_channel));
    bl_channel.gpio_num = TFT_BL;
    bl_channel.speed_mode = LEDC_LOW_SPEED_MODE;
    bl_channel.channel = LEDC_CHANNEL_1;
    bl_channel.timer_sel = LEDC_TIMER_1;
    bl_channel.duty = 200;  // ~80% brightness
    bl_channel.hpoint = 0;
    // flags sub-struct defaults from memset
    ESP_ERROR_CHECK(ledc_channel_config(&bl_channel));

    // Initialize TFT Display
    esp_lcd_panel_io_handle_t io_handle = NULL;
    esp_lcd_panel_handle_t panel_handle = NULL;
    ESP_ERROR_CHECK(display_init(&panel_handle, &io_handle));

    ESP_LOGI(TAG, "Hardware initialized successfully");
    return ESP_OK;
}

static esp_err_t initialize_storage(void)
{
    ESP_LOGI(TAG, "Mounting SPIFFS...");

    esp_vfs_spiffs_conf_t conf = {
        .base_path = "/spiffs",
        .partition_label = "spiffs",
        .max_files = 10,
        .format_if_mount_failed = true,
    };

    esp_err_t ret = esp_vfs_spiffs_register(&conf);
    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount or format filesystem");
        } else if (ret == ESP_ERR_NOT_FOUND) {
            ESP_LOGE(TAG, "Failed to find SPIFFS partition");
        } else {
            ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
        }
        return ret;
    }

    size_t total = 0, used = 0;
    esp_spiffs_info(conf.partition_label, &total, &used);
    ESP_LOGI(TAG, "SPIFFS: %zu KB total, %zu KB used", total / 1024, used / 1024);

    return ESP_OK;
}

// ============================================================================
// Boot Info - Shows A/B partition
// ============================================================================
static void print_boot_info(void)
{
    const esp_partition_t* running = esp_ota_get_running_partition();
    const esp_partition_t* next = esp_ota_get_next_update_partition(NULL);

    ESP_LOGI(TAG, "Boot Partition: %s (0x%06" PRIX32 ", %" PRIu32 " bytes)",
             running ? running->label : "unknown",
             running ? running->address : 0,
             running ? running->size : 0);

    if (next) {
        ESP_LOGI(TAG, "Next OTA Partition: %s (0x%06" PRIX32 ", %" PRIu32 " bytes)",
                 next->label, next->address, next->size);
    }

    // Show on display during boot
    if (running) {
        char part_slot = '?';
        if (strcmp(running->label, "ota_0") == 0) part_slot = 'A';
        else if (strcmp(running->label, "ota_1") == 0) part_slot = 'B';

        // This will be shown by display_init -> show_boot_screen
        ESP_LOGI(TAG, "Boot Slot: %c (%s)", part_slot, running->label);
    }

    // Check rollback status
    esp_ota_img_states_t ota_state;
    if (esp_ota_get_state_partition(running, &ota_state) == ESP_OK) {
        const char* state_str = "unknown";
        switch (ota_state) {
            case ESP_OTA_IMG_NEW: state_str = "NEW (pending verification)"; break;
            case ESP_OTA_IMG_PENDING_VERIFY: state_str = "PENDING_VERIFY"; break;
            case ESP_OTA_IMG_VALID: state_str = "VALID"; break;
            case ESP_OTA_IMG_INVALID: state_str = "INVALID"; break;
            case ESP_OTA_IMG_ABORTED: state_str = "ABORTED"; break;
            case ESP_OTA_IMG_UNDEFINED: state_str = "UNDEFINED"; break;
        }
        ESP_LOGI(TAG, "OTA State: %s", state_str);

        // If NEW, mark as valid after successful boot
        if (ota_state == ESP_OTA_IMG_NEW || ota_state == ESP_OTA_IMG_PENDING_VERIFY) {
            ESP_LOGI(TAG, "Marking running partition as valid...");
            esp_ota_mark_app_valid_cancel_rollback();
        }
    }
}

// ============================================================================
// Game Task - Core game loop (60 second ticks)
// ============================================================================
static void game_task(void* arg)
{
    ESP_LOGI(TAG, "Game task started");

    TickType_t last_wake = xTaskGetTickCount();
    const TickType_t tick_period = pdMS_TO_TICKS(PET_TICK_INTERVAL_MS);

    PetState* pet = pet_mgr_get_pet(&g_pet_mgr);
    const RulesData* rules = rules_get_data(&g_rules_mgr);

    while (1) {
        vTaskDelayUntil(&last_wake, tick_period);

        if (!rules || !rules->valid || !pet || !pet->initialized) {
            continue;
        }

        // Process game tick
        engine_tick(pet, rules, PET_TICK_INTERVAL_MS);

        // Check care mistakes
        engine_check_care_mistakes(pet, rules, esp_timer_get_time() / 1000);

        // Update missions
        engine_update_missions(pet, rules);

        // Update lifecycle
        engine_update_lifecycle(pet, rules);

        // Mark dirty for save
        pet_mgr_mark_dirty(&g_pet_mgr);

        // Trigger display update
        display_mgr_set_screen(&g_disp_mgr, SCREEN_MAIN);
    }
}

// ============================================================================
// Network Task - WiFi, HTTP, WebSocket
// ============================================================================
static void network_task(void* arg)
{
    ESP_LOGI(TAG, "Network task started");
    bool wifi_was_connected = false;
    bool wifi_unconfigured_logged = false;

    // Connect to WiFi
    esp_err_t ret = net_wifi_connect(&g_net_mgr);
    if (ret == ESP_OK) {
        xEventGroupSetBits(s_sync_events, EVT_WIFI_CONNECTED);
        ESP_LOGI(TAG, "WiFi connected");

        // Register/login
        ret = net_register_device(&g_net_mgr, TRAINER_NAME, DEFAULT_BIOME);
        if (ret != ESP_OK) {
            ret = net_login(&g_net_mgr);
        }

        // Initial sync
        if (ret == ESP_OK) {
            net_full_sync(&g_net_mgr, pet_mgr_get_pet(&g_pet_mgr), pet_mgr_get_partner(&g_pet_mgr));
        }

        // Connect WebSocket
        ret = net_ws_connect(&g_net_mgr);
        if (ret == ESP_OK) {
            xEventGroupSetBits(s_sync_events, EVT_WS_CONNECTED);
        }
    } else if (!wifi_unconfigured_logged) {
        ESP_LOGW(TAG, "WiFi is not configured or unavailable; retrying automatically");
        wifi_unconfigured_logged = true;
    }

    TickType_t last_sync = xTaskGetTickCount();
    TickType_t last_ws_poll = xTaskGetTickCount();
    TickType_t last_wifi_retry = xTaskGetTickCount();
    const TickType_t sync_period = pdMS_TO_TICKS(SERVER_SYNC_INTERVAL_MS);
    const TickType_t ws_poll_period = pdMS_TO_TICKS(100);
    const TickType_t wifi_retry_period = pdMS_TO_TICKS(10000);

    while (1) {
        TickType_t now = xTaskGetTickCount();

        // Poll WebSocket
        if (now - last_ws_poll >= ws_poll_period) {
            net_ws_poll(&g_net_mgr);
            last_ws_poll = now;
        }

        // Periodic server sync
        if (now - last_sync >= sync_period) {
            if (net_wifi_is_connected(&g_net_mgr)) {
                net_full_sync(&g_net_mgr, pet_mgr_get_pet(&g_pet_mgr), pet_mgr_get_partner(&g_pet_mgr));
            }
            last_sync = now;
        }

        // ESP-IDF reconnects the station from its event handler. Retry at a
        // bounded interval and only log connection state changes.
        if (net_wifi_is_connected(&g_net_mgr)) {
            if (!wifi_was_connected) {
                ESP_LOGI(TAG, "WiFi connection restored");
            }
            wifi_was_connected = true;
        } else {
            if (wifi_was_connected) {
                ESP_LOGW(TAG, "WiFi disconnected; reconnecting");
                wifi_was_connected = false;
            }
            if (now - last_wifi_retry >= wifi_retry_period) {
                net_wifi_connect(&g_net_mgr);
                last_wifi_retry = now;
            }
        }

        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

// ============================================================================
// Display Task - Rendering at ~30 FPS
// ============================================================================
static void display_task(void* arg)
{
    ESP_LOGI(TAG, "Display task started");

    TickType_t last_wake = xTaskGetTickCount();
    const TickType_t frame_period = pdMS_TO_TICKS(33); // ~30 FPS

    while (1) {
        vTaskDelayUntil(&last_wake, frame_period);

        uint32_t delta_ms = 33;
        display_mgr_update(&g_disp_mgr, delta_ms);
        display_mgr_render(&g_disp_mgr);
    }
}

// ============================================================================
// OTA Task - Periodic update checks
// ============================================================================
static void ota_task(void* arg)
{
    ESP_LOGI(TAG, "OTA task started");

    // Initial check after boot
    vTaskDelay(pdMS_TO_TICKS(30000));
    ota_check_for_updates(&g_ota_mgr);

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(60000)); // Check every minute if auto enabled

        if (g_ota_mgr.auto_check_enabled) {
            uint32_t now = esp_timer_get_time() / 1000;
            if (now - g_ota_mgr.last_check_time >= g_ota_mgr.check_interval_ms) {
                ota_check_for_updates(&g_ota_mgr);
            }
        }
    }
}

// ============================================================================
// Callbacks
// ============================================================================
static void on_rules_changed(const RulesData* rules, void* user_data)
{
    ESP_LOGI(TAG, "Rules changed, reloading...");
    xEventGroupSetBits(s_sync_events, EVT_RULES_LOADED);

    // Update display manager with new rules
    display_mgr_init(&g_disp_mgr, pet_mgr_get_pet(&g_pet_mgr),
                     pet_mgr_get_partner(&g_pet_mgr), rules);
}

static void on_pet_changed(const PetState* pet, void* user_data)
{
    // Pet state changed, mark for save
    pet_mgr_mark_dirty(&g_pet_mgr);
}

static void on_partner_updated(const PetState* partner, void* user_data)
{
    ESP_LOGI(TAG, "Partner state updated");
    pet_mgr_mark_partner_dirty(&g_pet_mgr);
}

static void on_sync_complete(bool success, void* user_data)
{
    if (success) {
        ESP_LOGI(TAG, "Sync complete");
        xEventGroupSetBits(s_sync_events, EVT_SYNC_COMPLETE);
    } else {
        ESP_LOGW(TAG, "Sync failed");
    }
}

static void on_ota_progress(uint8_t percent, const char* msg, void* user_data)
{
    ESP_LOGI(TAG, "OTA Progress: %d%% - %s", percent, msg);
    // Update display
    display_mgr_set_screen(&g_disp_mgr, SCREEN_OTA_UPDATE);
    render_ota_screen(&g_disp_mgr, msg, percent);
}

static void on_ota_complete(bool success, const char* version, void* user_data)
{
    if (success) {
        ESP_LOGI(TAG, "OTA complete: %s", version);
        xEventGroupSetBits(s_sync_events, EVT_OTA_COMPLETE);

        if (g_ota_mgr.state == OTA_STATE_REBOOTING) {
            ESP_LOGI(TAG, "Rebooting for firmware update...");
            vTaskDelay(pdMS_TO_TICKS(1000));
            esp_restart();
        }
    } else {
        ESP_LOGE(TAG, "OTA failed: %s", version);
    }
}

static void on_touch_event(const TouchPoint* point, void* user_data)
{
    display_mgr_handle_touch(&g_disp_mgr, point->x, point->y, point->pressed);
}

static void on_ws_message(const WSMessage* msg, void* user_data)
{
    ESP_LOGD(TAG, "WS Message: type=%d, from=%s", msg->type, msg->from_device);

    switch (msg->type) {
        case WS_MSG_TYPE_TREAT_RECEIVE: {
            // Parse treat payload and apply to pet
            cJSON* json = cJSON_Parse(msg->payload);
            if (json) {
                cJSON* item = cJSON_GetObjectItem(json, "item_id");
                cJSON* count = cJSON_GetObjectItem(json, "count");
                if (item && count) {
                    engine_add_item(pet_mgr_get_pet(&g_pet_mgr), item->valuestring, count->valueint);
                    pet_mgr_mark_dirty(&g_pet_mgr);
                }
                cJSON_Delete(json);
            }
            break;
        }
        case WS_MSG_TYPE_MAGIC_CAST: {
            // Handle co-op magic
            cJSON* json = cJSON_Parse(msg->payload);
            if (json) {
                cJSON* spell = cJSON_GetObjectItem(json, "spell_id");
                if (spell) {
                    engine_cast_spell(pet_mgr_get_pet(&g_pet_mgr), rules_get_data(&g_rules_mgr),
                                      spell->valuestring, pet_mgr_get_partner(&g_pet_mgr));
                    pet_mgr_mark_dirty(&g_pet_mgr);
                }
                cJSON_Delete(json);
            }
            break;
        }
        case WS_MSG_TYPE_TRADE_PROPOSE: {
            // Handle trade proposal
            // Show on display for user acceptance
            break;
        }
        case WS_MSG_TYPE_SYNC_RESPONSE: {
            // Handle partner sync response
            pet_mgr_handle_partner_data(&g_pet_mgr, msg->payload);
            break;
        }
        default:
            break;
    }
}