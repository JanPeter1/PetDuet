# PetDuet - Dual-Device Tamagotchi Game Specification

**Vision:** A competitive, community-driven Tamagotchi experience where players interact with their pet and friends' pets across two ESP32 displays (Cheap Yellow Display / ESP32-2432S028R), using PHP-based server infrastructure for device discovery, WebSocket real-time communication, cloud persistence, and seamless Over-The-Air (OTA) updates for both firmware binaries and game rules.

---

## Table of Contents
1. [Core Features & Tamagotchi Care Mechanics](#core-features--tamagotchi-care-mechanics)
2. [RPG Systems & Progression](#rpg-systems--progression)
   - [Professions, Cooking & Magic](#1-leveling--xp-scaling-formula)
   - [Daily Missions & Login Streak System](#6-daily-missions--login-streak-system)
   - [Shop & Marketplace System](#7-shop--marketplace-system-bazar)
3. [Game Rules Engine (`rules.json`)](#game-rules-engine-rulesjson)
4. [Technical Architecture & Network Protocols](#technical-architecture--network-protocols)
5. [Server Specifications](#server-specifications)
6. [ESP32 Firmware Specifications](#esp32-firmware-specifications)
7. [Display Screens & UI Layout (2.8" ILI9341)](#display-screens--ui-layout-28-ili9341)
8. [Over-the-Air (OTA) A/B Firmware & Dynamic Rules System](#over-the-air-ota-ab-firmware--dynamic-rules-system)
9. [Project Implementation Plan](#project-implementation-plan)
10. [Bilingual Support & Resolved Design Questions](#bilingual-support--resolved-design-questions)

---

## Core Features & Tamagotchi Care Mechanics

### 1. Social Pet Interaction
- Two ESP32 devices run persistent Tamagotchi RPGs with real-time server-mediated communication via WebSocket.
- Friend interactions include:
  - **Quick Chat:** Pre-set and contextual message packets.
  - **Treat Friend's Pet:** Transmits meals/treats across the network with LCD animations.
  - **Co-op Magic:** Simultaneous dual-cast blessings and stat shields.
  - **Item & Accessory Trading:** Server-escrowed peer swaps.
- RPG progression: XP gained from self-care, co-op quests, and shared interactions.
- Complex mechanics unlock dynamically as pets level up.

### 2. Pet Status, Care Mistake (CM) Engine & Lifecycle

#### 2.1 Core Need Metrics & Tick Decay
Every tick (default: 60 seconds), meters decay based on lifecycle phase, biome, and seasonal modifiers:

| Metric | Range | Default | Tick Decay | Behavioral Consequence |
|---|---|---|---|---|
| **Food (Hunger)** | 0 – 100 | 80 | -0.25 / tick | At 0, triggers **Attention Call (Beep)**. 15 min window before Care Mistake. At 0, HP drains by -1.5 / tick. |
| **Health (HP)** | 0 – 100 | 100 | 0.0 (drain on starve/sick) | Drops during untreated illness or starvation. At 0, enters critical state. |
| **Happiness (Mood)** | 0 – 100 | 75 | -0.20 / tick | At 0, triggers Attention Call. Reduces job success rate by 50% and halts evolution. |
| **Energy (Stamina)** | 0 – 100 | 100 | -0.15 / tick | At <20, pet refuses jobs/training. Restores at +1.2 / tick during sleep. |
| **Hygiene (Cleanliness)** | 0 – 100 | 100 | Drops on waste event | Waste occurs every 2–4 hours. If uncleaned for >20 min, causes sickness (skull icon). |
| **Mana (Magic Pool)** | 0 – 100 | 50 | 0.0 (+0.5 / tick natural regen) | Fuel for solo utility spells and co-op friend blessings. |

#### 2.2 Meals vs. Snacks (The Cavity Rule)
- **Meals (Cooked Dishes / Basic Grain):** Fills Hunger (+25 to +70), adds +1g weight, safe to feed until 100% full.
- **Snacks / Treats (Sweets / Honey):** Boosts Happiness (+20 to +30), adds +2g weight.
- **Cavity / Sickness Penalty:** Feeding more than 3 consecutive snacks without play/exercise causes a cavity / illness event requiring medicine treatment.

#### 2.3 The Care Mistake (CM) Engine
- When Food = 0, Happiness = 0, or pet is sick, an **Attention Call** icon lights up accompanied by audio beeps.
- **15-Minute Response Window:** If the player attends to the pet within 15 minutes, the icon clears with **0 mistakes**.
- **Failure:** If 15 minutes elapse without care, the icon shuts off and **+1 Care Mistake** is logged.
- **Sleep Care:** Leaving room lights on for >60 minutes after bedtime logs a mental care mistake.

#### 2.4 Lifecycle Progression Windows
1. **Egg (Phase 0):** Duration: 5 minutes (or 50 warm touch taps). Hatching triggered by player care.
2. **Baby (Phase 1):** Duration: 2 hours. Fragile, 2.5x stat decay, cannot work jobs, sleeps frequently.
3. **Youth / Child (Phase 2):** Duration: 24–48 hours (Min. Level 3). Unlocks basic jobs, starter recipes, and discipline.
4. **Adult (Phase 3):** Duration: 7+ days (Min. Level 8). Evolves into branching forms based on Care Mistakes (0–1 CM = Paragon Form; 2–4 CM = Balanced Form; 5+ CM = Wild/Scrappy Form).
5. **Old Age / Senior (Phase 4):** Min. Level 20. Stat decay slows down; teaches skills; bequeaths rare heirlooms to next generation egg.

### 3. Biome System
Pets choose a native habitat at setup that dynamically impacts stats, appearance, and resource gathering:

| Biome | Color Theme | Native Trait | Environmental Perk | Vulnerability / Need |
|---|---|---|---|---|
| **Desert** | `#FFD700` (Yellow) | *Sun-Forged* | +25% Mining yields; heat-immune | Food decays 15% faster; dislikes rain |
| **Grassland** | `#7CFC00` (Green) | *Verdant Spirit* | +20% Herb yields; eggs hatch 25% faster | Needs frequent play; happiness decays faster if idle |
| **Water** | `#1E90FF` (Blue) | *Aquatic Affinity* | +20 Max Mana; +30% River Pearl yields | Hygiene decays faster outside water; needs clean pools |
| **Forest** | `#8B4513` (Brown) | *Ancient Wisdom* | +25% Mushroom & Wood yield; +15% recipe XP | Bedtime starts earlier (19:30); requires high darkness |

### 4. Calendar-Week Seasonal Cycle (NTP Berlin Time)
Synchronized to ISO calendar weeks via NTP ($W = \text{ISO\_Week} \pmod 4$):
1. **Spring (Week 1):** Sprout theme. Egg warmth requirement reduced by 30%. Herb yields doubled.
2. **Summer (Week 2):** Heatwave theme. Energy decays 20% faster. Desert pets gain +20% stats.
3. **Autumn (Week 3):** Harvest theme. Mushroom yields quadrupled. Cooking gives +30% bonus XP.
4. **Winter (Week 4):** Frost/Snow theme. Food foraging drops reduced by 30% (scarcity). Staying warm requires touch snuggles. Rare Ice Crystals can be mined.

---

## RPG Systems & Progression

### 1. Leveling & XP Scaling Formula
To provide quick early milestones and sustained multi-week progression:

$$\text{RequiredXP}(\text{Level}) = 100 \times (\text{Level} - 1)^{1.8} + 50 \times (\text{Level} - 1)$$

* **Level 1 $\rightarrow$ 2:** 100 XP (~Day 1: 2 meals + 1 mini-game + 1 friend treat)
* **Level 5 (Youth):** ~750 XP (unlocks Tier 2 recipes, starter spells)
* **Level 8 (Adult):** ~1,680 XP (unlocks Adult evolution, full profession tree)
* **Level 20 (Senior):** ~7,800 XP (unlocks Master Crafting and Grand Co-op Magic)

### 2. Professions & Jobs
Pets can work jobs to earn currency ($ / Coins), raw ingredients, and XP:
- **Forager:** Gathers herbs, wood, and mushrooms (Energy Cost: 20, Duration: 60s).
- **Miner:** Mines sunstones, iron, and crystals (Energy Cost: 35, Duration: 180s).
- **Pearl Diver:** Dives for freshwater pearls and sea glass (Energy Cost: 30, Duration: 120s).

### 3. Cooking & Recipe Synthesis
Combining raw materials creates cooked meals with stat gains and temporary buffs:
- **Meadow Herb Salad:** 2x Herbs $\rightarrow$ +25 Food, +10 Energy, +20 XP.
- **Desert Jerky:** 2x Cactus, 1x Herb $\rightarrow$ +40 Food, +15 HP, +30 XP, "Stamina Shield".
- **Hearty River Stew:** 1x Pearl, 2x Herbs $\rightarrow$ +70 Food, +30 HP, +15 Mana, +65 XP.
- **Celestial Ambrosia:** 1x Sunstone, 1x Pearl, 2x Mushrooms $\rightarrow$ +100 Food, +50 HP, +50 Energy, +50 Mana, +120 XP.

### 4. Magic & Spellcraft System
- **Purify (Solo):** Instantly cleans waste and cures sickness (Cost: 15 MP, Level 2+).
- **Snooze Charm (Solo):** Restores energy 2x faster during sleep (Cost: 20 MP, Level 3+).
- **Friend Blessing (Co-op via WebSocket):** Sends healing wave to friend (+35 HP, +25 Happy, Cost: 30 MP, Level 3+).
- **Dual Astral Harmony (Dual-Cast):** Both devices press MGC within 10 seconds $\rightarrow$ Synchronized screen-filling aurora, +100 XP, rare accessory gift.

### 5. Server-Mediated Trading Protocol
1. **Initiation:** Player selects item to trade and selects requested item from friend catalog.
2. **Escrow:** Trade offer sent to server (`POST /api/sync`). Item is temporarily locked.
3. **Peer Prompt:** Friend device displays alert: *"Trade Offer from [Name]: [Item]! Accept?"*.
4. **Atomic Swap:** Upon acceptance, server swaps database ownership and broadcasts updated inventories to both devices over WebSocket.

### 6. Daily Missions & Login Streak System
The core engine features an automated daily engagement and reward loop:
- **Consecutive Login Streaks:** Evaluated daily on midnight boundary (Europe/Berlin time). Daily consecutive logins increment the streak ($1, 2, 3\dots$ days); missing a full calendar day resets streak to 1.
- **Dynamic Daily Objectives:** A rotating set of daily care tasks granting bonus Coins ($) and XP:
  - **Nourish Pet:** Feed pet 3 times (+40 Coins, +50 XP).
  - **Loyal Companion:** Maintain a 2-day login streak (+60 Coins, +80 XP).
  - **Playtime Fun:** Play 2 outdoor mini-games (+30 Coins, +40 XP).
  - **Bubble Bath:** Clean pet and keep hygiene at 100% (+25 Coins, +35 XP).
  - **Share the Love:** Transmit a snack or magic blessing to partner (+50 Coins, +60 XP).
- **Claim Mechanism:** Once an objective reaches its target count, the status flips to `Ready to Claim`. Players tap `Claim Reward` to cash in bonuses with victory audio fanfare.

### 7. Shop & Marketplace System (Bazar)
Pets spend their hard-earned Coins ($) accumulated from daily jobs, missions, and co-op adventures at the in-game Shop. The marketplace offers three dedicated aisles:

#### 7.1 Purchasable Items, Consumables & Gear
Physical goods stored in the pet's backpack (`inventory`). Equippable accessories provide passive stat and XP buffs:

| Item ID | Name (EN) | Name (DE) | Category | Cost ($) | Min. Level | Icon | Effects & Gameplay Benefits |
|---|---|---|---|---|---|---|---|
| `item_apple` | Fresh Apple | Frischer Apfel | Food | 5 | 1 | 🍎 | +15 Food, +5 Happiness. Basic snack. |
| `item_herb` | Meadow Herb Bundle | Wiesenkräuter-Bündel | Material | 8 | 1 | 🌿 | Basic synthesis herb for cooking & salves. |
| `item_salve` | Healing Salve | Wohltuende Heilsalbe | Potion | 25 | 2 | 🧴 | +40 Health (HP), immediately cures sickness/poison. |
| `item_energy_elixir` | Energy Tonic | Vitalitäts-Tonikum | Potion | 35 | 3 | 🧪 | +60 Energy, +15 Mana. Instantly cancels exhaustion. |
| `item_sunstone` | Sun Salt Crystals | Sonnensalz-Kristalle | Material | 45 | 3 | 🧂 | Desert mineral ingredient for high-tier cooking. |
| `item_blossom_crown` | Blossom Crown | Blütenkrone | Accessory (Head) | 75 | 2 | 👑 | +10 Happiness, +5% bonus XP on all activities. |
| `item_wizard_cowl` | Starry Wizard Cowl | Sternen-Zauberkappe | Accessory (Head) | 110 | 4 | 🧙 | +15 Max Mana, -10% spell mana cost. |
| `item_tophat` | Fancy Top Hat | Vornehmer Zylinder | Accessory (Head) | 140 | 5 | 🎩 | +15 Happiness, +15% Coin payout on jobs. |
| `item_evo_prism` | Astral Evolution Prism | Astrales Evolutionsprisma | Evolution | 250 | 8 | 🔮 | Radiant catalyst allowing voluntary early evolution into Adult/Elder forms with gleaming aura. |

#### 7.2 Purchasable Recipes & Cooking Blueprints
Buying a recipe permanently unlocks it in the pet's cookbook, permitting the pet to cook the dish anytime the required raw materials are collected:

| Recipe ID | Name (EN) | Name (DE) | Blueprint Cost ($) | Min. Level | Icon | Required Ingredients | Dish Effects & Buffs |
|---|---|---|---|---|---|---|---|
| `rcp_salad` | Meadow Herb Salad | Wiesenkräuter-Salat | 15 | 1 | 🥗 | 2x `mat_herb` | +25 Food, +10 Energy, +20 XP |
| `rcp_jerky` | Sun-Dried Jerky | Wüsten-Dörrfleisch | 35 | 2 | 🥩 | 2x `mat_meat`, 1x `mat_sunstone` | +45 Food, +20 Energy, +15 Happiness, +60 XP |
| `rcp_berry_pie` | Forest Berry Tart | Waldbeeren-Tarte | 65 | 3 | 🥧 | 1x `mat_herb`, 1x `mat_mushroom` | +35 Food, +25 Happiness, +15 Health, +80 XP |
| `rcp_stew` | Hearty River Stew | Herzhafter Flusseintopf | 110 | 4 | 🍲 | 2x `mat_mushroom`, 1x `mat_pearl` | +70 Food, +30 Health, +15 Mana, +110 XP |
| `rcp_ambrosia` | Celestial Ambrosia | Himmlische Ambrosia | 240 | 6 | ✨ | 1x `mat_sunstone`, 1x `mat_pearl`, 2x `mat_mushroom`, 2x `mat_herb` | +100 Food, +50 HP, +50 Energy, +50 Mana, +250 XP |

#### 7.3 Purchasable Magic Spells & Grimoires
Spell tomes inscribe ancient incantations into the pet's active spellbook once the level requirement is satisfied:

| Spell ID | Name (EN) | Name (DE) | Tome Cost ($) | Min. Level | Mana Cost | Target | Icon | Spell Effects & Network Action |
|---|---|---|---|---|---|---|---|---|
| `spl_cleanse` | Purify | Läuterung | 40 | 2 | 15 MP | Self | 🫧 | Instantly clears pet droppings, cleanses sickness, +20 XP. |
| `spl_heal` | Vitality Aura | Vitalitäts-Aura | 70 | 2 | 20 MP | Self | 💖 | Restores +25 Health (HP), +10 Happiness, +35 XP. |
| `spl_friend_blessing` | Friend Blessing | Freundes-Segen | 95 | 3 | 30 MP | Friend (Socket) | ✨ | Beams healing wave to partner device (+35 HP, +25 Happiness, +45 XP). |
| `spl_treat_burst` | Astral Feast | Astral-Festmahl | 130 | 4 | 25 MP | Self | 🍓 | Conjures ethereal mana fruit restoring +35 Food & +15 Energy. |
| `spl_astral_harmony` | Dual Astral Harmony | Astrale Harmonie | 260 | 6 | 45 MP | Both (Dual-Cast) | 🌟 | Screen-filling synchronized aurora on both CYD units (+50 HP, +40 Happy, +100 XP). |

#### 7.4 Shop Transaction & Validation Logic
1. **Level Check:** The shop validates that $\text{Pet.Level} \ge \text{ShopItem.MinLevel}$. Lower-level items remain locked with a lock badge (`🔒 Lv.X required`).
2. **Affordability Check:** Validates that $\text{Pet.Coins} \ge \text{ShopItem.Cost}$. If insufficient, purchase is disabled.
3. **Execution & Inventory Routing:**
   - **Items:** Deducts coins, adds quantity to backpack (`Pet.inventory`).
   - **Recipes:** Deducts coins, flags recipe as `unlocked: true` in pet state.
   - **Spells:** Deducts coins, registers spell ID into `Pet.knownSpells`.
4. **State Persistence & Relay:** Emits transaction confirmation to local storage/NVS flash and syncs wallet changes to PHP SQLite server.

---

## Game Rules Engine (`rules.json`)

All game mechanics, decay rates, formulas, recipes, jobs, and bilingual text strings are defined in a single human-readable `rules.json` parsed dynamically by the ESP32 (C++/ArduinoJson), the PHP Server (`json_decode`), and the Web Simulator:

```json
{
  "version": "1.0.0",
  "engine": "PetDuet-Unified-Rules",
  "meta": {
    "title": "PetDuet Core Game Rules",
    "supported_languages": ["en", "de"],
    "tick_interval_seconds": 60,
    "ntp_server": "pool.ntp.org",
    "timezone": "Europe/Berlin"
  },
  "stats": {
    "food": { "min": 0, "max": 100, "default": 80, "decay_per_tick": 0.25 },
    "health": { "min": 0, "max": 100, "default": 100, "decay_per_tick": 0.0, "starve_decay": 1.5 },
    "happiness": { "min": 0, "max": 100, "default": 75, "decay_per_tick": 0.20 },
    "energy": { "min": 0, "max": 100, "default": 100, "decay_per_tick": 0.15, "sleep_regen_per_tick": 1.2 },
    "mana": { "min": 0, "max": 100, "default": 50, "decay_per_tick": 0.0, "regen_per_tick": 0.5 }
  },
  "lifecycle": {
    "phases": [
      { "id": "egg", "name": "Egg", "name_de": "Ei", "duration_minutes": 5, "warmth_required": 50 },
      { "id": "baby", "name": "Baby", "name_de": "Baby", "duration_minutes": 120, "decay_multiplier": 2.5 },
      { "id": "youth", "name": "Youth", "name_de": "Jungtier", "min_level": 3, "duration_minutes": 2880 },
      { "id": "adult", "name": "Adult", "name_de": "Erwachsen", "min_level": 8, "duration_minutes": 10080 },
      { "id": "elder", "name": "Elder", "name_de": "Senior", "min_level": 20, "duration_minutes": -1 }
    ],
    "care_mistake_threshold_seconds": 900
  },
  "biomes": {
    "desert": {
      "name": "Desert", "name_de": "Wüste", "color": "#FFD700",
      "modifiers": { "food_decay": 1.15, "energy_decay": 1.20, "mining_yield": 1.25 }
    },
    "grassland": {
      "name": "Grassland", "name_de": "Grasland", "color": "#7CFC00",
      "modifiers": { "happiness_decay": 0.80, "egg_hatch_speed": 1.25, "herb_yield": 1.20 }
    },
    "water": {
      "name": "Water", "name_de": "Wasser", "color": "#1E90FF",
      "modifiers": { "max_mana": 20, "hygiene_decay": 1.25, "pearl_yield": 1.30 }
    },
    "forest": {
      "name": "Forest", "name_de": "Wald", "color": "#8B4513",
      "modifiers": { "recipe_xp_bonus": 1.15, "mushroom_yield": 1.25, "bedtime_hour": 19 }
    }
  },
  "recipes": [
    {
      "id": "rcp_salad",
      "name": "Meadow Herb Salad", "name_de": "Wiesenkräuter-Salat",
      "min_level": 1,
      "ingredients": { "mat_herb": 2 },
      "effects": { "food": 25, "energy": 10, "xp": 20 },
      "icon": "🥗"
    },
    {
      "id": "rcp_jerky",
      "name": "Desert Jerky", "name_de": "Wüsten-Dörrfleisch",
      "min_level": 2,
      "ingredients": { "mat_cactus": 2 },
      "effects": { "food": 35, "health": 10, "xp": 30 },
      "icon": "🥩"
    },
    {
      "id": "rcp_stew",
      "name": "Hearty River Stew", "name_de": "Herzhafter Flusseintopf",
      "min_level": 4,
      "ingredients": { "mat_pearl": 1, "mat_herb": 2 },
      "effects": { "food": 70, "health": 30, "mana": 15, "xp": 65 },
      "icon": "🍲"
    }
  ],
  "jobs": [
    {
      "id": "job_forage",
      "name": "Biome Foraging", "name_de": "Kräuter Sammeln",
      "min_level": 1, "energy_cost": 20, "duration_seconds": 60,
      "rewards": { "xp": 25, "coins": 15, "materials": { "mat_herb": 2 } },
      "icon": "🌿"
    },
    {
      "id": "job_mine",
      "name": "Crystal Mining", "name_de": "Kristall-Bergbau",
      "min_level": 3, "energy_cost": 35, "duration_seconds": 180,
      "rewards": { "xp": 50, "coins": 35, "materials": { "mat_sunstone": 1 } },
      "icon": "⛏️"
    }
  ],
  "spells": [
    {
      "id": "spl_cleanse",
      "name": "Purify", "name_de": "Läuterung",
      "min_level": 2, "mana_cost": 15, "target": "self",
      "effects": { "clean_poop": true, "cure_sickness": true, "xp": 20 },
      "icon": "🫧"
    },
    {
      "id": "spl_friend_blessing",
      "name": "Friend Blessing", "name_de": "Freundes-Segen",
      "min_level": 3, "mana_cost": 30, "target": "friend",
      "effects": { "health": 35, "happiness": 25, "xp": 45 },
      "icon": "✨"
    }
  ],
  "daily_missions": [
    {
      "id": "mission_feed",
      "action_type": "feed", "target_count": 3,
      "name": "Nourish Pet", "name_de": "Pet Füttern",
      "rewards": { "coins": 40, "xp": 50 }, "icon": "🍖"
    },
    {
      "id": "mission_login_streak",
      "action_type": "login_streak", "target_count": 2,
      "name": "Loyal Companion", "name_de": "Treuer Begleiter",
      "rewards": { "coins": 60, "xp": 80 }, "icon": "📅"
    }
  ],
  "shop": {
    "items": [
      { "id": "item_apple", "name": "Fresh Apple", "name_de": "Frischer Apfel", "price_coins": 5, "min_level": 1, "category": "food", "icon": "🍎" },
      { "id": "item_salve", "name": "Healing Salve", "name_de": "Wohltuende Heilsalbe", "price_coins": 25, "min_level": 2, "category": "potion", "icon": "🧴" },
      { "id": "item_blossom_crown", "name": "Blossom Crown", "name_de": "Blütenkrone", "price_coins": 75, "min_level": 2, "category": "accessory", "icon": "👑" },
      { "id": "item_tophat", "name": "Fancy Top Hat", "name_de": "Vornehmer Zylinder", "price_coins": 140, "min_level": 5, "category": "accessory", "icon": "🎩" }
    ],
    "recipes": [
      { "id": "rcp_salad", "name": "Meadow Herb Salad", "name_de": "Wiesenkräuter-Salat", "price_coins": 15, "min_level": 1, "icon": "🥗" },
      { "id": "rcp_jerky", "name": "Desert Jerky", "name_de": "Wüsten-Dörrfleisch", "price_coins": 35, "min_level": 2, "icon": "🥩" },
      { "id": "rcp_stew", "name": "Hearty River Stew", "name_de": "Herzhafter Flusseintopf", "price_coins": 110, "min_level": 4, "icon": "🍲" }
    ],
    "spells": [
      { "id": "spl_cleanse", "name": "Purify", "name_de": "Läuterung", "price_coins": 40, "min_level": 2, "mana_cost": 15, "target": "self", "icon": "🫧" },
      { "id": "spl_friend_blessing", "name": "Friend Blessing", "name_de": "Freundes-Segen", "price_coins": 95, "min_level": 3, "mana_cost": 30, "target": "friend", "icon": "✨" }
    ]
  }
}
```

---

## Technical Architecture & Network Protocols

### Communication Flow
```
ESP32 CYD 1 (Device A) <────[WebSocket]────> PHP Server Relay <────[WebSocket]────> ESP32 CYD 2 (Device B)
         │                                          │                                       │
         ▼                                          ▼                                       ▼
  Local SPIFFS/NVS                          SQLite Database                         Local SPIFFS/NVS
  (Offline Rules/Save)                    (https://familie-riegel.de)             (Offline Rules/Save)
```

**Key Decision:** All peer-to-peer interactions are routed through the PHP server via lightweight WebSocket frames. This avoids NAT traversal and firewall issues on local Wi-Fi while guaranteeing persistent delivery.

### WebSocket Message Schemas

#### 1. Peer Treat Event
```json
{
  "type": "TREAT_SEND",
  "from_device": "CYD_DEV1",
  "to_device": "CYD_DEV2",
  "payload": { "food_id": "rcp_salad", "food_boost": 25, "xp_reward": 15 }
}
```

#### 2. Co-op Magic Event
```json
{
  "type": "MAGIC_CAST",
  "from_device": "CYD_DEV1",
  "to_device": "CYD_DEV2",
  "payload": { "spell_id": "spl_friend_blessing", "hp_gain": 35, "happy_gain": 25 }
}
```

#### 3. Trade Proposal Event
```json
{
  "type": "TRADE_PROPOSE",
  "trade_id": "tr_1731055200",
  "from_device": "CYD_DEV1",
  "to_device": "CYD_DEV2",
  "payload": { "offered_item": "hat_wizard", "requested_item": "gem_sunstone" }
}
```

---

## Server Specifications

### Technology Stack
- **Backend:** PHP 8.x
- **Database:** SQLite 3 (`petduet.db`, maximum size < 1GB)
- **Real-Time Communication:** Ratchet PHP WebSocket server or native PHP socket server running as systemd service
- **Production Server Host:** `https://familie-riegel.de/petduet/` (SFTP: `ftp.familie-riegel.de:2222`)

### Core API Endpoints
| Endpoint | Method | Purpose |
|---|---|---|
| `/api/register-device` | POST | Register device ID, MAC address, token, trainer name, biome |
| `/api/login` | POST | Authenticate client and return session token |
| `/api/pet/get-pet` | GET | Retrieve complete pet state, inventory, and stats from SQLite |
| `/api/pet/update` | POST | Sync pet state changes (stats, XP, level, evolution) |
| `/api/sync` | POST | Trigger full 2-way state sync and process pending trade escrow |
| `/api/rules` | GET | Download latest `rules.json` to ESP32 SPIFFS |
| `/api/leaderboard` | GET | Fetch global leaderboards (Level, XP, Friends Treated) |
| `/api/ota/check` | GET | Query server for available firmware binary or rule file updates |
| `/api/ota/firmware/:file` | GET | Stream compiled ESP32 firmware binary for A/B OTA slot flashing |
| `/api/ota/report` | POST | Client reports upgrade success or automatic rollback event |

---

## ESP32 Firmware Specifications

### Hardware Platform (`cyd_config.h`)
- **Board:** ESP32-2432S028R "Cheap Yellow Display" (CYD)
- **MCU:** ESP32 Dual-Core Xtensa LX6 (240MHz, 520KB SRAM, 4MB SPI Flash)
- **Display Driver:** ILI9341 320x240 LCD (SPI2_HOST: MOSI 13, MISO 12, CLK 14, CS 15, DC 2, BL 21)
- **Touch Driver:** XPT2046 Resistive Touch (SPI3_HOST: MOSI 32, MISO 39, CLK 25, CS 33, IRQ 36)
- **Audio:** Onboard Piezo Buzzer / Speaker on GPIO 26
- **Status LED:** Onboard RGB LED (Active-Low: Red 4, Green 16, Blue 17)
- **Boot/AP Button:** GPIO 0

### Software Architecture
- **Framework:** ESP-IDF 5.x / 6.1 or Arduino-ESP32
- **Graphics Engine:** LVGL (Light and Versatile Graphics Library) or TFT_eSPI
- **Rule Parser:** ArduinoJson / cJSON reading `rules.json` from SPIFFS flash
- **Time Sync:** SNTP client syncing to `pool.ntp.org` with timezone `Europe/Berlin`
- **Wi-Fi Provisioning:** SoftAP portal fallback (`192.168.4.1`) when network is unreachable
- **OTA Architecture:** Dual-slot A/B system (`ota_0` and `ota_1`) with hardware rollback protection

---

## Display Screens & UI Layout (2.8" ILI9341 - 320x240 LCD)

### Dual-Pet Co-op Stage Architecture
A central architectural pillar of PetDuet is that **both pets are visible simultaneously on screen at all times**:
- **Your Own Pet** lives on the **left side of the arena** (X: 38 to 159).
- **Your Partner's Pet** lives on the **right side of the arena, directly to the right of your own pet** (X: 161 to 282).
- The two pets are joined by a central hardware wireless synchronization bridge (X: 160) showing live ping, socket heartbeat, and co-op packet transfers.
- **Leftmost Touch Column (X: 0 to 36):** Personal care actions for your pet (Feed, Play, Bath, RPG, Cloud Save).
- **Rightmost Touch Column (X: 284 to 320):** Social interaction actions targeting the partner pet (Treat, Chat, Magic+, Co-op Quest, Trade).

### Screen Diagram: 320 x 240 Pixels
```
+---------------------------------------------------------------------------------------+
| [TOP BAR: Special Actions & Status]                                                   |
|  Lv.8 Adult  [🌿 Jobs]  [✨ Magic]  [🍳 Cook]  [📶 Online]  Partner: CYD_DEV2 (Online)|
+---------------------------------------------------------------------------------------+
| [LEFT ACTIONS] | [LEFT STAGE: YOUR PET] | [SYNC] | [RIGHT STAGE: PARTNER PET] | [RIGHT]  |
|                |                        |   │    |                            |          |
| [ 🍖 FEED  ]   |     (\__/)             |   ●    |         (\__/)             | [ 🍩 TREAT]|
| [ ⚽ PLAY  ]   |     (•ㅅ•)  [Forest]   |   │    |   💬    (>.< )  [Desert]   | [ 💬 CHAT ]|
| [ 🫧 BATH  ]   |    / >❤️< \  Lv.8      |  WiFi  |  "Hi!" / >⭐< \  Lv.7      | [ ✨ MGC+ ]|
| [ 🎒 RPG   ]   |   HP:100%  FOOD:85%    |  Sync  |       HP:95%  FOOD:75%     | [ 🤝 TRADE]|
+---------------------------------------------------------------------------------------+
| [BOTTOM HUD: Status & Currency]                                                       |
|  YOU: 🪙 45  ❤️ 90%  ⚡ 90%   │  PARTNER: 🪙 60  ❤️ 85%  ⚡ 80%   │ Season: Spring 🌸   |
+---------------------------------------------------------------------------------------+
```

#### Detailed Zone Allocations:
1. **Left Touch Column (Width: 36px, X: 0..36):**
   - 5 vertically stacked touch buttons (44px height each): `FEED`, `PLAY`, `BATH`, `RPG`, `SAVE`.
2. **Left Stage - Own Pet (Width: 122px, X: 38..159):**
   - Own pet name, level badge, and biome indicator.
   - Real-time HP and Hunger progress bars + full XP progress meter.
   - Interactive animated 16-bit pixel-art sprite of your pet (tap to pet, warm egg, play).
   - Bottom status line: Coins ($), Happiness %, Energy %.
3. **Center Divider & Hardware Sync Bridge (Width: 8px, X: 160):**
   - Direct socket heartbeat LED and packet transfer animations.
   - Shows connection status: `LAN` (Direct Socket) or `NET` (PHP Server Relay).
4. **Right Stage - Partner Pet (Width: 122px, X: 161..282):**
   - Positioned **directly to the right of your own pet**.
   - Displays partner pet's name, level badge, online dot (`● Online` / `○ Asleep`), and partner's biome theme.
   - Real-time partner HP and Hunger bars (updated via WebSocket / UDP socket packets).
   - Live speech bubble displaying incoming quick chat messages or treat alerts.
   - Partner's animated pixel-art sprite showing real-time reactions (happy when treated, sleeping if offline).
5. **Right Touch Column (Width: 36px, X: 284..320):**
   - 5 vertically stacked touch buttons (44px height each): `TREAT`, `CHAT`, `MGC+`, `QUEST`, `TRADE`.

---

## Over-the-Air (OTA) A/B Firmware & Dynamic Rules System

To ensure that both ESP32 Cheap Yellow Displays can be maintained and updated seamlessly without requiring physical USB cable disconnections, PetDuet incorporates a dual-tier over-the-air update architecture:
1. **Binary Firmware OTA:** Hardware-enforced A/B dual partition switching with automatic rollback on boot failure.
2. **Dynamic Rule File OTA (`rules.json`):** Hot-reloaded configuration updates distributed via the PHP server without interrupting gameplay or requiring a firmware re-flash.

```
                              ┌──────────────────────────────────────────────┐
                              │  PHP Server (https://familie-riegel.de)      │
                              │  - /api/ota/check                            │
                              │  - /api/ota/firmware/petduet_vX.Y.bin        │
                              │  - /api/rules (rules.json)                   │
                              └──────────────────────┬───────────────────────┘
                                                     │ HTTPS Download
                         ┌───────────────────────────┴───────────────────────────┐
                         ▼                                                       ▼
            [FIRMWARE BINARY UPDATE]                                [DYNAMIC RULES UPDATE]
                         │                                                       │
                         ▼                                                       ▼
            Writes to Inactive A/B Slot                             Streams to SPIFFS (/rules.json)
          (Slot A: ota_0 <-> Slot B: ota_1)                                      │
                         │                                                       ▼
                         ▼                                          Engine hot-reloads in RAM
             SHA256 Checksum Validation                             (No reboot needed; 0 downtime)
                         │
                         ▼
        Bootloader Switches Active Boot Slot
                         │
                         ▼
              ESP32 Self-Reset into New Slot
                         │
        ┌────────────────┴────────────────┐
        ▼                                 ▼
[Self-Test Passes]               [Crash / Boot Loop]
esp_ota_mark_app_valid()         Hardware Rollback Triggered
Boot confirmed stable!           Bootloader reverts to previous slot!
```

### 1. 4MB Flash Partition Scheme (`partitions.csv`)
The 4MB (32Mbit) SPI Flash of the ESP32-2432S028R is partitioned to support dual-bank A/B firmware images alongside a dedicated SPIFFS filesystem for rules and assets:

```csv
# ESP32-2432S028R A/B OTA Partition Table
# Name,     Type, SubType, Offset,   Size,     Flags
nvs,        data, nvs,     0x9000,   0x5000,
otadata,    data, ota,     0xe000,   0x2000,
app0,       app,  ota_0,   0x10000,  0x180000, # 1.5MB (Slot A - Primary Firmware)
app1,       app,  ota_1,   0x190000, 0x180000, # 1.5MB (Slot B - Secondary Firmware)
spiffs,     data, spiffs,  0x310000, 0xE0000,  # ~896KB (rules.json, graphics, cache)
```

- **`otadata` (8KB):** Stores state registers indicating which partition (`ota_0` or `ota_1`) the second-stage bootloader should launch.
- **`app0` (1.5MB) & `app1` (1.5MB):** Mirrored app slots. The running app executes from one slot while incoming updates are written directly to the other.
- **`spiffs` (896KB):** Houses the active `rules.json`, cached partner avatars, and persistent offline game configurations.

### 2. A/B Firmware Update Workflow & Anti-Bricking Rollback
1. **Query Update Manifest:** On boot (and every 6 hours during Wi-Fi connection), the ESP32 queries `GET /api/ota/check?version=CURRENT_VER&device=DEVICE_ID`.
2. **Download to Inactive Slot:**
   - The device detects the running partition (`esp_ota_get_running_partition()`) and selects the target update partition (`esp_ota_get_next_update_partition()`).
   - The `.bin` file is streamed via HTTPS in 4KB chunks and written via `esp_ota_write()`.
3. **Integrity Verification:**
   - SHA256 checksum transmitted in the server manifest is validated against the written flash image.
   - `esp_ota_end()` checks image magic bytes (`0xE9`) and header integrity.
4. **Boot Slot Switch & Reset:**
   - `esp_ota_set_boot_partition(next_partition)` instructs `otadata` to boot the new slot on next power cycle.
   - The display notifies the user: `"Update complete! Rebooting..."` and triggers `esp_restart()`.
5. **Anti-Bricking Health Check & Rollback:**
   - Upon first boot of the new partition, the state is flagged as `ESP_OTA_IMG_PENDING_VERIFY`.
   - The firmware executes a diagnostic suite:
     - LCD & Touch controller initialization (`ILI9341` & `XPT2046`).
     - SPIFFS mount and NVS read.
     - Successful Wi-Fi connection and server handshake.
   - **Success:** Firmware calls `esp_ota_mark_app_valid_cancel_rollback()`. The update is permanently committed.
   - **Failure / Crash:** If a panic, watchdog reset, or failure occurs before confirmation, the ESP32 hardware bootloader marks the slot invalid and automatically rolls back to the previous stable partition!

### 3. Dynamic Rule-File Updates (`rules.json`)
Game balance adjustments, new cooking recipes, seasonal jobs, and spell balance patches do not require binary firmware reflashing:
1. **Manifest Check:** `GET /api/ota/check` compares the local SHA256 hash of `/spiffs/rules.json` with the server's published hash.
2. **Download & Atomic Replacement:**
   - The new JSON file is downloaded to a temporary file `/spiffs/rules_new.json`.
   - Once validated by `ArduinoJson`, the old file is replaced via `SPIFFS.rename()`.
3. **In-Memory Hot-Reload:**
   - The core C++ rule engine re-parses the JSON file into memory.
   - Stat decay rates, recipe tables, and UI text strings are refreshed immediately without resetting the ESP32 or interrupting the pet's active state.

### 4. Server-Side Manifest Specification (`/api/ota/check`)
The PHP server at `https://familie-riegel.de/petduet/` exposes a lightweight endpoint returning the update manifest:

```json
{
  "status": "success",
  "device_id": "CYD_DEV1",
  "firmware": {
    "update_available": true,
    "target_version": "1.2.0",
    "min_required_version": "1.0.0",
    "url": "https://familie-riegel.de/petduet/firmware/petduet_cyd_v1.2.0.bin",
    "size_bytes": 1310720,
    "sha256": "7d9b4b0a48d88e63286b51c8b39a3f2a1a8c3d98ef5c07c6f05a5a73e614138e",
    "release_notes_de": "Neues A/B OTA-System, verbesserte Touch-Kalibrierung & Koop-Zauber.",
    "release_notes_en": "New A/B OTA system, improved touch calibration & co-op spells."
  },
  "rules": {
    "update_available": true,
    "target_version": "1.0.4",
    "hash": "c3ab8ff13720e8ad9047dd39466b3c8974e592c2fa383d4a3960714caef0c4f2",
    "url": "https://familie-riegel.de/petduet/rules.json",
    "hot_reload": true
  }
}
```

### 5. On-Screen Display Feedback during OTA Updates
During binary OTA flashing, the 2.8" LCD displays a dedicated update screen to inform the player:

```
+------------------------------------------------------------------+
|                    ⚡ FIRMWARE UPDATE (OTA)                       |
+------------------------------------------------------------------+
|                                                                  |
|   Target Version: v1.2.0            Target Slot: Slot B (ota_1)  |
|                                                                  |
|   Downloading & Flashing Firmware:                               |
|   [████████████████████░░░░░░░░░░] 68%                           |
|   890 KB / 1310 KB (Speed: 52 KB/s)                              |
|                                                                  |
|   ⚠️  DO NOT DISCONNECT POWER! / GERÄT NICHT AUSSCHALTEN!         |
|   (Automatic rollback active in case of interruption)            |
|                                                                  |
+------------------------------------------------------------------+
| Status: Validating SHA256 Block Checksums...                     |
+------------------------------------------------------------------+
```

- **RGB Status LED:** Pulses amber during download and flashing; turns solid green upon verification; chimes victory tone before reboot.

---

## Project Implementation Plan

### Phase 1: Foundations & Rule Engine (Weeks 1–2)
1. Finalize `rules.json` schema and validate parsing with unit tests.
2. Integrate rule loader in C++ (ESP32) and PHP backend.
3. Test tick decay loop and Care Mistake countdown timer.

### Phase 2: Server Infrastructure & OTA Distribution (Weeks 3–4)
4. Set up SQLite schema on `https://familie-riegel.de/petduet/`.
5. Deploy REST API endpoints for device registration, state save, and leaderboards.
6. Build PHP OTA update manifest handler (`/api/ota/check`) and binary file streaming pipeline.
7. Deploy WebSocket relay server for bidirectional message routing.

### Phase 3: ESP32 Firmware, A/B OTA & UI (Weeks 5–7)
8. Configure display driver and touch matrix using `cyd_config.h`.
9. Flash A/B dual partition scheme (`partitions.csv`) onto ESP32-2432S028R boards.
10. Implement `esp_https_ota` client with rollback verification and progress callback.
11. Build dynamic SPIFFS `rules.json` hot-reloader.
12. Build LVGL screen layout (Top Bar, Action Columns, Status HUD, Dual-Pet Stage).
13. Integrate Wi-Fi manager and SoftAP captive portal.

### Phase 4: Integration, Testing & Deployment (Weeks 8–10)
14. Test dual-device socket communication across 2 physical CYD units.
15. Verify NTP time synchronization and weekly seasonal triggers.
16. Simulate OTA power loss during flash to test automatic bootloader rollback to previous slot.
17. Validate dynamic rule hot-reload across WebSocket without reboot.
18. Package firmware release binaries and deployment guide for production hosting.

---

## Bilingual Support & Resolved Design Questions

### 1. Bilingual Support (DE / EN)
- All rule entries in `rules.json` define both `name` and `name_de`, as well as `desc` and `desc_de`.
- The user can toggle between English and German in the startup setup screen or settings menu.
- Display buttons adapt dynamically:
  - EN: `FEED`, `PLAY`, `BATH`, `RPG`, `TREAT`, `CHAT`, `MGC+`, `TRADE`
  - DE: `FÜTTERN`, `SPIELEN`, `BADEN`, `RPG`, `LECKERLI`, `CHAT`, `MAGIE+`, `TAUSCH`

### 2. Resolved Design Questions
1. **Rule Format Specification:** Unified JSON specification defined in Section 3 (`rules.json`).
2. **Magic System:** Level-gated spell table with explicit MP costs, cooldowns, and XP yields (Section 2.4).
3. **Trading Protocol:** 4-step UI proposal $\rightarrow$ Server Escrow $\rightarrow$ Partner Prompt $\rightarrow$ Atomic Swap (Section 2.5).
4. **Seasonal Progression:** Driven deterministically by real-world ISO calendar week modulo 4 via Berlin NTP time.
5. **Peer Discovery:** Mediated through server registration and WebSocket room matching; no brittle local Wi-Fi broadcasting.
6. **Over-The-Air (OTA) Updates & Partitioning:**
   - Firmware binaries update over HTTPS into dual A/B partitions (`ota_0` and `ota_1`, 1.5MB each) with automatic bootloader rollback protection.
   - Game rules (`rules.json`) update dynamically into SPIFFS with in-memory hot-reloading (0 downtime, no reboot required).
   - Server distributes version manifests, binaries, and rule files via `https://familie-riegel.de/petduet/api/ota/check`.

---
*Updated on 2026-11-08 with complete Over-The-Air (OTA) A/B Firmware & Dynamic Rules System specifications.*
