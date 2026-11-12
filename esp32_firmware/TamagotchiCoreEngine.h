/**
 * ============================================================================
 * TamagotchiCoreEngine.h
 * Platform-Independent, UI-Agnostic C++ Game Runtime Engine
 * ============================================================================
 * Decoupled from screen resolution, display libraries (TFT_eSPI / LovyanGFX),
 * and hardware pins. Can be ported to any microcontroller (ESP32, RP2040, STM32)
 * or compiled inside Android NDK / desktop C++.
 *
 * Reads human-readable rules configuration (tamagotchi_rules.cfg) at runtime.
 * ============================================================================
 */
#ifndef TAMAGOTCHI_CORE_ENGINE_H
#define TAMAGOTCHI_CORE_ENGINE_H

#include <Arduino.h>

struct EngineStats {
  uint8_t health    = 100;
  uint8_t hunger    = 85;
  uint8_t energy    = 90;
  uint8_t hygiene   = 95;
  uint8_t happiness = 90;
  uint8_t mana      = 50;
};

struct EngineRecipe {
  char id[16]        = "";
  char name[24]      = "";
  char name_de[24]   = "";
  uint8_t minLevel   = 1;
  uint8_t hungerGain = 25;
  uint16_t xpReward  = 35;
};

struct EngineDailyMission {
  char id[16]         = "";
  char name[32]       = "";
  char actionType[16] = ""; // feed, play, bath, login_streak, send_treat
  uint8_t targetCount = 1;
  uint8_t currentCount = 0;
  uint16_t coinReward = 20;
  uint16_t xpReward   = 30;
  bool completed      = false;
  bool claimed        = false;
};

struct EngineShopItem {
  char id[16]        = "";
  char name[24]      = "";
  char name_de[24]   = "";
  uint16_t cost      = 10;
  uint8_t minLevel   = 1;
  uint8_t hungerGain = 20;
  uint8_t happyGain  = 10;
  uint8_t healthGain = 0;
};

struct EngineShopRecipe {
  char id[16]        = "";
  char name[24]      = "";
  char name_de[24]   = "";
  uint16_t cost      = 50;
  uint8_t minLevel   = 2;
  char recipeId[16]  = "";
};

struct EngineShopSpell {
  char id[16]        = "";
  char name[24]      = "";
  char name_de[24]   = "";
  uint16_t cost      = 75;
  uint8_t minLevel   = 3;
  uint8_t manaCost   = 25;
  uint8_t healPower  = 40;
};

struct EngineRules {
  uint8_t maxStatValue          = 100;
  uint8_t hungerDecay           = 1;
  uint8_t energyDecay           = 1;
  uint8_t happinessDecay        = 1;
  uint8_t starveThreshold       = 20;
  uint8_t healthStarveDecay     = 1;
  uint8_t naturalManaRegen      = 2;
  uint8_t eggWarmthRequired     = 100;
  uint8_t babyMinLevel          = 1;
  uint8_t youthMinLevel         = 4;
  uint8_t adultMinLevel         = 8;
  uint8_t elderMinLevel         = 15;
  uint16_t baseXpRequirement    = 100;
  float xpLevelMultiplier       = 1.30f;
  uint8_t treatHungerGain       = 25;
  uint8_t treatHappinessGain    = 20;
  uint8_t treatXpReward         = 30;
  uint8_t coopQuestCoinsReward  = 40;
  EngineRecipe recipes[8];
  uint8_t recipeCount           = 0;
  EngineDailyMission missions[6];
  uint8_t missionCount          = 0;
  EngineShopItem shopItems[6];
  uint8_t shopItemCount         = 0;
  EngineShopRecipe shopRecipes[4];
  uint8_t shopRecipeCount       = 0;
  EngineShopSpell shopSpells[4];
  uint8_t shopSpellCount        = 0;
};

class TamagotchiCoreEngine {
public:
  EngineRules rules;
  EngineStats stats;

  char name[16]    = "Leafkin";
  char biome[12]   = "grassland";
  char phase[10]   = "Egg";
  uint8_t level    = 1;
  uint16_t xp      = 0;
  uint16_t xpNext  = 100;
  uint16_t coins   = 50;
  uint8_t eggWarmth = 20;

  EngineDailyMission missions[6];
  uint8_t missionCount = 0;
  uint16_t consecutiveLoginDays = 1;
  bool streakClaimedToday = false;


  // Callback types for decoupled hardware event notifications
  typedef void (*EventCallback)(const char* eventName, void* data);
  EventCallback onEvent = nullptr;

  TamagotchiCoreEngine() {
    // Initialize default daily missions
    missionCount = 4;
    strcpy(missions[0].id, "feed_3");
    strcpy(missions[0].name, "Feed Pet 3 times");
    strcpy(missions[0].actionType, "feed");
    missions[0].targetCount = 3;
    missions[0].coinReward = 20;
    missions[0].xpReward = 30;

    strcpy(missions[1].id, "play_2");
    strcpy(missions[1].name, "Play Mini-Game 2 times");
    strcpy(missions[1].actionType, "play");
    missions[1].targetCount = 2;
    missions[1].coinReward = 25;
    missions[1].xpReward = 35;

    strcpy(missions[2].id, "bath_1");
    strcpy(missions[2].name, "Give Bath 1 time");
    strcpy(missions[2].actionType, "bath");
    missions[2].targetCount = 1;
    missions[2].coinReward = 15;
    missions[2].xpReward = 20;

    strcpy(missions[3].id, "treat_1");
    strcpy(missions[3].name, "Send Friend a Treat");
    strcpy(missions[3].actionType, "send_treat");
    missions[3].targetCount = 1;
    missions[3].coinReward = 30;
    missions[3].xpReward = 40;

    // Initialize default shop catalog
    rules.shopItemCount = 3;
    strcpy(rules.shopItems[0].id, "star_cookie");
    strcpy(rules.shopItems[0].name, "Star Cookie");
    strcpy(rules.shopItems[0].name_de, "Sternenkeks");
    rules.shopItems[0].cost = 15;
    rules.shopItems[0].minLevel = 1;
    rules.shopItems[0].hungerGain = 30;
    rules.shopItems[0].happyGain = 15;
    rules.shopItems[0].healthGain = 0;

    strcpy(rules.shopItems[1].id, "super_potion");
    strcpy(rules.shopItems[1].name, "Super Potion");
    strcpy(rules.shopItems[1].name_de, "Heiltrank");
    rules.shopItems[1].cost = 30;
    rules.shopItems[1].minLevel = 2;
    rules.shopItems[1].hungerGain = 0;
    rules.shopItems[1].happyGain = 20;
    rules.shopItems[1].healthGain = 50;

    strcpy(rules.shopItems[2].id, "golden_apple");
    strcpy(rules.shopItems[2].name, "Golden Apple");
    strcpy(rules.shopItems[2].name_de, "Goldapfel");
    rules.shopItems[2].cost = 60;
    rules.shopItems[2].minLevel = 4;
    rules.shopItems[2].hungerGain = 50;
    rules.shopItems[2].happyGain = 40;
    rules.shopItems[2].healthGain = 30;

    rules.shopRecipeCount = 2;
    strcpy(rules.shopRecipes[0].id, "recipe_pie");
    strcpy(rules.shopRecipes[0].name, "Berry Pie");
    strcpy(rules.shopRecipes[0].name_de, "Beerenkuchen");
    rules.shopRecipes[0].cost = 45;
    rules.shopRecipes[0].minLevel = 2;
    strcpy(rules.shopRecipes[0].recipeId, "berry_pie");

    strcpy(rules.shopRecipes[1].id, "recipe_stew");
    strcpy(rules.shopRecipes[1].name, "Dragon Stew");
    strcpy(rules.shopRecipes[1].name_de, "Dracheneintopf");
    rules.shopRecipes[1].cost = 80;
    rules.shopRecipes[1].minLevel = 5;
    strcpy(rules.shopRecipes[1].recipeId, "dragon_stew");

    rules.shopSpellCount = 2;
    strcpy(rules.shopSpells[0].id, "spell_spark");
    strcpy(rules.shopSpells[0].name, "Aura Spark");
    strcpy(rules.shopSpells[0].name_de, "Aura-Funke");
    rules.shopSpells[0].cost = 70;
    rules.shopSpells[0].minLevel = 3;
    rules.shopSpells[0].manaCost = 25;
    rules.shopSpells[0].healPower = 35;

    strcpy(rules.shopSpells[1].id, "spell_blessing");
    strcpy(rules.shopSpells[1].name, "Celestial Blessing");
    strcpy(rules.shopSpells[1].name_de, "Himmelssegen");
    rules.shopSpells[1].cost = 120;
    rules.shopSpells[1].minLevel = 6;
    rules.shopSpells[1].manaCost = 45;
    rules.shopSpells[1].healPower = 70;
  }

  // --------------------------------------------------------------------------
  // Runtime Config Loader (Reads human-readable .cfg file line by line)
  // --------------------------------------------------------------------------
  void loadConfigFromText(const String& cfgText) {
    int start = 0;
    while (start < cfgText.length()) {
      int end = cfgText.indexOf('\n', start);
      if (end == -1) end = cfgText.length();
      String line = cfgText.substring(start, end);
      line.trim();
      start = end + 1;

      if (line.length() == 0 || line.startsWith("#") || line.startsWith("[")) {
        continue;
      }

      int eq = line.indexOf('=');
      if (eq != -1) {
        String key = line.substring(0, eq);
        String val = line.substring(eq + 1);
        key.trim();
        val.trim();
        val.replace("\"", "");

        // Parse rules dynamically
        if (key == "hunger_decay") rules.hungerDecay = val.toInt();
        else if (key == "energy_decay") rules.energyDecay = val.toInt();
        else if (key == "happiness_decay") rules.happinessDecay = val.toInt();
        else if (key == "starve_threshold") rules.starveThreshold = val.toInt();
        else if (key == "health_starve_decay") rules.healthStarveDecay = val.toInt();
        else if (key == "natural_mana_regen") rules.naturalManaRegen = val.toInt();
        else if (key == "egg_warmth_required") rules.eggWarmthRequired = val.toInt();
        else if (key == "youth_min_level") rules.youthMinLevel = val.toInt();
        else if (key == "adult_min_level") rules.adultMinLevel = val.toInt();
        else if (key == "elder_min_level") rules.elderMinLevel = val.toInt();
        else if (key == "base_xp_requirement") rules.baseXpRequirement = val.toInt();
        else if (key == "treat_send_hunger_gain") rules.treatHungerGain = val.toInt();
        else if (key == "treat_send_happiness_gain") rules.treatHappinessGain = val.toInt();
      }
    }

    if (onEvent) onEvent("CONFIG_LOADED", nullptr);
  }

  // --------------------------------------------------------------------------
  // Simulation Loop Tick
  // --------------------------------------------------------------------------
  void tick() {
    if (strcmp(phase, "Egg") == 0) return; // Eggs do not decay

    if (stats.hunger > 0) stats.hunger = max(0, stats.hunger - rules.hungerDecay);
    if (stats.energy > 0) stats.energy = max(0, stats.energy - rules.energyDecay);
    if (stats.happiness > 0) stats.happiness = max(0, stats.happiness - rules.happinessDecay);

    // Starvation check
    if (stats.hunger < rules.starveThreshold && stats.health > 0) {
      stats.health = max(0, stats.health - rules.healthStarveDecay);
    }

    // Mana regeneration
    if (stats.mana < rules.maxStatValue) {
      stats.mana = min((int)rules.maxStatValue, stats.mana + rules.naturalManaRegen);
    }
  }

  // --------------------------------------------------------------------------
  // Daily Missions
  // --------------------------------------------------------------------------
  void recordAction(const char* actionType, uint8_t count = 1) {
    for (uint8_t i = 0; i < missionCount; i++) {
      if (strcmp(missions[i].actionType, actionType) == 0 && !missions[i].completed) {
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
      if (strcmp(missions[i].id, missionId) == 0 && missions[i].completed && !missions[i].claimed) {
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
  uint16_t getStreakBonusCoins() {
    uint8_t cycleDay = ((consecutiveLoginDays - 1) % 7) + 1;
    switch (cycleDay) {
      case 1: return 15;
      case 2: return 25;
      case 3: return 40;
      case 4: return 60;
      case 5: return 85;
      case 6: return 115;
      default: {
        uint16_t extra = min((int)100, (int)((consecutiveLoginDays - 1) / 7) * 15);
        return 150 + extra;
      }
    }
  }

  uint16_t getStreakBonusXP() {
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

  bool canClaimStreakBonus() {
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
  // Shop & Marketplace
  // --------------------------------------------------------------------------
  bool purchaseItem(const char* itemId) {
    for (uint8_t i = 0; i < rules.shopItemCount; i++) {
      if (strcmp(rules.shopItems[i].id, itemId) == 0) {
        if (coins >= rules.shopItems[i].cost && level >= rules.shopItems[i].minLevel) {
          coins -= rules.shopItems[i].cost;
          if (rules.shopItems[i].hungerGain > 0) feed(rules.shopItems[i].hungerGain);
          if (rules.shopItems[i].happyGain > 0) stats.happiness = min((int)rules.maxStatValue, stats.happiness + rules.shopItems[i].happyGain);
          if (rules.shopItems[i].healthGain > 0) stats.health = min((int)rules.maxStatValue, stats.health + rules.shopItems[i].healthGain);
          if (onEvent) onEvent("SHOP_PURCHASE_ITEM", (void*)&rules.shopItems[i]);
          return true;
        }
        return false;
      }
    }
    return false;
  }

  bool purchaseRecipe(const char* recipeId) {
    for (uint8_t i = 0; i < rules.shopRecipeCount; i++) {
      if (strcmp(rules.shopRecipes[i].id, recipeId) == 0) {
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
      if (strcmp(rules.shopSpells[i].id, spellId) == 0) {
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
  // Interactions
  // --------------------------------------------------------------------------
  void feed(uint8_t amount = 25) {
    stats.hunger = min((int)rules.maxStatValue, stats.hunger + amount);
    addXP(15);
    recordAction("feed", 1);
  }

  void play(uint8_t happy = 20, uint8_t cost = 10) {
    stats.happiness = min((int)rules.maxStatValue, stats.happiness + happy);
    stats.energy = max(0, stats.energy - cost);
    addXP(20);
    recordAction("play", 1);
  }

  void clean() {
    stats.hygiene = rules.maxStatValue;
    stats.happiness = min((int)rules.maxStatValue, stats.happiness + 10);
    addXP(10);
    recordAction("bath", 1);
  }

  bool warmEgg(uint8_t amount = 25) {
    if (strcmp(phase, "Egg") != 0) return false;
    eggWarmth = min((int)rules.eggWarmthRequired, eggWarmth + amount);
    if (eggWarmth >= rules.eggWarmthRequired) {
      strcpy(phase, "Baby");
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
      xpNext = rules.baseXpRequirement * pow(rules.xpLevelMultiplier, level - 1);
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
    if (strcmp(phase, "Egg") == 0) return;
    if (level >= rules.elderMinLevel) strcpy(phase, "Elder");
    else if (level >= rules.adultMinLevel) strcpy(phase, "Adult");
    else if (level >= rules.youthMinLevel) strcpy(phase, "Youth");
    else strcpy(phase, "Baby");
  }

  void receiveRemoteTreat() {
    stats.hunger = min((int)rules.maxStatValue, stats.hunger + rules.treatHungerGain);
    stats.happiness = min((int)rules.maxStatValue, stats.happiness + rules.treatHappinessGain);
    if (onEvent) onEvent("TREAT_RECEIVED", nullptr);
  }
};

#endif // TAMAGOTCHI_CORE_ENGINE_H
