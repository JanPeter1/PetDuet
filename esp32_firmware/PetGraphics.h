/**
 * ============================================================================
 * PetGraphics.h - Pixel Art, Biome Palettes & 320x240 LCD Layout
 * ============================================================================
 */
#ifndef PET_GRAPHICS_H
#define PET_GRAPHICS_H

#include <Arduino.h>
#include <TFT_eSPI.h>

// 16-bit 565 Color Constants for 2.8" LCD
#define COLOR_DESERT_BG   0x2960 // Dark Amber
#define COLOR_DESERT_MAIN 0xFD40 // Golden Amber
#define COLOR_DESERT_ACC  0xD380 // Burnt Orange

#define COLOR_GRASS_BG    0x09E2 // Deep Forest Meadow
#define COLOR_GRASS_MAIN  0x2E69 // Vibrant Emerald
#define COLOR_GRASS_ACC   0x0C85 // Forest Pine

#define COLOR_WATER_BG    0x0271 // Deep Ocean Navy
#define COLOR_WATER_MAIN  0x1DCF // Cyan Aqua
#define COLOR_WATER_ACC   0x0357 // Deep Azure

#define COLOR_FOREST_BG   0x2141 // Earthy Dark Brown
#define COLOR_FOREST_MAIN 0x8BE3 // Rustic Oak / Wood
#define COLOR_FOREST_ACC  0x5223 // Deep Mahogany

#define COLOR_UI_PANEL    0x10A2 // Sleek Dark Slate
#define COLOR_UI_BORDER   0x4A69 // Slate Border
#define COLOR_GOLD        0xFEA0 // Level / Coins
#define COLOR_HEART_RED   0xF986 // Health / Treats
#define COLOR_MANA_PURPLE 0x99FD // Mana / Magic

// Additional colors for icons
#define TFT_SILVER        0xC618
#define TFT_PINK          0xF81F
#define TFT_DARKGREY      0x7BEF

struct BiomeTheme {
  const char* name;
  uint16_t bgColor;
  uint16_t petColor;
  uint16_t accentColor;
  uint16_t textColor;
};

inline BiomeTheme getBiomeTheme(const char* biome) {
  if (strcmp(biome, "desert") == 0) {
    return {"Desert", COLOR_DESERT_BG, COLOR_DESERT_MAIN, COLOR_DESERT_ACC, TFT_YELLOW};
  } else if (strcmp(biome, "water") == 0) {
    return {"Water", COLOR_WATER_BG, COLOR_WATER_MAIN, COLOR_WATER_ACC, TFT_CYAN};
  } else if (strcmp(biome, "forest") == 0) {
    return {"Forest", COLOR_FOREST_BG, COLOR_FOREST_MAIN, COLOR_FOREST_ACC, 0xFEA0};
  }
  // Default: grassland
  return {"Grassland", COLOR_GRASS_BG, COLOR_GRASS_MAIN, COLOR_GRASS_ACC, TFT_GREEN};
}

// ----------------------------------------------------------------------------
// Procedural Pixel Art Drawing Functions for ILI9341 (320x240)
// ----------------------------------------------------------------------------

/**
 * Draws an Egg on the display
 */
inline void drawPixelEgg(TFT_eSPI& tft, int cx, int cy, uint16_t shellColor, uint8_t warmth) {
  // Egg body
  tft.fillEllipse(cx, cy + 4, 18, 22, shellColor);
  tft.drawEllipse(cx, cy + 4, 18, 22, TFT_BLACK);

  // Shading highlight
  tft.fillEllipse(cx - 5, cy - 3, 5, 8, TFT_WHITE);

  // Cracks if warmed
  if (warmth > 40) {
    tft.drawLine(cx, cy - 8, cx - 4, cy - 2, TFT_BLACK);
    tft.drawLine(cx - 4, cy - 2, cx + 2, cy + 4, TFT_BLACK);
  }
  if (warmth > 75) {
    tft.drawLine(cx + 4, cy + 4, cx + 8, cy + 10, TFT_BLACK);
    tft.drawLine(cx + 8, cy + 10, cx + 3, cy + 14, TFT_BLACK);
  }
}

/**
 * Draws Pet Critter based on Biome, Phase & Mood
 */
inline void drawPixelPet(TFT_eSPI& tft, int cx, int cy, const char* biome, const char* phase, bool isFriend = false) {
  BiomeTheme theme = getBiomeTheme(biome);
  bool isElder = strcmp(phase, "Elder") == 0;
  bool isAdult = strcmp(phase, "Adult") == 0 || isElder;
  int radius = isAdult ? 20 : (isElder ? 22 : 16);

  // Ground shadow
  tft.fillEllipse(cx, cy + radius + 4, radius + 4, 4, TFT_DARKGREY);

  // Ears / Biome Details
  if (strcmp(biome, "desert") == 0) {
    // Reptilian crest / spiky frills
    tft.fillTriangle(cx - 8, cy - radius, cx, cy - radius - 10, cx + 8, cy - radius, theme.accentColor);
    tft.drawTriangle(cx - 8, cy - radius, cx, cy - radius - 10, cx + 8, cy - radius, TFT_BLACK);
  } else if (strcmp(biome, "grassland") == 0) {
    // Bunny / Leaf ears
    tft.fillRoundRect(cx - 10, cy - radius - 12, 6, 14, 3, theme.petColor);
    tft.fillRoundRect(cx + 4, cy - radius - 12, 6, 14, 3, theme.petColor);
    tft.drawRoundRect(cx - 10, cy - radius - 12, 6, 14, 3, TFT_BLACK);
    tft.drawRoundRect(cx + 4, cy - radius - 12, 6, 14, 3, TFT_BLACK);
  } else if (strcmp(biome, "water") == 0) {
    // Axolotl frills
    tft.fillCircle(cx - radius - 4, cy - 4, 5, theme.accentColor);
    tft.fillCircle(cx + radius + 4, cy - 4, 5, theme.accentColor);
  } else if (strcmp(biome, "forest") == 0) {
    // Bear ears
    tft.fillCircle(cx - 12, cy - radius + 2, 6, theme.accentColor);
    tft.fillCircle(cx + 12, cy - radius + 2, 6, theme.accentColor);
  }

  // Main Body
  tft.fillCircle(cx, cy, radius, theme.petColor);
  tft.drawCircle(cx, cy, radius, TFT_BLACK);

  // Belly Patch
  tft.fillEllipse(cx, cy + 3, radius - 6, radius - 8, 0xFFE0);

  // Eyes
  tft.fillCircle(cx - 6, cy - 3, 3, TFT_BLACK);
  tft.fillCircle(cx + 6, cy - 3, 3, TFT_BLACK);
  tft.drawPixel(cx - 7, cy - 4, TFT_WHITE); // Eye catchlight
  tft.drawPixel(cx + 5, cy - 4, TFT_WHITE);

  // Cheeks
  tft.fillCircle(cx - 10, cy + 3, 2, COLOR_HEART_RED);
  tft.fillCircle(cx + 10, cy + 3, 2, COLOR_HEART_RED);

  // Smile
  tft.drawLine(cx - 3, cy + 4, cx, cy + 6, TFT_BLACK);
  tft.drawLine(cx, cy + 6, cx + 3, cy + 4, TFT_BLACK);

  // Accessory: Sunglasses on friend, Flower on local
  if (isFriend) {
    // Pixel Shades
    tft.fillRect(cx - 10, cy - 5, 20, 5, TFT_BLACK);
    tft.drawFastHLine(cx - 8, cy - 4, 5, TFT_CYAN);
    tft.drawFastHLine(cx + 3, cy - 4, 5, TFT_CYAN);
  } else {
    // Blossom flower
    tft.fillCircle(cx + 10, cy - radius + 3, 3, TFT_PINK);
    tft.fillCircle(cx + 10, cy - radius + 3, 1, TFT_YELLOW);
  }

  // Elder Sage Aura
  if (isElder) {
    tft.drawCircle(cx, cy, radius + 4, COLOR_GOLD);
    tft.drawLine(cx - 5, cy + 7, cx - 12, cy + 11, TFT_WHITE); // Whiskers
    tft.drawLine(cx + 5, cy + 7, cx + 12, cy + 11, TFT_WHITE);
  }
}

/**
 * Draws a standardized touch button with borders and large icon (no text)
 */
inline void drawTouchButton(TFT_eSPI& tft, int x, int y, int w, int h, const char* label, uint16_t fillCol, uint16_t textCol, uint8_t iconType = 0) {
  tft.fillRoundRect(x, y, w, h, 4, fillCol);
  tft.drawRoundRect(x, y, w, h, 4, TFT_WHITE);

  // Draw large centered icon (no text)
  int iconX = x + (w / 2);
  int iconY = y + (h / 2);

  if (iconType > 0) {
    switch (iconType) {
      case 1: { // Feed - large food bowl
        tft.fillRect(iconX - 8, iconY + 2, 16, 6, TFT_ORANGE);
        tft.fillRect(iconX - 6, iconY - 4, 12, 6, TFT_WHITE);
        tft.drawRect(iconX - 6, iconY - 4, 12, 6, TFT_BLACK);
        break;
      }
      case 2: { // Play - large game controller
        tft.fillCircle(iconX - 6, iconY - 4, 5, TFT_BLUE);
        tft.fillCircle(iconX + 6, iconY - 4, 5, TFT_BLUE);
        tft.fillCircle(iconX, iconY + 4, 5, TFT_BLUE);
        tft.fillCircle(iconX, iconY - 6, 5, TFT_BLUE);
        tft.fillCircle(iconX - 4, iconY - 2, 2, TFT_WHITE);
        tft.fillCircle(iconX + 4, iconY - 2, 2, TFT_WHITE);
        tft.fillCircle(iconX, iconY + 2, 2, TFT_WHITE);
        tft.fillCircle(iconX, iconY - 4, 2, TFT_WHITE);
        break;
      }
      case 3: { // Bath - large water drops
        tft.fillTriangle(iconX, iconY - 6, iconX - 4, iconY, iconX + 4, iconY, TFT_CYAN);
        tft.fillTriangle(iconX - 6, iconY + 2, iconX - 10, iconY + 8, iconX - 2, iconY + 8, TFT_CYAN);
        tft.fillTriangle(iconX + 6, iconY + 2, iconX + 2, iconY + 8, iconX + 10, iconY + 8, TFT_CYAN);
        break;
      }
      case 4: { // Magic - large sparkle
        tft.drawLine(iconX, iconY - 8, iconX, iconY + 8, TFT_YELLOW);
        tft.drawLine(iconX - 8, iconY, iconX + 8, iconY, TFT_YELLOW);
        tft.drawLine(iconX - 6, iconY - 6, iconX + 6, iconY + 6, TFT_YELLOW);
        tft.drawLine(iconX - 6, iconY + 6, iconX + 6, iconY - 6, TFT_YELLOW);
        tft.drawPixel(iconX - 3, iconY - 3, TFT_WHITE);
        tft.drawPixel(iconX + 3, iconY + 3, TFT_WHITE);
        tft.drawPixel(iconX - 3, iconY + 3, TFT_WHITE);
        tft.drawPixel(iconX + 3, iconY - 3, TFT_WHITE);
        break;
      }
      case 5: { // Save - large floppy/disk
        tft.fillRect(iconX - 8, iconY - 6, 16, 12, TFT_DARKGREY);
        tft.drawRect(iconX - 8, iconY - 6, 16, 12, TFT_BLACK);
        tft.fillRect(iconX - 4, iconY - 6, 8, 4, TFT_WHITE);
        tft.fillRect(iconX - 2, iconY - 4, 4, 2, TFT_SILVER);
        break;
      }
      case 6: { // Treat - large donut
        tft.drawCircle(iconX, iconY, 8, TFT_ORANGE);
        tft.drawCircle(iconX, iconY, 3, TFT_ORANGE);
        tft.fillCircle(iconX, iconY, 2, TFT_ORANGE);
        break;
      }
      case 7: { // Chat - large speech bubble
        tft.fillTriangle(iconX - 6, iconY + 4, iconX - 2, iconY + 4, iconX - 2, iconY + 8, TFT_BLUE);
        tft.fillRoundRect(iconX - 10, iconY - 8, 18, 12, 4, TFT_BLUE);
        tft.drawRoundRect(iconX - 10, iconY - 8, 18, 12, 4, TFT_WHITE);
        tft.fillRect(iconX - 6, iconY - 4, 4, 4, TFT_WHITE);
        tft.fillRect(iconX + 2, iconY - 4, 4, 4, TFT_WHITE);
        tft.fillRect(iconX - 6, iconY + 2, 12, 2, TFT_WHITE);
        break;
      }
      case 8: { // Magic Plus - large dual sparkles
        tft.drawLine(iconX - 8, iconY - 6, iconX - 8, iconY + 6, TFT_PINK);
        tft.drawLine(iconX - 11, iconY, iconX - 5, iconY, TFT_PINK);
        tft.drawLine(iconX - 7, iconY - 5, iconX - 9, iconY - 3, TFT_PINK);
        tft.drawLine(iconX - 7, iconY + 5, iconX - 9, iconY + 3, TFT_PINK);
        tft.drawLine(iconX + 4, iconY - 6, iconX + 4, iconY + 6, TFT_PINK);
        tft.drawLine(iconX + 1, iconY, iconX + 7, iconY, TFT_PINK);
        tft.drawLine(iconX + 3, iconY - 5, iconX + 5, iconY - 3, TFT_PINK);
        tft.drawLine(iconX + 3, iconY + 5, iconX + 5, iconY + 3, TFT_PINK);
        break;
      }
      case 9: { // Quest - large sword
        tft.drawLine(iconX, iconY - 10, iconX, iconY + 6, TFT_SILVER);
        tft.drawLine(iconX - 5, iconY + 4, iconX + 5, iconY + 4, TFT_SILVER);
        tft.fillTriangle(iconX, iconY - 12, iconX - 3, iconY - 10, iconX + 3, iconY - 10, TFT_SILVER);
        break;
      }
      case 10: { // Trade - large arrows
        tft.fillTriangle(iconX - 10, iconY - 4, iconX - 2, iconY, iconX - 10, iconY + 4, TFT_GOLD);
        tft.fillTriangle(iconX + 10, iconY - 4, iconX + 2, iconY, iconX + 10, iconY + 4, TFT_GOLD);
        tft.drawLine(iconX - 10, iconY, iconX + 10, iconY, TFT_GOLD);
        break;
      }
    }
  }
}

/**
 * Draws a status bar with border
 */
inline void drawMiniBar(TFT_eSPI& tft, int x, int y, int w, int h, uint8_t value, uint16_t barCol) {
  tft.drawRect(x, y, w, h, TFT_WHITE);
  tft.fillRect(x + 1, y + 1, w - 2, h - 2, TFT_BLACK);
  int fillW = map(value, 0, 100, 0, w - 2);
  if (fillW > 0) {
    tft.fillRect(x + 1, y + 1, fillW, h - 2, barCol);
  }
}

#endif // PET_GRAPHICS_H
