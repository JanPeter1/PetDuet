/**
 * ============================================================================
 * Internationalization (i18n) - English (EN) & Deutsch (DE)
 * ============================================================================
 * Provides bilingual dictionaries for:
 *  - CYD 2.8" LCD screen buttons and status bars
 *  - RPG Workshop (Recipes, Jobs, Magic, Inventory)
 *  - Trading Post, Quick Chat & Leaderboard
 *  - WiFi SoftAP Web Portal
 *  - Navigation & Top Bar
 */

export type Language = 'en' | 'de';

export const translations = {
  en: {
    // Top Bar & Navigation
    appTitle: 'CYD Tamagotchi RPG',
    navDualSim: 'Dual CYD Simulator',
    navRpgWorkshop: 'RPG Workshop',
    navFirmware: 'ESP32 C++ Firmware',
    navPhpServer: 'PHP Server Scripts',
    navMechanicsCfg: 'Mechanics (.cfg)',
    btnLeaderboard: '🏆 Leaderboard',
    btnWifiAp: '📶 WiFi AP Setup',
    btnLanguage: 'Language',

    // CYD LCD 2.8" Buttons (Left Column - Your Pet)
    btnFeed: 'FEED',
    btnPlay: 'PLAY',
    btnBath: 'BATH',
    btnRpg: 'RPG',
    btnSave: 'SAVE',

    // CYD LCD 2.8" Buttons (Right Column - Friend Pet)
    btnTreat: 'TREAT',
    btnChat: 'CHAT',
    btnMagicPlus: 'MGC+',
    btnQuest: 'QUEST',
    btnTrade: 'TRADE',

    // Status & HUD
    statHealth: 'HP',
    statHunger: 'FD',
    statEnergy: 'EN',
    statHygiene: 'CLN',
    statHappiness: 'HAPPY',
    statMana: 'MP',
    statXp: 'XP',
    statusOnline: 'ONLINE',
    statusAsleep: 'ASLEEP',
    tapToHatch: 'TAP TO HATCH!',
    eggWarmthLabel: 'WARMTH',

    // Biomes
    biomeDesert: 'Desert',
    biomeGrassland: 'Grassland',
    biomeWater: 'Water',
    biomeForest: 'Forest',

    // Seasons
    seasonSpring: 'Spring',
    seasonSummer: 'Summer',
    seasonAutumn: 'Autumn',
    seasonWinter: 'Winter',
    seasonActiveLabel: 'Season Active',

    // Evolution Phases
    phaseEgg: 'Egg',
    phaseBaby: 'Baby',
    phaseYouth: 'Youth',
    phaseAdult: 'Adult',
    phaseElder: 'Elder',

    // Notifications & Messages
    notifFed: 'Fed delicious meal! +25 FD',
    notifPlayed: 'Outdoor game! +20 Happy',
    notifBath: 'Sparkling bubble bath! 🫧',
    notifEggWarm: 'Egg warmed! Keep tapping 🥚',
    notifEggHatched: 'EGG HATCHED! Baby Born! 🐣',
    notifCuddled: 'Pet cuddled! ❤️',
    notifSaved: 'Saved to Cloud & SD! 💾',
    notifLevelUp: 'LEVEL UP! Pet Evolved! 🌟',
    notifTreatSent: 'Treat transmitted over Socket! 🍩',
    notifTreatReceived: 'Friend sent you a treat! 🍩 +25 FD',
    notifMagicSent: 'Casting Co-op Magic Blessing! ✨',
    notifMagicReceived: 'Received Friend Blessing! 🌟',
    notifQuestComplete: 'Co-op Quest Complete! +$35 ⚔️',
    notifTradeSent: 'Trade proposal sent! 🎁',
    notifTradeAccepted: 'Trade completed! 🤝',
    notifTradeDeclined: 'Trade was declined.',
    notifRebooting: 'ESP32 Rebooting...',

    // Quick Chat Presets
    chatPreset1: 'Hello friend! 👋',
    chatPreset2: 'Let\'s co-op quest! ⚔️',
    chatPreset3: 'Sending hearty treats! 🍖',
    chatPreset4: 'Cast magic spell! ✨',
    chatPreset5: 'Nice hat you have! 🎩',
    chatPreset6: 'My pet is starving! 🥺',
    chatPreset7: 'Let\'s trade gear! 🎁',
    chatPreset8: 'Awesome CYD setup! 🚀',

    // Dual View Bar
    dev1Label: 'CYD #1 · LOCAL DEVICE',
    dev2Label: 'CYD #2 · FRIEND\'S DEVICE',
    directSocketTitle: 'Direct Socket',
    directSocketDesc: 'WiFi IEEE 802.11 b/g/n · Sync: ~15ms',
    seasonalCycleTitle: 'Seasonal Cycle:',
    dev1BiomeTitle: 'Dev 1 Biome:',
    dev2BiomeTitle: 'Dev 2 Biome:',

    // RPG Workshop Modal
    rpgTitle: 'CYD RPG WORKSHOP',
    tabRecipes: 'Recipes',
    tabJobs: 'Jobs & Quests',
    tabMagic: 'Magic Spells',
    tabGear: 'Gear & Bag',
    tabMissions: 'Daily Missions',
    missionsTitle: 'Daily Quests & Login Streaks',
    missionsDesc: 'Complete daily activities to earn bonus coins and experience!',
    loginStreak: 'Login Streak',
    daysStreak: 'days in a row',
    dailyLoginStreak: 'Daily Login Streak',
    streakBonusTitle: 'Daily Login Streak Bonus',
    streakBonusDesc: 'Log in consecutive days to earn escalating coin rewards & XP!',
    btnClaimStreakBonus: 'Claim Streak Bonus',
    streakClaimedToday: 'Claimed Today',
    streakNextTomorrow: 'Next bonus unlocks tomorrow',
    streakDayMilestone: 'Day',
    streakMilestoneMax: '7-Day Mega Bonus!',
    streakBonusCoins: 'Extra Coins',
    btnClaimReward: 'Claim Reward',


    missionCompleted: 'Ready to Claim',
    missionClaimed: 'Claimed',
    missionProgress: 'Progress',
    btnCook: 'Cook Dish 🍳',
    btnStartJob: 'Start Job 🚀',
    btnCastSpell: 'Cast Spell ✨',
    btnEquip: 'Equip',
    equippedAccessories: 'Equipped Accessories',
    bagItems: 'Bag Items & Ingredients',

    // Trading Post Modal
    tradingTitle: 'CYD PEER TRADING POST',
    pendingTradeFrom: 'Pending Trade Request from',
    btnAcceptTrade: 'Accept 🤝',
    btnDeclineTrade: 'Decline',
    proposeTradeTitle: 'Propose New Trade to',
    labelYouOffer: '1. You Offer (From Your Bag):',
    labelYouRequest: '2. You Request in Return:',
    btnSendTrade: 'Send Trade Proposal via Socket 🚀',
    collectiblesCatalog: 'Rare Collectibles Catalog (Accessories & Decor)',

    // Leaderboard Modal
    leaderboardTitle: 'GLOBAL CYD LEADERBOARD',
    leaderboardSubtitle: '🏆 Community Top Trainers',
    leaderboardDesc: 'Ranked by Pet Level, EXP and shared community co-op activities.',
    btnRefreshScores: '🔄 Refresh Scores',
    colRank: 'Rank',
    colPetTrainer: 'Pet & Trainer',
    colBiome: 'Biome',
    colPhase: 'Phase',
    colLevel: 'Level',
    colXp: 'XP',
    colTreatsSent: 'Treats Sent',

    // WiFi AP Portal Modal
    wifiPortalTitle: 'ESP32 SoftAP Web Configuration',
    wifiPortalDesc: 'Configure your CYD device without touching or recompiling code. Settings are saved permanently into ESP32 NVS flash memory.',
    labelScannedNetworks: '1. Scanned 2.4GHz WiFi Networks:',
    labelWifiPass: '2. WiFi Password:',
    labelDeviceId: '3. Your CYD Device ID (e.g. CYD-DEV1):',
    labelTrainerName: '4. Trainer Name:',
    labelFriendId: '5. Friend\'s CYD Device ID (e.g. CYD-DEV2):',
    labelStartupBiome: '6. Startup Biome:',
    labelServerUrl: '7. PHP Server Base URL:',
    btnSaveReboot: 'Save to NVS Flash & Reboot CYD 🚀',
  },

  de: {
    // Top Bar & Navigation
    appTitle: 'CYD Tamagotchi RPG',
    navDualSim: 'Dual-CYD Simulator',
    navRpgWorkshop: 'RPG Werkstatt',
    navFirmware: 'ESP32 C++ Firmware',
    navPhpServer: 'PHP Server Skripte',
    navMechanicsCfg: 'Spielmechanik (.cfg)',
    btnLeaderboard: '🏆 Bestenliste',
    btnWifiAp: '📶 WiFi AP Konfig',
    btnLanguage: 'Sprache',

    // CYD LCD 2.8" Buttons (Left Column - Dein Haustier)
    btnFeed: 'FÜTTERN',
    btnPlay: 'SPIEL',
    btnBath: 'BAD',
    btnRpg: 'RPG',
    btnSave: 'SPEICH',

    // CYD LCD 2.8" Buttons (Right Column - Haustier des Freundes)
    btnTreat: 'SNACK',
    btnChat: 'CHAT',
    btnMagicPlus: 'MGC+',
    btnQuest: 'QUEST',
    btnTrade: 'TAUSCH',

    // Status & HUD
    statHealth: 'LP',
    statHunger: 'FD',
    statEnergy: 'EN',
    statHygiene: 'SAUB',
    statHappiness: 'GLÜCK',
    statMana: 'MP',
    statXp: 'XP',
    statusOnline: 'ONLINE',
    statusAsleep: 'SCHLÄFT',
    tapToHatch: 'TIPPEN ZUM BRÜTEN!',
    eggWarmthLabel: 'WÄRME',

    // Biome
    biomeDesert: 'Wüste',
    biomeGrassland: 'Grasland',
    biomeWater: 'Wasser',
    biomeForest: 'Wald',

    // Saisons
    seasonSpring: 'Frühling',
    seasonSummer: 'Sommer',
    seasonAutumn: 'Herbst',
    seasonWinter: 'Winter',
    seasonActiveLabel: 'Saison Aktiv',

    // Evolution Phases
    phaseEgg: 'Ei',
    phaseBaby: 'Baby',
    phaseYouth: 'Jugend',
    phaseAdult: 'Erwachsen',
    phaseElder: 'Weiser',

    // Notifications & Messages
    notifFed: 'Leckere Mahlzeit gefüttert! +25 FD',
    notifPlayed: 'Draußen gespielt! +20 Glück',
    notifBath: 'Herrliches Schaumbad! 🫧',
    notifEggWarm: 'Ei gewärmt! Weiter tippen 🥚',
    notifEggHatched: 'EI GESCHLÜPFT! Baby geboren! 🐣',
    notifCuddled: 'Haustier gestreichelt! ❤️',
    notifSaved: 'Auf Cloud & SD-Karte gespeichert! 💾',
    notifLevelUp: 'LEVEL UP! Haustier entwickelt! 🌟',
    notifTreatSent: 'Leckerli über Socket übertragen! 🍩',
    notifTreatReceived: 'Freund hat dir einen Snack geschickt! 🍩 +25 FD',
    notifMagicSent: 'Koop-Zaubersegen gewirkt! ✨',
    notifMagicReceived: 'Freundessegen empfangen! 🌟',
    notifQuestComplete: 'Koop-Quest abgeschlossen! +$35 ⚔️',
    notifTradeSent: 'Tauschangebot gesendet! 🎁',
    notifTradeAccepted: 'Tausch erfolgreich abgeschlossen! 🤝',
    notifTradeDeclined: 'Tauschangebot wurde abgelehnt.',
    notifRebooting: 'ESP32 startet neu...',

    // Quick Chat Presets (Deutsch)
    chatPreset1: 'Hallo mein Freund! 👋',
    chatPreset2: 'Lass uns eine Quest machen! ⚔️',
    chatPreset3: 'Schicke dir leckere Snacks! 🍖',
    chatPreset4: 'Wirke einen Zauberspruch! ✨',
    chatPreset5: 'Toller Hut auf deinem Pet! 🎩',
    chatPreset6: 'Mein Haustier verhungert! 🥺',
    chatPreset7: 'Lass uns Zubehör tauschen! 🎁',
    chatPreset8: 'Super CYD Hardware-Setup! 🚀',

    // Dual View Bar
    dev1Label: 'CYD #1 · LOKALES GERÄT',
    dev2Label: 'CYD #2 · GERÄT DES FREUNDES',
    directSocketTitle: 'Direkte Socket-Verbindung',
    directSocketDesc: 'WLAN IEEE 802.11 b/g/n · Sync: ~15ms',
    seasonalCycleTitle: 'Jahreszeiten-Zyklus:',
    dev1BiomeTitle: 'Gerät 1 Biom:',
    dev2BiomeTitle: 'Gerät 2 Biom:',

    // RPG Workshop Modal
    rpgTitle: 'CYD RPG WERKSTATT',
    tabRecipes: 'Rezepte',
    tabJobs: 'Berufe & Quests',
    tabMagic: 'Zaubersprüche',
    tabGear: 'Ausrüstung & Tasche',
    tabMissions: 'Tagesmissionen',
    missionsTitle: 'Tagesquests & Login-Serien',
    missionsDesc: 'Schließe tägliche Aktivitäten ab, um Bonus-Münzen und Erfahrung zu verdienen!',
    loginStreak: 'Login-Serie',
    daysStreak: 'Tage in Folge',
    dailyLoginStreak: 'Täglicher Login-Streak',
    streakBonusTitle: 'Täglicher Login-Streak Bonus',
    streakBonusDesc: 'Logge dich an aufeinanderfolgenden Tagen ein für steigende Münz-Boni & XP!',
    btnClaimStreakBonus: 'Streak-Bonus abholen',
    streakClaimedToday: 'Heute abgeholt',
    streakNextTomorrow: 'Nächster Bonus schaltet morgen frei',
    streakDayMilestone: 'Tag',
    streakMilestoneMax: '7-Tage Mega-Bonus!',
    streakBonusCoins: 'Extra-Münzen',
    btnClaimReward: 'Belohnung abholen',


    missionCompleted: 'Bereit zum Abholen',
    missionClaimed: 'Eingelöst',
    missionProgress: 'Fortschritt',
    btnCook: 'Gericht kochen 🍳',
    btnStartJob: 'Beruf starten 🚀',
    btnCastSpell: 'Zauber wirken ✨',
    btnEquip: 'Ausrüsten',
    equippedAccessories: 'Angelegtes Zubehör',
    bagItems: 'Inventar & Kochzutaten',

    // Trading Post Modal
    tradingTitle: 'CYD PEER-TAUSCHBÖRSE',
    pendingTradeFrom: 'Offene Tauschanfrage von',
    btnAcceptTrade: 'Annehmen 🤝',
    btnDeclineTrade: 'Ablehnen',
    proposeTradeTitle: 'Neuen Tausch vorschlagen an',
    labelYouOffer: '1. Dein Angebot (aus deiner Tasche):',
    labelYouRequest: '2. Deine Gegenforderung:',
    btnSendTrade: 'Tauschangebot über Socket senden 🚀',
    collectiblesCatalog: 'Katalog seltener Sammlerstücke (Zubehör & Deko)',

    // Leaderboard Modal
    leaderboardTitle: 'GLOBALE CYD BESTENLISTE',
    leaderboardSubtitle: '🏆 Top-Trainer der Community',
    leaderboardDesc: 'Sortiert nach Haustier-Level, Erfahrung und Koop-Aktivitäten.',
    btnRefreshScores: '🔄 Punktzahlen aktualisieren',
    colRank: 'Rang',
    colPetTrainer: 'Pet & Trainer',
    colBiome: 'Biom',
    colPhase: 'Phase',
    colLevel: 'Level',
    colXp: 'XP',
    colTreatsSent: 'Snacks gesendet',

    // WiFi AP Portal Modal
    wifiPortalTitle: 'ESP32 SoftAP Web-Konfiguration',
    wifiPortalDesc: 'Konfiguriere dein CYD Display drahtlos ohne Quellcode-Änderung. Die Einstellungen werden im NVS-Flash dauerhaft gespeichert.',
    labelScannedNetworks: '1. Gefundene 2,4 GHz WLAN-Netzwerke:',
    labelWifiPass: '2. WLAN-Passwort:',
    labelDeviceId: '3. Deine CYD Geräte-ID (z. B. CYD-DEV1):',
    labelTrainerName: '4. Trainer-Name:',
    labelFriendId: '5. Geräte-ID deines Freundes (z. B. CYD-DEV2):',
    labelStartupBiome: '6. Start-Biom:',
    labelServerUrl: '7. PHP-Server Basis-URL:',
    btnSaveReboot: 'Im NVS-Flash speichern & CYD neu starten 🚀',
  },
};
