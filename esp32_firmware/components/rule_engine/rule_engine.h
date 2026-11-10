/**
 * PetDuet Rule Engine
 * Parses and provides access to rules.json game configuration
 */

#ifndef RULE_ENGINE_H
#define RULE_ENGINE_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

// Version info
#define PETDUET_VERSION "1.0.0"
#define RULE_ENGINE_VERSION "1.0.4"

// Forward declarations
typedef struct rule_engine_t rule_engine_t;
typedef struct stat_config_t stat_config_t;
typedef struct lifecycle_phase_t lifecycle_phase_t;
typedef struct biome_config_t biome_config_t;
typedef struct recipe_t recipe_t;
typedef struct job_t job_t;
typedef struct spell_t spell_t;
typedef struct daily_mission_t daily_mission_t;
typedef struct shop_item_t shop_item_t;
typedef struct shop_recipe_t shop_recipe_t;
typedef struct shop_spell_t shop_spell_t;

// Stat configuration
struct stat_config_t {
    float min;
    float max;
    float default_val;
    float decay_per_tick;
    // Health-specific
    float starve_decay;
    // Energy-specific
    float sleep_regen_per_tick;
    // Mana-specific
    float regen_per_tick;
};

// Lifecycle phase
struct lifecycle_phase_t {
    char id[32];
    char name[32];
    char name_de[32];
    int duration_minutes;  // -1 for infinite
    float decay_multiplier; // For baby phase
    int min_level;          // For youth/adult/elder
    int warmth_required;    // For egg phase
};

// Biome configuration
struct biome_config_t {
    char id[32];
    char name[32];
    char name_de[32];
    char color[16];
    char rgb565[16];
    char trait[64];
    // Modifiers
    float food_decay;
    float energy_decay;
    float happiness_decay;
    float hygiene_decay;
    float mining_yield;
    float herb_yield;
    float mushroom_yield;
    float pearl_yield;
    float recipe_xp_bonus;
    float egg_hatch_speed;
    int bedtime_hour;
    int max_mana_bonus;
};

// Recipe
struct recipe_t {
    char id[64];
    char name[64];
    char name_de[64];
    int min_level;
    // Ingredients: material_id -> count (simplified as fixed array)
    char ingredient_ids[8][32];
    int ingredient_counts[8];
    int ingredient_count;
    // Effects
    float food_effect;
    float energy_effect;
    float happiness_effect;
    float health_effect;
    float mana_effect;
    int xp_reward;
    char icon[8];
    char description[256];
    char description_de[256];
};

// Job
struct job_t {
    char id[64];
    char name[64];
    char name_de[64];
    int min_level;
    int energy_cost;
    int duration_seconds;
    int xp_reward;
    int coins_reward;
    // Materials rewarded
    char material_ids[4][32];
    int material_counts[4];
    int material_count;
    char icon[8];
};

// Spell
struct spell_t {
    char id[64];
    char name[64];
    char name_de[64];
    int min_level;
    int mana_cost;
    char target[16]; // "self", "friend", "both"
    // Effects
    float health_effect;
    float happiness_effect;
    float food_effect;
    float energy_effect;
    float mana_effect;
    bool clean_poop;
    bool cure_sickness;
    int xp_reward;
    char icon[8];
    char description[256];
    char description_de[256];
};

// Daily Mission
struct daily_mission_t {
    char id[64];
    char action_type[32]; // "feed", "login_streak", "play", "bath", "send_treat"
    int target_count;
    char name[64];
    char name_de[64];
    char description[128];
    char description_de[128];
    int coins_reward;
    int xp_reward;
    char icon[8];
};

// Shop Item
struct shop_item_t {
    char id[64];
    char name[64];
    char name_de[64];
    int price_coins;
    int min_level;
    char category[32]; // "food", "material", "potion", "accessory", "evolution"
    char slot[32];     // For accessories: "head", "body", etc.
    char icon[8];
    char description[256];
    char description_de[256];
    // Effects
    float hunger_effect;
    float happiness_effect;
    float health_effect;
    float energy_effect;
    float mana_effect;
    bool cure_sickness;
    int max_mana_bonus;
    int happiness_bonus;
    float xp_bonus_pct;
    float job_coin_bonus_pct;
    bool evolution_unlock;
};

// Shop Recipe (blueprint)
struct shop_recipe_t {
    char id[64];
    char name[64];
    char name_de[64];
    int price_coins;
    int min_level;
    char icon[8];
    char description[256];
    char description_de[256];
    // Ingredients
    char ingredient_ids[8][32];
    int ingredient_counts[8];
    int ingredient_count;
    // Effects (same as recipe_t)
    float food_effect;
    float energy_effect;
    float happiness_effect;
    float health_effect;
    float mana_effect;
    int xp_reward;
};

// Shop Spell (tome)
struct shop_spell_t {
    char id[64];
    char name[64];
    char name_de[64];
    int price_coins;
    int min_level;
    int mana_cost;
    char target[16];
    char icon[8];
    char description[256];
    char description_de[256];
    // Effects
    float health_effect;
    float happiness_effect;
    float food_effect;
    float energy_effect;
    float mana_effect;
    bool clean_poop;
    bool cure_sickness;
    int xp_reward;
};

// Rule Engine API

/**
 * Load rules from SPIFFS file
 * @param path Path to rules.json in SPIFFS
 * @return ESP_OK on success
 */
esp_err_t rule_engine_load_from_spiffs(const char *path);

/**
 * Load built-in default rules (fallback)
 * @return ESP_OK on success
 */
esp_err_t rule_engine_load_defaults(void);

/**
 * Get rule engine version string
 * @return Version string
 */
const char *rule_engine_get_version(void);

/**
 * Get tick interval in seconds
 * @return Tick interval
 */
int rule_engine_get_tick_interval_seconds(void);

/**
 * Get timezone string
 * @return Timezone (e.g., "Europe/Berlin")
 */
const char *rule_engine_get_timezone(void);

/**
 * Get NTP server
 * @return NTP server hostname
 */
const char *rule_engine_get_ntp_server(void);

// Stat Config Accessors
const stat_config_t *rule_engine_get_stat_config(const char *stat_name);

// Lifecycle Accessors
const lifecycle_phase_t *rule_engine_get_lifecycle_phase(const char *phase_id);
const lifecycle_phase_t *rule_engine_get_lifecycle_phase_by_level(int level);
int rule_engine_get_lifecycle_phase_count(void);
const lifecycle_phase_t *rule_engine_get_lifecycle_phases(void);
int rule_engine_get_care_mistake_threshold_seconds(void);

// Biome Accessors
const biome_config_t *rule_engine_get_biome(const char *biome_id);
int rule_engine_get_biome_count(void);
const biome_config_t *rule_engine_get_biomes(void);

// Recipe Accessors
const recipe_t *rule_engine_get_recipe(const char *recipe_id);
int rule_engine_get_recipe_count(void);
const recipe_t *rule_engine_get_recipes(void);
const recipe_t *rule_engine_get_recipe_by_level(int level);

// Job Accessors
const job_t *rule_engine_get_job(const char *job_id);
int rule_engine_get_job_count(void);
const job_t *rule_engine_get_jobs(void);
const job_t *rule_engine_get_job_by_level(int level);

// Spell Accessors
const spell_t *rule_engine_get_spell(const char *spell_id);
int rule_engine_get_spell_count(void);
const spell_t *rule_engine_get_spells(void);
const spell_t *rule_engine_get_spell_by_level(int level);

// Daily Mission Accessors
const daily_mission_t *rule_engine_get_daily_mission(const char *mission_id);
int rule_engine_get_daily_mission_count(void);
const daily_mission_t *rule_engine_get_daily_missions(void);

// Shop Accessors
const shop_item_t *rule_engine_get_shop_item(const char *item_id);
int rule_engine_get_shop_item_count(void);
const shop_item_t *rule_engine_get_shop_items(void);
const shop_recipe_t *rule_engine_get_shop_recipe(const char *recipe_id);
int rule_engine_get_shop_recipe_count(void);
const shop_recipe_t *rule_engine_get_shop_recipes(void);
const shop_spell_t *rule_engine_get_shop_spell(const char *spell_id);
int rule_engine_get_shop_spell_count(void);
const shop_spell_t *rule_engine_get_shop_spells(void);

// Utility: Calculate XP required for level
int rule_engine_calculate_required_xp(int level);

// Utility: Get localized string
const char *rule_engine_get_localized_name(const char *name_en, const char *name_de);
const char *rule_engine_get_localized_desc(const char *desc_en, const char *desc_de);

// Cleanup
void rule_engine_cleanup(void);

#ifdef __cplusplus
}
#endif

#endif // RULE_ENGINE_H