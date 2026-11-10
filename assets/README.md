# PetDuet Asset Catalog & Graphics Repository

This directory contains the collected and researched graphics, pixel art sprites, animation keyframes, item icons, and UI symbols for **PetDuet: Dual-Device Tamagotchi RPG**.

---

## Directory Structure

```
assets/
├── README.md                          # Asset guide and integration documentation
├── sprites/
│   ├── pets/                          # Pet species sprites across 4 biomes & animation states
│   │   ├── grassland_pet_idle.svg     # Leafkin Idle breathing loop
│   │   ├── grassland_pet_happy.svg    # Leafkin Happy hop & smile
│   │   ├── grassland_pet_eating.svg   # Leafkin Eating munch
│   │   ├── grassland_pet_sleeping.svg # Leafkin Curled up rest
│   │   ├── grassland_pet_sick.svg     # Leafkin Trembling sick / care mistake state
│   │   ├── desert_pet_idle.svg        # Sunscale Idle
│   │   ├── desert_pet_happy.svg       # Sunscale Happy
│   │   ├── desert_pet_eating.svg      # Sunscale Eating
│   │   ├── desert_pet_sleeping.svg    # Sunscale Sleeping
│   │   ├── desert_pet_sick.svg        # Sunscale Sick
│   │   ├── water_pet_idle.svg         # Tideling Idle
│   │   ├── water_pet_happy.svg        # Tideling Happy
│   │   ├── water_pet_eating.svg       # Tideling Eating
│   │   ├── water_pet_sleeping.svg     # Tideling Sleeping
│   │   ├── water_pet_sick.svg         # Tideling Sick
│   │   ├── forest_pet_idle.svg        # Bramble Idle
│   │   ├── forest_pet_happy.svg       # Bramble Happy
│   │   ├── forest_pet_eating.svg      # Bramble Eating
│   │   ├── forest_pet_sleeping.svg    # Bramble Sleeping
│   │   ├── forest_pet_sick.svg        # Bramble Sick
│   │   └── egg_stages.svg             # Hatching Egg stages (intact, warm glow, cracking)
│   ├── emotions/                      # Floating reaction & balloon status icons
│   │   ├── emotion_heart.svg          # Love / cuddled / trade accepted
│   │   ├── emotion_happy_sparkle.svg  # Joy / level up / magic buff
│   │   ├── emotion_sleep_zzz.svg      # Floating sleeping ZZZ
│   │   ├── emotion_sweat_worry.svg    # Care mistake alarm / hunger call
│   │   ├── emotion_sick_skull.svg     # Sickness / dirty penalty
│   │   ├── emotion_exclamation.svg    # Tamagotchi attention call
│   │   └── emotion_musical_note.svg   # Whistle / mini-game fun
│   ├── items/                         # RPG consumable foods, materials, and wardrobe gear
│   │   ├── food_apple.svg             # Fresh Meadow Apple
│   │   ├── food_soup.svg              # Mushroom & Herb Stew
│   │   ├── food_honey_waffle.svg      # Golden Honey Waffle
│   │   ├── food_fish_skewer.svg       # Grilled Lagoon Fish
│   │   ├── food_sun_bread.svg         # Golden Dune Loaf
│   │   ├── mat_herb.svg               # Wild Medicinal Herb
│   │   ├── mat_sunstone.svg           # Radiant Sun Salt Crystal
│   │   ├── mat_mushroom.svg           # Forest Toadstool
│   │   ├── mat_kelp.svg               # Azure Lagoon Kelp
│   │   ├── mat_meat.svg               # Savory Tender Cutlet
│   │   ├── gear_straw_hat.svg         # Farmer Woven Straw Hat
│   │   ├── gear_wizard_cap.svg        # Mystic Star Wizard Hat
│   │   ├── gear_goggles.svg           # Steampunk Brass Goggles
│   │   └── gear_feather_charm.svg     # Avian Swift Feather Charm
│   └── ui/                            # Care actions and hardware touch UI icons
│       ├── icon_feed.svg              # Feed Meal / Snack
│       ├── icon_play.svg              # Play Game / Ball
│       ├── icon_bath.svg              # Clean Poop / Bath Sponge
│       ├── icon_heal.svg              # First Aid Medicine Cross
│       ├── icon_magic.svg             # Cast Elemental Spell
│       ├── icon_quest.svg             # Co-op Scout Quest
│       ├── icon_trade.svg             # Peer Item Exchange
│       ├── icon_chat.svg              # Socket Chat Balloon
│       ├── icon_save.svg              # Cloud / Flash Save
│       ├── icon_wifi_ap.svg           # SoftAP / WiFi Config
│       ├── icon_sync_link.svg         # Direct Hardware SYNC Bridge
│       └── icon_coin.svg              # Stamped Gold Coin
├── cyd_bitmaps/                       # C/C++ PROGMEM 16-bit RGB565 Header Files for ESP32 CYD
│   ├── PetSprites_RGB565.h            # ILI9341 16-bit bitmaps for pet animations & emotions
│   └── ItemIcons_RGB565.h             # ILI9341 16-bit bitmaps for items & care icons
└── previews/
    └── AssetCatalogPreview.html       # Interactive animated HTML gallery to test all graphics
```

---

## Technical Integration

### 1. Web / React SPA
All SVG sprites are pixel-perfect vectors built with crisp pixel coordinates (`shape-rendering: crispEdges`). They scale cleanly to any resolution while maintaining 100% retro pixel authenticity.

### 2. ESP32 CYD (TFT_eSPI / ILI9341 320x240)
The headers in `cyd_bitmaps/` provide static `const uint16_t[] PROGMEM` pixel arrays formatted in **RGB565** (little-endian or big-endian swap configurable).
Render directly using:
```cpp
#include "cyd_bitmaps/PetSprites_RGB565.h"
#include "cyd_bitmaps/ItemIcons_RGB565.h"

// Draw Leafkin Idle frame at (x, y)
tft.setSwapBytes(true);
tft.pushImage(x, y, 32, 32, leafkin_idle_frame1);
```
