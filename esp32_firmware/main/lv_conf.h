/**
 * LVGL v8.4 Configuration for ESP32-2432S028R (CYD 320x240, ESP-IDF 5/6)
 */
#if 1
#ifndef LV_CONF_H
#define LV_CONF_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* Color settings: 16-bit RGB565 Big-Endian for direct ILI9341 DMA */
#define LV_COLOR_DEPTH     16
#define LV_COLOR_16_SWAP   1
#define LV_COLOR_SCREEN_TRANSP 0
#define LV_COLOR_MIX_ROUND_OFS 0
#define LV_COLOR_CHROMA_KEY lv_color_hex(0x000000)

/* Memory settings (conserve ESP32 internal DRAM alongside WiFi/TLS) */
#define LV_MEM_CUSTOM      0
#define LV_MEM_SIZE        (32U * 1024U)
#define LV_MEM_ADR         0
#define LV_MEM_BUF_MAX_NUM 16
#define LV_MEMCPY_MEMSET_STD 1

/* HAL settings */
#define LV_DISP_DEF_REFR_PERIOD 20
#define LV_INDEV_DEF_READ_PERIOD 25
#define LV_TICK_CUSTOM     0
#define LV_DPI_DEF         130

/* Drawing & GPU */
#define LV_DRAW_COMPLEX    1
#define LV_SHADOW_CACHE_SIZE 0
#define LV_CIRCLE_CACHE_SIZE 4
#define LV_LAYER_SIMPLE_BUF_SIZE (8 * 1024)
#define LV_IMG_CACHE_DEF_SIZE 0
#define LV_GRADIENT_MAX_STOPS 2
#define LV_GRAD_CACHE_DEF_SIZE 0
#define LV_DISP_ROT_MAX_BUF (10 * 1024)

/* Logging disabled for production flash/speed */
#define LV_USE_LOG         0

/* Asserts */
#define LV_USE_ASSERT_NULL          1
#define LV_USE_ASSERT_MALLOC        1
#define LV_USE_ASSERT_STYLE         0
#define LV_USE_ASSERT_MEM_INTEGRITY 0
#define LV_USE_ASSERT_OBJ           0

/* Features */
#define LV_USE_USER_DATA   1
#define LV_USE_PERF_MONITOR 0
#define LV_USE_MEM_MONITOR  0

/* Built-in Montserrat fonts */
#define LV_FONT_MONTSERRAT_8   1
#define LV_FONT_MONTSERRAT_10  1
#define LV_FONT_MONTSERRAT_12  1
#define LV_FONT_MONTSERRAT_14  1
#define LV_FONT_MONTSERRAT_16  1
#define LV_FONT_MONTSERRAT_20  1
#define LV_FONT_DEFAULT        &lv_font_montserrat_12

/* Widgets enabled */
#define LV_USE_ARC        1
#define LV_USE_BAR        1
#define LV_USE_BTN        1
#define LV_USE_BTNMATRIX  1
#define LV_USE_CANVAS     1
#define LV_USE_CHECKBOX   1
#define LV_USE_DROPDOWN   1
#define LV_USE_IMG        1
#define LV_USE_LABEL      1
#define LV_USE_LINE       1
#define LV_USE_ROLLER     1
#define LV_USE_SLIDER     1
#define LV_USE_SWITCH     1
#define LV_USE_TEXTAREA   1
#define LV_USE_TABLE      1

/* Extra widgets */
#define LV_USE_TABVIEW    1
#define LV_USE_MSGBOX     1
#define LV_USE_FLEX       1
#define LV_USE_GRID       1

/* Themes */
#define LV_USE_THEME_DEFAULT 1
#define LV_THEME_DEFAULT_DARK 1
#define LV_THEME_DEFAULT_GROW 0
#define LV_THEME_DEFAULT_TRANSITION_TIME 0

#endif /* LV_CONF_H */
#endif
