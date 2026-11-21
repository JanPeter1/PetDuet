/**
 * ============================================================================
 * PetDuet ESP32 CYD - Hardware Pin Definitions (ESP32-2432S028R)
 * ============================================================================
 * Cheap Yellow Display (CYD) Pin Mapping
 * TFT: ILI9341 320x240 SPI
 * Touch: XPT2046 Resistive Touch Controller
 * ============================================================================
 */
#ifndef CYD_PINS_H
#define CYD_PINS_H

// ============================================================================
// TFT ILI9341 SPI (HSPI - Host SPI2)
// ============================================================================
#define TFT_SPI_HOST        SPI2_HOST
#define TFT_CS              15
#define TFT_DC              2
#define TFT_RST             -1  // Not connected, use software reset
#define TFT_BL              21  // Backlight control
#define TFT_MOSI            13
#define TFT_MISO            12
#define TFT_SCLK            14
#define TFT_BACKLIGHT_ON    1   // HIGH = on

// TFT SPI Settings
#define TFT_SPI_FREQ_HZ     40000000
#define TFT_SPI_READ_FREQ_HZ 20000000

// ============================================================================
// XPT2046 Touch Controller (VSPI - Host SPI3)
// ============================================================================
#define TOUCH_SPI_HOST      SPI3_HOST
#define TOUCH_CS            33
#define TOUCH_MOSI          32
#define TOUCH_MISO          39
#define TOUCH_CLK           25
#define TOUCH_IRQ           36
#define TOUCH_SPI_FREQ_HZ   2500000

// Touch Calibration (adjust per device)
#define TOUCH_X_MIN         200
#define TOUCH_X_MAX         3800
#define TOUCH_Y_MIN         240
#define TOUCH_Y_MAX         3800
#define TOUCH_SWAP_XY       0

// ============================================================================
// RGB LED (Active Low - Common Anode)
// ============================================================================
#define LED_R               4
#define LED_G               16
#define LED_B               17
#define LED_ACTIVE_LOW      1

// ============================================================================
// Buzzer (PWM via LEDC)
// ============================================================================
#define BUZZER_GPIO         26
#define BUZZER_LEDC_CHANNEL 0
#define BUZZER_LEDC_TIMER   0
#define BUZZER_FREQ_HZ      4000
#define BUZZER_RESOLUTION   8

// ============================================================================
// Light Dependent Resistor (LDR) - ADC1_CH0 (GPIO1)
// Note: Shares with UART0_TX, only works if serial not used for logging
// ============================================================================
#define LDR_GPIO            1
#define LDR_ADC_CHANNEL     ADC_CHANNEL_0
#define LDR_ADC_UNIT        ADC_UNIT_1

// ============================================================================
// MicroSD Card (SPI) - Not used in current design
// ============================================================================
#define SD_SPI_HOST         SPI3_HOST  // Shared with touch
#define SD_CS               5
#define SD_MOSI             32  // Shared with touch
#define SD_MISO             39  // Shared with touch
#define SD_SCLK             25  // Shared with touch

// ============================================================================
// Display Dimensions
// ============================================================================
#define TFT_WIDTH           320
#define TFT_HEIGHT          240

// ============================================================================
// Button Layout Constants (for touch regions)
// ============================================================================
// Top Bar
#define TOP_BAR_HEIGHT      24

// Left Action Buttons (Feed, Play, Bath, RPG, Save)
#define LEFT_BTN_X          0
#define LEFT_BTN_Y          (TOP_BAR_HEIGHT)
#define LEFT_BTN_W          50
#define LEFT_BTN_H          40
#define LEFT_BTN_GAP        2

// Right Action Buttons (Treat, Chat, Magic+, Quest, Trade)
#define RIGHT_BTN_X         (TFT_WIDTH - 50)
#define RIGHT_BTN_Y         (TOP_BAR_HEIGHT)
#define RIGHT_BTN_W         50
#define RIGHT_BTN_H         40
#define RIGHT_BTN_GAP       2

// Center Pet Display Areas
#define PET_AREA_W          122
#define PET_AREA_H          160
#define YOUR_PET_X          54
#define YOUR_PET_Y          (TOP_BAR_HEIGHT)
#define PARTNER_PET_X       (TFT_WIDTH - 54 - PET_AREA_W)
#define PARTNER_PET_Y       (TOP_BAR_HEIGHT)
#define SYNC_INDICATOR_X    (TFT_WIDTH / 2 - 4)
#define SYNC_INDICATOR_Y    (TOP_BAR_HEIGHT + 10)

// Bottom HUD
#define HUD_HEIGHT          40
#define HUD_Y               (TFT_HEIGHT - HUD_HEIGHT)

#endif // CYD_PINS_H