#include <Arduino.h>
#include <cassert>
#include <cstdio>
#include <cstring>
#include "TamagotchiCoreEngine.h"

static int g_testsRun = 0;
static int g_testsFailed = 0;
static int g_eventCount = 0;
static char g_lastEvent[32] = "";

static void testEventCallback(const char* eventName, void* /*data*/) {
  g_eventCount++;
  if (eventName) {
    std::strncpy(g_lastEvent, eventName, sizeof(g_lastEvent) - 1);
    g_lastEvent[sizeof(g_lastEvent) - 1] = '\0';
  }
}

#define CHECK(cond)                                                              \
  do {                                                                           \
    g_testsRun++;                                                                \
    if (!(cond)) {                                                               \
      g_testsFailed++;                                                           \
      std::printf("[FAIL] %s:%d -> %s\n", __FILE__, __LINE__, #cond);            \
    }                                                                            \
  } while (0)

static void testStartingStateAndInventory() {
  TamagotchiCoreEngine eng;
  CHECK(std::strcmp(eng.phase, "Egg") == 0);
  CHECK(eng.level == 1);
  CHECK(eng.coins == 45);
  CHECK(eng.getItemQuantity("food_apple") == 2);
  CHECK(eng.getItemQuantity("food_snack") == 1);
  CHECK(eng.getItemQuantity("mat_herb") == 2);
  CHECK(eng.getItemQuantity("nonexistent") == 0);

  uint8_t foods[8];
  CHECK(eng.getAvailableFoods(foods, 8) == 2);
  uint8_t potions[8];
  CHECK(eng.getAvailablePotions(potions, 8) == 0);

  // Add zero quantity fails; stacking caps at 99; unknown ID fails
  CHECK(!eng.addInventoryItem("food_apple", "Apple", "Apfel", 0, 0));
  CHECK(eng.addInventoryItem("food_apple", "Apple", "Apfel", 0, 98));
  CHECK(eng.getItemQuantity("food_apple") == 99);
  CHECK(!eng.addKnownItemById("unknown_item", 1));

  // Add all known item IDs and verify slot limits
  CHECK(eng.addKnownItemById("food_berry_pie", 1));
  CHECK(eng.addKnownItemById("food_sun_jerky", 1));
  CHECK(eng.addKnownItemById("food_kelp_stew", 1));
  CHECK(eng.addKnownItemById("item_salve", 1));
  CHECK(eng.addKnownItemById("item_energy_elixir", 1));
  CHECK(eng.addKnownItemById("mat_meat", 2));
  CHECK(eng.addKnownItemById("mat_mushroom", 1));
  CHECK(eng.addKnownItemById("mat_sunstone", 1));
  CHECK(eng.addKnownItemById("mat_kelp", 1));
  CHECK(eng.getAvailablePotions(potions, 8) == 2);

  // Consuming removes slot when qty hits 0
  CHECK(eng.consumeInventoryItem("food_kelp_stew", 1));
  CHECK(eng.getItemQuantity("food_kelp_stew") == 0);
  CHECK(!eng.consumeInventoryItem("food_kelp_stew", 1));

  // Fill remaining slots up to MAX_INVENTORY_SLOTS
  for (int i = 0; i < 20; i++) {
    char id[16];
    std::snprintf(id, sizeof(id), "extra_%d", i);
    eng.addInventoryItem(id, "Extra", "Extra", 1, 1);
  }
  CHECK(eng.inventoryCount == TamagotchiCoreEngine::MAX_INVENTORY_SLOTS);
  CHECK(!eng.addInventoryItem("overflow_item", "Over", "Over", 1, 1));
}

static void testFeedingSatietyAndSickness() {
  TamagotchiCoreEngine eng;
  std::strcpy(eng.phase, "Baby");
  eng.stats.hunger = 40;
  eng.stats.health = 80;

  // Apple feeding (+12 hunger, safe)
  setMockRandomValue(99);
  FeedResult r1 = eng.feedWithItem("food_apple");
  CHECK(r1.success);
  CHECK(!r1.becameSick);
  CHECK(r1.hungerGain == 12);
  CHECK(eng.stats.hunger == 52);
  CHECK(eng.getItemQuantity("food_apple") == 1);

  // Snack feeding with sick roll (< 30)
  setMockRandomValue(10);
  FeedResult r2 = eng.feedWithItem("food_snack");
  CHECK(r2.success);
  CHECK(r2.becameSick);
  CHECK(eng.isSick);
  CHECK(eng.stats.health == 67); // 80 + 2(from apple) - 15 = 67
  CHECK(eng.getItemQuantity("food_snack") == 0);

  // Feeding unavailable or material item fails
  FeedResult rFail = eng.feedWithItem("food_snack");
  CHECK(!rFail.success);
  FeedResult rMat = eng.feedWithItem("mat_herb");
  CHECK(!rMat.success);

  // Herb Salad grants satiety shield
  CHECK(eng.addKnownItemById("food_herb_salad", 1));
  FeedResult rSalad = eng.feedWithItem("food_herb_salad");
  CHECK(rSalad.success);
  CHECK(rSalad.satietyGained == 4);
  CHECK(eng.satietyTicks == 4);

  // Use Healing Salve cures sickness & restores HP
  CHECK(eng.addKnownItemById("item_salve", 1));
  CHECK(eng.usePotionItem("item_salve"));
  CHECK(!eng.isSick);
  CHECK(eng.stats.health == 100);

  // Energy Elixir restores energy & mana
  eng.stats.energy = 20;
  eng.stats.mana = 10;
  CHECK(eng.addKnownItemById("item_energy_elixir", 1));
  CHECK(eng.usePotionItem("item_energy_elixir"));
  CHECK(eng.stats.energy == 80);
  CHECK(eng.stats.mana == 25);

  // Non-potion rejected by usePotionItem
  CHECK(!eng.usePotionItem("mat_herb"));
}

static void testTickSimulation() {
  TamagotchiCoreEngine eng;

  // Egg skips decay
  uint8_t h0 = eng.stats.hunger;
  eng.tick();
  CHECK(eng.stats.hunger == h0);

  // Active Baby with satietyTicks blocks hunger decay
  std::strcpy(eng.phase, "Baby");
  eng.stats.hunger = 50;
  eng.stats.energy = 50;
  eng.stats.happiness = 50;
  eng.stats.mana = 50;
  eng.satietyTicks = 2;
  eng.tick();
  CHECK(eng.satietyTicks == 1);
  CHECK(eng.stats.hunger == 50);
  CHECK(eng.stats.energy == 49);
  CHECK(eng.stats.happiness == 49);
  CHECK(eng.stats.mana == 52);

  // Without satiety, starving + sick drains HP
  eng.satietyTicks = 0;
  eng.stats.hunger = 10;
  eng.stats.health = 50;
  eng.isSick = true;
  eng.tick();
  CHECK(eng.stats.hunger == 9);
  CHECK(eng.stats.health == 48); // -1 starve, -1 sick
}

static void testCookingMissionsStreakAndShop() {
  TamagotchiCoreEngine eng;
  eng.onEvent = testEventCallback;

  // Recipe 0: Herb Salad needs 2x mat_herb
  CHECK(eng.craftRecipe(0));
  CHECK(eng.getItemQuantity("food_herb_salad") == 1);
  CHECK(!eng.craftRecipe(0)); // herbs depleted

  // Recipe 1: level gated (<2 fails), then succeeds at Lv.2
  eng.addKnownItemById("mat_herb", 1);
  eng.addKnownItemById("mat_mushroom", 1);
  CHECK(!eng.craftRecipe(1));
  eng.level = 2;
  CHECK(eng.craftRecipe(1));
  CHECK(eng.getItemQuantity("food_berry_pie") == 1);

  // Recipe 2: Sun Jerky
  eng.addKnownItemById("mat_meat", 2);
  eng.addKnownItemById("mat_sunstone", 1);
  CHECK(eng.craftRecipe(2));
  CHECK(eng.getItemQuantity("food_sun_jerky") == 1);
  CHECK(!eng.craftRecipe(99));

  // Missions
  CHECK(!eng.claimMission("bath_1"));
  eng.clean(); // triggers bath_1
  CHECK(eng.missions[2].completed);
  uint16_t coinsBefore = eng.coins;
  CHECK(eng.claimMission("bath_1"));
  CHECK(eng.coins == coinsBefore + 15);
  CHECK(!eng.claimMission("bath_1")); // double-claim blocked

  // Daily Login Streak (days 1..8)
  CHECK(eng.canClaimStreakBonus());
  CHECK(eng.claimDailyLoginStreakBonus());
  CHECK(!eng.claimDailyLoginStreakBonus());
  for (uint16_t d = 1; d <= 15; d++) {
    eng.consecutiveLoginDays = d;
    CHECK(eng.getStreakBonusCoins() >= 15);
    CHECK(eng.getStreakBonusXP() >= 10);
  }

  // Shop purchases
  eng.coins = 500;
  eng.level = 5;
  CHECK(eng.purchaseItem("food_apple"));
  CHECK(eng.purchaseRecipe("rcp_berry_pie"));
  CHECK(eng.purchaseSpell("spl_treat_burst"));
  CHECK(!eng.purchaseItem("missing_item"));
  CHECK(!eng.purchaseRecipe("missing_recipe"));
  CHECK(!eng.purchaseSpell("missing_spell"));

  eng.coins = 1;
  CHECK(!eng.purchaseItem("food_apple"));
  CHECK(!eng.purchaseRecipe("rcp_berry_pie"));
  CHECK(!eng.purchaseSpell("spl_treat_burst"));
}

static void testEggEvolutionTreatAndConfig() {
  TamagotchiCoreEngine eng;
  eng.onEvent = testEventCallback;

  // Warm egg until hatched
  CHECK(!eng.warmEgg(25)); // 45
  CHECK(!eng.warmEgg(25)); // 70
  CHECK(!eng.warmEgg(25)); // 95
  CHECK(eng.warmEgg(25));  // >= 100 -> hatches to Baby!
  CHECK(std::strcmp(eng.phase, "Baby") == 0);
  CHECK(!eng.warmEgg(10)); // already hatched

  // Core actions & evolution
  eng.feed(20);
  eng.play(15, 5);
  eng.receiveRemoteTreat();
  eng.addXP(2000);
  CHECK(eng.level >= 4);
  eng.level = 4;  eng.checkEvolution(); CHECK(std::strcmp(eng.phase, "Youth") == 0);
  eng.level = 8;  eng.checkEvolution(); CHECK(std::strcmp(eng.phase, "Adult") == 0);
  eng.level = 15; eng.checkEvolution(); CHECK(std::strcmp(eng.phase, "Elder") == 0);

  // Runtime .cfg loader
  String cfg =
    "# Tamagotchi config comment\n"
    "[Decay]\n"
    "hunger_decay = 3\n"
    "energy_decay = 2\n"
    "happiness_decay = 2\n"
    "starve_threshold = 25\n"
    "health_starve_decay = 4\n"
    "natural_mana_regen = 5\n"
    "egg_warmth_required = 90\n"
    "youth_min_level = 3\n"
    "adult_min_level = 7\n"
    "elder_min_level = 14\n"
    "base_xp_requirement = 120\n"
    "treat_send_hunger_gain = \"30\"\n"
    "treat_send_happiness_gain = 22\n";
  g_eventCount = 0;
  eng.loadConfigFromText(cfg);
  CHECK(eng.rules.hungerDecay == 3);
  CHECK(eng.rules.treatHungerGain == 30);
  CHECK(eng.rules.treatHappinessGain == 22);
  CHECK(eng.rules.baseXpRequirement == 120);
  CHECK(std::strcmp(g_lastEvent, "CONFIG_LOADED") == 0);
}

static void testJsonRuleParser() {
  TamagotchiCoreEngine eng;
  eng.onEvent = testEventCallback;
  g_eventCount = 0;

  const char* fullSampleJson =
    "{\n"
    "  \"version\": \"1.1.0\",\n"
    "  \"meta\": { \"title\": \"PetDuet\", \"tick_interval_seconds\": 60 },\n"
    "  \"stats\": {\n"
    "    \"food\": { \"max\": 100, \"decay_per_tick\": 0.5 },\n"
    "    \"health\": { \"starve_decay\": 1.5, \"sick_decay\": 1.0 },\n"
    "    \"happiness\": { \"decay_per_tick\": 0.25 },\n"
    "    \"energy\": { \"decay_per_tick\": 0.5 },\n"
    "    \"mana\": { \"regen_per_tick\": 0.5 }\n"
    "  },\n"
    "  \"foods\": [\n"
    "    { \"id\": \"food_ambrosia\", \"name\": \"Elemental Ambrosia\", \"name_de\": \"Elementar-Ambrosia\", \"hunger_gain\": 100, \"happiness_gain\": 100, \"health_gain\": 100, \"energy_gain\": 100, \"satiety_ticks\": 12, \"sick_chance\": 0.0 }\n"
    "  ],\n"
    "  \"starting_inventory\": [\n"
    "    { \"id\": \"food_apple\", \"quantity\": 3 },\n"
    "    { \"id\": \"mat_herb\", \"quantity\": 2 }\n"
    "  ],\n"
    "  \"lifecycle\": {\n"
    "    \"phases\": [\n"
    "      { \"id\": \"egg\", \"warmth_required\": 50 },\n"
    "      { \"id\": \"youth\", \"min_level\": 3 },\n"
    "      { \"id\": \"adult\", \"min_level\": 8 },\n"
    "      { \"id\": \"elder\", \"min_level\": 20 }\n"
    "    ],\n"
    "    \"care_mistake_threshold_seconds\": 900\n"
    "  },\n"
    "  \"biomes\": {\n"
    "    \"desert\": { \"name\": \"Desert\", \"name_de\": \"W\\u00fcste\", \"modifiers\": { \"food_decay\": 2.0, \"energy_decay\": 2.0 } },\n"
    "    \"water\": { \"name\": \"Water\", \"name_de\": \"Wasser\", \"modifiers\": { \"max_mana\": 20 } }\n"
    "  },\n"
    "  \"recipes\": [\n"
    "    { \"id\": \"rcp_salad\", \"name\": \"Herb Salad\", \"name_de\": \"Wiesenkr\\u00e4uter-Salat\", \"min_level\": 1, \"ingredients\": { \"mat_herb\": 2 }, \"result_item_id\": \"food_herb_salad\", \"effects\": { \"food\": 35, \"health\": 10, \"energy\": 10, \"satiety_ticks\": 4, \"xp\": 35 } },\n"
    "    { \"id\": \"rcp_berry_pie\", \"name\": \"Berry Tart\", \"name_de\": \"Waldbeeren-Tarte\", \"min_level\": 2, \"ingredients\": { \"mat_mushroom\": 1, \"mat_herb\": 1 }, \"result_item_id\": \"food_berry_pie\", \"effects\": { \"food\": 45, \"happiness\": 25, \"health\": 15, \"satiety_ticks\": 5, \"xp\": 80 } }\n"
    "  ],\n"
    "  \"jobs\": [\n"
    "    { \"id\": \"job_mine\", \"name\": \"Crystal Miner\", \"name_de\": \"Kristall-Sammler\", \"min_level\": 3, \"energy_cost\": 30, \"duration_seconds\": 180, \"rewards\": { \"xp\": 90, \"coins\": 55, \"materials\": { \"mat_sunstone\": 1, \"mat_mushroom\": 1 }, \"food\": { \"food_sun_jerky\": 1 } } }\n"
    "  ],\n"
    "  \"spells\": [\n"
    "    { \"id\": \"spl_cleanse\", \"name\": \"Purify\", \"name_de\": \"L\\u00e4uterung\", \"min_level\": 1, \"mana_cost\": 15, \"target\": \"self\", \"effects\": { \"clean_poop\": true, \"cure_sickness\": true, \"health\": 10, \"xp\": 20 } }\n"
    "  ],\n"
    "  \"daily_missions\": [\n"
    "    { \"id\": \"mission_feed\", \"action_type\": \"feed\", \"target_count\": 3, \"name\": \"Nourish\", \"name_de\": \"F\\u00fcttern\", \"rewards\": { \"coins\": 40, \"xp\": 50 } }\n"
    "  ],\n"
    "  \"shop\": {\n"
    "    \"items\": [\n"
    "      { \"id\": \"food_berry_pie\", \"name\": \"Forest Berry Tart\", \"name_de\": \"Waldbeeren-Tarte\", \"price_coins\": 40, \"min_level\": 2, \"category\": \"food\", \"effects\": { \"hunger\": 45, \"happiness\": 25, \"health\": 15, \"satiety_ticks\": 5, \"sick_chance\": 0 } }\n"
    "    ],\n"
    "    \"recipes\": [],\n"
    "    \"spells\": []\n"
    "  }\n"
    "}\n";

  eng.loadConfigFromText(fullSampleJson);
  CHECK(std::strcmp(g_lastEvent, "CONFIG_LOADED") == 0);
  CHECK(std::strcmp(eng.rules.version, "1.1.0") == 0);
  CHECK(eng.rules.tickIntervalSeconds == 60);
  CHECK(eng.rules.eggWarmthRequired == 50);
  CHECK(eng.rules.youthMinLevel == 3);
  CHECK(eng.rules.adultMinLevel == 8);
  CHECK(eng.rules.elderMinLevel == 20);
  CHECK(eng.rules.careMistakeThresholdSec == 900);
  CHECK(eng.rules.foodDefCount == 1);
  CHECK(std::strcmp(eng.rules.foodDefs[0].id, "food_ambrosia") == 0);
  CHECK(eng.rules.foodDefs[0].satietyTicks == 12);

  // Starting inventory rule & Ambrosia dynamic food lookup
  eng.applyStartingInventoryFromRules();
  CHECK(eng.getItemQuantity("food_apple") == 3);
  CHECK(eng.getItemQuantity("mat_herb") == 2);
  CHECK(eng.addKnownItemById("food_ambrosia", 1));

  // Biome modifiers: Water max_mana +20 = 120; Desert doubles decay
  std::strcpy(eng.biome, "water");
  CHECK(eng.getEffectiveMaxMana() == 120);
  std::strcpy(eng.biome, "desert");
  CHECK(eng.getEffectiveMaxMana() == 100);

  std::strcpy(eng.phase, "Baby");
  eng.stats.hunger = 50;
  eng.stats.energy = 50;
  eng.stats.happiness = 50;
  eng.stats.mana = 50;
  eng.tick(); // desert: hunger 0.5*2.0=1.0 -> -1, energy 0.5*2.0=1.0 -> -1, mana +0.5 (acc)
  CHECK(eng.stats.hunger == 49);
  CHECK(eng.stats.energy == 49);
  CHECK(eng.stats.mana == 50);
  eng.tick(); // mana accumulator hits 1.0 -> +1
  CHECK(eng.stats.mana == 51);

  // Dynamic crafting by ID, spell cast, shop purchase, job rules
  CHECK(eng.craftRecipeById("rcp_salad"));
  CHECK(eng.getItemQuantity("food_herb_salad") == 1);
  eng.isSick = true;
  eng.stats.health = 60;
  CHECK(eng.castSpell(0));
  CHECK(!eng.isSick);
  CHECK(eng.stats.health == 70);
  CHECK(eng.stats.mana == 36);

  CHECK(eng.rules.jobCount == 1);
  CHECK(std::strcmp(eng.rules.jobs[0].matReward2Id, "mat_mushroom") == 0);
  CHECK(std::strcmp(eng.rules.jobs[0].foodRewardId, "food_sun_jerky") == 0);

  eng.coins = 100;
  eng.level = 2;
  CHECK(eng.purchaseItem("food_berry_pie"));
  CHECK(eng.coins == 60);
  CHECK(eng.getItemQuantity("food_berry_pie") >= 1);

  // Optional check against actual repository esp32_firmware/rules.json if accessible from CWD
  const char* candidatePaths[2] = { "esp32_firmware/rules.json", "../../esp32_firmware/rules.json" };
  for (int i = 0; i < 2; i++) {
    std::FILE* f = std::fopen(candidatePaths[i], "rb");
    if (f) {
      static char fileBuf[40960];
      size_t n = std::fread(fileBuf, 1, sizeof(fileBuf) - 1, f);
      std::fclose(f);
      fileBuf[n] = '\0';
      TamagotchiCoreEngine fullEng;
      CHECK(fullEng.loadRulesJson(fileBuf));
      CHECK(std::strcmp(fullEng.rules.version, "1.1.0") == 0);
      CHECK(fullEng.rules.foodDefCount == 7);
      CHECK(fullEng.rules.biomeCount == 4);
      CHECK(fullEng.rules.recipeCount == 5);
      CHECK(fullEng.rules.jobCount == 4);
      CHECK(fullEng.rules.spellCount == 5);
      CHECK(fullEng.missionCount == 5);
      CHECK(fullEng.rules.shopItemCount == 13);
      CHECK(fullEng.rules.shopRecipeCount == 5);
      CHECK(fullEng.rules.shopSpellCount == 5);
      std::strcpy(fullEng.biome, "water");
      CHECK(fullEng.getEffectiveMaxMana() == 120);
      break;
    }
  }
}

int main() {
  testStartingStateAndInventory();
  testFeedingSatietyAndSickness();
  testTickSimulation();
  testCookingMissionsStreakAndShop();
  testEggEvolutionTreatAndConfig();
  testJsonRuleParser();

  std::printf("[Firmware Unit Tests] Executed: %d assertions | Failed: %d\n",
              g_testsRun, g_testsFailed);
  return (g_testsFailed == 0) ? 0 : 1;
}
