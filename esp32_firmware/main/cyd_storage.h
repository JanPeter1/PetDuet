#ifndef CYD_STORAGE_H
#define CYD_STORAGE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

class TamagotchiCoreEngine;

struct CydConfig {
  char wifiSsid[64];
  char wifiPass[64];
  char deviceId[24];
  char trainerName[24];
  char friendId[24];
  char biome[16];
  char serverUrl[160];
};

bool cyd_storage_init(CydConfig& cfg);
void cyd_storage_load_config(CydConfig& cfg);
void cyd_storage_save_config(const CydConfig& cfg);

bool cyd_storage_has_pet_save(void);
void cyd_storage_load_pet(TamagotchiCoreEngine& engine, uint16_t& recipesCrafted, uint16_t& jobsCompleted, uint16_t& spellsCast, uint16_t& treatsSent);
void cyd_storage_save_pet(const TamagotchiCoreEngine& engine, uint16_t recipesCrafted, uint16_t jobsCompleted, uint16_t spellsCast, uint16_t treatsSent);

uint32_t cyd_storage_load_cloud_ts(void);
void cyd_storage_save_cloud_ts(uint32_t epochSec);

bool cyd_storage_load_rules(TamagotchiCoreEngine& engine);
bool cyd_storage_save_rules(const char* data, size_t len, TamagotchiCoreEngine& engine);

#endif // CYD_STORAGE_H
