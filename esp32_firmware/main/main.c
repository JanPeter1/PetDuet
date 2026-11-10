/**
 * PetDuet - Dual-Device Tamagotchi RPG
 * ESP32 CYD Firmware Entry Point
 */

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_err.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_spi_flash.h"
#include "driver/gpio.h"

#include "config.h"
#include "CYD_Pins.h"
#include "rule_engine.h"
#include "pet_state.h"
#include "cyd_display.h"
#include "wifi_manager.h"
#include "ota_client.h"

static const char *TAG = "PETDUET_MAIN";

// Forward declarations
static void petduet_init_hardware(void);
static void petduet_init_nvs(void);
static void petduet_init_rules(void);
static void petduet_init_pet_state(void);
static void petduet_init_display(void);
static void petduet_init_wifi(void);
static void petduet_init_ota(void);
static void petduet_start_game_loop(void);

void app_main(void)
{
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "PetDuet CYD Firmware v%s", PETDUET_VERSION);
    ESP_LOGI(TAG, "Device ID: %s", DEVICE_ID);
    ESP_LOGI(TAG, "Trainer: %s", TRAINER_NAME);
    ESP_LOGI(TAG, "Biome: %s", DEFAULT_BIOME);
    ESP_LOGI(TAG, "========================================");

    // Initialize hardware (GPIOs, SPI, etc.)
    petduet_init_hardware();

    // Initialize NVS for persistent storage
    petduet_init_nvs();

    // Load game rules from SPIFFS
    petduet_init_rules();

    // Initialize or load pet state
    petduet_init_pet_state();

    // Initialize display (LVGL + ILI9341)
    petduet_init_display();

    // Initialize WiFi and connect
    petduet_init_wifi();

    // Initialize OTA client
    petduet_init_ota();

    // Start main game loop (tick decay, care mistakes, etc.)
    petduet_start_game_loop();

    ESP_LOGI(TAG, "PetDuet initialization complete. Entering main loop...");
}

// Hardware initialization: GPIOs, SPI bus for TFT and Touch
static void petduet_init_hardware(void)
{
    ESP_LOGI(TAG, "Initializing hardware...");

    // Configure RGB LED pins (Active Low)
    gpio_config_t led_conf = {
        .pin_bit_mask = (1ULL << PIN_LED_R) | (1ULL << PIN_LED_G) | (1ULL << PIN_LED_B),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&led_conf);
    // Turn off all LEDs (Active Low = High = Off)
    gpio_set_level(PIN_LED_R, 1);
    gpio_set_level(PIN_LED_G, 1);
    gpio_set_level(PIN_LED_B, 1);

    // Configure Buzzer
    gpio_config_t buzzer_conf = {
        .pin_bit_mask = (1ULL << PIN_BUZZER),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&buzzer_conf);
    gpio_set_level(PIN_BUZZER, 0);

    ESP_LOGI(TAG, "Hardware initialized");
}

// Initialize NVS partition
static void petduet_init_nvs(void)
{
    ESP_LOGI(TAG, "Initializing NVS...");
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    ESP_LOGI(TAG, "NVS initialized");
}

// Load rules.json from SPIFFS into rule engine
static void petduet_init_rules(void)
{
    ESP_LOGI(TAG, "Loading game rules from SPIFFS...");
    esp_err_t err = rule_engine_load_from_spiffs("/spiffs/rules.json");
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Failed to load rules from SPIFFS, using defaults: %s", esp_err_to_name(err));
        rule_engine_load_defaults();
    } else {
        ESP_LOGI(TAG, "Rules loaded successfully: v%s", rule_engine_get_version());
    }
}

// Initialize pet state from NVS or create new
static void petduet_init_pet_state(void)
{
    ESP_LOGI(TAG, "Initializing pet state...");
    pet_state_init();
    ESP_LOGI(TAG, "Pet state ready: %s (Lv.%d)", pet_state_get_name(), pet_state_get_level());
}

// Initialize display with LVGL
static void petduet_init_display(void)
{
    ESP_LOGI(TAG, "Initializing display...");
    cyd_display_init();
    ESP_LOGI(TAG, "Display initialized");
}

// Initialize WiFi manager
static void petduet_init_wifi(void)
{
    ESP_LOGI(TAG, "Initializing WiFi...");
    wifi_manager_init();
    wifi_manager_connect(WIFI_SSID, WIFI_PASSWORD);
    ESP_LOGI(TAG, "WiFi initialization started");
}

// Initialize OTA client
static void petduet_init_ota(void)
{
    ESP_LOGI(TAG, "Initializing OTA client...");
    ota_client_init(PHP_SERVER_BASE_URL "/api/ota/check");
    ESP_LOGI(TAG, "OTA client initialized");
}

// Start main game loop task
static void petduet_start_game_loop(void)
{
    ESP_LOGI(TAG, "Starting game loop task...");
    xTaskCreate(pet_state_game_loop_task, "game_loop", 4096, NULL, 5, NULL);
    xTaskCreate(cyd_display_render_task, "display_render", 8192, NULL, 6, NULL);
}