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

export function parseGameRules(cfgText: string): GameRulesConfig {
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
