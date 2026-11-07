export type BiomeType = 'desert' | 'grassland' | 'water' | 'forest';

export type SeasonType = 'spring' | 'summer' | 'autumn' | 'winter';

export type EvolutionPhase = 'Egg' | 'Baby' | 'Youth' | 'Adult' | 'Elder';

export interface PetStats {
  health: number; // 0 - 100
  hunger: number; // 0 - 100 (100 = full)
  energy: number; // 0 - 100
  hygiene: number; // 0 - 100
  happiness: number; // 0 - 100
  mana: number; // 0 - 100
}

export interface InventoryItem {
  id: string;
  name: string;
  name_de?: string;
  type: 'food' | 'material' | 'accessory' | 'decoration' | 'potion';
  rarity: 'common' | 'uncommon' | 'rare' | 'legendary';
  description: string;
  description_de?: string;
  icon: string;
  quantity: number;
  biomeSource?: BiomeType;
  slot?: 'head' | 'face' | 'back' | 'environment';
  hungerGain?: number;
  healthGain?: number;
  energyGain?: number;
  happinessGain?: number;
}

export interface EquippedItems {
  head?: string;
  face?: string;
  back?: string;
  environment?: string;
}

export interface Recipe {
  id: string;
  name: string;
  name_de?: string;
  description: string;
  description_de?: string;
  icon: string;
  minLevel: number;
  ingredients: { itemId: string; name: string; name_de?: string; count: number }[];
  resultItem: InventoryItem;
  xpReward: number;
}

export interface Job {
  id: string;
  title: string;
  description: string;
  icon: string;
  minLevel: number;
  durationSeconds: number;
  coinReward: number;
  xpReward: number;
  possibleDrop: { itemId: string; name: string; chance: number };
}

export interface MagicSpell {
  id: string;
  name: string;
  description: string;
  icon: string;
  minLevel: number;
  manaCost: number;
  biome: BiomeType;
  target: 'self' | 'friend' | 'both';
  effectDescription: string;
}

export interface PetState {
  id: string;
  deviceId: string;
  name: string;
  trainerName: string;
  biome: BiomeType;
  season: SeasonType;
  phase: EvolutionPhase;
  level: number;
  xp: number;
  xpNext: number;
  stats: PetStats;
  coins: number;
  inventory: InventoryItem[];
  equipped: EquippedItems;
  activeJob: {
    jobId: string;
    startedAt: number;
    durationSeconds: number;
  } | null;
  recipesCraftedCount: number;
  jobsCompletedCount: number;
  spellsCastCount: number;
  treatsSentCount: number;
  poopCount: number;
  isSleeping: boolean;
  eggWarmth: number; // For egg hatching (0-100)
  createdAt: number;
  lastFedAt: number;
  lastInteractedAt: number;
}

export interface FriendPetSummary {
  deviceId: string;
  deviceName: string;
  petName: string;
  biome: BiomeType;
  phase: EvolutionPhase;
  level: number;
  stats: {
    health: number;
    hunger: number;
    energy: number;
    happiness: number;
  };
  equipped: EquippedItems;
  isOnline: boolean;
  ipAddress?: string;
  port?: number;
  lastMessage?: string;
  lastMessageTime?: number;
  lastInteractionType?: string;
}

export interface TradeOffer {
  id: string;
  fromDeviceId: string;
  fromPetName: string;
  toDeviceId: string;
  offeredItem: InventoryItem;
  requestedItemName: string;
  status: 'pending' | 'accepted' | 'declined';
  timestamp: number;
}

export interface LeaderboardEntry {
  id: string;
  deviceId: string;
  petName: string;
  trainerName: string;
  biome: BiomeType;
  phase: EvolutionPhase;
  level: number;
  xp: number;
  recipesCrafted: number;
  jobsCompleted: number;
  spellsCast: number;
  friendTreatsSent: number;
  updatedAt: number;
}

export interface CYDMessagePacket {
  id: string;
  fromDeviceId: string;
  toDeviceId: string;
  type: 'TREAT' | 'CHAT' | 'MAGIC' | 'TRADE_OFFER' | 'TRADE_RESPONSE' | 'STATUS_SYNC';
  payload: any;
  timestamp: number;
}
