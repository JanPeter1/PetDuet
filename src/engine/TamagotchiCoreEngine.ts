/**
 * ============================================================================
 * TamagotchiCoreEngine.ts
 * 100% Decoupled, UI-Agnostic, Screen-Agnostic Core Game Runtime Engine
 * ============================================================================
 * Runs on Web, React, Node.js, and can be ported 1:1 to Android (Kotlin) or C++.
 * Receives human-readable rules configuration at runtime and manages simulation logic.
 */

import {
  PetState,
  BiomeType,
  EvolutionPhase,
  CYDMessagePacket,
  DailyMission,
  DailyMissionsState,
  MissionActionType,
} from '../types/pet';
import { GameRulesConfig, parseGameRules } from './ConfigParser';
import defaultRulesJson from './config/rules.json?raw';
import fallbackRulesCfg from './config/tamagotchi_rules.cfg?raw';

export type EngineEvent =
  | 'stat_change'
  | 'level_up'
  | 'evolution'
  | 'egg_hatch'
  | 'job_complete'
  | 'packet_out'
  | 'notification'
  | 'mission_progress'
  | 'mission_complete';

export type EventListener = (eventName: EngineEvent, data: any) => void;

export class TamagotchiCoreEngine {
  private rules: GameRulesConfig;
  private state: PetState;
  private rawConfigText: string;
  private listeners: EventListener[] = [];
  private tickTimer: any = null;

  constructor(initialConfigText?: string, initialBiome: BiomeType = 'grassland') {
    this.rawConfigText = initialConfigText || defaultRulesJson || fallbackRulesCfg;
    this.rules = parseGameRules(this.rawConfigText);
    this.state = this.buildInitialState(initialBiome);
    this.checkDailyMissionsLogin();
  }

  // --------------------------------------------------------------------------
  // Configuration Management (Runtime Loading & Interpretation)
  // --------------------------------------------------------------------------
  public loadConfigText(newConfigText: string): boolean {
    try {
      const parsed = parseGameRules(newConfigText);
      this.rules = parsed;
      this.rawConfigText = newConfigText;
      this.emit('notification', { message: 'Game rules updated dynamically at runtime!' });
      return true;
    } catch (e: any) {
      console.error('[Engine] Failed to parse config text:', e);
      return false;
    }
  }

  public getRawConfigText(): string {
    return this.rawConfigText;
  }

  public getRules(): GameRulesConfig {
    return this.rules;
  }

  // --------------------------------------------------------------------------
  // Event Emitter (Decoupled Platform Notification)
  // --------------------------------------------------------------------------
  public on(listener: EventListener): () => void {
    this.listeners.push(listener);
    return () => {
      this.listeners = this.listeners.filter((l) => l !== listener);
    };
  }

  private emit(event: EngineEvent, data: any) {
    for (const listener of this.listeners) {
      listener(event, data);
    }
  }

  // --------------------------------------------------------------------------
  // State Initialization
  // --------------------------------------------------------------------------
  private buildInitialState(biome: BiomeType): PetState {
    const biomeConfig = this.rules.biomes[biome] || this.rules.biomes.grassland;
    return {
      id: `pet_${Date.now()}`,
      deviceId: 'CYD-DEV1',
      name: biomeConfig?.name ? `${biomeConfig.name}kin` : 'Leafkin',
      trainerName: 'PixelTrainer',
      biome,
      season: 'spring',
      phase: 'Egg',
      level: 1,
      xp: 0,
      xpNext: this.rules.general.baseXpRequirement,
      stats: {
        health: this.rules.general.maxStatValue,
        hunger: 85,
        energy: 90,
        hygiene: 95,
        happiness: 90,
        mana: 50,
      },
      coins: 50,
      inventory: [
        {
          id: 'mat_herb',
          name: 'Meadow Herb',
          type: 'material',
          rarity: 'common',
          description: 'Wild medicinal herb.',
          icon: '🌿',
          quantity: 4,
        },
        {
          id: 'food_apple',
          name: 'Crisp Apple',
          type: 'food',
          rarity: 'common',
          description: 'Restores 15 Hunger.',
          icon: '🍎',
          quantity: 3,
          hungerGain: 15,
        },
      ],
      equipped: {},
      activeJob: null,
      recipesCraftedCount: 0,
      jobsCompletedCount: 0,
      spellsCastCount: 0,
      treatsSentCount: 0,
      poopCount: 0,
      isSleeping: false,
      eggWarmth: 20,
      createdAt: Date.now(),
      lastFedAt: Date.now(),
      lastInteractedAt: Date.now(),
      dailyMissions: this.createInitialDailyMissionsState(1),
    };
  }

  private createInitialDailyMissionsState(consecutiveDays = 1): DailyMissionsState {
    const today = new Date().toISOString().split('T')[0];
    const missions: DailyMission[] = (this.rules.dailyMissions || []).map((m) => {
      const isLoginStreak = m.actionType === 'login_streak';
      const initialCount = isLoginStreak ? consecutiveDays : 0;
      return {
        id: m.id,
        title: m.name,
        title_de: m.name_de,
        description: m.description,
        description_de: m.description_de,
        actionType: m.actionType as MissionActionType,
        targetCount: m.targetCount,
        currentCount: initialCount,
        coinReward: m.coinReward,
        xpReward: m.xpReward,
        completed: initialCount >= m.targetCount,
        claimed: false,
        icon: m.icon,
      };
    });

    return {
      lastLoginDate: today,
      consecutiveLoginDays: consecutiveDays,
      missions,
    };
  }

  public getState(): PetState {
    return this.state;
  }

  public setState(newState: PetState) {
    this.state = newState;
    this.checkDailyMissionsLogin();
    this.emit('stat_change', this.state);
  }

  // --------------------------------------------------------------------------
  // Core Game Loop Tick (Simulation Runtime)
  // --------------------------------------------------------------------------
  public tick(): void {
    if (this.state.phase === 'Egg') return; // Egg doesn't decay

    const decay = this.rules.decayRates;
    const biomeRules = this.rules.biomes[this.state.biome];

    const stats = { ...this.state.stats };

    // Apply Biome Hunger Modifier
    let hungerDecay = decay.hungerDecay;
    if (biomeRules?.hungerDecayBonusPct) {
      hungerDecay += (decay.hungerDecay * biomeRules.hungerDecayBonusPct) / 100;
    }

    if (stats.hunger > 0) stats.hunger = Math.max(0, stats.hunger - hungerDecay);
    if (stats.energy > 0) stats.energy = Math.max(0, stats.energy - decay.energyDecay);
    if (stats.happiness > 0) stats.happiness = Math.max(0, stats.happiness - decay.happinessDecay);
    if (stats.hygiene > 0) stats.hygiene = Math.max(0, stats.hygiene - decay.hygieneDecay);

    // Starvation penalty
    if (stats.hunger < decay.starveThreshold && stats.health > 0) {
      stats.health = Math.max(0, stats.health - decay.healthStarveDecay);
    }

    // Natural Mana Regeneration
    if (stats.mana < this.rules.general.maxStatValue) {
      stats.mana = Math.min(this.rules.general.maxStatValue, stats.mana + decay.naturalManaRegen);
    }

    this.state.stats = stats;

    // Check Active Job Completion
    if (this.state.activeJob) {
      const elapsedSec = (Date.now() - this.state.activeJob.startedAt) / 1000;
      if (elapsedSec >= this.state.activeJob.durationSeconds) {
        this.completeJob();
      }
    }

    this.emit('stat_change', this.state);
  }

  // --------------------------------------------------------------------------
  // Pet Lifecycle & Interactions
  // --------------------------------------------------------------------------
  public feed(hungerAmount = 25, healthAmount = 10): void {
    this.state.stats.hunger = Math.min(
      this.rules.general.maxStatValue,
      this.state.stats.hunger + hungerAmount
    );
    this.state.stats.health = Math.min(
      this.rules.general.maxStatValue,
      this.state.stats.health + healthAmount
    );
    this.addXP(15);
    this.recordAction('feed', 1);
    this.emit('stat_change', this.state);
  }

  public play(happyAmount = 20, energyCost = 10): void {
    this.state.stats.happiness = Math.min(
      this.rules.general.maxStatValue,
      this.state.stats.happiness + happyAmount
    );
    this.state.stats.energy = Math.max(0, this.state.stats.energy - energyCost);
    this.addXP(20);
    this.recordAction('play', 1);
    this.emit('stat_change', this.state);
  }

  public clean(): void {
    this.state.stats.hygiene = this.rules.general.maxStatValue;
    this.state.stats.happiness = Math.min(
      this.rules.general.maxStatValue,
      this.state.stats.happiness + 10
    );
    this.addXP(10);
    this.recordAction('bath', 1);
    this.emit('stat_change', this.state);
  }

  public cuddle(): void {
    this.state.stats.happiness = Math.min(
      this.rules.general.maxStatValue,
      this.state.stats.happiness + 5
    );
    this.emit('stat_change', this.state);
  }

  public warmEgg(amount = 25): boolean {
    if (this.state.phase !== 'Egg') return false;

    this.state.eggWarmth = Math.min(
      this.rules.evolutionPhases.eggWarmthRequired,
      this.state.eggWarmth + amount
    );

    if (this.state.eggWarmth >= this.rules.evolutionPhases.eggWarmthRequired) {
      this.state.phase = 'Baby';
      this.state.stats.health = this.rules.general.maxStatValue;
      this.state.stats.hunger = this.rules.general.maxStatValue;
      this.state.stats.happiness = this.rules.general.maxStatValue;
      this.emit('egg_hatch', this.state);
      this.emit('notification', { message: 'The egg hatched! Baby was born! 🐣' });
    } else {
      this.emit('stat_change', this.state);
    }
    return true;
  }

  // --------------------------------------------------------------------------
  // RPG Progression: XP & Evolution Calculations
  // --------------------------------------------------------------------------
  public addXP(amount: number): void {
    this.state.xp += amount;
    let leveledUp = false;

    while (this.state.xp >= this.state.xpNext) {
      this.state.xp -= this.state.xpNext;
      this.state.level += 1;
      this.state.xpNext = Math.floor(
        this.rules.general.baseXpRequirement *
          Math.pow(this.rules.general.xpLevelMultiplier, this.state.level - 1)
      );
      leveledUp = true;
    }

    if (leveledUp) {
      this.state.stats.health = this.rules.general.maxStatValue;
      this.state.stats.energy = this.rules.general.maxStatValue;
      this.checkEvolutionMilestones();
      this.emit('level_up', { level: this.state.level, pet: this.state });
    } else {
      this.emit('stat_change', this.state);
    }
  }

  private checkEvolutionMilestones(): void {
    if (this.state.phase === 'Egg') return;

    const evo = this.rules.evolutionPhases;
    let nextPhase: EvolutionPhase = this.state.phase;

    if (this.state.level >= evo.elderMinLevel) nextPhase = 'Elder';
    else if (this.state.level >= evo.adultMinLevel) nextPhase = 'Adult';
    else if (this.state.level >= evo.youthMinLevel) nextPhase = 'Youth';
    else nextPhase = 'Baby';

    if (nextPhase !== this.state.phase) {
      this.state.phase = nextPhase;
      this.emit('evolution', { phase: nextPhase, pet: this.state });
    }
  }

  // --------------------------------------------------------------------------
  // RPG Recipes Crafting
  // --------------------------------------------------------------------------
  public craftRecipe(recipeId: string): boolean {
    const recipe = this.rules.recipes.find((r) => r.id === recipeId);
    if (!recipe) return false;
    if (this.state.level < recipe.minLevel) return false;

    // Check ingredients
    for (const req of recipe.ingredients) {
      const invItem = this.state.inventory.find((i) => i.id === req.itemId);
      if (!invItem || invItem.quantity < req.count) return false;
    }

    // Deduct ingredients
    for (const req of recipe.ingredients) {
      const invItem = this.state.inventory.find((i) => i.id === req.itemId)!;
      invItem.quantity -= req.count;
    }
    this.state.inventory = this.state.inventory.filter((i) => i.quantity > 0);

    // Apply stat bonuses
    if (recipe.hungerGain) {
      this.state.stats.hunger = Math.min(
        this.rules.general.maxStatValue,
        this.state.stats.hunger + recipe.hungerGain
      );
    }
    if (recipe.healthGain) {
      this.state.stats.health = Math.min(
        this.rules.general.maxStatValue,
        this.state.stats.health + recipe.healthGain
      );
    }
    if (recipe.energyGain) {
      this.state.stats.energy = Math.min(
        this.rules.general.maxStatValue,
        this.state.stats.energy + recipe.energyGain
      );
    }
    if (recipe.manaGain) {
      this.state.stats.mana = Math.min(
        this.rules.general.maxStatValue,
        this.state.stats.mana + recipe.manaGain
      );
    }

    this.state.recipesCraftedCount += 1;
    this.addXP(recipe.xpReward);
    this.recordAction('cook', 1);
    this.emit('notification', { message: `Cooked ${recipe.name}! ${recipe.icon}` });
    return true;
  }

  // --------------------------------------------------------------------------
  // RPG Jobs & Quests
  // --------------------------------------------------------------------------
  public startJob(jobId: string): boolean {
    if (this.state.activeJob) return false;
    const job = this.rules.jobs.find((j) => j.id === jobId);
    if (!job || this.state.level < job.minLevel) return false;

    this.state.activeJob = {
      jobId: job.id,
      startedAt: Date.now(),
      durationSeconds: job.durationSeconds,
    };

    this.recordAction('start_job', 1);
    this.emit('notification', { message: `Started job: ${job.title}! ${job.icon}` });
    this.emit('stat_change', this.state);
    return true;
  }

  private completeJob(): void {
    if (!this.state.activeJob) return;
    const job = this.rules.jobs.find((j) => j.id === this.state.activeJob!.jobId);
    this.state.activeJob = null;

    if (job) {
      this.state.coins += job.coinReward;
      this.state.jobsCompletedCount += 1;
      this.addXP(job.xpReward);

      // Check item drop
      if (Math.random() <= job.drop.chance) {
        const existing = this.state.inventory.find((i) => i.id === job.drop.itemId);
        if (existing) {
          existing.quantity += 1;
        } else {
          this.state.inventory.push({
            id: job.drop.itemId,
            name: job.drop.itemId.replace('mat_', '').replace('_', ' '),
            type: 'material',
            rarity: 'common',
            description: 'Gathered on quest.',
            icon: '📦',
            quantity: 1,
          });
        }
      }

      this.emit('job_complete', { job, pet: this.state });
      this.emit('notification', { message: `Completed ${job.title}! +${job.coinReward} coins` });
    }
  }

  // --------------------------------------------------------------------------
  // Magic Spell Casting
  // --------------------------------------------------------------------------
  public castSpell(spellId: string): boolean {
    const spell = this.rules.magicSpells.find((s) => s.id === spellId);
    if (!spell) return false;
    if (this.state.level < spell.minLevel) return false;
    if (this.state.stats.mana < spell.manaCost) return false;

    this.state.stats.mana -= spell.manaCost;
    this.state.spellsCastCount += 1;

    // Apply self effects
    if (spell.target === 'self' || spell.target === 'both') {
      if (spell.healthBonus) {
        this.state.stats.health = Math.min(
          this.rules.general.maxStatValue,
          this.state.stats.health + spell.healthBonus
        );
      }
      if (spell.happinessBonus) {
        this.state.stats.happiness = Math.min(
          this.rules.general.maxStatValue,
          this.state.stats.happiness + spell.happinessBonus
        );
      }
    }

    this.addXP(35);
    this.recordAction('cast_spell', 1);

    // If target touches friend, generate socket packet
    if (spell.target === 'friend' || spell.target === 'both') {
      this.emit('packet_out', {
        type: 'MAGIC',
        payload: {
          spellId: spell.id,
          name: spell.name,
          hpBonus: spell.healthBonus,
          happyBonus: spell.happinessBonus,
        },
      });
    }

    this.emit('notification', { message: `Cast ${spell.name}! ${spell.icon}` });
    return true;
  }

  public sendTreat(): void {
    this.state.treatsSentCount += 1;
    this.recordAction('send_treat', 1);
    this.addXP(10);
    this.emit('packet_out', {
      type: 'TREAT',
      payload: { from: this.state.name },
    });
    this.emit('notification', { message: 'Sent a yummy treat to your friend! 🍩' });
    this.emit('stat_change', this.state);
  }

  // --------------------------------------------------------------------------
  // Remote Peer Packet Ingestion (Direct Socket or Server Relay)
  // --------------------------------------------------------------------------
  public receivePacket(packet: CYDMessagePacket): void {
    if (packet.type === 'TREAT') {
      const boost = this.rules.socialActions.treatSendHungerGain;
      const happyBoost = this.rules.socialActions.treatSendHappinessGain;
      this.state.stats.hunger = Math.min(
        this.rules.general.maxStatValue,
        this.state.stats.hunger + boost
      );
      this.state.stats.happiness = Math.min(
        this.rules.general.maxStatValue,
        this.state.stats.happiness + happyBoost
      );
      this.emit('notification', { message: 'Friend sent you a delicious treat! 🍩' });
    } else if (packet.type === 'MAGIC') {
      this.state.stats.health = Math.min(
        this.rules.general.maxStatValue,
        this.state.stats.health + this.rules.socialActions.coopMagicHpBoost
      );
      this.state.stats.happiness = Math.min(
        this.rules.general.maxStatValue,
        this.state.stats.happiness + this.rules.socialActions.coopMagicHappinessBoost
      );
      this.emit('notification', { message: 'Received magical friend blessing! ✨' });
    }

    this.emit('stat_change', this.state);
  }

  // --------------------------------------------------------------------------
  // Daily Missions & Login Streaks
  // --------------------------------------------------------------------------
  public checkDailyMissionsLogin(): void {
    const today = new Date().toISOString().split('T')[0];
    if (!this.state.dailyMissions) {
      this.state.dailyMissions = this.createInitialDailyMissionsState(1);
      return;
    }

    const { lastLoginDate, consecutiveLoginDays } = this.state.dailyMissions;
    if (lastLoginDate === today) {
      // Ensure missing missions from config exist
      for (const rule of this.rules.dailyMissions || []) {
        const existing = this.state.dailyMissions.missions.find((m) => m.id === rule.id);
        if (!existing) {
          const isStreak = rule.actionType === 'login_streak';
          const count = isStreak ? consecutiveLoginDays : 0;
          this.state.dailyMissions.missions.push({
            id: rule.id,
            title: rule.name,
            title_de: rule.name_de,
            description: rule.description,
            description_de: rule.description_de,
            actionType: rule.actionType as MissionActionType,
            targetCount: rule.targetCount,
            currentCount: count,
            coinReward: rule.coinReward,
            xpReward: rule.xpReward,
            completed: count >= rule.targetCount,
            claimed: false,
            icon: rule.icon,
          });
        }
      }
      return;
    }

    // Different day: calculate difference in calendar days
    const lastTime = new Date(lastLoginDate).getTime();
    const currentTime = new Date(today).getTime();
    const diffDays = Math.round(Math.abs(currentTime - lastTime) / (1000 * 60 * 60 * 24));

    let newStreak = consecutiveLoginDays;
    if (diffDays === 1) {
      newStreak += 1;
    } else if (diffDays > 1) {
      newStreak = 1;
    }

    this.state.dailyMissions = this.createInitialDailyMissionsState(newStreak);
    this.emit('stat_change', this.state);
  }

  public recordAction(actionType: MissionActionType, count = 1): void {
    if (!this.state.dailyMissions) {
      this.checkDailyMissionsLogin();
    }
    const dm = this.state.dailyMissions;
    if (!dm) return;

    let changed = false;
    for (const mission of dm.missions) {
      if (mission.actionType === actionType && !mission.completed) {
        mission.currentCount = Math.min(mission.targetCount, mission.currentCount + count);
        changed = true;
        this.emit('mission_progress', { mission, pet: this.state });
        if (mission.currentCount >= mission.targetCount) {
          mission.completed = true;
          this.emit('mission_complete', { mission, pet: this.state });
          this.emit('notification', {
            message: `🎯 Mission complete: ${mission.title}! Claim bonus coins & XP!`,
          });
        }
      }
    }
    if (changed) {
      this.emit('stat_change', this.state);
    }
  }

  public claimDailyMission(missionId: string): boolean {
    if (!this.state.dailyMissions) return false;
    const mission = this.state.dailyMissions.missions.find((m) => m.id === missionId);
    if (!mission || !mission.completed || mission.claimed) return false;

    mission.claimed = true;
    this.state.coins += mission.coinReward;
    this.addXP(mission.xpReward);
    this.emit('notification', {
      message: `Claimed ${mission.title}! +${mission.coinReward} Coins, +${mission.xpReward} XP ✨`,
    });
    this.emit('stat_change', this.state);
    return true;
  }

  public getDailyMissions(): DailyMission[] {
    if (!this.state.dailyMissions) {
      this.checkDailyMissionsLogin();
    }
    return this.state.dailyMissions?.missions || [];
  }

  public getConsecutiveLoginDays(): number {
    if (!this.state.dailyMissions) {
      this.checkDailyMissionsLogin();
    }
    return this.state.dailyMissions?.consecutiveLoginDays || 1;
  }

  // --------------------------------------------------------------------------
  // Auto Tick Management
  // --------------------------------------------------------------------------
  public startAutoTick(intervalMs = 6000): void {
    this.stopAutoTick();
    this.tickTimer = setInterval(() => this.tick(), intervalMs);
  }

  public stopAutoTick(): void {
    if (this.tickTimer) {
      clearInterval(this.tickTimer);
      this.tickTimer = null;
    }
  }
}
