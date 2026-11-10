# PetDuet Graphics, Animations & Iconography Research

This document outlines the visual identity, pixel art sprite guidelines, animation state machine, and hardware-constrained graphics pipeline for **PetDuet: Dual-Device Tamagotchi RPG** on both the **ESP32-2432S028R Cheap Yellow Display (ILI9341 320x240 LCD)** and the **Web Application**.

---

## 1. Design Philosophy & Visual Language

### 1.1 Aesthetic Target
- **Style**: High-fidelity 16-bit retro pixel art with modern color harmony (reminiscent of Game Boy Advance / Neo Geo Pocket Color / Tamagotchi Connection v3).
- **Display Resolution Target**: 
  - ESP32 CYD: 320 × 240 pixels (landscape).
  - Effective Pet Stage: 130 × 170 pixels per viewport (left = Player, right = Partner).
- **Pixel Grid Ratio**: Native 1:1 pixel rendering (`shape-rendering: crispEdges` in SVG, direct buffer writes on CYD TFT).
- **Color Shading Rule**: 4 shades per material:
  1. `Outline`: Deep slate `#0f172a` (ESP32: `0x0841` or `0x0000`).
  2. `Main Tone`: Saturated primary biome hue.
  3. `Shadow / Shade`: -30% brightness, warm or cool shift.
  4. `Highlight / Belly`: +40% brightness, creamy soft contrast.

---

## 2. Biome Critter Designs & Evolution Stages

Each biome houses a distinctive species tailored to its environmental trait:

| Biome | Species Name | Animal Archetype | Primary Palette (Hex / RGB565) | Signature Silhouette Features |
|---|---|---|---|---|
| **Grassland** | **Leafkin** | Meadow Hare / Sprout | `#10b981` / `0x2E69` (Emerald) | Sprout on head, tall floppy ears, clover tail |
| **Desert** | **Sunscale** | Fennec / Dunerunner | `#f59e0b` / `0xFD40` (Amber Gold) | Suncrest horn frill, oversized ears, lizard tail |
| **Water** | **Tideling** | Axolotl / Sea Otter | `#06b6d4` / `0x1DCF` (Aqua Azure) | External coral gill frills, droplet tail, fin paws |
| **Forest** | **Bramble** | Woodland Bear Cub | `#8d6e63` / `0x8BE3` (Warm Bark) | Mushroom cap/leaves, acorn bell necklace, fluffy round ears |

### Evolution Phase Dimensions:
- **Egg**: 24 × 30 px (centered with animated warm glow halo and dynamic fracture lines).
- **Baby (Lv. 1–3)**: 28 × 28 px (round proportions, oversized head 60% of body, big curious eyes).
- **Youth (Lv. 4–7)**: 36 × 36 px (active stance, emerging horns/fins/sprouts, expressive paws).
- **Adult (Lv. 8–14)**: 48 × 48 px (distinctive biome features, equipment socket points).
- **Elder (Lv. 15+)**: 52 × 52 px (crowned/horned, glowing aura, master embroidery markings).

---

## 3. Pet Animation System & Emotional State Machine

To run efficiently on both the ESP32 (under 4MB flash, 520KB SRAM) and 60fps web browser displays, animations use a **2-to-4 frame loop architecture**:

### 3.1 Animation States & Frame Specs

```
                    ┌─────────────┐
                    │    IDLE     │◄───────────────────┐
                    │ (2 Frames)  │                    │
                    └──────┬──────┘                    │
                           │                           │
          ┌────────────────┼────────────────┐          │
          ▼                ▼                ▼          │
    ┌───────────┐    ┌───────────┐    ┌───────────┐    │
    │   HAPPY   │    │  EATING   │    │ SLEEPING  │    │
    │(Hop 2-Fr) │    │(Chew 4-Fr)│    │(Curl 2-Fr)│    │
    └─────┬─────┘    └─────┬─────┘    └─────┬─────┘    │
          │                │                │          │
          └────────────────┼────────────────┴──────────┘
                           ▼
                     ┌───────────┐
                     │ SICK/SAD  │
                     │(Tremble)  │
                     └───────────┘
```

1. **Idle Breathing (2 Frames, 1.2s cycle)**:
   - Frame 1: Resting stance.
   - Frame 2: Body scaled Y + 2px, ears/horns sway back 1px, belly breath expansion.
2. **Happy Hop (2 Frames, 400ms cycle)**:
   - Frame 1: Crouch prep (-2px Y, wide feet).
   - Frame 2: Airborne (+6px Y leap, ears perk up, eyes become happy crescents `^ ^`, cheek blushes brighten).
3. **Eating / Munching (4 Frames, 600ms cycle)**:
   - Frame 1: Item presented at mouth.
   - Frame 2: Big open mouth (`O`), eyes squint.
   - Frame 3: Closed mouth chew with puffed cheeks.
   - Frame 4: Gulp/swallow particle with satisfaction sparkle.
4. **Sleeping / Rest (2 Frames + Floating ZZZ, 2.0s cycle)**:
   - Frame 1: Curled up posture, closed eyes `- -`, gentle chest pulse.
   - Overlay: Animated floating `z`, `Z`, `Z` drifting diagonally upwards with opacity fade.
5. **Sick / Care Mistake Call (2 Frames, 600ms tremble)**:
   - Frame 1: Drooped ears, downcast teary eyes, slightly green/purple tint shift.
   - Frame 2: 1px horizontal jitter/tremble with popping sweatdrop / skull icon above head.

---

## 4. Emotional Emoticons & Balloon Indicators

Balloons render dynamically at `(cx + 12, cy - 24)` above the critter:

- **❤️ Heart**: Affection, cuddle response, trade accepted, treat received.
- **✨ Sparkles**: Level-up, spell cast, buffs applied.
- **💤 ZZZ**: Sleeping mode, restoring energy.
- **💧 Sweatdrop**: Attention prompt (hunger < 20%, Care Mistake grace window active).
- **💀 Poison/Sick**: Neglect threshold reached, medicine required.
- **💬 Speech Balloon**: Socket chat message packet received from partner CYD.
- **⚔️ Crossed Swords**: Co-op quest launched.

---

## 5. Items & UI Iconography Specifications

### 5.1 Item Sprites (24 × 24 px canvas, 32 × 32 display container)
1. **Foods**:
   - `food_apple`: Crisp Red Apple with green leaf.
   - `food_soup`: Hearty Woodland Mushroom Stew in earthen bowl with steam wisps.
   - `food_honey_waffle`: Golden waffle grid with amber honey drizzle and butter pat.
   - `food_fish_skewer`: Char-grilled Lagoon fish on bamboo skewer with lemon zest.
   - `food_sun_bread`: Golden crust desert loaf with spiral scoring.
2. **Crafting Materials**:
   - `mat_herb`: Bundle of three wild healing leaves bound by twine.
   - `mat_sunstone`: Prismatic radiant sun-salt crystal shard.
   - `mat_mushroom`: Spotted red forest toadstool mushroom.
   - `mat_kelp`: Deep teal braided ocean kelp ribbons.
   - `mat_meat`: Prime marbled savory cutlet.
3. **Wardrobe Equipment**:
   - `gear_straw_hat`: Woven farmer straw hat with red ribbon band.
   - `gear_wizard_cap`: Pointed mystic violet wizard hat with golden star.
   - `gear_goggles`: Brass rimmed steampunk explorer goggles.
   - `gear_feather_charm`: Shimmering wild avian feather pendant.

### 5.2 Care & Control UI Icons (20 × 20 px / 24 × 24 px)
- **Feed**: Earthen bowl with wooden feeding spoon.
- **Play**: Bouncing striped pet ball with motion sparks.
- **Bath**: Suds sponge with 3 iridescent soap bubbles.
- **Heal**: Emerald heart talisman with white medicinal cross.
- **Magic**: Crystal-tipped wand releasing stardust sparks.
- **Quest**: Dual crossed swords with gold crossguard.
- **Trade**: Interlocking reciprocal arrows forming an escrow cycle.
- **Chat**: Dual pixel speech bubbles with wireless signal dot.
- **WiFi Portal**: Dual broadcast waves radiating from antenna mast.
- **Coin / Gold**: Octagonal stamped bronze and gold coin.

---

## 6. ESP32 Hardware Integration & Format Conversion

For maximum hardware efficiency on the ESP32-2432S028R:
1. **Vector SVG Sources**: Maintained in `assets/sprites/` for web rendering, documentation, and scalability.
2. **C++ Header Files**: Maintained in `assets/cyd_bitmaps/` containing PROGMEM 16-bit 565 byte arrays (`uint16_t sprite_data[] PROGMEM`) ready to be drawn directly with TFT_eSPI:
   ```cpp
   tft.pushImage(x, y, width, height, bitmap_rgb565);
   ```
3. **Color Compatibility**: Hex codes translate directly to 16-bit 565 via `((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)`.
