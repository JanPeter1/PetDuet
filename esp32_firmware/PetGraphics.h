/**
 * ============================================================================
 * PetGraphics.h - Pixel Art, Biome Palettes & 320x240 LCD Layout
 * ============================================================================
 * High-contrast 16x16 shaded pixel-art sprites (0 heap allocation) matching
 * the Web Simulator aesthetic for ILI9341 320x240 CYD displays.
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

// Additional colors for icons & sprites
#define TFT_SILVER        0xC618
#define TFT_PINK          0xF81F
#define TFT_DARKGREY      0x7BEF

struct BiomeTheme {
  const char* name;
  uint16_t bgColor;
  uint16_t petColor;
  uint16_t accentColor;
  uint16_t bellyColor;
  uint16_t textColor;
};

inline BiomeTheme getBiomeTheme(const char* biome) {
  if (strcmp(biome, "desert") == 0) {
    return {"Desert", COLOR_DESERT_BG, COLOR_DESERT_MAIN, COLOR_DESERT_ACC, 0xFF77, TFT_YELLOW};
  } else if (strcmp(biome, "water") == 0) {
    return {"Water", COLOR_WATER_BG, COLOR_WATER_MAIN, COLOR_WATER_ACC, 0xCFFF, TFT_CYAN};
  } else if (strcmp(biome, "forest") == 0) {
    return {"Forest", COLOR_FOREST_BG, COLOR_FOREST_MAIN, COLOR_FOREST_ACC, 0xEF5D, 0xFEA0};
  }
  // Default: grassland
  return {"Grassland", COLOR_GRASS_BG, COLOR_GRASS_MAIN, COLOR_GRASS_ACC, 0xCFF9, TFT_GREEN};
}

// ----------------------------------------------------------------------------
// 16x16 Character-Mapped RGB565 Pixel Sprite Palette & Renderer
// ----------------------------------------------------------------------------
inline uint16_t mapSpriteColor(char c) {
  switch (c) {
    case 'k': return 0x10A4; // Rich Dark Slate Outline (#101420)
    case 'w': return 0xFFFF; // Pure White Highlight
    case 's': return 0xCE79; // Light Silver / Bone Shading
    case 'g': return 0x7BEF; // Slate Steel Grey
    case 'r': return 0xF842; // Vibrant Apple / Ruby Red
    case 'R': return 0xB002; // Deep Crimson Shadow
    case 'm': return 0xD244; // Roasted Meat Terracotta
    case 'M': return 0x8922; // Deep Meat Shadow
    case 'p': return 0xFC98; // Strawberry Frosting Pink
    case 'P': return 0xF81F; // Magenta Berry
    case 'o': return 0xFD20; // Warm Orange
    case 'O': return 0x9B00; // Crust / Wood Brown
    case 'y': return 0xFFE0; // Bright Yellow
    case 'Y': return 0xFEA0; // Rich Gold
    case 'l': return 0x9FE0; // Tennis Ball Chartreuse Lime
    case 'L': return 0x6E20; // Olive Lime Shadow
    case 'e': return 0x2E69; // Emerald Green
    case 'E': return 0x0C85; // Deep Forest Green
    case 'c': return 0x9FFF; // Pale Ice Cyan
    case 'C': return 0x067F; // Bright Aqua Cyan
    case 'b': return 0x3C3F; // Sky Blue
    case 'B': return 0x0255; // Deep Azure Blue
    case 'u': return 0xA4DF; // Soft Lavender Glow
    case 't': return 0xFECB; // Pastry / Donut Cream Dough
    default:  return 0x0000;
  }
}

inline void drawPixelSprite16(TFT_eSPI& tft, int topLeftX, int topLeftY, const char* const rows[16]) {
  for (int py = 0; py < 16; py++) {
    const char* row = rows[py];
    for (int px = 0; row[px] != '\0' && px < 16; px++) {
      if (row[px] != '.') {
        tft.drawPixel(topLeftX + px, topLeftY + py, mapSpriteColor(row[px]));
      }
    }
  }
}

// ----------------------------------------------------------------------------
// 16x16 Button Pixel-Art Sprites (Matching Simulator Emoji & Aesthetic)
// ----------------------------------------------------------------------------
static const char* const SPR_BTN_FEED[16] = {
  "..........kkk...",
  ".........kwwsk..",
  ".......kkkwwskk.",
  "......kmmkkkkwk.",
  ".....kmmmmwmmkk.",
  "....kmmmmwmmmk..",
  "...kmmmmmmmmmMk.",
  "..kmmmmmmmmmMk..",
  ".kkmmmmmmmmMk...",
  "kwwkmmmmmmMk....",
  "kwsskmmmmMk.....",
  ".kkwwkkkkk......",
  "..kssk..........",
  "...kk...........",
  "................",
  "................"
};

static const char* const SPR_BTN_PLAY[16] = {
  ".....kkkkkk.....",
  "...kkllllllkk...",
  "..klwwlllllllk..",
  ".klwllwwlllllLk.",
  ".klllwllwllllLk.",
  "kllllwlllwllllLk",
  "klllwlwllwllllLk",
  "kllwllwllwlllLLk",
  "kllwllwllwlllLLk",
  "kllwlllwllwllLLk",
  ".kllwlllwllwLLk.",
  ".klllwwllwwLLLk.",
  "..kllllllLLLLk..",
  "...kkLLLLLLkk...",
  ".....kkkkkk.....",
  "................"
};

static const char* const SPR_BTN_BATH[16] = {
  "...kkkk.........",
  "..kcccck........",
  ".kcwwccck.......",
  ".kcwcCCCk..kkk..",
  ".kccCCCCk.kccck.",
  "..kCCCCCkkcwCCk.",
  "...kkkk..kcCCCk.",
  ".....kkkk.kCCk..",
  "....kcccck.kk...",
  "...kcwwccck.....",
  "...kcwCCCCk.....",
  "...kccCCCCk.....",
  "....kCCCCk......",
  ".....kkkk.......",
  "................",
  "................"
};

static const char* const SPR_BTN_MAGIC[16] = {
  "......y.........",
  "......y.........",
  ".....kyk....u...",
  "....kywyk..uwu..",
  "....kywyk...u...",
  "..kkywwwykk.....",
  "kyywwwwwwwyyk...",
  "..kkywwwykk.....",
  "....kywyk...y...",
  "....kywyk..kyk..",
  ".....kyk..kywyk.",
  "......y....kyk..",
  "....u.......y...",
  "...uwu..........",
  "....u...........",
  "................"
};

static const char* const SPR_BTN_STATS[16] = {
  "..kkkkkkkkkkkk..",
  ".kwwwwwwwwwwwwk.",
  ".kwwbbbbbbwwwwk.",
  ".kwwwwwwwkkkwwk.",
  ".kwwkkkwwkYkwwk.",
  ".kwwkCkwwkYkwwk.",
  ".kwwkCkkkkYkwwk.",
  ".kwwkCkEkYkkwwk.",
  ".kwwkCkEkYkwwwk.",
  ".kwwkCkEkYkwwwk.",
  ".kwwkCkEkYkwwwk.",
  ".kwwkkkkkkkkwwk.",
  ".kssssssssssssk.",
  "..kkkkkkkkkkkk..",
  "................",
  "................"
};

static const char* const SPR_BTN_TREAT[16] = {
  "....kkkkkkkk....",
  "..kkppppppppkk..",
  ".kpwpppypppcpsk.",
  ".kppppppppppppk.",
  "kppwppkkkkppwppk",
  "kppppk....kppppk",
  "kppypk....kppcpk",
  "kpppppk..kpppppk",
  "kootppkkkkppptok",
  ".koottttttttook.",
  ".kkoooooooooook.",
  "..kkOOOOOOOOkk..",
  "....kkkkkkkk....",
  "................",
  "................",
  "................"
};

static const char* const SPR_BTN_CHAT[16] = {
  "..kkkkkkkkkkkk..",
  ".kwwwwwwwwwwwwk.",
  "kwwwwwwwwwwwwwwk",
  "kwwkkwwkkwwkkwwk",
  "kwwkkwwkkwwkkwwk",
  "kwwwwwwwwwwwwwwk",
  "kssssssssssssssk",
  ".kbbbbbbbbbbbbk.",
  "..kkkbbkkkkkkk..",
  "...kbbk.........",
  "..kbbk..........",
  "..kkk...........",
  "................",
  "................",
  "................",
  "................"
};

static const char* const SPR_BTN_MGC_PLUS[16] = {
  ".......kk.......",
  "......kyyk......",
  "..u...kyyk...u..",
  ".....kkyyk......",
  ".kkkkywwwykkkk..",
  "kyyyyywwwwyyyyk.",
  ".kyyywwwwwyyyk..",
  "..kyyyyyyyyyk...",
  "...kyyYyyYyk....",
  "..kyYYk..kYYyk..",
  ".kyYk......kYYk.",
  ".kkk........kkk.",
  "...c........c...",
  "................",
  "................",
  "................"
};

static const char* const SPR_BTN_QUEST[16] = {
  ".kk..........kk.",
  "kwwk........kwwk",
  "kwsgk......kgswk",
  ".kwsgk....kgswk.",
  "..kwsgk..kgswk..",
  "...kwsgkkgswk...",
  "....kwwsswwk....",
  "...kksssssskk...",
  "..kYkwsssswkYk..",
  ".kYYkwwkkwwkYYk.",
  "..kYkOkkkkOkYk..",
  ".kOkkk....kkkOk.",
  "krOk........kOrk",
  ".kk..........kk.",
  "................",
  "................"
};

static const char* const SPR_BTN_TRADE[16] = {
  "...kkk....kkk...",
  "..kYYyk..kyYYk..",
  "..kYYkkkkkkYYk..",
  ".kkkkkkkkkkkkkk.",
  ".krrrrkYYkrrrrk.",
  ".krrrrkYYkrrrrk.",
  ".kkkkkkkkkkkkkk.",
  "..kRRRkYYkRRRk..",
  "..kRRRkYYkRRRk..",
  "..kRRRkYYkRRRk..",
  "..kRRRkYYkRRRk..",
  "..kRRRkYYkRRRk..",
  "..kkkkkkkkkkkk..",
  "................",
  "................",
  "................"
};

// ----------------------------------------------------------------------------
// 16x16 Food Item Pixel-Art Sprites (Feed Flyout Bar)
// ----------------------------------------------------------------------------
static const char* const SPR_FOOD_APPLE[16] = {
  ".......kOOkkk...",
  ".......kOkeeek..",
  ".....kkkOkeeek..",
  "...kkrrrrrkkkk..",
  "..krwwrrrrrrrk..",
  ".krwwrrrrrrrrRk.",
  ".krrrrrrrrrrrRk.",
  ".krrrrrrrrrrrRk.",
  ".krrrrrrrrrrRRk.",
  "..krrrrrrrrRRk..",
  "..kRRRRrRRRRRk..",
  "...kkRRRRRRkk...",
  ".....kkkkkk.....",
  "................",
  "................",
  "................"
};

static const char* const SPR_FOOD_SNACK[16] = {
  "...kkkk..kkkk...",
  "..koooOkkoooOk..",
  ".kowwoOkkOowwok.",
  ".kooOkwoowkOook.",
  ".kooOkoooOkOook.",
  "..kookoOookook..",
  "...kkoOkkOokkk..",
  "..koOkoooOkOok..",
  ".koOwkOkkOkwOok.",
  ".koOookkkkOoOok.",
  "..kOOooooooOOk..",
  "...kkkkkkkkkk...",
  "................",
  "................",
  "................",
  "................"
};

static const char* const SPR_FOOD_SALAD[16] = {
  "....kkeeeekk....",
  "..kkeeelleeeek..",
  ".keellelrrelleek",
  ".kelllyeeeelrrek",
  ".keeeeeeeeeeeeek",
  "kwwwwwwwwwwwwwwk",
  "kssssssssssssssk",
  ".kssssssssssssk.",
  "..kggggggggggk..",
  "...kkkkkkkkkk...",
  "................",
  "................",
  "................",
  "................",
  "................",
  "................"
};

static const char* const SPR_FOOD_PIE[16] = {
  "................",
  "....kkkkkkkk....",
  "..kktPPpPPpPtk..",
  ".kttPPPrpPPprtk.",
  "kttppppppppppttk",
  "kttttttttttttttk",
  "kttttttttttttttk",
  ".koottttttttook.",
  "..kOOOOOOOOOOk..",
  "...kkkkkkkkkk...",
  "................",
  "................",
  "................",
  "................",
  "................",
  "................"
};

static const char* const SPR_FOOD_JERKY[16] = {
  "................",
  "...kkkkkkkkkk...",
  "..kmmwwmmmmmwk..",
  ".kmmmmmmwmmmwmk.",
  ".kmmmwwmmmmmmMk.",
  ".kmmmmmmmmwMMMk.",
  ".kOmmmmmmmmMMOk.",
  "..kOOOOOOOOOOk..",
  "...kkkkkkkkkk...",
  "................",
  "................",
  "................",
  "................",
  "................",
  "................",
  "................"
};

static const char* const SPR_FOOD_STEW[16] = {
  "...w........w...",
  "....w......w....",
  "...w........w...",
  "..kkkkkkkkkkkk..",
  ".kooooyrrroooOk.",
  "koooreoooerooOOk",
  "kggggggggggggggk",
  ".kssssssssssssk.",
  ".kssssssssssssk.",
  "..kggggggggggk..",
  "...kkkkkkkkkk...",
  "................",
  "................",
  "................",
  "................",
  "................"
};

// ----------------------------------------------------------------------------
// Procedural Pixel Art Drawing Functions for ILI9341 (320x240)
// ----------------------------------------------------------------------------

/**
 * Draws an Egg on the display with biome markings & warm highlights
 */
inline void drawPixelEgg(TFT_eSPI& tft, int cx, int cy, uint16_t shellColor, uint8_t warmth, const char* biome = "grassland") {
  // Ground shadow
  tft.fillEllipse(cx, cy + 27, 17, 4, 0x0842);

  // Egg body
  tft.fillEllipse(cx, cy + 4, 18, 22, shellColor);
  tft.drawEllipse(cx, cy + 4, 18, 22, 0x10A4);
  tft.drawEllipse(cx, cy + 4, 17, 21, 0x10A4);

  // Biome Egg Spots
  if (strcmp(biome, "desert") == 0) {
    tft.fillCircle(cx - 5, cy - 3, 3, 0xFF77);
    tft.fillCircle(cx + 6, cy + 8, 4, 0xFF77);
  } else if (strcmp(biome, "water") == 0) {
    tft.drawFastHLine(cx - 10, cy - 2, 20, 0xCFFF);
    tft.drawFastHLine(cx - 12, cy + 8, 24, 0xCFFF);
  } else if (strcmp(biome, "forest") == 0) {
    tft.drawCircle(cx, cy + 4, 6, 0x4A22);
    tft.drawCircle(cx, cy + 4, 11, 0x4A22);
  } else {
    tft.fillCircle(cx - 4, cy - 4, 3, 0x3F30);
    tft.fillCircle(cx + 7, cy + 7, 3, 0x3F30);
  }

  // Glossy specular highlight
  tft.fillEllipse(cx - 7, cy - 6, 4, 7, TFT_WHITE);

  // Cracks if warmed
  if (warmth > 40) {
    tft.drawLine(cx, cy - 8, cx - 4, cy - 2, 0x10A4);
    tft.drawLine(cx - 4, cy - 2, cx + 2, cy + 4, 0x10A4);
  }
  if (warmth > 75) {
    tft.drawLine(cx + 4, cy + 4, cx + 8, cy + 10, 0x10A4);
    tft.drawLine(cx + 8, cy + 10, cx + 3, cy + 14, 0x10A4);
  }
}

/**
 * Draws Pet Critter based on Biome, Phase & Mood
 */
inline void drawPixelPet(TFT_eSPI& tft, int cx, int cy, const char* biome, const char* phase, bool isFriend = false) {
  BiomeTheme theme = getBiomeTheme(biome);
  bool isElder = strcmp(phase, "Elder") == 0;
  bool isAdult = strcmp(phase, "Adult") == 0 || isElder;
  int radius = isAdult ? 20 : (isElder ? 22 : 17);

  // Ground shadow
  tft.fillEllipse(cx, cy + radius + 5, radius + 3, 4, 0x0842);

  // Ears / Biome Details
  if (strcmp(biome, "desert") == 0) {
    tft.fillTriangle(cx - 8, cy - radius + 1, cx, cy - radius - 11, cx + 8, cy - radius + 1, theme.accentColor);
    tft.drawTriangle(cx - 8, cy - radius + 1, cx, cy - radius - 11, cx + 8, cy - radius + 1, 0x10A4);
  } else if (strcmp(biome, "grassland") == 0) {
    tft.fillRoundRect(cx - 11, cy - radius - 12, 7, 15, 3, theme.petColor);
    tft.fillRoundRect(cx + 4, cy - radius - 12, 7, 15, 3, theme.petColor);
    tft.drawRoundRect(cx - 11, cy - radius - 12, 7, 15, 3, 0x10A4);
    tft.drawRoundRect(cx + 4, cy - radius - 12, 7, 15, 3, 0x10A4);
    tft.fillRect(cx - 9, cy - radius - 8, 3, 6, COLOR_HEART_RED);
    tft.fillRect(cx + 6, cy - radius - 8, 3, 6, COLOR_HEART_RED);
  } else if (strcmp(biome, "water") == 0) {
    tft.fillCircle(cx - radius - 3, cy - 5, 5, theme.accentColor);
    tft.fillCircle(cx + radius + 3, cy - 5, 5, theme.accentColor);
    tft.drawCircle(cx - radius - 3, cy - 5, 5, 0x10A4);
    tft.drawCircle(cx + radius + 3, cy - 5, 5, 0x10A4);
  } else if (strcmp(biome, "forest") == 0) {
    tft.fillCircle(cx - 12, cy - radius + 2, 6, theme.accentColor);
    tft.fillCircle(cx + 12, cy - radius + 2, 6, theme.accentColor);
    tft.drawCircle(cx - 12, cy - radius + 2, 6, 0x10A4);
    tft.drawCircle(cx + 12, cy - radius + 2, 6, 0x10A4);
    tft.fillCircle(cx - 12, cy - radius + 2, 3, theme.bellyColor);
    tft.fillCircle(cx + 12, cy - radius + 2, 3, theme.bellyColor);
  }

  // Little Paws
  tft.fillEllipse(cx - 10, cy + radius, 6, 3, theme.accentColor);
  tft.fillEllipse(cx + 10, cy + radius, 6, 3, theme.accentColor);
  tft.drawEllipse(cx - 10, cy + radius, 6, 3, 0x10A4);
  tft.drawEllipse(cx + 10, cy + radius, 6, 3, 0x10A4);

  // Main Body
  tft.fillCircle(cx, cy, radius, theme.petColor);
  tft.drawCircle(cx, cy, radius, 0x10A4);

  // Belly Patch
  tft.fillEllipse(cx, cy + 4, radius - 6, radius - 8, theme.bellyColor);

  // Big Retro Eyes
  tft.fillCircle(cx - 6, cy - 4, 3, 0x10A4);
  tft.fillCircle(cx + 6, cy - 4, 3, 0x10A4);
  tft.fillRect(cx - 7, cy - 5, 2, 2, TFT_WHITE);
  tft.fillRect(cx + 5, cy - 5, 2, 2, TFT_WHITE);

  // Cheeks
  tft.fillCircle(cx - 10, cy + 2, 2, COLOR_HEART_RED);
  tft.fillCircle(cx + 10, cy + 2, 2, COLOR_HEART_RED);

  // Cute Smile
  tft.drawLine(cx - 3, cy + 3, cx, cy + 5, 0x10A4);
  tft.drawLine(cx, cy + 5, cx + 3, cy + 3, 0x10A4);

  // Accessory: Cool Shades on friend, Blossom flower on local
  if (isFriend) {
    tft.fillRect(cx - 11, cy - 6, 10, 5, 0x0841);
    tft.fillRect(cx + 1, cy - 6, 10, 5, 0x0841);
    tft.drawFastHLine(cx - 1, cy - 4, 2, 0x0841);
    tft.drawFastHLine(cx - 9, cy - 5, 5, TFT_CYAN);
    tft.drawFastHLine(cx + 3, cy - 5, 5, TFT_CYAN);
  } else {
    tft.fillCircle(cx + 9, cy - radius + 2, 3, TFT_WHITE);
    tft.fillCircle(cx + 9, cy - radius + 2, 1, TFT_YELLOW);
  }

  // Elder Sage Aura
  if (isElder) {
    tft.drawCircle(cx, cy, radius + 4, COLOR_GOLD);
    tft.drawLine(cx - 5, cy + 6, cx - 12, cy + 10, TFT_WHITE);
    tft.drawLine(cx + 5, cy + 6, cx + 12, cy + 10, TFT_WHITE);
  }
}

/**
 * Draws a modern dark-tinted CYD touch button with 16x16 pixel-art icon + crisp label
 */
inline void drawTouchButton(
  TFT_eSPI& tft,
  int x, int y, int w, int h,
  const char* label,
  uint16_t fillCol,
  uint16_t borderCol,
  uint8_t iconType = 0,
  uint16_t labelCol = TFT_WHITE
) {
  tft.fillRoundRect(x, y, w, h, 4, fillCol);
  tft.drawRoundRect(x, y, w, h, 4, borderCol);

  // Subtle top glossy rim inside card
  tft.drawFastHLine(x + 3, y + 1, w - 6, borderCol);

  // Draw crisp 16x16 pixel-art icon centered in upper section
  int sprX = x + (w - 16) / 2;
  int sprY = y + (label && strlen(label) > 0 ? 5 : (h - 16) / 2);

  switch (iconType) {
    case 1:  drawPixelSprite16(tft, sprX, sprY, SPR_BTN_FEED);     break;
    case 2:  drawPixelSprite16(tft, sprX, sprY, SPR_BTN_PLAY);     break;
    case 3:  drawPixelSprite16(tft, sprX, sprY, SPR_BTN_BATH);     break;
    case 4:  drawPixelSprite16(tft, sprX, sprY, SPR_BTN_MAGIC);    break;
    case 5:  drawPixelSprite16(tft, sprX, sprY, SPR_BTN_STATS);    break;
    case 6:  drawPixelSprite16(tft, sprX, sprY, SPR_BTN_TREAT);    break;
    case 7:  drawPixelSprite16(tft, sprX, sprY, SPR_BTN_CHAT);     break;
    case 8:  drawPixelSprite16(tft, sprX, sprY, SPR_BTN_MGC_PLUS); break;
    case 9:  drawPixelSprite16(tft, sprX, sprY, SPR_BTN_QUEST);    break;
    case 10: drawPixelSprite16(tft, sprX, sprY, SPR_BTN_TRADE);    break;
    default: break;
  }

  // Compact label below icon
  if (label && strlen(label) > 0) {
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(labelCol, fillCol);
    tft.drawString(label, x + w / 2, y + h - 8, 1);
  }
}

/**
 * Draws a crisp 16x16 pixel-art food icon inside feed flyout buttons
 */
inline void drawFoodIcon(TFT_eSPI& tft, int cx, int cy, const char* foodId) {
  int sx = cx - 8;
  int sy = cy - 8;
  if (strcmp(foodId, "food_apple") == 0) {
    drawPixelSprite16(tft, sx, sy, SPR_FOOD_APPLE);
  } else if (strcmp(foodId, "food_snack") == 0) {
    drawPixelSprite16(tft, sx, sy, SPR_FOOD_SNACK);
  } else if (strcmp(foodId, "food_herb_salad") == 0) {
    drawPixelSprite16(tft, sx, sy, SPR_FOOD_SALAD);
  } else if (strcmp(foodId, "food_berry_pie") == 0) {
    drawPixelSprite16(tft, sx, sy, SPR_FOOD_PIE);
  } else if (strcmp(foodId, "food_sun_jerky") == 0) {
    drawPixelSprite16(tft, sx, sy, SPR_FOOD_JERKY);
  } else {
    drawPixelSprite16(tft, sx, sy, SPR_FOOD_STEW);
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
