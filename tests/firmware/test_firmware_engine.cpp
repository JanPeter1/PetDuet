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

int main() {
  testStartingStateAndInventory();
  testFeedingSatietyAndSickness();
  testTickSimulation();
  testCookingMissionsStreakAndShop();
  testEggEvolutionTreatAndConfig();

  std::printf("[Firmware Unit Tests] Executed: %d assertions | Failed: %d\n",
              g_testsRun, g_testsFailed);
  return (g_testsFailed == 0) ? 0 : 1;
}
