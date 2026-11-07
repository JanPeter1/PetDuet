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

  // Callback types for decoupled hardware event notifications
  typedef void (*EventCallback)(const char* eventName, void* data);
  EventCallback onEvent = nullptr;

  TamagotchiCoreEngine() {}

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
  // Interactions
  // --------------------------------------------------------------------------
  void feed(uint8_t amount = 25) {
    stats.hunger = min((int)rules.maxStatValue, stats.hunger + amount);
    addXP(15);
  }

  void play(uint8_t happy = 20, uint8_t cost = 10) {
    stats.happiness = min((int)rules.maxStatValue, stats.happiness + happy);
    stats.energy = max(0, stats.energy - cost);
    addXP(20);
  }

  void clean() {
    stats.hygiene = rules.maxStatValue;
    stats.happiness = min((int)rules.maxStatValue, stats.happiness + 10);
    addXP(10);
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
