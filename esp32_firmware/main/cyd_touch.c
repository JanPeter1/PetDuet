#include "cyd_touch.h"
#include "../CYD_Pins.h"
#include "../cyd_config.h"

#include "driver/spi_master.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "CYD_TOUCH";
static spi_device_handle_t s_touch_spi = NULL;

static uint16_t xpt2046_read_channel(uint8_t cmd) {
  if (!s_touch_spi) return 0;
  uint8_t tx[3] = {cmd, 0x00, 0x00};
  uint8_t rx[3] = {0, 0, 0};
  spi_transaction_t t;
  memset(&t, 0, sizeof(t));
  t.length = 24;
  t.tx_buffer = tx;
  t.rx_buffer = rx;
  if (spi_device_polling_transmit(s_touch_spi, &t) != ESP_OK) {
    return 0;
  }
  return (uint16_t)(((rx[1] << 8) | rx[2]) >> 3);
}

void cyd_touch_init(void) {
  spi_bus_config_t buscfg;
  memset(&buscfg, 0, sizeof(buscfg));
  buscfg.mosi_io_num = XPT2046_MOSI;
  buscfg.miso_io_num = XPT2046_MISO;
  buscfg.sclk_io_num = XPT2046_CLK;
  buscfg.quadwp_io_num = -1;
  buscfg.quadhd_io_num = -1;
  buscfg.data4_io_num = -1;
  buscfg.data5_io_num = -1;
  buscfg.data6_io_num = -1;
  buscfg.data7_io_num = -1;
  buscfg.max_transfer_sz = 32;
  // These are tiny three-byte XPT2046 transfers; polling without DMA avoids
  // bounce-buffer and alignment issues with the stack-based tx/rx packets.
  esp_err_t ret = spi_bus_initialize(BOARD_TOUCH_SPI_HOST, &buscfg, SPI_DMA_DISABLED);
  if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
    ESP_LOGE(TAG, "Touch SPI bus init failed: %s", esp_err_to_name(ret));
    return;
  }

  spi_device_interface_config_t devcfg = {
    .mode = 0,
    .clock_speed_hz = 2500 * 1000, // 2.5 MHz XPT2046
    .spics_io_num = XPT2046_CS,
    .queue_size = 1
  };
  ret = spi_bus_add_device(BOARD_TOUCH_SPI_HOST, &devcfg, &s_touch_spi);
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "Touch SPI add device failed: %s", esp_err_to_name(ret));
  } else {
    ESP_LOGI(TAG, "XPT2046 touch initialized on SPI3_HOST (2.5MHz, pressure polling)");
  }
}

static uint16_t map_clamp(int32_t v, int32_t in_min, int32_t in_max, int32_t out_min, int32_t out_max) {
  if (in_max <= in_min) return (uint16_t)out_min;
  int32_t res = ((v - in_min) * (out_max - out_min)) / (in_max - in_min) + out_min;
  if (res < out_min) res = out_min;
  if (res > out_max) res = out_max;
  return (uint16_t)res;
}

bool cyd_touch_read(uint16_t *out_x, uint16_t *out_y) {
  if (!s_touch_spi) return false;

  // Poll pressure instead of gating on IRQ: GPIO36 has no internal pull-up,
  // and some CYD revisions leave the PENIRQ line unreliable.
  int32_t sum_x = 0;
  int32_t sum_y = 0;
  int valid = 0;

  (void)xpt2046_read_channel(0xB1); // Wake ADC
  for (int i = 0; i < 4; i++) {
    uint16_t z1 = xpt2046_read_channel(0xB1);
    uint16_t z2 = xpt2046_read_channel(0xC1);
    int32_t pressure = (int32_t)z1 + (4095 - (int32_t)z2);
    if (pressure < 350) continue;

    uint16_t raw_x = xpt2046_read_channel(0xD1); // X in landscape orientation
    uint16_t raw_y = xpt2046_read_channel(0x91); // Y in landscape orientation
    if (i > 0) {
      sum_x += raw_x;
      sum_y += raw_y;
      valid++;
    }
  }

  // Power down touch ADC until next IRQ
  (void)xpt2046_read_channel(0xD0);

  if (valid == 0) return false;

  int32_t avg_x = sum_x / valid;
  int32_t avg_y = sum_y / valid;

  if (out_x) *out_x = map_clamp(avg_x, BOARD_TOUCH_MIN_X, BOARD_TOUCH_MAX_X, 0, BOARD_DISPLAY_WIDTH - 1);
  if (out_y) *out_y = map_clamp(avg_y, BOARD_TOUCH_MIN_Y, BOARD_TOUCH_MAX_Y, 0, BOARD_DISPLAY_HEIGHT - 1);
  return true;
}
