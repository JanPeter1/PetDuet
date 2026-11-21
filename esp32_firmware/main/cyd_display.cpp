#include "cyd_display.h"
#include "../CYD_Pins.h"
#include "../cyd_config.h"
#include "cyd_font_5x7.h"

#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <stdlib.h>
#include <string.h>

static const char* TAG = "CYD_DISP";
static spi_device_handle_t s_tft_spi = NULL;
static uint16_t* s_dma_buf = NULL;
static const size_t DMA_BUF_PIXELS = BOARD_DISPLAY_WIDTH * BOARD_DISPLAY_BUFFER_ROWS;

static inline uint16_t swap16(uint16_t c) {
  return (uint16_t)((c >> 8) | (c << 8));
}

// Pre-transfer callback sets DC line from transaction user pointer (0 = cmd, 1 = data)
static void IRAM_ATTR ili_spi_pre_cb(spi_transaction_t* t) {
  int dc = (int)(uintptr_t)t->user;
  gpio_set_level((gpio_num_t)CYD_TFT_DC, dc);
}

bool CydDisplay::init() {
  gpio_config_t dc_conf = {
    .pin_bit_mask = (1ULL << CYD_TFT_DC),
    .mode = GPIO_MODE_OUTPUT,
    .pull_up_en = GPIO_PULLUP_DISABLE,
    .pull_down_en = GPIO_PULLDOWN_DISABLE,
    .intr_type = GPIO_INTR_DISABLE
  };
  gpio_config(&dc_conf);

  spi_bus_config_t buscfg;
  memset(&buscfg, 0, sizeof(buscfg));
  buscfg.mosi_io_num = CYD_TFT_MOSI;
  buscfg.miso_io_num = CYD_TFT_MISO;
  buscfg.sclk_io_num = CYD_TFT_SCLK;
  buscfg.quadwp_io_num = -1;
  buscfg.quadhd_io_num = -1;
  buscfg.data4_io_num = -1;
  buscfg.data5_io_num = -1;
  buscfg.data6_io_num = -1;
  buscfg.data7_io_num = -1;
  buscfg.max_transfer_sz = (int)(DMA_BUF_PIXELS * 2 + 16);
  buscfg.flags = 0;
  buscfg.isr_cpu_id = ESP_INTR_CPU_AFFINITY_AUTO;
  buscfg.intr_flags = 0;
  buscfg.dma_burst_size = 0;
  buscfg.data_io_default_level = 0;
  esp_err_t ret = spi_bus_initialize(BOARD_TFT_SPI_HOST, &buscfg, SPI_DMA_CH_AUTO);
  if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
    ESP_LOGE(TAG, "TFT SPI bus init failed: %s", esp_err_to_name(ret));
    return false;
  }

  spi_device_interface_config_t devcfg;
  memset(&devcfg, 0, sizeof(devcfg));
  devcfg.mode = 0;
  devcfg.clock_speed_hz = 40 * 1000 * 1000; // 40 MHz
  devcfg.spics_io_num = CYD_TFT_CS;
  devcfg.queue_size = 7;
  devcfg.pre_cb = ili_spi_pre_cb;
  devcfg.command_bits = 0;
  devcfg.address_bits = 0;
  devcfg.dummy_bits = 0;
  devcfg.clock_source = SPI_CLK_SRC_DEFAULT;
  devcfg.duty_cycle_pos = 128;
  devcfg.cs_ena_pretrans = 0;
  devcfg.cs_ena_posttrans = 0;
  devcfg.input_delay_ns = 0;
  devcfg.sample_point = SPI_SAMPLING_POINT_PHASE_1;
  devcfg.flags = 0;
  devcfg.post_cb = NULL;
  ret = spi_bus_add_device(BOARD_TFT_SPI_HOST, &devcfg, &s_tft_spi);
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "TFT SPI add device failed: %s", esp_err_to_name(ret));
    return false;
  }

  if (!s_dma_buf) {
    s_dma_buf = (uint16_t*)heap_caps_malloc(DMA_BUF_PIXELS * sizeof(uint16_t), MALLOC_CAP_DMA);
    if (!s_dma_buf) {
      s_dma_buf = (uint16_t*)malloc(DMA_BUF_PIXELS * sizeof(uint16_t));
    }
  }

  // ILI9341 Reset & Initialization Sequence (CYD 2.8" Landscape 320x240)
  writeCmd(0x01); // SWRESET
  vTaskDelay(pdMS_TO_TICKS(120));

  writeCmd(0x11); // SLPOUT
  vTaskDelay(pdMS_TO_TICKS(120));

  static const uint8_t pwrA[] = {0x39, 0x2C, 0x00, 0x34, 0x02};
  writeCmd(0xCB); writeData(pwrA, sizeof(pwrA));

  static const uint8_t pwrB[] = {0x00, 0xC1, 0x30};
  writeCmd(0xCF); writeData(pwrB, sizeof(pwrB));

  static const uint8_t dtA[] = {0x85, 0x00, 0x78};
  writeCmd(0xE8); writeData(dtA, sizeof(dtA));

  static const uint8_t dtB[] = {0x00, 0x00};
  writeCmd(0xEA); writeData(dtB, sizeof(dtB));

  static const uint8_t pwrSeq[] = {0x64, 0x03, 0x12, 0x81};
  writeCmd(0xED); writeData(pwrSeq, sizeof(pwrSeq));

  static const uint8_t pump[] = {0x20};
  writeCmd(0xF7); writeData(pump, sizeof(pump));

  static const uint8_t pwr1[] = {0x23};
  writeCmd(0xC0); writeData(pwr1, sizeof(pwr1));

  static const uint8_t pwr2[] = {0x10};
  writeCmd(0xC1); writeData(pwr2, sizeof(pwr2));

  static const uint8_t vcom1[] = {0x3E, 0x28};
  writeCmd(0xC5); writeData(vcom1, sizeof(vcom1));

  static const uint8_t vcom2[] = {0x86};
  writeCmd(0xC7); writeData(vcom2, sizeof(vcom2));

  // Landscape rotation 1 for ILI9341_2_DRIVER CYD (MV | BGR = 0x28)
  static const uint8_t madctl[] = {0x28};
  writeCmd(0x36); writeData(madctl, sizeof(madctl));

  static const uint8_t pixfmt[] = {0x55}; // 16-bit/pixel RGB565
  writeCmd(0x3A); writeData(pixfmt, sizeof(pixfmt));

  static const uint8_t frmctr[] = {0x00, 0x18};
  writeCmd(0xB1); writeData(frmctr, sizeof(frmctr));

  static const uint8_t dfunc[] = {0x08, 0x82, 0x27};
  writeCmd(0xB6); writeData(dfunc, sizeof(dfunc));

  static const uint8_t gamma3[] = {0x00};
  writeCmd(0xF2); writeData(gamma3, sizeof(gamma3));

  static const uint8_t gammaSet[] = {0x01};
  writeCmd(0x26); writeData(gammaSet, sizeof(gammaSet));

  // CYD Panel Color Inversion Fix (0x21)
  writeCmd(0x21);

  writeCmd(0x13); // NORON
  writeCmd(0x29); // DISPON
  vTaskDelay(pdMS_TO_TICKS(20));

  fillScreen(TFT_BLACK);
  setBacklight(true);
  ESP_LOGI(TAG, "ILI9341 320x240 display ready on SPI2_HOST (40MHz)");
  return true;
}

void CydDisplay::setBacklight(bool on) {
  gpio_set_level((gpio_num_t)CYD_TFT_BL, on ? 1 : 0);
}

void CydDisplay::setInversion(bool on) {
  writeCmd(on ? 0x21 : 0x20);
}

void CydDisplay::writeCmd(uint8_t cmd) {
  if (!s_tft_spi) return;
  spi_transaction_t t;
  memset(&t, 0, sizeof(t));
  t.length = 8;
  t.tx_buffer = &cmd;
  t.user = (void*)0;
  spi_device_polling_transmit(s_tft_spi, &t);
}

void CydDisplay::writeData(const uint8_t* data, size_t len) {
  if (!s_tft_spi || len == 0) return;
  spi_transaction_t t;
  memset(&t, 0, sizeof(t));
  t.length = len * 8;
  t.tx_buffer = data;
  t.user = (void*)1;
  spi_device_polling_transmit(s_tft_spi, &t);
}

void CydDisplay::setAddrWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
  uint8_t caset[4] = {(uint8_t)(x0 >> 8), (uint8_t)(x0 & 0xFF), (uint8_t)(x1 >> 8), (uint8_t)(x1 & 0xFF)};
  uint8_t raset[4] = {(uint8_t)(y0 >> 8), (uint8_t)(y0 & 0xFF), (uint8_t)(y1 >> 8), (uint8_t)(y1 & 0xFF)};
  writeCmd(0x2A);
  writeData(caset, 4);
  writeCmd(0x2B);
  writeData(raset, 4);
  writeCmd(0x2C);
}

void CydDisplay::pushColors(const uint16_t* colors, size_t count) {
  if (!s_tft_spi || count == 0) return;
  size_t sent = 0;
  while (sent < count) {
    size_t chunk = count - sent;
    if (chunk > DMA_BUF_PIXELS) chunk = DMA_BUF_PIXELS;
    spi_transaction_t t;
    memset(&t, 0, sizeof(t));
    t.length = chunk * 16;
    t.tx_buffer = colors + sent;
    t.user = (void*)1;
    spi_device_polling_transmit(s_tft_spi, &t);
    sent += chunk;
  }
}

void CydDisplay::flushRgb565Rect(int x1, int y1, int x2, int y2, const uint16_t* pixels, bool alreadySwapped) {
  if (!s_tft_spi || !pixels || x2 < x1 || y2 < y1) return;
  if (x1 < 0) x1 = 0;
  if (y1 < 0) y1 = 0;
  if (x2 >= BOARD_DISPLAY_WIDTH) x2 = BOARD_DISPLAY_WIDTH - 1;
  if (y2 >= BOARD_DISPLAY_HEIGHT) y2 = BOARD_DISPLAY_HEIGHT - 1;
  size_t total = (size_t)(x2 - x1 + 1) * (size_t)(y2 - y1 + 1);
  setAddrWindow((uint16_t)x1, (uint16_t)y1, (uint16_t)x2, (uint16_t)y2);

  if (alreadySwapped) {
    pushColors(pixels, total);
    return;
  }
  if (!s_dma_buf) {
    for (size_t i = 0; i < total; i++) {
      uint16_t be = swap16(pixels[i]);
      writeData((const uint8_t*)&be, 2);
    }
    return;
  }
  size_t sent = 0;
  while (sent < total) {
    size_t chunk = total - sent;
    if (chunk > DMA_BUF_PIXELS) chunk = DMA_BUF_PIXELS;
    for (size_t i = 0; i < chunk; i++) {
      s_dma_buf[i] = swap16(pixels[sent + i]);
    }
    pushColors(s_dma_buf, chunk);
    sent += chunk;
  }
}

void CydDisplay::fillScreen(uint16_t color) {
  fillRect(0, 0, BOARD_DISPLAY_WIDTH, BOARD_DISPLAY_HEIGHT, color);
}

void CydDisplay::drawPixel(int x, int y, uint16_t color) {
  if (x < 0 || y < 0 || x >= BOARD_DISPLAY_WIDTH || y >= BOARD_DISPLAY_HEIGHT) return;
  setAddrWindow((uint16_t)x, (uint16_t)y, (uint16_t)x, (uint16_t)y);
  uint16_t be = swap16(color);
  writeData((const uint8_t*)&be, 2);
}

void CydDisplay::fillRect(int x, int y, int w, int h, uint16_t color) {
  if (w <= 0 || h <= 0) return;
  if (x < 0) { w += x; x = 0; }
  if (y < 0) { h += y; y = 0; }
  if (x + w > BOARD_DISPLAY_WIDTH)  w = BOARD_DISPLAY_WIDTH - x;
  if (y + h > BOARD_DISPLAY_HEIGHT) h = BOARD_DISPLAY_HEIGHT - y;
  if (w <= 0 || h <= 0) return;

  setAddrWindow((uint16_t)x, (uint16_t)y, (uint16_t)(x + w - 1), (uint16_t)(y + h - 1));
  uint16_t be = swap16(color);

  if (!s_dma_buf) {
    for (int i = 0; i < w * h; i++) {
      writeData((const uint8_t*)&be, 2);
    }
    return;
  }

  size_t total = (size_t)w * (size_t)h;
  size_t fillCount = (total < DMA_BUF_PIXELS) ? total : DMA_BUF_PIXELS;
  for (size_t i = 0; i < fillCount; i++) {
    s_dma_buf[i] = be;
  }

  size_t remaining = total;
  while (remaining > 0) {
    size_t chunk = (remaining < DMA_BUF_PIXELS) ? remaining : DMA_BUF_PIXELS;
    pushColors(s_dma_buf, chunk);
    remaining -= chunk;
  }
}

void CydDisplay::drawFastHLine(int x, int y, int w, uint16_t color) {
  fillRect(x, y, w, 1, color);
}

void CydDisplay::drawFastVLine(int x, int y, int h, uint16_t color) {
  fillRect(x, y, 1, h, color);
}

void CydDisplay::drawRect(int x, int y, int w, int h, uint16_t color) {
  if (w <= 0 || h <= 0) return;
  drawFastHLine(x, y, w, color);
  drawFastHLine(x, y + h - 1, w, color);
  drawFastVLine(x, y, h, color);
  drawFastVLine(x + w - 1, y, h, color);
}

void CydDisplay::drawLine(int x0, int y0, int x1, int y1, uint16_t color) {
  int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
  int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
  int err = dx + dy;
  while (true) {
    drawPixel(x0, y0, color);
    if (x0 == x1 && y0 == y1) break;
    int e2 = 2 * err;
    if (e2 >= dy) { err += dy; x0 += sx; }
    if (e2 <= dx) { err += dx; y0 += sy; }
  }
}

void CydDisplay::drawCircleHelper(int x0, int y0, int r, uint8_t cornername, uint16_t color) {
  int f = 1 - r;
  int ddF_x = 1;
  int ddF_y = -2 * r;
  int x = 0;
  int y = r;
  while (x < y) {
    if (f >= 0) {
      y--;
      ddF_y += 2;
      f += ddF_y;
    }
    x++;
    ddF_x += 2;
    f += ddF_x;
    if (cornername & 0x4) {
      drawPixel(x0 + x, y0 + y, color);
      drawPixel(x0 + y, y0 + x, color);
    }
    if (cornername & 0x2) {
      drawPixel(x0 + x, y0 - y, color);
      drawPixel(x0 + y, y0 - x, color);
    }
    if (cornername & 0x8) {
      drawPixel(x0 - y, y0 + x, color);
      drawPixel(x0 - x, y0 + y, color);
    }
    if (cornername & 0x1) {
      drawPixel(x0 - y, y0 - x, color);
      drawPixel(x0 - x, y0 - y, color);
    }
  }
}

void CydDisplay::fillCircleHelper(int x0, int y0, int r, uint8_t corners, int delta, uint16_t color) {
  int f = 1 - r;
  int ddF_x = 1;
  int ddF_y = -2 * r;
  int x = 0;
  int y = r;
  int px = x;
  int py = y;
  delta++;
  while (x < y) {
    if (f >= 0) {
      y--;
      ddF_y += 2;
      f += ddF_y;
    }
    x++;
    ddF_x += 2;
    f += ddF_x;
    if (x < (y + 1)) {
      if (corners & 1) drawFastVLine(x0 + x, y0 - y, 2 * y + delta, color);
      if (corners & 2) drawFastVLine(x0 - x, y0 - y, 2 * y + delta, color);
    }
    if (y != py) {
      if (corners & 1) drawFastVLine(x0 + py, y0 - px, 2 * px + delta, color);
      if (corners & 2) drawFastVLine(x0 - py, y0 - px, 2 * px + delta, color);
      py = y;
    }
    px = x;
  }
}

void CydDisplay::drawRoundRect(int x, int y, int w, int h, int r, uint16_t color) {
  if (w <= 0 || h <= 0) return;
  if (r * 2 > w) r = w / 2;
  if (r * 2 > h) r = h / 2;
  drawFastHLine(x + r, y, w - 2 * r, color);
  drawFastHLine(x + r, y + h - 1, w - 2 * r, color);
  drawFastVLine(x, y + r, h - 2 * r, color);
  drawFastVLine(x + w - 1, y + r, h - 2 * r, color);
  drawCircleHelper(x + r, y + r, r, 1, color);
  drawCircleHelper(x + w - r - 1, y + r, r, 2, color);
  drawCircleHelper(x + w - r - 1, y + h - r - 1, r, 4, color);
  drawCircleHelper(x + r, y + h - r - 1, r, 8, color);
}

void CydDisplay::fillRoundRect(int x, int y, int w, int h, int r, uint16_t color) {
  if (w <= 0 || h <= 0) return;
  if (r * 2 > w) r = w / 2;
  if (r * 2 > h) r = h / 2;
  fillRect(x + r, y, w - 2 * r, h, color);
  fillCircleHelper(x + w - r - 1, y + r, r, 1, h - 2 * r - 1, color);
  fillCircleHelper(x + r, y + r, r, 2, h - 2 * r - 1, color);
}

void CydDisplay::fillCircle(int x0, int y0, int r, uint16_t color) {
  drawFastVLine(x0, y0 - r, 2 * r + 1, color);
  fillCircleHelper(x0, y0, r, 3, 0, color);
}

void CydDisplay::drawCircle(int x0, int y0, int r, uint16_t color) {
  int f = 1 - r;
  int ddF_x = 1;
  int ddF_y = -2 * r;
  int x = 0;
  int y = r;
  drawPixel(x0, y0 + r, color);
  drawPixel(x0, y0 - r, color);
  drawPixel(x0 + r, y0, color);
  drawPixel(x0 - r, y0, color);
  while (x < y) {
    if (f >= 0) {
      y--;
      ddF_y += 2;
      f += ddF_y;
    }
    x++;
    ddF_x += 2;
    f += ddF_x;
    drawPixel(x0 + x, y0 + y, color);
    drawPixel(x0 - x, y0 + y, color);
    drawPixel(x0 + x, y0 - y, color);
    drawPixel(x0 - x, y0 - y, color);
    drawPixel(x0 + y, y0 + x, color);
    drawPixel(x0 - y, y0 + x, color);
    drawPixel(x0 + y, y0 - x, color);
    drawPixel(x0 - y, y0 - x, color);
  }
}

void CydDisplay::fillEllipse(int x0, int y0, int rx, int ry, uint16_t color) {
  if (rx < 1 || ry < 1) return;
  int32_t rx2 = (int32_t)rx * rx;
  int32_t ry2 = (int32_t)ry * ry;
  for (int dy = -ry; dy <= ry; dy++) {
    int32_t num = rx2 * (ry2 - (int32_t)dy * dy);
    if (num < 0) continue;
    int dx = 0;
    while ((int32_t)(dx + 1) * (dx + 1) * ry2 <= num) dx++;
    drawFastHLine(x0 - dx, y0 + dy, 2 * dx + 1, color);
  }
}

void CydDisplay::drawEllipse(int x0, int y0, int rx, int ry, uint16_t color) {
  if (rx < 1 || ry < 1) return;
  int32_t rx2 = (int32_t)rx * rx;
  int32_t ry2 = (int32_t)ry * ry;
  int32_t twoRx2 = 2 * rx2;
  int32_t twoRy2 = 2 * ry2;
  int x = 0, y = ry;
  int32_t px = 0, py = twoRx2 * y;

  drawPixel(x0 + x, y0 + y, color);
  drawPixel(x0 - x, y0 + y, color);
  drawPixel(x0 + x, y0 - y, color);
  drawPixel(x0 - x, y0 - y, color);

  int32_t p = (int32_t)(ry2 - (rx2 * ry) + (rx2 / 4));
  while (px < py) {
    x++;
    px += twoRy2;
    if (p < 0) {
      p += ry2 + px;
    } else {
      y--;
      py -= twoRx2;
      p += ry2 + px - py;
    }
    drawPixel(x0 + x, y0 + y, color);
    drawPixel(x0 - x, y0 + y, color);
    drawPixel(x0 + x, y0 - y, color);
    drawPixel(x0 - x, y0 - y, color);
  }

  p = (int32_t)(ry2 * (x * x + x) + rx2 * (y - 1) * (y - 1) - rx2 * ry2);
  while (y > 0) {
    y--;
    py -= twoRx2;
    if (p > 0) {
      p += rx2 - py;
    } else {
      x++;
      px += twoRy2;
      p += rx2 - py + px;
    }
    drawPixel(x0 + x, y0 + y, color);
    drawPixel(x0 - x, y0 + y, color);
    drawPixel(x0 + x, y0 - y, color);
    drawPixel(x0 - x, y0 - y, color);
  }
}

void CydDisplay::drawTriangle(int x0, int y0, int x1, int y1, int x2, int y2, uint16_t color) {
  drawLine(x0, y0, x1, y1, color);
  drawLine(x1, y1, x2, y2, color);
  drawLine(x2, y2, x0, y0, color);
}

static inline void swapInt(int& a, int& b) { int t = a; a = b; b = t; }

void CydDisplay::fillTriangle(int x0, int y0, int x1, int y1, int x2, int y2, uint16_t color) {
  if (y0 > y1) { swapInt(y0, y1); swapInt(x0, x1); }
  if (y1 > y2) { swapInt(y2, y1); swapInt(x2, x1); }
  if (y0 > y1) { swapInt(y0, y1); swapInt(x0, x1); }
  if (y0 == y2) {
    int a = x0, b = x0;
    if (x1 < a) a = x1; else if (x1 > b) b = x1;
    if (x2 < a) a = x2; else if (x2 > b) b = x2;
    drawFastHLine(a, y0, b - a + 1, color);
    return;
  }
  int dx01 = x1 - x0, dy01 = y1 - y0;
  int dx02 = x2 - x0, dy02 = y2 - y0;
  int dx12 = x2 - x1, dy12 = y2 - y1;
  int32_t sa = 0, sb = 0;
  int last = (y1 == y2) ? y1 : y1 - 1;
  int y = y0;
  for (; y <= last; y++) {
    int a = x0 + (dy01 ? sa / dy01 : 0);
    int b = x0 + (dy02 ? sb / dy02 : 0);
    sa += dx01;
    sb += dx02;
    if (a > b) swapInt(a, b);
    drawFastHLine(a, y, b - a + 1, color);
  }
  sa = dx12 * (y - y1);
  sb = dx02 * (y - y0);
  for (; y <= y2; y++) {
    int a = x1 + (dy12 ? sa / dy12 : 0);
    int b = x0 + (dy02 ? sb / dy02 : 0);
    sa += dx12;
    sb += dx02;
    if (a > b) swapInt(a, b);
    drawFastHLine(a, y, b - a + 1, color);
  }
}

void CydDisplay::setTextDatum(uint8_t datum) { _datum = datum; }
void CydDisplay::setTextColor(uint16_t fg, uint16_t bg) { _fgColor = fg; _bgColor = bg; }
void CydDisplay::setTextSize(uint8_t size) { _textSize = (size < 1) ? 1 : size; }

void CydDisplay::drawChar(int x, int y, unsigned char c, uint8_t scale) {
  if (c < 0x20 || c > 0x7E) c = '?';
  const uint8_t* bitmap = CYD_FONT5X7[c - 0x20];
  for (int col = 0; col < 5; col++) {
    uint8_t bits = bitmap[col];
    for (int row = 0; row < 7; row++) {
      uint16_t colVal = (bits & (1 << row)) ? _fgColor : _bgColor;
      if (colVal != _bgColor || _bgColor != _fgColor) {
        if (scale == 1) {
          drawPixel(x + col, y + row, colVal);
        } else {
          fillRect(x + col * scale, y + row * scale, scale, scale, colVal);
        }
      }
    }
  }
  if (_bgColor != _fgColor) {
    fillRect(x + 5 * scale, y, scale, 7 * scale, _bgColor);
  }
}

// Transliterate UTF-8 strings (German umlauts / strip emojis) into clean 7-bit ASCII
static size_t sanitizeAscii(const char* src, char* dst, size_t maxLen) {
  size_t out = 0;
  const unsigned char* p = (const unsigned char*)src;
  while (p && *p && out + 2 < maxLen) {
    if (*p < 0x80) {
      dst[out++] = (char)*p++;
    } else if (*p == 0xC3 && p[1]) {
      unsigned char nxt = p[1];
      p += 2;
      switch (nxt) {
        case 0xA4: dst[out++] = 'a'; dst[out++] = 'e'; break; // ä
        case 0xB6: dst[out++] = 'o'; dst[out++] = 'e'; break; // ö
        case 0xBC: dst[out++] = 'u'; dst[out++] = 'e'; break; // ü
        case 0x84: dst[out++] = 'A'; dst[out++] = 'e'; break; // Ä
        case 0x96: dst[out++] = 'O'; dst[out++] = 'e'; break; // Ö
        case 0x9C: dst[out++] = 'U'; dst[out++] = 'e'; break; // Ü
        case 0x9F: dst[out++] = 's'; dst[out++] = 's'; break; // ß
        default: break;
      }
    } else if ((*p & 0xE0) == 0xC0 && p[1]) {
      p += 2;
    } else if ((*p & 0xF0) == 0xE0 && p[1] && p[2]) {
      p += 3; // Skip symbols like ⚡ ✨
    } else if ((*p & 0xF8) == 0xF0 && p[1] && p[2] && p[3]) {
      p += 4; // Skip emoji
    } else {
      p++;
    }
  }
  dst[out] = '\0';
  return out;
}

void CydDisplay::drawString(const char* text, int x, int y, uint8_t font) {
  if (!text) return;
  char ascii[96];
  size_t len = sanitizeAscii(text, ascii, sizeof(ascii));
  if (len == 0) return;

  uint8_t scale = _textSize;
  if (font >= 4 && scale == 1) scale = 2;

  int charW = 6 * scale;
  int charH = 8 * scale;
  int totalW = (int)len * charW;

  int startX = x;
  int startY = y;

  if (_datum == TC_DATUM || _datum == MC_DATUM) {
    startX = x - totalW / 2;
  } else if (_datum == TR_DATUM || _datum == MR_DATUM) {
    startX = x - totalW;
  }

  if (_datum == ML_DATUM || _datum == MC_DATUM || _datum == MR_DATUM) {
    startY = y - charH / 2;
  }

  for (size_t i = 0; i < len; i++) {
    drawChar(startX + (int)i * charW, startY, (unsigned char)ascii[i], scale);
  }
}
