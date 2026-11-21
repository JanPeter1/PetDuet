/**
 * ============================================================================
 * PetDuet ESP32 CYD - Tamagotchi Core Engine
 * ============================================================================
 * Platform-independent game engine with fixed-arrays (zero-heap)
 * All mechanics driven by rules.json
 * ============================================================================
 */
#ifndef TAMAGOTCHI_CORE_ENGINE_H
#define TAMAGOTCHI_CORE_ENGINE_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"
#include "config.h"

// ============================================================================
// Constants & Limits (Fixed Array Sizes - optimized for actual rules content)
// ============================================================================
#define MAX_INVENTORY_SLOTS     16
#define MAX_INVENTORY_ITEMS     32
#define MAX_RECIPES             8
#define MAX_JOBS                8
#define MAX_SPELLS              8
#define MAX_SHOP_ITEMS          24
#define MAX_MISSIONS            8
#define MAX_BIOMES              4
#define MAX_LIFECYCLE_PHASES    5
#define MAX_STRING_LEN          32
#define MAX_RULES_JSON_SIZE     (64 * 1024)

// ============================================================================
// Stat Types (matching rules.json: food, health, happiness, energy, mana)
// ============================================================================
typedef enum {
    STAT_HEALTH = 0,
    STAT_FOOD,
    STAT_HAPPINESS,
    STAT_ENERGY,
    STAT_MANA,
} StatType;
#define MAX_STATS 5

// ============================================================================
// Item Categories
// ============================================================================
typedef enum {
    ITEM_CAT_FOOD = 0,
    ITEM_CAT_MEDICINE,
    ITEM_CAT_TOY,
    ITEM_CAT_MATERIAL,
    ITEM_CAT_RECIPE,
    ITEM_CAT_SPELL,
    ITEM_CAT_SPECIAL
} ItemCategory;

// ============================================================================
// Pet Lifecycle Phases
// ============================================================================
typedef enum {
    PHASE_EGG = 0,
    PHASE_BABY,
    PHASE_CHILD,
    PHASE_TEEN,
    PHASE_ADULT,
    PHASE_ELDER,
    PHASE_ANCESTOR
} LifePhase;

// ============================================================================
// Data Structures
// ============================================================================

// Stat Definition from rules.json
typedef struct {
    char id[MAX_STRING_LEN];
    char name[MAX_STRING_LEN];
    char name_de[MAX_STRING_LEN];
    float max_value;
    float decay_per_tick;
    float min_value;
    float critical_threshold;
    float starve_decay;  // Additional decay when at 0
} StatDef;

// Item Definition
typedef struct {
    char id[MAX_STRING_LEN];
    char name[MAX_STRING_LEN];
    char name_de[MAX_STRING_LEN];
    char description[MAX_STRING_LEN];
    char description_de[MAX_STRING_LEN];
    ItemCategory category;
    uint16_t buy_price;
    uint16_t sell_price;
    uint8_t stack_size;
    uint8_t min_level;
    // Effects (stat changes on use)
    struct {
        StatType stat;
        float value;
        bool is_percentage;
    } effects[MAX_STATS];
    uint8_t effect_count;
    // Biome affinity
    char preferred_biome[MAX_STRING_LEN];
    float biome_bonus;
} ItemDef;

// Recipe Definition
typedef struct {
    char id[MAX_STRING_LEN];
    char name[MAX_STRING_LEN];
    char name_de[MAX_STRING_LEN];
    struct {
        char item_id[MAX_STRING_LEN];
        uint8_t count;
    } ingredients[MAX_INVENTORY_ITEMS];
    uint8_t ingredient_count;
    struct {
        char item_id[MAX_STRING_LEN];
        uint8_t count;
    } results[MAX_INVENTORY_ITEMS];
    uint8_t result_count;
    uint8_t min_level;
    uint16_t xp_reward;
    uint16_t energy_cost;
    char required_biome[MAX_STRING_LEN];
} RecipeDef;

// Job Definition
typedef struct {
    char id[MAX_STRING_LEN];
    char name[MAX_STRING_LEN];
    char name_de[MAX_STRING_LEN];
    char description[MAX_STRING_LEN];
    char description_de[MAX_STRING_LEN];
    uint8_t min_level;
    uint16_t energy_cost;
    uint32_t duration_ms;
    struct {
        char item_id[MAX_STRING_LEN];
        uint8_t min_count;
        uint8_t max_count;
        float chance;
    } rewards[MAX_INVENTORY_ITEMS];
    uint8_t reward_count;
    struct {
        StatType stat;
        float value;
    } stat_changes[MAX_STATS];
    uint8_t stat_change_count;
    char required_biome[MAX_STRING_LEN];
    float biome_efficiency_bonus;
} JobDef;

// Spell Definition
typedef struct {
    char id[MAX_STRING_LEN];
    char name[MAX_STRING_LEN];
    char name_de[MAX_STRING_LEN];
    char description[MAX_STRING_LEN];
    char description_de[MAX_STRING_LEN];
    uint16_t mana_cost;
    uint8_t min_level;
    char target_type[MAX_STRING_LEN];  // "self", "partner", "both"
    struct {
        StatType stat;
        float value;
        bool is_percentage;
    } effects[MAX_STATS];
    uint8_t effect_count;
    uint32_t cooldown_ms;
    char required_biome[MAX_STRING_LEN];
} SpellDef;

// Shop Entry
typedef struct {
    char item_id[MAX_STRING_LEN];
    uint16_t price;
    uint8_t stock;
    bool unlimited;
    uint8_t min_level;
    char required_biome[MAX_STRING_LEN];
} ShopEntry;

// Daily Mission
typedef struct {
    char id[MAX_STRING_LEN];
    char name[MAX_STRING_LEN];
    char name_de[MAX_STRING_LEN];
    char description[MAX_STRING_LEN];
    char description_de[MAX_STRING_LEN];
    char action_type[MAX_STRING_LEN];  // "feed", "play", "job", "cast", "trade", etc.
    char target_item[MAX_STRING_LEN];
    uint16_t target_count;
    struct {
        char item_id[MAX_STRING_LEN];
        uint8_t count;
    } rewards[MAX_INVENTORY_ITEMS];
    uint8_t reward_count;
    uint16_t xp_reward;
    uint32_t coin_reward;
} MissionDef;

// Biome Definition
typedef struct {
    char id[MAX_STRING_LEN];
    char name[MAX_STRING_LEN];
    char name_de[MAX_STRING_LEN];
    struct {
        float food_decay;
        float happiness_decay;
        float energy_decay;
        float hygiene_decay;
        float mining_yield;
        float foraging_yield;
        float fishing_yield;
        float farming_yield;
        float magic_regen;
        float xp_bonus;
    } modifiers;
    uint16_t color_rgb565;
    uint8_t r, g, b;
    char trait[MAX_STRING_LEN];
    char trait_de[MAX_STRING_LEN];
} BiomeDef;

// Lifecycle Phase
typedef struct {
    LifePhase phase;
    char name[MAX_STRING_LEN];
    char name_de[MAX_STRING_LEN];
    uint16_t min_age_ticks;
    uint16_t max_age_ticks;
    float stat_multipliers[MAX_STATS];
    uint8_t unlocked_jobs[MAX_JOBS];
    uint8_t unlocked_job_count;
    uint8_t unlocked_spells[MAX_SPELLS];
    uint8_t unlocked_spell_count;
    char sprite_suffix[MAX_STRING_LEN];  // e.g., "_baby", "_adult"
} LifecyclePhaseDef;

// Inventory Slot
typedef struct {
    char item_id[MAX_STRING_LEN];
    uint16_t count;
    bool valid;
} InventorySlot;

// Active Mission (runtime)
typedef struct {
    MissionDef def;
    uint16_t current_progress;
    bool completed;
    bool claimed;
    uint32_t assigned_time;
} ActiveMission;

// Pet State (Complete runtime state)
typedef struct {
    // Identity
    char device_id[MAX_STRING_LEN];
    char trainer_name[MAX_STRING_LEN];
    char biome_id[MAX_STRING_LEN];
    char partner_device_id[MAX_STRING_LEN];
    char partner_trainer_name[MAX_STRING_LEN];
    char partner_biome_id[MAX_STRING_LEN];

    // Core Stats
    float stats[MAX_STATS];
    float stat_max[MAX_STATS];

    // Lifecycle
    LifePhase current_phase;
    uint32_t age_ticks;
    uint32_t total_ticks_lived;

    // Experience & Level
    uint32_t experience;
    uint16_t level;
    uint32_t xp_to_next_level;

    // Currency
    uint32_t coins;

    // Inventory
    InventorySlot inventory[MAX_INVENTORY_SLOTS];
    uint8_t inventory_count;

    // Active Jobs/Spells
    struct {
        char job_id[MAX_STRING_LEN];
        uint32_t start_time;
        uint32_t end_time;
        bool active;
    } current_job;

    struct {
        char spell_id[MAX_STRING_LEN];
        uint32_t last_cast_time;
        bool on_cooldown;
    } spell_cooldowns[MAX_SPELLS];

    // Missions
    ActiveMission active_missions[MAX_MISSIONS];
    uint8_t mission_count;
    uint32_t last_mission_reset;

    // Care Mistake Tracking
    uint32_t last_care_time[MAX_STATS];
    uint8_t care_mistakes;
    bool care_mistake_pending[MAX_STATS];

    // Social/Partner
    bool partner_online;
    uint32_t last_partner_sync;
    struct {
        char offered_item[MAX_STRING_LEN];
        uint8_t offered_count;
        char requested_item[MAX_STRING_LEN];
        uint8_t requested_count;
        uint32_t timestamp;
        bool pending;
    } pending_trade;

    // OTA / Rules
    char rules_version[MAX_STRING_LEN];
    char firmware_version[MAX_STRING_LEN];
    bool rules_dirty;

    // Timestamps
    uint32_t last_tick_time;
    uint32_t last_server_sync;
    uint32_t last_ota_check;
    uint32_t boot_time;

    // Flags
    bool initialized;
    bool dirty;  // Needs save
} PetState;

// Rules Container (Parsed from JSON)
typedef struct {
    StatDef stats[MAX_STATS];
    uint8_t stat_count;

    ItemDef items[MAX_INVENTORY_ITEMS];
    uint8_t item_count;

    RecipeDef recipes[MAX_RECIPES];
    uint8_t recipe_count;

    JobDef jobs[MAX_JOBS];
    uint8_t job_count;

    SpellDef spells[MAX_SPELLS];
    uint8_t spell_count;

    ShopEntry shop_items[MAX_SHOP_ITEMS];
    uint8_t shop_item_count;

    MissionDef missions[MAX_MISSIONS];
    uint8_t mission_count;

    BiomeDef biomes[MAX_BIOMES];
    uint8_t biome_count;

    LifecyclePhaseDef lifecycle_phases[MAX_LIFECYCLE_PHASES];
    uint8_t lifecycle_phase_count;

    // Meta
    uint32_t tick_interval_seconds;
    char timezone[MAX_STRING_LEN];
    uint32_t care_mistake_threshold_seconds;
    float season_modifiers[4][MAX_STATS];  // 4 seasons

    bool valid;
} RulesData;

// ============================================================================
// Engine API
// ============================================================================

// Initialization
esp_err_t engine_init(PetState* pet, const RulesData* rules);
esp_err_t engine_load_rules(const char* json_path, RulesData* rules);
esp_err_t engine_save_pet(const PetState* pet, const char* nvs_namespace);
esp_err_t engine_load_pet(PetState* pet, const char* nvs_namespace);

// Main Game Loop
void engine_tick(PetState* pet, const RulesData* rules, uint32_t delta_ms);
void engine_process_tick(PetState* pet, const RulesData* rules);

// Stat Management
float engine_get_stat(const PetState* pet, StatType stat);
void engine_set_stat(PetState* pet, StatType stat, float value);
void engine_modify_stat(PetState* pet, StatType stat, float delta);
bool engine_is_stat_critical(const PetState* pet, StatType stat);

// Actions
esp_err_t engine_feed(PetState* pet, const RulesData* rules, const char* item_id);
esp_err_t engine_play(PetState* pet, const RulesData* rules, const char* item_id);
esp_err_t engine_bathe(PetState* pet, const RulesData* rules);
esp_err_t engine_sleep(PetState* pet, const RulesData* rules);
esp_err_t engine_do_job(PetState* pet, const RulesData* rules, const char* job_id);
esp_err_t engine_cast_spell(PetState* pet, const RulesData* rules,
                            const char* spell_id, PetState* target_pet);
esp_err_t engine_cook(PetState* pet, const RulesData* rules, const char* recipe_id);

// Inventory
esp_err_t engine_add_item(PetState* pet, const char* item_id, uint16_t count);
esp_err_t engine_remove_item(PetState* pet, const char* item_id, uint16_t count);
uint16_t engine_get_item_count(const PetState* pet, const char* item_id);
bool engine_has_item(const PetState* pet, const char* item_id, uint16_t count);

// Shop
esp_err_t engine_buy_item(PetState* pet, const RulesData* rules,
                          const char* item_id, uint8_t count);
esp_err_t engine_sell_item(PetState* pet, const RulesData* rules,
                           const char* item_id, uint8_t count);

// Missions
void engine_update_missions(PetState* pet, const RulesData* rules);
esp_err_t engine_claim_mission(PetState* pet, uint8_t mission_index);

// Social/Partner
esp_err_t engine_send_treat(PetState* pet, const RulesData* rules,
                            const char* item_id, PetState* partner_pet);
esp_err_t engine_propose_trade(PetState* pet, const char* offered_item,
                               uint8_t offered_count, const char* requested_item,
                               uint8_t requested_count);
esp_err_t engine_accept_trade(PetState* pet, PetState* partner_pet);

// Lifecycle
void engine_update_lifecycle(PetState* pet, const RulesData* rules);
const char* engine_get_phase_name(LifePhase phase, bool german);

// Utility
const ItemDef* engine_find_item(const RulesData* rules, const char* item_id);
const RecipeDef* engine_find_recipe(const RulesData* rules, const char* recipe_id);
const JobDef* engine_find_job(const RulesData* rules, const char* job_id);
const SpellDef* engine_find_spell(const RulesData* rules, const char* spell_id);
const BiomeDef* engine_find_biome(const RulesData* rules, const char* biome_id);
const MissionDef* engine_find_mission(const RulesData* rules, const char* mission_id);

// Seasonal
uint8_t engine_get_current_season(const RulesData* rules);
float engine_get_season_modifier(const RulesData* rules, uint8_t season, StatType stat);

// Biome Modifiers
float engine_get_biome_modifier(const RulesData* rules, const char* biome_id,
                                const char* modifier_name);

// XP/Level
void engine_add_xp(PetState* pet, uint32_t xp);
uint32_t engine_calc_xp_for_level(uint16_t level);

// Care Mistakes
void engine_check_care_mistakes(PetState* pet, const RulesData* rules, uint32_t now);
uint8_t engine_get_care_mistakes(const PetState* pet);

// JSON Helpers (for rules parsing)
esp_err_t engine_parse_rules_json(const char* json_str, RulesData* rules);
char* engine_generate_pet_json(const PetState* pet);
esp_err_t engine_parse_pet_json(const char* json_str, PetState* pet);

#endif // TAMAGOTCHI_CORE_ENGINE_H