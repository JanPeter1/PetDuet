/**
 * ============================================================================
 * PetDuet ESP32 CYD - Touch Manager Implementation (XPT2046)
 * ============================================================================
 * Resistive touch controller driver with calibration
 * ============================================================================
 */
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "esp_log.h"
#include "esp_timer.h"
#include "nvs.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "TouchManager.h"
#include "CYD_Pins.h"
#include "PetGraphics.h"

static const char* TAG = "TouchMgr";

// XPT2046 Commands
#define XPT2046_CMD_X     0xD0
#define XPT2046_CMD_Y     0x90
#define XPT2046_CMD_Z1    0xB0
#define XPT2046_CMD_Z2    0xC0

esp_err_t touch_init(TouchManager* tm) {
    if (!tm) return ESP_ERR_INVALID_ARG;

    memset(tm, 0, sizeof(TouchManager));

    // Configure SPI device
    spi_device_interface_config_t devcfg = {
        .command_bits = 0,
        .address_bits = 0,
        .dummy_bits = 0,
        .mode = 0,
        .clock_source = SPI_CLK_SRC_DEFAULT,
        .duty_cycle_pos = 128,
        .clock_speed_hz = TOUCH_SPI_FREQ_HZ,
        .input_delay_ns = 0,
        .spics_io_num = TOUCH_CS,
        .flags = SPI_DEVICE_NO_DUMMY,
        .queue_size = 3,
        .pre_cb = NULL,
        .post_cb = NULL,
    };

    esp_err_t ret = spi_bus_add_device(TOUCH_SPI_HOST, &devcfg, &tm->spi_dev);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to add touch SPI device: %s", esp_err_to_name(ret));
        return ret;
    }

    // Load calibration from NVS
    touch_load_calibration(tm);

    // Set defaults if not calibrated
    if (!tm->calibration_valid) {
        tm->calibration.x_min = TOUCH_X_MIN;
        tm->calibration.x_max = TOUCH_X_MAX;
        tm->calibration.y_min = TOUCH_Y_MIN;
        tm->calibration.y_max = TOUCH_Y_MAX;
        tm->calibration.swap_xy = TOUCH_SWAP_XY;
    }

    // Configure IRQ pin (optional, for interrupt-driven)
    gpio_config_t irq_cfg = {
        .pin_bit_mask = (1ULL << TOUCH_IRQ),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&irq_cfg));

    ESP_LOGI(TAG, "Touch manager initialized");
    return ESP_OK;
}

void touch_deinit(TouchManager* tm) {
    if (!tm) return;
    if (tm->spi_dev) {
        spi_bus_remove_device(tm->spi_dev);
    }
}

static esp_err_t touch_read_register(TouchManager* tm, uint8_t cmd, uint16_t* value) {
    if (!tm || !tm->spi_dev || !value) return ESP_ERR_INVALID_ARG;

    spi_transaction_t trans;
    memset(&trans, 0, sizeof(trans));
    trans.flags = SPI_TRANS_USE_TXDATA | SPI_TRANS_USE_RXDATA;
    trans.cmd = 0;
    trans.addr = 0;
    trans.length = 8;
    trans.rxlength = 16;
    trans.user = NULL;
    trans.tx_data[0] = cmd;
    trans.tx_data[1] = 0;
    trans.tx_data[2] = 0;
    trans.tx_data[3] = 0;
    trans.rx_buffer = NULL;
    esp_err_t ret = spi_device_polling_transmit(tm->spi_dev, &trans);
    if (ret == ESP_OK) {
        *value = (trans.rx_data[0] << 8) | trans.rx_data[1];
        *value >>= 3; // 12-bit result
    }
    return ret;
}

bool touch_read_raw(TouchManager* tm, uint16_t* x, uint16_t* y, uint16_t* z) {
    if (!tm || !tm->spi_dev) return false;

    uint16_t raw_x, raw_y, raw_z1, raw_z2;

    // Read multiple samples and average
    const int samples = 3;
    uint32_t sum_x = 0, sum_y = 0, sum_z1 = 0, sum_z2 = 0;

    for (int i = 0; i < samples; i++) {
        if (touch_read_register(tm, XPT2046_CMD_X, &raw_x) != ESP_OK) return false;
        if (touch_read_register(tm, XPT2046_CMD_Y, &raw_y) != ESP_OK) return false;
        if (touch_read_register(tm, XPT2046_CMD_Z1, &raw_z1) != ESP_OK) return false;
        if (touch_read_register(tm, XPT2046_CMD_Z2, &raw_z2) != ESP_OK) return false;

        sum_x += raw_x;
        sum_y += raw_y;
        sum_z1 += raw_z1;
        sum_z2 += raw_z2;
    }

    tm->raw_x = sum_x / samples;
    tm->raw_y = sum_y / samples;
    tm->raw_z1 = sum_z1 / samples;
    tm->raw_z2 = sum_z2 / samples;

    if (x) *x = tm->raw_x;
    if (y) *y = tm->raw_y;
    if (z) *z = tm->raw_z1 + tm->raw_z2; // Pressure proxy

    return true;
}

bool touch_read_point(TouchManager* tm, TouchPoint* point) {
    if (!tm || !point) return false;

    uint16_t x, y, z;
    if (!touch_read_raw(tm, &x, &y, &z)) return false;

    // Calculate pressure (simplified)
    uint16_t pressure = (z > 100) ? z : 0;

    // Check if pressed (pressure threshold)
    bool pressed = pressure > 50;

    // Transform to screen coordinates
    int16_t screen_x, screen_y;
    touch_raw_to_screen(tm, x, y, &screen_x, &screen_y);

    // Debounce
    uint32_t now = esp_timer_get_time() / 1000;
    if (pressed != tm->current.pressed) {
        if (now - tm->last_change_time > 20) { // 20ms debounce
            tm->last_change_time = now;
            tm->debounce_count = 0;
            tm->current.pressed = pressed;
        } else {
            tm->debounce_count++;
            if (tm->debounce_count < 3) {
                pressed = tm->current.pressed;
            }
        }
    }

    point->x = screen_x;
    point->y = screen_y;
    point->pressure = pressure;
    point->pressed = pressed;
    point->timestamp = now;

    tm->current = *point;
    return true;
}

void touch_poll(TouchManager* tm) {
    if (!tm) return;

    TouchPoint point;
    if (touch_read_point(tm, &point)) {
        if (tm->on_touch) {
            tm->on_touch(&point, tm->user_data);
        }
    }
}

void touch_raw_to_screen(const TouchManager* tm, uint16_t raw_x, uint16_t raw_y,
                         int16_t* screen_x, int16_t* screen_y) {
    if (!tm || !screen_x || !screen_y) return;

    int16_t x = raw_x;
    int16_t y = raw_y;

    // Apply calibration
    if (tm->calibration.x_max > tm->calibration.x_min) {
        x = (x - tm->calibration.x_min) * TFT_WIDTH /
            (tm->calibration.x_max - tm->calibration.x_min);
    }
    if (tm->calibration.y_max > tm->calibration.y_min) {
        y = (y - tm->calibration.y_min) * TFT_HEIGHT /
            (tm->calibration.y_max - tm->calibration.y_min);
    }

    // Clamp
    x = x < 0 ? 0 : (x >= TFT_WIDTH ? TFT_WIDTH - 1 : x);
    y = y < 0 ? 0 : (y >= TFT_HEIGHT ? TFT_HEIGHT - 1 : y);

    // Swap if needed
    if (tm->calibration.swap_xy) {
        int16_t tmp = x;
        x = y;
        y = tmp;
    }

    *screen_x = x;
    *screen_y = y;
}

esp_err_t touch_calibrate(TouchManager* tm, esp_lcd_panel_handle_t panel) {
    if (!tm || !panel) return ESP_ERR_INVALID_ARG;

    ESP_LOGI(TAG, "Starting touch calibration...");

    // Draw calibration screen
    draw_calibration_screen(panel);

    // Calibration points (4 corners + center)
    struct { int16_t x, y; } points[] = {
        {20, 20}, {TFT_WIDTH - 20, 20}, {TFT_WIDTH - 20, TFT_HEIGHT - 20},
        {20, TFT_HEIGHT - 20}, {TFT_WIDTH / 2, TFT_HEIGHT / 2}
    };

    uint32_t raw_x_sum = 0, raw_y_sum = 0;
    int valid_points = 0;

    for (int i = 0; i < 5; i++) {
        // Draw target
        char buf[32];
        snprintf(buf, sizeof(buf), "Touch point %d/5", i + 1);
        draw_centered_text(panel, 100, buf, TFT_WHITE, TFT_BLACK, 1);

        // Draw crosshair
        draw_fast_hline(panel, points[i].x - 10, points[i].y, 20, TFT_RED);
        draw_fast_vline(panel, points[i].x, points[i].y - 10, 20, TFT_RED);

        // Wait for touch
        uint32_t start = esp_timer_get_time() / 1000;
        while (esp_timer_get_time() / 1000 - start < 10000) { // 10 second timeout
            TouchPoint pt;
            if (touch_read_point(tm, &pt) && pt.pressed) {
                raw_x_sum += tm->raw_x;
                raw_y_sum += tm->raw_y;
                valid_points++;
                vTaskDelay(pdMS_TO_TICKS(500)); // Wait for release
                while (touch_read_point(tm, &pt) && pt.pressed) {
                    vTaskDelay(pdMS_TO_TICKS(10));
                }
                break;
            }
            vTaskDelay(pdMS_TO_TICKS(10));
        }
    }

    if (valid_points >= 3) {
        tm->calibration.x_min = (raw_x_sum / valid_points) - 200;
        tm->calibration.x_max = (raw_x_sum / valid_points) + 200;
        tm->calibration.y_min = (raw_y_sum / valid_points) - 200;
        tm->calibration.y_max = (raw_y_sum / valid_points) + 200;
        tm->calibration.calibrated = true;
        tm->calibration_valid = true;

        touch_save_calibration(tm);
        ESP_LOGI(TAG, "Calibration complete: X(%d-%d) Y(%d-%d)",
                 tm->calibration.x_min, tm->calibration.x_max,
                 tm->calibration.y_min, tm->calibration.y_max);
        return ESP_OK;
    }

    ESP_LOGE(TAG, "Calibration failed - not enough points");
    return ESP_FAIL;
}

esp_err_t touch_load_calibration(TouchManager* tm) {
    if (!tm) return ESP_ERR_INVALID_ARG;

    nvs_handle_t handle;
    esp_err_t err = nvs_open("touch_cal", NVS_READONLY, &handle);
    if (err != ESP_OK) return err;

    int32_t x_min, x_max, y_min, y_max;
    int8_t swap;

    err = nvs_get_i32(handle, "x_min", &x_min);
    if (err == ESP_OK) {
        nvs_get_i32(handle, "x_max", &x_max);
        nvs_get_i32(handle, "y_min", &y_min);
        nvs_get_i32(handle, "y_max", &y_max);
        nvs_get_i8(handle, "swap", &swap);

        tm->calibration.x_min = x_min;
        tm->calibration.x_max = x_max;
        tm->calibration.y_min = y_min;
        tm->calibration.y_max = y_max;
        tm->calibration.swap_xy = swap;
        tm->calibration.calibrated = true;
        tm->calibration_valid = true;
    }

    nvs_close(handle);
    return err;
}

esp_err_t touch_save_calibration(TouchManager* tm) {
    if (!tm) return ESP_ERR_INVALID_ARG;

    nvs_handle_t handle;
    esp_err_t err = nvs_open("touch_cal", NVS_READWRITE, &handle);
    if (err != ESP_OK) return err;

    nvs_set_i32(handle, "x_min", tm->calibration.x_min);
    nvs_set_i32(handle, "x_max", tm->calibration.x_max);
    nvs_set_i32(handle, "y_min", tm->calibration.y_min);
    nvs_set_i32(handle, "y_max", tm->calibration.y_max);
    nvs_set_i8(handle, "swap", tm->calibration.swap_xy);

    err = nvs_commit(handle);
    nvs_close(handle);
    return err;
}

void touch_set_calibration(TouchManager* tm, int16_t x_min, int16_t x_max,
                           int16_t y_min, int16_t y_max, bool swap_xy) {
    if (!tm) return;
    tm->calibration.x_min = x_min;
    tm->calibration.x_max = x_max;
    tm->calibration.y_min = y_min;
    tm->calibration.y_max = y_max;
    tm->calibration.swap_xy = swap_xy;
    tm->calibration.calibrated = true;
    tm->calibration_valid = true;
    touch_save_calibration(tm);
}

void touch_set_callback(TouchManager* tm,
                        void (*callback)(const TouchPoint*, void*), void* user_data) {
    if (!tm) return;
    tm->on_touch = callback;
    tm->user_data = user_data;
}

bool touch_is_pressed(const TouchManager* tm) {
    return tm && tm->current.pressed;
}

TouchPoint touch_get_last_point(const TouchManager* tm) {
    return tm ? tm->current : (TouchPoint){0, 0, 0, false, 0};
}