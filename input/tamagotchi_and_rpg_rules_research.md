# Comprehensive Research: Tamagotchi Care Mechanics & RPG Systems
**Reference & Design Blueprint for PetDuet (Dual-Device ESP32 CYD & PHP Server)**

---

## 1. Executive Summary & Design Synthesis

The goal of **PetDuet** is to unite two historically distinct but complementary gaming traditions:
1. **The Real-Time Virtual Pet (Tamagotchi / Digimon V-Pet / Monster Rancher):** Characterized by persistent real-time decay, empathetic bonding, time-sensitive care windows ("Care Mistakes"), and emergent lifecycle evolution.
2. **The Progression-Driven Role-Playing Game (Pokemon, Rune Factory, Classic RPGs):** Characterized by experience points (XP), leveling curves, resource management, crafting/recipes, profession roles, magic/spells, and social trading.

By combining these paradigms, pet maintenance (feeding, cleaning, resting) ceases to be a chore and becomes the foundational engine generating stamina, happiness, and ingredients for active RPG adventures, co-op quests, and peer interactions across two ESP32 displays.

---

## 2. Authentic Tamagotchi Care Mechanics & Formulas

Across authentic Bandai releases (1996 P1/P2, Tamagotchi Connection Series, and modern Pix/Uni releases) as well as Bandai's battle-oriented Digimon V-Pets, virtual pet health is governed by deterministic timers, discrete meter thresholds, and a hidden penalty tracking system known as **Care Mistakes (CM)**.

### 2.1 Core Needs & Stat Decay Rates

Virtual pets operate on internal tick timers (typically 1 tick = 1 minute or 1 tick = 4 minutes depending on species and growth phase):

| Need / Metric | Scale | Typical Decay Rate | Behavioral Consequence |
|---|---|---|---|
| **Satiety / Hunger (Food)** | 0 – 100 (or 4 hearts) | -1 point every 3–6 min (Baby: -1/min; Adult: -1/8 min) | At 0, triggers **Attention Call (Beep)**. Triggers Care Mistake after 15 min. Health starts draining (-2 HP/tick). |
| **Happiness (Mood)** | 0 – 100 (or 4 hearts) | -1 point every 4–8 min | At 0, triggers **Attention Call**. Reduces job success rate by 50%. Inhibits evolution. |
| **Energy / Stamina** | 0 – 100 | -1 point every 5 min during day; restores to 100 during sleep | At <20, pet becomes sleepy, refuses training/jobs. Falls asleep automatically at bedtime. |
| **Hygiene / Cleanliness** | 0 – 100 | Drops by -25 per waste event (poop every 2–4 hours) | If waste remains for >20 min, causes sickness (skull icon) and environmental filth. |
| **Health (HP)** | 0 – 100 | 100 under normal care; drains only during sickness or starvation | If HP hits 0, pet enters Critical/Fainting state. If untreated for 6 hours, pet passes away / departs. |

### 2.2 Meals vs. Snacks (The Weight & Sickness Paradox)
* **Meals (Standard Food / Bread / Rice / Meat):**
  - Fills Hunger (+25 to +40 points).
  - Increases Weight by +1 gram/oz.
  - Safe to feed until hunger is 100% full.
* **Snacks / Treats (Candy / Cake / Sweets):**
  - Instantly boosts Happiness (+20 to +30 points).
  - Increases Weight by +2 grams/oz.
  - **Overfeeding Penalty (Cavity / Toothache Rule):** Feeding more than 3 consecutive snacks without exercise/play causes a **Cavity / Sickness Event** requiring 1–2 doses of medicine.

### 2.3 The Care Mistake (CM) Engine
A **Care Mistake** is the foundational hidden metric that determines evolution branches.

```
+-----------------------------------------------------------+
| Condition Met:                                            |
|   Hunger = 0  OR  Happiness = 0  OR  Asleep with Light ON  |
+-----------------------------------------------------------+
                             │
                             ▼
+-----------------------------------------------------------+
| Trigger Attention Call:                                   |
|   - Icon active on screen                                 |
|   - Audio alert (beeping)                                 |
|   - Start 15-Minute Countdown Timer                       |
+-----------------------------------------------------------+
             │                                   │
             ▼                                   ▼
[Player responds within 15 min]     [Player fails to respond (15m elapsed)]
             │                                   │
             ▼                                   ▼
     Icon turns OFF                      Icon turns OFF
    NO MISTAKE LOGGED                   +1 CARE MISTAKE RECORDED
```

* **Physical Care Mistake:** Hunger drops to 0 and is ignored for 15 minutes; or Sickness (skull) ignored for 15 minutes.
* **Mental Care Mistake:** Happiness drops to 0 and is ignored for 15 minutes; or pet goes to sleep and lights stay on for >60 minutes.
* **Discipline / Training Call:** Pet cries or beeps for no reason (meters full). If scolded/time-out within 15 min, Discipline rises by +25%. If ignored, counts as a discipline miss.

### 2.4 Lifecycle Progression Windows

Virtual pet lifecycles are divided into distinct growth phases:

```
[EGG] ──(5 min / 50 Taps)──► [BABY] ──(1-2 Hours)──► [YOUTH / CHILD]
                                                           │
                                                           ▼ (24-48 Hours)
[OLD AGE / SENIOR] ◄──(7+ Days)─── [ADULT (Branch A/B/C)] ◄┘
```

1. **Egg Phase:** Requires physical warming (tapping screen, rubbing, or temperature comfort). Hatch time: 2–5 minutes.
2. **Baby Phase (Infant):** Very fragile; hunger and happiness drop 3x faster; cannot perform jobs; sleeps frequently. Duration: 1 to 2 hours.
3. **Youth / Child Phase:** Learns basic jobs and starter spells; starts building personality; discipline meter unlocked. Duration: 24 to 48 hours.
4. **Adult Phase:** Peak attributes; full access to advanced recipes, high-tier jobs, and magic; branching form determined by accumulated Care Mistakes and Biome affinity.
5. **Old Age / Senior Phase:** Stat decay slows down; gains high wisdom/luck; teaches skills or passes down rare heirlooms/accessories to the next generation egg (prestige/inheritance system).

---

## 3. RPG Mechanics Harmonized with Virtual Pet Design

To transform the pet simulation into a compelling, cooperative RPG, classic role-playing mechanics are mapped onto pet care activities.

### 3.1 Primary Attributes & Derived Stats

```
                    ┌─────────────────────────┐
                    │      CORE ATTRIBUTES    │
                    ├─────────────────────────┤
                    │ STR (Strength)          │
                    │ INT (Intelligence)      │
                    │ AGI (Agility / Speed)   │
                    │ VIT (Vitality)          │
                    │ LCK (Luck)              │
                    └────────────┬────────────┘
                                 │
           ┌─────────────────────┼─────────────────────┐
           ▼                     ▼                     ▼
┌──────────────────┐   ┌──────────────────┐   ┌──────────────────┐
│ PHYSICAL COMBAT  │   │ MAGIC & SPELLS   │   │ WORK & FORAGING  │
├──────────────────┤   ├──────────────────┤   ├──────────────────┤
│ Max HP (from VIT)│   │ Max MP (from INT)│   │ Job Yield (STR)  │
│ Attack Power     │   │ Spell Power (INT)│   │ Forage Drop (LCK)│
│ Defense Shield   │   │ Cooldown (AGI)   │   │ Energy Efficiency│
└──────────────────┘   └──────────────────┘   └──────────────────┘
```

* **Strength (STR):** Increases physical labor efficiency in Jobs (higher currency $ and material yield).
* **Intelligence (INT):** Determines Mana Pool size ($MP = 20 + 5 \times INT$), spell damage, and healing magnitude.
* **Agility (AGI):** Reduces action cooldowns, speeds up job completion, and improves mini-game reaction windows.
* **Vitality (VIT):** Increases maximum Health Points ($HP = 80 + 10 \times VIT$) and reduces hunger decay rate.
* **Luck (LCK):** Boosts rare item drop rates during foraging and increases critical bonus chances in jobs and trades.

### 3.2 Experience (XP) & Leveling Progression Formula

To keep the game rewarding in the early game while providing long-term goals over weeks of real-time play, a quadratic progression curve is optimal:

$$\text{RequiredXP}(\text{Level}) = \text{BaseXP} \times (\text{Level} - 1)^{1.8} + 50 \times (\text{Level} - 1)$$

Where:
* $\text{BaseXP} = 100$
* **Level 1 $\rightarrow$ 2:** 100 XP (achievable in day 1 with 2 meals + 1 mini-game + 1 friend treat)
* **Level 5:** ~750 XP (unlocks Tier 2 recipes, starter magic, and Youth evolution)
* **Level 10:** ~2,400 XP (unlocks Adult evolution, Tier 3 spells, specialized professions)
* **Level 20:** ~7,800 XP (unlocks Master Crafting and Grand Cooperative Magic)

#### XP Award Matrix
| Activity | Base XP | Bonus Multipliers |
|---|---|---|
| **Feed Regular Meal** | +10 XP | +5 XP if cooked recipe matching biome preference |
| **Mini-Game / Training** | +15 XP | Up to +25 XP based on mini-game score |
| **Clean Waste / Bath** | +10 XP | +5 XP if cleaned immediately within 5 minutes |
| **Complete Job** | +30–80 XP | Scaled by Job Tier and pet happiness (>80% happy = +25% XP) |
| **Send Friend Treat (Socket)** | +25 XP | +10 XP to both devices upon successful peer delivery |
| **Co-op Activity / Magic** | +40–60 XP | Awarded to both pets simultaneously |

### 3.3 Professions & Job System

Professions define how the pet interacts with the game economy and generates resources for recipes:

```
                      ┌──────────────────────┐
                      │    PROFESSIONS       │
                      └──────────┬───────────┘
           ┌─────────────────────┼─────────────────────┐
           ▼                     ▼                     ▼
┌──────────────────┐   ┌──────────────────┐   ┌──────────────────┐
│ 🌿 FORAGER       │   │ 🍳 CULINARIAN    │   │ ✨ MYSTIC        │
├──────────────────┤   ├──────────────────┤   ├──────────────────┤
│ - Herb Gathering │   │ - Recipe Brewing │   │ - Mana Crystals  │
│ - Mineral Mining │   │ - Stat Buff Foods│   │ - Enchanting     │
│ - Pearl Diving   │   │ - Trade Goods    │   │ - Co-op Blessings│
└──────────────────┘   └──────────────────┘   └──────────────────┘
```

1. **Job Mechanics:**
   - Costs **Energy** (-15 to -35 points) and takes a real-time duration (e.g., 5 min, 15 min, or 1 hour).
   - If energy is too low (<20) or hunger is 0, pet refuses to work.
   - Payout: Local currency ($ / Coins), raw materials (herbs, sunstones, mushrooms, pearls), and XP.
2. **Profession Synergy with Biomes:**
   - **Desert Pet + Miner:** +25% Sunstone and gold yield.
   - **Water Pet + Pearl Diver:** +30% River Pearl and clean water yield.
   - **Forest Pet + Forager:** +25% Rare Truffle and enchanted wood yield.
   - **Grassland Pet + Farmer/Chef:** +25% Meadow grain and cooking efficiency.

### 3.4 Cooking & Recipe Synthesis

Feeding is not just an empty heart refill; recipes are synthesized using gathered materials:

$$\text{Dish Effectiveness} = \text{Base Nutrition} \times (1 + \text{BiomeAffinity}) \times (1 + \text{FreshnessBonus})$$

* **Sample Recipe Tier Matrix:**
  * **Tier 1 (Lvl 1+):** *Meadow Herb Salad* (2x Meadow Herbs) $\rightarrow$ +25 Food, +10 Energy.
  * **Tier 2 (Lvl 3+):** *Spiced Desert Jerky* (2x Cactus Flesh, 1x Herb) $\rightarrow$ +40 Food, +15 HP, 2 hr "Stamina Shield".
  * **Tier 3 (Lvl 5+):** *River Pearl Stew* (1x River Pearl, 2x Mushrooms, 1x Herb) $\rightarrow$ +65 Food, +30 HP, +20 Mana.
  * **Tier 4 (Lvl 8+):** *Celestial Ambrosia* (1x Sunstone, 1x Pearl, 2x Truffles) $\rightarrow$ +100 Food, +50 HP, +50 Energy, +50 Mana, +120 XP.

### 3.5 Magic & Spellcraft (Solo & Cooperative Dual-Cast)

Magic draws from the pet's **Mana Pool (MP)** and provides distinct solo utilities and socket-transmitted friend interactions:

```
[Device 1 (Pet A)]                              [Device 2 (Pet B)]
  Casts "Friend Blessing"                         Receives Packet:
  - Spends 25 MP           ──[WebSocket / TCP]──► - Golden Sparkles on LCD
  - Earns +35 XP                                  - Gains +30 HP & +20 Happy
```

* **Solo Spells:**
  - *Purify (Cleanse):* Instantly dissolves waste and cleanses illness (Cost: 15 MP).
  - *Snooze Charm (Deep Sleep):* Accelerates energy restoration by 2x during nap (Cost: 20 MP).
  - *Harvest Surge:* Instantly completes the active Foraging Job (Cost: 35 MP, 2 hr cooldown).
* **Co-op Friend Spells (Server / Socket Transmitted):**
  - *Rejuvenation Ray:* Restores 35 HP and 25 Food to friend's pet if they are hungry (Cost: 30 MP).
  - *Bravery Ward:* Shields friend's pet from stat decay for 3 hours (Cost: 40 MP).
  - *Dual Astral Harmony (Dual-Cast):* Both players press MGC within 10 seconds $\rightarrow$ Triggers screen-filling aurora effect on both CYD displays, grants +100 XP and a rare accessory gift to both players!

### 3.6 Trading & Economy System

To foster community and cooperation across two devices:
1. **Trade Initiation:** Player A selects an inventory item (e.g. *Golden Crown* or *Rare Pearl*) and specifies a desired category or item from Player B.
2. **Server Escrow:** The offer is locked on the server via `POST /api/sync`.
3. **Friend Notification:** Device 2 chimes and displays: `"Trade Offer from [PetName]: [Item]! Accept [Y/N]?"`.
4. **Atomic Exchange:** When Player B accepts, the server atomically swaps ownership in the database; both devices receive updated inventory payloads over WebSocket/TCP without duplication risk.

---

## 4. Environmental Dynamics: Biomes & Seasonal Cycles

In accordance with `PetDuet.md`, environments dynamically alter pet appearance, behavioral quirks, and growth requirements.

### 4.1 The 4 Core Biomes

| Biome | Color Theme | Native Pet Trait | Environmental Perk | Vulnerability / Need |
|---|---|---|---|---|
| **Desert** | `#FFD700` (Golden Yellow) | *Sun-Forged:* Heat resistant; high physical STR | +25% Mining yields; immune to heatwaves | Food/Hydration decays 15% faster; dislikes rain |
| **Grassland** | `#7CFC00` (Lawn Green) | *Verdant Spirit:* High base happiness; fast play | +20% Herb yields; eggs hatch 25% faster | Needs frequent play; gets bored quickly |
| **Water** | `#1E90FF` (Deep Sky Blue) | *Aquatic Affinity:* High mana pool (+20 Max MP) | +25% Fishing & Pearl dive yield; fast swim play | Loses Hygiene faster if display is dirty; needs clean water |
| **Forest** | `#8B4513` (Saddle Brown) | *Ancient Wisdom:* High INT and job resilience | +25% Wood & Mushroom foraging; recipes give +15% more XP | Sleeps earlier (starts bedtime at 19:30); requires high darkness |

### 4.2 The Calendar-Week Seasonal Cycle (NTP Berlin Time)

Because the ESP32 retrieves real-world time via NTP (Berlin timezone, as specified in `PetDuet.md`), seasonal cycles sync to ISO calendar week numbers ($W = \text{ISO\_Week} \pmod 4$):

```
Week 1 (Spring) ──► Week 2 (Summer) ──► Week 3 (Autumn) ──► Week 4 (Winter) ──► (Repeat)
```

1. **Spring (Week 1 / Grün/Pastel):**
   - *Theme:* Floral blooms, mild breeze.
   - *Effect:* Egg hatching warmth requirement reduced by 30%. Herb yields doubled. Pet happiness decay slowed by 25%.
2. **Summer (Week 2 / Vibrant Golden):**
   - *Theme:* Sun rays, heatwave shimmers.
   - *Effect:* Energy decays 20% faster due to heat. Desert pets gain +20% all stats. Water pets require frequent hydration/bathing.
3. **Autumn (Week 3 / Rust & Amber):**
   - *Theme:* Falling leaves, rich harvest.
   - *Effect:* Mushroom and fruit drops quadrupled. Cooking gives +30% bonus XP. Optimal time for leveling cooking and crafting.
4. **Winter (Week 4 / Crisp Frost & Snowflakes):**
   - *Theme:* Falling snow pixel particles, frosty borders.
   - *Effect:* Food foraging drops reduced by 30% (scarcity). Staying warm requires snuggle interactions. Rare Ice Crystals can be mined for high-tier ice magic.

---

## 5. Architectural Translation: The Unified `rules.json`

To satisfy the core specification in `PetDuet.md`—where **all game mechanics are human-readable, loaded and interpreted dynamically at runtime by both the ESP32 and the PHP server**—the rules are represented as a structured, deterministic JSON schema.

### 5.1 JSON Rules Schema (`rules.json`)

```json
{
  "version": "1.0.0",
  "engine": "PetDuet-Unified-Rules",
  "meta": {
    "title": "PetDuet Core Game Rules",
    "supported_languages": ["en", "de"],
    "tick_interval_seconds": 60
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
  ]
}
```

---

## 6. Synthesis: Recommended Implementation Roadmap for PetDuet

1. **Rule Engine Parser (`rules.json`):**
   - Implement in TypeScript for web preview / simulator.
   - Implement in C++ (ArduinoJson / cJSON) for the ESP32 firmware.
   - Implement in PHP (`json_decode`) for server-side verification and cloud sync.
2. **ESP32 Firmware Display Layout (320x240 ILI9341 via LVGL & `cyd_config.h`):**
   - **Left Column (Width: 36px):** 5 touch action buttons for your pet (Feed, Play, Bath, RPG Menu, Cloud Save).
   - **Left Stage (Width: 122px):** Own pet sprite, level badge, emotion bubbles, HP/Food bars, and seasonal effects.
   - **Center Divider (Width: 8px):** Hardware wireless direct-socket sync bridge & connection heartbeat LED.
   - **Right Stage (Width: 122px, Right of Own Pet):** Partner pet live sprite, partner level, online status, HP/Food bars, and speech balloons.
   - **Right Column (Width: 36px):** 5 touch interaction buttons targeting friend (Treat, Quick Chat, Co-op Magic+, Quest, Trade).
   - **Bottom HUD:** Status indicators, coins, happiness, energy, and active season.
3. **Server & Networking:**
   - Lightweight PHP backend on `https://familie-riegel.de/petduet/` using SQLite database (`petduet.db`).
   - WebSocket relay for instantaneous sub-100ms peer updates (treat delivery, spell casts, trade offers).
   - REST API fallback for initial registration, cloud saves, and leaderboard rankings.

---
*Document prepared for PetDuet Development — 2026-11-08*
