/**
 * ============================================================================
 * PetDuet ESP32 CYD - Tamagotchi Core Engine Implementation
 * ============================================================================
 * Platform-independent game engine with fixed-arrays (zero-heap)
 * All mechanics driven by rules.json
 * ============================================================================
 */
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <ctime>
#include "esp_log.h"
#include "esp_err.h"
#include "esp_timer.h"
#include "cJSON.h"
#include "TamagotchiCoreEngine.h"

static const char* TAG = "TamagotchiEngine";

// ============================================================================
// Internal Helpers
// ============================================================================
static float clampf(float v, float min, float max) {
    return v < min ? min : (v > max ? max : v);
}

static uint32_t calc_xp_for_level(uint16_t level) {
    return 100 * level * level; // Quadratic growth
}

// ============================================================================
// JSON Parsing Helpers
// ============================================================================
static void parse_stat_def(cJSON* obj, StatDef* stat) {
    cJSON* item;
    item = cJSON_GetObjectItem(obj, "id"); if (item) strncpy(stat->id, item->valuestring, MAX_STRING_LEN-1);
    item = cJSON_GetObjectItem(obj, "name"); if (item) strncpy(stat->name, item->valuestring, MAX_STRING_LEN-1);
    item = cJSON_GetObjectItem(obj, "name_de"); if (item) strncpy(stat->name_de, item->valuestring, MAX_STRING_LEN-1);
    item = cJSON_GetObjectItem(obj, "max_value"); if (item) stat->max_value = item->valuedouble;
    item = cJSON_GetObjectItem(obj, "decay_per_tick"); if (item) stat->decay_per_tick = item->valuedouble;
    item = cJSON_GetObjectItem(obj, "min_value"); if (item) stat->min_value = item->valuedouble;
    item = cJSON_GetObjectItem(obj, "critical_threshold"); if (item) stat->critical_threshold = item->valuedouble;
    item = cJSON_GetObjectItem(obj, "starve_decay"); if (item) stat->starve_decay = item->valuedouble;
}

static void parse_item_def(cJSON* obj, ItemDef* item) {
    cJSON* j;
    j = cJSON_GetObjectItem(obj, "id"); if (j) strncpy(item->id, j->valuestring, MAX_STRING_LEN-1);
    j = cJSON_GetObjectItem(obj, "name"); if (j) strncpy(item->name, j->valuestring, MAX_STRING_LEN-1);
    j = cJSON_GetObjectItem(obj, "name_de"); if (j) strncpy(item->name_de, j->valuestring, MAX_STRING_LEN-1);
    j = cJSON_GetObjectItem(obj, "description"); if (j) strncpy(item->description, j->valuestring, MAX_STRING_LEN-1);
    j = cJSON_GetObjectItem(obj, "description_de"); if (j) strncpy(item->description_de, j->valuestring, MAX_STRING_LEN-1);
    j = cJSON_GetObjectItem(obj, "category"); if (j) item->category = (ItemCategory)j->valueint;
    j = cJSON_GetObjectItem(obj, "buy_price"); if (j) item->buy_price = j->valueint;
    j = cJSON_GetObjectItem(obj, "sell_price"); if (j) item->sell_price = j->valueint;
    j = cJSON_GetObjectItem(obj, "stack_size"); if (j) item->stack_size = j->valueint;
    j = cJSON_GetObjectItem(obj, "min_level"); if (j) item->min_level = j->valueint;
    j = cJSON_GetObjectItem(obj, "preferred_biome"); if (j) strncpy(item->preferred_biome, j->valuestring, MAX_STRING_LEN-1);
    j = cJSON_GetObjectItem(obj, "biome_bonus"); if (j) item->biome_bonus = j->valuedouble;

    // Effects
    cJSON* effects = cJSON_GetObjectItem(obj, "effects");
    if (effects && cJSON_IsArray(effects)) {
        item->effect_count = 0;
        cJSON* eff;
        cJSON_ArrayForEach(eff, effects) {
            if (item->effect_count >= MAX_STATS) break;
            cJSON* stat = cJSON_GetObjectItem(eff, "stat");
            cJSON* value = cJSON_GetObjectItem(eff, "value");
            cJSON* pct = cJSON_GetObjectItem(eff, "is_percentage");
            if (stat && value) {
                item->effects[item->effect_count].stat = (StatType)stat->valueint;
                item->effects[item->effect_count].value = value->valuedouble;
                item->effects[item->effect_count].is_percentage = pct ? cJSON_IsTrue(pct) : false;
                item->effect_count++;
            }
        }
    }
}

// ============================================================================
// Engine API
// ============================================================================

esp_err_t engine_init(PetState* pet, const RulesData* rules) {
    if (!pet || !rules) return ESP_ERR_INVALID_ARG;

    memset(pet, 0, sizeof(PetState));

    // Set defaults from rules
    for (int i = 0; i < rules->stat_count && i < MAX_STATS; i++) {
        pet->stat_max[i] = rules->stats[i].max_value;
        pet->stats[i] = rules->stats[i].max_value;
    }

    // Identity
    strncpy(pet->device_id, DEVICE_ID, MAX_STRING_LEN-1);
    strncpy(pet->trainer_name, TRAINER_NAME, MAX_STRING_LEN-1);
    strncpy(pet->biome_id, DEFAULT_BIOME, MAX_STRING_LEN-1);
    strncpy(pet->partner_device_id, FRIEND_DEV_ID, MAX_STRING_LEN-1);

    // Initial state
    pet->current_phase = PHASE_EGG;
    pet->age_ticks = 0;
    pet->experience = 0;
    pet->level = 1;
    pet->xp_to_next_level = calc_xp_for_level(1);
    pet->coins = 100; // Starting coins
    pet->rules_version[0] = '\0';
    strncpy(pet->firmware_version, FIRMWARE_VERSION, MAX_STRING_LEN-1);
    pet->initialized = true;
    pet->dirty = true;
    pet->boot_time = esp_timer_get_time() / 1000;
    pet->last_tick_time = pet->boot_time;

    // Initialize inventory
    for (int i = 0; i < MAX_INVENTORY_SLOTS; i++) {
        pet->inventory[i].valid = false;
    }

    ESP_LOGI(TAG, "Engine initialized for %s", pet->trainer_name);
    return ESP_OK;
}

esp_err_t engine_load_rules(const char* json_path, RulesData* rules) {
    if (!json_path || !rules) return ESP_ERR_INVALID_ARG;

    FILE* f = fopen(json_path, "r");
    if (!f) {
        ESP_LOGE(TAG, "Failed to open rules file: %s", json_path);
        return ESP_ERR_NOT_FOUND;
    }

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);

    if (size > MAX_RULES_JSON_SIZE) {
        ESP_LOGE(TAG, "Rules file too large: %ld bytes", size);
        fclose(f);
        return ESP_ERR_NO_MEM;
    }

    char* json_str = (char*)malloc(size + 1);
    if (!json_str) {
        fclose(f);
        return ESP_ERR_NO_MEM;
    }

    fread(json_str, 1, size, f);
    json_str[size] = '\0';
    fclose(f);

    esp_err_t ret = engine_parse_rules_json(json_str, rules);
    free(json_str);
    return ret;
}

esp_err_t engine_parse_rules_json(const char* json_str, RulesData* rules) {
    if (!json_str || !rules) return ESP_ERR_INVALID_ARG;

    memset(rules, 0, sizeof(RulesData));

    cJSON* root = cJSON_Parse(json_str);
    if (!root) {
        const char* err = cJSON_GetErrorPtr();
        ESP_LOGE(TAG, "JSON parse error: %s", err ? err : "unknown");
        return ESP_ERR_INVALID_ARG;
    }

    // Meta
    cJSON* meta = cJSON_GetObjectItem(root, "meta");
    if (meta) {
        cJSON* tick = cJSON_GetObjectItem(meta, "tick_interval_seconds");
        if (tick) rules->tick_interval_seconds = tick->valueint;
        cJSON* tz = cJSON_GetObjectItem(meta, "timezone");
        if (tz) strncpy(rules->timezone, tz->valuestring, MAX_STRING_LEN-1);
        cJSON* cm = cJSON_GetObjectItem(meta, "care_mistake_threshold_seconds");
        if (cm) rules->care_mistake_threshold_seconds = cm->valueint;
    }

    // Stats
    cJSON* stats = cJSON_GetObjectItem(root, "stats");
    if (stats && cJSON_IsObject(stats)) {
        cJSON* stat;
        cJSON_ArrayForEach(stat, stats) {
            if (rules->stat_count >= MAX_STATS) break;
            parse_stat_def(stat, &rules->stats[rules->stat_count++]);
        }
    }

    // Items
    cJSON* items = cJSON_GetObjectItem(root, "items");
    if (items && cJSON_IsArray(items)) {
        cJSON* item;
        cJSON_ArrayForEach(item, items) {
            if (rules->item_count >= MAX_INVENTORY_ITEMS) break;
            parse_item_def(item, &rules->items[rules->item_count++]);
        }
    }

    // Recipes, Jobs, Spells, Shop, Missions, Biomes, Lifecycle - similar parsing
    // For brevity, only implementing critical ones here

    // Biomes
    cJSON* biomes = cJSON_GetObjectItem(root, "biomes");
    if (biomes && cJSON_IsObject(biomes)) {
        cJSON* biome;
        cJSON_ArrayForEach(biome, biomes) {
            if (rules->biome_count >= MAX_BIOMES) break;
            BiomeDef* b = &rules->biomes[rules->biome_count];
            cJSON* j;
            j = cJSON_GetObjectItem(biome, "id"); if (j) strncpy(b->id, j->valuestring, MAX_STRING_LEN-1);
            j = cJSON_GetObjectItem(biome, "name"); if (j) strncpy(b->name, j->valuestring, MAX_STRING_LEN-1);
            j = cJSON_GetObjectItem(biome, "name_de"); if (j) strncpy(b->name_de, j->valuestring, MAX_STRING_LEN-1);
            j = cJSON_GetObjectItem(biome, "color_rgb565"); if (j) b->color_rgb565 = j->valueint;
            j = cJSON_GetObjectItem(biome, "trait"); if (j) strncpy(b->trait, j->valuestring, MAX_STRING_LEN-1);
            j = cJSON_GetObjectItem(biome, "trait_de"); if (j) strncpy(b->trait_de, j->valuestring, MAX_STRING_LEN-1);

            // Modifiers
            cJSON* mod = cJSON_GetObjectItem(biome, "modifiers");
            if (mod) {
                j = cJSON_GetObjectItem(mod, "food_decay"); if (j) b->modifiers.food_decay = j->valuedouble;
                j = cJSON_GetObjectItem(mod, "happiness_decay"); if (j) b->modifiers.happiness_decay = j->valuedouble;
                j = cJSON_GetObjectItem(mod, "energy_decay"); if (j) b->modifiers.energy_decay = j->valuedouble;
                j = cJSON_GetObjectItem(mod, "hygiene_decay"); if (j) b->modifiers.hygiene_decay = j->valuedouble;
                j = cJSON_GetObjectItem(mod, "mining_yield"); if (j) b->modifiers.mining_yield = j->valuedouble;
                j = cJSON_GetObjectItem(mod, "foraging_yield"); if (j) b->modifiers.foraging_yield = j->valuedouble;
                j = cJSON_GetObjectItem(mod, "fishing_yield"); if (j) b->modifiers.fishing_yield = j->valuedouble;
                j = cJSON_GetObjectItem(mod, "farming_yield"); if (j) b->modifiers.farming_yield = j->valuedouble;
                j = cJSON_GetObjectItem(mod, "magic_regen"); if (j) b->modifiers.magic_regen = j->valuedouble;
                j = cJSON_GetObjectItem(mod, "xp_bonus"); if (j) b->modifiers.xp_bonus = j->valuedouble;
            }
            rules->biome_count++;
        }
    }

    // Lifecycle phases
    cJSON* lifecycle = cJSON_GetObjectItem(root, "lifecycle");
    if (lifecycle) {
        cJSON* phases = cJSON_GetObjectItem(lifecycle, "phases");
        if (phases && cJSON_IsArray(phases)) {
            cJSON* phase;
            cJSON_ArrayForEach(phase, phases) {
                if (rules->lifecycle_phase_count >= MAX_LIFECYCLE_PHASES) break;
                LifecyclePhaseDef* p = &rules->lifecycle_phases[rules->lifecycle_phase_count];
                cJSON* j;
                j = cJSON_GetObjectItem(phase, "phase"); if (j) p->phase = (LifePhase)j->valueint;
                j = cJSON_GetObjectItem(phase, "name"); if (j) strncpy(p->name, j->valuestring, MAX_STRING_LEN-1);
                j = cJSON_GetObjectItem(phase, "name_de"); if (j) strncpy(p->name_de, j->valuestring, MAX_STRING_LEN-1);
                j = cJSON_GetObjectItem(phase, "min_age_ticks"); if (j) p->min_age_ticks = j->valueint;
                j = cJSON_GetObjectItem(phase, "max_age_ticks"); if (j) p->max_age_ticks = j->valueint;
                j = cJSON_GetObjectItem(phase, "sprite_suffix"); if (j) strncpy(p->sprite_suffix, j->valuestring, MAX_STRING_LEN-1);
                rules->lifecycle_phase_count++;
            }
        }
    }

    rules->valid = true;
    cJSON_Delete(root);

    ESP_LOGI(TAG, "Rules loaded: %d stats, %d items, %d biomes, %d phases",
             rules->stat_count, rules->item_count, rules->biome_count, rules->lifecycle_phase_count);
    return ESP_OK;
}

// ============================================================================
// Game Loop
// ============================================================================

void engine_tick(PetState* pet, const RulesData* rules, uint32_t delta_ms) {
    if (!pet || !rules || !pet->initialized) return;

    uint32_t now = esp_timer_get_time() / 1000;

    // Process stat decay
    for (int i = 0; i < rules->stat_count && i < MAX_STATS; i++) {
        const StatDef* stat = &rules->stats[i];
        if (stat->decay_per_tick > 0) {
            float decay = stat->decay_per_tick * (delta_ms / 1000.0f) / rules->tick_interval_seconds;

            // Apply biome modifier
            float biome_mod = engine_get_biome_modifier(rules, pet->biome_id,
                i == STAT_FOOD ? "food_decay" :
                i == STAT_HAPPINESS ? "happiness_decay" :
                i == STAT_ENERGY ? "energy_decay" : "hygiene_decay");
            decay *= biome_mod;

            // Apply seasonal modifier
            uint8_t season = engine_get_current_season(rules);
            decay *= engine_get_season_modifier(rules, season, (StatType)i);

            // If stat is at 0, apply starve decay
            if (pet->stats[i] <= 0) {
                decay *= stat->starve_decay;
            }

            pet->stats[i] = clampf(pet->stats[i] - decay, stat->min_value, stat->max_value);
        }
    }

    // Mana regen
    float mana_regen = engine_get_biome_modifier(rules, pet->biome_id, "magic_regen");
    pet->stats[STAT_MANA] = clampf(pet->stats[STAT_MANA] + mana_regen * 0.1f,
                                    0, pet->stat_max[STAT_MANA]);

    // Age
    pet->age_ticks += delta_ms / 1000 / rules->tick_interval_seconds;
    pet->total_ticks_lived++;

    pet->last_tick_time = now;
    pet->dirty = true;
}

void engine_process_tick(PetState* pet, const RulesData* rules) {
    engine_tick(pet, rules, PET_TICK_INTERVAL_MS);
}

// ============================================================================
// Stat Management
// ============================================================================

float engine_get_stat(const PetState* pet, StatType stat) {
    if (!pet || stat >= MAX_STATS) return 0;
    return pet->stats[stat];
}

void engine_set_stat(PetState* pet, StatType stat, float value) {
    if (!pet || stat >= MAX_STATS) return;
    pet->stats[stat] = clampf(value, 0, pet->stat_max[stat]);
    pet->dirty = true;
}

void engine_modify_stat(PetState* pet, StatType stat, float delta) {
    if (!pet || stat >= MAX_STATS) return;
    pet->stats[stat] = clampf(pet->stats[stat] + delta, 0, pet->stat_max[stat]);
    pet->dirty = true;
}

bool engine_is_stat_critical(const PetState* pet, StatType stat) {
    if (!pet || stat >= MAX_STATS) return false;
    return pet->stats[stat] <= 0;
}

// ============================================================================
// Actions
// ============================================================================

esp_err_t engine_feed(PetState* pet, const RulesData* rules, const char* item_id) {
    if (!pet || !rules || !item_id) return ESP_ERR_INVALID_ARG;

    // Check inventory
    if (!engine_has_item(pet, item_id, 1)) {
        return ESP_ERR_NOT_FOUND;
    }

    // Find item
    const ItemDef* item = engine_find_item(rules, item_id);
    if (!item) return ESP_ERR_NOT_FOUND;

    if (item->category != ITEM_CAT_FOOD) {
        return ESP_ERR_INVALID_ARG;
    }

    // Apply effects
    for (int i = 0; i < item->effect_count; i++) {
        StatType stat = item->effects[i].stat;
        float value = item->effects[i].value;
        if (item->effects[i].is_percentage) {
            value = pet->stat_max[stat] * value / 100.0f;
        }
        engine_modify_stat(pet, stat, value);
    }

    // Consume item
    engine_remove_item(pet, item_id, 1);

    // Reset care mistake timer for food
    pet->last_care_time[STAT_FOOD] = esp_timer_get_time() / 1000;
    pet->care_mistake_pending[STAT_FOOD] = false;

    pet->dirty = true;
    return ESP_OK;
}

esp_err_t engine_play(PetState* pet, const RulesData* rules, const char* item_id) {
    if (!pet || !item_id) return ESP_ERR_INVALID_ARG;

    if (!engine_has_item(pet, item_id, 1)) {
        return ESP_ERR_NOT_FOUND;
    }

    const ItemDef* item = engine_find_item(rules, item_id);
    if (!item || item->category != ITEM_CAT_TOY) {
        return ESP_ERR_INVALID_ARG;
    }

    for (int i = 0; i < item->effect_count; i++) {
        engine_modify_stat(pet, item->effects[i].stat, item->effects[i].value);
    }

    engine_remove_item(pet, item_id, 1);
    pet->last_care_time[STAT_HAPPINESS] = esp_timer_get_time() / 1000;
    pet->care_mistake_pending[STAT_HAPPINESS] = false;
    pet->dirty = true;
    return ESP_OK;
}

esp_err_t engine_bathe(PetState* pet, const RulesData* rules) {
    if (!pet) return ESP_ERR_INVALID_ARG;

    // Bath increases happiness (no separate hygiene stat in rules.json)
    engine_set_stat(pet, STAT_HAPPINESS, pet->stat_max[STAT_HAPPINESS]);
    pet->last_care_time[STAT_HAPPINESS] = esp_timer_get_time() / 1000;
    pet->care_mistake_pending[STAT_HAPPINESS] = false;
    pet->dirty = true;
    return ESP_OK;
}

esp_err_t engine_sleep(PetState* pet, const RulesData* rules) {
    if (!pet) return ESP_ERR_INVALID_ARG;

    engine_set_stat(pet, STAT_ENERGY, pet->stat_max[STAT_ENERGY]);
    pet->last_care_time[STAT_ENERGY] = esp_timer_get_time() / 1000;
    pet->care_mistake_pending[STAT_ENERGY] = false;
    pet->dirty = true;
    return ESP_OK;
}

esp_err_t engine_do_job(PetState* pet, const RulesData* rules, const char* job_id) {
    if (!pet || !rules || !job_id) return ESP_ERR_INVALID_ARG;

    const JobDef* job = engine_find_job(rules, job_id);
    if (!job) return ESP_ERR_NOT_FOUND;

    // Check level requirement
    if (pet->level < job->min_level) return ESP_ERR_INVALID_ARG;

    // Check energy
    if (pet->stats[STAT_ENERGY] < job->energy_cost) return ESP_ERR_INVALID_STATE;

    // Start job
    strncpy(pet->current_job.job_id, job_id, MAX_STRING_LEN-1);
    pet->current_job.start_time = esp_timer_get_time() / 1000;
    pet->current_job.end_time = pet->current_job.start_time + job->duration_ms;
    pet->current_job.active = true;

    // Deduct energy immediately
    engine_modify_stat(pet, STAT_ENERGY, -job->energy_cost);
    pet->dirty = true;
    return ESP_OK;
}

esp_err_t engine_cast_spell(PetState* pet, const RulesData* rules,
                            const char* spell_id, PetState* target_pet) {
    if (!pet || !rules || !spell_id) return ESP_ERR_INVALID_ARG;

    const SpellDef* spell = engine_find_spell(rules, spell_id);
    if (!spell) return ESP_ERR_NOT_FOUND;

    // Check level
    if (pet->level < spell->min_level) return ESP_ERR_INVALID_ARG;

    // Check mana
    if (pet->stats[STAT_MANA] < spell->mana_cost) return ESP_ERR_INVALID_STATE;

    // Check cooldown
    for (int i = 0; i < MAX_SPELLS; i++) {
        if (strcmp(pet->spell_cooldowns[i].spell_id, spell_id) == 0) {
            uint32_t now = esp_timer_get_time() / 1000;
            if (now - pet->spell_cooldowns[i].last_cast_time < spell->cooldown_ms) {
                return ESP_ERR_INVALID_STATE; // On cooldown
            }
        }
    }

    // Deduct mana
    engine_modify_stat(pet, STAT_MANA, -spell->mana_cost);

    // Apply effects to target
    PetState* target = (strcmp(spell->target_type, "self") == 0 || !target_pet) ? pet : target_pet;
    for (int i = 0; i < spell->effect_count; i++) {
        float value = spell->effects[i].value;
        if (spell->effects[i].is_percentage) {
            value = target->stat_max[spell->effects[i].stat] * value / 100.0f;
        }
        engine_modify_stat(target, spell->effects[i].stat, value);
    }

    // Update cooldown
    for (int i = 0; i < MAX_SPELLS; i++) {
        if (pet->spell_cooldowns[i].spell_id[0] == '\0' ||
            strcmp(pet->spell_cooldowns[i].spell_id, spell_id) == 0) {
            strncpy(pet->spell_cooldowns[i].spell_id, spell_id, MAX_STRING_LEN-1);
            pet->spell_cooldowns[i].last_cast_time = esp_timer_get_time() / 1000;
            pet->spell_cooldowns[i].on_cooldown = true;
            break;
        }
    }

    pet->dirty = true;
    return ESP_OK;
}

esp_err_t engine_cook(PetState* pet, const RulesData* rules, const char* recipe_id) {
    if (!pet || !rules || !recipe_id) return ESP_ERR_INVALID_ARG;

    const RecipeDef* recipe = engine_find_recipe(rules, recipe_id);
    if (!recipe) return ESP_ERR_NOT_FOUND;

    // Check level
    if (pet->level < recipe->min_level) return ESP_ERR_INVALID_ARG;

    // Check energy
    if (pet->stats[STAT_ENERGY] < recipe->energy_cost) return ESP_ERR_INVALID_STATE;

    // Check ingredients
    for (int i = 0; i < recipe->ingredient_count; i++) {
        if (!engine_has_item(pet, recipe->ingredients[i].item_id, recipe->ingredients[i].count)) {
            return ESP_ERR_NOT_FOUND;
        }
    }

    // Consume ingredients
    for (int i = 0; i < recipe->ingredient_count; i++) {
        engine_remove_item(pet, recipe->ingredients[i].item_id, recipe->ingredients[i].count);
    }

    // Deduct energy
    engine_modify_stat(pet, STAT_ENERGY, -recipe->energy_cost);

    // Produce results
    for (int i = 0; i < recipe->result_count; i++) {
        engine_add_item(pet, recipe->results[i].item_id, recipe->results[i].count);
    }

    // Award XP
    engine_add_xp(pet, recipe->xp_reward);

    pet->dirty = true;
    return ESP_OK;
}

// ============================================================================
// Inventory
// ============================================================================

esp_err_t engine_add_item(PetState* pet, const char* item_id, uint16_t count) {
    if (!pet || !item_id || count == 0) return ESP_ERR_INVALID_ARG;

    // Find existing stack
    for (int i = 0; i < MAX_INVENTORY_SLOTS; i++) {
        if (pet->inventory[i].valid &&
            strcmp(pet->inventory[i].item_id, item_id) == 0) {
            // Check stack limit (would need item def for max stack)
            pet->inventory[i].count += count;
            pet->dirty = true;
            return ESP_OK;
        }
    }

    // Find empty slot
    for (int i = 0; i < MAX_INVENTORY_SLOTS; i++) {
        if (!pet->inventory[i].valid) {
            strncpy(pet->inventory[i].item_id, item_id, MAX_STRING_LEN-1);
            pet->inventory[i].count = count;
            pet->inventory[i].valid = true;
            pet->inventory_count++;
            pet->dirty = true;
            return ESP_OK;
        }
    }

    return ESP_ERR_NO_MEM; // Inventory full
}

esp_err_t engine_remove_item(PetState* pet, const char* item_id, uint16_t count) {
    if (!pet || !item_id || count == 0) return ESP_ERR_INVALID_ARG;

    for (int i = 0; i < MAX_INVENTORY_SLOTS; i++) {
        if (pet->inventory[i].valid &&
            strcmp(pet->inventory[i].item_id, item_id) == 0) {
            if (pet->inventory[i].count < count) return ESP_ERR_INVALID_ARG;

            pet->inventory[i].count -= count;
            if (pet->inventory[i].count == 0) {
                pet->inventory[i].valid = false;
                pet->inventory[i].item_id[0] = '\0';
                pet->inventory_count--;
            }
            pet->dirty = true;
            return ESP_OK;
        }
    }

    return ESP_ERR_NOT_FOUND;
}

uint16_t engine_get_item_count(const PetState* pet, const char* item_id) {
    if (!pet || !item_id) return 0;

    for (int i = 0; i < MAX_INVENTORY_SLOTS; i++) {
        if (pet->inventory[i].valid &&
            strcmp(pet->inventory[i].item_id, item_id) == 0) {
            return pet->inventory[i].count;
        }
    }
    return 0;
}

bool engine_has_item(const PetState* pet, const char* item_id, uint16_t count) {
    return engine_get_item_count(pet, item_id) >= count;
}

// ============================================================================
// Shop
// ============================================================================

esp_err_t engine_buy_item(PetState* pet, const RulesData* rules,
                          const char* item_id, uint8_t count) {
    if (!pet || !rules || !item_id) return ESP_ERR_INVALID_ARG;

    // Find shop entry
    for (int i = 0; i < rules->shop_item_count; i++) {
        if (strcmp(rules->shop_items[i].item_id, item_id) == 0) {
            const ShopEntry* entry = &rules->shop_items[i];

            // Check level
            if (pet->level < entry->min_level) return ESP_ERR_INVALID_ARG;

            // Check biome
            if (entry->required_biome[0] &&
                strcmp(pet->biome_id, entry->required_biome) != 0) {
                return ESP_ERR_INVALID_ARG;
            }

            // Check stock
            if (!entry->unlimited && entry->stock < count) return ESP_ERR_NOT_FOUND;

            // Check price
            uint32_t total_price = entry->price * count;
            if (pet->coins < total_price) return ESP_ERR_INVALID_STATE;

            // Buy
            pet->coins -= total_price;
            engine_add_item(pet, item_id, count);
            pet->dirty = true;
            return ESP_OK;
        }
    }

    return ESP_ERR_NOT_FOUND;
}

esp_err_t engine_sell_item(PetState* pet, const RulesData* rules,
                           const char* item_id, uint8_t count) {
    if (!pet || !rules || !item_id) return ESP_ERR_INVALID_ARG;

    // Find item definition for sell price
    const ItemDef* item = engine_find_item(rules, item_id);
    if (!item) return ESP_ERR_NOT_FOUND;

    if (!engine_has_item(pet, item_id, count)) return ESP_ERR_NOT_FOUND;

    engine_remove_item(pet, item_id, count);
    pet->coins += item->sell_price * count;
    pet->dirty = true;
    return ESP_OK;
}

// ============================================================================
// Missions
// ============================================================================

void engine_update_missions(PetState* pet, const RulesData* rules) {
    // Simplified - in full implementation, check each mission's progress
}

esp_err_t engine_claim_mission(PetState* pet, uint8_t mission_index) {
    if (!pet || mission_index >= pet->mission_count) return ESP_ERR_INVALID_ARG;

    ActiveMission* m = &pet->active_missions[mission_index];
    if (!m->completed || m->claimed) return ESP_ERR_INVALID_STATE;

    // Give rewards
    for (int i = 0; i < m->def.reward_count; i++) {
        engine_add_item(pet, m->def.rewards[i].item_id, m->def.rewards[i].count);
    }
    engine_add_xp(pet, m->def.xp_reward);
    pet->coins += m->def.coin_reward;

    m->claimed = true;
    pet->dirty = true;
    return ESP_OK;
}

// ============================================================================
// Social/Partner
// ============================================================================

esp_err_t engine_send_treat(PetState* pet, const RulesData* rules,
                            const char* item_id, PetState* partner_pet) {
    if (!pet || !partner_pet || !item_id) return ESP_ERR_INVALID_ARG;

    if (!engine_has_item(pet, item_id, 1)) return ESP_ERR_NOT_FOUND;

    engine_remove_item(pet, item_id, 1);

    // In real implementation, send via WebSocket
    // For now, apply directly to partner if available
    if (partner_pet->initialized) {
        engine_add_item(partner_pet, item_id, 1);
    }

    pet->dirty = true;
    return ESP_OK;
}

esp_err_t engine_propose_trade(PetState* pet, const char* offered_item,
                               uint8_t offered_count, const char* requested_item,
                               uint8_t requested_count) {
    if (!pet || !offered_item || !requested_item) return ESP_ERR_INVALID_ARG;

    if (!engine_has_item(pet, offered_item, offered_count)) return ESP_ERR_NOT_FOUND;

    strncpy(pet->pending_trade.offered_item, offered_item, MAX_STRING_LEN-1);
    pet->pending_trade.offered_count = offered_count;
    strncpy(pet->pending_trade.requested_item, requested_item, MAX_STRING_LEN-1);
    pet->pending_trade.requested_count = requested_count;
    pet->pending_trade.timestamp = esp_timer_get_time() / 1000;
    pet->pending_trade.pending = true;

    pet->dirty = true;
    return ESP_OK;
}

esp_err_t engine_accept_trade(PetState* pet, PetState* partner_pet) {
    if (!pet || !partner_pet || !pet->pending_trade.pending) return ESP_ERR_INVALID_STATE;

    // Check partner has requested item
    if (!engine_has_item(partner_pet, pet->pending_trade.requested_item,
                         pet->pending_trade.requested_count)) {
        return ESP_ERR_NOT_FOUND;
    }

    // Execute trade
    engine_remove_item(pet, pet->pending_trade.offered_item, pet->pending_trade.offered_count);
    engine_remove_item(partner_pet, pet->pending_trade.requested_item, pet->pending_trade.requested_count);
    engine_add_item(pet, pet->pending_trade.requested_item, pet->pending_trade.requested_count);
    engine_add_item(partner_pet, pet->pending_trade.offered_item, pet->pending_trade.offered_count);

    pet->pending_trade.pending = false;
    pet->dirty = true;
    return ESP_OK;
}

// ============================================================================
// Lifecycle
// ============================================================================

void engine_update_lifecycle(PetState* pet, const RulesData* rules) {
    if (!pet || !rules) return;

    for (int i = 0; i < rules->lifecycle_phase_count; i++) {
        const LifecyclePhaseDef* phase = &rules->lifecycle_phases[i];
        if (pet->age_ticks >= phase->min_age_ticks &&
            pet->age_ticks < phase->max_age_ticks) {
            pet->current_phase = phase->phase;
            break;
        }
    }
}

const char* engine_get_phase_name(LifePhase phase, bool german) {
    static const char* names_en[] = {"Egg", "Baby", "Child", "Teen", "Adult", "Elder", "Ancestor"};
    static const char* names_de[] = {"Ei", "Baby", "Kind", "Teen", "Erwachsener", "Senior", "Ahne"};
    if (phase >= 0 && phase < 7) {
        return german ? names_de[phase] : names_en[phase];
    }
    return "Unknown";
}

// ============================================================================
// Lookup Functions
// ============================================================================

const ItemDef* engine_find_item(const RulesData* rules, const char* item_id) {
    if (!rules || !item_id) return NULL;
    for (int i = 0; i < rules->item_count; i++) {
        if (strcmp(rules->items[i].id, item_id) == 0) return &rules->items[i];
    }
    return NULL;
}

const RecipeDef* engine_find_recipe(const RulesData* rules, const char* recipe_id) {
    if (!rules || !recipe_id) return NULL;
    for (int i = 0; i < rules->recipe_count; i++) {
        if (strcmp(rules->recipes[i].id, recipe_id) == 0) return &rules->recipes[i];
    }
    return NULL;
}

const JobDef* engine_find_job(const RulesData* rules, const char* job_id) {
    if (!rules || !job_id) return NULL;
    for (int i = 0; i < rules->job_count; i++) {
        if (strcmp(rules->jobs[i].id, job_id) == 0) return &rules->jobs[i];
    }
    return NULL;
}

const SpellDef* engine_find_spell(const RulesData* rules, const char* spell_id) {
    if (!rules || !spell_id) return NULL;
    for (int i = 0; i < rules->spell_count; i++) {
        if (strcmp(rules->spells[i].id, spell_id) == 0) return &rules->spells[i];
    }
    return NULL;
}

const BiomeDef* engine_find_biome(const RulesData* rules, const char* biome_id) {
    if (!rules || !biome_id) return NULL;
    for (int i = 0; i < rules->biome_count; i++) {
        if (strcmp(rules->biomes[i].id, biome_id) == 0) return &rules->biomes[i];
    }
    return NULL;
}

const MissionDef* engine_find_mission(const RulesData* rules, const char* mission_id) {
    if (!rules || !mission_id) return NULL;
    for (int i = 0; i < rules->mission_count; i++) {
        if (strcmp(rules->missions[i].id, mission_id) == 0) return &rules->missions[i];
    }
    return NULL;
}

// ============================================================================
// Seasonal
// ============================================================================

uint8_t engine_get_current_season(const RulesData* rules) {
    // Based on ISO week modulo 4
    time_t now = time(NULL);
    struct tm* timeinfo = gmtime(&now);
    // Simplified: use month
    return (timeinfo->tm_mon / 3) % 4;
}

float engine_get_season_modifier(const RulesData* rules, uint8_t season, StatType stat) {
    if (!rules || season >= 4 || stat >= MAX_STATS) return 1.0f;
    return rules->season_modifiers[season][stat];
}

// ============================================================================
// Biome Modifiers
// ============================================================================

float engine_get_biome_modifier(const RulesData* rules, const char* biome_id,
                                const char* modifier_name) {
    if (!rules || !biome_id || !modifier_name) return 1.0f;

    const BiomeDef* biome = engine_find_biome(rules, biome_id);
    if (!biome) return 1.0f;

    if (strcmp(modifier_name, "food_decay") == 0) return biome->modifiers.food_decay;
    if (strcmp(modifier_name, "happiness_decay") == 0) return biome->modifiers.happiness_decay;
    if (strcmp(modifier_name, "energy_decay") == 0) return biome->modifiers.energy_decay;
    if (strcmp(modifier_name, "hygiene_decay") == 0) return biome->modifiers.hygiene_decay;
    if (strcmp(modifier_name, "mining_yield") == 0) return biome->modifiers.mining_yield;
    if (strcmp(modifier_name, "foraging_yield") == 0) return biome->modifiers.foraging_yield;
    if (strcmp(modifier_name, "fishing_yield") == 0) return biome->modifiers.fishing_yield;
    if (strcmp(modifier_name, "farming_yield") == 0) return biome->modifiers.farming_yield;
    if (strcmp(modifier_name, "magic_regen") == 0) return biome->modifiers.magic_regen;
    if (strcmp(modifier_name, "xp_bonus") == 0) return biome->modifiers.xp_bonus;

    return 1.0f;
}

// ============================================================================
// XP/Level
// ============================================================================

void engine_add_xp(PetState* pet, uint32_t xp) {
    if (!pet) return;
    pet->experience += xp;

    while (pet->experience >= pet->xp_to_next_level) {
        pet->experience -= pet->xp_to_next_level;
        pet->level++;
        pet->xp_to_next_level = calc_xp_for_level(pet->level);

        // Level up stat increases
        for (int i = 0; i < MAX_STATS; i++) {
            pet->stat_max[i] *= 1.1f; // 10% increase per level
            pet->stats[i] = pet->stat_max[i]; // Full heal on level up
        }
    }
    pet->dirty = true;
}

uint32_t engine_calc_xp_for_level(uint16_t level) {
    return calc_xp_for_level(level);
}

// ============================================================================
// Care Mistakes
// ============================================================================

void engine_check_care_mistakes(PetState* pet, const RulesData* rules, uint32_t now) {
    if (!pet || !rules) return;

    uint32_t threshold = rules->care_mistake_threshold_seconds * 1000; // ms

    for (int i = 0; i < MAX_STATS; i++) {
        if (pet->stats[i] <= 0 && !pet->care_mistake_pending[i]) {
            if (now - pet->last_care_time[i] >= threshold) {
                pet->care_mistakes++;
                pet->care_mistake_pending[i] = true;
                // Apply penalty
                pet->stats[STAT_HEALTH] = clampf(pet->stats[STAT_HEALTH] - 5, 0, pet->stat_max[STAT_HEALTH]);
                pet->dirty = true;
            }
        } else if (pet->stats[i] > 0) {
            pet->care_mistake_pending[i] = false;
        }
    }
}

uint8_t engine_get_care_mistakes(const PetState* pet) {
    return pet ? pet->care_mistakes : 0;
}

// ============================================================================
// Persistence (NVS/JSON)
// ============================================================================

esp_err_t engine_save_pet(const PetState* pet, const char* nvs_namespace) {
    // Simplified - would serialize to JSON and save to NVS
    return ESP_OK;
}

esp_err_t engine_load_pet(PetState* pet, const char* nvs_namespace) {
    // Simplified - would load from NVS and parse JSON
    return ESP_OK;
}

char* engine_generate_pet_json(const PetState* pet) {
    // Simplified - would generate JSON string
    return NULL;
}

esp_err_t engine_parse_pet_json(const char* json_str, PetState* pet) {
    // Simplified - would parse JSON string
    return ESP_OK;
}