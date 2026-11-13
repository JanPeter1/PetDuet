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

#include <Arduino.h>

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
  uint8_t category     = 0;  // 0 = food, 1 = material, 2 = potion
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
  bool success       = false;
  bool becameSick    = false;
  uint8_t hungerGain = 0;
  uint8_t satietyGained = 0;
};

struct EngineRecipe {
  char id[16]        = "";
  char name[24]      = "";
  char name_de[24]   = "";
  uint8_t minLevel   = 1;
  uint8_t hungerGain = 25;
  uint8_t satietyTicks = 4;
  uint16_t xpReward  = 35;
};

struct EngineDailyMission {
  char id[16]          = "";
  char name[32]        = "";
  char name_de[32]     = "";
  char actionType[16]  = ""; // feed, play, bath, magic, send_treat
  uint8_t targetCount  = 1;
  uint8_t currentCount = 0;
  uint16_t coinReward  = 20;
  uint16_t xpReward    = 30;
  bool completed       = false;
  bool claimed         = false;
};

struct EngineShopItem {
  char id[20]          = "";
  char name[24]        = "";
  char name_de[24]     = "";
  uint16_t cost        = 10;
  uint8_t minLevel     = 1;
  uint8_t category     = 0; // 0 = food, 1 = material, 2 = potion
  uint8_t hungerGain   = 0;
  uint8_t happyGain    = 0;
  uint8_t healthGain   = 0;
  uint8_t energyGain   = 0;
  uint8_t satietyTicks = 0;
  uint8_t sickChancePct = 0;
  bool cureSickness    = false;
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
  EngineShopItem shopItems[8];
  uint8_t shopItemCount         = 0;
  EngineShopRecipe shopRecipes[4];
  uint8_t shopRecipeCount       = 0;
  EngineShopSpell shopSpells[4];
  uint8_t shopSpellCount        = 0;
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
    initDefaultShop();
  }

  void initStartingInventory() {
    inventoryCount = 0;
    // Start with simple food & herbs: 2x Apple, 1x Snack, 2x Meadow Herb
    addKnownItemById("food_apple", 2);
    addKnownItemById("food_snack", 1);
    addKnownItemById("mat_herb", 2);
  }

  void initDefaultMissions() {
    missionCount = 5;

    strncpy(missions[0].id, "feed_3", sizeof(missions[0].id) - 1);
    strncpy(missions[0].name, "Feed Pet 3 times", sizeof(missions[0].name) - 1);
    strncpy(missions[0].name_de, "3x Fuettern", sizeof(missions[0].name_de) - 1);
    strncpy(missions[0].actionType, "feed", sizeof(missions[0].actionType) - 1);
    missions[0].targetCount = 3;
    missions[0].coinReward = 20;
    missions[0].xpReward = 30;

    strncpy(missions[1].id, "play_2", sizeof(missions[1].id) - 1);
    strncpy(missions[1].name, "Play Mini-Game 2x", sizeof(missions[1].name) - 1);
    strncpy(missions[1].name_de, "2x Spielen", sizeof(missions[1].name_de) - 1);
    strncpy(missions[1].actionType, "play", sizeof(missions[1].actionType) - 1);
    missions[1].targetCount = 2;
    missions[1].coinReward = 25;
    missions[1].xpReward = 35;

    strncpy(missions[2].id, "bath_1", sizeof(missions[2].id) - 1);
    strncpy(missions[2].name, "Give Bath 1x", sizeof(missions[2].name) - 1);
    strncpy(missions[2].name_de, "1x Baden", sizeof(missions[2].name_de) - 1);
    strncpy(missions[2].actionType, "bath", sizeof(missions[2].actionType) - 1);
    missions[2].targetCount = 1;
    missions[2].coinReward = 15;
    missions[2].xpReward = 20;

    strncpy(missions[3].id, "treat_1", sizeof(missions[3].id) - 1);
    strncpy(missions[3].name, "Send Friend Treat", sizeof(missions[3].name) - 1);
    strncpy(missions[3].name_de, "Snack senden", sizeof(missions[3].name_de) - 1);
    strncpy(missions[3].actionType, "send_treat", sizeof(missions[3].actionType) - 1);
    missions[3].targetCount = 1;
    missions[3].coinReward = 30;
    missions[3].xpReward = 40;

    strncpy(missions[4].id, "magic_1", sizeof(missions[4].id) - 1);
    strncpy(missions[4].name, "Cast or Study Magic", sizeof(missions[4].name) - 1);
    strncpy(missions[4].name_de, "Magie wirken/forschen", sizeof(missions[4].name_de) - 1);
    strncpy(missions[4].actionType, "magic", sizeof(missions[4].actionType) - 1);
    missions[4].targetCount = 1;
    missions[4].coinReward = 25;
    missions[4].xpReward = 35;
  }

  void initDefaultShop() {
    rules.shopItemCount = 6;

    // 0: Crisp Apple
    strncpy(rules.shopItems[0].id, "food_apple", 19);
    strncpy(rules.shopItems[0].name, "Crisp Apple", 23);
    strncpy(rules.shopItems[0].name_de, "Frischer Apfel", 23);
    rules.shopItems[0].cost = 5;
    rules.shopItems[0].minLevel = 1;
    rules.shopItems[0].category = 0;
    rules.shopItems[0].hungerGain = 12;
    rules.shopItems[0].happyGain = 5;
    rules.shopItems[0].healthGain = 2;

    // 1: Sweet Pixel Snack (30% sick risk)
    strncpy(rules.shopItems[1].id, "food_snack", 19);
    strncpy(rules.shopItems[1].name, "Pixel Snack", 23);
    strncpy(rules.shopItems[1].name_de, "Pixel-Snack", 23);
    rules.shopItems[1].cost = 8;
    rules.shopItems[1].minLevel = 1;
    rules.shopItems[1].category = 0;
    rules.shopItems[1].hungerGain = 25;
    rules.shopItems[1].happyGain = 15;
    rules.shopItems[1].energyGain = 5;
    rules.shopItems[1].sickChancePct = 30;

    // 2: Meadow Herb Salad (4 Ticks Satiety)
    strncpy(rules.shopItems[2].id, "food_herb_salad", 19);
    strncpy(rules.shopItems[2].name, "Herb Salad", 23);
    strncpy(rules.shopItems[2].name_de, "Kraeuter-Salat", 23);
    rules.shopItems[2].cost = 22;
    rules.shopItems[2].minLevel = 1;
    rules.shopItems[2].category = 0;
    rules.shopItems[2].hungerGain = 35;
    rules.shopItems[2].happyGain = 10;
    rules.shopItems[2].healthGain = 10;
    rules.shopItems[2].energyGain = 10;
    rules.shopItems[2].satietyTicks = 4;

    // 3: Healing Salve (Cures sickness)
    strncpy(rules.shopItems[3].id, "item_salve", 19);
    strncpy(rules.shopItems[3].name, "Healing Salve", 23);
    strncpy(rules.shopItems[3].name_de, "Heilsalbe", 23);
    rules.shopItems[3].cost = 25;
    rules.shopItems[3].minLevel = 1;
    rules.shopItems[3].category = 2;
    rules.shopItems[3].healthGain = 40;
    rules.shopItems[3].cureSickness = true;

    // 4: Energy Tonic
    strncpy(rules.shopItems[4].id, "item_energy_elixir", 19);
    strncpy(rules.shopItems[4].name, "Energy Tonic", 23);
    strncpy(rules.shopItems[4].name_de, "Vital-Tonikum", 23);
    rules.shopItems[4].cost = 35;
    rules.shopItems[4].minLevel = 2;
    rules.shopItems[4].category = 2;
    rules.shopItems[4].energyGain = 60;

    // 5: Meadow Herb (Cooking ingredient)
    strncpy(rules.shopItems[5].id, "mat_herb", 19);
    strncpy(rules.shopItems[5].name, "Meadow Herb", 23);
    strncpy(rules.shopItems[5].name_de, "Wiesenkraeuter", 23);
    rules.shopItems[5].cost = 8;
    rules.shopItems[5].minLevel = 1;
    rules.shopItems[5].category = 1;

    rules.shopRecipeCount = 2;
    strncpy(rules.shopRecipes[0].id, "rcp_berry_pie", 15);
    strncpy(rules.shopRecipes[0].name, "Berry Tart", 23);
    strncpy(rules.shopRecipes[0].name_de, "Waldbeeren-Tarte", 23);
    rules.shopRecipes[0].cost = 45;
    rules.shopRecipes[0].minLevel = 2;
    strncpy(rules.shopRecipes[0].recipeId, "rcp_berry_pie", 15);

    strncpy(rules.shopRecipes[1].id, "rcp_stew", 15);
    strncpy(rules.shopRecipes[1].name, "Kelp Stew", 23);
    strncpy(rules.shopRecipes[1].name_de, "Pilzeintopf", 23);
    rules.shopRecipes[1].cost = 80;
    rules.shopRecipes[1].minLevel = 4;
    strncpy(rules.shopRecipes[1].recipeId, "rcp_stew", 15);

    rules.shopSpellCount = 2;
    strncpy(rules.shopSpells[0].id, "spl_treat_burst", 15);
    strncpy(rules.shopSpells[0].name, "Astral Feast", 23);
    strncpy(rules.shopSpells[0].name_de, "Astral-Festmahl", 23);
    rules.shopSpells[0].cost = 70;
    rules.shopSpells[0].minLevel = 3;
    rules.shopSpells[0].manaCost = 25;
    rules.shopSpells[0].healPower = 35;

    strncpy(rules.shopSpells[1].id, "spl_rain_dance", 15);
    strncpy(rules.shopSpells[1].name, "Astral Harmony", 23);
    strncpy(rules.shopSpells[1].name_de, "Astrale Harmonie", 23);
    rules.shopSpells[1].cost = 120;
    rules.shopSpells[1].minLevel = 5;
    rules.shopSpells[1].manaCost = 45;
    rules.shopSpells[1].healPower = 50;
  }

  // --------------------------------------------------------------------------
  // Runtime Config Loader (Reads human-readable .cfg or simple key=val lines)
  // --------------------------------------------------------------------------
  void loadConfigFromText(const String& cfgText) {
    int start = 0;
    while (start < (int)cfgText.length()) {
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
  // Fixed-Array Inventory Management (0 Heap Allocation)
  // --------------------------------------------------------------------------
  int findInventoryIndex(const char* itemId) const {
    for (uint8_t i = 0; i < inventoryCount; i++) {
      if (strcmp(inventory[i].id, itemId) == 0 && inventory[i].quantity > 0) {
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
    strncpy(slot.id, id, sizeof(slot.id) - 1);
    slot.id[sizeof(slot.id) - 1] = '\0';
    strncpy(slot.name, itemName, sizeof(slot.name) - 1);
    slot.name[sizeof(slot.name) - 1] = '\0';
    strncpy(slot.name_de, itemNameDe, sizeof(slot.name_de) - 1);
    slot.name_de[sizeof(slot.name_de) - 1] = '\0';
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
    if (strcmp(itemId, "food_apple") == 0) {
      return addInventoryItem("food_apple", "Crisp Apple", "Frischer Apfel", 0, qty, 12, 2, 5, 0, 0, 0, false);
    }
    if (strcmp(itemId, "food_snack") == 0) {
      return addInventoryItem("food_snack", "Pixel Snack", "Pixel-Snack", 0, qty, 25, 0, 15, 5, 0, 30, false);
    }
    if (strcmp(itemId, "food_herb_salad") == 0) {
      return addInventoryItem("food_herb_salad", "Herb Salad", "Kraeuter-Salat", 0, qty, 35, 10, 10, 10, 4, 0, false);
    }
    if (strcmp(itemId, "food_berry_pie") == 0) {
      return addInventoryItem("food_berry_pie", "Berry Tart", "Beeren-Tarte", 0, qty, 45, 15, 25, 5, 5, 0, false);
    }
    if (strcmp(itemId, "food_sun_jerky") == 0) {
      return addInventoryItem("food_sun_jerky", "Sun Jerky", "Doerrfleisch", 0, qty, 55, 10, 15, 20, 6, 0, false);
    }
    if (strcmp(itemId, "food_kelp_stew") == 0) {
      return addInventoryItem("food_kelp_stew", "Kelp Stew", "Pilzeintopf", 0, qty, 75, 30, 30, 15, 8, 0, false);
    }
    if (strcmp(itemId, "item_salve") == 0) {
      return addInventoryItem("item_salve", "Healing Salve", "Heilsalbe", 2, qty, 0, 40, 0, 0, 0, 0, true);
    }
    if (strcmp(itemId, "item_energy_elixir") == 0) {
      return addInventoryItem("item_energy_elixir", "Energy Tonic", "Vital-Tonikum", 2, qty, 0, 0, 0, 60, 0, 0, false);
    }
    if (strcmp(itemId, "mat_herb") == 0) {
      return addInventoryItem("mat_herb", "Meadow Herb", "Wiesenkraeuter", 1, qty);
    }
    if (strcmp(itemId, "mat_meat") == 0) {
      return addInventoryItem("mat_meat", "Raw Meat", "Rohes Fleisch", 1, qty);
    }
    if (strcmp(itemId, "mat_mushroom") == 0) {
      return addInventoryItem("mat_mushroom", "Forest Mushroom", "Waldpilz", 1, qty);
    }
    if (strcmp(itemId, "mat_sunstone") == 0) {
      return addInventoryItem("mat_sunstone", "Sun Salt", "Sonnensalz", 1, qty);
    }
    if (strcmp(itemId, "mat_kelp") == 0) {
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
  // Simulation Loop Tick (Honors Satiety Shield & Snack Sickness)
  // --------------------------------------------------------------------------
  void tick() {
    if (strcmp(phase, "Egg") == 0) return;

    if (satietyTicks > 0) {
      satietyTicks--;
    } else if (stats.hunger > 0) {
      stats.hunger = (uint8_t)max(0, (int)stats.hunger - (int)rules.hungerDecay);
    }

    if (stats.energy > 0) {
      stats.energy = (uint8_t)max(0, (int)stats.energy - (int)rules.energyDecay);
    }
    if (stats.happiness > 0) {
      stats.happiness = (uint8_t)max(0, (int)stats.happiness - (int)rules.happinessDecay);
    }

    if (stats.hunger < rules.starveThreshold && stats.health > 0) {
      stats.health = (uint8_t)max(0, (int)stats.health - (int)rules.healthStarveDecay);
    }

    if (isSick && stats.health > 0) {
      stats.health = (uint8_t)max(0, (int)stats.health - 1);
    }

    if (stats.mana < rules.maxStatValue) {
      stats.mana = (uint8_t)min((int)rules.maxStatValue, (int)stats.mana + (int)rules.naturalManaRegen);
    }
  }

  // --------------------------------------------------------------------------
  // Inventory-Gated Feeding & Potion Usage & Cooking
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
      if (random(0, 100) < (long)copySlot.sickChancePct) {
        rolledSick = true;
      }
    }

    stats.hunger    = (uint8_t)min((int)rules.maxStatValue, (int)stats.hunger + (int)copySlot.hungerGain);
    stats.happiness = (uint8_t)min((int)rules.maxStatValue, (int)stats.happiness + (int)copySlot.happyGain);
    stats.energy    = (uint8_t)min((int)rules.maxStatValue, (int)stats.energy + (int)copySlot.energyGain);

    int hp = (int)stats.health + (int)copySlot.healthGain - (rolledSick ? 15 : 0);
    stats.health = (uint8_t)constrain(hp, 0, (int)rules.maxStatValue);

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

    stats.health = (uint8_t)min((int)rules.maxStatValue, (int)stats.health + (int)copySlot.healthGain);
    stats.energy = (uint8_t)min((int)rules.maxStatValue, (int)stats.energy + (int)copySlot.energyGain);
    if (strcmp(itemId, "item_energy_elixir") == 0) {
      stats.mana = (uint8_t)min((int)rules.maxStatValue, (int)stats.mana + 15);
    }
    if (copySlot.cureSickness) {
      isSick = false;
    }
    return true;
  }

  // Cooks recipe into inventory (instead of eating immediately)
  bool craftRecipe(uint8_t recipeIdx) {
    if (recipeIdx == 0) {
      // Recipe 0: Meadow Herb Salad (2x mat_herb -> food_herb_salad)
      if (getItemQuantity("mat_herb") < 2) return false;
      consumeInventoryItem("mat_herb", 2);
      addKnownItemById("food_herb_salad", 1);
      addXP(35);
      return true;
    }
    if (recipeIdx == 1) {
      // Recipe 1: Forest Berry Tart (1x mat_herb + 1x mat_mushroom -> food_berry_pie)
      if (level < 2 || getItemQuantity("mat_herb") < 1 || getItemQuantity("mat_mushroom") < 1) return false;
      consumeInventoryItem("mat_herb", 1);
      consumeInventoryItem("mat_mushroom", 1);
      addKnownItemById("food_berry_pie", 1);
      addXP(60);
      return true;
    }
    if (recipeIdx == 2) {
      // Recipe 2: Sun-Dried Jerky (2x mat_meat + 1x mat_sunstone -> food_sun_jerky)
      if (level < 2 || getItemQuantity("mat_meat") < 2 || getItemQuantity("mat_sunstone") < 1) return false;
      consumeInventoryItem("mat_meat", 2);
      consumeInventoryItem("mat_sunstone", 1);
      addKnownItemById("food_sun_jerky", 1);
      addXP(80);
      return true;
    }
    return false;
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
        uint16_t extra = (uint16_t)min((int)100, (int)((consecutiveLoginDays - 1) / 7) * 15);
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
      if (strcmp(rules.shopItems[i].id, itemId) == 0) {
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
  // Core Interactions
  // --------------------------------------------------------------------------
  void feed(uint8_t amount = 25) {
    stats.hunger = (uint8_t)min((int)rules.maxStatValue, (int)stats.hunger + (int)amount);
    addXP(15);
    recordAction("feed", 1);
  }

  void play(uint8_t happy = 20, uint8_t cost = 10) {
    stats.happiness = (uint8_t)min((int)rules.maxStatValue, (int)stats.happiness + (int)happy);
    stats.energy = (uint8_t)max(0, (int)stats.energy - (int)cost);
    addXP(20);
    recordAction("play", 1);
  }

  void clean() {
    stats.hygiene = rules.maxStatValue;
    stats.happiness = (uint8_t)min((int)rules.maxStatValue, (int)stats.happiness + 10);
    addXP(10);
    recordAction("bath", 1);
  }

  bool warmEgg(uint8_t amount = 25) {
    if (strcmp(phase, "Egg") != 0) return false;
    eggWarmth = (uint8_t)min((int)rules.eggWarmthRequired, (int)eggWarmth + (int)amount);
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
      xpNext = (uint16_t)(rules.baseXpRequirement * pow(rules.xpLevelMultiplier, level - 1));
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
    stats.hunger = (uint8_t)min((int)rules.maxStatValue, (int)stats.hunger + (int)rules.treatHungerGain);
    stats.happiness = (uint8_t)min((int)rules.maxStatValue, (int)stats.happiness + (int)rules.treatHappinessGain);
    if (onEvent) onEvent("TREAT_RECEIVED", nullptr);
  }
};

#endif // TAMAGOTCHI_CORE_ENGINE_H
