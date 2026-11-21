#ifndef CYD_NET_H
#define CYD_NET_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

class TamagotchiCoreEngine;
struct CydConfig;

struct FriendState {
  bool isOnline;
  char deviceId[24];
  char name[24];
  char biome[16];
  char phase[12];
  uint8_t level;
  uint8_t health;
  uint8_t hunger;
  char ipAddress[20];
  uint16_t port;
  char speechBalloon[36];
  uint32_t balloonUntilMs;
  char linkMode[8]; // "OFF", "NET", "LAN"
};

struct NetPacket {
  char type[20];
  char from[24];
  char text[64];
  char tradeId[24];
  char offeredId[20];
  char offeredName[28];
  char requestedId[20];
  char requestedName[28];
};

typedef void (*OtaProgressCb)(const char* stage, int percent);
typedef void (*NetPacketCb)(const NetPacket& pkt);

void cyd_net_init_stack(void);
void cyd_net_mark_ota_valid(const CydConfig* cfg = nullptr);

bool cyd_net_connect_sta(const CydConfig& cfg, char* outIpStr, size_t ipLen);
void cyd_net_stop_all(void);
void cyd_net_start_ap_portal(CydConfig& cfg);
void cyd_net_stop_ap_portal(void);

bool cyd_net_is_wifi_connected(void);
bool cyd_net_test_php_server(const CydConfig& cfg);
bool cyd_net_sync_php_server(const CydConfig& cfg, const TamagotchiCoreEngine& engine, FriendState& friendPet);

bool cyd_net_cloud_save(
  const CydConfig& cfg,
  const TamagotchiCoreEngine& engine,
  uint16_t recipesCrafted = 0,
  uint16_t jobsCompleted = 0,
  uint16_t spellsCast = 0,
  uint16_t treatsSent = 0,
  uint32_t* outUpdatedAt = nullptr
);
bool cyd_net_cloud_load(
  const CydConfig& cfg,
  TamagotchiCoreEngine& engine,
  uint16_t& recipesCrafted,
  uint16_t& jobsCompleted,
  uint16_t& spellsCast,
  uint16_t& treatsSent,
  char* outTrainerName = nullptr,
  size_t trainerNameLen = 0,
  uint32_t* outUpdatedAt = nullptr
);

// Validates shop purchase against php_server/shop.php when online.
// Returns: 1 = approved by server, 0 = rejected by server (402/403/404), -1 = offline/unreachable (allow local fallback)
int cyd_net_shop_purchase(
  const CydConfig& cfg,
  const char* itemType,
  const char* itemId,
  uint16_t currentCoins,
  uint8_t currentLevel,
  char* outErrMsg,
  size_t errLen
);

void cyd_net_start_p2p_server(uint16_t port);
void cyd_net_poll_p2p_socket(NetPacketCb onPacket);
void cyd_net_poll_relay(const CydConfig& cfg, NetPacketCb onPacket);

bool cyd_net_send_packet_ex(const CydConfig& cfg, FriendState& friendPet, const NetPacket& pkt);
void cyd_net_send_packet(const CydConfig& cfg, FriendState& friendPet, const char* type, const char* textPayload);

void cyd_net_report_ota(const CydConfig& cfg, const char* event, const char* detail);

// SNTP Time Sync (pool.ntp.org, Europe/Berlin)
void cyd_net_init_sntp(void);
bool cyd_net_get_berlin_time(int* outIsoWeek, uint32_t* outYyyymmdd, int* outHour);

// Returns: 1 = updated/reloading, 0 = no update, -1 = error
int cyd_net_check_ota(
  const CydConfig& cfg,
  TamagotchiCoreEngine& engine,
  OtaProgressCb progressCb,
  char* outMessage,
  size_t outMsgLen
);

#endif // CYD_NET_H
