import {
  BiomeType,
  SeasonType,
  EvolutionPhase,
  PetStats,
  PetState,
  Recipe,
  Job,
  MagicSpell,
  InventoryItem,
} from '../types/pet';

export const BIOMES: Record<
  BiomeType,
  {
    name: string;
    label: string;
    color: string;
    bgClass: string;
    borderClass: string;
    textClass: string;
    primaryHex: string;
    accentHex: string;
    description: string;
    defaultPetName: string;
    growthRequirement: string;
    specialTrait: string;
  }
> = {
  desert: {
    name: 'Desert',
    label: 'Desert (Yellow)',
    color: 'amber',
    bgClass: 'bg-amber-950/80',
    borderClass: 'border-amber-500',
    textClass: 'text-amber-400',
    primaryHex: '#f59e0b',
    accentHex: '#d97706',
    description: 'Scorching dunes and radiant heat. Pets develop tough scales and solar stamina.',
    defaultPetName: 'Sunscale',
    growthRequirement: 'Needs regular hydration & Sunstones to flourish.',
    specialTrait: '+25% Energy generation in sunshine, thirsty nature.',
  },
  grassland: {
    name: 'Grassland',
    label: 'Grassland (Green)',
    color: 'emerald',
    bgClass: 'bg-emerald-950/80',
    borderClass: 'border-emerald-500',
    textClass: 'text-emerald-400',
    primaryHex: '#10b981',
    accentHex: '#059669',
    description: 'Lush green rolling meadows. Gentle breezes, abundant herbs and fast recovery.',
    defaultPetName: 'Leafkin',
    growthRequirement: 'Needs frequent play and fresh vegetable snacks.',
    specialTrait: 'Balanced metabolism, double happiness from outdoor games.',
  },
  water: {
    name: 'Water',
    label: 'Water (Blue)',
    color: 'cyan',
    bgClass: 'bg-cyan-950/80',
    borderClass: 'border-cyan-500',
    textClass: 'text-cyan-400',
    primaryHex: '#06b6d4',
    accentHex: '#0284c7',
    description: 'Crystal lagoons and deep coral reefs. Agile aquatic pets with tidal affinity.',
    defaultPetName: 'Bubbles',
    growthRequirement: 'Needs constant hygiene / bubble baths to keep gills clean.',
    specialTrait: 'Natural water healing spells, never dehydrates.',
  },
  forest: {
    name: 'Forest',
    label: 'Forest (Brown/Wood)',
    color: 'stone',
    bgClass: 'bg-stone-900/90',
    borderClass: 'border-amber-700',
    textClass: 'text-amber-600',
    primaryHex: '#8d6e63',
    accentHex: '#5d4037',
    description: 'Ancient whispering canopy with mossy roots, mushrooms and rich natural lore.',
    defaultPetName: 'Bramble',
    growthRequirement: 'Thrives on hearty mushroom stews and shaded rest.',
    specialTrait: 'Superior foraging drop rates for rare woods & herbs.',
  },
};

export const SEASONS: Record<
  SeasonType,
  {
    name: string;
    icon: string;
    color: string;
    description: string;
    resourceBonus: string;
  }
> = {
  spring: {
    name: 'Spring',
    icon: '🌸',
    color: 'text-pink-400',
    description: 'Gentle warmth and blooming flowers. Wildlife is active.',
    resourceBonus: '+20% Foraging drops and extra happiness from treats.',
  },
  summer: {
    name: 'Summer',
    icon: '☀️',
    color: 'text-amber-400',
    description: 'High solar heat. Desert pets thrive while water pets need baths.',
    resourceBonus: 'Rare Sunstones gatherable; hunger decays 15% faster.',
  },
  autumn: {
    name: 'Autumn',
    icon: '🍂',
    color: 'text-orange-400',
    description: 'Golden harvests and crisp breezes. Perfect for hearty cooking.',
    resourceBonus: 'Harvest festival: +50% XP from cooking recipes.',
  },
  winter: {
    name: 'Winter',
    icon: '❄️',
    color: 'text-sky-300',
    description: 'Cold frost blankets the land. Pets need warmth and cozy meals.',
    resourceBonus: 'Rare Frost Crystals found; resting restores double energy.',
  },
};

export const EVOLUTION_DATA: Record<
  EvolutionPhase,
  {
    minLevel: number;
    title: string;
    description: string;
    spriteScale: number;
  }
> = {
  Egg: {
    minLevel: 0,
    title: 'Mystic Egg',
    description: 'Incubating. Keep warm and tap to speed up hatching!',
    spriteScale: 0.8,
  },
  Baby: {
    minLevel: 1,
    title: 'Playful Baby',
    description: 'Curious and hungry. Needs milk, naps, and gentle cuddles.',
    spriteScale: 0.9,
  },
  Youth: {
    minLevel: 4,
    title: 'Adventurous Youth',
    description: 'Energetic learner. Unlocks basic foraging jobs and simple recipes.',
    spriteScale: 1.0,
  },
  Adult: {
    minLevel: 8,
    title: 'Mighty Adult',
    description: 'Full RPG prowess! Master magic spells, rare crafting and trading.',
    spriteScale: 1.15,
  },
  Elder: {
    minLevel: 15,
    title: 'Ancient Sage',
    description: 'Legendary aura. Grants immense friend blessings and rare relic gifts.',
    spriteScale: 1.25,
  },
};

// RPG Recipes Catalog
export const RECIPES: Recipe[] = [
  {
    id: 'rcp_salad',
    name: 'Meadow Herb Salad',
    name_de: 'Wiesenkräuter-Salat',
    description: 'Fresh crisp greens picked at dawn. Light and wholesome.',
    description_de: 'Frischer, knackiger Klee und aromatische Kräuter. Leicht & bekömmlich.',
    icon: '🥗',
    minLevel: 1,
    ingredients: [{ itemId: 'mat_herb', name: 'Meadow Herb', name_de: 'Wiesenkräuter', count: 2 }],
    resultItem: {
      id: 'food_herb_salad',
      name: 'Meadow Herb Salad',
      name_de: 'Wiesenkräuter-Salat',
      type: 'food',
      rarity: 'common',
      description: 'Restores 25 Hunger & 10 Health.',
      description_de: 'Stellt 25 Hunger & 10 Leben wieder her.',
      icon: '🥗',
      quantity: 1,
      hungerGain: 25,
      healthGain: 10,
      happinessGain: 10,
    },
    xpReward: 35,
  },
  {
    id: 'rcp_jerky',
    name: 'Sun-Dried Jerky',
    name_de: 'Sonnen-Dörrfleisch',
    description: 'Cured under the intense desert sun. Rich in protein.',
    description_de: 'Unter intensiver Wüstensonne getrocknet. Reich an Proteinen.',
    icon: '🥩',
    minLevel: 2,
    ingredients: [
      { itemId: 'mat_meat', name: 'Raw Meat', name_de: 'Rohes Fleisch', count: 2 },
      { itemId: 'mat_sunstone', name: 'Sun Salt', name_de: 'Sonnensalz', count: 1 },
    ],
    resultItem: {
      id: 'food_sun_jerky',
      name: 'Sun-Dried Jerky',
      name_de: 'Sonnen-Dörrfleisch',
      type: 'food',
      rarity: 'uncommon',
      description: 'Restores 45 Hunger & 20 Energy.',
      description_de: 'Stellt 45 Hunger & 20 Energie wieder her.',
      icon: '🥩',
      quantity: 1,
      hungerGain: 45,
      energyGain: 20,
      happinessGain: 15,
    },
    xpReward: 60,
  },
  {
    id: 'rcp_berry_pie',
    name: 'Forest Berry Tart',
    name_de: 'Waldbeeren-Tarte',
    description: 'Sweet wild forest berries baked in a cozy campfire oven.',
    description_de: 'Süße Waldbeeren, duftend gebacken am gemütlichen Lagerfeuer.',
    icon: '🥧',
    minLevel: 3,
    ingredients: [
      { itemId: 'mat_herb', name: 'Meadow Herb', name_de: 'Wiesenkräuter', count: 1 },
      { itemId: 'mat_mushroom', name: 'Forest Mushroom', name_de: 'Waldpilz', count: 1 },
    ],
    resultItem: {
      id: 'food_berry_pie',
      name: 'Forest Berry Tart',
      name_de: 'Waldbeeren-Tarte',
      type: 'food',
      rarity: 'uncommon',
      description: 'Restores 35 Hunger & 25 Happiness.',
      description_de: 'Stellt 35 Hunger & 25 Glück wieder her.',
      icon: '🥧',
      quantity: 1,
      hungerGain: 35,
      happinessGain: 25,
      healthGain: 15,
    },
    xpReward: 80,
  },
  {
    id: 'rcp_stew',
    name: 'Kelp & Mushroom Stew',
    name_de: 'Tang- & Pilzeintopf',
    description: 'A steaming pot infused with forest truffles and sea minerals.',
    description_de: 'Dampfender Eintopf verfeinert mit Waldpilzen und Meeresmineralien.',
    icon: '🍲',
    minLevel: 4,
    ingredients: [
      { itemId: 'mat_mushroom', name: 'Forest Mushroom', name_de: 'Waldpilz', count: 2 },
      { itemId: 'mat_kelp', name: 'Lagoon Kelp', name_de: 'Lagunentang', count: 2 },
    ],
    resultItem: {
      id: 'food_kelp_stew',
      name: 'Kelp & Mushroom Stew',
      name_de: 'Tang- & Pilzeintopf',
      type: 'food',
      rarity: 'rare',
      description: 'Restores 60 Hunger, 30 Health & 25 Mana.',
      description_de: 'Stellt 60 Hunger, 30 Leben & 25 Mana wieder her.',
      icon: '🍲',
      quantity: 1,
      hungerGain: 60,
      healthGain: 30,
      happinessGain: 30,
    },
    xpReward: 110,
  },
  {
    id: 'rcp_ambrosia',
    name: 'Elemental Ambrosia',
    name_de: 'Elementar-Ambrosia',
    description: 'A shimmering dessert blessed with all four biome essences.',
    description_de: 'Ein schimmerndes Festmahl, gesegnet mit den Essenzen aller 4 Biome.',
    icon: '🧁',
    minLevel: 7,
    ingredients: [
      { itemId: 'mat_sunstone', name: 'Sun Salt', name_de: 'Sonnensalz', count: 2 },
      { itemId: 'mat_kelp', name: 'Lagoon Kelp', name_de: 'Lagunentang', count: 2 },
      { itemId: 'mat_mushroom', name: 'Forest Mushroom', name_de: 'Waldpilz', count: 2 },
      { itemId: 'mat_herb', name: 'Meadow Herb', name_de: 'Wiesenkräuter', count: 2 },
    ],
    resultItem: {
      id: 'food_ambrosia',
      name: 'Elemental Ambrosia',
      name_de: 'Elementar-Ambrosia',
      type: 'food',
      rarity: 'legendary',
      description: 'Fully restores All Stats + 50 Mana!',
      description_de: 'Stellt alle Werte vollständig wieder her + 50 Mana!',
      icon: '🧁',
      quantity: 1,
      hungerGain: 100,
      healthGain: 100,
      energyGain: 100,
      happinessGain: 100,
    },
    xpReward: 250,
  },
];

// RPG Jobs Catalog
export const JOBS: Job[] = [
  {
    id: 'job_forage',
    title: 'Herb & Berry Forager',
    description: 'Scour local thickets for wild herbs and cooking ingredients.',
    icon: '🧺',
    minLevel: 1,
    durationSeconds: 15,
    coinReward: 15,
    xpReward: 25,
    possibleDrop: { itemId: 'mat_herb', name: 'Meadow Herb', chance: 0.9 },
  },
  {
    id: 'job_patrol',
    title: 'CYD Village Scout',
    description: 'Keep perimeter paths clear of mischievous pixel critters.',
    icon: '🛡️',
    minLevel: 2,
    durationSeconds: 25,
    coinReward: 30,
    xpReward: 50,
    possibleDrop: { itemId: 'mat_meat', name: 'Raw Meat', chance: 0.7 },
  },
  {
    id: 'job_mine',
    title: 'Biome Crystal Miner',
    description: 'Delve into subterranean veins for glowing geodes and minerals.',
    icon: '⛏️',
    minLevel: 4,
    durationSeconds: 40,
    coinReward: 55,
    xpReward: 90,
    possibleDrop: { itemId: 'mat_sunstone', name: 'Sun Salt', chance: 0.6 },
  },
  {
    id: 'job_alchemist',
    title: 'Royal CYD Alchemist',
    description: 'Brew mystical draughts and enchant accessories for friends.',
    icon: '⚗️',
    minLevel: 6,
    durationSeconds: 60,
    coinReward: 90,
    xpReward: 150,
    possibleDrop: { itemId: 'mat_kelp', name: 'Lagoon Kelp', chance: 0.65 },
  },
];

// RPG Magic Spells Catalog
export const SPELLS: MagicSpell[] = [
  {
    id: 'spl_heal',
    name: 'Vitality Aura',
    description: 'Channels primal life energy to heal your pet and restore 25 HP.',
    icon: '✨',
    minLevel: 2,
    manaCost: 20,
    biome: 'grassland',
    target: 'self',
    effectDescription: '+25 Health to Self',
  },
  {
    id: 'spl_treat_burst',
    name: 'Astral Feast',
    description: 'Conjures a spectral mana-berry that satisfies your pet.',
    icon: '🍓',
    minLevel: 3,
    manaCost: 25,
    biome: 'desert',
    target: 'self',
    effectDescription: '+35 Hunger to Self',
  },
  {
    id: 'spl_friend_bless',
    name: 'Friend Resonance',
    description: 'Beams a surge of pure friendship across the socket to your buddy!',
    icon: '💖',
    minLevel: 4,
    manaCost: 35,
    biome: 'water',
    target: 'friend',
    effectDescription: 'Sends +30 Happiness & +20 Energy to Friend',
  },
  {
    id: 'spl_rain_dance',
    name: 'Biome Harmony',
    description: 'Summons an elemental shower restoring both pets simultaneously!',
    icon: '🌈',
    minLevel: 6,
    manaCost: 50,
    biome: 'forest',
    target: 'both',
    effectDescription: '+40 Health & +30 Cleanliness to Both Pets!',
  },
];

// Rare Accessories & Decorations for Trading
export const CATALOG_ITEMS: InventoryItem[] = [
  // Accessories
  {
    id: 'acc_tophat',
    name: 'Fancy Top Hat',
    type: 'accessory',
    slot: 'head',
    rarity: 'rare',
    description: 'A distinguished velvet top hat for the refined CYD companion.',
    icon: '🎩',
    quantity: 1,
    happinessGain: 15,
  },
  {
    id: 'acc_wizard',
    name: 'Starry Wizard Cowl',
    type: 'accessory',
    slot: 'head',
    rarity: 'rare',
    description: 'Infused with starlight; boosts spell casting confidence.',
    icon: '🧙',
    quantity: 1,
    happinessGain: 20,
  },
  {
    id: 'acc_flower',
    name: 'Blossom Crown',
    type: 'accessory',
    slot: 'head',
    rarity: 'uncommon',
    description: 'Hand-woven fresh daisies picked in the spring grassland.',
    icon: '👑',
    quantity: 1,
    happinessGain: 10,
  },
  {
    id: 'acc_shades',
    name: 'Pixel Aviator Shades',
    type: 'accessory',
    slot: 'face',
    rarity: 'rare',
    description: '100% UV-proof cool factor for bright desert treks.',
    icon: '🕶️',
    quantity: 1,
    happinessGain: 15,
  },
  {
    id: 'acc_cape',
    name: 'Heroic Crimson Cape',
    type: 'accessory',
    slot: 'back',
    rarity: 'legendary',
    description: 'Flutters gloriously in the wind during shared adventures.',
    icon: '🦸',
    quantity: 1,
    happinessGain: 25,
  },
  {
    id: 'acc_wings',
    name: 'Fairy Sparkle Wings',
    type: 'accessory',
    slot: 'back',
    rarity: 'legendary',
    description: 'Ethereal wings that shimmer with every bounce.',
    icon: '🪽',
    quantity: 1,
    happinessGain: 30,
  },

  // Environment Decorations
  {
    id: 'dec_campfire',
    name: 'Cozy Crackling Campfire',
    type: 'decoration',
    slot: 'environment',
    rarity: 'uncommon',
    description: 'Warms cold winter nights and illuminates the CYD screen.',
    icon: '🔥',
    quantity: 1,
  },
  {
    id: 'dec_bonsai',
    name: 'Ancient Mystic Bonsai',
    type: 'decoration',
    slot: 'environment',
    rarity: 'rare',
    description: 'A 200-year-old miniature tree promoting Zen happiness.',
    icon: '🪴',
    quantity: 1,
  },
  {
    id: 'dec_arcade',
    name: 'Retro Mini Arcade',
    type: 'decoration',
    slot: 'environment',
    rarity: 'legendary',
    description: 'A tiny 8-bit cabinet playing CYD synth melodies.',
    icon: '🕹️',
    quantity: 1,
  },
  {
    id: 'dec_cactus',
    name: 'Flowering Saguaro',
    type: 'decoration',
    slot: 'environment',
    rarity: 'common',
    description: 'A friendly prickly resident from the sunny dunes.',
    icon: '🌵',
    quantity: 1,
  },
];

export function getXPForNextLevel(level: number): number {
  return Math.floor(100 * Math.pow(1.3, level - 1));
}

export function checkEvolution(level: number, currentPhase: EvolutionPhase): EvolutionPhase {
  if (currentPhase === 'Egg') return 'Egg'; // Must hatch manually
  if (level >= 15) return 'Elder';
  if (level >= 8) return 'Adult';
  if (level >= 4) return 'Youth';
  return 'Baby';
}

export function createInitialPet(
  deviceId = 'CYD-DEV1',
  trainerName = 'Trainer',
  biome: BiomeType = 'grassland'
): PetState {
  const biomeInfo = BIOMES[biome];
  return {
    id: `pet-${Date.now()}`,
    deviceId,
    name: biomeInfo.defaultPetName,
    trainerName,
    biome,
    season: 'spring',
    phase: 'Egg',
    level: 1,
    xp: 0,
    xpNext: 100,
    stats: {
      health: 100,
      hunger: 90,
      energy: 95,
      hygiene: 100,
      happiness: 85,
      mana: 50,
    },
    coins: 45,
    inventory: [
      {
        id: 'food_apple',
        name: 'Crisp Apple',
        type: 'food',
        rarity: 'common',
        description: 'Restores 15 Hunger.',
        icon: '🍎',
        quantity: 4,
        hungerGain: 15,
        happinessGain: 5,
      },
      {
        id: 'mat_herb',
        name: 'Meadow Herb',
        type: 'material',
        rarity: 'common',
        description: 'Fragrant herb for cooking & remedies.',
        icon: '🌿',
        quantity: 3,
      },
      {
        id: 'acc_flower',
        name: 'Blossom Crown',
        type: 'accessory',
        slot: 'head',
        rarity: 'uncommon',
        description: 'Hand-woven fresh daisies picked in the spring grassland.',
        icon: '👑',
        quantity: 1,
      },
    ],
    equipped: {
      head: 'acc_flower',
    },
    activeJob: null,
    recipesCraftedCount: 0,
    jobsCompletedCount: 0,
    spellsCastCount: 0,
    treatsSentCount: 0,
    poopCount: 0,
    isSleeping: false,
    eggWarmth: 35,
    createdAt: Date.now(),
    lastFedAt: Date.now(),
    lastInteractedAt: Date.now(),
  };
}

export function createInitialFriendPet(
  deviceId = 'CYD-FRIEND2',
  biome: BiomeType = 'desert'
): PetState {
  const biomeInfo = BIOMES[biome];
  return {
    id: `pet-friend-${Date.now()}`,
    deviceId,
    name: biomeInfo.defaultPetName,
    trainerName: 'FriendPixel',
    biome,
    season: 'spring',
    phase: 'Baby',
    level: 2,
    xp: 40,
    xpNext: 130,
    stats: {
      health: 95,
      hunger: 80,
      energy: 85,
      hygiene: 90,
      happiness: 90,
      mana: 60,
    },
    coins: 70,
    inventory: [
      {
        id: 'mat_sunstone',
        name: 'Sun Salt',
        type: 'material',
        rarity: 'uncommon',
        description: 'Warm glowing crystals from desert sands.',
        icon: '🧂',
        quantity: 4,
      },
      {
        id: 'acc_shades',
        name: 'Pixel Aviator Shades',
        type: 'accessory',
        slot: 'face',
        rarity: 'rare',
        description: 'Cool shades for bright sunny walks.',
        icon: '🕶️',
        quantity: 1,
      },
    ],
    equipped: {
      face: 'acc_shades',
    },
    activeJob: null,
    recipesCraftedCount: 1,
    jobsCompletedCount: 2,
    spellsCastCount: 1,
    treatsSentCount: 3,
    poopCount: 0,
    isSleeping: false,
    eggWarmth: 100,
    createdAt: Date.now() - 3600000 * 24,
    lastFedAt: Date.now() - 1800000,
    lastInteractedAt: Date.now() - 600000,
  };
}
