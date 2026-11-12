/**
 * Human-Readable Config Parser for Game Mechanics
 * Parses INI / TOML / CFG text files into strongly-typed GameRulesConfig.
 * Platform-agnostic: Can run in Web, Node.js, and ported directly to Android (Kotlin) & ESP32.
 */

import { Recipe } from '../types/pet';

export interface ParsedRecipeRule {
  id: string;
  name: string;
  name_de?: string;
  description?: string;
  description_de?: string;
  minLevel: number;
  ingredients: { itemId: string; count: number; name?: string; name_de?: string }[];
  hungerGain: number;
  healthGain: number;
  energyGain: number;
  manaGain: number;
  xpReward: number;
  icon: string;
}

export interface ParsedJobRule {
  id: string;
  title: string;
  minLevel: number;
  durationSeconds: number;
  coinReward: number;
  xpReward: number;
  drop: { itemId: string; chance: number };
  icon: string;
}

export interface ParsedSpellRule {
  id: string;
  name: string;
  minLevel: number;
  manaCost: number;
  target: 'self' | 'friend' | 'both';
  healthBonus: number;
  happinessBonus: number;
  energyBonus: number;
  hygieneBonus: number;
  biome: string;
  icon: string;
}

export interface ParsedDailyMissionRule {
  id: string;
  name: string;
  name_de?: string;
  description: string;
  description_de?: string;
  actionType: 'feed' | 'login_streak' | 'play' | 'bath' | 'send_treat' | 'cast_spell' | 'cook' | 'start_job';
  targetCount: number;
  coinReward: number;
  xpReward: number;
  icon: string;
}

export const DEFAULT_DAILY_MISSIONS: ParsedDailyMissionRule[] = [
  {
    id: 'mission_feed',
    name: 'Nourish Pet',
    name_de: 'Pet Füttern',
    description: 'Feed your pet 3 times today',
    description_de: 'Füttere dein Haustier heute 3 Mal',
    actionType: 'feed',
    targetCount: 3,
    coinReward: 40,
    xpReward: 50,
    icon: '🍖',
  },
  {
    id: 'mission_login_streak',
    name: 'Loyal Companion',
    name_de: 'Treuer Begleiter',
    description: 'Log in 2 days in a row',
    description_de: 'Logge dich 2 Tage hintereinander ein',
    actionType: 'login_streak',
    targetCount: 2,
    coinReward: 60,
    xpReward: 80,
    icon: '📅',
  },
  {
    id: 'mission_play',
    name: 'Playtime Fun',
    name_de: 'Spiel & Spass',
    description: 'Play with your pet 2 times',
    description_de: 'Spiele 2 Mal mit deinem Haustier',
    actionType: 'play',
    targetCount: 2,
    coinReward: 30,
    xpReward: 40,
    icon: '🎾',
  },
  {
    id: 'mission_bath',
    name: 'Bubble Bath',
    name_de: 'Blitzsauber',
    description: 'Give your pet a sparkling bath',
    description_de: 'Bade dein Haustier oder putze',
    actionType: 'bath',
    targetCount: 1,
    coinReward: 25,
    xpReward: 35,
    icon: '🧼',
  },
  {
    id: 'mission_treat',
    name: 'Share the Love',
    name_de: 'Freundes-Leckerli',
    description: 'Send a treat or blessing to your partner',
    description_de: 'Sende ein Leckerli an dein Partner-Pet',
    actionType: 'send_treat',
    targetCount: 1,
    coinReward: 50,
    xpReward: 60,
    icon: '🎁',
  },
];

export interface ParsedShopItemRule {
  id: string;
  name: string;
  name_de?: string;
  priceCoins: number;
  minLevel: number;
  category: string;
  slot?: string;
  icon: string;
  description?: string;
  description_de?: string;
  effects?: Record<string, any>;
}

export interface ParsedShopRecipeRule {
  id: string;
  name: string;
  name_de?: string;
  priceCoins: number;
  minLevel: number;
  icon: string;
  description?: string;
  description_de?: string;
  ingredients?: Record<string, number>;
  effects?: Record<string, any>;
}

export interface ParsedShopSpellRule {
  id: string;
  name: string;
  name_de?: string;
  priceCoins: number;
  minLevel: number;
  manaCost: number;
  target: 'self' | 'friend' | 'both';
  icon: string;
  description?: string;
  description_de?: string;
  effects?: Record<string, any>;
}

export interface ParsedShopConfig {
  items: ParsedShopItemRule[];
  recipes: ParsedShopRecipeRule[];
  spells: ParsedShopSpellRule[];
}

export const DEFAULT_SHOP_CONFIG: ParsedShopConfig = {
  items: [
    {
      id: 'item_apple',
      name: 'Fresh Apple',
      name_de: 'Frischer Apfel',
      priceCoins: 5,
      minLevel: 1,
      category: 'food',
      icon: '🍎',
      description: 'Crisp red orchard apple that restores basic hunger.',
      description_de: 'Knackiger roter Apfel, der den kleinen Hunger stillt.',
      effects: { hunger: 15, happiness: 5 },
    },
    {
      id: 'item_herb',
      name: 'Meadow Herb Bundle',
      name_de: 'Wiesenkräuter-Bündel',
      priceCoins: 8,
      minLevel: 1,
      category: 'material',
      icon: '🌿',
      description: 'Wild medicinal herb used for brewing remedies and salads.',
      description_de: 'Frisch gepflückte Wiesenkräuter für Salate und Tinkturen.',
      effects: {},
    },
    {
      id: 'item_salve',
      name: 'Healing Salve',
      name_de: 'Wohltuende Heilsalbe',
      priceCoins: 25,
      minLevel: 2,
      category: 'potion',
      icon: '🧴',
      description: 'Herbal balm that restores 40 HP and cures illness.',
      description_de: 'Heilende Kräutersalbe, die 40 HP wiederherstellt und Krankheiten heilt.',
      effects: { health: 40, cure_sickness: true },
    },
    {
      id: 'item_energy_elixir',
      name: 'Energy Tonic',
      name_de: 'Vitalitäts-Tonikum',
      priceCoins: 35,
      minLevel: 3,
      category: 'potion',
      icon: '🧪',
      description: 'Invigorating elixir restoring 60 Energy and 15 Mana.',
      description_de: 'Stärkendes Tonikum für 60 Energie und 15 Mana.',
      effects: { energy: 60, mana: 15 },
    },
    {
      id: 'item_sunstone',
      name: 'Sun Salt Crystals',
      name_de: 'Sonnensalz-Kristalle',
      priceCoins: 45,
      minLevel: 3,
      category: 'material',
      icon: '🧂',
      description: 'Rare desert mineral crystal required for gourmet curing.',
      description_de: 'Sonnengereiftes Wüstensalz für nahrhaftes Dörrfleisch.',
      effects: {},
    },
    {
      id: 'item_blossom_crown',
      name: 'Blossom Crown',
      name_de: 'Blütenkrone',
      priceCoins: 75,
      minLevel: 2,
      category: 'accessory',
      slot: 'head',
      icon: '👑',
      description: 'Hand-woven daisy wreath. Grants +10 Happiness and +5% XP.',
      description_de: 'Handgeflochtene Blumenkrone für +10 Glück und +5% XP.',
      effects: { happiness: 10, xp_bonus_pct: 5 },
    },
    {
      id: 'item_wizard_cowl',
      name: 'Starry Wizard Cowl',
      name_de: 'Sternen-Zauberkappe',
      priceCoins: 110,
      minLevel: 4,
      category: 'accessory',
      slot: 'head',
      icon: '🧙',
      description: 'Star-patterned mystical hood boosting mana efficiency.',
      description_de: 'Sternenverzierte Kappe, die das Mana um +15 erhöht.',
      effects: { max_mana_bonus: 15 },
    },
    {
      id: 'item_tophat',
      name: 'Fancy Top Hat',
      name_de: 'Vornehmer Zylinder',
      priceCoins: 140,
      minLevel: 5,
      category: 'accessory',
      slot: 'head',
      icon: '🎩',
      description: 'Distinguished velvet top hat granting +15% coin rewards on jobs.',
      description_de: 'Eleganter Samtzylinder für +15% Münzgewinn bei Berufen.',
      effects: { job_coin_bonus_pct: 15 },
    },
    {
      id: 'item_evo_prism',
      name: 'Astral Evolution Prism',
      name_de: 'Astrales Evolutionsprisma',
      priceCoins: 250,
      minLevel: 8,
      category: 'evolution',
      icon: '🔮',
      description: 'Legendary radiant prism catalyzing evolution into Elder forms.',
      description_de: 'Legendäres Prisma zur Beschleunigung der Evolution zur Meisterstufe.',
      effects: { evolution_unlock: true },
    },
  ],
  recipes: [
    {
      id: 'rcp_salad',
      name: 'Meadow Herb Salad',
      name_de: 'Wiesenkräuter-Salat',
      priceCoins: 15,
      minLevel: 1,
      icon: '🥗',
      description: 'Blueprint teaching the synthesis of crisp meadow herbs.',
      description_de: 'Rezeptrolle zum Zubereiten von frischem Wiesensalat.',
      ingredients: { mat_herb: 2 },
      effects: { food: 25, energy: 10, xp: 20 },
    },
    {
      id: 'rcp_jerky',
      name: 'Desert Jerky',
      name_de: 'Wüsten-Dörrfleisch',
      priceCoins: 35,
      minLevel: 2,
      icon: '🥩',
      description: 'Blueprint for hearty sun-dried cured protein rations.',
      description_de: 'Rezeptrolle für sonnengetrocknetes Dörrfleisch.',
      ingredients: { mat_meat: 2, mat_sunstone: 1 },
      effects: { food: 45, energy: 20, happiness: 15, xp: 60 },
    },
    {
      id: 'rcp_berry_pie',
      name: 'Forest Berry Tart',
      name_de: 'Waldbeeren-Tarte',
      priceCoins: 65,
      minLevel: 3,
      icon: '🥧',
      description: 'Blueprint for warm, sweet woodland berry pastries.',
      description_de: 'Rezeptrolle für süße, duftende Waldbeerenkuchen.',
      ingredients: { mat_herb: 1, mat_mushroom: 1 },
      effects: { food: 35, happiness: 25, health: 15, xp: 80 },
    },
    {
      id: 'rcp_stew',
      name: 'Hearty River Stew',
      name_de: 'Herzhafter Flusseintopf',
      priceCoins: 110,
      minLevel: 4,
      icon: '🍲',
      description: 'Blueprint for deep mineral and seafood chowder.',
      description_de: 'Rezeptrolle für kräftigenden Flusseintopf.',
      ingredients: { mat_mushroom: 2, mat_pearl: 1 },
      effects: { food: 70, health: 30, mana: 15, xp: 110 },
    },
    {
      id: 'rcp_ambrosia',
      name: 'Celestial Ambrosia',
      name_de: 'Himmlische Ambrosia',
      priceCoins: 240,
      minLevel: 6,
      icon: '✨',
      description: 'Master recipe synthesizing essences of all 4 biomes.',
      description_de: 'Meisterrezept mit Essenzen aller vier Biome.',
      ingredients: { mat_sunstone: 1, mat_pearl: 1, mat_mushroom: 2, mat_herb: 2 },
      effects: { food: 100, health: 50, energy: 50, mana: 50, xp: 250 },
    },
  ],
  spells: [
    {
      id: 'spl_cleanse',
      name: 'Purify',
      name_de: 'Läuterung',
      priceCoins: 40,
      minLevel: 2,
      manaCost: 15,
      target: 'self',
      icon: '🫧',
      description: 'Tome teaching the instant cleansing of waste and disease.',
      description_de: 'Zauberbuch zur sofortigen Reinigung und Krankheitsheilung.',
      effects: { clean_poop: true, cure_sickness: true, xp: 20 },
    },
    {
      id: 'spl_heal',
      name: 'Vitality Aura',
      name_de: 'Vitalitäts-Aura',
      priceCoins: 70,
      minLevel: 2,
      manaCost: 20,
      target: 'self',
      icon: '💖',
      description: 'Tome radiating self-healing light restoring 25 HP.',
      description_de: 'Zauberbuch für eine wärmende Aura mit +25 HP.',
      effects: { health: 25, happiness: 10, xp: 35 },
    },
    {
      id: 'spl_friend_blessing',
      name: 'Friend Blessing',
      name_de: 'Freundes-Segen',
      priceCoins: 95,
      minLevel: 3,
      manaCost: 30,
      target: 'friend',
      icon: '✨',
      description: 'Tome enabling socket transmission of friendship waves.',
      description_de: 'Zauberbuch zum Senden von Segnungen an das Partner-CYD.',
      effects: { health: 35, happiness: 25, xp: 45 },
    },
    {
      id: 'spl_treat_burst',
      name: 'Astral Feast',
      name_de: 'Astral-Festmahl',
      priceCoins: 130,
      minLevel: 4,
      manaCost: 25,
      target: 'self',
      icon: '🍓',
      description: 'Tome conjuring spectral mana fruits that fulfill hunger.',
      description_de: 'Zauberbuch zur Beschwörung nahrhafter Mana-Beeren.',
      effects: { hunger: 35, energy: 15, xp: 50 },
    },
    {
      id: 'spl_astral_harmony',
      name: 'Dual Astral Harmony',
      name_de: 'Astrale Harmonie',
      priceCoins: 260,
      minLevel: 6,
      manaCost: 45,
      target: 'both',
      icon: '🌟',
      description: 'Ancient grand tome triggering dual synchronized screen auroras.',
      description_de: 'Uraltes Großzauberbuch für synchronisierte Nordlichter auf beiden Geräten.',
      effects: { health: 50, happiness: 40, xp: 100 },
    },
  ],
};

export interface GameRulesConfig {
  general: {
    gameTitle: string;
    version: string;
    maxStatValue: number;
    tickRateSeconds: number;
    socketPort: number;
    baseXpRequirement: number;
    xpLevelMultiplier: number;
  };
  decayRates: {
    hungerDecay: number;
    energyDecay: number;
    hygieneDecay: number;
    happinessDecay: number;
    healthStarveDecay: number;
    starveThreshold: number;
    naturalManaRegen: number;
  };
  evolutionPhases: {
    eggWarmthRequired: number;
    babyMinLevel: number;
    youthMinLevel: number;
    adultMinLevel: number;
    elderMinLevel: number;
  };
  biomes: Record<
    string,
    {
      name: string;
      colorHex: string;
      rgb565: string;
      trait: string;
      description?: string;
      growthRequirement?: string;
      energyBonusPct: number;
      hungerDecayBonusPct: number;
    }
  >;
  seasons: {
    springForageBonusPct: number;
    summerThirstDecayPct: number;
    autumnCookXpBonusPct: number;
    winterRestEnergyBonusPct: number;
  };
  recipes: ParsedRecipeRule[];
  jobs: ParsedJobRule[];
  magicSpells: ParsedSpellRule[];
  dailyMissions: ParsedDailyMissionRule[];
  shop: ParsedShopConfig;
  socialActions: {
    treatSendHungerGain: number;
    treatSendHappinessGain: number;
    treatSendXpReward: number;
    coopQuestCoinsReward: number;
    coopQuestXpReward: number;
    coopMagicHpBoost: number;
    coopMagicHappinessBoost: number;
  };
}

export function parseJsonGameRules(json: any): GameRulesConfig {
  const biomes: GameRulesConfig['biomes'] = {};
  if (json.biomes) {
    for (const [key, b] of Object.entries<any>(json.biomes)) {
      biomes[key] = {
        name: b.name || key,
        colorHex: b.color || '#7CFC00',
        rgb565: b.rgb565 || '0x7FE0',
        trait: b.trait || 'Resilient',
        description: b.name_de ? `${b.name} (${b.name_de})` : b.name,
        energyBonusPct: b.modifiers?.energy_decay ? Math.round((b.modifiers.energy_decay - 1) * 100) : 0,
        hungerDecayBonusPct: b.modifiers?.food_decay ? Math.round((b.modifiers.food_decay - 1) * 100) : 0,
      };
    }
  }

  const recipes: ParsedRecipeRule[] = [];
  if (Array.isArray(json.recipes)) {
    for (const r of json.recipes) {
      const ingredients = [];
      if (r.ingredients) {
        for (const [itemId, count] of Object.entries<any>(r.ingredients)) {
          ingredients.push({ itemId, count: Number(count) || 1 });
        }
      }
      recipes.push({
        id: r.id,
        name: r.name || r.id,
        name_de: r.name_de,
        description: r.description,
        description_de: r.description_de,
        minLevel: r.min_level || 1,
        ingredients,
        hungerGain: r.effects?.food || 25,
        healthGain: r.effects?.health || 0,
        energyGain: r.effects?.energy || 0,
        manaGain: r.effects?.mana || 0,
        xpReward: r.effects?.xp || 20,
        icon: r.icon || '🍲',
      });
    }
  }

  const jobs: ParsedJobRule[] = [];
  if (Array.isArray(json.jobs)) {
    for (const j of json.jobs) {
      jobs.push({
        id: j.id,
        title: j.name || j.id,
        minLevel: j.min_level || 1,
        durationSeconds: j.duration_seconds || 60,
        coinReward: j.rewards?.coins || 15,
        xpReward: j.rewards?.xp || 25,
        drop: {
          itemId: Object.keys(j.rewards?.materials || {})[0] || 'mat_herb',
          chance: 100,
        },
        icon: j.icon || '🌿',
      });
    }
  }

  const spells: ParsedSpellRule[] = [];
  if (Array.isArray(json.spells)) {
    for (const s of json.spells) {
      spells.push({
        id: s.id,
        name: s.name || s.id,
        minLevel: s.min_level || 1,
        manaCost: s.mana_cost || 15,
        target: s.target || 'self',
        healthBonus: s.effects?.health || 0,
        happinessBonus: s.effects?.happiness || 0,
        energyBonus: s.effects?.energy || 0,
        hygieneBonus: s.effects?.clean_poop ? 100 : 0,
        biome: s.biome || 'universal',
        icon: s.icon || '✨',
      });
    }
  }

  const dailyMissions: ParsedDailyMissionRule[] = [];
  if (Array.isArray(json.daily_missions)) {
    for (const m of json.daily_missions) {
      dailyMissions.push({
        id: m.id || `mission_${dailyMissions.length}`,
        name: m.name || m.title || 'Daily Mission',
        name_de: m.name_de || m.title_de,
        description: m.description || '',
        description_de: m.description_de || '',
        actionType: m.action_type || m.actionType || 'feed',
        targetCount: m.target_count || m.targetCount || 1,
        coinReward: m.rewards?.coins || m.coinReward || 30,
        xpReward: m.rewards?.xp || m.xpReward || 40,
        icon: m.icon || '🎯',
      });
    }
  }

  const shopItems: ParsedShopItemRule[] = [];
  if (Array.isArray(json.shop?.items)) {
    for (const it of json.shop.items) {
      shopItems.push({
        id: it.id,
        name: it.name || it.id,
        name_de: it.name_de,
        priceCoins: it.price_coins || it.cost || 10,
        minLevel: it.min_level || 1,
        category: it.category || 'misc',
        slot: it.slot,
        icon: it.icon || '📦',
        description: it.description,
        description_de: it.description_de,
        effects: it.effects || {},
      });
    }
  }

  const shopRecipes: ParsedShopRecipeRule[] = [];
  if (Array.isArray(json.shop?.recipes)) {
    for (const r of json.shop.recipes) {
      shopRecipes.push({
        id: r.id,
        name: r.name || r.id,
        name_de: r.name_de,
        priceCoins: r.price_coins || r.cost || 20,
        minLevel: r.min_level || 1,
        icon: r.icon || '📜',
        description: r.description,
        description_de: r.description_de,
        ingredients: r.ingredients || {},
        effects: r.effects || {},
      });
    }
  }

  const shopSpells: ParsedShopSpellRule[] = [];
  if (Array.isArray(json.shop?.spells)) {
    for (const s of json.shop.spells) {
      shopSpells.push({
        id: s.id,
        name: s.name || s.id,
        name_de: s.name_de,
        priceCoins: s.price_coins || s.cost || 50,
        minLevel: s.min_level || 1,
        manaCost: s.mana_cost || 20,
        target: s.target || 'self',
        icon: s.icon || '✨',
        description: s.description,
        description_de: s.description_de,
        effects: s.effects || {},
      });
    }
  }

  const parsedShop: ParsedShopConfig = {
    items: shopItems.length > 0 ? shopItems : DEFAULT_SHOP_CONFIG.items,
    recipes: shopRecipes.length > 0 ? shopRecipes : DEFAULT_SHOP_CONFIG.recipes,
    spells: shopSpells.length > 0 ? shopSpells : DEFAULT_SHOP_CONFIG.spells,
  };

  return {
    general: {
      gameTitle: json.meta?.title || 'PetDuet',
      version: json.version || '1.0.4',
      maxStatValue: 100,
      tickRateSeconds: json.meta?.tick_interval_seconds || 60,
      socketPort: 8888,
      baseXpRequirement: 100,
      xpLevelMultiplier: 1.8,
    },
    decayRates: {
      hungerDecay: json.stats?.food?.decay_per_tick || 0.25,
      energyDecay: json.stats?.energy?.decay_per_tick || 0.15,
      hygieneDecay: 0.10,
      happinessDecay: json.stats?.happiness?.decay_per_tick || 0.20,
      healthStarveDecay: json.stats?.health?.starve_decay || 1.5,
      starveThreshold: 20,
      naturalManaRegen: json.stats?.mana?.regen_per_tick || 0.5,
    },
    evolutionPhases: {
      eggWarmthRequired: 50,
      babyMinLevel: 1,
      youthMinLevel: 3,
      adultMinLevel: 8,
      elderMinLevel: 20,
    },
    biomes,
    seasons: {
      springForageBonusPct: 20,
      summerThirstDecayPct: 25,
      autumnCookXpBonusPct: 30,
      winterRestEnergyBonusPct: 50,
    },
    recipes,
    jobs,
    magicSpells: spells,
    dailyMissions: dailyMissions.length > 0 ? dailyMissions : DEFAULT_DAILY_MISSIONS,
    shop: parsedShop,
    socialActions: {
      treatSendHungerGain: 25,
      treatSendHappinessGain: 20,
      treatSendXpReward: 15,
      coopQuestCoinsReward: 35,
      coopQuestXpReward: 40,
      coopMagicHpBoost: 35,
      coopMagicHappinessBoost: 25,
    },
  };
}

export function parseGameRules(cfgText: string): GameRulesConfig {
  const trimmed = cfgText.trim();
  if (trimmed.startsWith('{')) {
    try {
      const parsedJson = JSON.parse(trimmed);
      return parseJsonGameRules(parsedJson);
    } catch (err) {
      console.warn('[ConfigParser] JSON parsing failed, attempting INI parse:', err);
    }
  }

  const lines = cfgText.split('\n');
  let currentSection = 'general';

  const rawSections: Record<string, Record<string, string>> = {};

  for (let rawLine of lines) {
    let line = rawLine.trim();
    if (!line || line.startsWith('#') || line.startsWith(';')) continue;

    // Section header [section]
    if (line.startsWith('[') && line.endsWith(']')) {
      currentSection = line.slice(1, -1).trim().toLowerCase();
      if (!rawSections[currentSection]) {
        rawSections[currentSection] = {};
      }
      continue;
    }

    // Key = Value
    const eqIdx = line.indexOf('=');
    if (eqIdx !== -1) {
      const key = line.slice(0, eqIdx).trim();
      let val = line.slice(eqIdx + 1).trim();
      // Remove surrounding quotes
      if ((val.startsWith('"') && val.endsWith('"')) || (val.startsWith("'") && val.endsWith("'"))) {
        val = val.slice(1, -1);
      }
      if (!rawSections[currentSection]) {
        rawSections[currentSection] = {};
      }
      rawSections[currentSection][key] = val;
    }
  }

  const getNum = (sec: string, key: string, fallback: number): number => {
    const v = rawSections[sec]?.[key];
    if (v === undefined || v === '') return fallback;
    const n = parseFloat(v);
    return isNaN(n) ? fallback : n;
  };

  const getStr = (sec: string, key: string, fallback: string): string => {
    return rawSections[sec]?.[key] ?? fallback;
  };

  // Parse pipe-delimited recipes (with full bilingual support: English & Deutsch)
  const parsedRecipes: ParsedRecipeRule[] = [];
  const recipeSec = rawSections['recipes'] || {};
  for (const [id, valStr] of Object.entries(recipeSec)) {
    const parts = valStr.split('|').map((p) => p.trim());
    const name = parts[0] || id;
    let name_de = '';
    let description = '';
    let description_de = '';
    let minLevel = 1;
    let hungerGain = 20;
    let healthGain = 0;
    let energyGain = 0;
    let manaGain = 0;
    let xpReward = 30;
    let icon = '🍲';
    const ingredients: { itemId: string; count: number; name?: string; name_de?: string }[] = [];

    for (let i = 1; i < parts.length; i++) {
      const part = parts[i];
      if (part.startsWith('de:') || part.startsWith('name_de:')) {
        name_de = part.replace(/^(de|name_de):/, '').trim();
      } else if (part.startsWith('desc:') || part.startsWith('description:')) {
        description = part.replace(/^(desc|description):/, '').trim();
      } else if (part.startsWith('desc_de:') || part.startsWith('description_de:')) {
        description_de = part.replace(/^(desc_de|description_de):/, '').trim();
      } else {
        const [k, v] = part.split(':').map((s) => s.trim());
        if (k === 'min_level') minLevel = parseInt(v) || 1;
        else if (k === 'hunger') hungerGain = parseInt(v) || 0;
        else if (k === 'hp') healthGain = parseInt(v) || 0;
        else if (k === 'energy') energyGain = parseInt(v) || 0;
        else if (k === 'mana') manaGain = parseInt(v) || 0;
        else if (k === 'xp') xpReward = parseInt(v) || 0;
        else if (k === 'icon') icon = v || '🍲';
        else if (k === 'ingredients' && v) {
          const itemPairs = v.split(',');
          for (const pair of itemPairs) {
            const [itemId, countStr] = pair.split(':').map((s) => s.trim());
            if (itemId) ingredients.push({ itemId, count: parseInt(countStr) || 1 });
          }
        }
      }
    }

    parsedRecipes.push({
      id,
      name,
      name_de: name_de || name,
      description,
      description_de,
      minLevel,
      ingredients,
      hungerGain,
      healthGain,
      energyGain,
      manaGain,
      xpReward,
      icon,
    });
  }

  // Parse pipe-delimited jobs
  const parsedJobs: ParsedJobRule[] = [];
  const jobSec = rawSections['jobs'] || {};
  for (const [id, valStr] of Object.entries(jobSec)) {
    const parts = valStr.split('|').map((p) => p.trim());
    const title = parts[0] || id;
    let minLevel = 1;
    let durationSeconds = 20;
    let coinReward = 25;
    let xpReward = 40;
    let drop = { itemId: 'mat_herb', chance: 0.8 };
    let icon = '🧺';

    for (let i = 1; i < parts.length; i++) {
      const [k, v] = parts[i].split(':').map((s) => s.trim());
      if (k === 'min_level') minLevel = parseInt(v) || 1;
      else if (k === 'duration') durationSeconds = parseInt(v) || 20;
      else if (k === 'coins') coinReward = parseInt(v) || 10;
      else if (k === 'xp') xpReward = parseInt(v) || 20;
      else if (k === 'icon') icon = v || '🧺';
      else if (k === 'drop') {
        const dropParts = parts[i].replace('drop:', '').split(':').map((s) => s.trim());
        if (dropParts.length >= 2) {
          drop = { itemId: dropParts[0], chance: parseFloat(dropParts[1]) || 0.5 };
        }
      }
    }

    parsedJobs.push({
      id,
      title,
      minLevel,
      durationSeconds,
      coinReward,
      xpReward,
      drop,
      icon,
    });
  }

  // Parse pipe-delimited spells
  const parsedSpells: ParsedSpellRule[] = [];
  const spellSec = rawSections['magic_spells'] || {};
  for (const [id, valStr] of Object.entries(spellSec)) {
    const parts = valStr.split('|').map((p) => p.trim());
    const name = parts[0] || id;
    let minLevel = 1;
    let manaCost = 20;
    let target: 'self' | 'friend' | 'both' = 'self';
    let healthBonus = 0;
    let happinessBonus = 0;
    let energyBonus = 0;
    let hygieneBonus = 0;
    let biome = 'grassland';
    let icon = '✨';

    for (let i = 1; i < parts.length; i++) {
      const [k, v] = parts[i].split(':').map((s) => s.trim());
      if (k === 'min_level') minLevel = parseInt(v) || 1;
      else if (k === 'mana') manaCost = parseInt(v) || 20;
      else if (k === 'target') target = (v as any) || 'self';
      else if (k === 'hp') healthBonus = parseInt(v) || 0;
      else if (k === 'happy') happinessBonus = parseInt(v) || 0;
      else if (k === 'energy') energyBonus = parseInt(v) || 0;
      else if (k === 'hygiene') hygieneBonus = parseInt(v) || 0;
      else if (k === 'biome') biome = v || 'grassland';
      else if (k === 'icon') icon = v || '✨';
    }

    parsedSpells.push({
      id,
      name,
      minLevel,
      manaCost,
      target,
      healthBonus,
      happinessBonus,
      energyBonus,
      hygieneBonus,
      biome,
      icon,
    });
  }

  // Biomes parsing
  const biomes: GameRulesConfig['biomes'] = {};
  const biomeKeys = ['desert', 'grassland', 'water', 'forest'];
  for (const b of biomeKeys) {
    const secName = `biome.${b}`;
    biomes[b] = {
      name: getStr(secName, 'name', b.toUpperCase()),
      colorHex: getStr(secName, 'color_hex', '#10b981'),
      rgb565: getStr(secName, 'rgb565', '0x2E69'),
      trait: getStr(secName, 'trait', 'Resilience'),
      description: getStr(secName, 'description', ''),
      growthRequirement: getStr(secName, 'growth_requirement', ''),
      energyBonusPct: getNum(secName, 'energy_bonus_pct', 0),
      hungerDecayBonusPct: getNum(secName, 'hunger_decay_bonus_pct', 0),
    };
  }

  return {
    general: {
      gameTitle: getStr('general', 'game_title', 'CYD Tamagotchi RPG'),
      version: getStr('general', 'version', '2.0.0'),
      maxStatValue: getNum('general', 'max_stat_value', 100),
      tickRateSeconds: getNum('general', 'tick_rate_seconds', 6.0),
      socketPort: getNum('general', 'socket_port', 8888),
      baseXpRequirement: getNum('general', 'base_xp_requirement', 100),
      xpLevelMultiplier: getNum('general', 'xp_level_multiplier', 1.3),
    },
    decayRates: {
      hungerDecay: getNum('decay_rates', 'hunger_decay', 1),
      energyDecay: getNum('decay_rates', 'energy_decay', 1),
      hygieneDecay: getNum('decay_rates', 'hygiene_decay', 0),
      happinessDecay: getNum('decay_rates', 'happiness_decay', 1),
      healthStarveDecay: getNum('decay_rates', 'health_starve_decay', 1),
      starveThreshold: getNum('decay_rates', 'starve_threshold', 20),
      naturalManaRegen: getNum('decay_rates', 'natural_mana_regen', 2),
    },
    evolutionPhases: {
      eggWarmthRequired: getNum('evolution_phases', 'egg_warmth_required', 100),
      babyMinLevel: getNum('evolution_phases', 'baby_min_level', 1),
      youthMinLevel: getNum('evolution_phases', 'youth_min_level', 4),
      adultMinLevel: getNum('evolution_phases', 'adult_min_level', 8),
      elderMinLevel: getNum('evolution_phases', 'elder_min_level', 15),
    },
    biomes,
    seasons: {
      springForageBonusPct: getNum('seasons', 'spring_forage_bonus_pct', 20),
      summerThirstDecayPct: getNum('seasons', 'summer_thirst_decay_pct', 25),
      autumnCookXpBonusPct: getNum('seasons', 'autumn_cook_xp_bonus_pct', 50),
      winterRestEnergyBonusPct: getNum('seasons', 'winter_rest_energy_bonus_pct', 100),
    },
    recipes: parsedRecipes,
    jobs: parsedJobs,
    magicSpells: parsedSpells,
    dailyMissions: DEFAULT_DAILY_MISSIONS,
    shop: DEFAULT_SHOP_CONFIG,
    socialActions: {
      treatSendHungerGain: getNum('social_actions', 'treat_send_hunger_gain', 25),
      treatSendHappinessGain: getNum('social_actions', 'treat_send_happiness_gain', 20),
      treatSendXpReward: getNum('social_actions', 'treat_send_xp_reward', 30),
      coopQuestCoinsReward: getNum('social_actions', 'coop_quest_coins_reward', 40),
      coopQuestXpReward: getNum('social_actions', 'coop_quest_xp_reward', 50),
      coopMagicHpBoost: getNum('social_actions', 'coop_magic_hp_boost', 30),
      coopMagicHappinessBoost: getNum('social_actions', 'coop_magic_happiness_boost', 25),
    },
  };
}

/**
 * Converts a parsed recipe rule from the .cfg configuration text into a complete runtime Recipe
 * with full bilingual titles, descriptions, ingredient names, and result item stats.
 */
export function convertParsedRecipeToRecipe(rule: ParsedRecipeRule): Recipe {
  const itemNamesEn: Record<string, string> = {
    mat_herb: 'Meadow Herb',
    mat_meat: 'Raw Meat',
    mat_sunstone: 'Sun Salt',
    mat_mushroom: 'Forest Mushroom',
    mat_kelp: 'Lagoon Kelp',
  };
  const itemNamesDe: Record<string, string> = {
    mat_herb: 'Wiesenkräuter',
    mat_meat: 'Rohes Fleisch',
    mat_sunstone: 'Sonnensalz',
    mat_mushroom: 'Waldpilz',
    mat_kelp: 'Lagunentang',
  };

  const ingredients = rule.ingredients.map((ing) => ({
    itemId: ing.itemId,
    count: ing.count,
    name: ing.name || itemNamesEn[ing.itemId] || ing.itemId.replace('mat_', '').replace('_', ' '),
    name_de: ing.name_de || itemNamesDe[ing.itemId] || ing.name || ing.itemId.replace('mat_', '').replace('_', ' '),
  }));

  const descEn = rule.description || `Restores ${rule.hungerGain || 0} Hunger${rule.healthGain ? ` & ${rule.healthGain} Health` : ''}.`;
  const descDe = rule.description_de || `Stellt ${rule.hungerGain || 0} Hunger${rule.healthGain ? ` & ${rule.healthGain} Leben` : ''} wieder her.`;

  return {
    id: rule.id,
    name: rule.name,
    name_de: rule.name_de || rule.name,
    description: descEn,
    description_de: descDe,
    icon: rule.icon,
    minLevel: rule.minLevel,
    ingredients,
    resultItem: {
      id: `food_${rule.id}`,
      name: rule.name,
      name_de: rule.name_de || rule.name,
      type: 'food',
      rarity: rule.minLevel >= 7 ? 'legendary' : rule.minLevel >= 4 ? 'rare' : rule.minLevel >= 2 ? 'uncommon' : 'common',
      description: descEn,
      description_de: descDe,
      icon: rule.icon,
      quantity: 1,
      hungerGain: rule.hungerGain,
      healthGain: rule.healthGain,
      energyGain: rule.energyGain,
      happinessGain: 15,
    },
    xpReward: rule.xpReward,
  };
}
