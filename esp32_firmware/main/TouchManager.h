/**
 * ============================================================================
 * PetDuet ESP32 CYD - Touch Manager (XPT2046)
 * ============================================================================
 * Resistive touch controller driver with calibration
 * ============================================================================
 */
#ifndef TOUCH_MANAGER_H
#define TOUCH_MANAGER_H

#include <stdint.h>
#include <stdbool.h>
#include "CYD_Pins.h"

// ============================================================================
// Touch Data
// ============================================================================
typedef struct {
    int16_t x;
    int16_t y;
    uint16_t pressure;
    bool pressed;
    uint32_t timestamp;
} TouchPoint;

typedef struct {
    int16_t x_min, x_max;
    int16_t y_min, y_max;
    bool swap_xy;
    bool calibrated;
} TouchCalibration;

// ============================================================================
// Touch Manager Context
// ============================================================================
typedef struct {
    spi_device_handle_t spi_dev;
    TouchCalibration calibration;

    // Raw readings
    uint16_t raw_x, raw_y, raw_z1, raw_z2;

    // Filtered/processed
    TouchPoint current;
    TouchPoint previous;

    // Debounce
    uint32_t last_change_time;
    uint8_t debounce_count;

    // Calibration data (stored in NVS)
    bool calibration_valid;

    // Callback
    void (*on_touch)(const TouchPoint* point, void* user_data);
    void* user_data;
} TouchManager;

// ============================================================================
// API
// ============================================================================

// Initialization
esp_err_t touch_init(TouchManager* tm);
void touch_deinit(TouchManager* tm);

// Main loop
void touch_poll(TouchManager* tm);

// Reading
bool touch_read_raw(TouchManager* tm, uint16_t* x, uint16_t* y, uint16_t* z);
bool touch_read_point(TouchManager* tm, TouchPoint* point);

// Calibration
esp_err_t touch_calibrate(TouchManager* tm, esp_lcd_panel_handle_t panel);
esp_err_t touch_load_calibration(TouchManager* tm);
esp_err_t touch_save_calibration(TouchManager* tm);
void touch_set_calibration(TouchManager* tm, int16_t x_min, int16_t x_max,
                           int16_t y_min, int16_t y_max, bool swap_xy);

// Coordinate transformation
void touch_raw_to_screen(const TouchManager* tm, uint16_t raw_x, uint16_t raw_y,
                         int16_t* screen_x, int16_t* screen_y);

// Callback
void touch_set_callback(TouchManager* tm,
                        void (*callback)(const TouchPoint*, void*), void* user_data);

// Utility
bool touch_is_pressed(const TouchManager* tm);
TouchPoint touch_get_last_point(const TouchManager* tm);

#endif // TOUCH_MANAGER_H