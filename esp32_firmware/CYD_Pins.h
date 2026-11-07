/**
 * ============================================================================
 * ESP32 Cheap Yellow Display (ESP32-2432S028R) Pin Definitions
 * Hardware Reference: https://github.com/JanPeter1/ESP_CoGo
 * ============================================================================
 */
#ifndef CYD_PINS_H
#define CYD_PINS_H

// ----------------------------------------------------------------------------
// TFT Display: 2.8" ILI9341 SPI (320x240 Landscape)
// ----------------------------------------------------------------------------
#define CYD_TFT_MISO 12
#define CYD_TFT_MOSI 13
#define CYD_TFT_SCLK 14
#define CYD_TFT_CS   15
#define CYD_TFT_DC    2
#define CYD_TFT_RST  -1
#define CYD_TFT_BL   21 // Backlight (PWM / digital output)

// ----------------------------------------------------------------------------
// Resistive Touch Controller: XPT2046 SPI
// ----------------------------------------------------------------------------
#define XPT2046_CLK  25
#define XPT2046_CS   33
#define XPT2046_MOSI 32
#define XPT2046_MISO 39
#define XPT2046_IRQ  36 // Interrupt pin

// ----------------------------------------------------------------------------
// Onboard RGB LED (Active LOW: LOW = ON, HIGH = OFF)
// ----------------------------------------------------------------------------
#define CYD_LED_RED    4
#define CYD_LED_GREEN 16
#define CYD_LED_BLUE  17

// ----------------------------------------------------------------------------
// Onboard Piezo Speaker / Buzzer
// ----------------------------------------------------------------------------
#define CYD_SPEAKER_PIN 26
#define BUZZER_LEDC_CH   0
#define BUZZER_LEDC_RES  8 // 8-bit resolution

// ----------------------------------------------------------------------------
// Light Sensor (LDR - Light Dependent Resistor)
// ----------------------------------------------------------------------------
#define CYD_LDR_PIN     34 // ADC1_CH6

// ----------------------------------------------------------------------------
// Micro-SD Card Interface (SPI)
// ----------------------------------------------------------------------------
#define CYD_SD_CS        5
#define CYD_SD_MOSI     23
#define CYD_SD_MISO     19
#define CYD_SD_SCK      18

#endif // CYD_PINS_H
