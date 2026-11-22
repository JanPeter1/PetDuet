#ifndef CYD_DISPLAY_H
#define CYD_DISPLAY_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus

// Standard 16-bit RGB565 Colors matching TFT_eSPI constants
#ifndef TFT_BLACK
#define TFT_BLACK       0x0000
#define TFT_NAVY        0x000F
#define TFT_DARKGREEN   0x03E0
#define TFT_DARKCYAN    0x03EF
#define TFT_MAROON      0x7800
#define TFT_PURPLE      0x780F
#define TFT_OLIVE       0x7BE0
#define TFT_LIGHTGREY   0xD69A
#define TFT_DARKGREY    0x7BEF
#define TFT_BLUE        0x001F
#define TFT_GREEN       0x07E0
#define TFT_CYAN        0x07FF
#define TFT_RED         0xF800
#define TFT_MAGENTA     0xF81F
#define TFT_YELLOW      0xFFE0
#define TFT_WHITE       0xFFFF
#define TFT_ORANGE      0xFDA0
#define TFT_GREENYELLOW 0xB7E0
#define TFT_PINK        0xF81F
#define TFT_GOLD        0xFEA0
#define TFT_SILVER      0xC618
#endif

// Text Alignment Datums
#ifndef TL_DATUM
#define TL_DATUM 0
#define TC_DATUM 1
#define TR_DATUM 2
#define ML_DATUM 3
#define MC_DATUM 4
#define MR_DATUM 5
#endif

class CydDisplay {
public:
  CydDisplay() = default;

  bool init();
  void setBacklight(bool on);
  void setInversion(bool on);
  void flushRgb565Rect(int x1, int y1, int x2, int y2, const uint16_t* pixels, bool alreadySwapped = true);

  void fillScreen(uint16_t color);
  void drawPixel(int x, int y, uint16_t color);
  void drawFastHLine(int x, int y, int w, uint16_t color);
  void drawFastVLine(int x, int y, int h, uint16_t color);
  void drawLine(int x0, int y0, int x1, int y1, uint16_t color);

  void fillRect(int x, int y, int w, int h, uint16_t color);
  void drawRect(int x, int y, int w, int h, uint16_t color);
  void fillRoundRect(int x, int y, int w, int h, int r, uint16_t color);
  void drawRoundRect(int x, int y, int w, int h, int r, uint16_t color);

  void fillCircle(int x0, int y0, int r, uint16_t color);
  void drawCircle(int x0, int y0, int r, uint16_t color);
  void fillEllipse(int x0, int y0, int rx, int ry, uint16_t color);
  void drawEllipse(int x0, int y0, int rx, int ry, uint16_t color);

  void fillTriangle(int x0, int y0, int x1, int y1, int x2, int y2, uint16_t color);
  void drawTriangle(int x0, int y0, int x1, int y1, int x2, int y2, uint16_t color);

  void setTextDatum(uint8_t datum);
  void setTextColor(uint16_t fg, uint16_t bg);
  void setTextSize(uint8_t size);
  void drawString(const char* text, int x, int y, uint8_t font = 1);

  int width() const { return 320; }
  int height() const { return 240; }

private:
  uint8_t  _datum = TL_DATUM;
  uint16_t _fgColor = TFT_WHITE;
  uint16_t _bgColor = TFT_BLACK;
  uint8_t  _textSize = 1;

  void writeCmd(uint8_t cmd);
  void writeData(const uint8_t* data, size_t len);
  void setAddrWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1);
  void pushColors(const uint16_t* colors, size_t count);
  void drawCircleHelper(int x0, int y0, int r, uint8_t cornername, uint16_t color);
  void fillCircleHelper(int x0, int y0, int r, uint8_t corners, int delta, uint16_t color);
  void drawChar(int x, int y, unsigned char c, uint8_t scale);
};

#endif // __cplusplus
#endif // CYD_DISPLAY_H
