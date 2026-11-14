#ifndef CYD_NET_H
#define CYD_NET_H

#include <stdbool.h>
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
};

typedef void (*OtaProgressCb)(const char* stage, int percent);
typedef void (*NetPacketCb)(const char* type, const char* text);

void cyd_net_init_stack(void);
void cyd_net_mark_ota_valid(void);

bool cyd_net_connect_sta(const CydConfig& cfg, char* outIpStr, size_t ipLen);
void cyd_net_stop_all(void);
void cyd_net_start_ap_portal(CydConfig& cfg);
void cyd_net_stop_ap_portal(void);

bool cyd_net_is_wifi_connected(void);
bool cyd_net_test_php_server(const CydConfig& cfg);
bool cyd_net_sync_php_server(const CydConfig& cfg, const TamagotchiCoreEngine& engine, FriendState& friendPet);
bool cyd_net_cloud_save(const CydConfig& cfg, const TamagotchiCoreEngine& engine);

void cyd_net_start_p2p_server(uint16_t port);
void cyd_net_poll_p2p_socket(NetPacketCb onPacket);
void cyd_net_send_packet(const CydConfig& cfg, const FriendState& friendPet, const char* type, const char* textPayload);

// Returns: 1 = updated/reloading, 0 = no update, -1 = error
int cyd_net_check_ota(
  const CydConfig& cfg,
  TamagotchiCoreEngine& engine,
  OtaProgressCb progressCb,
  char* outMessage,
  size_t outMsgLen
);

#endif // CYD_NET_H
