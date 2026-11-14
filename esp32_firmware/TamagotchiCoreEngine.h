/**
 * ============================================================================
 * TamagotchiCoreEngine.h
 * Platform-Independent, UI-Agnostic C++ Game Runtime Engine
 * ============================================================================
 * Decoupled from screen resolution, display libraries (TFT_eSPI / LovyanGFX),
 * and hardware pins. Can be ported to any microcontroller (ESP32, RP2040, STM32)
 * or compiled inside Android NDK / desktop C++.
 *
 * Zero dynamic heap allocation: uses fixed-size structs and buffers.
 * Reads human-readable rules configuration (tamagotchi_rules.cfg / rules.json)
 * at runtime.
 * ============================================================================
 */
#ifndef TAMAGOTCHI_CORE_ENGINE_H
#define TAMAGOTCHI_CORE_ENGINE_H

#if defined(ARDUINO)
  #include <Arduino.h>
#elif defined(MOCK_ARDUINO_H)
  #include <Arduino.h>
#else
  #include <algorithm>
  #include <cmath>
  #include <cstdint>
  #include <cstdlib>
  #include <cstring>
  #ifdef ESP_PLATFORM
    #include "esp_random.h"
  #endif
#endif

static inline int engineMin(int a, int b) { return (a < b) ? a : b; }
static inline int engineMax(int a, int b) { return (a > b) ? a : b; }
static inline int engineClamp(int v, int lo, int hi) { return (v < lo) ? lo : ((v > hi) ? hi : v); }
static inline long engineRandom(long lo, long hi) {
  if (lo >= hi) return lo;
#if defined(ARDUINO) || defined(MOCK_ARDUINO_H)
  return random(lo, hi);
#elif defined(ESP_PLATFORM)
  return lo + (long)(esp_random() % (uint32_t)(hi - lo));
#else
  return lo + (std::rand() % (hi - lo));
#endif
}

struct EngineStats {
  uint8_t health    = 100;
  uint8_t hunger    = 85;
  uint8_t energy    = 90;
  uint8_t hygiene   = 95;
  uint8_t happiness = 90;
  uint8_t mana      = 50;
};

struct EngineInventorySlot {
  char id[20]          = "";
  char name[24]        = "";
  char name_de[24]     = "";
  uint8_t category     = 0;  // 0 = food, 1 = material, 2 = potion, 3 = accessory
  uint8_t quantity     = 0;
  uint8_t hungerGain   = 0;
  uint8_t healthGain   = 0;
  uint8_t happyGain    = 0;
  uint8_t energyGain   = 0;
  uint8_t satietyTicks = 0;
  uint8_t sickChancePct = 0; // 0..100
  bool cureSickness    = false;
};

struct FeedResult {
  bool success          = false;
  bool becameSick       = false;
  uint8_t hungerGain    = 0;
  uint8_t satietyGained = 0;
};

struct EngineFoodDef {
  char id[20]          = "";
  char name[24]        = "";
  char name_de[24]     = "";
  uint8_t category     = 0; // 0 = food, 1 = material, 2 = potion, 3 = accessory
  uint8_t hungerGain   = 0;
  uint8_t healthGain   = 0;
  uint8_t happyGain    = 0;
  uint8_t energyGain   = 0;
  uint8_t manaGain     = 0;
  uint8_t satietyTicks = 0;
  uint8_t sickChancePct = 0;
  bool cureSickness    = false;
};

struct EngineStartingItem {
  char id[20]      = "";
  uint8_t quantity = 1;
};

struct EngineBiomeRule {
  char id[16]              = "";
  char name[16]            = "";
  char name_de[16]         = "";
  float foodDecayMult      = 1.0f;
  float energyDecayMult    = 1.0f;
  float happinessDecayMult = 1.0f;
  float eggHatchSpeed      = 1.0f;
  int16_t maxManaBonus     = 0;
};

struct EngineRecipeIngredient {
  char itemId[20] = "";
  uint8_t count   = 1;
};

struct EngineRecipe {
  char id[16]           = "";
  char name[24]         = "";
  char name_de[24]      = "";
  char resultItemId[20] = "";
  uint8_t minLevel      = 1;
  EngineRecipeIngredient ingredients[4];
  uint8_t ingredientCount = 0;
  uint8_t hungerGain    = 25;
  uint8_t healthGain    = 0;
  uint8_t happyGain     = 10;
  uint8_t energyGain    = 0;
  uint8_t satietyTicks  = 4;
  uint16_t xpReward     = 35;
};

struct EngineJob {
  char id[16]            = "";
  char name[28]          = "";
  char name_de[28]       = "";
  uint8_t minLevel       = 1;
  uint8_t energyCost     = 15;
  uint16_t durationSec   = 60;
  uint16_t xpReward      = 25;
  uint16_t coinReward    = 15;
  char matRewardId[20]   = "";
  uint8_t matRewardQty   = 0;
  char matReward2Id[20]  = "";
  uint8_t matReward2Qty  = 0;
  char foodRewardId[20]  = "";
  uint8_t foodRewardQty  = 0;
};

struct EngineSpell {
  char id[24]          = "";
  char name[24]        = "";
  char name_de[24]     = "";
  uint8_t minLevel     = 1;
  uint8_t manaCost     = 15;
  uint8_t target       = 0; // 0 = self, 1 = friend, 2 = both
  bool cleanPoop       = false;
  bool cureSickness    = false;
  uint8_t healthGain   = 0;
  uint8_t happyGain    = 0;
  uint8_t hungerGain   = 0;
  uint8_t satietyTicks = 0;
  uint16_t xpReward    = 20;
};

struct EngineDailyMission {
  char id[24]         = "";
  char name[32]       = "";
  char name_de[32]    = "";
  char actionType[16] = ""; // feed, play, bath, magic, send_treat, login_streak
  uint8_t targetCount = 1;
  uint8_t currentCount = 0;
  uint16_t coinReward = 20;
  uint16_t xpReward   = 30;
  bool completed      = false;
  bool claimed        = false;
};

struct EngineShopItem {
  char id[20]           = "";
  char name[24]         = "";
  char name_de[24]      = "";
  uint16_t cost         = 10;
  uint8_t minLevel      = 1;
  uint8_t category      = 0; // 0 = food, 1 = material, 2 = potion, 3 = accessory
  uint8_t hungerGain    = 0;
  uint8_t happyGain     = 0;
  uint8_t healthGain    = 0;
  uint8_t energyGain    = 0;
  uint8_t manaGain      = 0;
  uint8_t satietyTicks  = 0;
  uint8_t sickChancePct = 0;
  bool cureSickness     = false;
};

struct EngineShopRecipe {
  char id[20]       = "";
  char name[24]     = "";
  char name_de[24]  = "";
  uint16_t cost     = 50;
  uint8_t minLevel  = 2;
  char recipeId[20] = "";
};

struct EngineShopSpell {
  char id[24]       = "";
  char name[24]     = "";
  char name_de[24]  = "";
  uint16_t cost     = 75;
  uint8_t minLevel  = 3;
  uint8_t manaCost  = 25;
  uint8_t healPower = 40;
};

struct EngineRules {
  char version[16]              = "1.0.4";
  uint16_t tickIntervalSeconds  = 6;
  uint8_t maxStatValue          = 100;
  uint8_t hungerDecay           = 1;
  uint8_t energyDecay           = 1;
  uint8_t happinessDecay        = 1;
  uint8_t starveThreshold       = 20;
  uint8_t healthStarveDecay     = 1;
  uint8_t naturalManaRegen      = 2;
  float hungerDecayFloat        = 1.0f;
  float energyDecayFloat        = 1.0f;
  float happinessDecayFloat     = 1.0f;
  float healthStarveDecayFloat  = 1.0f;
  float sickDecayFloat          = 1.0f;
  float manaRegenFloat          = 2.0f;
  uint8_t eggWarmthRequired     = 100;
  uint8_t babyMinLevel          = 1;
  uint8_t youthMinLevel         = 4;
  uint8_t adultMinLevel         = 8;
  uint8_t elderMinLevel         = 15;
  uint16_t careMistakeThresholdSec = 900;
  uint16_t baseXpRequirement    = 100;
  float xpLevelMultiplier       = 1.30f;
  uint8_t treatHungerGain       = 25;
  uint8_t treatHappinessGain    = 20;
  uint8_t treatXpReward         = 30;
  uint8_t coopQuestCoinsReward  = 40;

  EngineBiomeRule biomes[4];
  uint8_t biomeCount            = 0;
  EngineFoodDef foodDefs[12];
  uint8_t foodDefCount          = 0;
  EngineStartingItem startingInventory[8];
  uint8_t startingInventoryCount = 0;
  EngineRecipe recipes[8];
  uint8_t recipeCount           = 0;
  EngineJob jobs[6];
  uint8_t jobCount              = 0;
  EngineSpell spells[6];
  uint8_t spellCount            = 0;
  EngineDailyMission missions[6];
  uint8_t missionCount          = 0;
  EngineShopItem shopItems[16];
  uint8_t shopItemCount         = 0;
  EngineShopRecipe shopRecipes[6];
  uint8_t shopRecipeCount       = 0;
  EngineShopSpell shopSpells[6];
  uint8_t shopSpellCount        = 0;
};

// ============================================================================
// ZERO-HEAP RECURSIVE-DESCENT JSON CURSOR (For SPIFFS/OTA rules.json)
// ============================================================================
struct EngineJsonCursor {
  const char* p;

  void skipWs() {
    while (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n') p++;
  }

  bool consume(char ch) {
    skipWs();
    if (*p == ch) {
      p++;
      return true;
    }
    return false;
  }

  bool readString(char* dst, size_t maxLen) {
    skipWs();
    if (*p != '"') {
      if (dst && maxLen > 0) dst[0] = '\0';
      return false;
    }
    p++;
    size_t i = 0;
    while (*p != '\0' && *p != '"') {
      char c = *p++;
      if (c == '\\' && *p != '\0') {
        char esc = *p++;
        if (esc == 'n') c = '\n';
        else if (esc == 'r') c = '\r';
        else if (esc == 't') c = '\t';
        else if (esc == 'u' && p[0] && p[1] && p[2] && p[3]) {
          unsigned int code = 0;
          for (int k = 0; k < 4; k++) {
            char h = p[k];
            code <<= 4;
            if (h >= '0' && h <= '9') code |= (unsigned int)(h - '0');
            else if (h >= 'a' && h <= 'f') code |= (unsigned int)(h - 'a' + 10);
            else if (h >= 'A' && h <= 'F') code |= (unsigned int)(h - 'A' + 10);
          }
          p += 4;
          if (code < 0x80) {
            c = (char)code;
          } else if (code < 0x800 && dst && i + 2 < maxLen) {
            dst[i++] = (char)(0xC0 | (code >> 6));
            c = (char)(0x80 | (code & 0x3F));
          } else {
            c = '?';
          }
        } else {
          c = esc;
        }
      }
      if (dst && i + 1 < maxLen) {
        dst[i++] = c;
      }
    }
    if (*p == '"') p++;
    if (dst && maxLen > 0) dst[i] = '\0';
    return true;
  }

  bool readDouble(double& out) {
    skipWs();
    char* endPtr = nullptr;
    double v = std::strtod(p, &endPtr);
    if (endPtr == p) return false;
    p = endPtr;
    out = v;
    return true;
  }

  bool readBool(bool& out) {
    skipWs();
    if (std::strncmp(p, "true", 4) == 0) {
      p += 4;
      out = true;
      return true;
    }
    if (std::strncmp(p, "false", 5) == 0) {
      p += 5;
      out = false;
      return true;
    }
    return false;
  }

  void skipValue() {
    skipWs();
    if (*p == '"') {
      readString(nullptr, 0);
    } else if (*p == '{') {
      p++;
      if (!consume('}')) {
        while (*p != '\0') {
          skipValue();
          consume(':');
          skipValue();
          if (consume(',')) continue;
          consume('}');
          break;
        }
      }
    } else if (*p == '[') {
      p++;
      if (!consume(']')) {
        while (*p != '\0') {
          skipValue();
          if (consume(',')) continue;
          consume(']');
          break;
        }
      }
    } else {
      while (*p != '\0' && *p != ',' && *p != '}' && *p != ']' &&
             *p != ' ' && *p != '\t' && *p != '\r' && *p != '\n') {
        p++;
      }
    }
  }

  template <typename Callback>
  bool eachObjectEntry(Callback cb) {
    if (!consume('{')) return false;
    if (consume('}')) return true;
    char key[48];
    while (*p != '\0') {
      if (!readString(key, sizeof(key))) break;
      consume(':');
      skipWs();
      const char* beforeVal = p;
      cb(key, *this);
      if (p == beforeVal) {
        skipValue();
      }
      if (consume(',')) continue;
      consume('}');
      break;
    }
    return true;
  }

  template <typename Callback>
  bool eachArrayElement(Callback cb) {
    if (!consume('[')) return false;
    if (consume(']')) return true;
    int idx = 0;
    while (*p != '\0') {
      skipWs();
      const char* beforeVal = p;
      cb(idx, *this);
      if (p == beforeVal) {
        skipValue();
      }
      idx++;
      if (consume(',')) continue;
      consume(']');
      break;
    }
    return true;
  }
};

class TamagotchiCoreEngine {
public:
  static const uint8_t MAX_INVENTORY_SLOTS = 16;

  EngineRules rules;
  EngineStats stats;

  char name[16]     = "Leafkin";
  char biome[12]    = "grassland";
  char phase[10]    = "Egg";
  uint8_t level     = 1;
  uint16_t xp       = 0;
  uint16_t xpNext   = 100;
  uint16_t coins    = 45;
  uint8_t eggWarmth = 20;

  // Hunger freeze shield & Snack sickness
  uint8_t satietyTicks = 0;
  bool isSick          = false;

  // Fractional decay accumulators (zero heap)
  float hungerAcc    = 0.0f;
  float energyAcc    = 0.0f;
  float happinessAcc = 0.0f;
  float starveAcc    = 0.0f;
  float sickAcc      = 0.0f;
  float manaAcc      = 0.0f;

  // Fixed-size inventory array (zero heap allocation)
  EngineInventorySlot inventory[MAX_INVENTORY_SLOTS];
  uint8_t inventoryCount = 0;

  EngineDailyMission missions[6];
  uint8_t missionCount = 0;
  uint16_t consecutiveLoginDays = 1;
  bool streakClaimedToday = false;

  // Callback types for decoupled hardware event notifications
  typedef void (*EventCallback)(const char* eventName, void* data);
  EventCallback onEvent = nullptr;

  TamagotchiCoreEngine() {
    initStartingInventory();
    initDefaultMissions();
    initDefaultRecipes();
    initDefaultJobs();
    initDefaultSpells();
    initDefaultShop();
  }

  static void copyStr(char* dst, size_t maxLen, const char* src) {
    if (!dst || maxLen == 0) return;
    if (!src) {
      dst[0] = '\0';
      return;
    }
    std::strncpy(dst, src, maxLen - 1);
    dst[maxLen - 1] = '\0';
  }

  static uint8_t parseCategoryStr(const char* cat) {
    if (!cat) return 1;
    if (std::strcmp(cat, "food") == 0) return 0;
    if (std::strcmp(cat, "potion") == 0) return 2;
    if (std::strcmp(cat, "accessory") == 0 || std::strcmp(cat, "evolution") == 0) return 3;
    return 1;
  }

  void initStartingInventory() {
    inventoryCount = 0;
    addKnownItemById("food_apple", 2);
    addKnownItemById("food_snack", 1);
    addKnownItemById("mat_herb", 2);
  }

  void applyStartingInventoryFromRules() {
    if (rules.startingInventoryCount == 0) {
      initStartingInventory();
      return;
    }
    inventoryCount = 0;
    for (uint8_t i = 0; i < rules.startingInventoryCount; i++) {
      addKnownItemById(rules.startingInventory[i].id, rules.startingInventory[i].quantity);
    }
  }

  void initDefaultMissions() {
    missionCount = 5;

    copyStr(missions[0].id, sizeof(missions[0].id), "feed_3");
    copyStr(missions[0].name, sizeof(missions[0].name), "Feed Pet 3 times");
    copyStr(missions[0].name_de, sizeof(missions[0].name_de), "3x Fuettern");
    copyStr(missions[0].actionType, sizeof(missions[0].actionType), "feed");
    missions[0].targetCount = 3;
    missions[0].coinReward = 20;
    missions[0].xpReward = 30;

    copyStr(missions[1].id, sizeof(missions[1].id), "play_2");
    copyStr(missions[1].name, sizeof(missions[1].name), "Play Mini-Game 2x");
    copyStr(missions[1].name_de, sizeof(missions[1].name_de), "2x Spielen");
    copyStr(missions[1].actionType, sizeof(missions[1].actionType), "play");
    missions[1].targetCount = 2;
    missions[1].coinReward = 25;
    missions[1].xpReward = 35;

    copyStr(missions[2].id, sizeof(missions[2].id), "bath_1");
    copyStr(missions[2].name, sizeof(missions[2].name), "Give Bath 1x");
    copyStr(missions[2].name_de, sizeof(missions[2].name_de), "1x Baden");
    copyStr(missions[2].actionType, sizeof(missions[2].actionType), "bath");
    missions[2].targetCount = 1;
    missions[2].coinReward = 15;
    missions[2].xpReward = 20;

    copyStr(missions[3].id, sizeof(missions[3].id), "treat_1");
    copyStr(missions[3].name, sizeof(missions[3].name), "Send Friend Treat");
    copyStr(missions[3].name_de, sizeof(missions[3].name_de), "Snack senden");
    copyStr(missions[3].actionType, sizeof(missions[3].actionType), "send_treat");
    missions[3].targetCount = 1;
    missions[3].coinReward = 30;
    missions[3].xpReward = 40;

    copyStr(missions[4].id, sizeof(missions[4].id), "magic_1");
    copyStr(missions[4].name, sizeof(missions[4].name), "Cast or Study Magic");
    copyStr(missions[4].name_de, sizeof(missions[4].name_de), "Magie wirken/forschen");
    copyStr(missions[4].actionType, sizeof(missions[4].actionType), "magic");
    missions[4].targetCount = 1;
    missions[4].coinReward = 25;
    missions[4].xpReward = 35;

    rules.missionCount = missionCount;
    for (uint8_t i = 0; i < missionCount; i++) {
      rules.missions[i] = missions[i];
    }
  }

  void initDefaultRecipes() {
    rules.recipeCount = 5;

    // 0: Meadow Herb Salad
    rules.recipes[0] = {};
    copyStr(rules.recipes[0].id, 16, "rcp_salad");
    copyStr(rules.recipes[0].name, 24, "Herb Salad");
    copyStr(rules.recipes[0].name_de, 24, "Kraeuter-Salat");
    copyStr(rules.recipes[0].resultItemId, 20, "food_herb_salad");
    rules.recipes[0].minLevel = 1;
    rules.recipes[0].ingredientCount = 1;
    copyStr(rules.recipes[0].ingredients[0].itemId, 20, "mat_herb");
    rules.recipes[0].ingredients[0].count = 2;
    rules.recipes[0].hungerGain = 35;
    rules.recipes[0].healthGain = 10;
    rules.recipes[0].energyGain = 10;
    rules.recipes[0].satietyTicks = 4;
    rules.recipes[0].xpReward = 35;

    // 1: Forest Berry Tart
    rules.recipes[1] = {};
    copyStr(rules.recipes[1].id, 16, "rcp_berry_pie");
    copyStr(rules.recipes[1].name, 24, "Berry Tart");
    copyStr(rules.recipes[1].name_de, 24, "Beeren-Tarte");
    copyStr(rules.recipes[1].resultItemId, 20, "food_berry_pie");
    rules.recipes[1].minLevel = 2;
    rules.recipes[1].ingredientCount = 2;
    copyStr(rules.recipes[1].ingredients[0].itemId, 20, "mat_herb");
    rules.recipes[1].ingredients[0].count = 1;
    copyStr(rules.recipes[1].ingredients[1].itemId, 20, "mat_mushroom");
    rules.recipes[1].ingredients[1].count = 1;
    rules.recipes[1].hungerGain = 45;
    rules.recipes[1].healthGain = 15;
    rules.recipes[1].happyGain = 25;
    rules.recipes[1].satietyTicks = 5;
    rules.recipes[1].xpReward = 60;

    // 2: Sun-Dried Jerky
    rules.recipes[2] = {};
    copyStr(rules.recipes[2].id, 16, "rcp_jerky");
    copyStr(rules.recipes[2].name, 24, "Sun Jerky");
    copyStr(rules.recipes[2].name_de, 24, "Doerrfleisch");
    copyStr(rules.recipes[2].resultItemId, 20, "food_sun_jerky");
    rules.recipes[2].minLevel = 2;
    rules.recipes[2].ingredientCount = 2;
    copyStr(rules.recipes[2].ingredients[0].itemId, 20, "mat_meat");
    rules.recipes[2].ingredients[0].count = 2;
    copyStr(rules.recipes[2].ingredients[1].itemId, 20, "mat_sunstone");
    rules.recipes[2].ingredients[1].count = 1;
    rules.recipes[2].hungerGain = 55;
    rules.recipes[2].healthGain = 10;
    rules.recipes[2].energyGain = 20;
    rules.recipes[2].satietyTicks = 6;
    rules.recipes[2].xpReward = 80;

    // 3: Kelp & Mushroom Stew
    rules.recipes[3] = {};
    copyStr(rules.recipes[3].id, 16, "rcp_stew");
    copyStr(rules.recipes[3].name, 24, "Kelp Stew");
    copyStr(rules.recipes[3].name_de, 24, "Pilzeintopf");
    copyStr(rules.recipes[3].resultItemId, 20, "food_kelp_stew");
    rules.recipes[3].minLevel = 4;
    rules.recipes[3].ingredientCount = 2;
    copyStr(rules.recipes[3].ingredients[0].itemId, 20, "mat_kelp");
    rules.recipes[3].ingredients[0].count = 2;
    copyStr(rules.recipes[3].ingredients[1].itemId, 20, "mat_mushroom");
    rules.recipes[3].ingredients[1].count = 2;
    rules.recipes[3].hungerGain = 75;
    rules.recipes[3].healthGain = 30;
    rules.recipes[3].energyGain = 15;
    rules.recipes[3].satietyTicks = 8;
    rules.recipes[3].xpReward = 110;

    // 4: Elemental Ambrosia
    rules.recipes[4] = {};
    copyStr(rules.recipes[4].id, 16, "rcp_ambrosia");
    copyStr(rules.recipes[4].name, 24, "Ambrosia");
    copyStr(rules.recipes[4].name_de, 24, "Ambrosia");
    copyStr(rules.recipes[4].resultItemId, 20, "food_ambrosia");
    rules.recipes[4].minLevel = 6;
    rules.recipes[4].ingredientCount = 4;
    copyStr(rules.recipes[4].ingredients[0].itemId, 20, "mat_sunstone");
    rules.recipes[4].ingredients[0].count = 2;
    copyStr(rules.recipes[4].ingredients[1].itemId, 20, "mat_kelp");
    rules.recipes[4].ingredients[1].count = 2;
    copyStr(rules.recipes[4].ingredients[2].itemId, 20, "mat_mushroom");
    rules.recipes[4].ingredients[2].count = 2;
    copyStr(rules.recipes[4].ingredients[3].itemId, 20, "mat_herb");
    rules.recipes[4].ingredients[3].count = 2;
    rules.recipes[4].hungerGain = 100;
    rules.recipes[4].healthGain = 100;
    rules.recipes[4].energyGain = 100;
    rules.recipes[4].satietyTicks = 12;
    rules.recipes[4].xpReward = 250;
  }

  void initDefaultJobs() {
    rules.jobCount = 4;

    rules.jobs[0] = {};
    copyStr(rules.jobs[0].id, 16, "job_forage");
    copyStr(rules.jobs[0].name, 28, "Herb & Apple Forager");
    copyStr(rules.jobs[0].name_de, 28, "Kraeuter- & Apfelsammler");
    rules.jobs[0].minLevel = 1;
    rules.jobs[0].energyCost = 15;
    rules.jobs[0].durationSec = 60;
    rules.jobs[0].xpReward = 25;
    rules.jobs[0].coinReward = 15;
    copyStr(rules.jobs[0].matRewardId, 20, "mat_herb");
    rules.jobs[0].matRewardQty = 2;
    copyStr(rules.jobs[0].foodRewardId, 20, "food_apple");
    rules.jobs[0].foodRewardQty = 2;

    rules.jobs[1] = {};
    copyStr(rules.jobs[1].id, 16, "job_patrol");
    copyStr(rules.jobs[1].name, 28, "CYD Village Scout");
    copyStr(rules.jobs[1].name_de, 28, "Dorfwache & Bote");
    rules.jobs[1].minLevel = 2;
    rules.jobs[1].energyCost = 20;
    rules.jobs[1].durationSec = 120;
    rules.jobs[1].xpReward = 50;
    rules.jobs[1].coinReward = 30;
    copyStr(rules.jobs[1].matRewardId, 20, "mat_meat");
    rules.jobs[1].matRewardQty = 1;
    copyStr(rules.jobs[1].foodRewardId, 20, "food_snack");
    rules.jobs[1].foodRewardQty = 1;

    rules.jobs[2] = {};
    copyStr(rules.jobs[2].id, 16, "job_mine");
    copyStr(rules.jobs[2].name, 28, "Biome Crystal Miner");
    copyStr(rules.jobs[2].name_de, 28, "Kristall- & Pilzsammler");
    rules.jobs[2].minLevel = 3;
    rules.jobs[2].energyCost = 30;
    rules.jobs[2].durationSec = 180;
    rules.jobs[2].xpReward = 90;
    rules.jobs[2].coinReward = 55;
    copyStr(rules.jobs[2].matRewardId, 20, "mat_sunstone");
    rules.jobs[2].matRewardQty = 1;
    copyStr(rules.jobs[2].matReward2Id, 20, "mat_mushroom");
    rules.jobs[2].matReward2Qty = 1;
    copyStr(rules.jobs[2].foodRewardId, 20, "food_sun_jerky");
    rules.jobs[2].foodRewardQty = 1;

    rules.jobs[3] = {};
    copyStr(rules.jobs[3].id, 16, "job_alchemist");
    copyStr(rules.jobs[3].name, 28, "Royal CYD Alchemist");
    copyStr(rules.jobs[3].name_de, 28, "Hof-Alchemist");
    rules.jobs[3].minLevel = 5;
    rules.jobs[3].energyCost = 35;
    rules.jobs[3].durationSec = 300;
    rules.jobs[3].xpReward = 150;
    rules.jobs[3].coinReward = 90;
    copyStr(rules.jobs[3].matRewardId, 20, "mat_kelp");
    rules.jobs[3].matRewardQty = 2;
    copyStr(rules.jobs[3].foodRewardId, 20, "food_herb_salad");
    rules.jobs[3].foodRewardQty = 1;
  }

  void initDefaultSpells() {
    rules.spellCount = 5;

    rules.spells[0] = {};
    copyStr(rules.spells[0].id, 24, "spl_cleanse");
    copyStr(rules.spells[0].name, 24, "Purify");
    copyStr(rules.spells[0].name_de, 24, "Laeuterung");
    rules.spells[0].minLevel = 1;
    rules.spells[0].manaCost = 15;
    rules.spells[0].target = 0;
    rules.spells[0].cleanPoop = true;
    rules.spells[0].cureSickness = true;
    rules.spells[0].healthGain = 10;
    rules.spells[0].xpReward = 20;

    rules.spells[1] = {};
    copyStr(rules.spells[1].id, 24, "spl_heal");
    copyStr(rules.spells[1].name, 24, "Vitality Aura");
    copyStr(rules.spells[1].name_de, 24, "Vitalitaets-Aura");
    rules.spells[1].minLevel = 2;
    rules.spells[1].manaCost = 20;
    rules.spells[1].target = 0;
    rules.spells[1].healthGain = 25;
    rules.spells[1].happyGain = 10;
    rules.spells[1].xpReward = 35;

    rules.spells[2] = {};
    copyStr(rules.spells[2].id, 24, "spl_treat_burst");
    copyStr(rules.spells[2].name, 24, "Astral Feast");
    copyStr(rules.spells[2].name_de, 24, "Astral-Festmahl");
    rules.spells[2].minLevel = 3;
    rules.spells[2].manaCost = 25;
    rules.spells[2].target = 0;
    rules.spells[2].hungerGain = 35;
    rules.spells[2].satietyTicks = 3;
    rules.spells[2].xpReward = 40;

    rules.spells[3] = {};
    copyStr(rules.spells[3].id, 24, "spl_friend_blessing");
    copyStr(rules.spells[3].name, 24, "Friend Blessing");
    copyStr(rules.spells[3].name_de, 24, "Freundes-Segen");
    rules.spells[3].minLevel = 3;
    rules.spells[3].manaCost = 30;
    rules.spells[3].target = 1;
    rules.spells[3].healthGain = 35;
    rules.spells[3].happyGain = 25;
    rules.spells[3].xpReward = 45;

    rules.spells[4] = {};
    copyStr(rules.spells[4].id, 24, "spl_astral_harmony");
    copyStr(rules.spells[4].name, 24, "Dual Harmony");
    copyStr(rules.spells[4].name_de, 24, "Astrale Harmonie");
    rules.spells[4].minLevel = 5;
    rules.spells[4].manaCost = 45;
    rules.spells[4].target = 2;
    rules.spells[4].healthGain = 50;
    rules.spells[4].happyGain = 40;
    rules.spells[4].xpReward = 100;
  }

  void initDefaultShop() {
    rules.shopItemCount = 6;

    rules.shopItems[0] = {};
    copyStr(rules.shopItems[0].id, 20, "food_apple");
    copyStr(rules.shopItems[0].name, 24, "Crisp Apple");
    copyStr(rules.shopItems[0].name_de, 24, "Frischer Apfel");
    rules.shopItems[0].cost = 5;
    rules.shopItems[0].minLevel = 1;
    rules.shopItems[0].category = 0;
    rules.shopItems[0].hungerGain = 12;
    rules.shopItems[0].happyGain = 5;
    rules.shopItems[0].healthGain = 2;

    rules.shopItems[1] = {};
    copyStr(rules.shopItems[1].id, 20, "food_snack");
    copyStr(rules.shopItems[1].name, 24, "Pixel Snack");
    copyStr(rules.shopItems[1].name_de, 24, "Pixel-Snack");
    rules.shopItems[1].cost = 8;
    rules.shopItems[1].minLevel = 1;
    rules.shopItems[1].category = 0;
    rules.shopItems[1].hungerGain = 25;
    rules.shopItems[1].happyGain = 15;
    rules.shopItems[1].energyGain = 5;
    rules.shopItems[1].sickChancePct = 30;

    rules.shopItems[2] = {};
    copyStr(rules.shopItems[2].id, 20, "food_herb_salad");
    copyStr(rules.shopItems[2].name, 24, "Herb Salad");
    copyStr(rules.shopItems[2].name_de, 24, "Kraeuter-Salat");
    rules.shopItems[2].cost = 22;
    rules.shopItems[2].minLevel = 1;
    rules.shopItems[2].category = 0;
    rules.shopItems[2].hungerGain = 35;
    rules.shopItems[2].happyGain = 10;
    rules.shopItems[2].healthGain = 10;
    rules.shopItems[2].energyGain = 10;
    rules.shopItems[2].satietyTicks = 4;

    rules.shopItems[3] = {};
    copyStr(rules.shopItems[3].id, 20, "item_salve");
    copyStr(rules.shopItems[3].name, 24, "Healing Salve");
    copyStr(rules.shopItems[3].name_de, 24, "Heilsalbe");
    rules.shopItems[3].cost = 25;
    rules.shopItems[3].minLevel = 1;
    rules.shopItems[3].category = 2;
    rules.shopItems[3].healthGain = 40;
    rules.shopItems[3].cureSickness = true;

    rules.shopItems[4] = {};
    copyStr(rules.shopItems[4].id, 20, "item_energy_elixir");
    copyStr(rules.shopItems[4].name, 24, "Energy Tonic");
    copyStr(rules.shopItems[4].name_de, 24, "Vital-Tonikum");
    rules.shopItems[4].cost = 35;
    rules.shopItems[4].minLevel = 2;
    rules.shopItems[4].category = 2;
    rules.shopItems[4].energyGain = 60;
    rules.shopItems[4].manaGain = 15;

    rules.shopItems[5] = {};
    copyStr(rules.shopItems[5].id, 20, "mat_herb");
    copyStr(rules.shopItems[5].name, 24, "Meadow Herb");
    copyStr(rules.shopItems[5].name_de, 24, "Wiesenkraeuter");
    rules.shopItems[5].cost = 8;
    rules.shopItems[5].minLevel = 1;
    rules.shopItems[5].category = 1;

    rules.shopRecipeCount = 2;
    rules.shopRecipes[0] = {};
    copyStr(rules.shopRecipes[0].id, 20, "rcp_berry_pie");
    copyStr(rules.shopRecipes[0].name, 24, "Berry Tart");
    copyStr(rules.shopRecipes[0].name_de, 24, "Waldbeeren-Tarte");
    rules.shopRecipes[0].cost = 45;
    rules.shopRecipes[0].minLevel = 2;
    copyStr(rules.shopRecipes[0].recipeId, 20, "rcp_berry_pie");

    rules.shopRecipes[1] = {};
    copyStr(rules.shopRecipes[1].id, 20, "rcp_stew");
    copyStr(rules.shopRecipes[1].name, 24, "Kelp Stew");
    copyStr(rules.shopRecipes[1].name_de, 24, "Pilzeintopf");
    rules.shopRecipes[1].cost = 80;
    rules.shopRecipes[1].minLevel = 4;
    copyStr(rules.shopRecipes[1].recipeId, 20, "rcp_stew");

    rules.shopSpellCount = 2;
    rules.shopSpells[0] = {};
    copyStr(rules.shopSpells[0].id, 24, "spl_treat_burst");
    copyStr(rules.shopSpells[0].name, 24, "Astral Feast");
    copyStr(rules.shopSpells[0].name_de, 24, "Astral-Festmahl");
    rules.shopSpells[0].cost = 70;
    rules.shopSpells[0].minLevel = 3;
    rules.shopSpells[0].manaCost = 25;
    rules.shopSpells[0].healPower = 35;

    rules.shopSpells[1] = {};
    copyStr(rules.shopSpells[1].id, 24, "spl_rain_dance");
    copyStr(rules.shopSpells[1].name, 24, "Astral Harmony");
    copyStr(rules.shopSpells[1].name_de, 24, "Astrale Harmonie");
    rules.shopSpells[1].cost = 120;
    rules.shopSpells[1].minLevel = 5;
    rules.shopSpells[1].manaCost = 45;
    rules.shopSpells[1].healPower = 50;
  }

  const EngineBiomeRule* findBiomeRule(const char* biomeId) const {
    if (!biomeId) return nullptr;
    for (uint8_t i = 0; i < rules.biomeCount; i++) {
      if (std::strcmp(rules.biomes[i].id, biomeId) == 0) {
        return &rules.biomes[i];
      }
    }
    return nullptr;
  }

  uint8_t getEffectiveMaxMana() const {
    const EngineBiomeRule* b = findBiomeRule(biome);
    int cap = (int)rules.maxStatValue + (b ? (int)b->maxManaBonus : 0);
    return (uint8_t)engineClamp(cap, 50, 150);
  }

  // --------------------------------------------------------------------------
  // Runtime Config Loader (Full Zero-Heap JSON rules.json + .cfg fallback)
  // --------------------------------------------------------------------------
  static void trimAndStrip(char* s) {
    if (!s) return;
    int len = (int)std::strlen(s);
    int start = 0;
    while (start < len && (s[start] == ' ' || s[start] == '\t' || s[start] == '\r' || s[start] == '\n' || s[start] == '"' || s[start] == ',')) {
      start++;
    }
    int end = len;
    while (end > start && (s[end - 1] == ' ' || s[end - 1] == '\t' || s[end - 1] == '\r' || s[end - 1] == '\n' || s[end - 1] == '"' || s[end - 1] == ',')) {
      end--;
    }
    if (start > 0) {
      std::memmove(s, s + start, (size_t)(end - start));
    }
    s[end - start] = '\0';
  }

  static uint8_t parseSickPct(double rawVal) {
    if (rawVal <= 0.0) return 0;
    if (rawVal <= 1.0) return (uint8_t)engineClamp((int)std::llround(rawVal * 100.0), 0, 100);
    return (uint8_t)engineClamp((int)std::llround(rawVal), 0, 100);
  }

  void syncMissionsFromRules() {
    if (rules.missionCount == 0) return;
    EngineDailyMission prev[6];
    uint8_t prevCount = missionCount;
    for (uint8_t i = 0; i < prevCount && i < 6; i++) prev[i] = missions[i];

    missionCount = rules.missionCount;
    for (uint8_t i = 0; i < missionCount; i++) {
      missions[i] = rules.missions[i];
      for (uint8_t j = 0; j < prevCount; j++) {
        if (std::strcmp(prev[j].id, missions[i].id) == 0 ||
            std::strcmp(prev[j].actionType, missions[i].actionType) == 0) {
          missions[i].currentCount = (uint8_t)engineMin((int)prev[j].currentCount, (int)missions[i].targetCount);
          missions[i].completed = (missions[i].currentCount >= missions[i].targetCount) || prev[j].completed;
          missions[i].claimed = prev[j].claimed;
          break;
        }
      }
    }
  }

  bool loadRulesJson(const char* jsonText) {
    if (!jsonText) return false;
    EngineJsonCursor cur{ jsonText };
    cur.skipWs();
    if (*cur.p != '{') return false;

    bool ok = cur.eachObjectEntry([&](const char* key, EngineJsonCursor& c) {
      if (std::strcmp(key, "version") == 0) {
        c.readString(rules.version, sizeof(rules.version));
      } else if (std::strcmp(key, "meta") == 0) {
        c.eachObjectEntry([&](const char* mk, EngineJsonCursor& mc) {
          double dv = 0;
          if (std::strcmp(mk, "tick_interval_seconds") == 0 && mc.readDouble(dv)) {
            rules.tickIntervalSeconds = (uint16_t)engineMax(1, (int)dv);
          } else if (std::strcmp(mk, "base_xp_requirement") == 0 && mc.readDouble(dv)) {
            rules.baseXpRequirement = (uint16_t)engineMax(10, (int)dv);
          } else if (std::strcmp(mk, "xp_level_multiplier") == 0 && mc.readDouble(dv)) {
            if (dv >= 1.0 && dv <= 5.0) rules.xpLevelMultiplier = (float)dv;
          }
        });
      } else if (std::strcmp(key, "stats") == 0) {
        c.eachObjectEntry([&](const char* sk, EngineJsonCursor& sc) {
          if (std::strcmp(sk, "food") == 0) {
            sc.eachObjectEntry([&](const char* fk, EngineJsonCursor& fc) {
              double dv = 0;
              if (std::strcmp(fk, "max") == 0 && fc.readDouble(dv)) {
                rules.maxStatValue = (uint8_t)engineClamp((int)dv, 50, 250);
              } else if (std::strcmp(fk, "decay_per_tick") == 0 && fc.readDouble(dv)) {
                rules.hungerDecayFloat = (float)dv;
                rules.hungerDecay = (uint8_t)(dv > 0.0 ? engineMax(1, (int)std::llround(dv)) : 0);
              }
            });
          } else if (std::strcmp(sk, "health") == 0) {
            sc.eachObjectEntry([&](const char* hk, EngineJsonCursor& hc) {
              double dv = 0;
              if (std::strcmp(hk, "starve_decay") == 0 && hc.readDouble(dv)) {
                rules.healthStarveDecayFloat = (float)dv;
                rules.healthStarveDecay = (uint8_t)(dv > 0.0 ? engineMax(1, (int)std::llround(dv)) : 0);
              } else if (std::strcmp(hk, "sick_decay") == 0 && hc.readDouble(dv)) {
                rules.sickDecayFloat = (float)dv;
              }
            });
          } else if (std::strcmp(sk, "happiness") == 0) {
            sc.eachObjectEntry([&](const char* hk, EngineJsonCursor& hc) {
              double dv = 0;
              if (std::strcmp(hk, "decay_per_tick") == 0 && hc.readDouble(dv)) {
                rules.happinessDecayFloat = (float)dv;
                rules.happinessDecay = (uint8_t)(dv > 0.0 ? engineMax(1, (int)std::llround(dv)) : 0);
              }
            });
          } else if (std::strcmp(sk, "energy") == 0) {
            sc.eachObjectEntry([&](const char* ek, EngineJsonCursor& ec) {
              double dv = 0;
              if (std::strcmp(ek, "decay_per_tick") == 0 && ec.readDouble(dv)) {
                rules.energyDecayFloat = (float)dv;
                rules.energyDecay = (uint8_t)(dv > 0.0 ? engineMax(1, (int)std::llround(dv)) : 0);
              }
            });
          } else if (std::strcmp(sk, "mana") == 0) {
            sc.eachObjectEntry([&](const char* mk, EngineJsonCursor& mc) {
              double dv = 0;
              if (std::strcmp(mk, "regen_per_tick") == 0 && mc.readDouble(dv)) {
                rules.manaRegenFloat = (float)dv;
                rules.naturalManaRegen = (uint8_t)(dv > 0.0 ? engineMax(1, (int)std::llround(dv)) : 0);
              }
            });
          }
        });
      } else if (std::strcmp(key, "foods") == 0) {
        rules.foodDefCount = 0;
        c.eachArrayElement([&](int /*idx*/, EngineJsonCursor& ec) {
          if (rules.foodDefCount >= 12) return;
          EngineFoodDef fd = {};
          fd.category = 0;
          ec.eachObjectEntry([&](const char* fk, EngineJsonCursor& fc) {
            double dv = 0;
            if (std::strcmp(fk, "id") == 0) fc.readString(fd.id, sizeof(fd.id));
            else if (std::strcmp(fk, "name") == 0) fc.readString(fd.name, sizeof(fd.name));
            else if (std::strcmp(fk, "name_de") == 0) fc.readString(fd.name_de, sizeof(fd.name_de));
            else if (std::strcmp(fk, "hunger_gain") == 0 && fc.readDouble(dv)) fd.hungerGain = (uint8_t)dv;
            else if (std::strcmp(fk, "happiness_gain") == 0 && fc.readDouble(dv)) fd.happyGain = (uint8_t)dv;
            else if (std::strcmp(fk, "health_gain") == 0 && fc.readDouble(dv)) fd.healthGain = (uint8_t)dv;
            else if (std::strcmp(fk, "energy_gain") == 0 && fc.readDouble(dv)) fd.energyGain = (uint8_t)dv;
            else if (std::strcmp(fk, "satiety_ticks") == 0 && fc.readDouble(dv)) fd.satietyTicks = (uint8_t)dv;
            else if (std::strcmp(fk, "sick_chance") == 0 && fc.readDouble(dv)) fd.sickChancePct = parseSickPct(dv);
          });
          if (std::strlen(fd.id) > 0) {
            if (std::strlen(fd.name_de) == 0) copyStr(fd.name_de, sizeof(fd.name_de), fd.name);
            rules.foodDefs[rules.foodDefCount++] = fd;
          }
        });
      } else if (std::strcmp(key, "starting_inventory") == 0) {
        rules.startingInventoryCount = 0;
        c.eachArrayElement([&](int /*idx*/, EngineJsonCursor& ec) {
          if (rules.startingInventoryCount >= 8) return;
          EngineStartingItem si = {};
          ec.eachObjectEntry([&](const char* sk, EngineJsonCursor& sc) {
            double dv = 0;
            if (std::strcmp(sk, "id") == 0) sc.readString(si.id, sizeof(si.id));
            else if (std::strcmp(sk, "quantity") == 0 && sc.readDouble(dv)) si.quantity = (uint8_t)engineMax(1, (int)dv);
          });
          if (std::strlen(si.id) > 0) {
            rules.startingInventory[rules.startingInventoryCount++] = si;
          }
        });
      } else if (std::strcmp(key, "lifecycle") == 0) {
        c.eachObjectEntry([&](const char* lk, EngineJsonCursor& lc) {
          double dv = 0;
          if (std::strcmp(lk, "care_mistake_threshold_seconds") == 0 && lc.readDouble(dv)) {
            rules.careMistakeThresholdSec = (uint16_t)engineMax(60, (int)dv);
          } else if (std::strcmp(lk, "phases") == 0) {
            lc.eachArrayElement([&](int /*idx*/, EngineJsonCursor& pc) {
              char pid[16] = "";
              uint8_t minLv = 0;
              uint8_t warmthReq = 0;
              pc.eachObjectEntry([&](const char* pk, EngineJsonCursor& pfc) {
                double pdv = 0;
                if (std::strcmp(pk, "id") == 0) pfc.readString(pid, sizeof(pid));
                else if (std::strcmp(pk, "min_level") == 0 && pfc.readDouble(pdv)) minLv = (uint8_t)pdv;
                else if (std::strcmp(pk, "warmth_required") == 0 && pfc.readDouble(pdv)) warmthReq = (uint8_t)pdv;
              });
              if (std::strcmp(pid, "egg") == 0 && warmthReq > 0) rules.eggWarmthRequired = warmthReq;
              else if (std::strcmp(pid, "baby") == 0 && minLv > 0) rules.babyMinLevel = minLv;
              else if (std::strcmp(pid, "youth") == 0 && minLv > 0) rules.youthMinLevel = minLv;
              else if (std::strcmp(pid, "adult") == 0 && minLv > 0) rules.adultMinLevel = minLv;
              else if (std::strcmp(pid, "elder") == 0 && minLv > 0) rules.elderMinLevel = minLv;
            });
          }
        });
      } else if (std::strcmp(key, "biomes") == 0) {
        rules.biomeCount = 0;
        c.eachObjectEntry([&](const char* bk, EngineJsonCursor& bc) {
          if (rules.biomeCount >= 4) return;
          EngineBiomeRule br = {};
          copyStr(br.id, sizeof(br.id), bk);
          bc.eachObjectEntry([&](const char* pk, EngineJsonCursor& pc) {
            if (std::strcmp(pk, "name") == 0) pc.readString(br.name, sizeof(br.name));
            else if (std::strcmp(pk, "name_de") == 0) pc.readString(br.name_de, sizeof(br.name_de));
            else if (std::strcmp(pk, "modifiers") == 0) {
              pc.eachObjectEntry([&](const char* mk, EngineJsonCursor& mc) {
                double dv = 0;
                if (!mc.readDouble(dv)) return;
                if (std::strcmp(mk, "food_decay") == 0) br.foodDecayMult = (float)dv;
                else if (std::strcmp(mk, "energy_decay") == 0) br.energyDecayMult = (float)dv;
                else if (std::strcmp(mk, "happiness_decay") == 0) br.happinessDecayMult = (float)dv;
                else if (std::strcmp(mk, "egg_hatch_speed") == 0) br.eggHatchSpeed = (float)dv;
                else if (std::strcmp(mk, "max_mana") == 0) br.maxManaBonus = (int16_t)dv;
              });
            }
          });
          rules.biomes[rules.biomeCount++] = br;
        });
      } else if (std::strcmp(key, "recipes") == 0) {
        rules.recipeCount = 0;
        c.eachArrayElement([&](int /*idx*/, EngineJsonCursor& rc) {
          if (rules.recipeCount >= 8) return;
          EngineRecipe r = {};
          rc.eachObjectEntry([&](const char* rk, EngineJsonCursor& rfc) {
            double dv = 0;
            if (std::strcmp(rk, "id") == 0) rfc.readString(r.id, sizeof(r.id));
            else if (std::strcmp(rk, "name") == 0) rfc.readString(r.name, sizeof(r.name));
            else if (std::strcmp(rk, "name_de") == 0) rfc.readString(r.name_de, sizeof(r.name_de));
            else if (std::strcmp(rk, "result_item_id") == 0) rfc.readString(r.resultItemId, sizeof(r.resultItemId));
            else if (std::strcmp(rk, "min_level") == 0 && rfc.readDouble(dv)) r.minLevel = (uint8_t)engineMax(1, (int)dv);
            else if (std::strcmp(rk, "ingredients") == 0) {
              rfc.eachObjectEntry([&](const char* ik, EngineJsonCursor& ic) {
                double cnt = 0;
                if (r.ingredientCount < 4 && ic.readDouble(cnt)) {
                  copyStr(r.ingredients[r.ingredientCount].itemId, sizeof(r.ingredients[0].itemId), ik);
                  r.ingredients[r.ingredientCount].count = (uint8_t)engineMax(1, (int)cnt);
                  r.ingredientCount++;
                }
              });
            } else if (std::strcmp(rk, "effects") == 0) {
              rfc.eachObjectEntry([&](const char* ek, EngineJsonCursor& ec) {
                double edv = 0;
                if (!ec.readDouble(edv)) return;
                if (std::strcmp(ek, "food") == 0 || std::strcmp(ek, "hunger") == 0) r.hungerGain = (uint8_t)edv;
                else if (std::strcmp(ek, "health") == 0) r.healthGain = (uint8_t)edv;
                else if (std::strcmp(ek, "happiness") == 0) r.happyGain = (uint8_t)edv;
                else if (std::strcmp(ek, "energy") == 0) r.energyGain = (uint8_t)edv;
                else if (std::strcmp(ek, "satiety_ticks") == 0) r.satietyTicks = (uint8_t)edv;
                else if (std::strcmp(ek, "xp") == 0) r.xpReward = (uint16_t)edv;
              });
            }
          });
          if (std::strlen(r.id) > 0) {
            if (std::strlen(r.name_de) == 0) copyStr(r.name_de, sizeof(r.name_de), r.name);
            if (std::strlen(r.resultItemId) == 0) copyStr(r.resultItemId, sizeof(r.resultItemId), "food_herb_salad");
            rules.recipes[rules.recipeCount++] = r;
          }
        });
      } else if (std::strcmp(key, "jobs") == 0) {
        rules.jobCount = 0;
        c.eachArrayElement([&](int /*idx*/, EngineJsonCursor& jc) {
          if (rules.jobCount >= 6) return;
          EngineJob jb = {};
          jb.durationSec = 60;
          jb.energyCost = 15;
          jc.eachObjectEntry([&](const char* jk, EngineJsonCursor& jfc) {
            double dv = 0;
            if (std::strcmp(jk, "id") == 0) jfc.readString(jb.id, sizeof(jb.id));
            else if (std::strcmp(jk, "name") == 0) jfc.readString(jb.name, sizeof(jb.name));
            else if (std::strcmp(jk, "name_de") == 0) jfc.readString(jb.name_de, sizeof(jb.name_de));
            else if (std::strcmp(jk, "min_level") == 0 && jfc.readDouble(dv)) jb.minLevel = (uint8_t)engineMax(1, (int)dv);
            else if (std::strcmp(jk, "energy_cost") == 0 && jfc.readDouble(dv)) jb.energyCost = (uint8_t)dv;
            else if (std::strcmp(jk, "duration_seconds") == 0 && jfc.readDouble(dv)) jb.durationSec = (uint16_t)engineMax(5, (int)dv);
            else if (std::strcmp(jk, "rewards") == 0) {
              jfc.eachObjectEntry([&](const char* rk, EngineJsonCursor& rfc) {
                double rdv = 0;
                if (std::strcmp(rk, "xp") == 0 && rfc.readDouble(rdv)) jb.xpReward = (uint16_t)rdv;
                else if (std::strcmp(rk, "coins") == 0 && rfc.readDouble(rdv)) jb.coinReward = (uint16_t)rdv;
                else if (std::strcmp(rk, "materials") == 0) {
                  rfc.eachObjectEntry([&](const char* mk, EngineJsonCursor& mc) {
                    double q = 0;
                    if (mc.readDouble(q) && q > 0) {
                      if (jb.matRewardQty == 0) {
                        copyStr(jb.matRewardId, sizeof(jb.matRewardId), mk);
                        jb.matRewardQty = (uint8_t)q;
                      } else if (jb.matReward2Qty == 0) {
                        copyStr(jb.matReward2Id, sizeof(jb.matReward2Id), mk);
                        jb.matReward2Qty = (uint8_t)q;
                      }
                    }
                  });
                } else if (std::strcmp(rk, "food") == 0) {
                  rfc.eachObjectEntry([&](const char* fk, EngineJsonCursor& fc) {
                    double q = 0;
                    if (fc.readDouble(q) && q > 0 && jb.foodRewardQty == 0) {
                      copyStr(jb.foodRewardId, sizeof(jb.foodRewardId), fk);
                      jb.foodRewardQty = (uint8_t)q;
                    }
                  });
                }
              });
            }
          });
          if (std::strlen(jb.id) > 0) {
            if (std::strlen(jb.name_de) == 0) copyStr(jb.name_de, sizeof(jb.name_de), jb.name);
            rules.jobs[rules.jobCount++] = jb;
          }
        });
      } else if (std::strcmp(key, "spells") == 0) {
        rules.spellCount = 0;
        c.eachArrayElement([&](int /*idx*/, EngineJsonCursor& sc) {
          if (rules.spellCount >= 6) return;
          EngineSpell sp = {};
          sp.manaCost = 15;
          sc.eachObjectEntry([&](const char* sk, EngineJsonCursor& sfc) {
            double dv = 0;
            if (std::strcmp(sk, "id") == 0) sfc.readString(sp.id, sizeof(sp.id));
            else if (std::strcmp(sk, "name") == 0) sfc.readString(sp.name, sizeof(sp.name));
            else if (std::strcmp(sk, "name_de") == 0) sfc.readString(sp.name_de, sizeof(sp.name_de));
            else if (std::strcmp(sk, "min_level") == 0 && sfc.readDouble(dv)) sp.minLevel = (uint8_t)engineMax(1, (int)dv);
            else if (std::strcmp(sk, "mana_cost") == 0 && sfc.readDouble(dv)) sp.manaCost = (uint8_t)dv;
            else if (std::strcmp(sk, "target") == 0) {
              char tgt[16] = "";
              sfc.readString(tgt, sizeof(tgt));
              if (std::strcmp(tgt, "friend") == 0) sp.target = 1;
              else if (std::strcmp(tgt, "both") == 0) sp.target = 2;
              else sp.target = 0;
            } else if (std::strcmp(sk, "effects") == 0) {
              sfc.eachObjectEntry([&](const char* ek, EngineJsonCursor& ec) {
                bool bv = false;
                double edv = 0;
                if (std::strcmp(ek, "clean_poop") == 0 && ec.readBool(bv)) sp.cleanPoop = bv;
                else if (std::strcmp(ek, "cure_sickness") == 0 && ec.readBool(bv)) sp.cureSickness = bv;
                else if (std::strcmp(ek, "health") == 0 && ec.readDouble(edv)) sp.healthGain = (uint8_t)edv;
                else if (std::strcmp(ek, "happiness") == 0 && ec.readDouble(edv)) sp.happyGain = (uint8_t)edv;
                else if (std::strcmp(ek, "hunger") == 0 && ec.readDouble(edv)) sp.hungerGain = (uint8_t)edv;
                else if (std::strcmp(ek, "satiety_ticks") == 0 && ec.readDouble(edv)) sp.satietyTicks = (uint8_t)edv;
                else if (std::strcmp(ek, "xp") == 0 && ec.readDouble(edv)) sp.xpReward = (uint16_t)edv;
              });
            }
          });
          if (std::strlen(sp.id) > 0) {
            if (std::strlen(sp.name_de) == 0) copyStr(sp.name_de, sizeof(sp.name_de), sp.name);
            rules.spells[rules.spellCount++] = sp;
          }
        });
      } else if (std::strcmp(key, "daily_missions") == 0) {
        rules.missionCount = 0;
        c.eachArrayElement([&](int /*idx*/, EngineJsonCursor& mc) {
          if (rules.missionCount >= 6) return;
          EngineDailyMission dm = {};
          mc.eachObjectEntry([&](const char* mk, EngineJsonCursor& mfc) {
            double dv = 0;
            if (std::strcmp(mk, "id") == 0) mfc.readString(dm.id, sizeof(dm.id));
            else if (std::strcmp(mk, "action_type") == 0) mfc.readString(dm.actionType, sizeof(dm.actionType));
            else if (std::strcmp(mk, "name") == 0) mfc.readString(dm.name, sizeof(dm.name));
            else if (std::strcmp(mk, "name_de") == 0) mfc.readString(dm.name_de, sizeof(dm.name_de));
            else if (std::strcmp(mk, "target_count") == 0 && mfc.readDouble(dv)) dm.targetCount = (uint8_t)engineMax(1, (int)dv);
            else if (std::strcmp(mk, "rewards") == 0) {
              mfc.eachObjectEntry([&](const char* rk, EngineJsonCursor& rfc) {
                double rdv = 0;
                if (std::strcmp(rk, "coins") == 0 && rfc.readDouble(rdv)) dm.coinReward = (uint16_t)rdv;
                else if (std::strcmp(rk, "xp") == 0 && rfc.readDouble(rdv)) dm.xpReward = (uint16_t)rdv;
              });
            }
          });
          if (std::strlen(dm.id) > 0) {
            if (std::strlen(dm.name_de) == 0) copyStr(dm.name_de, sizeof(dm.name_de), dm.name);
            rules.missions[rules.missionCount++] = dm;
          }
        });
        syncMissionsFromRules();
      } else if (std::strcmp(key, "shop") == 0) {
        c.eachObjectEntry([&](const char* shk, EngineJsonCursor& shc) {
          if (std::strcmp(shk, "items") == 0) {
            rules.shopItemCount = 0;
            shc.eachArrayElement([&](int /*idx*/, EngineJsonCursor& ic) {
              if (rules.shopItemCount >= 16) return;
              EngineShopItem it = {};
              it.category = 1;
              ic.eachObjectEntry([&](const char* ik, EngineJsonCursor& ifc) {
                double dv = 0;
                if (std::strcmp(ik, "id") == 0) ifc.readString(it.id, sizeof(it.id));
                else if (std::strcmp(ik, "name") == 0) ifc.readString(it.name, sizeof(it.name));
                else if (std::strcmp(ik, "name_de") == 0) ifc.readString(it.name_de, sizeof(it.name_de));
                else if ((std::strcmp(ik, "price_coins") == 0 || std::strcmp(ik, "cost") == 0) && ifc.readDouble(dv)) it.cost = (uint16_t)dv;
                else if (std::strcmp(ik, "min_level") == 0 && ifc.readDouble(dv)) it.minLevel = (uint8_t)engineMax(1, (int)dv);
                else if (std::strcmp(ik, "category") == 0) {
                  char cat[20] = "";
                  ifc.readString(cat, sizeof(cat));
                  it.category = parseCategoryStr(cat);
                } else if (std::strcmp(ik, "effects") == 0) {
                  ifc.eachObjectEntry([&](const char* ek, EngineJsonCursor& ec) {
                    bool bv = false;
                    double edv = 0;
                    if (std::strcmp(ek, "cure_sickness") == 0 && ec.readBool(bv)) it.cureSickness = bv;
                    else if (std::strcmp(ek, "hunger") == 0 && ec.readDouble(edv)) it.hungerGain = (uint8_t)edv;
                    else if (std::strcmp(ek, "happiness") == 0 && ec.readDouble(edv)) it.happyGain = (uint8_t)edv;
                    else if (std::strcmp(ek, "health") == 0 && ec.readDouble(edv)) it.healthGain = (uint8_t)edv;
                    else if (std::strcmp(ek, "energy") == 0 && ec.readDouble(edv)) it.energyGain = (uint8_t)edv;
                    else if (std::strcmp(ek, "mana") == 0 && ec.readDouble(edv)) it.manaGain = (uint8_t)edv;
                    else if (std::strcmp(ek, "satiety_ticks") == 0 && ec.readDouble(edv)) it.satietyTicks = (uint8_t)edv;
                    else if (std::strcmp(ek, "sick_chance") == 0 && ec.readDouble(edv)) it.sickChancePct = parseSickPct(edv);
                  });
                }
              });
              if (std::strlen(it.id) > 0) {
                if (std::strlen(it.name_de) == 0) copyStr(it.name_de, sizeof(it.name_de), it.name);
                rules.shopItems[rules.shopItemCount++] = it;
              }
            });
          } else if (std::strcmp(shk, "recipes") == 0) {
            rules.shopRecipeCount = 0;
            shc.eachArrayElement([&](int /*idx*/, EngineJsonCursor& rc) {
              if (rules.shopRecipeCount >= 6) return;
              EngineShopRecipe sr = {};
              rc.eachObjectEntry([&](const char* rk, EngineJsonCursor& rfc) {
                double dv = 0;
                if (std::strcmp(rk, "id") == 0) rfc.readString(sr.id, sizeof(sr.id));
                else if (std::strcmp(rk, "name") == 0) rfc.readString(sr.name, sizeof(sr.name));
                else if (std::strcmp(rk, "name_de") == 0) rfc.readString(sr.name_de, sizeof(sr.name_de));
                else if ((std::strcmp(rk, "price_coins") == 0 || std::strcmp(rk, "cost") == 0) && rfc.readDouble(dv)) sr.cost = (uint16_t)dv;
                else if (std::strcmp(rk, "min_level") == 0 && rfc.readDouble(dv)) sr.minLevel = (uint8_t)engineMax(1, (int)dv);
              });
              if (std::strlen(sr.id) > 0) {
                copyStr(sr.recipeId, sizeof(sr.recipeId), sr.id);
                if (std::strlen(sr.name_de) == 0) copyStr(sr.name_de, sizeof(sr.name_de), sr.name);
                rules.shopRecipes[rules.shopRecipeCount++] = sr;
              }
            });
          } else if (std::strcmp(shk, "spells") == 0) {
            rules.shopSpellCount = 0;
            shc.eachArrayElement([&](int /*idx*/, EngineJsonCursor& sc) {
              if (rules.shopSpellCount >= 6) return;
              EngineShopSpell ss = {};
              sc.eachObjectEntry([&](const char* sk, EngineJsonCursor& sfc) {
                double dv = 0;
                if (std::strcmp(sk, "id") == 0) sfc.readString(ss.id, sizeof(ss.id));
                else if (std::strcmp(sk, "name") == 0) sfc.readString(ss.name, sizeof(ss.name));
                else if (std::strcmp(sk, "name_de") == 0) sfc.readString(ss.name_de, sizeof(ss.name_de));
                else if ((std::strcmp(sk, "price_coins") == 0 || std::strcmp(sk, "cost") == 0) && sfc.readDouble(dv)) ss.cost = (uint16_t)dv;
                else if (std::strcmp(sk, "min_level") == 0 && sfc.readDouble(dv)) ss.minLevel = (uint8_t)engineMax(1, (int)dv);
                else if (std::strcmp(sk, "mana_cost") == 0 && sfc.readDouble(dv)) ss.manaCost = (uint8_t)dv;
                else if (std::strcmp(sk, "effects") == 0) {
                  sfc.eachObjectEntry([&](const char* ek, EngineJsonCursor& ec) {
                    double edv = 0;
                    if (std::strcmp(ek, "health") == 0 && ec.readDouble(edv)) ss.healPower = (uint8_t)edv;
                  });
                }
              });
              if (std::strlen(ss.id) > 0) {
                if (std::strlen(ss.name_de) == 0) copyStr(ss.name_de, sizeof(ss.name_de), ss.name);
                rules.shopSpells[rules.shopSpellCount++] = ss;
              }
            });
          }
        });
      }
    });

    if (ok) {
      checkEvolution();
      if (onEvent) onEvent("CONFIG_LOADED", nullptr);
    }
    return ok;
  }

  void loadConfigFromText(const char* cfgText) {
    if (!cfgText) return;
    const char* p = cfgText;
    while (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n') p++;
    if (*p == '{') {
      loadRulesJson(p);
      return;
    }

    char line[160];
    while (*p != '\0') {
      const char* nl = std::strchr(p, '\n');
      size_t lineLen = nl ? (size_t)(nl - p) : std::strlen(p);
      size_t copyLen = (lineLen < sizeof(line) - 1) ? lineLen : (sizeof(line) - 1);
      std::memcpy(line, p, copyLen);
      line[copyLen] = '\0';
      p = nl ? (nl + 1) : (p + lineLen);

      char* s = line;
      while (*s == ' ' || *s == '\t' || *s == '\r') s++;
      if (*s == '\0' || *s == '#' || *s == '[' || *s == '{' || *s == '}') {
        continue;
      }

      char* sep = std::strchr(s, '=');
      if (!sep) sep = std::strchr(s, ':');
      if (sep) {
        char key[64];
        char val[64];
        size_t kLen = (size_t)(sep - s);
        if (kLen >= sizeof(key)) kLen = sizeof(key) - 1;
        std::memcpy(key, s, kLen);
        key[kLen] = '\0';
        std::strncpy(val, sep + 1, sizeof(val) - 1);
        val[sizeof(val) - 1] = '\0';

        trimAndStrip(key);
        trimAndStrip(val);

        if (val[0] == '{' || val[0] == '[' || val[0] == '\0') {
          continue;
        }
        int ival = (int)std::strtol(val, nullptr, 10);

        if (std::strcmp(key, "version") == 0) copyStr(rules.version, sizeof(rules.version), val);
        else if (std::strcmp(key, "hunger_decay") == 0) { rules.hungerDecay = (uint8_t)ival; rules.hungerDecayFloat = (float)ival; }
        else if (std::strcmp(key, "energy_decay") == 0) { rules.energyDecay = (uint8_t)ival; rules.energyDecayFloat = (float)ival; }
        else if (std::strcmp(key, "happiness_decay") == 0) { rules.happinessDecay = (uint8_t)ival; rules.happinessDecayFloat = (float)ival; }
        else if (std::strcmp(key, "starve_threshold") == 0 || std::strcmp(key, "starveThreshold") == 0) rules.starveThreshold = (uint8_t)ival;
        else if (std::strcmp(key, "health_starve_decay") == 0 || std::strcmp(key, "starveDecay") == 0) { rules.healthStarveDecay = (uint8_t)ival; rules.healthStarveDecayFloat = (float)ival; }
        else if (std::strcmp(key, "natural_mana_regen") == 0 || std::strcmp(key, "naturalManaRegen") == 0) { rules.naturalManaRegen = (uint8_t)ival; rules.manaRegenFloat = (float)ival; }
        else if (std::strcmp(key, "egg_warmth_required") == 0 || std::strcmp(key, "warmthRequired") == 0) rules.eggWarmthRequired = (uint8_t)ival;
        else if (std::strcmp(key, "youth_min_level") == 0 || std::strcmp(key, "youthMinLevel") == 0) rules.youthMinLevel = (uint8_t)ival;
        else if (std::strcmp(key, "adult_min_level") == 0 || std::strcmp(key, "adultMinLevel") == 0) rules.adultMinLevel = (uint8_t)ival;
        else if (std::strcmp(key, "elder_min_level") == 0 || std::strcmp(key, "elderMinLevel") == 0) rules.elderMinLevel = (uint8_t)ival;
        else if (std::strcmp(key, "base_xp_requirement") == 0 || std::strcmp(key, "baseXpRequirement") == 0) rules.baseXpRequirement = (uint16_t)ival;
        else if (std::strcmp(key, "treat_send_hunger_gain") == 0) rules.treatHungerGain = (uint8_t)ival;
        else if (std::strcmp(key, "treat_send_happiness_gain") == 0) rules.treatHappinessGain = (uint8_t)ival;
      }
    }

    if (onEvent) onEvent("CONFIG_LOADED", nullptr);
  }

#if defined(ARDUINO) || defined(MOCK_ARDUINO_H)
  void loadConfigFromText(const String& cfgText) {
    loadConfigFromText(cfgText.c_str());
  }
#endif

  // --------------------------------------------------------------------------
  // Fixed-Array Inventory Management (0 Heap Allocation)
  // --------------------------------------------------------------------------
  int findInventoryIndex(const char* itemId) const {
    for (uint8_t i = 0; i < inventoryCount; i++) {
      if (std::strcmp(inventory[i].id, itemId) == 0 && inventory[i].quantity > 0) {
        return (int)i;
      }
    }
    return -1;
  }

  uint8_t getItemQuantity(const char* itemId) const {
    int idx = findInventoryIndex(itemId);
    return (idx >= 0) ? inventory[idx].quantity : 0;
  }

  bool addInventoryItem(
    const char* id,
    const char* itemName,
    const char* itemNameDe,
    uint8_t category,
    uint8_t qty,
    uint8_t hungerGain = 0,
    uint8_t healthGain = 0,
    uint8_t happyGain = 0,
    uint8_t energyGain = 0,
    uint8_t satTicks = 0,
    uint8_t sickPct = 0,
    bool cureSick = false
  ) {
    if (qty == 0) return false;
    int existingIdx = findInventoryIndex(id);
    if (existingIdx >= 0) {
      uint16_t sum = (uint16_t)inventory[existingIdx].quantity + (uint16_t)qty;
      inventory[existingIdx].quantity = (sum > 99) ? 99 : (uint8_t)sum;
      return true;
    }

    if (inventoryCount >= MAX_INVENTORY_SLOTS) return false;

    EngineInventorySlot& slot = inventory[inventoryCount];
    copyStr(slot.id, sizeof(slot.id), id);
    copyStr(slot.name, sizeof(slot.name), itemName);
    copyStr(slot.name_de, sizeof(slot.name_de), itemNameDe);
    slot.category = category;
    slot.quantity = qty;
    slot.hungerGain = hungerGain;
    slot.healthGain = healthGain;
    slot.happyGain = happyGain;
    slot.energyGain = energyGain;
    slot.satietyTicks = satTicks;
    slot.sickChancePct = sickPct;
    slot.cureSickness = cureSick;
    inventoryCount++;
    return true;
  }

  bool addKnownItemById(const char* itemId, uint8_t qty = 1) {
    if (!itemId || qty == 0) return false;

    // 1. Dynamic lookup from parsed rules.json foods[]
    for (uint8_t i = 0; i < rules.foodDefCount; i++) {
      if (std::strcmp(rules.foodDefs[i].id, itemId) == 0) {
        const EngineFoodDef& fd = rules.foodDefs[i];
        return addInventoryItem(fd.id, fd.name, fd.name_de, 0, qty,
                                fd.hungerGain, fd.healthGain, fd.happyGain,
                                fd.energyGain, fd.satietyTicks, fd.sickChancePct, fd.cureSickness);
      }
    }

    // 2. Dynamic lookup from parsed rules.json shop.items[]
    for (uint8_t i = 0; i < rules.shopItemCount; i++) {
      if (std::strcmp(rules.shopItems[i].id, itemId) == 0) {
        const EngineShopItem& sh = rules.shopItems[i];
        return addInventoryItem(sh.id, sh.name, sh.name_de, sh.category, qty,
                                sh.hungerGain, sh.healthGain, sh.happyGain,
                                sh.energyGain, sh.satietyTicks, sh.sickChancePct, sh.cureSickness);
      }
    }

    // 3. Static fallback table
    if (std::strcmp(itemId, "food_apple") == 0) {
      return addInventoryItem("food_apple", "Crisp Apple", "Frischer Apfel", 0, qty, 12, 2, 5, 0, 0, 0, false);
    }
    if (std::strcmp(itemId, "food_snack") == 0) {
      return addInventoryItem("food_snack", "Pixel Snack", "Pixel-Snack", 0, qty, 25, 0, 15, 5, 0, 30, false);
    }
    if (std::strcmp(itemId, "food_herb_salad") == 0) {
      return addInventoryItem("food_herb_salad", "Herb Salad", "Kraeuter-Salat", 0, qty, 35, 10, 10, 10, 4, 0, false);
    }
    if (std::strcmp(itemId, "food_berry_pie") == 0) {
      return addInventoryItem("food_berry_pie", "Berry Tart", "Beeren-Tarte", 0, qty, 45, 15, 25, 5, 5, 0, false);
    }
    if (std::strcmp(itemId, "food_sun_jerky") == 0) {
      return addInventoryItem("food_sun_jerky", "Sun Jerky", "Doerrfleisch", 0, qty, 55, 10, 15, 20, 6, 0, false);
    }
    if (std::strcmp(itemId, "food_kelp_stew") == 0) {
      return addInventoryItem("food_kelp_stew", "Kelp Stew", "Pilzeintopf", 0, qty, 75, 30, 30, 15, 8, 0, false);
    }
    if (std::strcmp(itemId, "food_ambrosia") == 0) {
      return addInventoryItem("food_ambrosia", "Ambrosia", "Ambrosia", 0, qty, 100, 100, 100, 100, 12, 0, false);
    }
    if (std::strcmp(itemId, "item_salve") == 0) {
      return addInventoryItem("item_salve", "Healing Salve", "Heilsalbe", 2, qty, 0, 40, 0, 0, 0, 0, true);
    }
    if (std::strcmp(itemId, "item_energy_elixir") == 0) {
      return addInventoryItem("item_energy_elixir", "Energy Tonic", "Vital-Tonikum", 2, qty, 0, 0, 0, 60, 0, 0, false);
    }
    if (std::strcmp(itemId, "mat_herb") == 0) {
      return addInventoryItem("mat_herb", "Meadow Herb", "Wiesenkraeuter", 1, qty);
    }
    if (std::strcmp(itemId, "mat_meat") == 0) {
      return addInventoryItem("mat_meat", "Raw Meat", "Rohes Fleisch", 1, qty);
    }
    if (std::strcmp(itemId, "mat_mushroom") == 0) {
      return addInventoryItem("mat_mushroom", "Forest Mushroom", "Waldpilz", 1, qty);
    }
    if (std::strcmp(itemId, "mat_sunstone") == 0) {
      return addInventoryItem("mat_sunstone", "Sun Salt", "Sonnensalz", 1, qty);
    }
    if (std::strcmp(itemId, "mat_kelp") == 0) {
      return addInventoryItem("mat_kelp", "Lagoon Kelp", "Lagunentang", 1, qty);
    }
    return false;
  }

  bool consumeInventoryItem(const char* itemId, uint8_t qty = 1) {
    int idx = findInventoryIndex(itemId);
    if (idx < 0 || inventory[idx].quantity < qty) return false;
    inventory[idx].quantity -= qty;
    if (inventory[idx].quantity == 0) {
      for (uint8_t j = (uint8_t)idx; j + 1 < inventoryCount; j++) {
        inventory[j] = inventory[j + 1];
      }
      inventoryCount--;
    }
    return true;
  }

  uint8_t getAvailableFoods(uint8_t* outIndices, uint8_t maxOut) const {
    uint8_t count = 0;
    for (uint8_t i = 0; i < inventoryCount && count < maxOut; i++) {
      if (inventory[i].category == 0 && inventory[i].quantity > 0) {
        outIndices[count++] = i;
      }
    }
    return count;
  }

  uint8_t getAvailablePotions(uint8_t* outIndices, uint8_t maxOut) const {
    uint8_t count = 0;
    for (uint8_t i = 0; i < inventoryCount && count < maxOut; i++) {
      if (inventory[i].category == 2 && inventory[i].quantity > 0) {
        outIndices[count++] = i;
      }
    }
    return count;
  }

  // --------------------------------------------------------------------------
  // Simulation Loop Tick (Honors Satiety Shield, Biomes & Fractional Decay)
  // --------------------------------------------------------------------------
  void tick() {
    if (std::strcmp(phase, "Egg") == 0) return;

    const EngineBiomeRule* b = findBiomeRule(biome);
    float foodMult  = b ? b->foodDecayMult : 1.0f;
    float enerMult  = b ? b->energyDecayMult : 1.0f;
    float happyMult = b ? b->happinessDecayMult : 1.0f;

    if (satietyTicks > 0) {
      satietyTicks--;
    } else if (stats.hunger > 0) {
      hungerAcc += rules.hungerDecayFloat * foodMult;
      int drop = (int)hungerAcc;
      if (drop > 0) {
        hungerAcc -= (float)drop;
        stats.hunger = (uint8_t)engineMax(0, (int)stats.hunger - drop);
      }
    }

    if (stats.energy > 0) {
      energyAcc += rules.energyDecayFloat * enerMult;
      int drop = (int)energyAcc;
      if (drop > 0) {
        energyAcc -= (float)drop;
        stats.energy = (uint8_t)engineMax(0, (int)stats.energy - drop);
      }
    }

    if (stats.happiness > 0) {
      happinessAcc += rules.happinessDecayFloat * happyMult;
      int drop = (int)happinessAcc;
      if (drop > 0) {
        happinessAcc -= (float)drop;
        stats.happiness = (uint8_t)engineMax(0, (int)stats.happiness - drop);
      }
    }

    if (stats.hunger < rules.starveThreshold && stats.health > 0) {
      starveAcc += rules.healthStarveDecayFloat;
      int drop = (int)starveAcc;
      if (drop > 0) {
        starveAcc -= (float)drop;
        stats.health = (uint8_t)engineMax(0, (int)stats.health - drop);
      }
    }

    if (isSick && stats.health > 0) {
      sickAcc += rules.sickDecayFloat;
      int drop = (int)sickAcc;
      if (drop > 0) {
        sickAcc -= (float)drop;
        stats.health = (uint8_t)engineMax(0, (int)stats.health - drop);
      }
    }

    uint8_t maxMana = getEffectiveMaxMana();
    if (stats.mana < maxMana) {
      manaAcc += rules.manaRegenFloat;
      int gain = (int)manaAcc;
      if (gain > 0) {
        manaAcc -= (float)gain;
        stats.mana = (uint8_t)engineMin((int)maxMana, (int)stats.mana + gain);
      }
    }
  }

  // --------------------------------------------------------------------------
  // Inventory-Gated Feeding, Potions, Dynamic Cooking & Dynamic Spells
  // --------------------------------------------------------------------------
  FeedResult feedWithItem(const char* itemId) {
    FeedResult res;
    int idx = findInventoryIndex(itemId);
    if (idx < 0 || inventory[idx].category != 0 || inventory[idx].quantity == 0) {
      return res;
    }

    EngineInventorySlot copySlot = inventory[idx];
    if (!consumeInventoryItem(itemId, 1)) {
      return res;
    }

    bool rolledSick = false;
    if (copySlot.sickChancePct > 0) {
      if (engineRandom(0, 100) < (long)copySlot.sickChancePct) {
        rolledSick = true;
      }
    }

    stats.hunger    = (uint8_t)engineMin((int)rules.maxStatValue, (int)stats.hunger + (int)copySlot.hungerGain);
    stats.happiness = (uint8_t)engineMin((int)rules.maxStatValue, (int)stats.happiness + (int)copySlot.happyGain);
    stats.energy    = (uint8_t)engineMin((int)rules.maxStatValue, (int)stats.energy + (int)copySlot.energyGain);

    int hp = (int)stats.health + (int)copySlot.healthGain - (rolledSick ? 15 : 0);
    stats.health = (uint8_t)engineClamp(hp, 0, (int)rules.maxStatValue);

    if (copySlot.satietyTicks > satietyTicks) {
      satietyTicks = copySlot.satietyTicks;
    }
    if (rolledSick) {
      isSick = true;
    }

    addXP(15);
    recordAction("feed", 1);

    res.success = true;
    res.becameSick = rolledSick;
    res.hungerGain = copySlot.hungerGain;
    res.satietyGained = copySlot.satietyTicks;
    return res;
  }

  bool usePotionItem(const char* itemId) {
    int idx = findInventoryIndex(itemId);
    if (idx < 0 || inventory[idx].category != 2 || inventory[idx].quantity == 0) {
      return false;
    }
    EngineInventorySlot copySlot = inventory[idx];
    if (!consumeInventoryItem(itemId, 1)) return false;

    stats.health = (uint8_t)engineMin((int)rules.maxStatValue, (int)stats.health + (int)copySlot.healthGain);
    stats.energy = (uint8_t)engineMin((int)rules.maxStatValue, (int)stats.energy + (int)copySlot.energyGain);
    if (std::strcmp(itemId, "item_energy_elixir") == 0) {
      uint8_t maxMana = getEffectiveMaxMana();
      stats.mana = (uint8_t)engineMin((int)maxMana, (int)stats.mana + 15);
    }
    if (copySlot.cureSickness) {
      isSick = false;
    }
    return true;
  }

  // Dynamic rule-driven crafting into inventory
  bool craftRecipe(uint8_t recipeIdx) {
    if (recipeIdx >= rules.recipeCount) return false;
    const EngineRecipe& r = rules.recipes[recipeIdx];
    if (level < r.minLevel || r.ingredientCount == 0) return false;

    for (uint8_t i = 0; i < r.ingredientCount; i++) {
      if (getItemQuantity(r.ingredients[i].itemId) < r.ingredients[i].count) {
        return false;
      }
    }
    for (uint8_t i = 0; i < r.ingredientCount; i++) {
      consumeInventoryItem(r.ingredients[i].itemId, r.ingredients[i].count);
    }

    const char* outId = (std::strlen(r.resultItemId) > 0) ? r.resultItemId : r.id;
    if (!addKnownItemById(outId, 1)) {
      addInventoryItem(outId, r.name, r.name_de, 0, 1,
                       r.hungerGain, r.healthGain, r.happyGain, r.energyGain, r.satietyTicks, 0, false);
    }
    addXP(r.xpReward);
    return true;
  }

  bool craftRecipeById(const char* recipeId) {
    if (!recipeId) return false;
    for (uint8_t i = 0; i < rules.recipeCount; i++) {
      if (std::strcmp(rules.recipes[i].id, recipeId) == 0) {
        return craftRecipe(i);
      }
    }
    return false;
  }

  // Dynamic rule-driven spell execution
  bool castSpell(uint8_t spellIdx) {
    if (spellIdx >= rules.spellCount) return false;
    const EngineSpell& sp = rules.spells[spellIdx];
    if (level < sp.minLevel || stats.mana < sp.manaCost) return false;

    stats.mana -= sp.manaCost;
    if (sp.target == 0 || sp.target == 2) {
      if (sp.cureSickness) isSick = false;
      if (sp.cleanPoop) stats.hygiene = rules.maxStatValue;
      if (sp.healthGain > 0) {
        stats.health = (uint8_t)engineMin((int)rules.maxStatValue, (int)stats.health + (int)sp.healthGain);
      }
      if (sp.happyGain > 0) {
        stats.happiness = (uint8_t)engineMin((int)rules.maxStatValue, (int)stats.happiness + (int)sp.happyGain);
      }
      if (sp.hungerGain > 0) {
        stats.hunger = (uint8_t)engineMin((int)rules.maxStatValue, (int)stats.hunger + (int)sp.hungerGain);
      }
      if (sp.satietyTicks > satietyTicks) {
        satietyTicks = sp.satietyTicks;
      }
    }
    addXP(sp.xpReward);
    recordAction("magic", 1);
    return true;
  }

  // --------------------------------------------------------------------------
  // Daily Missions
  // --------------------------------------------------------------------------
  void recordAction(const char* actionType, uint8_t count = 1) {
    for (uint8_t i = 0; i < missionCount; i++) {
      if (std::strcmp(missions[i].actionType, actionType) == 0 && !missions[i].completed) {
        missions[i].currentCount += count;
        if (missions[i].currentCount >= missions[i].targetCount) {
          missions[i].currentCount = missions[i].targetCount;
          missions[i].completed = true;
          if (onEvent) onEvent("MISSION_COMPLETE", (void*)&missions[i]);
        } else {
          if (onEvent) onEvent("MISSION_PROGRESS", (void*)&missions[i]);
        }
      }
    }
  }

  bool claimMission(const char* missionId) {
    for (uint8_t i = 0; i < missionCount; i++) {
      if (std::strcmp(missions[i].id, missionId) == 0 && missions[i].completed && !missions[i].claimed) {
        missions[i].claimed = true;
        coins += missions[i].coinReward;
        addXP(missions[i].xpReward);
        if (onEvent) onEvent("MISSION_CLAIMED", (void*)&missions[i]);
        return true;
      }
    }
    return false;
  }

  // --------------------------------------------------------------------------
  // Daily Login Streak Bonus
  // --------------------------------------------------------------------------
  uint16_t getStreakBonusCoins() const {
    uint8_t cycleDay = ((consecutiveLoginDays - 1) % 7) + 1;
    switch (cycleDay) {
      case 1: return 15;
      case 2: return 25;
      case 3: return 40;
      case 4: return 60;
      case 5: return 85;
      case 6: return 115;
      default: {
        uint16_t extra = (uint16_t)engineMin((int)100, (int)((consecutiveLoginDays - 1) / 7) * 15);
        return 150 + extra;
      }
    }
  }

  uint16_t getStreakBonusXP() const {
    uint8_t cycleDay = ((consecutiveLoginDays - 1) % 7) + 1;
    switch (cycleDay) {
      case 1: return 10;
      case 2: return 15;
      case 3: return 25;
      case 4: return 35;
      case 5: return 50;
      case 6: return 65;
      default: return 100;
    }
  }

  bool canClaimStreakBonus() const {
    return !streakClaimedToday;
  }

  bool claimDailyLoginStreakBonus() {
    if (streakClaimedToday) return false;
    uint16_t cReward = getStreakBonusCoins();
    uint16_t xReward = getStreakBonusXP();
    coins += cReward;
    addXP(xReward);
    streakClaimedToday = true;
    recordAction("login_streak", 1);
    if (onEvent) onEvent("STREAK_BONUS_CLAIMED", (void*)&cReward);
    return true;
  }

  // --------------------------------------------------------------------------
  // Shop & Marketplace (Adds purchased items into inventory!)
  // --------------------------------------------------------------------------
  bool purchaseItem(const char* itemId) {
    for (uint8_t i = 0; i < rules.shopItemCount; i++) {
      if (std::strcmp(rules.shopItems[i].id, itemId) == 0) {
        EngineShopItem& s = rules.shopItems[i];
        if (coins >= s.cost && level >= s.minLevel) {
          bool added = addInventoryItem(
            s.id, s.name, s.name_de, s.category, 1,
            s.hungerGain, s.healthGain, s.happyGain, s.energyGain,
            s.satietyTicks, s.sickChancePct, s.cureSickness
          );
          if (!added) return false;
          coins -= s.cost;
          if (onEvent) onEvent("SHOP_PURCHASE_ITEM", (void*)&s);
          return true;
        }
        return false;
      }
    }
    return false;
  }

  bool purchaseRecipe(const char* recipeId) {
    for (uint8_t i = 0; i < rules.shopRecipeCount; i++) {
      if (std::strcmp(rules.shopRecipes[i].id, recipeId) == 0) {
        if (coins >= rules.shopRecipes[i].cost && level >= rules.shopRecipes[i].minLevel) {
          coins -= rules.shopRecipes[i].cost;
          addXP(30);
          if (onEvent) onEvent("SHOP_PURCHASE_RECIPE", (void*)&rules.shopRecipes[i]);
          return true;
        }
        return false;
      }
    }
    return false;
  }

  bool purchaseSpell(const char* spellId) {
    for (uint8_t i = 0; i < rules.shopSpellCount; i++) {
      if (std::strcmp(rules.shopSpells[i].id, spellId) == 0) {
        if (coins >= rules.shopSpells[i].cost && level >= rules.shopSpells[i].minLevel) {
          coins -= rules.shopSpells[i].cost;
          addXP(45);
          if (onEvent) onEvent("SHOP_PURCHASE_SPELL", (void*)&rules.shopSpells[i]);
          return true;
        }
        return false;
      }
    }
    return false;
  }

  // --------------------------------------------------------------------------
  // Core Interactions
  // --------------------------------------------------------------------------
  void feed(uint8_t amount = 25) {
    stats.hunger = (uint8_t)engineMin((int)rules.maxStatValue, (int)stats.hunger + (int)amount);
    addXP(15);
    recordAction("feed", 1);
  }

  void play(uint8_t happy = 20, uint8_t cost = 10) {
    stats.happiness = (uint8_t)engineMin((int)rules.maxStatValue, (int)stats.happiness + (int)happy);
    stats.energy = (uint8_t)engineMax(0, (int)stats.energy - (int)cost);
    addXP(20);
    recordAction("play", 1);
  }

  void clean() {
    stats.hygiene = rules.maxStatValue;
    stats.happiness = (uint8_t)engineMin((int)rules.maxStatValue, (int)stats.happiness + 10);
    addXP(10);
    recordAction("bath", 1);
  }

  bool warmEgg(uint8_t amount = 25) {
    if (std::strcmp(phase, "Egg") != 0) return false;
    const EngineBiomeRule* b = findBiomeRule(biome);
    uint8_t scaled = (b && b->eggHatchSpeed > 1.0f) ? (uint8_t)std::llround((float)amount * b->eggHatchSpeed) : amount;
    eggWarmth = (uint8_t)engineMin((int)rules.eggWarmthRequired, (int)eggWarmth + (int)scaled);
    if (eggWarmth >= rules.eggWarmthRequired) {
      std::strcpy(phase, "Baby");
      stats.health = rules.maxStatValue;
      stats.hunger = rules.maxStatValue;
      stats.happiness = rules.maxStatValue;
      if (onEvent) onEvent("EGG_HATCHED", nullptr);
      return true;
    }
    return false;
  }

  void addXP(uint16_t amount) {
    xp += amount;
    bool leveled = false;
    while (xp >= xpNext) {
      xp -= xpNext;
      level++;
      xpNext = (uint16_t)(rules.baseXpRequirement * std::pow(rules.xpLevelMultiplier, level - 1));
      leveled = true;
    }

    if (leveled) {
      stats.health = rules.maxStatValue;
      stats.energy = rules.maxStatValue;
      checkEvolution();
      if (onEvent) onEvent("LEVEL_UP", (void*)&level);
    }
  }

  void checkEvolution() {
    if (std::strcmp(phase, "Egg") == 0) return;
    if (level >= rules.elderMinLevel) std::strcpy(phase, "Elder");
    else if (level >= rules.adultMinLevel) std::strcpy(phase, "Adult");
    else if (level >= rules.youthMinLevel) std::strcpy(phase, "Youth");
    else std::strcpy(phase, "Baby");
  }

  void receiveRemoteTreat() {
    stats.hunger = (uint8_t)engineMin((int)rules.maxStatValue, (int)stats.hunger + (int)rules.treatHungerGain);
    stats.happiness = (uint8_t)engineMin((int)rules.maxStatValue, (int)stats.happiness + (int)rules.treatHappinessGain);
    if (onEvent) onEvent("TREAT_RECEIVED", nullptr);
  }
};

#endif // TAMAGOTCHI_CORE_ENGINE_H
