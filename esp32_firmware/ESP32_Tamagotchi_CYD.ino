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
  #define TXT_TREAT      "SNACK"
  #define TXT_CHAT       "CHAT"
  #define TXT_MAGIC_PLUS "MGC+"
  #define TXT_QUEST      "QUEST"
  #define TXT_TRADE      "TAUSCH"
  #define TXT_ONLINE     "ONLINE"
  #define TXT_ASLEEP     "SCHLAFT"
  #define TXT_TAP_EGG    "TIPPEN!"
#else
  #define TXT_FEED       "FEED"
  #define TXT_PLAY       "PLAY"
  #define TXT_BATH       "BATH"
  #define TXT_MAGIC      "MGC"
  #define TXT_SAVE       "SAVE"
  #define TXT_TREAT      "TREAT"
  #define TXT_CHAT       "CHAT"
  #define TXT_MAGIC_PLUS "MGC+"
  #define TXT_QUEST      "QUEST"
  #define TXT_TRADE      "TRADE"
  #define TXT_ONLINE     "ONLINE"
  #define TXT_ASLEEP     "ASLEEP"
  #define TXT_TAP_EGG    "TAP EGG!"
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

// Timing State
unsigned long lastTickTime = 0;
unsigned long lastServerSyncTime = 0;
unsigned long lastOtaCheckTime = 0;
bool screenNeedsRefresh = true;
char activeBannerMessage[32] = "";
unsigned long bannerTimeout = 0;


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

// ----------------------------------------------------------------------------
// Forward Declarations
// ----------------------------------------------------------------------------
void renderCYDScreen();
void handleTouchPress(uint16_t x, uint16_t y);
void pollDirectSockets();
void sendDirectSocketPacket(const char* type, const char* textPayload = "");
void syncWithPHPServer();
void checkLevelUp();
void loadSettingsFromNVS();
void saveSettingsToNVS();
void startAPConfigPortal();
void renderAPPortalScreen();

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

  // 2. Initialize TFT Display (ILI9341)
  pinMode(CYD_TFT_BL, OUTPUT);
  digitalWrite(CYD_TFT_BL, HIGH); // Backlight ON
  tft.init();
  tft.setRotation(1); // 320x240 Landscape
  tft.fillScreen(TFT_BLACK);

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
  touch.setRotation(1);

  // 4. Load persistent settings from NVS
  loadSettingsFromNVS();

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
  } else {
    // If WiFi credentials are unconfigured or fail to connect, launch AP Portal automatically!
    Serial.println("\nWiFi connection failed! Starting AP Portal for easy setup...");
    startAPConfigPortal();
    return;
  }

  // 6. Register with PHP Server, check OTA, & draw initial LCD screen
  syncWithPHPServer();
  checkForOTAUpdate();
  renderCYDScreen();
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


  // 4. Pet Decay Loop (Every 6 seconds)
  if (now - lastTickTime > PET_TICK_INTERVAL_MS) {
    if (strcmp(myPet.phase, "Egg") != 0) {
      if (myPet.hunger > 0) myPet.hunger--;
      if (myPet.happiness > 0) myPet.happiness--;
      if (myPet.hunger < 20 && myPet.health > 0) myPet.health--;
      if (myPet.mana < 100) myPet.mana = min(100, myPet.mana + 2);
    }
    screenNeedsRefresh = true;
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
    screenNeedsRefresh = true;
  }

  // 6. Refresh Display if dirty
  if (screenNeedsRefresh) {
    renderCYDScreen();
    screenNeedsRefresh = false;
  }

  delay(15);
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
  // Connection Status Dot
  tft.fillCircle(160, 120, 5, friendPet.isOnline ? TFT_GREEN : TFT_RED);
  tft.drawCircle(160, 120, 5, TFT_WHITE);

  // =========================================================================
  // LEFT COLUMN: YOUR PET CONTROLS (X: 0 to 38)
  // =========================================================================
  tft.fillRect(0, 0, 38, 240, COLOR_UI_PANEL);
  tft.drawFastVLine(37, 0, 240, COLOR_UI_BORDER);

  drawTouchButton(tft, 2, 2,   34, 44, TXT_FEED,  0x1404, TFT_WHITE);
  drawTouchButton(tft, 2, 50,  34, 44, TXT_PLAY,  0x02F5, TFT_WHITE);
  drawTouchButton(tft, 2, 98,  34, 44, TXT_BATH,  0x047A, TFT_WHITE);
  drawTouchButton(tft, 2, 146, 34, 44, TXT_MAGIC, 0x78D8, TFT_WHITE);
  drawTouchButton(tft, 2, 194, 34, 44, TXT_SAVE,  0x9300, TFT_WHITE);

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

  // Footer: Coins & Energy
  tft.setTextDatum(TL_DATUM);
  tft.setTextColor(COLOR_GOLD, myBiome.bgColor);
  tft.drawString(String("$") + myPet.coins, 42, 218, 1);
  tft.setTextColor(TFT_CYAN, myBiome.bgColor);
  tft.drawString(String("EN:") + myPet.energy, 92, 218, 1);

  // =========================================================================
  // RIGHT VIEWPORT: FRIEND'S PET (X: 160 to 282)
  // =========================================================================
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

  // =========================================================================
  // RIGHT COLUMN: FRIEND INTERACTIONS (X: 282 to 320)
  // =========================================================================
  tft.fillRect(282, 0, 38, 240, COLOR_UI_PANEL);
  tft.drawFastVLine(282, 0, 240, COLOR_UI_BORDER);

  drawTouchButton(tft, 284, 2,   34, 44, TXT_TREAT,      0xD9E0, TFT_WHITE);
  drawTouchButton(tft, 284, 50,  34, 44, TXT_CHAT,       0x03FF, TFT_BLACK);
  drawTouchButton(tft, 284, 98,  34, 44, TXT_MAGIC_PLUS, 0x78D8, TFT_WHITE);
  drawTouchButton(tft, 284, 146, 34, 44, TXT_QUEST,      0x1404, TFT_WHITE);
  drawTouchButton(tft, 284, 194, 34, 44, TXT_TRADE,      COLOR_GOLD, TFT_BLACK);

  // Top Notification Toast
  if (strlen(activeBannerMessage) > 0) {
    tft.fillRoundRect(48, 2, 224, 20, 4, 0x18E3);
    tft.drawRoundRect(48, 2, 224, 20, 4, TFT_GOLD);
    tft.setTextColor(TFT_YELLOW, 0x18E3);
    tft.setTextDatum(MC_DATUM);
    tft.drawString(activeBannerMessage, 160, 12, 1);
  }
}

// ============================================================================
// TOUCH EVENT DISPATCHER
// ============================================================================
void handleTouchPress(uint16_t x, uint16_t y) {
  soundClick();

  // 1. LEFT COLUMN: Your Actions (X: 0 to 38)
  if (x < 38) {
    if (y < 48) {
      soundMunch();
      myPet.hunger = min(100, myPet.hunger + 25);
      myPet.xp += 15;
      engine.recordAction("feed", 1);
      showBanner("Fed Meal! +25 FD");
    } else if (y < 96) {
      myPet.happiness = min(100, myPet.happiness + 20);
      myPet.energy = max(0, myPet.energy - 10);
      myPet.xp += 20;
      engine.recordAction("play", 1);
      showBanner("Played Minigame! +20 Happy");
    } else if (y < 144) {
      myPet.hygiene = 100;
      myPet.happiness = min(100, myPet.happiness + 10);
      engine.recordAction("bath", 1);
      showBanner("Clean Bubble Bath! 🫧");
    } else if (y < 192) {
      if (myPet.mana >= 20) {
        myPet.mana -= 20;
        myPet.health = min(100, myPet.health + 30);
        myPet.xp += 30;
        myPet.spellsCast++;
        engine.recordAction("magic", 1);
        soundTreatSent();
        showBanner("Vitality Aura Cast! ✨");
      } else {
        showBanner("Need 20 Mana! ⚡");
      }
    } else {
      // Long press SAVE or tap: Save to Cloud & trigger OTA check
      syncWithPHPServer();
      checkForOTAUpdate();
      showBanner("Cloud Saved & OTA Checked! 💾");
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
      showBanner("Treat Sent to Friend! 🍩");
    } else if (y < 96) {
      sendDirectSocketPacket("CHAT", "Hi Friend! ^_^");
      showBanner("Chat Beamed over Socket! 💬");
    } else if (y < 144) {
      if (myPet.mana >= 30) {
        myPet.mana -= 30;
        sendDirectSocketPacket("MAGIC", "Friend Blessing");
        myPet.spellsCast++;
        myPet.xp += 40;
        engine.recordAction("magic", 1);
        soundTreatSent();
        showBanner("Blessing Beamed to Friend! 🌟");
      } else {
        showBanner("Need 30 Mana! ⚡");
      }
    } else if (y < 192) {
      sendDirectSocketPacket("QUEST", "Co-op Scout");
      myPet.coins += 35;
      myPet.jobsCompleted++;
      myPet.xp += 45;
      showBanner("Co-op Quest Complete! +$35 ⚔️");
    } else {
      sendDirectSocketPacket("TRADE", "Top Hat");
      showBanner("Trade Proposal Sent! 🎁");
    }
    checkLevelUp();
    screenNeedsRefresh = true;
    return;
  }

  // 3. CENTRAL BRIDGE STATUS DOT (X: 150 to 170, Y: 105 to 135): Trigger OTA Check
  if (x >= 150 && x <= 170 && y >= 105 && y <= 135) {
    showBanner("Checking Server for OTA... ⚡");
    checkForOTAUpdate();
    screenNeedsRefresh = true;
    return;
  }

  // 4. FOOTER COINS / SHOP & MISSIONS TOUCH (X: 40 to 105, Y: 205 to 240)
  if (x >= 40 && x <= 105 && y >= 205 && y <= 240) {
    bool missionClaimed = false;
    for (uint8_t m = 0; m < engine.missionCount; m++) {
      if (engine.missions[m].completed && !engine.missions[m].claimed) {
        engine.claimMission(engine.missions[m].id);
        myPet.coins += engine.missions[m].coinReward;
        myPet.xp += engine.missions[m].xpReward;
        missionClaimed = true;
        soundLevelUp();
        showBanner("Mission Claimed! +Coins & XP 🎁", 3000);
        break;
      }
    }
    if (!missionClaimed) {
      // Quick Shop Purchase: Star Cookie (15 coins)
      if (myPet.coins >= 15) {
        myPet.coins -= 15;
        myPet.hunger = min(100, myPet.hunger + 30);
        myPet.happiness = min(100, myPet.happiness + 15);
        myPet.xp += 10;
        soundMunch();
        showBanner("Shop: Star Cookie Bought! 🍪", 2500);
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
        showBanner("EGG HATCHED! Baby Born! 🐣", 3500);
      } else {
        showBanner("Egg Warmed! Keep Tapping 🥚");
      }
    } else {
      myPet.happiness = min(100, myPet.happiness + 5);
      showBanner("Pet Cuddled! ❤️");
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
        myPet.health = min(100, myPet.health + 30);
        myPet.happiness = min(100, myPet.happiness + 25);
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

