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
#include <esp_ota_ops.h>    // For esp_ota_mark_app_valid_cancel_rollback()
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
  #define TXT_STATS      "STATS"
  #define TXT_TREAT      "SNACK"
  #define TXT_CHAT       "CHAT"
  #define TXT_MAGIC_PLUS "MGC+"
  #define TXT_QUEST      "QUEST"
  #define TXT_TRADE      "TAUSCH"
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
  #define TXT_STATS      "STATS"
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
  // Satiety shield & sickness (matching web sim)
  uint8_t satietyTicks = 0;
  bool isSick = false;
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

// Timing State
unsigned long lastTickTime = 0;
unsigned long lastServerSyncTime = 0;
unsigned long lastOtaCheckTime = 0;
unsigned long lastRenderTime = 0;
bool screenNeedsRefresh = true;
char activeBannerMessage[32] = "";
unsigned long bannerTimeout = 0;
bool inServerErrorState = false;
bool firstRender = true;  // Track initial full render

// Flyout Menu State (matching web simulator)
bool feedMenuOpen = false;
bool spellMenuOpen = false;
bool prevFeedMenuOpen = false;
bool prevSpellMenuOpen = false;
bool forceFullRender = false;
uint8_t selectedFoodIndex = 0;
uint8_t selectedSpellIndex = 0;
uint8_t availableFoodCount = 0;
uint8_t availableSpellCount = 0;

// Screen State (matching web simulator RPG modal tabs)
enum ScreenState { SCREEN_MAIN, SCREEN_STATS, SCREEN_INVENTORY, SCREEN_JOBS, SCREEN_MAGIC, SCREEN_SHOP, SCREEN_MISSIONS };
ScreenState currentScreen = SCREEN_MAIN;
uint8_t statsTabIndex = 0;  // 0=Stats, 1=Inventory, 2=Jobs, 3=Magic, 4=Shop, 5=Missions
const char* statsTabNames[6] = { "Stats", "Inventar", "Jobs", "Magie", "Shop", "Missionen" };
const char* statsTabNamesDE[6] = { "Status", "Inventar", "Berufe", "Zauber", "Shop", "Missionen" };

// Stats screen scrolling
int16_t statsScrollY = 0;
int16_t statsScrollMaxY = 0;
int16_t lastTouchY = -1;
bool isScrolling = false;

// Food item indices for feed flyout (mapped to inventory)
struct FoodItem {
  char id[20];
  char name[16];
  char name_de[16];
  uint8_t hungerGain;
  uint8_t healthGain;
  uint8_t energyGain;
  uint8_t happinessGain;
  uint8_t satietyTicks;
  uint8_t sickChancePct;
  bool cureSickness;
  uint8_t quantity;
  const char* icon;
};
FoodItem feedFlyoutItems[8];

// Spell data for spell flyout (matching web simulator SPELLS catalog)
struct SpellData {
  char id[20];
  char name[24];
  char name_de[24];
  uint8_t manaCost;
  char target;  // 's'=self, 'f'=friend, 'b'=both
  uint8_t healthGain;
  uint8_t hungerGain;
  uint8_t happinessGain;
  uint8_t energyGain;
  uint8_t satietyTicks;
  bool curesSickness;
  const char* icon;
};
SpellData spellFlyoutItems[5];


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

void showBanner(const char* msg, uint16_t durationMs = 2500) {
  strncpy(activeBannerMessage, msg, sizeof(activeBannerMessage) - 1);
  bannerTimeout = millis() + durationMs;
  screenNeedsRefresh = true;
}

void showBanner(const String& msg, uint16_t durationMs = 2500) {
  showBanner(msg.c_str(), durationMs);
}

// ----------------------------------------------------------------------------
// Forward Declarations
// ----------------------------------------------------------------------------
void renderCYDScreen(bool fullRender = false);
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

  // Apply trainer name to local pet (was defaulting to "Leafkin")
  if (cfg_trainer_name.length() > 0) {
    strncpy(myPet.name, cfg_trainer_name.c_str(), sizeof(myPet.name) - 1);
  }

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
  tft.drawString("CYD Tamagotchi RPG", 160, 70, 4);
  tft.drawString(String("Connecting to ") + cfg_wifi_ssid + "...", 160, 110, 2);

  // Show firmware & rules version
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.setTextDatum(MC_DATUM);
  tft.setTextSize(1);
  tft.drawString("FW: " + String(FIRMWARE_VERSION) + "  |  Rules: " + String(RULES_VERSION), 160, 140, 1);
  tft.drawString("Device: " + String(DEVICE_ID), 160, 155, 1);

  // Show active OTA partition (A/B)
  const esp_partition_t* running = esp_ota_get_running_partition();
  String partLabel = running ? String(running->label) : "?";
  String partSlot = partLabel == "ota_0" ? "A" : (partLabel == "ota_1" ? "B" : partLabel);
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.drawString("Boot Slot: " + partSlot + " (" + partLabel + ")", 160, 170, 1);

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

    // 8. Prevent OTA rollback - mark this firmware as valid after successful boot
    // This must be called after successful initialization to prevent bootloader from rolling back
    esp_err_t otaErr = esp_ota_mark_app_valid_cancel_rollback();
    if (otaErr == ESP_OK) {
      Serial.println("[OTA] Rollback cancelled - firmware marked as valid");
    } else {
      Serial.printf("[OTA] Warning: Could not cancel rollback: %s\n", esp_err_to_name(otaErr));
    }
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


  // 4. Pet Decay Loop (Every 6 seconds) - Logic only
  if (now - lastTickTime > PET_TICK_INTERVAL_MS) {
    if (strcmp(myPet.phase, "Egg") != 0) {
      bool statsChanged = false;
      if (myPet.hunger > 0) { myPet.hunger--; statsChanged = true; }
      if (myPet.happiness > 0) { myPet.happiness--; statsChanged = true; }
      if (myPet.hunger < 20 && myPet.health > 0) { myPet.health--; statsChanged = true; }
      if (myPet.mana < 100) { myPet.mana = min(100, myPet.mana + 2); statsChanged = true; }
      // Only request visual refresh if stats actually changed visibly (bar width changed)
      if (statsChanged) screenNeedsRefresh = true;
    }
    lastTickTime = now;
  }

  // 5. Speech balloon / Banner expiration
  if (friendPet.balloonTimeout > 0 && now > friendPet.balloonTimeout) {
    friendPet.balloonTimeout = 0;
    screenNeedsRefresh = true;
  }
  if (bannerTimeout > 0 && now > bannerTimeout) {
    activeBannerMessage[0] = '\0';
    bannerTimeout = 0;
    forceFullRender = true;  // Force full render to clear banner area in center partition
    screenNeedsRefresh = true;
  }

  // Detect flyout close -> force full render to clear flyout area
  if (prevFeedMenuOpen && !feedMenuOpen) {
    forceFullRender = true;
  }
  if (prevSpellMenuOpen && !spellMenuOpen) {
    forceFullRender = true;
  }
  prevFeedMenuOpen = feedMenuOpen;
  prevSpellMenuOpen = spellMenuOpen;

  // 6. Refresh Display if dirty (throttle to max 10 FPS to prevent flicker)
  if (screenNeedsRefresh && (now - lastRenderTime > 100)) {
    bool doFullRender = firstRender || forceFullRender;
    renderCYDScreen(doFullRender);
    firstRender = false;
    forceFullRender = false;
    screenNeedsRefresh = false;
    lastRenderTime = now;
  }

  delay(15);
}

// ============================================================================
// Forward declarations for tab render functions
// ============================================================================
int renderStatsTab(int y, int h, BiomeTheme& myBiome, bool measureOnly);
int renderInventoryTab(int y, int h, BiomeTheme& myBiome, bool measureOnly);
int renderJobsTab(int y, int h, BiomeTheme& myBiome, bool measureOnly);
int renderMagicTab(int y, int h, BiomeTheme& myBiome, bool measureOnly);
int renderShopTab(int y, int h, BiomeTheme& myBiome, bool measureOnly);
int renderMissionsTab(int y, int h, BiomeTheme& myBiome, bool measureOnly);

// ============================================================================
// STATS/INVENTORY/JOBS/MAGIC SCREEN (matching web simulator RPG modal)
// ============================================================================
void renderStatsScreen(bool fullRender, BiomeTheme& myBiome) {
  if (fullRender) {
    // Clear full screen
    tft.fillScreen(myBiome.bgColor);

    // Top bar with back button and title
    tft.fillRect(0, 0, 320, 30, 0x4A69);
    // Back button (left)
    tft.fillRoundRect(2, 2, 40, 26, 4, 0x78D8);
    tft.drawRoundRect(2, 2, 40, 26, 4, TFT_WHITE);
    tft.setTextColor(TFT_WHITE, 0x78D8);
    tft.setTextDatum(MC_DATUM);
    tft.drawString("<", 22, 15, 2);

    // Title
    tft.setTextColor(TFT_WHITE, 0x4A69);
    tft.setTextDatum(MC_DATUM);
    tft.drawString(LANG_DE ? statsTabNamesDE[statsTabIndex] : statsTabNames[statsTabIndex], 160, 15, 2);

    // Tab bar (6 tabs, scrollable)
    int tabY = 32;
    int tabHeight = 22;
    int tabWidth = 320 / 3;  // Show 3 tabs at a time
    int startTab = max(0, (int)statsTabIndex - 1);
    if (startTab > 3) startTab = 3;

    for (int i = 0; i < 3 && (startTab + i) < 6; i++) {
      int tabIdx = startTab + i;
      int tabX = i * tabWidth;
      uint16_t bgColor = (tabIdx == statsTabIndex) ? 0x78D8 : 0x18E3;
      uint16_t borderColor = (tabIdx == statsTabIndex) ? TFT_WHITE : 0x4A69;
      uint16_t textColor = (tabIdx == statsTabIndex) ? TFT_WHITE : TFT_LIGHTGREY;

      tft.fillRect(tabX, tabY, tabWidth, tabHeight, bgColor);
      tft.drawRect(tabX, tabY, tabWidth, tabHeight, borderColor);
      tft.setTextColor(textColor, bgColor);
      tft.setTextDatum(MC_DATUM);
      const char* tabName = LANG_DE ? statsTabNamesDE[tabIdx] : statsTabNames[tabIdx];
      tft.drawString(tabName, tabX + tabWidth/2, tabY + tabHeight/2, 1);
    }
  }

  // Content area with scrolling
  int contentY = 56 - statsScrollY;
  int contentH = 240 - 56;
  int viewportH = contentH;

  // Clear content area
  tft.fillRect(0, 56, 320, viewportH, myBiome.bgColor);

  // Calculate content height and max scroll (measure pass)
  int16_t contentHeight = 0;
  switch (statsTabIndex) {
    case 0: contentHeight = renderStatsTab(contentY, contentH, myBiome, true); break;
    case 1: contentHeight = renderInventoryTab(contentY, contentH, myBiome, true); break;
    case 2: contentHeight = renderJobsTab(contentY, contentH, myBiome, true); break;
    case 3: contentHeight = renderMagicTab(contentY, contentH, myBiome, true); break;
    case 4: contentHeight = renderShopTab(contentY, contentH, myBiome, true); break;
    case 5: contentHeight = renderMissionsTab(contentY, contentH, myBiome, true); break;
  }
  statsScrollMaxY = max(0, contentHeight - viewportH);
  if (statsScrollY > statsScrollMaxY) statsScrollY = statsScrollMaxY;

  // Adjust contentY for current scroll position
  contentY = 56 - statsScrollY;

  // Draw pass - render actual content with viewport clipping
  tft.setViewport(0, 56, 320, viewportH);
  switch (statsTabIndex) {
    case 0: renderStatsTab(contentY, contentH, myBiome, false); break;
    case 1: renderInventoryTab(contentY, contentH, myBiome, false); break;
    case 2: renderJobsTab(contentY, contentH, myBiome, false); break;
    case 3: renderMagicTab(contentY, contentH, myBiome, false); break;
    case 4: renderShopTab(contentY, contentH, myBiome, false); break;
    case 5: renderMissionsTab(contentY, contentH, myBiome, false); break;
  }
  tft.resetViewport();

  // Draw scroll indicator on right edge
  if (statsScrollMaxY > 0) {
    int barH = max(10, (viewportH * viewportH) / (contentHeight + viewportH));
    int barY = 56 + (statsScrollY * (viewportH - barH)) / statsScrollMaxY;
    tft.fillRect(315, 56, 5, viewportH, 0x18E3);
    tft.fillRect(315, barY, 5, barH, 0x78D8);
  }
}

// ============================================================================
// Stats Tab: Detailed pet stats
// ============================================================================
int renderStatsTab(int y, int h, BiomeTheme& myBiome, bool measureOnly) {
  int startY = y;
  tft.setTextDatum(TL_DATUM);
  int lineH = 18;
  int x = 10;

  tft.setTextColor(TFT_WHITE, myBiome.bgColor);
  if (!measureOnly) { tft.drawString("Name: " + String(myPet.name), x, y, 2); y += lineH + 2; }
  else { y += lineH + 2; }
  if (!measureOnly) { tft.drawString("Level: " + String(myPet.level) + "  XP: " + String(myPet.xp) + "/" + String(myPet.xpNext), x, y, 1); y += lineH; }
  else { y += lineH; }
  if (!measureOnly) { tft.drawString("Phase: " + String(myPet.phase), x, y, 1); y += lineH; }
  else { y += lineH; }
  if (!measureOnly) { tft.drawString("Biome: " + String(myPet.biome), x, y, 1); y += lineH + 4; }
  else { y += lineH + 4; }

  tft.setTextColor(TFT_YELLOW, myBiome.bgColor);
  if (!measureOnly) { tft.drawString("Coins: $" + String(myPet.coins), x, y, 2); y += lineH + 2; }
  else { y += lineH + 2; }

  tft.setTextColor(TFT_RED, myBiome.bgColor);
  if (!measureOnly) { tft.drawString("HP: " + String(myPet.health) + "/100", x, y, 1); y += lineH; }
  else { y += lineH; }
  if (!measureOnly) { drawMiniBar(tft, x, y, 200, 10, myPet.health, TFT_RED); y += 14; }
  else { y += 14; }

  tft.setTextColor(TFT_ORANGE, myBiome.bgColor);
  if (!measureOnly) { tft.drawString("Hunger: " + String(myPet.hunger) + "/100", x, y, 1); y += lineH; }
  else { y += lineH; }
  if (!measureOnly) { drawMiniBar(tft, x, y, 200, 10, myPet.hunger, TFT_ORANGE); y += 14; }
  else { y += 14; }

  tft.setTextColor(TFT_GREEN, myBiome.bgColor);
  if (!measureOnly) { tft.drawString("Happiness: " + String(myPet.happiness) + "/100", x, y, 1); y += lineH; }
  else { y += lineH; }
  if (!measureOnly) { drawMiniBar(tft, x, y, 200, 10, myPet.happiness, TFT_GREEN); y += 14; }
  else { y += 14; }

  tft.setTextColor(TFT_CYAN, myBiome.bgColor);
  if (!measureOnly) { tft.drawString("Energy: " + String(myPet.energy) + "/100", x, y, 1); y += lineH; }
  else { y += lineH; }
  if (!measureOnly) { drawMiniBar(tft, x, y, 200, 10, myPet.energy, TFT_CYAN); y += 14; }
  else { y += 14; }

  tft.setTextColor(COLOR_MANA_PURPLE, myBiome.bgColor);
  if (!measureOnly) { tft.drawString("Mana: " + String(myPet.mana) + "/100", x, y, 1); y += lineH; }
  else { y += lineH; }
  if (!measureOnly) { drawMiniBar(tft, x, y, 200, 10, myPet.mana, COLOR_MANA_PURPLE); y += 14; }
  else { y += 14; }

  tft.setTextColor(TFT_LIGHTGREY, myBiome.bgColor);
  if (!measureOnly) { tft.drawString("Hygiene: " + String(myPet.hygiene) + "/100", x, y, 1); y += lineH; }
  else { y += lineH; }
  if (!measureOnly) { drawMiniBar(tft, x, y, 200, 10, myPet.hygiene, TFT_LIGHTGREY); y += 14; }
  else { y += 14; }

  y += 6;
  tft.setTextColor(TFT_WHITE, myBiome.bgColor);
  if (!measureOnly) { tft.drawString("Spells Cast: " + String(myPet.spellsCast), x, y, 1); y += lineH; }
  else { y += lineH; }
  if (!measureOnly) { tft.drawString("Treats Sent: " + String(myPet.treatsSent), x, y, 1); y += lineH; }
  else { y += lineH; }
  if (!measureOnly) { tft.drawString("Login Streak: " + String(engine.consecutiveLoginDays), x, y, 1); y += lineH; }
  else { y += lineH; }

  return y - startY;
}

// ============================================================================
// Inventory Tab: Items owned
// ============================================================================
int renderInventoryTab(int y, int h, BiomeTheme& myBiome, bool measureOnly) {
  int startY = y;
  tft.setTextDatum(TL_DATUM);
  int lineH = 18;
  int x = 10;

  tft.setTextColor(TFT_YELLOW, myBiome.bgColor);
  if (!measureOnly) { tft.drawString("INVENTAR", x, y, 2); }
  y += lineH + 4;

  // Food items
  tft.setTextColor(TFT_ORANGE, myBiome.bgColor);
  if (!measureOnly) { tft.drawString("Nahrung:", x, y, 1); }
  y += lineH;
  for (uint8_t i = 0; i < engine.inventoryCount; i++) {
    EngineInventorySlot& slot = engine.inventory[i];
    if (slot.quantity > 0 && (strstr(slot.id, "food_") == slot.id || strstr(slot.id, "item_salve") == slot.id)) {
      tft.setTextColor(TFT_WHITE, myBiome.bgColor);
      if (!measureOnly) { tft.drawString("  " + String(slot.name) + " x" + String(slot.quantity), x, y, 1); }
      y += lineH;
    }
  }
  y += 4;

  // Materials
  tft.setTextColor(TFT_CYAN, myBiome.bgColor);
  if (!measureOnly) { tft.drawString("Materialien:", x, y, 1); }
  y += lineH;
  for (uint8_t i = 0; i < engine.inventoryCount; i++) {
    EngineInventorySlot& slot = engine.inventory[i];
    if (slot.quantity > 0 && strstr(slot.id, "mat_") == slot.id) {
      tft.setTextColor(TFT_WHITE, myBiome.bgColor);
      if (!measureOnly) { tft.drawString("  " + String(slot.name) + " x" + String(slot.quantity), x, y, 1); }
      y += lineH;
    }
  }
  y += 4;

  // Other items
  tft.setTextColor(0xD9E0, myBiome.bgColor);
  if (!measureOnly) { tft.drawString("Sonstiges:", x, y, 1); }
  y += lineH;
  for (uint8_t i = 0; i < engine.inventoryCount; i++) {
    EngineInventorySlot& slot = engine.inventory[i];
    if (slot.quantity > 0 && strstr(slot.id, "food_") != slot.id && strstr(slot.id, "mat_") != slot.id && strstr(slot.id, "item_salve") != slot.id) {
      tft.setTextColor(TFT_WHITE, myBiome.bgColor);
      if (!measureOnly) { tft.drawString("  " + String(slot.name) + " x" + String(slot.quantity), x, y, 1); }
      y += lineH;
    }
  }

  return y - startY;
}

// ============================================================================
// Jobs Tab: Available jobs
// ============================================================================
int renderJobsTab(int y, int h, BiomeTheme& myBiome, bool measureOnly) {
  int startY = y;
  tft.setTextDatum(TL_DATUM);
  int lineH = 18;
  int x = 10;

  tft.setTextColor(TFT_YELLOW, myBiome.bgColor);
  if (!measureOnly) { tft.drawString("JOBS / BERUFE", x, y, 2); y += lineH + 4; }
  else { y += lineH + 4; }

  // Define jobs matching rules.json
  struct JobInfo { const char* id; const char* name; const char* name_de; uint8_t energy; uint8_t minLvl; };
  JobInfo jobs[] = {
    {"job_forage", "Forage", "Sammeln", 20, 1},
    {"job_mine", "Mining", "Bergbau", 30, 2},
    {"job_fish", "Fishing", "Angeln", 25, 2},
    {"job_craft", "Crafting", "Handwerk", 35, 3},
    {"job_hunt", "Hunting", "Jagen", 40, 4},
    {"job_explore", "Explore", "Erforschen", 50, 5}
  };

  for (int i = 0; i < 6; i++) {
    bool unlocked = myPet.level >= jobs[i].minLvl;
    bool canAfford = myPet.energy >= jobs[i].energy;

    uint16_t color = unlocked ? (canAfford ? TFT_WHITE : TFT_ORANGE) : TFT_DARKGREY;
    tft.setTextColor(color, myBiome.bgColor);

    String line = String(jobs[i].name_de) + " (Lvl " + jobs[i].minLvl + ") - " + jobs[i].energy + " EN";
    if (!unlocked) line = "[" + line + "]";
    if (!canAfford && unlocked) line += " (!EN)";
    if (!measureOnly) { tft.drawString(line, x, y, 1); y += lineH; }
    else { y += lineH; }
  }

  y += 4;
  tft.setTextColor(TFT_CYAN, myBiome.bgColor);
  if (!measureOnly) { tft.drawString("Energie: " + String(myPet.energy) + "/100", x, y, 2); y += 24; }
  else { y += 24; }

  return y - startY;
}

// ============================================================================
// Magic Tab: Learned spells
// ============================================================================
int renderMagicTab(int y, int h, BiomeTheme& myBiome, bool measureOnly) {
  int startY = y;
  tft.setTextDatum(TL_DATUM);
  int lineH = 18;
  int x = 10;

  tft.setTextColor(TFT_YELLOW, myBiome.bgColor);
  if (!measureOnly) { tft.drawString("MAGIE / ZAUBER", x, y, 2); y += lineH + 4; }
  else { y += lineH + 4; }

  // Show all spells with level requirement
  struct SpellInfo { const char* id; const char* name; const char* name_de; uint8_t mana; char target; uint8_t minLvl; };
  SpellInfo spells[] = {
    {"spl_cleanse", "Purify", "Lauterung", 15, 's', 1},
    {"spl_heal", "Vitality Aura", "Vitalitaet", 20, 's', 2},
    {"spl_treat_burst", "Astral Feast", "Astral-Fest", 25, 's', 3},
    {"spl_friend_bless", "Friend Blessing", "Freundes-Segen", 30, 'f', 3},
    {"spl_rain_dance", "Astral Harmony", "Astrale Harm", 45, 'b', 5}
  };

  for (int i = 0; i < 5; i++) {
    bool learned = myPet.level >= spells[i].minLvl;
    bool canAfford = myPet.mana >= spells[i].mana;

    uint16_t color = learned ? (canAfford ? TFT_WHITE : TFT_ORANGE) : TFT_DARKGREY;
    tft.setTextColor(color, myBiome.bgColor);

    const char* targetStr = spells[i].target == 's' ? "Selbst" : (spells[i].target == 'f' ? "Freund" : "Beide");
    String line = String(spells[i].name_de) + " - " + spells[i].mana + " MN - " + targetStr;
    if (!learned) line = "[" + line + " Lvl" + spells[i].minLvl + "]";
    if (!canAfford && learned) line += " (!MN)";
    if (!measureOnly) { tft.drawString(line, x, y, 1); y += lineH; }
    else { y += lineH; }
  }

  y += 4;
  tft.setTextColor(COLOR_MANA_PURPLE, myBiome.bgColor);
  if (!measureOnly) { tft.drawString("Mana: " + String(myPet.mana) + "/100", x, y, 2); y += 24; }
  else { y += 24; }

  return y - startY;
}

// ============================================================================
// Shop Tab
// ============================================================================
int renderShopTab(int y, int h, BiomeTheme& myBiome, bool measureOnly) {
  int startY = y;
  tft.setTextDatum(TL_DATUM);
  int lineH = 18;
  int x = 10;

  tft.setTextColor(TFT_YELLOW, myBiome.bgColor);
  if (!measureOnly) { tft.drawString("SHOP", x, y, 2); }
  y += lineH + 4;

  tft.setTextColor(TFT_WHITE, myBiome.bgColor);
  if (!measureOnly) { tft.drawString("Verwende PHP Server oder", x, y, 1); }
  y += lineH;
  if (!measureOnly) { tft.drawString("Web-Simulator fuer Kaufe.", x, y, 1); }
  y += lineH + 4;

  tft.setTextColor(COLOR_GOLD, myBiome.bgColor);
  if (!measureOnly) { tft.drawString("Coins: $" + String(myPet.coins), x, y, 2); }
  y += 24;

  return y - startY;
}

// ============================================================================
// Missions Tab
// ============================================================================
int renderMissionsTab(int y, int h, BiomeTheme& myBiome, bool measureOnly = false) {
  int startY = y;
  tft.setTextDatum(TL_DATUM);
  int lineH = 18;
  int x = 10;

  tft.setTextColor(TFT_YELLOW, myBiome.bgColor);
  if (!measureOnly) { tft.drawString("TAEGLICHE MISSIONEN", x, y, 2); y += lineH + 4; }
  else { y += lineH + 4; }

  tft.setTextColor(TFT_WHITE, myBiome.bgColor);
  if (!measureOnly) { tft.drawString("Missionen werden ueber", x, y, 1); y += lineH; }
  else { y += lineH; }
  if (!measureOnly) { tft.drawString("PHP Server synchronisiert.", x, y, 1); y += lineH + 4; }
  else { y += lineH + 4; }

  tft.setTextColor(TFT_GREEN, myBiome.bgColor);
  if (!measureOnly) { tft.drawString("Streak: " + String(engine.consecutiveLoginDays) + " Tage", x, y, 2); y += 24; }
  else { y += 24; }
  if (engine.canClaimStreakBonus()) {
    tft.setTextColor(TFT_YELLOW, myBiome.bgColor);
    if (!measureOnly) { tft.drawString("Bonus verfuegbar!", x, y, 1); y += lineH; }
    else { y += lineH; }
  }

  return y - startY;
}

// ============================================================================
// 2.8" LCD 320x240 UI RENDERING ENGINE
// ============================================================================
void renderCYDScreen(bool fullRender) {
  BiomeTheme myBiome = getBiomeTheme(myPet.biome);
  BiomeTheme frBiome = getBiomeTheme(friendPet.biome);

  // =========================================================================
  // STATS/INVENTORY/JOBS/MAGIC SCREEN (overlay - full screen)
  // =========================================================================
  if (currentScreen != SCREEN_MAIN) {
    renderStatsScreen(fullRender, myBiome);
    // Draw flyouts on top if open (shouldn't happen in stats screen but safe)
    if (feedMenuOpen) { populateFeedFlyout(); drawFeedFlyout(); }
    if (spellMenuOpen) { populateSpellFlyout(); drawSpellFlyout(); }
    return;
  }

  // =========================================================================
  // MAIN SCREEN - FULL RENDER: Static UI (backgrounds, buttons, labels, borders)
  // =========================================================================
  if (fullRender) {
    // 1. Clear backgrounds for Left (Your Pet) & Right (Friend Pet)
    tft.fillRect(38, 0, 122, 240, myBiome.bgColor);
    tft.fillRect(160, 0, 122, 240, frBiome.bgColor);

    // 2. Center Partition Line & Connection Dot
    tft.drawFastVLine(160, 0, 240, COLOR_UI_BORDER);
    tft.fillCircle(160, 120, 5, friendPet.isOnline ? TFT_GREEN : TFT_RED);
    tft.drawCircle(160, 120, 5, TFT_WHITE);

    // LEFT COLUMN: YOUR PET CONTROLS (X: 0 to 38)
    tft.fillRect(0, 0, 38, 240, COLOR_UI_PANEL);
    tft.drawFastVLine(37, 0, 240, COLOR_UI_BORDER);

    drawTouchButton(tft, 2, 2,   34, 44, TXT_FEED,  0x1404, TFT_WHITE, 1);
    drawTouchButton(tft, 2, 50,  34, 44, TXT_PLAY,  0x02F5, TFT_WHITE, 2);
    drawTouchButton(tft, 2, 98,  34, 44, TXT_BATH,  0x047A, TFT_WHITE, 3);
    drawTouchButton(tft, 2, 146, 34, 44, TXT_MAGIC, 0x78D8, TFT_WHITE, 4);
    drawTouchButton(tft, 2, 194, 34, 44, TXT_STATS, 0x047A, TFT_WHITE, 5);  // STATS instead of SAVE

    // RIGHT COLUMN: FRIEND INTERACTIONS (X: 282 to 320)
    tft.fillRect(282, 0, 38, 240, COLOR_UI_PANEL);
    tft.drawFastVLine(282, 0, 240, COLOR_UI_BORDER);

    drawTouchButton(tft, 284, 2,   34, 44, TXT_TREAT,      0xD9E0, TFT_WHITE, 6);
    drawTouchButton(tft, 284, 50,  34, 44, TXT_CHAT,       0x03FF, TFT_BLACK, 7);
    drawTouchButton(tft, 284, 98,  34, 44, TXT_MAGIC_PLUS, 0x78D8, TFT_WHITE, 8);
    drawTouchButton(tft, 284, 146, 34, 44, TXT_QUEST,      0x1404, TFT_WHITE, 9);
    drawTouchButton(tft, 284, 194, 34, 44, TXT_TRADE,      COLOR_GOLD, TFT_BLACK, 10);
  }

  // =========================================================================
  // MAIN SCREEN - PARTIAL RENDER: Dynamic elements (bars, values, sprites, balloons)
  // =========================================================================
  // LEFT VIEWPORT: YOUR PET HUD & SPRITE (X: 38 to 160)
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
  uint8_t xpPct = (myPet.xp * 100) / myPet.xpNext;
  drawMiniBar(tft, 58, 54, 56, 5, xpPct, COLOR_MANA_PURPLE);

  // Center Sprite
  if (strcmp(myPet.phase, "Egg") == 0) {
    drawPixelEgg(tft, 95, 126, myBiome.petColor, myPet.eggWarmth);
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
  tft.drawString(String("S:") + engine.consecutiveLoginDays + (engine.canClaimStreakBonus() ? "!" : ""), 70, 218, 1);
  tft.setTextColor(TFT_CYAN, myBiome.bgColor);
  tft.drawString(String("EN:") + myPet.energy, 98, 218, 1);

  // RIGHT VIEWPORT: FRIEND'S PET (X: 160 to 282)
  tft.setTextColor(TFT_WHITE, frBiome.bgColor);
  tft.drawString(friendPet.name, 166, 6, 2);
  tft.drawString(String("Lv.") + friendPet.level, 244, 6, 2);

  tft.setTextColor(friendPet.isOnline ? TFT_GREEN : TFT_LIGHTGREY, frBiome.bgColor);
  tft.drawString(friendPet.isOnline ? TXT_ONLINE : TXT_ASLEEP, 166, 22, 1);

  // Friend HP
  tft.setTextColor(TFT_RED, frBiome.bgColor);
  tft.drawString("HP", 166, 34, 1);
  drawMiniBar(tft, 182, 34, 56, 7, friendPet.health, TFT_RED);

  // Friend Sprite
  drawPixelPet(tft, 220, 126, friendPet.biome, friendPet.phase, true);

  // Friend Live Speech Balloon
  if (friendPet.balloonTimeout > 0 || strlen(friendPet.speechBalloon) > 0) {
    tft.fillRoundRect(164, 154, 114, 26, 4, TFT_WHITE);
    tft.drawRoundRect(164, 154, 114, 26, 4, TFT_BLACK);
    tft.setTextColor(TFT_BLACK, TFT_WHITE);
    tft.setTextDatum(MC_DATUM);
    tft.drawString(friendPet.speechBalloon, 221, 167, 1);
  }

  // Friend Footer
  tft.setTextDatum(TL_DATUM);
  tft.setTextColor(TFT_CYAN, frBiome.bgColor);
  tft.drawString(friendPet.deviceId, 166, 218, 1);

  // Top Notification Toast
  if (strlen(activeBannerMessage) > 0) {
    tft.fillRoundRect(48, 2, 224, 20, 4, 0x18E3);
    tft.drawRoundRect(48, 2, 224, 20, 4, TFT_GOLD);
    tft.setTextColor(TFT_YELLOW, 0x18E3);
    tft.setTextDatum(MC_DATUM);
    tft.drawString(activeBannerMessage, 160, 12, 1);
  }

  // =========================================================================
  // FLYOUTS: Draw on TOP of everything (highest Z-order)
  // =========================================================================
  if (feedMenuOpen) {
    populateFeedFlyout();
    drawFeedFlyout();
  }
  if (spellMenuOpen) {
    populateSpellFlyout();
    drawSpellFlyout();
  }
}

// ============================================================================
// TOUCH EVENT DISPATCHER
// ============================================================================
void handleTouchPress(uint16_t x, uint16_t y) {
  soundClick();

  // Handle Feed Flyout touches first (overlay on left viewport)
  if (feedMenuOpen) {
    // Close button at top-right (284-318, 2-38)
    if (x >= 284 && x <= 318 && y >= 2 && y <= 38) {
      feedMenuOpen = false;
      screenNeedsRefresh = true;
      return;
    }
    // Food items area (40-280, 2-38)
    if (x >= 40 && x <= 280 && y >= 2 && y <= 38) {
      int itemWidth = 44;
      int gap = 4;
      int startX = 40;
      for (uint8_t i = 0; i < availableFoodCount; i++) {
        int itemX = startX + i * (itemWidth + gap);
        if (x >= itemX && x <= itemX + itemWidth) {
          selectedFoodIndex = i;
          FeedResult res = engine.feedWithItem(feedFlyoutItems[i].id);
          if (res.success) {
            myPet.hunger = min(100, myPet.hunger + res.hungerGain);
            if (res.satietyGained > myPet.satietyTicks) myPet.satietyTicks = res.satietyGained;
            if (res.becameSick) myPet.isSick = true;
            soundMunch();
            showBanner(res.becameSick ? "Gefressen aber krank! 🤢" : "Lecker gegessen! 🍖");
            myPet.xp += 15;
            engine.recordAction("feed", 1);
            checkLevelUp();
          }
          feedMenuOpen = false;
          screenNeedsRefresh = true;
          return;
        }
      }
    }
    // Tap outside flyout closes it
    feedMenuOpen = false;
    screenNeedsRefresh = true;
    return;
  }

  // Handle Spell Flyout touches (overlay at y=127-163)
  if (spellMenuOpen) {
    // Close button at top-right (284-318, 127-163)
    if (x >= 284 && x <= 318 && y >= 127 && y <= 163) {
      spellMenuOpen = false;
      screenNeedsRefresh = true;
      return;
    }
    // Spell items area (40-280, 127-163)
    if (x >= 40 && x <= 280 && y >= 127 && y <= 163) {
      int itemWidth = 48;
      int gap = 4;
      int startX = 40;
      for (uint8_t i = 0; i < availableSpellCount; i++) {
        int itemX = startX + i * (itemWidth + gap);
        if (x >= itemX && x <= itemX + itemWidth) {
          selectedSpellIndex = i;
          SpellData& spell = spellFlyoutItems[i];
          if (myPet.mana >= spell.manaCost) {
            // Cast spell
            myPet.mana -= spell.manaCost;
            myPet.spellsCast++;
            engine.recordAction("magic", 1);

            // Apply effects based on target
            if (spell.target == 's' || spell.target == 'b') {
              // Self effects
              myPet.health = min(100, myPet.health + spell.healthGain);
              myPet.hunger = min(100, myPet.hunger + spell.hungerGain);
              myPet.happiness = min(100, myPet.happiness + spell.happinessGain);
              myPet.energy = min(100, myPet.energy + spell.energyGain);
              if (spell.satietyTicks > myPet.satietyTicks) myPet.satietyTicks = spell.satietyTicks;
              if (spell.curesSickness) {
                myPet.isSick = false;
                myPet.hygiene = 100;
              }
            }

            soundTreatSent(); // Magic sound

            if (spell.target == 'f' || spell.target == 'b') {
              // Send to friend via socket
              sendDirectSocketPacket("MAGIC", spell.name);
              showBanner(String("Zauber: ") + spell.name + " -> Freund! ✨");
              myPet.xp += 40;
            } else {
              showBanner(String("Zauber: ") + spell.name + " gewirkt! ✨");
              myPet.xp += 30;
            }

            if (spell.target == 'b') myPet.xp += 15; // Bonus for dual target
            checkLevelUp();
          } else {
            showBanner("Nicht genug Mana! ⚡");
          }
          spellMenuOpen = false;
          screenNeedsRefresh = true;
          return;
        }
      }
    }
    // Tap outside flyout closes it
    spellMenuOpen = false;
    screenNeedsRefresh = true;
    return;
  }

  // =========================================================================
  // STATS SCREEN TOUCH HANDLING
  // =========================================================================
  if (currentScreen != SCREEN_MAIN) {
    // Back button (top-left: 2-42, 2-28)
    if (x >= 2 && x <= 42 && y >= 2 && y <= 28) {
      currentScreen = SCREEN_MAIN;
      forceFullRender = true;
      screenNeedsRefresh = true;
      return;
    }
    // Tab bar (y: 32-54, 3 tabs visible at a time)
    if (y >= 32 && y <= 54) {
      int tabWidth = 320 / 3;
      int startTab = max(0, (int)statsTabIndex - 1);
      if (startTab > 3) startTab = 3;

      for (int i = 0; i < 3 && (startTab + i) < 6; i++) {
        int tabX = i * tabWidth;
        if (x >= tabX && x <= tabX + tabWidth) {
          statsTabIndex = startTab + i;
          forceFullRender = true;
          screenNeedsRefresh = true;
          return;
        }
      }
    }
    // Content area - handle scrolling
    if (y >= 56 && y <= 240) {
      if (lastTouchY == -1) {
        // Start scroll
        lastTouchY = y;
        isScrolling = true;
      } else if (isScrolling) {
        // Scroll
        int16_t delta = lastTouchY - y;
        statsScrollY = constrain(statsScrollY + delta, 0, statsScrollMaxY);
        lastTouchY = y;
        forceFullRender = true;
        screenNeedsRefresh = true;
      }
      return;
    }
    // Touch released - end scroll
    if (isScrolling) {
      isScrolling = false;
      lastTouchY = -1;
    }
    return;
  }

  // 1. LEFT COLUMN: Your Actions (X: 0 to 38)
  if (x < 38) {
    if (y < 48) {
      // FEED button - toggle flyout
      if (feedMenuOpen) {
        feedMenuOpen = false;
      } else {
        feedMenuOpen = true;
        spellMenuOpen = false; // Close other flyout
        populateFeedFlyout();
      }
    } else if (y < 96) {
      // PLAY
      myPet.happiness = min(100, myPet.happiness + 20);
      myPet.energy = max(0, myPet.energy - 10);
      myPet.xp += 20;
      engine.recordAction("play", 1);
      showBanner(TXT_BANNER_PLAY);
    } else if (y < 144) {
      // BATH
      myPet.hygiene = 100;
      myPet.happiness = min(100, myPet.happiness + 10);
      engine.recordAction("bath", 1);
      showBanner(TXT_BANNER_BATH);
    } else if (y < 192) {
      // MAGIC button - toggle spell flyout
      if (spellMenuOpen) {
        spellMenuOpen = false;
      } else {
        spellMenuOpen = true;
        feedMenuOpen = false; // Close other flyout
        populateSpellFlyout();
      }
    } else {
      // STATS button - Open Stats/Inventory/Jobs/Magic screen (like web simulator)
      currentScreen = SCREEN_STATS;
      statsTabIndex = 0;
      feedMenuOpen = false;
      spellMenuOpen = false;
      forceFullRender = true;
    }
    checkLevelUp();
    screenNeedsRefresh = true;
    return;
  }

  // 2. RIGHT COLUMN: Friend Actions (X: 282 to 320)
  if (x > 282) {
    if (y < 48) {
      soundTreatSent();
      sendDirectSocketPacket("TREAT", "Heart Donut");
      myPet.treatsSent++;
      myPet.xp += 25;
      engine.recordAction("send_treat", 1);
      showBanner(TXT_BANNER_TREAT_SENT);
    } else if (y < 96) {
      sendDirectSocketPacket("CHAT", "Hi Friend! ^_^");
      showBanner(TXT_BANNER_CHAT_SENT);
    } else if (y < 144) {
      if (myPet.mana >= 30) {
        myPet.mana -= 30;
        sendDirectSocketPacket("MAGIC", "Friend Blessing");
        myPet.spellsCast++;
        myPet.xp += 40;
        engine.recordAction("magic", 1);
        soundTreatSent();
        showBanner(TXT_BANNER_BLESSING);
      } else {
        showBanner(TXT_BANNER_NO_MANA30);
      }
    } else if (y < 192) {
      sendDirectSocketPacket("QUEST", "Co-op Scout");
      myPet.coins += 35;
      myPet.jobsCompleted++;
      myPet.xp += 45;
      showBanner(TXT_BANNER_QUEST);
    } else {
      sendDirectSocketPacket("TRADE", "Top Hat");
      showBanner(TXT_BANNER_TRADE);
    }
    checkLevelUp();
    screenNeedsRefresh = true;
    return;
  }

  // 3. CENTRAL BRIDGE STATUS DOT (X: 150 to 170, Y: 105 to 135): Trigger OTA Check
  if (x >= 150 && x <= 170 && y >= 105 && y <= 135) {
    // Persistent banner (0 = no timeout) - will be replaced by OTA progress screen
    showBanner(TXT_BANNER_OTA_CHECK, 0);
    checkForOTAUpdate();
    screenNeedsRefresh = true;
    return;
  }

  // 4. FOOTER COINS / STREAK / MISSIONS / SHOP (X: 40 to 105, Y: 205 to 240)
  if (x >= 40 && x <= 105 && y >= 205 && y <= 240) {
    // 4.1 Claim Daily Login Streak Bonus first if available
    if (engine.canClaimStreakBonus()) {
      uint16_t cReward = engine.getStreakBonusCoins();
      uint16_t xReward = engine.getStreakBonusXP();
      engine.claimDailyLoginStreakBonus();
      myPet.coins += cReward;
      myPet.xp += xReward;
      soundLevelUp();
      showBanner(String(TXT_BANNER_STREAK_CLAIMED) + engine.consecutiveLoginDays + "! +" + cReward + "$ +" + xReward + "XP", 3500);
      checkLevelUp();
      screenNeedsRefresh = true;
      return;
    }

    // 4.2 Claim Completed Daily Missions
    bool missionClaimed = false;
    for (uint8_t m = 0; m < engine.missionCount; m++) {
      if (engine.missions[m].completed && !engine.missions[m].claimed) {
        engine.claimMission(engine.missions[m].id);
        myPet.coins += engine.missions[m].coinReward;
        myPet.xp += engine.missions[m].xpReward;
        missionClaimed = true;
        soundLevelUp();
        showBanner(TXT_BANNER_MISSION_CLAIMED, 3000);
        break;
      }
    }

    // 4.3 Fallback: Quick Shop Purchase (Star Cookie) or balance show
    if (!missionClaimed) {
      if (myPet.coins >= 15) {
        myPet.coins -= 15;
        myPet.hunger = min(100, myPet.hunger + 30);
        myPet.happiness = min(100, myPet.happiness + 15);
        myPet.xp += 10;
        soundMunch();
        showBanner(TXT_BANNER_SHOP_COOKIE, 2500);
      } else {
        showBanner(String("Shop Bazar: $") + myPet.coins + " Coins", 2000);
      }
    }
    checkLevelUp();
    screenNeedsRefresh = true;
    return;
  }

  // 5. CENTER LEFT: Tap Egg to Hatch or Petting
  if (x >= 40 && x <= 155 && y >= 70 && y <= 180) {
    if (strcmp(myPet.phase, "Egg") == 0) {
      myPet.eggWarmth += 25;
      if (myPet.eggWarmth >= 100) {
        strcpy(myPet.phase, "Baby");

        soundHatch();
        showBanner(TXT_BANNER_EGG_HATCHED, 3500);
      } else {
        showBanner(TXT_BANNER_EGG_WARMED);
      }
    } else {
      myPet.happiness = min(100, myPet.happiness + 5);
      showBanner(TXT_BANNER_CUDDLED);
    }
    screenNeedsRefresh = true;
  }
}

// ============================================================================
// LEVEL UP PROGRESSION
// ============================================================================
void checkLevelUp() {
  if (myPet.xp >= myPet.xpNext) {
    myPet.xp -= myPet.xpNext;
    myPet.level++;
    myPet.xpNext = 100 * myPet.level * 1.3;
    myPet.health = 100;
    myPet.energy = 100;

    if (myPet.level >= 15) strcpy(myPet.phase, "Elder");
    else if (myPet.level >= 8) strcpy(myPet.phase, "Adult");
    else if (myPet.level >= 4 && strcmp(myPet.phase, "Egg") != 0) strcpy(myPet.phase, "Youth");

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
        myPet.hunger = min(100, myPet.hunger + 25);
        myPet.happiness = min(100, myPet.happiness + 20);
        showBanner("Friend Sent You a Treat! 🍩", 3000);
      } else if (strcmp(type, "CHAT") == 0) {
        soundTreatReceived();
        if (text) {
          strncpy(friendPet.speechBalloon, text, sizeof(friendPet.speechBalloon) - 1);
          friendPet.balloonTimeout = millis() + 6000;
        }
      } else if (strcmp(type, "MAGIC") == 0) {
        soundTreatReceived();
        // Handle different spell types from friend
        if (text && strcmp(text, "Purify") == 0) {
          myPet.health = min(100, myPet.health + 10);
          myPet.isSick = false;
          myPet.hygiene = 100;
          showBanner("Friend cast Purify on you! 🫧", 3000);
        } else if (text && strcmp(text, "Vitality Aura") == 0) {
          myPet.health = min(100, myPet.health + 25);
          myPet.happiness = min(100, myPet.happiness + 10);
          showBanner("Friend cast Vitality Aura! 💖", 3000);
        } else if (text && strcmp(text, "Astral Feast") == 0) {
          myPet.hunger = min(100, myPet.hunger + 35);
          myPet.satietyTicks = 3;
          showBanner("Friend shared Astral Feast! 🍓", 3000);
        } else if (text && strcmp(text, "Friend Blessing") == 0) {
          myPet.health = min(100, myPet.health + 30);
          myPet.happiness = min(100, myPet.happiness + 25);
          showBanner("Received Friend Blessing! ✨", 3000);
        } else if (text && strcmp(text, "Astral Harmony") == 0) {
          myPet.health = min(100, myPet.health + 50);
          myPet.happiness = min(100, myPet.happiness + 40);
          showBanner("Astral Harmony received! 🌟", 3000);
        } else {
          // Default/legacy
          myPet.health = min(100, myPet.health + 30);
          myPet.happiness = min(100, myPet.happiness + 25);
          showBanner("Received Magic! ✨", 3000);
        }
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

// ----------------------------------------------------------------------------
// Flyout Menu Helpers
// ----------------------------------------------------------------------------
void populateFeedFlyout() {
  availableFoodCount = 0;
  uint8_t foodIndices[8];
  uint8_t count = engine.getAvailableFoods(foodIndices, 8);
  for (uint8_t i = 0; i < count && availableFoodCount < 8; i++) {
    EngineInventorySlot& slot = engine.inventory[foodIndices[i]];
    if (slot.quantity > 0) {
      FoodItem& item = feedFlyoutItems[availableFoodCount];
      strncpy(item.id, slot.id, sizeof(item.id) - 1);
      strncpy(item.name, slot.name, sizeof(item.name) - 1);
      strncpy(item.name_de, slot.name_de, sizeof(item.name_de) - 1);
      item.hungerGain = slot.hungerGain;
      item.healthGain = slot.healthGain;
      item.energyGain = slot.energyGain;
      item.happinessGain = slot.happyGain;
      item.satietyTicks = slot.satietyTicks;
      item.sickChancePct = slot.sickChancePct;
      item.cureSickness = slot.cureSickness;
      item.quantity = slot.quantity;
      // Map icons
      if (strcmp(slot.id, "food_apple") == 0) item.icon = "🍎";
      else if (strcmp(slot.id, "food_snack") == 0) item.icon = "🥨";
      else if (strcmp(slot.id, "food_herb_salad") == 0) item.icon = "🥗";
      else if (strcmp(slot.id, "food_berry_pie") == 0) item.icon = "🥧";
      else if (strcmp(slot.id, "food_sun_jerky") == 0) item.icon = "🥩";
      else if (strcmp(slot.id, "food_kelp_stew") == 0) item.icon = "🍲";
      else if (strcmp(slot.id, "food_ambrosia") == 0) item.icon = "🧁";
      else if (strcmp(slot.id, "item_salve") == 0) item.icon = "🧴";
      else item.icon = "🍴";
      availableFoodCount++;
    }
  }
}

void populateSpellFlyout() {
  availableSpellCount = 0;
  // Define all available spells (matching web simulator)
  SpellData allSpells[5] = {
    {"spl_cleanse", "Purify", "Lauterung", 15, 's', 10, 0, 0, 0, 0, true, "🫧"},
    {"spl_heal", "Vitality Aura", "Vitalitaet", 20, 's', 25, 0, 10, 0, 0, false, "💖"},
    {"spl_treat_burst", "Astral Feast", "Astral-Fest", 25, 's', 0, 35, 0, 0, 3, false, "🍓"},
    {"spl_friend_bless", "Friend Blessing", "Freundes-Segen", 30, 'f', 30, 0, 25, 0, 0, false, "✨"},
    {"spl_rain_dance", "Astral Harmony", "Astrale Harm", 45, 'b', 50, 0, 40, 0, 0, false, "🌟"}
  };

  // Only show learned spells (for now, show all if level >= minLevel)
  for (uint8_t i = 0; i < 5; i++) {
    uint8_t minLevel = 1;
    if (i == 1) minLevel = 2;
    else if (i == 2 || i == 3) minLevel = 3;
    else if (i == 4) minLevel = 5;

    if (myPet.level >= minLevel) {
      spellFlyoutItems[availableSpellCount] = allSpells[i];
      availableSpellCount++;
    }
  }
}

void drawFeedFlyout() {
  if (availableFoodCount == 0) {
    // Draw "No food - Go to Shop" button
    tft.fillRoundRect(40, 2, 240, 36, 6, 0xD9E0);
    tft.drawRoundRect(40, 2, 240, 36, 6, TFT_RED);
    tft.setTextColor(TFT_WHITE, 0xD9E0);
    tft.setTextDatum(MC_DATUM);
    tft.drawString("Kein Essen! Shop oeffnen", 160, 20, 2);
    // Close button
    tft.fillRoundRect(284, 2, 34, 36, 6, 0x4A69);
    tft.drawRoundRect(284, 2, 34, 36, 6, TFT_WHITE);
    tft.setTextColor(TFT_WHITE, 0x4A69);
    tft.drawString("X", 301, 20, 2);
    return;
  }

  // Draw food items horizontally scrollable
  int startX = 40;
  int itemWidth = 44;
  int gap = 4;

  for (uint8_t i = 0; i < availableFoodCount; i++) {
    FoodItem& item = feedFlyoutItems[i];
    int x = startX + i * (itemWidth + gap);
    if (x > 280) break; // Don't draw off-screen

    uint16_t bgColor = (item.sickChancePct > 0) ? 0xFD40 : (item.satietyTicks > 0 ? 0x2E69 : 0x18E3);
    uint16_t borderColor = (item.sickChancePct > 0) ? TFT_ORANGE : (item.satietyTicks > 0 ? TFT_GREEN : TFT_WHITE);

    tft.fillRoundRect(x, 2, itemWidth, 36, 4, bgColor);
    tft.drawRoundRect(x, 2, itemWidth, 36, 4, borderColor);

    // Icon
    tft.setTextColor(TFT_WHITE, bgColor);
    tft.setTextDatum(MC_DATUM);
    tft.drawString(item.icon, x + 8, 12, 2);

    // Quantity
    tft.setTextColor(TFT_YELLOW, bgColor);
    tft.drawString(String("x") + item.quantity, x + 22, 12, 1);

    // Hunger gain
    tft.setTextColor(TFT_ORANGE, bgColor);
    tft.drawString(String("+") + item.hungerGain, x + 8, 30, 1);

    // Satiety shield indicator
    if (item.satietyTicks > 0) {
      tft.setTextColor(TFT_GREEN, bgColor);
      tft.drawString(String("S") + item.satietyTicks, x + 22, 30, 1);
    }
    // Sickness risk indicator
    if (item.sickChancePct > 0) {
      tft.setTextColor(TFT_RED, bgColor);
      tft.drawString("!", x + 36, 30, 1);
    }

    // Highlight selected
    if (i == selectedFoodIndex) {
      tft.drawRoundRect(x - 1, 1, itemWidth + 2, 38, 5, TFT_YELLOW);
    }
  }

  // Close button on right
  tft.fillRoundRect(284, 2, 34, 36, 6, 0x4A69);
  tft.drawRoundRect(284, 2, 34, 36, 6, TFT_WHITE);
  tft.setTextColor(TFT_WHITE, 0x4A69);
  tft.setTextDatum(MC_DATUM);
  tft.drawString("X", 301, 20, 2);
}

void drawSpellFlyout() {
  if (availableSpellCount == 0) {
    // Draw "No spells learned" button
    tft.fillRoundRect(40, 127, 240, 36, 6, 0x78D8);
    tft.drawRoundRect(40, 127, 240, 36, 6, TFT_PURPLE);
    tft.setTextColor(TFT_WHITE, 0x78D8);
    tft.setTextDatum(MC_DATUM);
    tft.drawString("Keine Zauber! RPG -> Magie", 160, 145, 2);
    // Close button
    tft.fillRoundRect(284, 127, 34, 36, 6, 0x4A69);
    tft.drawRoundRect(284, 127, 34, 36, 6, TFT_WHITE);
    tft.setTextColor(TFT_WHITE, 0x4A69);
    tft.drawString("X", 301, 145, 2);
    return;
  }

  // Draw spell items horizontally scrollable
  int startX = 40;
  int itemWidth = 48;
  int gap = 4;

  for (uint8_t i = 0; i < availableSpellCount; i++) {
    SpellData& spell = spellFlyoutItems[i];
    int x = startX + i * (itemWidth + gap);
    if (x > 280) break;

    bool canAfford = myPet.mana >= spell.manaCost;
    uint16_t bgColor = canAfford ? 0x78D8 : 0x18E3;
    uint16_t borderColor = canAfford ? TFT_PURPLE : 0x4A69;
    uint16_t textColor = canAfford ? TFT_WHITE : 0x7BEF;

    tft.fillRoundRect(x, 127, itemWidth, 36, 4, bgColor);
    tft.drawRoundRect(x, 127, itemWidth, 36, 4, borderColor);

    // Icon
    tft.setTextColor(textColor, bgColor);
    tft.setTextDatum(MC_DATUM);
    tft.drawString(spell.icon, x + 8, 137, 2);

    // Target indicator
    char targetIcon = (spell.target == 'f') ? 'F' : (spell.target == 'b' ? 'B' : 'S');
    tft.setTextColor(TFT_YELLOW, bgColor);
    tft.drawString(String(targetIcon), x + 24, 137, 1);

    // Mana cost
    tft.setTextColor(TFT_CYAN, bgColor);
    tft.drawString(String(spell.manaCost) + "MP", x + 8, 155, 1);

    // Satiety shield
    if (spell.satietyTicks > 0) {
      tft.setTextColor(TFT_GREEN, bgColor);
      tft.drawString(String("S") + spell.satietyTicks, x + 24, 155, 1);
    }
    // Cure sickness
    if (spell.curesSickness) {
      tft.setTextColor(TFT_RED, bgColor);
      tft.drawString("Rx", x + 36, 155, 1);
    }

    // Highlight selected
    if (i == selectedSpellIndex) {
      tft.drawRoundRect(x - 1, 126, itemWidth + 2, 38, 5, TFT_YELLOW);
    }
  }

  // Close button on right
  tft.fillRoundRect(284, 127, 34, 36, 6, 0x4A69);
  tft.drawRoundRect(284, 127, 34, 36, 6, TFT_WHITE);
  tft.setTextColor(TFT_WHITE, 0x4A69);
  tft.setTextDatum(MC_DATUM);
  tft.drawString("X", 301, 145, 2);
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
  if (WiFi.status() != WL_CONNECTED) {
    showBanner("No WiFi!");
    return;
  }
  Serial.println("[OTA] Querying PHP Server for available firmware or rules update...");

  HTTPClient http;
  String url = cfg_server_url + "/ota_check.php?device=" + cfg_device_id + "&fw_version=" + FIRMWARE_VERSION + "&rules_version=" + RULES_VERSION;
  http.begin(url);
  http.setTimeout(10000);
  int httpCode = http.GET();

  if (httpCode == HTTP_CODE_OK) {
    String resp = http.getString();
    Serial.printf("[OTA] Response: %s\n", resp.c_str());
    StaticJsonDocument<1024> doc;
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
      } else if (fwAvailable) {
        // 2. Hardware Binary OTA (A/B Partition Switch)
        const char* fUrl = doc["fw_url"];
        size_t fSize = doc["fw_size"] | 0;
        if (fUrl) {
          String fullFwUrl = String(fUrl).startsWith("http") ? String(fUrl) : (cfg_server_url + "/" + String(fUrl));
          performFirmwareOTA(fullFwUrl, fSize);
          return; // performFirmwareOTA handles reboot on success
        }
      } else {
        // No updates available
        showBanner("Kein Update verfuegbar", 3000);
        Serial.println("[OTA] No updates available");
      }
    } else {
      Serial.printf("[OTA] JSON parse error: %s\n", err.c_str());
      showBanner("OTA: JSON Error!", 3000);
    }
  } else {
    Serial.printf("[OTA] HTTP error: %d\n", httpCode);
    showBanner("OTA Server Error: " + String(httpCode), 3000);
  }
  http.end();
}

