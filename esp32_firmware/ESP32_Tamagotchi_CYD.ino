/**
 * ============================================================================
 * ESP32 CYD (ESP32-2432S028R) Retro Tamagotchi Dual-Pet RPG Companion
 * Hardware Reference: https://github.com/JanPeter1/ESP_CoGo
 * ============================================================================
 * Features:
 *  - 2.8" TFT LCD: 320x240 ILI9341 SPI (Landscape Mode 1)
 *  - Resistive Touch: XPT2046 SPI (Calibrated coordinates)
 *  - Onboard RGB LED: RED=GPIO 4, GREEN=GPIO 16, BLUE=GPIO 17 (Active LOW)
 *  - Onboard Speaker: GPIO 26 (LEDC Tone Generator)
 *  - WiFi AP Config Web Portal: SoftAP mode at 192.168.4.1 to configure WiFi,
 *    device IDs, biomes, and server URLs without reflashing!
 *  - NVS Persistence: Uses ESP32 Preferences to store credentials & RPG save
 *  - Direct TCP Socket (Port 8888) + PHP Server Sync
 * ============================================================================
 *
 * CYD DISPLAY HARDWARE CONFIGURATION (TFT_eSPI):
 * ----------------------------------------------
 * Driver:        ILI9341_2_DRIVER  (alternative driver for CYD panel orientation)
 * Rotation:      tft.setRotation(1)  (90° landscape)
 * Color Fix:     tft.writecommand(0x21)  // Display Inversion ON (fixes white bg/black text)
 * Touch:         touch.setRotation(1)  (matches display)
 *
 * PINS (from CYD_Pins.h / platformio.ini):
 *   TFT:   MOSI=13, MISO=12, SCLK=14, CS=15, DC=2, BL=21, RST=-1
 *   Touch: MOSI=32, MISO=39, CLK=25, CS=33, IRQ=36
 *   LED:   R=4, G=16, B=17 (Active LOW)
 *   Buzzer: GPIO 26
 *   Boot Button: GPIO 0
 *
 * PLATFORMIO.INI BUILD FLAGS:
 *   -D ILI9341_2_DRIVER=1
 *   -D TFT_WIDTH=320 -D TFT_HEIGHT=240
 *   -D TFT_MISO=12 -D TFT_MOSI=13 -D TFT_SCLK=14 -D TFT_CS=15 -D TFT_DC=2 -D TFT_RST=-1 -D TFT_BL=21
 *   -D SPI_FREQUENCY=40000000
 * ============================================================================
 */

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiServer.h>
#include <WebServer.h>      // Built-in ESP32 WebServer for AP configuration portal
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Preferences.h>    // Non-Volatile Storage (NVS) for persistent settings
#include <SPI.h>
#include <FS.h>
#include <SPIFFS.h>         // SPIFFS for dynamic rules.json storage
#include <Update.h>         // ESP32 A/B Partition OTA updater
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>

#include "CYD_Pins.h"
#include "config.h"
#include "PetGraphics.h"
#include "TamagotchiCoreEngine.h"


// ----------------------------------------------------------------------------
// Hardware Peripheral Instances
// ----------------------------------------------------------------------------
TFT_eSPI tft = TFT_eSPI();
SPIClass touchSPI = SPIClass(VSPI);
XPT2046_Touchscreen touch(XPT2046_CS, XPT2046_IRQ);
WiFiServer socketServer(SOCKET_PORT);
WebServer apServer(80); // Web portal on port 80
Preferences prefs;      // Persistent storage for WiFi & device config
TamagotchiCoreEngine engine; // Core logic, Daily Missions & Shop engine


// ----------------------------------------------------------------------------
// Runtime Configuration Strings (Loaded from NVS or config.h)
// ----------------------------------------------------------------------------
String cfg_wifi_ssid     = WIFI_SSID;
String cfg_wifi_pass     = WIFI_PASSWORD;
String cfg_device_id     = DEVICE_ID;
String cfg_trainer_name  = TRAINER_NAME;
String cfg_friend_id     = FRIEND_DEV_ID;
String cfg_biome         = DEFAULT_BIOME;
String cfg_server_url    = PHP_SERVER_BASE_URL;

bool inAPConfigMode = false;

// ----------------------------------------------------------------------------
// Bilingual Localization Macros (English & Deutsch)
// ----------------------------------------------------------------------------
#if LANG_DE
  #define TXT_FEED       "FUTT"
  #define TXT_PLAY       "SPIEL"
  #define TXT_BATH       "BAD"
  #define TXT_MAGIC      "ZAUB"
  #define TXT_SAVE       "SPEICH"
  #define TXT_STATS      "STAT"
  #define TXT_TREAT      "SNACK"
  #define TXT_CHAT       "CHAT"
  #define TXT_MAGIC_PLUS "MGC+"
  #define TXT_QUEST      "QUEST"
  #define TXT_TRADE      "TAUS."
  #define TXT_ONLINE     "ONLINE"
  #define TXT_ASLEEP     "SCHLAFT"
  #define TXT_TAP_EGG    "TIPPEN!"

  #define TXT_BANNER_FED             "Gefuettert! +25 Hunger"
  #define TXT_BANNER_PLAY            "Gespielt! +20 Freude"
  #define TXT_BANNER_BATH            "Sauber gebadet! 🫧"
  #define TXT_BANNER_AURA            "Heilaura gewirkt! ✨"
  #define TXT_BANNER_NO_MANA20       "Braucht 20 Mana! ⚡"
  #define TXT_BANNER_NO_MANA30       "Braucht 30 Mana! ⚡"
  #define TXT_BANNER_SAVED           "Cloud gespeichert & OTA! 💾"
  #define TXT_BANNER_TREAT_SENT      "Snack gesendet! 🍩"
  #define TXT_BANNER_CHAT_SENT       "Chat gesendet! 💬"
  #define TXT_BANNER_BLESSING        "Segen gesendet! 🌟"
  #define TXT_BANNER_QUEST           "Koop-Quest fertig! +$35 ⚔️"
  #define TXT_BANNER_TRADE           "Tausch gesendet! 🎁"
  #define TXT_BANNER_OTA_CHECK       "Pruefe auf OTA-Update... ⚡"
  #define TXT_BANNER_STREAK_CLAIMED  "Streak-Bonus Tag "
  #define TXT_BANNER_MISSION_CLAIMED "Mission eingeloest! +Coins & XP 🎁"
  #define TXT_BANNER_SHOP_COOKIE     "Sternenkeks gekauft! 🍪"
  #define TXT_BANNER_EGG_HATCHED     "EI GESCHLUEPFT! Baby da! 🐣"
  #define TXT_BANNER_EGG_WARMED      "Ei gewaermt! Weiter tippen 🥚"
  #define TXT_BANNER_CUDDLED         "Haustier gestreichelt! ❤️"
#else
  #define TXT_FEED       "FEED"
  #define TXT_PLAY       "PLAY"
  #define TXT_BATH       "BATH"
  #define TXT_MAGIC      "MGC"
  #define TXT_SAVE       "SAVE"
  #define TXT_STATS      "STAT"
  #define TXT_TREAT      "TREAT"
  #define TXT_CHAT       "CHAT"
  #define TXT_MAGIC_PLUS "MGC+"
  #define TXT_QUEST      "QUEST"
  #define TXT_TRADE      "TRADE"
  #define TXT_ONLINE     "ONLINE"
  #define TXT_ASLEEP     "ASLEEP"
  #define TXT_TAP_EGG    "TAP EGG!"

  #define TXT_BANNER_FED             "Fed Meal! +25 Hunger"
  #define TXT_BANNER_PLAY            "Played Minigame! +20 Happy"
  #define TXT_BANNER_BATH            "Clean Bubble Bath! 🫧"
  #define TXT_BANNER_AURA            "Vitality Aura Cast! ✨"
  #define TXT_BANNER_NO_MANA20       "Need 20 Mana! ⚡"
  #define TXT_BANNER_NO_MANA30       "Need 30 Mana! ⚡"
  #define TXT_BANNER_SAVED           "Cloud Saved & OTA Checked! 💾"
  #define TXT_BANNER_TREAT_SENT      "Treat Sent to Friend! 🍩"
  #define TXT_BANNER_CHAT_SENT       "Chat Beamed over Socket! 💬"
  #define TXT_BANNER_BLESSING        "Blessing Beamed to Friend! 🌟"
  #define TXT_BANNER_QUEST           "Co-op Quest Complete! +$35 ⚔️"
  #define TXT_BANNER_TRADE           "Trade Proposal Sent! 🎁"
  #define TXT_BANNER_OTA_CHECK       "Checking Server for OTA... ⚡"
  #define TXT_BANNER_STREAK_CLAIMED  "Streak Bonus Day "
  #define TXT_BANNER_MISSION_CLAIMED "Mission Claimed! +Coins & XP 🎁"
  #define TXT_BANNER_SHOP_COOKIE     "Shop: Star Cookie Bought! 🍪"
  #define TXT_BANNER_EGG_HATCHED     "EGG HATCHED! Baby Born! 🐣"
  #define TXT_BANNER_EGG_WARMED      "Egg Warmed! Keep Tapping 🥚"
  #define TXT_BANNER_CUDDLED         "Pet Cuddled! ❤️"
#endif

// ----------------------------------------------------------------------------
// Local Pet Data Structure (RPG Stats, Evolution & Biome)
// ----------------------------------------------------------------------------
struct LocalPet {
  char name[16] = "Leafkin";
  char biome[12] = "grassland";
  char phase[10] = "Egg"; // Egg -> Baby -> Youth -> Adult -> Elder
  uint8_t level = 1;
  uint16_t xp = 0;
  uint16_t xpNext = 100;
  uint8_t health = 100;
  uint8_t hunger = 85;
  uint8_t energy = 90;
  uint8_t hygiene = 95;
  uint8_t happiness = 90;
  uint8_t mana = 50;
  uint16_t coins = 45;
  uint8_t eggWarmth = 20; // 0..100% to hatch
  uint16_t recipesCrafted = 0;
  uint16_t jobsCompleted = 0;
  uint16_t spellsCast = 0;
  uint16_t treatsSent = 0;
} myPet;

// ----------------------------------------------------------------------------
// Remote Friend Pet State (Synchronized via Direct Sockets / PHP Server)
// ----------------------------------------------------------------------------
struct FriendPet {
  bool isOnline = false;
  char deviceId[16] = "CYD-DEV2";
  char name[16] = "Sunscale";
  char biome[12] = "desert";
  char phase[10] = "Baby";
  uint8_t level = 2;
  uint8_t health = 95;
  uint8_t hunger = 80;
  char ipAddress[18] = "";
  uint16_t port = SOCKET_PORT;
  char speechBalloon[28] = "Hello Buddy! ^_^";
  unsigned long balloonTimeout = 0;
  unsigned long lastSeen = 0;
} friendPet;

// Timing & UI Navigation State
unsigned long lastTickTime = 0;
unsigned long lastServerSyncTime = 0;
unsigned long lastOtaCheckTime = 0;
unsigned long lastRenderTime = 0;
unsigned long lastAutoSaveTime = 0;
unsigned long lastJobCountdownRefresh = 0;
const unsigned long AUTO_SAVE_INTERVAL_MS = 120000; // Auto-save every 2 minutes
int8_t activeJobIdx = -1;                           // -1 = idle, 0 = Forager (60s), 1 = Scout (120s), 2 = Miner (180s)
unsigned long activeJobEndMs = 0;
bool feedMenuOpen = false;   // Horizontal food flyout beside FEED button
bool chatMenuOpen = false;   // Quick-Chat preset selector modal overlay
bool inSubScreen = false;
uint8_t subScreenTab = 0;    // 0 = STATS, 1 = BAG/COOK/SHOP, 2 = JOBS, 3 = MAGIC, 4 = QUESTS
uint8_t bagSubMode = 0;      // 0 = Bag, 1 = Cook Recipes, 2 = Shop/Bazar
bool screenNeedsRefresh = true;

// 8 Bilingual Quick-Chat Presets (ASCII-safe <=18 chars to fit CYD buttons & friend speech balloon)
const char* getChatPreset(uint8_t idx) {
  static const char* const PRESETS_DE[8] = {
    "Hallo Freund! :D",
    "Lass uns Questen!",
    "Schicke Snacks!",
    "Wirke Zauber! *",
    "Toller Hut! ^_^",
    "Pet hat Hunger!",
    "Lass uns tauschen!",
    "Super CYD Setup!"
  };
  static const char* const PRESETS_EN[8] = {
    "Hello friend! :)",
    "Co-op quest time!",
    "Sending treats!",
    "Cast magic spell!",
    "Nice hat! ^_^",
    "My pet is hungry!",
    "Let's trade gear!",
    "Awesome CYD! :D"
  };
  if (idx >= 8) idx = 0;
  return LANG_DE ? PRESETS_DE[idx] : PRESETS_EN[idx];
}
char activeBannerMessage[40] = "";
unsigned long bannerTimeout = 0;
bool inServerErrorState = false;


// ----------------------------------------------------------------------------
// RGB LED Control Helper (Active LOW: LOW = ON, HIGH = OFF)
// ----------------------------------------------------------------------------
void setRGB(bool r, bool g, bool b) {
  digitalWrite(CYD_LED_RED,   r ? LOW : HIGH);
  digitalWrite(CYD_LED_GREEN, g ? LOW : HIGH);
  digitalWrite(CYD_LED_BLUE,  b ? LOW : HIGH);
}

// ----------------------------------------------------------------------------
// 8-bit Audio Synthesizer (GPIO 26 Buzzer)
// ----------------------------------------------------------------------------
void playTone(uint16_t freq, uint16_t durationMs) {
  ledcWriteTone(BUZZER_LEDC_CH, freq);
  delay(durationMs);
  ledcWriteTone(BUZZER_LEDC_CH, 0);
}

void soundClick() {
  playTone(1000, 20);
}

void soundMunch() {
  playTone(350, 30); delay(15);
  playTone(500, 35); delay(15);
  playTone(650, 45);
}

void soundTreatSent() {
  setRGB(false, true, true); // Cyan flash
  playTone(523, 50); delay(20);
  playTone(659, 50); delay(20);
  playTone(784, 90);
  setRGB(false, true, false);
}

void soundTreatReceived() {
  setRGB(false, false, true); // Blue flash
  playTone(659, 70); delay(30);
  playTone(880, 120);
  setRGB(false, true, false);
}

void soundLevelUp() {
  setRGB(true, true, false); // Gold
  uint16_t notes[] = {523, 659, 784, 1046, 1318};
  for (int i = 0; i < 5; i++) {
    playTone(notes[i], 70);
    delay(20);
  }
  setRGB(false, true, false);
}

void soundHatch() {
  setRGB(true, true, true); // White flash
  uint16_t notes[] = {440, 554, 659, 880, 1108};
  for (int i = 0; i < 5; i++) {
    playTone(notes[i], 80);
    delay(25);
  }
  setRGB(false, true, false);
}

void soundWarning() {
  setRGB(true, false, false);
  playTone(220, 90); delay(20);
  playTone(170, 120);
  setRGB(false, true, false);
}

void showBanner(const char* msg, uint16_t durationMs = 2500) {
  strncpy(activeBannerMessage, msg, sizeof(activeBannerMessage) - 1);
  activeBannerMessage[sizeof(activeBannerMessage) - 1] = '\0';
  bannerTimeout = millis() + durationMs;
  screenNeedsRefresh = true;
}

void showBanner(const String& msg, uint16_t durationMs = 2500) {
  showBanner(msg.c_str(), durationMs);
}

void syncToEngine() {
  strncpy(engine.name, myPet.name, sizeof(engine.name) - 1);
  strncpy(engine.biome, myPet.biome, sizeof(engine.biome) - 1);
  strncpy(engine.phase, myPet.phase, sizeof(engine.phase) - 1);
  engine.level = myPet.level;
  engine.xp = myPet.xp;
  engine.xpNext = myPet.xpNext;
  engine.coins = myPet.coins;
  engine.eggWarmth = myPet.eggWarmth;
  engine.stats.health = myPet.health;
  engine.stats.hunger = myPet.hunger;
  engine.stats.energy = myPet.energy;
  engine.stats.hygiene = myPet.hygiene;
  engine.stats.happiness = myPet.happiness;
  engine.stats.mana = myPet.mana;
}

void syncFromEngine() {
  strncpy(myPet.phase, engine.phase, sizeof(myPet.phase) - 1);
  myPet.level = engine.level;
  myPet.xp = engine.xp;
  myPet.xpNext = engine.xpNext;
  myPet.coins = engine.coins;
  myPet.eggWarmth = engine.eggWarmth;
  myPet.health = engine.stats.health;
  myPet.hunger = engine.stats.hunger;
  myPet.energy = engine.stats.energy;
  myPet.hygiene = engine.stats.hygiene;
  myPet.happiness = engine.stats.happiness;
  myPet.mana = engine.stats.mana;
}

// ----------------------------------------------------------------------------
// Forward Declarations
// ----------------------------------------------------------------------------
void renderCYDScreen();
void renderSubScreen();
void handleTouchPress(uint16_t x, uint16_t y);
void pollDirectSockets();
void sendDirectSocketPacket(const char* type, const char* textPayload = "");
void syncWithPHPServer();
void checkLevelUp();
void loadSettingsFromNVS();
void saveSettingsToNVS();
void startAPConfigPortal();
void renderAPPortalScreen();
bool testPHPServerConnection();
void showServerErrorScreen();
void loadRulesFromSPIFFS();
void checkForOTAUpdate();

// ============================================================================
// NVS SETTINGS LOAD / SAVE (ESP32 Preferences)
// ============================================================================
void loadSettingsFromNVS() {
  prefs.begin("cyd_pet", false);
  cfg_wifi_ssid    = prefs.getString("ssid", WIFI_SSID);
  cfg_wifi_pass    = prefs.getString("pass", WIFI_PASSWORD);
  cfg_device_id    = prefs.getString("dev_id", DEVICE_ID);
  cfg_trainer_name = prefs.getString("trainer", TRAINER_NAME);
  cfg_friend_id    = prefs.getString("friend", FRIEND_DEV_ID);
  cfg_biome        = prefs.getString("biome", DEFAULT_BIOME);
  cfg_server_url   = prefs.getString("srv_url", PHP_SERVER_BASE_URL);

  // Apply to runtime structs
  strncpy(myPet.biome, cfg_biome.c_str(), sizeof(myPet.biome) - 1);
  strncpy(friendPet.deviceId, cfg_friend_id.c_str(), sizeof(friendPet.deviceId) - 1);
  prefs.end();

  Serial.println("[NVS] Loaded configuration from flash memory:");
  Serial.printf("  SSID: %s, DeviceID: %s, Biome: %s\n", cfg_wifi_ssid.c_str(), cfg_device_id.c_str(), cfg_biome.c_str());
}

void saveSettingsToNVS() {
  prefs.begin("cyd_pet", false);
  prefs.putString("ssid", cfg_wifi_ssid);
  prefs.putString("pass", cfg_wifi_pass);
  prefs.putString("dev_id", cfg_device_id);
  prefs.putString("trainer", cfg_trainer_name);
  prefs.putString("friend", cfg_friend_id);
  prefs.putString("biome", cfg_biome);
  prefs.putString("srv_url", cfg_server_url);
  prefs.end();
  Serial.println("[NVS] Configuration saved successfully to flash memory.");
}

// ============================================================================
// WIFI ACCESS POINT & WEB CONFIGURATION PORTAL (192.168.4.1)
// ============================================================================
void startAPConfigPortal() {
  inAPConfigMode = true;
  setRGB(false, false, true); // Solid blue for AP mode

  // Disconnect any active STA connection
  WiFi.disconnect(true);
  delay(100);

  // Scan surrounding 2.4GHz WiFi networks
  int n = WiFi.scanNetworks();
  String wifiOptions = "";
  for (int i = 0; i < n; ++i) {
    String ssid = WiFi.SSID(i);
    int rssi = WiFi.RSSI(i);
    wifiOptions += "<option value='" + ssid + "'>" + ssid + " (" + rssi + " dBm)</option>";
  }

  // Start Access Point
  String apSSID = "CYD-Tamagotchi-AP";
  WiFi.mode(WIFI_AP);
  WiFi.softAP(apSSID.c_str());
  IPAddress apIP = WiFi.softAPIP();

  Serial.println("\n[AP Portal] Started Access Point!");
  Serial.printf("  SSID: %s\n", apSSID.c_str());
  Serial.printf("  Web Portal IP: %s\n", apIP.toString().c_str());

  // Render AP info on the 2.8" LCD
  renderAPPortalScreen();

  // Root Web Portal Page Handler
  apServer.on("/", HTTP_GET, [wifiOptions]() {
    String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>CYD Tamagotchi WiFi Setup</title>
  <style>
    body { font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, sans-serif; background: #0f172a; color: #f8fafc; padding: 1.5rem; margin: 0; }
    .card { max-width: 440px; margin: 0 auto; background: #1e293b; border: 1px solid #334155; border-radius: 12px; padding: 1.5rem; box-shadow: 0 10px 25px rgba(0,0,0,0.5); }
    h2 { color: #f59e0b; margin-top: 0; font-size: 1.3rem; display: flex; align-items: center; gap: 8px; }
    label { font-size: 0.8rem; font-weight: 600; color: #94a3b8; display: block; margin-top: 0.8rem; margin-bottom: 0.2rem; }
    input, select { width: 100%; box-sizing: border-box; padding: 10px; border-radius: 8px; border: 1px solid #475569; background: #0f172a; color: #f8fafc; font-size: 0.9rem; }
    input:focus, select:focus { outline: none; border-color: #f59e0b; }
    .btn { width: 100%; margin-top: 1.4rem; padding: 12px; background: #f59e0b; color: #0f172a; border: none; border-radius: 8px; font-weight: 700; font-size: 1rem; cursor: pointer; transition: 0.2s; }
    .btn:hover { background: #d97706; }
    .note { font-size: 0.75rem; color: #64748b; margin-top: 0.8rem; text-align: center; }
  </style>
</head>
<body>
  <div class="card">
    <h2>👾 CYD WiFi & RPG Config</h2>
    <form action="/save" method="POST">
      <label>1. Select WiFi Network (2.4GHz):</label>
      <select name="ssid" id="ssid_sel" onchange="document.getElementById('ssid_txt').value=this.value;">
        <option value="">-- Choose Scanned Network --</option>
)rawliteral" + wifiOptions + R"rawliteral(
      </select>
      <input type="text" name="ssid_custom" id="ssid_txt" placeholder="Or enter SSID manually..." style="margin-top:6px;" value=")rawliteral" + cfg_wifi_ssid + R"rawliteral(">

      <label>2. WiFi Password:</label>
      <input type="password" name="password" placeholder="Enter WiFi password..." value=")rawliteral" + cfg_wifi_pass + R"rawliteral(">

      <label>3. Your CYD Device ID (e.g. CYD-DEV1):</label>
      <input type="text" name="device_id" value=")rawliteral" + cfg_device_id + R"rawliteral(">

      <label>4. Trainer Name:</label>
      <input type="text" name="trainer_name" value=")rawliteral" + cfg_trainer_name + R"rawliteral(">

      <label>5. Friend's CYD Device ID (e.g. CYD-DEV2):</label>
      <input type="text" name="friend_id" value=")rawliteral" + cfg_friend_id + R"rawliteral(">

      <label>6. Startup Biome:</label>
      <select name="biome">
        <option value="grassland")rawliteral" + String(cfg_biome == "grassland" ? " selected" : "") + R"rawliteral(>Grassland (Green - Balanced & Herbs)</option>
        <option value="desert")rawliteral" + String(cfg_biome == "desert" ? " selected" : "") + R"rawliteral(>Desert (Yellow - Solar Energy & Heat)</option>
        <option value="water")rawliteral" + String(cfg_biome == "water" ? " selected" : "") + R"rawliteral(>Water (Blue - Tidal Magic & Hydration)</option>
        <option value="forest")rawliteral" + String(cfg_biome == "forest" ? " selected" : "") + R"rawliteral(>Forest (Brown - Stamina & Wood Craft)</option>
      </select>

      <label>7. PHP Server Base URL:</label>
      <input type="text" name="server_url" value=")rawliteral" + cfg_server_url + R"rawliteral(">

      <button type="submit" class="btn">Save & Connect 🚀</button>
      <div class="note">CYD will save settings to flash memory and reboot automatically.</div>
    </form>
  </div>
</body>
</html>
)rawliteral";
    apServer.send(200, "text/html", html);
  });

  // Handle Form Submission POST /save
  apServer.on("/save", HTTP_POST, []() {
    String new_ssid = apServer.hasArg("ssid_custom") && apServer.arg("ssid_custom").length() > 0
                        ? apServer.arg("ssid_custom")
                        : apServer.arg("ssid");
    String new_pass = apServer.arg("password");
    String new_dev  = apServer.arg("device_id");
    String new_trn  = apServer.arg("trainer_name");
    String new_frn  = apServer.arg("friend_id");
    String new_bio  = apServer.arg("biome");
    String new_srv  = apServer.arg("server_url");

    if (new_ssid.length() > 0) cfg_wifi_ssid = new_ssid;
    if (new_pass.length() > 0) cfg_wifi_pass = new_pass;
    if (new_dev.length() > 0)  cfg_device_id = new_dev;
    if (new_trn.length() > 0)  cfg_trainer_name = new_trn;
    if (new_frn.length() > 0)  cfg_friend_id = new_frn;
    if (new_bio.length() > 0)  cfg_biome = new_bio;
    if (new_srv.length() > 0)  cfg_server_url = new_srv;

    saveSettingsToNVS();

    String successHtml = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>Saved!</title>
  <style>
    body { font-family: sans-serif; background: #0f172a; color: #10b981; text-align: center; padding: 3rem 1rem; }
    .card { max-width: 380px; margin: 0 auto; background: #1e293b; padding: 2rem; border-radius: 12px; }
  </style>
</head>
<body>
  <div class="card">
    <h2>✓ Settings Saved!</h2>
    <p style="color:#e2e8f0;">Connecting to <strong>)rawliteral" + cfg_wifi_ssid + R"rawliteral(</strong>...</p>
    <p style="color:#94a3b8; font-size:0.85rem;">You may now close this page. Look at your CYD display!</p>
  </div>
</body>
</html>
)rawliteral";
    apServer.send(200, "text/html", successHtml);

    delay(1000);
    ESP.restart(); // Reboot into Station mode
  });

  apServer.begin();
}

void renderAPPortalScreen() {
  tft.fillScreen(TFT_BLACK);
  tft.fillRoundRect(8, 8, 304, 224, 8, COLOR_UI_PANEL);
  tft.drawRoundRect(8, 8, 304, 224, 8, TFT_GOLD);

  tft.setTextColor(TFT_YELLOW, COLOR_UI_PANEL);
  tft.setTextDatum(MC_DATUM);
  tft.drawString("WIFI SETUP MODE", 160, 28, 4);

  tft.setTextColor(TFT_WHITE, COLOR_UI_PANEL);
  tft.drawString("1. Connect your phone or laptop to AP:", 160, 60, 2);

  tft.setTextColor(TFT_CYAN, COLOR_UI_PANEL);
  tft.drawString("SSID: CYD-Tamagotchi-AP", 160, 84, 4);

  tft.setTextColor(TFT_WHITE, COLOR_UI_PANEL);
  tft.drawString("2. Open your web browser to:", 160, 118, 2);

  tft.setTextColor(TFT_GREEN, COLOR_UI_PANEL);
  tft.drawString("http://192.168.4.1", 160, 144, 4);

  tft.setTextColor(TFT_LIGHTGREY, COLOR_UI_PANEL);
  tft.drawString("Select WiFi & enter password in the web page.", 160, 180, 2);

  tft.setTextColor(COLOR_HEART_RED, COLOR_UI_PANEL);
  tft.drawString("Tap screen to cancel & run offline", 160, 210, 1);
}

// ============================================================================
// PHP SERVER CONNECTIVITY TEST
// ============================================================================
bool testPHPServerConnection() {
  HTTPClient http;
  String testUrl = cfg_server_url + "/register.php";
  http.begin(testUrl);
  http.addHeader("Content-Type", "application/json");
  http.setTimeout(5000);

  StaticJsonDocument<256> doc;
  doc["deviceId"] = cfg_device_id;
  doc["deviceName"] = "test";
  String body;
  serializeJson(doc, body);

  int httpCode = http.POST(body);
  http.end();

  if (httpCode == 200) {
    Serial.println("[Server] PHP Server reachable!");
    return true;
  }
  Serial.printf("[Server] PHP Server unreachable! HTTP Code: %d\n", httpCode);
  return false;
}

void showServerErrorScreen() {
  setRGB(true, false, true); // Magenta = WiFi OK but Server unreachable

  tft.fillScreen(TFT_BLACK);
  tft.fillRoundRect(8, 8, 304, 224, 8, COLOR_UI_PANEL);
  tft.drawRoundRect(8, 8, 304, 224, 8, TFT_RED);

  tft.setTextColor(TFT_RED, COLOR_UI_PANEL);
  tft.setTextDatum(MC_DATUM);
  tft.drawString("⚠ SERVER UNREACHABLE ⚠", 160, 30, 3);

  tft.setTextColor(TFT_WHITE, COLOR_UI_PANEL);
  tft.setTextDatum(MC_DATUM);
  tft.drawString("WiFi Connected Successfully!", 160, 65, 2);

  tft.setTextColor(TFT_YELLOW, COLOR_UI_PANEL);
  tft.drawString("IP: " + WiFi.localIP().toString(), 160, 95, 2);

  tft.setTextColor(TFT_CYAN, COLOR_UI_PANEL);
  tft.drawString("Server URL:", 160, 130, 1);
  tft.setTextColor(TFT_WHITE, COLOR_UI_PANEL);
  tft.drawString(cfg_server_url, 160, 150, 1);

  tft.setTextColor(TFT_ORANGE, COLOR_UI_PANEL);
  tft.drawString("Fix server URL in Config Portal:", 160, 180, 1);

  tft.setTextColor(TFT_GREEN, COLOR_UI_PANEL);
  tft.drawString("1. Tap screen to open AP Portal", 160, 205, 1);
  tft.drawString("2. Connect to: CYD-Tamagotchi-AP", 160, 220, 1);
  tft.drawString("3. Browse to: http://192.168.4.1", 160, 232, 1);
}

// ============================================================================
// Arduino SETUP
// ============================================================================
void setup() {
  Serial.begin(115200);
  delay(100);
  Serial.println("\n[CYD Tamagotchi] Booting ESP_CoGo 2.8\" Display...");

  // 1. Initialize Onboard RGB LED & Speaker
  pinMode(CYD_LED_RED, OUTPUT);
  pinMode(CYD_LED_GREEN, OUTPUT);
  pinMode(CYD_LED_BLUE, OUTPUT);
  setRGB(true, true, false); // Yellow boot indicator

  ledcSetup(BUZZER_LEDC_CH, 2000, BUZZER_LEDC_RES);
  ledcAttachPin(CYD_SPEAKER_PIN, BUZZER_LEDC_CH);
  playTone(880, 60);

  // 2. Initialize TFT Display (ILI9341) via TFT_eSPI
  pinMode(CYD_TFT_BL, OUTPUT);
  digitalWrite(CYD_TFT_BL, LOW); // Backlight OFF during init

  // Initialize TFT_eSPI (handles SPI, ILI9341 init, rotation automatically)
  // Use alternative ILI9341_2_DRIVER for CYD panel orientation
  tft.init();

  // Fix inverted colors: enable display inversion
  tft.writecommand(0x21); // Display Inversion ON

  // Use TFT_eSPI's built-in rotation (0=0°, 1=90°, 2=180°, 3=270°)
  // Rotation 1 works with ILI9341_2_DRIVER
  tft.setRotation(1);

  tft.fillScreen(TFT_BLACK);

  // Turn on backlight
  digitalWrite(CYD_TFT_BL, HIGH);

  // 2b. Initialize SPIFFS and Load Game Rules
  if (!SPIFFS.begin(true)) {
    Serial.println("[SPIFFS] Mount failed! Using embedded rules.");
  } else {
    Serial.println("[SPIFFS] Mounted successfully.");
    loadRulesFromSPIFFS();
  }

  // 3. Initialize Touch (XPT2046)
  touchSPI.begin(XPT2046_CLK, XPT2046_MISO, XPT2046_MOSI, XPT2046_CS);
  touch.begin(touchSPI);
  touch.setRotation(1);  // Match display rotation 1

  // 4. Load persistent settings from NVS
  loadSettingsFromNVS();

  // Apply trainer name to local pet & sync engine
  if (cfg_trainer_name.length() > 0) {
    strncpy(myPet.name, cfg_trainer_name.c_str(), sizeof(myPet.name) - 1);
  }
  syncToEngine();

  // Check if BOOT button (GPIO 0) is held down during power-on
  pinMode(0, INPUT_PULLUP);
  if (digitalRead(0) == LOW) {
    Serial.println("[BOOT] Button held! Entering WiFi AP Portal...");
    startAPConfigPortal();
    return;
  }

  // 5. Connect to WiFi in Station Mode
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextDatum(MC_DATUM);
  tft.drawString("CYD Tamagotchi RPG", 160, 90, 4);
  tft.drawString(String("Connecting to ") + cfg_wifi_ssid + "...", 160, 130, 2);

  WiFi.mode(WIFI_STA);
  WiFi.begin(cfg_wifi_ssid.c_str(), cfg_wifi_pass.c_str());

  uint8_t retries = 0;
  while (WiFi.status() != WL_CONNECTED && retries < 25) {
    delay(400);
    Serial.print(".");
    retries++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.printf("\nWiFi Connected! Local IP: %s\n", WiFi.localIP().toString().c_str());
    socketServer.begin();
    setRGB(false, true, false); // Green = Online & Ready
    showBanner("WiFi Connected! Socket Ready");
    soundLevelUp();

    // 6. Test PHP Server connectivity
    if (!testPHPServerConnection()) {
      inServerErrorState = true;
      showServerErrorScreen();
      return; // Stay in error state, user can tap to enter AP config
    }

    // 7. Register with PHP Server, check OTA, & draw initial LCD screen
    syncWithPHPServer();
    checkForOTAUpdate();
    renderCYDScreen();
  } else {
    // If WiFi credentials are unconfigured or fail to connect, launch AP Portal automatically!
    Serial.println("\nWiFi connection failed! Starting AP Portal for easy setup...");
    startAPConfigPortal();
    return;
  }
}

// ============================================================================
// Arduino MAIN LOOP
// ============================================================================
void loop() {
  // If running in AP Config Portal Mode
  if (inAPConfigMode) {
    apServer.handleClient();

    // Check if user taps screen to cancel AP mode and play offline
    if (touch.touched()) {
      inAPConfigMode = false;
      WiFi.softAPdisconnect(true);
      WiFi.mode(WIFI_OFF);
      soundClick();
      setRGB(true, false, false); // Red = Offline
      showBanner("Offline Mode");
      renderCYDScreen();
      delay(300);
    }
    delay(10);
    return;
  }

  // If in server error state - allow tap to enter AP config
  if (inServerErrorState) {
    if (touch.touched()) {
      inServerErrorState = false;
      startAPConfigPortal();
      delay(300);
    }
    delay(10);
    return;
  }

  unsigned long now = millis();

  // 1. Process Resistive Touch Input
  if (touch.touched()) {
    TS_Point p = touch.getPoint();
    uint16_t x = map(p.x, 260, 3750, 0, 320);
    uint16_t y = map(p.y, 350, 3850, 0, 240);
    x = constrain(x, 0, 319);
    y = constrain(y, 0, 239);
    handleTouchPress(x, y);
    delay(180); // Debounce touch
  }

  // 2. Poll Direct Sockets (Peer CYD incoming messages)
  pollDirectSockets();

  // 3. Periodic Server Heartbeat & Discovery (Every 10 seconds)
  if (now - lastServerSyncTime > SERVER_SYNC_INTERVAL_MS) {
    syncWithPHPServer();
    lastServerSyncTime = now;
  }

  // 3b. Periodic Over-the-Air (OTA) Check (Every 15 minutes)
  if (now - lastOtaCheckTime > OTA_CHECK_INTERVAL_MS) {
    checkForOTAUpdate();
    lastOtaCheckTime = now;
  }

  // 3c. Automatic Save Loop (Every 2 minutes: saves to NVS Flash and Cloud)
  if (now - lastAutoSaveTime > AUTO_SAVE_INTERVAL_MS) {
    saveSettingsToNVS();
    if (WiFi.status() == WL_CONNECTED && !inServerErrorState) {
      syncWithPHPServer();
    }
    Serial.println("[AutoSave] Periodic state saved to NVS and Server.");
    lastAutoSaveTime = now;
  }

  // 4. Pet Decay Loop (Every 6 seconds - uses Engine Satiety Shield & Sickness)
  if (now - lastTickTime > PET_TICK_INTERVAL_MS) {
    syncToEngine();
    engine.tick();
    syncFromEngine();
    screenNeedsRefresh = true;
    lastTickTime = now;
  }

  // 4b. Active Job Completion & 1s Sub-Screen Countdown Refresh
  if (activeJobIdx >= 0) {
    if ((long)(now - activeJobEndMs) >= 0) {
      int8_t finishedJob = activeJobIdx;
      activeJobIdx = -1;
      syncToEngine();
      if (finishedJob == 0) {
        engine.coins += 15;
        engine.addXP(25);
        engine.addKnownItemById("mat_herb", 1);
        engine.addKnownItemById("food_apple", 2);
        showBanner(LANG_DE ? "Beruf fertig! +$15 +Kraut +2 Apfel" : "Job Done! +$15 +Herb +2 Apple", 3200);
      } else if (finishedJob == 1) {
        engine.coins += 30;
        engine.addXP(50);
        engine.addKnownItemById("mat_meat", 1);
        engine.addKnownItemById("food_snack", 1);
        showBanner(LANG_DE ? "Wache fertig! +$30 +Fleisch +Snack" : "Scout Done! +$30 +Meat +Snack", 3200);
      } else {
        engine.coins += 55;
        engine.addXP(90);
        engine.addKnownItemById("mat_mushroom", 1);
        engine.addKnownItemById("mat_sunstone", 1);
        engine.addKnownItemById("food_sun_jerky", 1);
        showBanner(LANG_DE ? "Mine fertig! +$55 +Salz +Jerky" : "Mine Done! +$55 +Salt +Jerky", 3200);
      }
      syncFromEngine();
      myPet.jobsCompleted++;
      soundLevelUp();
      checkLevelUp();
      screenNeedsRefresh = true;
    } else if (inSubScreen && (subScreenTab == 0 || subScreenTab == 2) && (now - lastJobCountdownRefresh >= 1000)) {
      lastJobCountdownRefresh = now;
      screenNeedsRefresh = true;
    }
  }

  // 5. Speech balloon / Banner expiration
  if (friendPet.balloonTimeout > 0 && now > friendPet.balloonTimeout) {
    friendPet.balloonTimeout = 0;
    screenNeedsRefresh = true;
  }
  if (bannerTimeout > 0 && now > bannerTimeout) {
    activeBannerMessage[0] = '\0';
    bannerTimeout = 0;
    screenNeedsRefresh = true;
  }

  // 6. Refresh Display if dirty (throttle to max 10 FPS to prevent flicker)
  if (screenNeedsRefresh && (now - lastRenderTime > 100)) {
    if (inSubScreen) {
      renderSubScreen();
    } else {
      renderCYDScreen();
    }
    screenNeedsRefresh = false;
    lastRenderTime = now;
  }

  delay(15);
}

// ============================================================================
// 2.8" LCD 320x240 SUB-SCREEN (Stats, Bag/Cook/Shop, Jobs, Magic, Quests)
// ============================================================================
void renderSubScreen() {
  tft.fillScreen(COLOR_UI_PANEL);

  // 1. TOP TABS NAVIGATION BAR (5 Tabs + Close X, Y: 3 to 27)
  // Tab 0: STAT (X: 2 to 56)
  bool act0 = (subScreenTab == 0);
  tft.fillRoundRect(2, 3, 54, 24, 3, act0 ? 0x028A : 0x18C5);
  tft.drawRoundRect(2, 3, 54, 24, 3, act0 ? TFT_GOLD : COLOR_UI_BORDER);
  tft.setTextColor(act0 ? TFT_WHITE : TFT_LIGHTGREY, act0 ? 0x028A : 0x18C5);
  tft.setTextDatum(MC_DATUM);
  tft.drawString(LANG_DE ? "STAT" : "STAT", 29, 15, 1);

  // Tab 1: BAG / COOK / SHOP (X: 58 to 112)
  bool act1 = (subScreenTab == 1);
  tft.fillRoundRect(58, 3, 54, 24, 3, act1 ? 0x028A : 0x18C5);
  tft.drawRoundRect(58, 3, 54, 24, 3, act1 ? TFT_GOLD : COLOR_UI_BORDER);
  tft.setTextColor(act1 ? TFT_WHITE : TFT_LIGHTGREY, act1 ? 0x028A : 0x18C5);
  tft.drawString(LANG_DE ? "TASCH" : "BAG", 85, 15, 1);

  // Tab 2: JOBS (X: 114 to 168)
  bool act2 = (subScreenTab == 2);
  tft.fillRoundRect(114, 3, 54, 24, 3, act2 ? 0x028A : 0x18C5);
  tft.drawRoundRect(114, 3, 54, 24, 3, act2 ? TFT_GOLD : COLOR_UI_BORDER);
  tft.setTextColor(act2 ? TFT_WHITE : TFT_LIGHTGREY, act2 ? 0x028A : 0x18C5);
  tft.drawString(LANG_DE ? "JOBS" : "JOBS", 141, 15, 1);

  // Tab 3: MAGIC (X: 170 to 225)
  bool act3 = (subScreenTab == 3);
  tft.fillRoundRect(170, 3, 55, 24, 3, act3 ? 0x78D8 : 0x18C5);
  tft.drawRoundRect(170, 3, 55, 24, 3, act3 ? TFT_GOLD : COLOR_UI_BORDER);
  tft.setTextColor(act3 ? TFT_WHITE : TFT_LIGHTGREY, act3 ? 0x78D8 : 0x18C5);
  tft.drawString(LANG_DE ? "MAGIE" : "MAGIC", 197, 15, 1);

  // Tab 4: QUESTS & MISSIONS (X: 227 to 285)
  bool act4 = (subScreenTab == 4);
  tft.fillRoundRect(227, 3, 58, 24, 3, act4 ? 0x1404 : 0x18C5);
  tft.drawRoundRect(227, 3, 58, 24, 3, act4 ? TFT_GOLD : COLOR_UI_BORDER);
  tft.setTextColor(act4 ? TFT_WHITE : TFT_LIGHTGREY, act4 ? 0x1404 : 0x18C5);
  tft.drawString(LANG_DE ? "QUEST" : "QUEST", 256, 15, 1);

  // Close Button: [ X ] (X: 288 to 317)
  tft.fillRoundRect(288, 3, 29, 24, 3, TFT_RED);
  tft.drawRoundRect(288, 3, 29, 24, 3, TFT_WHITE);
  tft.setTextColor(TFT_WHITE, TFT_RED);
  tft.drawString("X", 302, 15, 2);

  // Divider Line
  tft.drawFastHLine(0, 30, 320, COLOR_UI_BORDER);

  // --------------------------------------------------------------------------
  // TAB 0: COMPLETE STATS & VITALS OVERVIEW
  // --------------------------------------------------------------------------
  if (subScreenTab == 0) {
    tft.fillRoundRect(6, 34, 150, 178, 4, 0x0842);
    tft.drawRoundRect(6, 34, 150, 178, 4, COLOR_UI_BORDER);

    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(COLOR_GOLD, 0x0842);
    tft.drawString(myPet.name, 12, 40, 2);

    tft.setTextColor(TFT_WHITE, 0x0842);
    tft.drawString(String("Lv.") + myPet.level + " (" + myPet.phase + ")", 12, 58, 1);

    tft.setTextColor(TFT_CYAN, 0x0842);
    tft.drawString(String("Biom: ") + myPet.biome, 12, 70, 1);

    // XP Bar
    tft.setTextColor(COLOR_MANA_PURPLE, 0x0842);
    tft.drawString(String("XP: ") + myPet.xp + "/" + myPet.xpNext, 12, 84, 1);
    uint8_t xpPct = (myPet.xpNext > 0) ? (uint8_t)min((int)100, (int)((myPet.xp * 100) / myPet.xpNext)) : 0;
    drawMiniBar(tft, 12, 95, 138, 6, xpPct, COLOR_MANA_PURPLE);

    // Coins & Streak
    tft.setTextColor(COLOR_GOLD, 0x0842);
    tft.drawString(String("Muenzen: $") + myPet.coins, 12, 106, 1);
    tft.setTextColor(TFT_ORANGE, 0x0842);
    tft.drawString(String("Streak: ") + engine.consecutiveLoginDays + (LANG_DE ? " Tage" : " days"), 12, 118, 1);

    // Satiety Shield & Health Condition
    tft.setTextColor(engine.satietyTicks > 0 ? TFT_GREEN : TFT_LIGHTGREY, 0x0842);
    tft.drawString((LANG_DE ? "Satt-Schutz: " : "Satiety: ") + String(engine.satietyTicks) + "T", 12, 132, 1);

    tft.setTextColor(engine.isSick ? TFT_RED : TFT_GREEN, 0x0842);
    tft.drawString(engine.isSick ? (LANG_DE ? "Zustand: KRANK!" : "Status: SICK!") : (LANG_DE ? "Zustand: Gesund" : "Status: Healthy"), 12, 144, 1);

    // Records & Active Job Countdown
    tft.setTextColor(TFT_LIGHTGREY, 0x0842);
    tft.drawString(String("Rezepte: ") + myPet.recipesCrafted + " | Jobs: " + myPet.jobsCompleted, 12, 158, 1);
    tft.drawString(String("Zauber: ") + myPet.spellsCast + " | Inv: " + engine.inventoryCount, 12, 170, 1);

    if (activeJobIdx >= 0) {
      long remMs = (long)(activeJobEndMs - millis());
      long remSec = (remMs > 0) ? (remMs / 1000L) : 0L;
      const char* jShort = (activeJobIdx == 0) ? (LANG_DE ? "Kraeuter" : "Forage")
                         : (activeJobIdx == 1) ? (LANG_DE ? "Wache" : "Scout")
                         : (LANG_DE ? "Mine" : "Mine");
      tft.setTextColor(TFT_YELLOW, 0x0842);
      tft.drawString(String(LANG_DE ? "Beruf: " : "Job: ") + jShort + " (" + remSec + "s)", 12, 184, 1);
    } else {
      tft.setTextColor(TFT_GREEN, 0x0842);
      tft.drawString(LANG_DE ? "* Auto-Save: 2 Min aktiv" : "* Auto-Save: 2m active", 12, 184, 1);
    }

    // Right Box: Full 6 Vitals Bars
    tft.fillRoundRect(162, 34, 152, 178, 4, 0x0842);
    tft.drawRoundRect(162, 34, 152, 178, 4, COLOR_UI_BORDER);

    tft.setTextColor(TFT_RED, 0x0842);
    tft.drawString(String("HP (Leben): ") + myPet.health + "/100", 170, 40, 1);
    drawMiniBar(tft, 170, 52, 136, 8, myPet.health, TFT_RED);

    tft.setTextColor(TFT_ORANGE, 0x0842);
    tft.drawString(String("FD (Hunger): ") + myPet.hunger + "/100", 170, 66, 1);
    drawMiniBar(tft, 170, 78, 136, 8, myPet.hunger, TFT_ORANGE);

    tft.setTextColor(TFT_CYAN, 0x0842);
    tft.drawString(String("EN (Energie): ") + myPet.energy + "/100", 170, 92, 1);
    drawMiniBar(tft, 170, 104, 136, 8, myPet.energy, TFT_CYAN);

    tft.setTextColor(TFT_GREEN, 0x0842);
    tft.drawString(String("SAUB (Hygiene): ") + myPet.hygiene + "/100", 170, 118, 1);
    drawMiniBar(tft, 170, 130, 136, 8, myPet.hygiene, TFT_GREEN);

    tft.setTextColor(TFT_PINK, 0x0842);
    tft.drawString(String("GLUECK: ") + myPet.happiness + "/100", 170, 144, 1);
    drawMiniBar(tft, 170, 156, 136, 8, myPet.happiness, TFT_PINK);

    tft.setTextColor(COLOR_MANA_PURPLE, 0x0842);
    tft.drawString(String("MP (Mana): ") + myPet.mana + "/100", 170, 170, 1);
    drawMiniBar(tft, 170, 182, 136, 8, myPet.mana, COLOR_MANA_PURPLE);
  }

  // --------------------------------------------------------------------------
  // TAB 1: INVENTORY / RECIPES / SHOP SUB-MODES
  // --------------------------------------------------------------------------
  else if (subScreenTab == 1) {
    // Sub-mode pills (Y: 33 to 51)
    tft.fillRoundRect(6, 33, 98, 18, 3, bagSubMode == 0 ? 0x028A : 0x18C5);
    tft.drawRoundRect(6, 33, 98, 18, 3, bagSubMode == 0 ? TFT_WHITE : COLOR_UI_BORDER);
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(TFT_WHITE, bagSubMode == 0 ? 0x028A : 0x18C5);
    tft.drawString(LANG_DE ? "TASCHE" : "BAG", 55, 42, 1);

    tft.fillRoundRect(110, 33, 98, 18, 3, bagSubMode == 1 ? 0x9300 : 0x18C5);
    tft.drawRoundRect(110, 33, 98, 18, 3, bagSubMode == 1 ? TFT_WHITE : COLOR_UI_BORDER);
    tft.setTextColor(TFT_WHITE, bagSubMode == 1 ? 0x9300 : 0x18C5);
    tft.drawString(LANG_DE ? "KOCHEN" : "COOK", 159, 42, 1);

    tft.fillRoundRect(214, 33, 100, 18, 3, bagSubMode == 2 ? 0x1404 : 0x18C5);
    tft.drawRoundRect(214, 33, 100, 18, 3, bagSubMode == 2 ? TFT_WHITE : COLOR_UI_BORDER);
    tft.setTextColor(TFT_WHITE, bagSubMode == 2 ? 0x1404 : 0x18C5);
    tft.drawString(String("SHOP $") + myPet.coins, 264, 42, 1);

    if (bagSubMode == 0) {
      // REAL BAG INVENTORY (up to 4 visible slots at Y: 55, 94, 133, 172)
      if (engine.inventoryCount == 0) {
        tft.setTextColor(TFT_ORANGE, COLOR_UI_PANEL);
        tft.drawString(LANG_DE ? "Tasche leer! Kaufe im SHOP oder starte JOBS." : "Bag empty! Visit SHOP or start JOBS.", 160, 120, 2);
      } else {
        uint8_t shown = min((uint8_t)4, engine.inventoryCount);
        for (uint8_t i = 0; i < shown; i++) {
          EngineInventorySlot& sl = engine.inventory[i];
          int ry = 55 + i * 39;
          tft.fillRoundRect(6, ry, 308, 35, 4, 0x0842);
          tft.drawRoundRect(6, ry, 308, 35, 4, sl.category == 0 ? TFT_ORANGE : (sl.category == 2 ? TFT_CYAN : COLOR_UI_BORDER));

          tft.setTextDatum(TL_DATUM);
          tft.setTextColor(sl.category == 0 ? TFT_YELLOW : (sl.category == 2 ? TFT_CYAN : TFT_GREEN), 0x0842);
          tft.drawString(String(LANG_DE ? sl.name_de : sl.name) + " x" + String(sl.quantity), 14, ry + 5, 2);

          tft.setTextColor(TFT_LIGHTGREY, 0x0842);
          char infoBuf[44];
          if (sl.category == 0) {
            if (sl.satietyTicks > 0) snprintf(infoBuf, sizeof(infoBuf), "+%d FD | Schutz: %dT", sl.hungerGain, sl.satietyTicks);
            else if (sl.sickChancePct > 0) snprintf(infoBuf, sizeof(infoBuf), "+%d FD | ! %d%% Krank", sl.hungerGain, sl.sickChancePct);
            else snprintf(infoBuf, sizeof(infoBuf), "+%d FD | +%d Happy", sl.hungerGain, sl.happyGain);
          } else if (sl.category == 2) {
            if (sl.cureSickness) {
              snprintf(infoBuf, sizeof(infoBuf), "+%d HP | %s", sl.healthGain, LANG_DE ? "Heilt Krank!" : "Cures Sick!");
            } else {
              snprintf(infoBuf, sizeof(infoBuf), "+%d EN | +15 MP", sl.energyGain);
            }
          } else {
            snprintf(infoBuf, sizeof(infoBuf), "%s", LANG_DE ? "Kochzutat (siehe KOCHEN)" : "Ingredient (see COOK)");
          }
          tft.drawString(infoBuf, 14, ry + 22, 1);

          // Action button (X: 238 to 308)
          uint16_t bCol = (sl.category == 0) ? 0x1404 : (sl.category == 2 ? 0x7800 : 0x9300);
          tft.fillRoundRect(238, ry + 5, 70, 25, 3, bCol);
          tft.drawRoundRect(238, ry + 5, 70, 25, 3, TFT_WHITE);
          tft.setTextDatum(MC_DATUM);
          tft.setTextColor(TFT_WHITE, bCol);
          const char* actLbl = (sl.category == 0) ? (LANG_DE ? "FUETTERN" : "FEED")
                            : (sl.category == 2) ? (LANG_DE ? "NUTZEN" : "USE")
                            : (LANG_DE ? "KOCHEN" : "COOK");
          tft.drawString(actLbl, 273, ry + 17, 1);
        }
      }
    } else if (bagSubMode == 1) {
      // COOK RECIPES INTO INVENTORY (3 recipes at Y: 56, 108, 160)
      uint8_t qtyHerb = engine.getItemQuantity("mat_herb");
      uint8_t qtyMush = engine.getItemQuantity("mat_mushroom");
      uint8_t qtyMeat = engine.getItemQuantity("mat_meat");
      uint8_t qtySalt = engine.getItemQuantity("mat_sunstone");

      // Recipe 0: Herb Salad
      tft.fillRoundRect(6, 56, 308, 48, 4, 0x0842);
      tft.drawRoundRect(6, 56, 308, 48, 4, TFT_GREEN);
      tft.setTextDatum(TL_DATUM);
      tft.setTextColor(TFT_GREEN, 0x0842);
      tft.drawString(LANG_DE ? "1. Kraeuter-Salat (+35 FD, 4T)" : "1. Herb Salad (+35 FD, 4T)", 14, 62, 2);
      tft.setTextColor(TFT_WHITE, 0x0842);
      tft.drawString(String(LANG_DE ? "Zutaten: 2x Kraeuter (" : "Need: 2x Herb (") + String(qtyHerb) + "/2)", 14, 84, 1);
      tft.fillRoundRect(238, 65, 70, 30, 3, qtyHerb >= 2 ? 0x1404 : 0x3186);
      tft.drawRoundRect(238, 65, 70, 30, 3, TFT_WHITE);
      tft.setTextDatum(MC_DATUM);
      tft.setTextColor(TFT_WHITE, qtyHerb >= 2 ? 0x1404 : 0x3186);
      tft.drawString(LANG_DE ? "KOCHEN" : "COOK", 273, 80, 1);

      // Recipe 1: Berry Tart
      bool canR1 = (myPet.level >= 2 && qtyHerb >= 1 && qtyMush >= 1);
      tft.fillRoundRect(6, 108, 308, 48, 4, 0x0842);
      tft.drawRoundRect(6, 108, 308, 48, 4, COLOR_GOLD);
      tft.setTextDatum(TL_DATUM);
      tft.setTextColor(COLOR_GOLD, 0x0842);
      tft.drawString(LANG_DE ? "2. Beeren-Tarte (+45 FD, 5T)" : "2. Berry Tart (+45 FD, 5T)", 14, 114, 2);
      tft.setTextColor(TFT_WHITE, 0x0842);
      tft.drawString(String("Lv.2 | Kraeut:") + qtyHerb + "/1 Pilz:" + qtyMush + "/1", 14, 136, 1);
      tft.fillRoundRect(238, 117, 70, 30, 3, canR1 ? 0x9300 : 0x3186);
      tft.drawRoundRect(238, 117, 70, 30, 3, TFT_WHITE);
      tft.setTextDatum(MC_DATUM);
      tft.setTextColor(TFT_WHITE, canR1 ? 0x9300 : 0x3186);
      tft.drawString(LANG_DE ? "KOCHEN" : "COOK", 273, 132, 1);

      // Recipe 2: Sun Jerky
      bool canR2 = (myPet.level >= 2 && qtyMeat >= 2 && qtySalt >= 1);
      tft.fillRoundRect(6, 160, 308, 48, 4, 0x0842);
      tft.drawRoundRect(6, 160, 308, 48, 4, TFT_ORANGE);
      tft.setTextDatum(TL_DATUM);
      tft.setTextColor(TFT_ORANGE, 0x0842);
      tft.drawString(LANG_DE ? "3. Doerrfleisch (+55 FD, 6T)" : "3. Sun Jerky (+55 FD, 6T)", 14, 166, 2);
      tft.setTextColor(TFT_WHITE, 0x0842);
      tft.drawString(String("Lv.2 | Fleisch:") + qtyMeat + "/2 Salz:" + qtySalt + "/1", 14, 188, 1);
      tft.fillRoundRect(238, 169, 70, 30, 3, canR2 ? 0x9300 : 0x3186);
      tft.drawRoundRect(238, 169, 70, 30, 3, TFT_WHITE);
      tft.setTextDatum(MC_DATUM);
      tft.setTextColor(TFT_WHITE, canR2 ? 0x9300 : 0x3186);
      tft.drawString(LANG_DE ? "KOCHEN" : "COOK", 273, 184, 1);
    } else {
      // SHOP / BAZAR (4 items at Y: 55, 94, 133, 172)
      const char* sIds[4] = {"food_apple", "food_snack", "food_herb_salad", "item_salve"};
      const uint16_t sPrices[4] = {5, 8, 22, 25};
      const char* sTitleDe[4] = {"Frischer Apfel", "Pixel-Snack", "Kraeuter-Salat", "Heilsalbe (Med)"};
      const char* sTitleEn[4] = {"Crisp Apple", "Pixel Snack", "Herb Salad", "Healing Salve"};
      const char* sSubDe[4]   = {"+12 FD | Sicher", "+25 FD | 30% Krank!", "+35 FD | 4T Schutz", "+40 HP | Heilt Krank!"};
      const char* sSubEn[4]   = {"+12 FD | Safe", "+25 FD | 30% Sick!", "+35 FD | 4T Satiety", "+40 HP | Cures Sick!"};

      for (uint8_t i = 0; i < 4; i++) {
        int ry = 55 + i * 39;
        bool afford = (myPet.coins >= sPrices[i]);
        tft.fillRoundRect(6, ry, 308, 35, 4, 0x0842);
        tft.drawRoundRect(6, ry, 308, 35, 4, afford ? TFT_GREEN : COLOR_UI_BORDER);
        tft.setTextDatum(TL_DATUM);
        tft.setTextColor(TFT_WHITE, 0x0842);
        tft.drawString(String(LANG_DE ? sTitleDe[i] : sTitleEn[i]) + " (x" + engine.getItemQuantity(sIds[i]) + ")", 14, ry + 5, 2);
        tft.setTextColor(i == 1 ? TFT_ORANGE : TFT_CYAN, 0x0842);
        tft.drawString(LANG_DE ? sSubDe[i] : sSubEn[i], 14, ry + 22, 1);

        tft.fillRoundRect(236, ry + 5, 72, 25, 3, afford ? 0x1404 : 0x3186);
        tft.drawRoundRect(236, ry + 5, 72, 25, 3, TFT_WHITE);
        tft.setTextDatum(MC_DATUM);
        tft.setTextColor(TFT_WHITE, afford ? 0x1404 : 0x3186);
        tft.drawString((LANG_DE ? "KAUF $" : "BUY $") + String(sPrices[i]), 272, ry + 17, 1);
      }
    }
  }

  // --------------------------------------------------------------------------
  // TAB 2: JOBS & PROFESSIONS (Earn Coins, XP & Food/Ingredients)
  // --------------------------------------------------------------------------
  else if (subScreenTab == 2) {
    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(TFT_GREEN, COLOR_UI_PANEL);
    tft.drawString(LANG_DE ? "BERUFE: Muenzen, Zutaten & Essen verdienen" : "JOBS: Earn Coins, Materials & Food", 10, 34, 1);

    long jobRemMs = (activeJobIdx >= 0) ? (long)(activeJobEndMs - millis()) : 0L;
    long jobRemSec = (jobRemMs > 0) ? (jobRemMs / 1000L) : 0L;

    // Job 1: Orchard & Herbs (60s)
    bool j0Active = (activeJobIdx == 0);
    tft.fillRoundRect(8, 48, 304, 44, 4, 0x0842);
    tft.drawRoundRect(8, 48, 304, 44, 4, j0Active ? TFT_GREEN : COLOR_UI_BORDER);
    tft.setTextColor(TFT_GREEN, 0x0842);
    tft.drawString(LANG_DE ? "1. Kraeutersammler (60s)" : "1. Herb Forager (60s)", 16, 54, 2);
    tft.setTextColor(TFT_WHITE, 0x0842);
    tft.drawString(LANG_DE ? "-15 EN | +$15 +25XP +2 Aepfel +Kraut" : "-15 EN | +$15 +25XP +2 Apple +Herb", 16, 74, 1);
    tft.fillRoundRect(236, 54, 70, 32, 3, (activeJobIdx >= 0) ? 0x3186 : 0x1404);
    tft.drawRoundRect(236, 54, 70, 32, 3, TFT_WHITE);
    tft.setTextColor(TFT_WHITE, (activeJobIdx >= 0) ? 0x3186 : 0x1404);
    tft.setTextDatum(MC_DATUM);
    tft.drawString(j0Active ? (String(jobRemSec) + "s") : "START", 271, 70, 1);

    // Job 2: Village Scout (120s)
    bool j1Active = (activeJobIdx == 1);
    tft.fillRoundRect(8, 96, 304, 44, 4, 0x0842);
    tft.drawRoundRect(8, 96, 304, 44, 4, j1Active ? TFT_YELLOW : COLOR_UI_BORDER);
    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(COLOR_GOLD, 0x0842);
    tft.drawString(LANG_DE ? "2. Dorfwache Lv.2 (120s)" : "2. Village Scout Lv.2 (120s)", 16, 102, 2);
    tft.setTextColor(TFT_WHITE, 0x0842);
    tft.drawString(LANG_DE ? "-20 EN | +$30 +50XP +Snack +Fleisch" : "-20 EN | +$30 +50XP +Snack +Meat", 16, 122, 1);
    tft.fillRoundRect(236, 102, 70, 32, 3, (activeJobIdx >= 0) ? 0x3186 : 0x9300);
    tft.drawRoundRect(236, 102, 70, 32, 3, TFT_WHITE);
    tft.setTextColor(TFT_WHITE, (activeJobIdx >= 0) ? 0x3186 : 0x9300);
    tft.setTextDatum(MC_DATUM);
    tft.drawString(j1Active ? (String(jobRemSec) + "s") : "START", 271, 118, 1);

    // Job 3: Crystal & Mushroom Mine (180s)
    bool j2Active = (activeJobIdx == 2);
    tft.fillRoundRect(8, 144, 304, 44, 4, 0x0842);
    tft.drawRoundRect(8, 144, 304, 44, 4, j2Active ? TFT_CYAN : COLOR_UI_BORDER);
    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(TFT_CYAN, 0x0842);
    tft.drawString(LANG_DE ? "3. Kristallmine Lv.3 (180s)" : "3. Crystal Mine Lv.3 (180s)", 16, 150, 2);
    tft.setTextColor(TFT_WHITE, 0x0842);
    tft.drawString(LANG_DE ? "-30 EN | +$55 +90XP +Salz +Jerky" : "-30 EN | +$55 +90XP +Salt +Jerky", 16, 170, 1);
    tft.fillRoundRect(236, 150, 70, 32, 3, (activeJobIdx >= 0) ? 0x3186 : 0x028A);
    tft.drawRoundRect(236, 150, 70, 32, 3, TFT_WHITE);
    tft.setTextColor(TFT_WHITE, (activeJobIdx >= 0) ? 0x3186 : 0x028A);
    tft.setTextDatum(MC_DATUM);
    tft.drawString(j2Active ? (String(jobRemSec) + "s") : "START", 271, 166, 1);

    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(TFT_LIGHTGREY, COLOR_UI_PANEL);
    tft.drawString(String("Energie: ") + myPet.energy + " EN | Jobs: " + myPet.jobsCompleted, 10, 196, 1);
  }

  // --------------------------------------------------------------------------
  // TAB 3: SELECTABLE MAGIC SPELLS & ARCANE RESEARCH
  // --------------------------------------------------------------------------
  else if (subScreenTab == 3) {
    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(COLOR_MANA_PURPLE, COLOR_UI_PANEL);
    tft.drawString(String(LANG_DE ? "ZAUBER AUSWAHL | Mana: " : "SELECT SPELL | Mana: ") + myPet.mana + "/100 | EN: " + myPet.energy, 10, 33, 1);

    // Spell 0: Purify / Laeuterung (15 MP, cures sickness!)
    tft.fillRoundRect(8, 45, 304, 39, 4, 0x0842);
    tft.drawRoundRect(8, 45, 304, 39, 4, engine.isSick ? TFT_GREEN : COLOR_UI_BORDER);
    tft.setTextColor(TFT_CYAN, 0x0842);
    tft.drawString(LANG_DE ? "1. Laeuterung (15 MP)" : "1. Purify (15 MP)", 16, 49, 2);
    tft.setTextColor(TFT_WHITE, 0x0842);
    tft.drawString(LANG_DE ? "Heilt Bauchweh/Krankheit & 100% Sauber" : "Cures Sickness & 100% Hygiene", 16, 68, 1);
    tft.fillRoundRect(236, 51, 70, 27, 3, myPet.mana >= 15 ? 0x028A : 0x3186);
    tft.drawRoundRect(236, 51, 70, 27, 3, TFT_WHITE);
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(TFT_WHITE, myPet.mana >= 15 ? 0x028A : 0x3186);
    tft.drawString(LANG_DE ? "WIRKEN" : "CAST", 271, 64, 1);

    // Spell 1: Vitality Aura (20 MP)
    tft.fillRoundRect(8, 87, 304, 39, 4, 0x0842);
    tft.drawRoundRect(8, 87, 304, 39, 4, COLOR_UI_BORDER);
    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(TFT_PINK, 0x0842);
    tft.drawString(LANG_DE ? "2. Heilaura (20 MP)" : "2. Vitality Aura (20 MP)", 16, 91, 2);
    tft.setTextColor(TFT_WHITE, 0x0842);
    tft.drawString(LANG_DE ? "+30 HP Lebenspunkte & +10 Glueck" : "+30 Health & +10 Happiness", 16, 110, 1);
    tft.fillRoundRect(236, 93, 70, 27, 3, myPet.mana >= 20 ? 0x78D8 : 0x3186);
    tft.drawRoundRect(236, 93, 70, 27, 3, TFT_WHITE);
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(TFT_WHITE, myPet.mana >= 20 ? 0x78D8 : 0x3186);
    tft.drawString(LANG_DE ? "WIRKEN" : "CAST", 271, 106, 1);

    // Spell 2: Astral Feast / Friend Blessing (30 MP)
    tft.fillRoundRect(8, 129, 304, 39, 4, 0x0842);
    tft.drawRoundRect(8, 129, 304, 39, 4, COLOR_UI_BORDER);
    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(COLOR_GOLD, 0x0842);
    tft.drawString(LANG_DE ? "3. Astral-Festmahl (30 MP)" : "3. Astral Feast (30 MP)", 16, 133, 2);
    tft.setTextColor(TFT_WHITE, 0x0842);
    tft.drawString(LANG_DE ? "+35 Hunger, 3T Schutz + Partner-Segen!" : "+35 Hunger, 3T Satiety + Buddy Heal!", 16, 152, 1);
    tft.fillRoundRect(236, 135, 70, 27, 3, myPet.mana >= 30 ? 0x78D8 : 0x3186);
    tft.drawRoundRect(236, 135, 70, 27, 3, TFT_WHITE);
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(TFT_WHITE, myPet.mana >= 30 ? 0x78D8 : 0x3186);
    tft.drawString(LANG_DE ? "WIRKEN" : "CAST", 271, 148, 1);

    // Research Magic Button
    tft.fillRoundRect(8, 173, 304, 36, 4, myPet.energy >= 15 ? 0x78D8 : 0x3186);
    tft.drawRoundRect(8, 173, 304, 36, 4, TFT_WHITE);
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(TFT_WHITE, myPet.energy >= 15 ? 0x78D8 : 0x3186);
    tft.drawString(LANG_DE ? "MAGIE ERFORSCHEN (-15 EN -> +35 XP +15 MP)" : "RESEARCH MAGIC (-15 EN -> +35 XP +15 MP)", 160, 191, 1);
  }

  // --------------------------------------------------------------------------
  // TAB 4: SELECTABLE QUESTS & DAILY MISSIONS
  // --------------------------------------------------------------------------
  else if (subScreenTab == 4) {
    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(TFT_YELLOW, COLOR_UI_PANEL);
    tft.drawString(LANG_DE ? "QUESTS, TAGESMISSIONEN & STREAK" : "QUESTS, DAILY MISSIONS & STREAK", 10, 33, 1);

    // Card 0: Daily Login Streak Bonus
    bool canStreak = engine.canClaimStreakBonus();
    tft.fillRoundRect(8, 45, 304, 39, 4, 0x0842);
    tft.drawRoundRect(8, 45, 304, 39, 4, canStreak ? COLOR_GOLD : COLOR_UI_BORDER);
    tft.setTextColor(COLOR_GOLD, 0x0842);
    tft.drawString(String(LANG_DE ? "Login-Streak Tag " : "Login Streak Day ") + engine.consecutiveLoginDays, 16, 49, 2);
    tft.setTextColor(TFT_WHITE, 0x0842);
    tft.drawString(String("Bonus: +$") + engine.getStreakBonusCoins() + " | +" + engine.getStreakBonusXP() + " XP", 16, 68, 1);
    tft.fillRoundRect(236, 51, 70, 27, 3, canStreak ? 0x9300 : 0x3186);
    tft.drawRoundRect(236, 51, 70, 27, 3, TFT_WHITE);
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(TFT_WHITE, canStreak ? 0x9300 : 0x3186);
    tft.drawString(canStreak ? (LANG_DE ? "ABHOLEN" : "CLAIM") : (LANG_DE ? "ERLEDIGT" : "DONE"), 271, 64, 1);

    // Card 1: Next Active / Claimable Daily Mission
    int8_t activeMissionIdx = -1;
    for (uint8_t m = 0; m < engine.missionCount; m++) {
      if (engine.missions[m].completed && !engine.missions[m].claimed) {
        activeMissionIdx = (int8_t)m;
        break;
      }
    }
    if (activeMissionIdx < 0) {
      for (uint8_t m = 0; m < engine.missionCount; m++) {
        if (!engine.missions[m].claimed) {
          activeMissionIdx = (int8_t)m;
          break;
        }
      }
    }
    tft.fillRoundRect(8, 87, 304, 39, 4, 0x0842);
    bool mReady = (activeMissionIdx >= 0 && engine.missions[activeMissionIdx].completed && !engine.missions[activeMissionIdx].claimed);
    tft.drawRoundRect(8, 87, 304, 39, 4, mReady ? TFT_GREEN : COLOR_UI_BORDER);
    tft.setTextDatum(TL_DATUM);
    if (activeMissionIdx >= 0) {
      EngineDailyMission& dm = engine.missions[activeMissionIdx];
      tft.setTextColor(TFT_CYAN, 0x0842);
      tft.drawString(LANG_DE ? dm.name_de : dm.name, 16, 91, 2);
      tft.setTextColor(TFT_WHITE, 0x0842);
      tft.drawString(String(dm.currentCount) + "/" + dm.targetCount + " | +$" + dm.coinReward + " +" + dm.xpReward + "XP", 16, 110, 1);
      tft.fillRoundRect(236, 93, 70, 27, 3, mReady ? 0x1404 : 0x3186);
      tft.drawRoundRect(236, 93, 70, 27, 3, TFT_WHITE);
      tft.setTextDatum(MC_DATUM);
      tft.setTextColor(TFT_WHITE, mReady ? 0x1404 : 0x3186);
      tft.drawString(mReady ? (LANG_DE ? "EINLOESEN" : "CLAIM") : (LANG_DE ? "LAEUFT" : "ACTIVE"), 271, 106, 1);
    } else {
      tft.setTextColor(TFT_GREEN, 0x0842);
      tft.drawString(LANG_DE ? "Alle Tagesmissionen erledigt!" : "All Daily Missions Complete!", 16, 100, 2);
    }

    // Card 2: Co-op Quest 1 - Scout
    tft.fillRoundRect(8, 129, 304, 39, 4, 0x0842);
    tft.drawRoundRect(8, 129, 304, 39, 4, COLOR_UI_BORDER);
    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(TFT_GREEN, 0x0842);
    tft.drawString(LANG_DE ? "Koop-Quest: Spaeher-Patrouille" : "Co-op Quest: Scout Patrol", 16, 133, 2);
    tft.setTextColor(TFT_WHITE, 0x0842);
    tft.drawString(LANG_DE ? "-15 EN | +$40 +50XP +1 Apfel" : "-15 EN | +$40 +50XP +1 Apple", 16, 152, 1);
    tft.fillRoundRect(236, 135, 70, 27, 3, myPet.energy >= 15 ? 0x1404 : 0x3186);
    tft.drawRoundRect(236, 135, 70, 27, 3, TFT_WHITE);
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(TFT_WHITE, myPet.energy >= 15 ? 0x1404 : 0x3186);
    tft.drawString("START", 271, 148, 1);

    // Card 3: Co-op Quest 2 - Treasure Hunt
    tft.fillRoundRect(8, 171, 304, 39, 4, 0x0842);
    tft.drawRoundRect(8, 171, 304, 39, 4, COLOR_UI_BORDER);
    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(TFT_ORANGE, 0x0842);
    tft.drawString(LANG_DE ? "Koop-Quest: Schatzsuche" : "Co-op Quest: Treasure Hunt", 16, 175, 2);
    tft.setTextColor(TFT_WHITE, 0x0842);
    tft.drawString(LANG_DE ? "-25 EN | +$65 +75XP +Heilsalbe" : "-25 EN | +$65 +75XP +Salve", 16, 194, 1);
    tft.fillRoundRect(236, 177, 70, 27, 3, myPet.energy >= 25 ? 0x9300 : 0x3186);
    tft.drawRoundRect(236, 177, 70, 27, 3, TFT_WHITE);
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(TFT_WHITE, myPet.energy >= 25 ? 0x9300 : 0x3186);
    tft.drawString("START", 271, 190, 1);
  }

  // --------------------------------------------------------------------------
  // BOTTOM RETURN BAR (Tap to Exit)
  // --------------------------------------------------------------------------
  tft.fillRoundRect(8, 216, 304, 20, 3, 0x028A);
  tft.drawRoundRect(8, 216, 304, 20, 3, TFT_WHITE);
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(TFT_WHITE, 0x028A);
  tft.drawString(LANG_DE ? "< ZURUECK ZUM PET <" : "< RETURN TO PET SCREEN <", 160, 226, 1);
}

// ============================================================================
// 2.8" LCD 320x240 UI RENDERING ENGINE
// ============================================================================
void renderCYDScreen() {
  BiomeTheme myBiome = getBiomeTheme(myPet.biome);
  BiomeTheme frBiome = getBiomeTheme(friendPet.biome);

  // 1. Clear backgrounds for Left (Your Pet) & Right (Friend Pet)
  tft.fillRect(38, 0, 122, 240, myBiome.bgColor);
  tft.fillRect(160, 0, 122, 240, frBiome.bgColor);

  // 2. Center Partition Line
  tft.drawFastVLine(160, 0, 240, COLOR_UI_BORDER);
  tft.fillCircle(160, 120, 5, friendPet.isOnline ? TFT_GREEN : TFT_RED);
  tft.drawCircle(160, 120, 5, TFT_WHITE);

  // =========================================================================
  // LEFT COLUMN: YOUR PET CONTROLS (X: 0 to 38)
  // =========================================================================
  tft.fillRect(0, 0, 38, 240, COLOR_UI_PANEL);
  tft.drawFastVLine(37, 0, 240, COLOR_UI_BORDER);

  drawTouchButton(tft, 1, 2,   36, 44, TXT_FEED,  feedMenuOpen ? 0x03E5 : 0x0243, feedMenuOpen ? COLOR_GOLD : 0x2E69, 1, 0xA7F4);
  drawTouchButton(tft, 1, 50,  36, 44, TXT_PLAY,  0x0189, 0x3DDF, 2, 0xBDF7);
  drawTouchButton(tft, 1, 98,  36, 44, TXT_BATH,  0x020A, 0x05BB, 3, 0xA7FF);
  drawTouchButton(tft, 1, 146, 36, 44, TXT_MAGIC, 0x286B, 0x99FD, 4, 0xE5FF);
  drawTouchButton(tft, 1, 194, 36, 44, TXT_STATS, 0x0229, 0x26BF, 5, 0xBFFB);

  // =========================================================================
  // LEFT VIEWPORT: YOUR PET HUD & SPRITE (X: 38 to 160)
  // =========================================================================
  tft.setTextColor(TFT_WHITE, myBiome.bgColor);
  tft.setTextDatum(TL_DATUM);
  tft.drawString(myPet.name, 42, 6, 2);
  tft.drawString(String("Lv.") + myPet.level, 120, 6, 2);

  tft.setTextColor(myBiome.textColor, myBiome.bgColor);
  tft.drawString(myPet.phase, 42, 22, 1);

  // HP Bar
  tft.setTextColor(TFT_RED, myBiome.bgColor);
  tft.drawString("HP", 42, 34, 1);
  drawMiniBar(tft, 58, 34, 56, 7, myPet.health, TFT_RED);

  // Hunger Bar
  tft.setTextColor(TFT_ORANGE, myBiome.bgColor);
  tft.drawString("FD", 42, 44, 1);
  drawMiniBar(tft, 58, 44, 56, 7, myPet.hunger, TFT_ORANGE);

  // EXP Bar
  tft.setTextColor(COLOR_MANA_PURPLE, myBiome.bgColor);
  tft.drawString("XP", 42, 54, 1);
  uint8_t xpPct = (myPet.xpNext > 0) ? (uint8_t)min((int)100, (int)((myPet.xp * 100) / myPet.xpNext)) : 0;
  drawMiniBar(tft, 58, 54, 56, 5, xpPct, COLOR_MANA_PURPLE);

  // Satiety & Sickness HUD Badges (Y: 64)
  if (engine.satietyTicks > 0) {
    tft.fillRoundRect(42, 63, 46, 12, 2, 0x0320);
    tft.setTextColor(TFT_GREEN, 0x0320);
    tft.drawString(String("SATT:") + engine.satietyTicks + "T", 45, 65, 1);
  }
  if (engine.isSick) {
    int sx = (engine.satietyTicks > 0) ? 92 : 42;
    tft.fillRoundRect(sx, 63, 44, 12, 2, TFT_RED);
    tft.setTextColor(TFT_WHITE, TFT_RED);
    tft.drawString(LANG_DE ? "KRANK!" : "SICK!", sx + 4, 65, 1);
  }

  // Center Sprite
  if (strcmp(myPet.phase, "Egg") == 0) {
    drawPixelEgg(tft, 95, 126, myBiome.petColor, myPet.eggWarmth, myPet.biome);
    tft.setTextColor(TFT_YELLOW, myBiome.bgColor);
    tft.setTextDatum(MC_DATUM);
    tft.drawString(TXT_TAP_EGG, 95, 170, 1);
  } else {
    drawPixelPet(tft, 95, 126, myPet.biome, myPet.phase, false);
  }

  // Footer: Coins, Streak & Energy
  tft.setTextDatum(TL_DATUM);
  tft.setTextColor(COLOR_GOLD, myBiome.bgColor);
  tft.drawString(String("$") + myPet.coins, 42, 218, 1);
  tft.setTextColor(engine.canClaimStreakBonus() ? TFT_YELLOW : TFT_ORANGE, myBiome.bgColor);
  tft.drawString(String("S:") + engine.consecutiveLoginDays + (engine.canClaimStreakBonus() ? "!" : ""), 72, 218, 1);
  tft.setTextColor(TFT_CYAN, myBiome.bgColor);
  tft.drawString(String("EN:") + myPet.energy, 102, 218, 1);

  // =========================================================================
  // RIGHT VIEWPORT: FRIEND'S PET (X: 160 to 282)
  // =========================================================================
  tft.setTextColor(TFT_WHITE, frBiome.bgColor);
  tft.drawString(friendPet.name, 166, 6, 2);
  tft.drawString(String("Lv.") + friendPet.level, 244, 6, 2);

  tft.setTextColor(friendPet.isOnline ? TFT_GREEN : TFT_LIGHTGREY, frBiome.bgColor);
  tft.drawString(friendPet.isOnline ? TXT_ONLINE : TXT_ASLEEP, 166, 22, 1);

  tft.setTextColor(TFT_RED, frBiome.bgColor);
  tft.drawString("HP", 166, 34, 1);
  drawMiniBar(tft, 182, 34, 56, 7, friendPet.health, TFT_RED);

  drawPixelPet(tft, 220, 126, friendPet.biome, friendPet.phase, true);

  if (friendPet.balloonTimeout > 0 || strlen(friendPet.speechBalloon) > 0) {
    tft.fillRoundRect(164, 154, 114, 26, 4, TFT_WHITE);
    tft.drawRoundRect(164, 154, 114, 26, 4, TFT_BLACK);
    tft.setTextColor(TFT_BLACK, TFT_WHITE);
    tft.setTextDatum(MC_DATUM);
    tft.drawString(friendPet.speechBalloon, 221, 167, 1);
  }

  tft.setTextDatum(TL_DATUM);
  tft.setTextColor(TFT_CYAN, frBiome.bgColor);
  tft.drawString(friendPet.deviceId, 166, 218, 1);

  // =========================================================================
  // RIGHT COLUMN: FRIEND INTERACTIONS (X: 282 to 320)
  // =========================================================================
  tft.fillRect(282, 0, 38, 240, COLOR_UI_PANEL);
  tft.drawFastVLine(282, 0, 240, COLOR_UI_BORDER);

  drawTouchButton(tft, 283, 2,   36, 44, TXT_TREAT,      0x4044, 0xF9CA, 6,  0xFDDD);
  drawTouchButton(tft, 283, 50,  36, 44, TXT_CHAT,       chatMenuOpen ? 0x026F : 0x012B, chatMenuOpen ? COLOR_GOLD : 0x3C3F, 7, 0xBEFF);
  drawTouchButton(tft, 283, 98,  36, 44, TXT_MAGIC_PLUS, 0x186B, 0x633F, 8,  0xCFD9);
  drawTouchButton(tft, 283, 146, 36, 44, TXT_QUEST,      0x0244, 0x2E69, 9,  0xA7F4);
  drawTouchButton(tft, 283, 194, 36, 44, TXT_TRADE,      0x41C0, COLOR_GOLD, 10, 0xFEF2);

  // =========================================================================
  // HORIZONTAL FEED FLYOUT BAR BESIDE TOP-LEFT FEED BUTTON (X: 39 to 280, Y: 2 to 46)
  // =========================================================================
  if (feedMenuOpen) {
    tft.fillRoundRect(39, 2, 241, 44, 4, 0x0842);
    tft.drawRoundRect(39, 2, 241, 44, 4, TFT_GREEN);

    uint8_t foodIdx[6];
    uint8_t foodCnt = engine.getAvailableFoods(foodIdx, 6);
    if (foodCnt == 0) {
      tft.setTextDatum(TL_DATUM);
      tft.setTextColor(TFT_ORANGE, 0x0842);
      tft.drawString(LANG_DE ? "Kein Essen im Inventar!" : "No food in bag!", 46, 16, 1);

      tft.fillRoundRect(216, 7, 58, 34, 3, 0x1404);
      tft.drawRoundRect(216, 7, 58, 34, 3, TFT_WHITE);
      tft.setTextDatum(MC_DATUM);
      tft.setTextColor(TFT_WHITE, 0x1404);
      tft.drawString("SHOP", 245, 24, 1);
    } else {
      uint8_t showCnt = min((uint8_t)5, foodCnt);
      for (uint8_t k = 0; k < showCnt; k++) {
        EngineInventorySlot& f = engine.inventory[foodIdx[k]];
        int bx = 43 + k * 47;
        uint16_t slotBg = (f.sickChancePct > 0) ? 0x3800 : 0x10A2;
        uint16_t slotBorder = (f.satietyTicks > 0) ? TFT_GREEN : (f.sickChancePct > 0 ? TFT_ORANGE : TFT_WHITE);
        tft.fillRoundRect(bx, 5, 44, 38, 3, slotBg);
        tft.drawRoundRect(bx, 5, 44, 38, 3, slotBorder);

        drawFoodIcon(tft, bx + 15, 17, f.id);

        tft.setTextDatum(TR_DATUM);
        tft.setTextColor(TFT_YELLOW, slotBg);
        tft.drawString(String("x") + f.quantity, bx + 42, 7, 1);

        tft.setTextDatum(TL_DATUM);
        tft.setTextColor(TFT_WHITE, slotBg);
        tft.drawString(String("+") + f.hungerGain, bx + 3, 30, 1);

        if (f.satietyTicks > 0) {
          tft.setTextDatum(TR_DATUM);
          tft.setTextColor(TFT_GREEN, slotBg);
          tft.drawString(String("S") + f.satietyTicks, bx + 42, 30, 1);
        } else if (f.sickChancePct > 0) {
          tft.setTextDatum(TR_DATUM);
          tft.setTextColor(TFT_RED, slotBg);
          tft.drawString("!", bx + 41, 30, 1);
        }
      }
    }
  }

  // Top Notification Toast (rendered below flyout if open so it stays visible)
  if (!chatMenuOpen && strlen(activeBannerMessage) > 0) {
    int bannerY = feedMenuOpen ? 50 : 2;
    tft.fillRoundRect(46, bannerY, 228, 20, 4, 0x18E3);
    tft.drawRoundRect(46, bannerY, 228, 20, 4, TFT_GOLD);
    tft.setTextColor(TFT_YELLOW, 0x18E3);
    tft.setTextDatum(MC_DATUM);
    tft.drawString(activeBannerMessage, 160, bannerY + 10, 1);
  }

  // =========================================================================
  // QUICK-CHAT PRESET PICKER MODAL OVERLAY (X: 8..312, Y: 12..228)
  // =========================================================================
  if (chatMenuOpen) {
    tft.fillRoundRect(8, 12, 304, 216, 6, 0x0842);
    tft.drawRoundRect(8, 12, 304, 216, 6, 0x3C3F);

    // Header
    tft.fillRoundRect(10, 14, 300, 24, 4, 0x012B);
    tft.setTextDatum(ML_DATUM);
    tft.setTextColor(TFT_WHITE, 0x012B);
    tft.drawString(String(LANG_DE ? "CHAT AN: " : "CHAT TO: ") + friendPet.name, 16, 26, 2);

    // Close [X] button (X: 280..308, Y: 16..36)
    tft.fillRoundRect(280, 16, 28, 20, 3, TFT_RED);
    tft.drawRoundRect(280, 16, 28, 20, 3, TFT_WHITE);
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(TFT_WHITE, TFT_RED);
    tft.drawString("X", 294, 26, 2);

    // 8 Selectable Preset Buttons (2 columns x 4 rows)
    static const uint16_t presetBorders[8] = {
      0x3C3F, TFT_GREEN, TFT_ORANGE, 0x99FD,
      COLOR_GOLD, TFT_RED, COLOR_GOLD, TFT_CYAN
    };
    for (uint8_t i = 0; i < 8; i++) {
      uint8_t col = i % 2;
      uint8_t row = i / 2;
      int bx = (col == 0) ? 14 : 164;
      int by = 42 + row * 38;
      tft.fillRoundRect(bx, by, 142, 33, 4, 0x10A2);
      tft.drawRoundRect(bx, by, 142, 33, 4, presetBorders[i]);
      tft.setTextDatum(MC_DATUM);
      tft.setTextColor(TFT_WHITE, 0x10A2);
      tft.drawString(getChatPreset(i), bx + 71, by + 16, 1);
    }

    // Bottom Cancel Bar
    tft.fillRoundRect(14, 196, 292, 24, 4, 0x028A);
    tft.drawRoundRect(14, 196, 292, 24, 4, TFT_WHITE);
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(TFT_WHITE, 0x028A);
    tft.drawString(LANG_DE ? "< ABBRECHEN <" : "< CANCEL <", 160, 208, 1);
  }
}

// ============================================================================
// TOUCH EVENT DISPATCHER
// ============================================================================
void handleTouchPress(uint16_t x, uint16_t y) {
  soundClick();

  // If in Sub-Screen (Stats, Bag/Cook/Shop, Jobs, Magic, Quests)
  if (inSubScreen) {
    // 1. Close button (X: 288 to 320, Y: 0 to 32) or bottom exit bar (Y >= 215)
    if ((x >= 288 && y <= 32) || y >= 215) {
      soundClick();
      inSubScreen = false;
      screenNeedsRefresh = true;
      return;
    }

    // 2. Tab selection (Y: 0 to 32)
    if (y <= 32) {
      if (x < 57) subScreenTab = 0;        // STAT
      else if (x < 113) subScreenTab = 1;  // BAG / COOK / SHOP
      else if (x < 169) subScreenTab = 2;  // JOBS
      else if (x < 226) subScreenTab = 3;  // MAGIC
      else subScreenTab = 4;               // QUESTS
      soundClick();
      renderSubScreen();
      return;
    }

    // 3. Tab 1: BAG / COOK / SHOP
    if (subScreenTab == 1) {
      if (y >= 32 && y <= 52) {
        if (x < 108) bagSubMode = 0;
        else if (x < 212) bagSubMode = 1;
        else bagSubMode = 2;
        soundClick();
        renderSubScreen();
        return;
      }

      if (bagSubMode == 0) {
        // Real inventory slots (Y: 55, 94, 133, 172)
        for (uint8_t i = 0; i < 4 && i < engine.inventoryCount; i++) {
          int ry = 55 + i * 39;
          if (y >= (uint16_t)ry && y <= (uint16_t)(ry + 35)) {
            EngineInventorySlot& sl = engine.inventory[i];
            if (sl.category == 0) {
              syncToEngine();
              FeedResult res = engine.feedWithItem(sl.id);
              syncFromEngine();
              if (res.success) {
                soundMunch();
                if (res.becameSick) {
                  soundWarning();
                  showBanner(LANG_DE ? "Bauchweh vom Snack! -15 HP" : "Snack Tummyache! -15 HP", 3000);
                } else {
                  showBanner(LANG_DE ? "Gefuettert! +" + String(res.hungerGain) + " FD" : "Fed! +" + String(res.hungerGain) + " FD");
                }
              }
            } else if (sl.category == 2) {
              bool wasSick = engine.isSick;
              syncToEngine();
              if (engine.usePotionItem(sl.id)) {
                syncFromEngine();
                soundTreatSent();
                if (wasSick && !engine.isSick) {
                  showBanner(LANG_DE ? "Geheilt! Pet wieder fit!" : "Cured! Pet feels great!");
                } else {
                  showBanner(LANG_DE ? "Arznei benutzt!" : "Item used!");
                }
              }
            } else {
              bagSubMode = 1; // Switch to Cook Recipes
            }
            checkLevelUp();
            renderSubScreen();
            return;
          }
        }
      } else if (bagSubMode == 1) {
        // Cook Recipes (Y: 56, 108, 160)
        uint8_t rIdx = 255;
        if (y >= 56 && y <= 104) rIdx = 0;
        else if (y >= 108 && y <= 156) rIdx = 1;
        else if (y >= 160 && y <= 208) rIdx = 2;

        if (rIdx < 3) {
          syncToEngine();
          if (engine.craftRecipe(rIdx)) {
            syncFromEngine();
            myPet.recipesCrafted++;
            soundLevelUp();
            showBanner(LANG_DE ? "Gericht gekocht -> Tasche!" : "Cooked dish -> Bag!");
            checkLevelUp();
          } else {
            soundWarning();
            showBanner(LANG_DE ? "Zutaten/Level fehlen!" : "Missing items/level!");
          }
          renderSubScreen();
          return;
        }
      } else {
        // Shop / Bazar purchases (Y: 55, 94, 133, 172)
        const char* sIds[4] = {"food_apple", "food_snack", "food_herb_salad", "item_salve"};
        for (uint8_t i = 0; i < 4; i++) {
          int ry = 55 + i * 39;
          if (y >= (uint16_t)ry && y <= (uint16_t)(ry + 35)) {
            syncToEngine();
            if (engine.purchaseItem(sIds[i])) {
              syncFromEngine();
              soundLevelUp();
              showBanner(LANG_DE ? "Gekauft -> im Inventar!" : "Bought -> added to Bag!");
            } else {
              soundWarning();
              showBanner(LANG_DE ? "Zu wenig Muenzen!" : "Not enough coins!");
            }
            renderSubScreen();
            return;
          }
        }
      }
      return;
    }

    // 4. Tab 2: JOBS (Start timed job, rewards payout upon completion)
    if (subScreenTab == 2) {
      if (activeJobIdx >= 0) {
        long remMs = (long)(activeJobEndMs - millis());
        long rem = (remMs > 0) ? (remMs / 1000L) : 0L;
        soundWarning();
        showBanner(String(LANG_DE ? "Beruf laeuft noch " : "Job running for ") + rem + "s");
        renderSubScreen();
        return;
      }
      if (y >= 48 && y <= 92) { // Job 1: Herbs + Apples (60s)
        if (myPet.energy >= 15) {
          myPet.energy -= 15;
          activeJobIdx = 0;
          activeJobEndMs = millis() + 60000UL;
          syncToEngine();
          engine.recordAction("play", 1);
          soundClick();
          showBanner(LANG_DE ? "Kraeutersammler gestartet (60s)!" : "Forager dispatched (60s)!");
        } else {
          soundWarning();
          showBanner(LANG_DE ? "Braucht 15 Energie!" : "Need 15 Energy!");
        }
        renderSubScreen();
        return;
      }
      if (y >= 96 && y <= 140) { // Job 2: Village Scout (120s)
        if (myPet.level < 2) {
          soundWarning();
          showBanner(LANG_DE ? "Benoetigt Level 2!" : "Requires Level 2!");
        } else if (myPet.energy >= 20) {
          myPet.energy -= 20;
          activeJobIdx = 1;
          activeJobEndMs = millis() + 120000UL;
          syncToEngine();
          soundClick();
          showBanner(LANG_DE ? "Dorfwache gestartet (120s)!" : "Scout dispatched (120s)!");
        } else {
          soundWarning();
          showBanner(LANG_DE ? "Braucht 20 Energie!" : "Need 20 Energy!");
        }
        renderSubScreen();
        return;
      }
      if (y >= 144 && y <= 188) { // Job 3: Crystal & Mushroom Mine (180s)
        if (myPet.level < 3) {
          soundWarning();
          showBanner(LANG_DE ? "Benoetigt Level 3!" : "Requires Level 3!");
        } else if (myPet.energy >= 30) {
          myPet.energy -= 30;
          activeJobIdx = 2;
          activeJobEndMs = millis() + 180000UL;
          syncToEngine();
          soundClick();
          showBanner(LANG_DE ? "Kristallmine gestartet (180s)!" : "Miner dispatched (180s)!");
        } else {
          soundWarning();
          showBanner(LANG_DE ? "Braucht 30 Energie!" : "Need 30 Energy!");
        }
        renderSubScreen();
        return;
      }
      return;
    }

    // 5. Tab 3: SELECTABLE MAGIC SPELLS & RESEARCH
    if (subScreenTab == 3) {
      if (y >= 45 && y <= 84) { // Spell 0: Purify (15 MP)
        if (myPet.mana >= 15) {
          myPet.mana -= 15;
          myPet.hygiene = 100;
          myPet.health = (uint8_t)min(100, (int)myPet.health + 10);
          engine.isSick = false;
          myPet.spellsCast++;
          syncToEngine();
          engine.addXP(25);
          engine.recordAction("magic", 1);
          syncFromEngine();
          soundTreatSent();
          showBanner(LANG_DE ? "Laeuterung! Bauchweh geheilt!" : "Purified! Sickness cured!");
          checkLevelUp();
        } else {
          soundWarning();
          showBanner(LANG_DE ? "Braucht 15 Mana!" : "Need 15 Mana!");
        }
        renderSubScreen();
        return;
      }
      if (y >= 87 && y <= 126) { // Spell 1: Vitality Aura (20 MP)
        if (myPet.mana >= 20) {
          myPet.mana -= 20;
          myPet.health = (uint8_t)min(100, (int)myPet.health + 30);
          myPet.happiness = (uint8_t)min(100, (int)myPet.happiness + 10);
          myPet.spellsCast++;
          syncToEngine();
          engine.addXP(30);
          engine.recordAction("magic", 1);
          syncFromEngine();
          soundTreatSent();
          showBanner(TXT_BANNER_AURA);
          checkLevelUp();
        } else {
          soundWarning();
          showBanner(TXT_BANNER_NO_MANA20);
        }
        renderSubScreen();
        return;
      }
      if (y >= 129 && y <= 168) { // Spell 2: Astral Feast + Buddy Bless (30 MP)
        if (myPet.mana >= 30) {
          myPet.mana -= 30;
          myPet.hunger = (uint8_t)min(100, (int)myPet.hunger + 35);
          if (engine.satietyTicks < 3) engine.satietyTicks = 3;
          myPet.spellsCast++;
          sendDirectSocketPacket("MAGIC", "Astral Feast Blessing");
          syncToEngine();
          engine.addXP(40);
          engine.recordAction("magic", 1);
          syncFromEngine();
          soundLevelUp();
          showBanner(LANG_DE ? "Astral-Festmahl! +35 FD & 3T" : "Astral Feast! +35 FD & 3T");
          checkLevelUp();
        } else {
          soundWarning();
          showBanner(TXT_BANNER_NO_MANA30);
        }
        renderSubScreen();
        return;
      }
      if (y >= 173 && y <= 210) { // Research Magic
        if (myPet.energy >= 15) {
          myPet.energy -= 15;
          myPet.mana = (uint8_t)min(100, (int)myPet.mana + 15);
          myPet.spellsCast++;
          syncToEngine();
          engine.addXP(35);
          engine.recordAction("magic", 1);
          syncFromEngine();
          soundTreatSent();
          showBanner(LANG_DE ? "Magie erforscht! +35 XP +15 MP" : "Magic studied! +35 XP +15 MP");
          checkLevelUp();
        } else {
          soundWarning();
          showBanner(LANG_DE ? "Braucht 15 Energie!" : "Need 15 Energy!");
        }
        renderSubScreen();
        return;
      }
      return;
    }

    // 6. Tab 4: SELECTABLE QUESTS & DAILY MISSIONS
    if (subScreenTab == 4) {
      if (y >= 45 && y <= 84) { // Streak Claim
        syncToEngine();
        if (engine.canClaimStreakBonus()) {
          uint16_t cR = engine.getStreakBonusCoins();
          uint16_t xR = engine.getStreakBonusXP();
          engine.claimDailyLoginStreakBonus();
          syncFromEngine();
          soundLevelUp();
          showBanner(String(TXT_BANNER_STREAK_CLAIMED) + engine.consecutiveLoginDays + " +$" + cR, 3000);
          checkLevelUp();
        } else {
          showBanner(LANG_DE ? "Heute schon abgeholt!" : "Already claimed today!");
        }
        renderSubScreen();
        return;
      }
      if (y >= 87 && y <= 126) { // Claim Completed Daily Mission
        syncToEngine();
        bool claimedAny = false;
        for (uint8_t m = 0; m < engine.missionCount; m++) {
          if (engine.missions[m].completed && !engine.missions[m].claimed) {
            engine.claimMission(engine.missions[m].id);
            syncFromEngine();
            claimedAny = true;
            soundLevelUp();
            showBanner(TXT_BANNER_MISSION_CLAIMED, 3000);
            checkLevelUp();
            break;
          }
        }
        if (!claimedAny) {
          soundWarning();
          showBanner(LANG_DE ? "Noch keine Mission fertig!" : "No mission ready yet!");
        }
        renderSubScreen();
        return;
      }
      if (y >= 129 && y <= 168) { // Co-op Quest 1: Scout Patrol
        if (myPet.energy >= 15) {
          myPet.energy -= 15;
          sendDirectSocketPacket("QUEST", "Co-op Scout");
          syncToEngine();
          engine.coins += 40;
          engine.addXP(50);
          engine.addKnownItemById("food_apple", 1);
          syncFromEngine();
          myPet.jobsCompleted++;
          soundLevelUp();
          showBanner(LANG_DE ? "Spaeher fertig! +$40 +1 Apfel" : "Scout Done! +$40 +1 Apple");
          checkLevelUp();
        } else {
          soundWarning();
          showBanner(LANG_DE ? "Braucht 15 Energie!" : "Need 15 Energy!");
        }
        renderSubScreen();
        return;
      }
      if (y >= 171 && y <= 210) { // Co-op Quest 2: Treasure Hunt
        if (myPet.energy >= 25) {
          myPet.energy -= 25;
          sendDirectSocketPacket("QUEST", "Treasure Hunt");
          syncToEngine();
          engine.coins += 65;
          engine.addXP(75);
          engine.addKnownItemById("item_salve", 1);
          syncFromEngine();
          myPet.jobsCompleted++;
          soundLevelUp();
          showBanner(LANG_DE ? "Schatz! +$65 +1 Heilsalbe" : "Treasure! +$65 +1 Salve");
          checkLevelUp();
        } else {
          soundWarning();
          showBanner(LANG_DE ? "Braucht 25 Energie!" : "Need 25 Energy!");
        }
        renderSubScreen();
        return;
      }
      return;
    }
    return;
  }

  // =========================================================================
  // MAIN SCREEN: CHECK QUICK-CHAT SELECTOR MODAL IF OPEN
  // =========================================================================
  if (chatMenuOpen) {
    // Close [X] (x >= 276 && y <= 40), Bottom Cancel Bar (y >= 194), or outside modal
    if ((x >= 276 && y <= 40) || y >= 194 || x < 8 || x > 312 || y < 12) {
      chatMenuOpen = false;
      screenNeedsRefresh = true;
      return;
    }
    // Check 8 Preset Buttons (2 columns x 4 rows)
    for (uint8_t i = 0; i < 8; i++) {
      uint8_t col = i % 2;
      uint8_t row = i / 2;
      int bx = (col == 0) ? 14 : 164;
      int by = 42 + row * 38;
      if ((int)x >= bx && (int)x <= bx + 142 && (int)y >= by && (int)y <= by + 33) {
        const char* msg = getChatPreset(i);
        chatMenuOpen = false;
        soundTreatSent();
        sendDirectSocketPacket("CHAT", msg);
        showBanner(String(LANG_DE ? "Chat: " : "Sent: ") + msg, 2500);
        screenNeedsRefresh = true;
        return;
      }
    }
    return;
  }

  // =========================================================================
  // MAIN SCREEN: CHECK OPEN HORIZONTAL FEED FLYOUT FIRST (X: 39..281, Y: 2..46)
  // =========================================================================
  if (feedMenuOpen && x >= 39 && x <= 281 && y >= 2 && y <= 46) {
    uint8_t foodIdx[6];
    uint8_t foodCnt = engine.getAvailableFoods(foodIdx, 6);

    if (foodCnt == 0) {
      if (x >= 214) {
        // Go straight to Shop in SubScreen
        feedMenuOpen = false;
        inSubScreen = true;
        subScreenTab = 1;
        bagSubMode = 2;
        soundClick();
        renderSubScreen();
        return;
      }
      feedMenuOpen = false;
      screenNeedsRefresh = true;
      return;
    }

    uint8_t showCnt = min((uint8_t)5, foodCnt);
    for (uint8_t k = 0; k < showCnt; k++) {
      int bx = 43 + k * 47;
      if ((int)x >= bx && (int)x <= bx + 44) {
        syncToEngine();
        FeedResult res = engine.feedWithItem(engine.inventory[foodIdx[k]].id);
        syncFromEngine();
        feedMenuOpen = false;
        if (res.success) {
          soundMunch();
          if (res.becameSick) {
            soundWarning();
            showBanner(LANG_DE ? "Bauchweh vom Snack! -15 HP" : "Snack Tummyache! -15 HP", 3000);
          } else if (res.satietyGained > 0) {
            showBanner(String("+") + res.hungerGain + " FD & SATT:" + res.satietyGained + "T", 2800);
          } else {
            showBanner(String(LANG_DE ? "Gefuettert! +" : "Fed! +") + res.hungerGain + " FD");
          }
          checkLevelUp();
        } else {
          soundWarning();
          showBanner(LANG_DE ? "Kein Essen vorhanden!" : "Food not available!");
        }
        screenNeedsRefresh = true;
        return;
      }
    }
    return;
  }

  // 1. LEFT COLUMN: Your Actions (X: 0 to 38)
  if (x < 38) {
    if (y < 48) {
      // Button 1: Toggle horizontal Feed Flyout Bar right next to FEED button
      feedMenuOpen = !feedMenuOpen;
      screenNeedsRefresh = true;
      return;
    } else if (y < 96) {
      feedMenuOpen = false;
      myPet.happiness = (uint8_t)min(100, (int)myPet.happiness + 20);
      myPet.energy = (uint8_t)max(0, (int)myPet.energy - 10);
      syncToEngine();
      engine.addXP(20);
      engine.recordAction("play", 1);
      syncFromEngine();
      showBanner(TXT_BANNER_PLAY);
    } else if (y < 144) {
      feedMenuOpen = false;
      myPet.hygiene = 100;
      myPet.happiness = (uint8_t)min(100, (int)myPet.happiness + 10);
      syncToEngine();
      engine.addXP(10);
      engine.recordAction("bath", 1);
      syncFromEngine();
      showBanner(TXT_BANNER_BATH);
    } else if (y < 192) {
      // Button 4 Left: Open Selectable Magic Spells & Research Menu
      feedMenuOpen = false;
      inSubScreen = true;
      subScreenTab = 3;
      soundClick();
      renderSubScreen();
      return;
    } else {
      // Button 5 Left: Open Full Stats & Overview Sub-Screen
      feedMenuOpen = false;
      inSubScreen = true;
      subScreenTab = 0;
      soundClick();
      renderSubScreen();
      return;
    }
    checkLevelUp();
    screenNeedsRefresh = true;
    return;
  }

  // 2. RIGHT COLUMN: Friend Actions (X: 282 to 320)
  if (x > 282) {
    feedMenuOpen = false;
    if (y < 48) {
      soundTreatSent();
      sendDirectSocketPacket("TREAT", "Heart Donut");
      myPet.treatsSent++;
      syncToEngine();
      engine.addXP(25);
      engine.recordAction("send_treat", 1);
      syncFromEngine();
      showBanner(TXT_BANNER_TREAT_SENT);
    } else if (y < 96) {
      // Button 2 Right: Open Selectable Quick-Chat Preset Menu
      chatMenuOpen = true;
      soundClick();
      screenNeedsRefresh = true;
      return;
    } else if (y < 144) {
      if (myPet.mana >= 30) {
        myPet.mana -= 30;
        sendDirectSocketPacket("MAGIC", "Friend Blessing");
        myPet.spellsCast++;
        syncToEngine();
        engine.addXP(40);
        engine.recordAction("magic", 1);
        syncFromEngine();
        soundTreatSent();
        showBanner(TXT_BANNER_BLESSING);
      } else {
        soundWarning();
        showBanner(TXT_BANNER_NO_MANA30);
      }
    } else if (y < 192) {
      // Button 4 Right: Open Selectable Quests & Daily Missions Menu
      inSubScreen = true;
      subScreenTab = 4;
      soundClick();
      renderSubScreen();
      return;
    } else {
      sendDirectSocketPacket("TRADE", "Top Hat");
      showBanner(TXT_BANNER_TRADE);
    }
    checkLevelUp();
    screenNeedsRefresh = true;
    return;
  }

  // Any other tap on the viewport closes open feed flyout if active
  if (feedMenuOpen) {
    feedMenuOpen = false;
    screenNeedsRefresh = true;
    return;
  }

  // 3. CENTRAL BRIDGE STATUS DOT (X: 150 to 170, Y: 105 to 135): Trigger OTA Check
  if (x >= 150 && x <= 170 && y >= 105 && y <= 135) {
    showBanner(TXT_BANNER_OTA_CHECK);
    checkForOTAUpdate();
    screenNeedsRefresh = true;
    return;
  }

  // 4. FOOTER COINS / STREAK / MISSIONS / SHOP (X: 40 to 105, Y: 205 to 240)
  if (x >= 40 && x <= 105 && y >= 205 && y <= 240) {
    // Open Shop or Missions directly cleanly
    if (engine.canClaimStreakBonus()) {
      syncToEngine();
      uint16_t cReward = engine.getStreakBonusCoins();
      uint16_t xReward = engine.getStreakBonusXP();
      engine.claimDailyLoginStreakBonus();
      syncFromEngine();
      soundLevelUp();
      showBanner(String(TXT_BANNER_STREAK_CLAIMED) + engine.consecutiveLoginDays + "! +$" + cReward + " +" + xReward + "XP", 3500);
      checkLevelUp();
      screenNeedsRefresh = true;
      return;
    }
    inSubScreen = true;
    subScreenTab = 1;
    bagSubMode = 2; // Open Shop directly
    soundClick();
    renderSubScreen();
    return;
  }

  // 5. CENTER LEFT: Tap Egg to Hatch or Petting
  if (x >= 40 && x <= 155 && y >= 70 && y <= 180) {
    if (strcmp(myPet.phase, "Egg") == 0) {
      myPet.eggWarmth += 25;
      if (myPet.eggWarmth >= 100) {
        strcpy(myPet.phase, "Baby");
        syncToEngine();
        soundHatch();
        showBanner(TXT_BANNER_EGG_HATCHED, 3500);
      } else {
        showBanner(TXT_BANNER_EGG_WARMED);
      }
    } else {
      myPet.happiness = (uint8_t)min(100, (int)myPet.happiness + 5);
      syncToEngine();
      showBanner(TXT_BANNER_CUDDLED);
    }
    screenNeedsRefresh = true;
  }
}

// ============================================================================
// LEVEL UP PROGRESSION
// ============================================================================
void checkLevelUp() {
  bool evolved = false;
  while (myPet.xp >= myPet.xpNext) {
    myPet.xp -= myPet.xpNext;
    myPet.level++;
    myPet.xpNext = (uint16_t)(100 * pow(1.30f, myPet.level - 1));
    myPet.health = 100;
    myPet.energy = 100;

    if (myPet.level >= 15) strcpy(myPet.phase, "Elder");
    else if (myPet.level >= 8) strcpy(myPet.phase, "Adult");
    else if (myPet.level >= 4 && strcmp(myPet.phase, "Egg") != 0) strcpy(myPet.phase, "Youth");
    evolved = true;
  }
  if (evolved) {
    syncToEngine();
    soundLevelUp();
    showBanner("LEVEL UP! Pet Evolved! 🌟", 3000);
  }
}

// ============================================================================
// DIRECT PEER-TO-PEER SOCKET SERVER & CLIENT (LAN SYNC)
// ============================================================================
void pollDirectSockets() {
  WiFiClient client = socketServer.available();
  if (client) {
    String jsonStr = client.readStringUntil('\n');
    StaticJsonDocument<384> doc;
    DeserializationError error = deserializeJson(doc, jsonStr);

    if (!error) {
      const char* type = doc["type"];
      const char* text = doc["text"];

      if (strcmp(type, "TREAT") == 0) {
        soundTreatReceived();
        myPet.hunger = (uint8_t)min(100, (int)myPet.hunger + 25);
        myPet.happiness = (uint8_t)min(100, (int)myPet.happiness + 20);
        syncToEngine();
        showBanner("Friend Sent You a Treat! 🍩", 3000);
      } else if (strcmp(type, "CHAT") == 0) {
        soundTreatReceived();
        if (text) {
          strncpy(friendPet.speechBalloon, text, sizeof(friendPet.speechBalloon) - 1);
          friendPet.speechBalloon[sizeof(friendPet.speechBalloon) - 1] = '\0';
          friendPet.balloonTimeout = millis() + 6000;
        }
      } else if (strcmp(type, "MAGIC") == 0) {
        soundTreatReceived();
        myPet.health = (uint8_t)min(100, (int)myPet.health + 30);
        myPet.happiness = (uint8_t)min(100, (int)myPet.happiness + 25);
        syncToEngine();
        showBanner("Received Friend Blessing! ✨", 3000);
      }
      screenNeedsRefresh = true;
    }
    client.stop();
  }
}

void sendDirectSocketPacket(const char* type, const char* textPayload) {
  if (WiFi.status() != WL_CONNECTED) return;

  if (strlen(friendPet.ipAddress) > 0) {
    WiFiClient client;
    if (client.connect(friendPet.ipAddress, friendPet.port)) {
      StaticJsonDocument<256> doc;
      doc["from"] = cfg_device_id;
      doc["type"] = type;
      doc["text"] = textPayload;
      serializeJson(doc, client);
      client.println();
      client.stop();
      return;
    }
  }

  // Fallback: Relay via PHP Server sync.php endpoint
  HTTPClient http;
  String url = cfg_server_url + "/sync.php";
  http.begin(url);
  http.addHeader("Content-Type", "application/json");

  StaticJsonDocument<256> doc;
  doc["fromDeviceId"] = cfg_device_id;
  doc["toDeviceId"]   = friendPet.deviceId;
  doc["type"]         = type;
  JsonObject payload  = doc.createNestedObject("payload");
  payload["text"]     = textPayload;

  String body;
  serializeJson(doc, body);
  http.POST(body);
  http.end();
}

// ============================================================================
// PHP SERVER REGISTRATION & DISCOVERY (WAN / CLOUD SYNC)
// ============================================================================
void syncWithPHPServer() {
  if (WiFi.status() != WL_CONNECTED) return;

  HTTPClient http;
  String url = cfg_server_url + "/register.php";
  http.begin(url);
  http.addHeader("Content-Type", "application/json");

  StaticJsonDocument<384> doc;
  doc["deviceId"]   = cfg_device_id;
  doc["deviceName"] = cfg_trainer_name;
  doc["petName"]    = myPet.name;
  doc["biome"]      = myPet.biome;
  doc["level"]      = myPet.level;
  doc["phase"]      = myPet.phase;
  doc["ipAddress"]  = WiFi.localIP().toString();
  doc["port"]       = SOCKET_PORT;

  JsonObject stats = doc.createNestedObject("stats");
  stats["health"]    = myPet.health;
  stats["hunger"]    = myPet.hunger;
  stats["energy"]    = myPet.energy;
  stats["happiness"] = myPet.happiness;

  String body;
  serializeJson(doc, body);
  int httpCode = http.POST(body);

  if (httpCode == 200) {
    HTTPClient friendHttp;
    String friendUrl = cfg_server_url + "/find_friend.php?friendId=" + cfg_friend_id;
    friendHttp.begin(friendUrl);
    int friendCode = friendHttp.GET();

    if (friendCode == 200) {
      String resp = friendHttp.getString();
      StaticJsonDocument<512> fDoc;
      deserializeJson(fDoc, resp);

      if (fDoc.containsKey("friend")) {
        JsonObject f = fDoc["friend"];
        friendPet.isOnline = f["isOnline"] | false;
        strncpy(friendPet.name, f["petName"] | "Friend", sizeof(friendPet.name) - 1);
        strncpy(friendPet.biome, f["biome"] | "desert", sizeof(friendPet.biome) - 1);
        friendPet.level = f["level"] | 1;
        const char* ip = f["ipAddress"];
        if (ip) strncpy(friendPet.ipAddress, ip, sizeof(friendPet.ipAddress) - 1);
      }
    }
    friendHttp.end();
  }
  http.end();
}

// ============================================================================
// SPIFFS DYNAMIC RULES LOADER (Zero-Downtime Hot-Reload)
// ============================================================================
void loadRulesFromSPIFFS() {
  if (!SPIFFS.exists("/rules.json")) {
    Serial.println("[SPIFFS] No /rules.json found. Creating initial config file...");
    File f = SPIFFS.open("/rules.json", "w");
    if (f) {
      StaticJsonDocument<512> initDoc;
      initDoc["version"] = RULES_VERSION;
      initDoc["hunger_decay"] = 1;
      initDoc["energy_decay"] = 1;
      initDoc["happiness_decay"] = 1;
      initDoc["starve_threshold"] = 20;
      serializeJson(initDoc, f);
      f.close();
    }
    return;
  }

  File f = SPIFFS.open("/rules.json", "r");
  if (!f) {
    Serial.println("[SPIFFS] Failed to open /rules.json for reading");
    return;
  }

  String rulesText = f.readString();
  f.close();

  Serial.printf("[SPIFFS] Loaded rules.json (%d bytes). Updating engine...\n", rulesText.length());
  engine.loadConfigFromText(rulesText);
}

// ============================================================================
// OVER-THE-AIR (OTA) A/B FIRMWARE & RULES UPDATE SYSTEM
// Hardware Reference: Dual 1.5MB Partitions (ota_0 / ota_1) via partitions.csv
// ============================================================================
void drawOTAProgressScreen(const String& stage, int percent) {
  tft.fillScreen(0x0821); // Dark Navy / Slate
  tft.drawRect(8, 8, 304, 224, TFT_YELLOW);
  tft.drawRect(10, 10, 300, 220, TFT_ORANGE);

  tft.setTextColor(TFT_YELLOW, 0x0821);
  tft.setTextDatum(TC_DATUM);
  tft.drawString("⚡ FIRMWARE UPDATE (OTA)", 160, 25, 4);

  tft.setTextColor(TFT_WHITE, 0x0821);
  tft.drawString("Dual-Slot A/B Partition Switch", 160, 65, 2);

  tft.setTextColor(TFT_CYAN, 0x0821);
  tft.drawString(stage, 160, 100, 2);

  // LCD Progress Bar
  tft.drawRect(30, 130, 260, 24, TFT_WHITE);
  int fillW = constrain((percent * 256) / 100, 0, 256);
  tft.fillRect(32, 132, fillW, 20, TFT_GREEN);
  tft.fillRect(32 + fillW, 132, 256 - fillW, 20, 0x18C3);

  tft.setTextColor(TFT_WHITE, 0x0821);
  tft.drawString(String(percent) + "%", 160, 162, 2);

  tft.setTextColor(TFT_RED, 0x0821);
  tft.drawString("DO NOT POWER OFF THE DEVICE!", 160, 195, 2);
}

bool performFirmwareOTA(const String& downloadUrl, size_t binarySize) {
  Serial.printf("[OTA] Starting Firmware OTA from: %s (Size: %u bytes)\n", downloadUrl.c_str(), binarySize);
  drawOTAProgressScreen("Connecting to Server...", 0);
  soundLevelUp();

  HTTPClient http;
  http.begin(downloadUrl);
  int httpCode = http.GET();
  if (httpCode != HTTP_CODE_OK) {
    Serial.printf("[OTA] HTTP GET failed, error code: %d\n", httpCode);
    showBanner("OTA Download Failed!");
    http.end();
    return false;
  }

  int contentLength = http.getSize();
  if (contentLength <= 0 && binarySize > 0) {
    contentLength = binarySize;
  }

  bool canBegin = Update.begin(contentLength > 0 ? contentLength : UPDATE_SIZE_UNKNOWN, U_FLASH);
  if (!canBegin) {
    Serial.println("[OTA] Not enough flash space to begin OTA update");
    showBanner("OTA Partition Error!");
    http.end();
    return false;
  }

  WiFiClient* stream = http.getStreamPtr();
  size_t written = 0;
  uint8_t buff[1024];
  int lastPct = -1;

  while (http.connected() && (written < (size_t)contentLength || contentLength <= 0)) {
    size_t avail = stream->available();
    if (avail) {
      int readBytes = stream->readBytes(buff, min(avail, sizeof(buff)));
      if (readBytes > 0) {
        Update.write(buff, readBytes);
        written += readBytes;

        if (contentLength > 0) {
          int pct = (written * 100) / contentLength;
          if (pct != lastPct && pct % 5 == 0) {
            lastPct = pct;
            drawOTAProgressScreen("Flashing Alternate Slot...", pct);
            Serial.printf("[OTA] Flashed: %d%% (%u / %d bytes)\n", pct, written, contentLength);
          }
        }
      }
    }
    delay(1);
    if (contentLength > 0 && written >= (size_t)contentLength) break;
  }

  if (Update.end()) {
    if (Update.isFinished()) {
      drawOTAProgressScreen("Success! Rebooting Slot...", 100);
      Serial.println("[OTA] Flash complete! Bootloader switching boot slot. Rebooting...");
      soundLevelUp();
      delay(1500);
      ESP.restart();
      return true;
    } else {
      Serial.println("[OTA] Flashing was not completed successfully.");
    }
  } else {
    Serial.printf("[OTA] Error encountered during update: #%d\n", Update.getError());
  }

  http.end();
  return false;
}

bool performRulesOTA(const String& downloadUrl) {
  Serial.printf("[OTA] Downloading updated game rules from: %s\n", downloadUrl.c_str());
  HTTPClient http;
  http.begin(downloadUrl);
  int httpCode = http.GET();

  if (httpCode == HTTP_CODE_OK) {
    String payload = http.getString();
    File f = SPIFFS.open("/rules.json", "w");
    if (f) {
      f.print(payload);
      f.close();
      Serial.println("[OTA] Dynamic rules saved to SPIFFS /rules.json");
      engine.loadConfigFromText(payload);
      soundLevelUp();
      showBanner("Rules Hot-Reloaded (OTA)! 📜", 3500);
      http.end();
      return true;
    }
  }
  http.end();
  return false;
}

void checkForOTAUpdate() {
  if (WiFi.status() != WL_CONNECTED) return;
  Serial.println("[OTA] Querying PHP Server for available firmware or rules update...");

  HTTPClient http;
  String url = cfg_server_url + "/ota_check.php?device=" + cfg_device_id + "&fw_version=" + FIRMWARE_VERSION + "&rules_version=" + RULES_VERSION;
  http.begin(url);
  int httpCode = http.GET();

  if (httpCode == HTTP_CODE_OK) {
    String resp = http.getString();
    StaticJsonDocument<768> doc;
    DeserializationError err = deserializeJson(doc, resp);
    if (!err) {
      bool fwAvailable    = doc["fw_update_available"] | false;
      bool rulesAvailable = doc["rules_update_available"] | false;

      // 1. Dynamic Rules Hot-Reload (SPIFFS)
      if (rulesAvailable) {
        const char* rUrl = doc["rules_url"];
        if (rUrl) {
          String fullUrl = String(rUrl).startsWith("http") ? String(rUrl) : (cfg_server_url + "/" + String(rUrl));
          performRulesOTA(fullUrl);
        }
      }

      // 2. Hardware Binary OTA (A/B Partition Switch)
      if (fwAvailable) {
        const char* fUrl = doc["fw_url"];
        size_t fSize = doc["fw_size"] | 0;
        if (fUrl) {
          String fullFwUrl = String(fUrl).startsWith("http") ? String(fUrl) : (cfg_server_url + "/" + String(fUrl));
          performFirmwareOTA(fullFwUrl, fSize);
        }
      }
    }
  }
  http.end();
}

