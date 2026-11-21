import { useState, useEffect, useRef } from 'react';
import confetti from 'canvas-confetti';
import {
  PetState,
  FriendPetSummary,
  BiomeType,
  Recipe,
  Job,
  MagicSpell,
  InventoryItem,
  TradeOffer,
  MissionActionType,
} from './types/pet';
import {
  createInitialPet,
  createInitialFriendPet,
  BIOMES,
  SEASONS,
  getXPForNextLevel,
  checkEvolution,
  CATALOG_ITEMS,
  SHOP_ITEMS_CATALOG,
  ShopCatalogItem,
  FOOD_CATALOG,
  JOBS,
  RECIPES,
} from './utils/petLogic';
import { cydAudio, toggleAudio } from './utils/audio';
import { DualDeviceView } from './components/DualDeviceView';
import { RPGSubsystemsModal, RPGModalTab } from './components/RPGSubsystemsModal';
import { TradingModal } from './components/TradingModal';
import { ChatModal } from './components/ChatModal';
import { LeaderboardModal } from './components/LeaderboardModal';
import { WiFiAPModal } from './components/WiFiAPModal';
import { FirmwareTab } from './components/FirmwareTab';
import { PHPServerTab } from './components/PHPServerTab';
import { RulesEditorTab } from './components/RulesEditorTab';
import { TamagotchiCoreEngine } from './engine/TamagotchiCoreEngine';
import { convertParsedRecipeToRecipe } from './engine/ConfigParser';
import { translations, Language } from './utils/i18n';
import { SPELLS } from './utils/petLogic';

export default function App() {
  // Language State: 'de' | 'en' (default to 'de' as requested by user)
  const [language, setLanguage] = useState<Language>(() => {
    const saved = localStorage.getItem('cyd_language');
    return saved === 'en' || saved === 'de' ? saved : 'de';
  });

  const t = translations[language] || translations.de;

  // Navigation Tabs: 'dual_cyd' | 'firmware' | 'php_server' | 'rules_editor'
  const [currentTab, setCurrentTab] = useState<'dual_cyd' | 'firmware' | 'php_server' | 'rules_editor'>('dual_cyd');
  const engineRef = useRef<TamagotchiCoreEngine>(new TamagotchiCoreEngine());

  // Dynamically Configured Recipes from .cfg Engine Rules (Runtime Hot-Reloadable & Bilingual)
  const [activeRecipes, setActiveRecipes] = useState<Recipe[]>(() => {
    try {
      const parsed = engineRef.current.getRules().recipes;
      if (parsed && parsed.length > 0) {
        return parsed.map(convertParsedRecipeToRecipe);
      }
    } catch {
      // Fallback
    }
    return RECIPES;
  });

  // Audio State
  const [soundOn, setSoundOn] = useState(true);

  // Active Modal: 'rpg' | 'trade' | 'chat' | 'leaderboard' | 'wifi' | null
  const [activeModal, setActiveModal] = useState<'rpg' | 'trade' | 'chat' | 'leaderboard' | 'wifi' | null>(null);
  const [rpgInitialTab, setRpgInitialTab] = useState<RPGModalTab>('stats');
  const [wifiConfig, setWifiConfig] = useState({
    ssid: 'Home-WiFi-2.4G',
    deviceId: 'CYD-DEV1',
    trainerName: 'TrainerAlex',
    friendId: 'CYD-DEV2',
    serverUrl: 'http://192.168.1.100/cyd-tamagotchi',
  });

  // Device 1 (Local Player) State
  const [dev1Pet, setDev1Pet] = useState<PetState>(() => {
    const saved = localStorage.getItem('cyd_dev1_pet');
    if (saved) {
      try {
        const parsed = JSON.parse(saved);
        return {
          ...parsed,
          satietyTicks: parsed.satietyTicks ?? 0,
          isSick: parsed.isSick ?? false,
        };
      } catch {
        // Fallback
      }
    }
    return createInitialPet('CYD-LOCAL', 'TrainerAlex', 'grassland');
  });

  // Device 2 (Friend Player) State
  const [dev2Pet, setDev2Pet] = useState<PetState>(() => {
    const saved = localStorage.getItem('cyd_dev2_pet');
    if (saved) {
      try {
        const parsed = JSON.parse(saved);
        return {
          ...parsed,
          satietyTicks: parsed.satietyTicks ?? 0,
          isSick: parsed.isSick ?? false,
        };
      } catch {
        // Fallback
      }
    }
    return createInitialFriendPet('CYD-FRIEND', 'desert');
  });

  // Simulated Hardware RGB LEDs
  const [ledDev1, setLedDev1] = useState<'off' | 'red' | 'green' | 'blue' | 'cyan' | 'gold'>('green');
  const [ledDev2, setLedDev2] = useState<'off' | 'red' | 'green' | 'blue' | 'cyan' | 'gold'>('green');

  // Screen Notifications & Speech Balloons
  const [notifDev1, setNotifDev1] = useState<string | null>(null);
  const [notifDev2, setNotifDev2] = useState<string | null>(null);
  const [chatBalloonDev1, setChatBalloonDev1] = useState<string | null>(null);
  const [chatBalloonDev2, setChatBalloonDev2] = useState<string | null>(null);

  // Active Flying Socket Packet
  const [activePacket, setActivePacket] = useState<{
    from: 'DEV1' | 'DEV2';
    type: string;
    label: string;
  } | null>(null);

  // Pending Trade Offers & Perspective ('DEV1' | 'DEV2')
  const [tradeOffers, setTradeOffers] = useState<TradeOffer[]>([]);
  const [tradePerspective, setTradePerspective] = useState<'DEV1' | 'DEV2'>('DEV1');

  // Co-op Dual-Cast 10s Window & Full-Screen Aurora State
  const [coopCastWindow, setCoopCastWindow] = useState<{
    initiator: 'DEV1' | 'DEV2';
    expiresAt: number;
  } | null>(null);
  const [coopRemainingSec, setCoopRemainingSec] = useState<number>(0);
  const [isAuroraActive, setIsAuroraActive] = useState<boolean>(false);
  const auroraTimerRef = useRef<number | null>(null);

  // Auto-save to LocalStorage
  useEffect(() => {
    localStorage.setItem('cyd_dev1_pet', JSON.stringify(dev1Pet));
  }, [dev1Pet]);

  useEffect(() => {
    localStorage.setItem('cyd_dev2_pet', JSON.stringify(dev2Pet));
  }, [dev2Pet]);

  // Periodic background auto-save every 2 minutes (120,000ms)
  useEffect(() => {
    const autoSaveInterval = setInterval(() => {
      fetch('/api/cloud_save', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ deviceId: dev1Pet.deviceId, petData: dev1Pet }),
      }).catch(() => {});
    }, 120000);
    return () => clearInterval(autoSaveInterval);
  }, [dev1Pet]);

  // Daily Missions Login Streak Check on Startup
  useEffect(() => {
    engineRef.current.setState(dev1Pet);
    engineRef.current.checkDailyMissionsLogin();
    const state = engineRef.current.getState();
    if (state.dailyMissions) {
      setDev1Pet((prev) => ({
        ...prev,
        dailyMissions: state.dailyMissions,
      }));
    }
  }, []);

  // Lifecycle Game Loop (Hunger, Satiety Shield, Sickness & Jobs)
  useEffect(() => {
    const interval = setInterval(() => {
      // 1. Check Dev 1 decay
      setDev1Pet((prev) => {
        if (prev.phase === 'Egg') return prev; // Eggs don't decay stats
        const newStats = { ...prev.stats };
        let nextSatiety = prev.satietyTicks || 0;

        if (nextSatiety > 0) {
          nextSatiety -= 1;
        } else if (newStats.hunger > 0) {
          newStats.hunger = Math.max(0, newStats.hunger - 1);
        }

        if (newStats.energy > 0) newStats.energy = Math.max(0, newStats.energy - 1);
        if (newStats.hunger < 20 && newStats.health > 0) newStats.health = Math.max(0, newStats.health - 1);
        if (prev.isSick && newStats.health > 0) newStats.health = Math.max(0, newStats.health - 1);

        // Natural mana regeneration
        if (newStats.mana < 100) newStats.mana = Math.min(100, newStats.mana + 2);

        return { ...prev, stats: newStats, satietyTicks: nextSatiety };
      });

      // 2. Check Dev 2 decay
      setDev2Pet((prev) => {
        if (prev.phase === 'Egg') return prev;
        const newStats = { ...prev.stats };
        let nextSatiety = prev.satietyTicks || 0;
        if (nextSatiety > 0) {
          nextSatiety -= 1;
        } else if (newStats.hunger > 0) {
          newStats.hunger = Math.max(0, newStats.hunger - 1);
        }
        if (newStats.energy > 0) newStats.energy = Math.max(0, newStats.energy - 1);
        if (prev.isSick && newStats.health > 0) newStats.health = Math.max(0, newStats.health - 1);
        if (newStats.mana < 100) newStats.mana = Math.min(100, newStats.mana + 2);
        return { ...prev, stats: newStats, satietyTicks: nextSatiety };
      });
    }, 8000);

    return () => clearInterval(interval);
  }, []);

  // 1-second Active Job Completion Watcher (fires promptly when countdown hits 0)
  useEffect(() => {
    if (!dev1Pet.activeJob) return;
    const jobTimer = setInterval(() => {
      setDev1Pet((prev) => {
        if (!prev.activeJob) return prev;
        const elapsed = (Date.now() - prev.activeJob.startedAt) / 1000;
        if (elapsed < prev.activeJob.durationSeconds) return prev;

        const completedJob = JOBS.find((j) => j.id === prev.activeJob!.jobId);
        const nextInv = prev.inventory.map((it) => ({ ...it }));

        const pushDropToInv = (itemTemplate: InventoryItem, qty: number) => {
          const idx = nextInv.findIndex((i) => i.id === itemTemplate.id);
          if (idx >= 0) {
            nextInv[idx].quantity += qty;
          } else {
            nextInv.push({ ...itemTemplate, quantity: qty });
          }
        };

        const dropLabels: string[] = [];

        if (completedJob) {
          if (Math.random() <= completedJob.possibleDrop.chance) {
            const matId = completedJob.possibleDrop.itemId;
            const shopTemplate = SHOP_ITEMS_CATALOG.find((s) => s.id === matId)?.item;
            const matItem: InventoryItem = shopTemplate
              ? { ...shopTemplate }
              : {
                  id: matId,
                  name: completedJob.possibleDrop.name,
                  name_de: completedJob.possibleDrop.name_de,
                  type: 'material',
                  rarity: 'common',
                  description: 'Foraged crafting ingredient.',
                  description_de: 'Gesammelte Kochzutat.',
                  icon: matId === 'mat_meat' ? '🥩' : matId === 'mat_kelp' ? '🥬' : matId === 'mat_sunstone' ? '🧂' : '🌿',
                  quantity: 1,
                };
            pushDropToInv(matItem, 1);
            dropLabels.push(language === 'de' ? (completedJob.possibleDrop.name_de || completedJob.possibleDrop.name) : completedJob.possibleDrop.name);
          }

          if (completedJob.bonusFoodDrop && Math.random() <= completedJob.bonusFoodDrop.chance) {
            const foodTemplate = FOOD_CATALOG[completedJob.bonusFoodDrop.itemId];
            if (foodTemplate) {
              pushDropToInv(foodTemplate, completedJob.bonusFoodDrop.quantity);
              dropLabels.push(`${completedJob.bonusFoodDrop.quantity}x ${foodTemplate.icon}`);
            }
          }
        }

        const coinsGain = completedJob?.coinReward || 30;
        const xpGain = completedJob?.xpReward || 45;

        cydAudio.levelUp();
        triggerNotificationDev1(
          dropLabels.length > 0
            ? (language === 'de'
                ? `Beruf fertig! +${coinsGain}🪙 +${xpGain}XP (${dropLabels.join(', ')})`
                : `Job Done! +${coinsGain}🪙 +${xpGain}XP (${dropLabels.join(', ')})`)
            : (language === 'de'
                ? `Beruf abgeschlossen! +${coinsGain} Münzen & +${xpGain} XP 🧺`
                : `Job Completed! +${coinsGain} Coins & +${xpGain} XP 🧺`)
        );

        let currentXP = prev.xp + xpGain;
        let currentLevel = prev.level;
        let nextReq = prev.xpNext;
        while (currentXP >= nextReq) {
          currentXP -= nextReq;
          currentLevel += 1;
          nextReq = getXPForNextLevel(currentLevel);
        }

        return {
          ...prev,
          activeJob: null,
          inventory: nextInv,
          coins: prev.coins + coinsGain,
          xp: currentXP,
          level: currentLevel,
          xpNext: nextReq,
          phase: checkEvolution(currentLevel, prev.phase),
          jobsCompletedCount: prev.jobsCompletedCount + 1,
        };
      });
    }, 1000);

    return () => clearInterval(jobTimer);
  }, [dev1Pet.activeJob?.startedAt, dev1Pet.activeJob?.jobId, language]);

  // Co-op Dual-Cast 10s Countdown Ticker
  useEffect(() => {
    if (!coopCastWindow) {
      setCoopRemainingSec(0);
      return;
    }
    const updateRemaining = () => {
      const remMs = coopCastWindow.expiresAt - Date.now();
      if (remMs <= 0) {
        setCoopCastWindow(null);
        setCoopRemainingSec(0);
      } else {
        setCoopRemainingSec(Math.ceil(remMs / 1000));
      }
    };
    updateRemaining();
    const timerId = window.setInterval(updateRemaining, 250);
    return () => clearInterval(timerId);
  }, [coopCastWindow]);

  // Notifications Helpers with Auto-Dismiss
  const triggerNotificationDev1 = (msg: string) => {
    setNotifDev1(msg);
    setTimeout(() => setNotifDev1(null), 3000);
  };

  const triggerNotificationDev2 = (msg: string) => {
    setNotifDev2(msg);
    setTimeout(() => setNotifDev2(null), 3000);
  };

  // Summaries shared across the simulated socket
  const dev1FriendSummary: FriendPetSummary = {
    deviceId: dev2Pet.deviceId,
    deviceName: dev2Pet.trainerName,
    petName: dev2Pet.name,
    biome: dev2Pet.biome,
    phase: dev2Pet.phase,
    level: dev2Pet.level,
    stats: dev2Pet.stats,
    equipped: dev2Pet.equipped,
    isOnline: true,
  };

  const dev2FriendSummary: FriendPetSummary = {
    deviceId: dev1Pet.deviceId,
    deviceName: dev1Pet.trainerName,
    petName: dev1Pet.name,
    biome: dev1Pet.biome,
    phase: dev1Pet.phase,
    level: dev1Pet.level,
    stats: dev1Pet.stats,
    equipped: dev1Pet.equipped,
    isOnline: true,
  };

  // Pure Inventory Helpers (immutable, quantity-stacked)
  const stackAddItem = (inventory: InventoryItem[], item: InventoryItem, qty = 1): InventoryItem[] => {
    const next = inventory.map((i) => ({ ...i }));
    const idx = next.findIndex((i) => i.id === item.id);
    if (idx >= 0) {
      next[idx].quantity += qty;
    } else {
      next.push({ ...item, quantity: qty });
    }
    return next;
  };

  const deductInventoryItem = (
    pet: PetState,
    itemId: string,
    qty = 1
  ): { inventory: InventoryItem[]; equipped: PetState['equipped']; removed: boolean } => {
    const target = pet.inventory.find((i) => i.id === itemId && i.quantity >= qty);
    if (!target) return { inventory: pet.inventory, equipped: pet.equipped, removed: false };

    let remainingAfter = 0;
    const nextInv = pet.inventory
      .map((i) => {
        if (i.id === itemId) {
          remainingAfter = i.quantity - qty;
          return { ...i, quantity: remainingAfter };
        }
        return i;
      })
      .filter((i) => i.quantity > 0);

    const nextEquipped = { ...pet.equipped };
    if (remainingAfter <= 0) {
      (Object.keys(nextEquipped) as (keyof PetState['equipped'])[]).forEach((slot) => {
        if (nextEquipped[slot] === itemId) {
          delete nextEquipped[slot];
        }
      });
    }

    return { inventory: nextInv, equipped: nextEquipped, removed: true };
  };

  const applyXpToPetState = (prev: PetState, amount: number): { pet: PetState; leveledUp: boolean } => {
    let currentXP = prev.xp + amount;
    let currentLevel = prev.level;
    let nextReq = prev.xpNext;
    let leveledUp = false;

    while (currentXP >= nextReq) {
      currentXP -= nextReq;
      currentLevel += 1;
      nextReq = getXPForNextLevel(currentLevel);
      leveledUp = true;
    }

    return {
      leveledUp,
      pet: {
        ...prev,
        level: currentLevel,
        xp: currentXP,
        xpNext: nextReq,
        phase: checkEvolution(currentLevel, prev.phase),
      },
    };
  };

  // Helper to add XP & check level up
  const addXPDev1 = (amount: number) => {
    setDev1Pet((prev) => {
      const { pet: updated, leveledUp } = applyXpToPetState(prev, amount);

      if (leveledUp) {
        cydAudio.levelUp();
        confetti({ particleCount: 80, spread: 60, origin: { y: 0.6 } });
        triggerNotificationDev1(`LEVEL UP! Now Lv.${updated.level} 🌟`);
        setLedDev1('gold');
        setTimeout(() => setLedDev1('green'), 3000);
      }

      return updated;
    });
  };

  // -------------------------------------------------------------------------
  // Daily Missions Progression & Claiming
  // -------------------------------------------------------------------------
  const recordMissionAction = (actionType: MissionActionType, count = 1) => {
    engineRef.current.setState(dev1Pet);
    engineRef.current.recordAction(actionType, count);
    const updated = engineRef.current.getState();
    if (updated.dailyMissions) {
      setDev1Pet((prev) => ({
        ...prev,
        dailyMissions: updated.dailyMissions,
      }));
    }
  };

  const handleClaimMission = (missionId: string) => {
    engineRef.current.setState(dev1Pet);
    const success = engineRef.current.claimDailyMission(missionId);
    if (success) {
      const updated = engineRef.current.getState();
      setDev1Pet((prev) => ({
        ...prev,
        coins: updated.coins,
        xp: updated.xp,
        dailyMissions: updated.dailyMissions,
      }));
      cydAudio.fanfare();
      confetti({ particleCount: 70, spread: 60, origin: { y: 0.6 } });
      const mission = updated.dailyMissions?.missions.find((m) => m.id === missionId);
      const title = mission ? (language === 'de' && mission.title_de ? mission.title_de : mission.title) : 'Mission';
      triggerNotificationDev1(
        language === 'de'
          ? `Tagesmission eingelöst: ${title}! +🪙 & +XP`
          : `Daily mission claimed: ${title}! +🪙 & +XP`
      );
    }
  };

  const handleClaimStreakBonus = () => {
    engineRef.current.setState(dev1Pet);
    const result = engineRef.current.claimDailyLoginStreakBonus();
    if (result.success) {
      const updated = engineRef.current.getState();
      setDev1Pet((prev) => ({
        ...prev,
        coins: updated.coins,
        xp: updated.xp,
        dailyMissions: updated.dailyMissions,
      }));
      cydAudio.fanfare();
      confetti({ particleCount: 90, spread: 75, origin: { y: 0.55 } });
      triggerNotificationDev1(
        language === 'de'
          ? `🔥 Streak-Bonus Tag ${result.streak} abgeholt: +${result.coins} Münzen, +${result.xp} XP!`
          : `🔥 Day ${result.streak} Streak Bonus Claimed: +${result.coins} Coins, +${result.xp} XP!`
      );
    }
  };

  // Inventory-Gated Feed Handler (CYD #1 or CYD #2)
  const handleFeedWithItem = (device: 'dev1' | 'dev2', itemId: string) => {
    const targetPet = device === 'dev1' ? dev1Pet : dev2Pet;
    const setTargetPet = device === 'dev1' ? setDev1Pet : setDev2Pet;
    const notify = device === 'dev1' ? triggerNotificationDev1 : triggerNotificationDev2;
    const setLed = device === 'dev1' ? setLedDev1 : setLedDev2;

    const foodItem = targetPet.inventory.find(
      (i) => i.id === itemId && i.type === 'food' && i.quantity > 0
    );

    if (!foodItem) {
      cydAudio.warning();
      notify(t.emptyFoodMsg);
      return;
    }

    const rolledSick = Boolean(
      foodItem.sickChance && foodItem.sickChance > 0 && Math.random() < foodItem.sickChance
    );

    setTargetPet((prev) => {
      const updatedInv = prev.inventory
        .map((it) => (it.id === itemId ? { ...it, quantity: it.quantity - 1 } : it))
        .filter((it) => it.quantity > 0);

      const newStats = {
        ...prev.stats,
        hunger: Math.min(100, prev.stats.hunger + (foodItem.hungerGain || 0)),
        health: Math.max(0, Math.min(100, prev.stats.health + (foodItem.healthGain || 0) - (rolledSick ? 15 : 0))),
        energy: Math.min(100, prev.stats.energy + (foodItem.energyGain || 0)),
        happiness: Math.min(100, prev.stats.happiness + (foodItem.happinessGain || 0)),
      };

      const nextSatiety = Math.max(prev.satietyTicks || 0, foodItem.satietyTicks || 0);

      return {
        ...prev,
        inventory: updatedInv,
        stats: newStats,
        satietyTicks: nextSatiety,
        isSick: prev.isSick || rolledSick,
        lastFedAt: Date.now(),
      };
    });

    cydAudio.munch();
    const foodLabel = language === 'de' ? (foodItem.name_de || foodItem.name) : foodItem.name;

    if (rolledSick) {
      setTimeout(() => cydAudio.warning(), 220);
      setLed('red');
      setTimeout(() => setLed('green'), 2200);
      notify(
        language === 'de'
          ? `Oh nein! ${foodLabel} verursachte Bauchweh! 🤢 (-15 LP)`
          : `Oh no! ${foodLabel} caused tummyache! 🤢 (-15 HP)`
      );
    } else if ((foodItem.satietyTicks || 0) > 0) {
      notify(
        language === 'de'
          ? `${foodLabel} gefüttert! +${foodItem.hungerGain || 0} FD & 🛡️ ${foodItem.satietyTicks}T`
          : `Fed ${foodLabel}! +${foodItem.hungerGain || 0} FD & 🛡️ ${foodItem.satietyTicks}T`
      );
    } else {
      notify(
        language === 'de'
          ? `${foodLabel} gefüttert! +${foodItem.hungerGain || 0} FD`
          : `Fed ${foodLabel}! +${foodItem.hungerGain || 0} FD`
      );
    }

    if (device === 'dev1') {
      addXPDev1(15);
      recordMissionAction('feed', 1);
    }
  };

  // =========================================================================
  // ACTIONS DISPATCHER FOR DEVICE 1 (YOU)
  // =========================================================================

  const handleDev1Action = (action: string) => {
    switch (action) {
      case 'feed': {
        const firstFood = dev1Pet.inventory.find((i) => i.type === 'food' && i.quantity > 0);
        if (firstFood) {
          handleFeedWithItem('dev1', firstFood.id);
        } else {
          cydAudio.warning();
          triggerNotificationDev1(t.emptyFoodMsg);
        }
        break;
      }

      case 'play': {
        cydAudio.play();
        setDev1Pet((prev) => ({
          ...prev,
          stats: {
            ...prev.stats,
            happiness: Math.min(100, prev.stats.happiness + 20),
            energy: Math.max(0, prev.stats.energy - 10),
          },
        }));
        triggerNotificationDev1(t.notifPlayed);
        addXPDev1(20);
        recordMissionAction('play', 1);
        break;
      }

      case 'bath': {
        cydAudio.clean();
        setDev1Pet((prev) => ({
          ...prev,
          stats: {
            ...prev.stats,
            hygiene: 100,
            happiness: Math.min(100, prev.stats.happiness + 10),
          },
        }));
        triggerNotificationDev1(t.notifBath);
        addXPDev1(10);
        recordMissionAction('bath', 1);
        break;
      }

      case 'egg_tap': {
        if (dev1Pet.phase === 'Egg') {
          const newWarmth = Math.min(100, dev1Pet.eggWarmth + 20);
          if (newWarmth >= 100) {
            cydAudio.eggHatch();
            confetti({ particleCount: 120, spread: 80, origin: { y: 0.5 } });
            setDev1Pet((prev) => ({
              ...prev,
              phase: 'Baby',
              eggWarmth: 100,
              stats: { ...prev.stats, health: 100, hunger: 100, happiness: 100 },
            }));
            triggerNotificationDev1(t.notifEggHatched);
            setLedDev1('gold');
            setTimeout(() => setLedDev1('green'), 4000);
          } else {
            cydAudio.click();
            setDev1Pet((prev) => ({ ...prev, eggWarmth: newWarmth }));
            triggerNotificationDev1(`${t.notifEggWarm} (${newWarmth}%)`);
          }
        }
        break;
      }

      case 'pet_touch': {
        cydAudio.play();
        setDev1Pet((prev) => ({
          ...prev,
          stats: {
            ...prev.stats,
            happiness: Math.min(100, prev.stats.happiness + 5),
          },
        }));
        triggerNotificationDev1(t.notifCuddled);
        break;
      }

      case 'open_rpg':
        setRpgInitialTab('recipes');
        setActiveModal('rpg');
        break;

      case 'open_stats':
        setRpgInitialTab('stats');
        setActiveModal('rpg');
        break;

      case 'cloud_save': {
        cydAudio.click();
        triggerNotificationDev1(t.notifSaved);
        fetch('/api/cloud_save', {
          method: 'POST',
          headers: { 'Content-Type': 'application/json' },
          body: JSON.stringify({ deviceId: dev1Pet.deviceId, petData: dev1Pet }),
        }).catch(() => {});
        break;
      }

      // --- REMOTE FRIEND INTERACTIONS (DIRECT SOCKET TRANSMISSION) ---
      case 'send_treat': {
        cydAudio.treatSend();
        setActivePacket({ from: 'DEV1', type: 'TREAT', label: '🍖 TREAT' });
        triggerNotificationDev1(t.notifTreatSent);
        recordMissionAction('send_treat', 1);

        setTimeout(() => {
          cydAudio.treatReceive();
          setActivePacket(null);
          setLedDev2('cyan');
          triggerNotificationDev2(t.notifTreatReceived);
          setDev2Pet((prev) => ({
            ...prev,
            stats: {
              ...prev.stats,
              hunger: Math.min(100, prev.stats.hunger + 25),
              happiness: Math.min(100, prev.stats.happiness + 20),
            },
          }));
          // Dev 1 gains treat sent score & XP
          setDev1Pet((prev) => ({
            ...prev,
            treatsSentCount: prev.treatsSentCount + 1,
          }));
          addXPDev1(30);
          setTimeout(() => setLedDev2('green'), 2500);
        }, 600);
        break;
      }

      case 'open_chat':
        setActiveModal('chat');
        break;

      case 'cast_magic': {
<<<<<<< HEAD
        // Legacy co-op magic (Friend Blessing) - kept for backward compatibility
        if (dev1Pet.stats.mana < 25) {
          cydAudio.warning();
          triggerNotificationDev1(language === 'de' ? '25 Mana erforderlich! ⚡' : 'Need 25 Mana to cast! ⚡');
          return;
        }
        cydAudio.magicCast();
        setActivePacket({ from: 'DEV1', type: 'MAGIC', label: '✨ BLESSING' });
        setDev1Pet((prev) => ({
          ...prev,
          stats: { ...prev.stats, mana: prev.stats.mana - 25 },
          spellsCastCount: prev.spellsCastCount + 1,
        }));
        triggerNotificationDev1(t.notifMagicSent);
        recordMissionAction('cast_spell', 1);

        setTimeout(() => {
          setActivePacket(null);
          setLedDev1('gold');
          setLedDev2('gold');
          confetti({ particleCount: 50, spread: 60 });
          triggerNotificationDev2(t.notifMagicReceived);
          setDev2Pet((prev) => ({
            ...prev,
            stats: {
              ...prev.stats,
              health: Math.min(100, prev.stats.health + 30),
              happiness: Math.min(100, prev.stats.happiness + 25),
            },
          }));
          addXPDev1(40);
          setTimeout(() => {
            setLedDev1('green');
            setLedDev2('green');
          }, 2500);
        }, 700);
=======
        handleCoopCast('DEV1');
>>>>>>> b7fa75a5ef0c5a05de74501debb43b891b4eae32
        break;
      }

      case 'cast_spell': {
        // New spell flyout: cast specific learned spell
        const spellId = action.split(':')[1];
        const spell = SPELLS.find((s) => s.id === spellId);
        if (!spell) {
          cydAudio.warning();
          triggerNotificationDev1('Unknown spell!');
          return;
        }
        if (dev1Pet.stats.mana < spell.manaCost) {
          cydAudio.warning();
          triggerNotificationDev1(
            language === 'de'
              ? `${spell.manaCost} Mana erforderlich! ⚡`
              : `Need ${spell.manaCost} Mana! ⚡`
          );
          return;
        }
        // Check if spell is learned
        if (!dev1Pet.learnedSpells?.includes(spellId)) {
          cydAudio.warning();
          triggerNotificationDev1(language === 'de' ? 'Zauber nicht gelernt!' : 'Spell not learned!');
          return;
        }

        cydAudio.magicCast();
        const spellTitle = language === 'de' ? (spell.name_de || spell.name) : spell.name;
        const spellLabel = spell.target === 'friend' ? '✨ BLESSING' : spell.target === 'both' ? '🌟 HARMONY' : '✨ SPELL';
        setActivePacket({ from: 'DEV1', type: 'MAGIC', label: spellLabel });

        setDev1Pet((prev) => ({
          ...prev,
          stats: { ...prev.stats, mana: Math.max(0, prev.stats.mana - spell.manaCost) },
          spellsCastCount: prev.spellsCastCount + 1,
        }));

        // Apply spell effects to self
        setDev1Pet((prev) => {
          const stats = { ...prev.stats };
          let nextSatiety = prev.satietyTicks || 0;
          let nextSick = prev.isSick;

          if (spell.target === 'self' || spell.target === 'both') {
            if (spell.healthGain) stats.health = Math.min(100, stats.health + spell.healthGain);
            if (spell.hungerGain) stats.hunger = Math.min(100, stats.hunger + spell.hungerGain);
            if (spell.happinessGain) stats.happiness = Math.min(100, stats.happiness + spell.happinessGain);
            if (spell.energyGain) stats.energy = Math.min(100, stats.energy + spell.energyGain);
            if (spell.satietyTicks) nextSatiety = Math.max(nextSatiety, spell.satietyTicks);
            if (spell.curesSickness) {
              nextSick = false;
              stats.hygiene = 100;
            }
          }
          return {
            ...prev,
            stats,
            satietyTicks: nextSatiety,
            isSick: nextSick,
          };
        });

        triggerNotificationDev1(
          spell.curesSickness && dev1Pet.isSick
            ? `${spellTitle}: ${t.curedSickness}`
            : language === 'de'
            ? `${spellTitle} gewirkt! ✨`
            : `Cast ${spellTitle}! ✨`
        );
        recordMissionAction('cast_spell', 1);

        // Apply spell effects to friend (if target is friend or both)
        if (spell.target === 'friend' || spell.target === 'both') {
          setTimeout(() => {
            setActivePacket(null);
            setLedDev1('gold');
            setLedDev2('gold');
            confetti({ particleCount: 50, spread: 60 });
            const friendSpellTitle = language === 'de' ? (spell.name_de || spell.name) : spell.name;
            triggerNotificationDev2(
              language === 'de' ? `${friendSpellTitle} empfangen! ✨` : `Received ${friendSpellTitle}! ✨`
            );
            setDev2Pet((p) => ({
              ...p,
              stats: {
                ...p.stats,
                health: Math.min(100, p.stats.health + (spell.healthGain ?? 30)),
                happiness: Math.min(100, p.stats.happiness + (spell.happinessGain ?? 20)),
              },
            }));
            addXPDev1(35);
            setTimeout(() => {
              setLedDev1('green');
              setLedDev2('green');
            }, 2500);
          }, 600);
        } else {
          setTimeout(() => {
            setActivePacket(null);
            setLedDev1('green');
          }, 600);
        }
        addXPDev1(35);
        break;
      }

      case 'start_quest': {
        cydAudio.click();
        setActivePacket({ from: 'DEV1', type: 'QUEST', label: '⚔️ CO-OP QUEST' });
        triggerNotificationDev1(language === 'de' ? 'Koop-Erkundungsquest gestartet! ⚔️' : 'Launched Co-op Scout Quest! ⚔️');

        setTimeout(() => {
          setActivePacket(null);
          triggerNotificationDev2(t.notifQuestComplete);
          setDev1Pet((prev) => ({
            ...prev,
            coins: prev.coins + 40,
            jobsCompletedCount: prev.jobsCompletedCount + 1,
          }));
          setDev2Pet((prev) => ({
            ...prev,
            coins: prev.coins + 40,
          }));
          addXPDev1(50);
        }, 800);
        break;
      }

      case 'open_trade':
        setTradePerspective('DEV1');
        setActiveModal('trade');
        break;

      case 'reset': {
        cydAudio.click();
        triggerNotificationDev1(t.notifRebooting);
        setLedDev1('red');
        setTimeout(() => setLedDev1('green'), 1200);
        break;
      }

      case 'boot': {
        cydAudio.click();
        triggerNotificationDev1('BOOT mode active (GPIO 0)');
        break;
      }
    }
  };

  // =========================================================================
  // ACTIONS DISPATCHER FOR DEVICE 2 (FRIEND)
  // =========================================================================
  const handleDev2Action = (action: string) => {
    switch (action) {
      case 'feed': {
        const firstFriendFood = dev2Pet.inventory.find((i) => i.type === 'food' && i.quantity > 0);
        if (firstFriendFood) {
          handleFeedWithItem('dev2', firstFriendFood.id);
        } else {
          cydAudio.warning();
          triggerNotificationDev2(t.emptyFoodMsg);
        }
        break;
      }

      case 'send_treat': {
        cydAudio.treatSend();
        setActivePacket({ from: 'DEV2', type: 'TREAT', label: '🍖 TREAT' });
        triggerNotificationDev2('Sending treat to CYD #1...');

        setTimeout(() => {
          cydAudio.treatReceive();
          setActivePacket(null);
          setLedDev1('cyan');
          triggerNotificationDev1('Friend sent you a treat! 🍩 +25 FD');
          setDev1Pet((prev) => ({
            ...prev,
            stats: {
              ...prev.stats,
              hunger: Math.min(100, prev.stats.hunger + 25),
              happiness: Math.min(100, prev.stats.happiness + 20),
            },
          }));
          setTimeout(() => setLedDev1('green'), 2500);
        }, 600);
        break;
      }

      case 'open_chat': {
        cydAudio.chatPing();
        setChatBalloonDev2('Hi! CYD looking good! ✨');
        setTimeout(() => setChatBalloonDev2(null), 5000);
        break;
      }

      case 'cast_magic': {
        handleCoopCast('DEV2');
        break;
      }

      case 'open_rpg':
        setRpgInitialTab('recipes');
        setActiveModal('rpg');
        break;

      case 'open_stats':
        setRpgInitialTab('stats');
        setActiveModal('rpg');
        break;

      case 'open_trade':
        setTradePerspective('DEV2');
        setActiveModal('trade');
        break;

      default:
        cydAudio.click();
        break;
    }
  };

  // =========================================================================
  // CO-OP DUAL-CAST HANDLER (10s SYNC WINDOW -> DUAL ASTRAL HARMONY)
  // =========================================================================
  const triggerDualAstralHarmony = (casterSide: 'DEV1' | 'DEV2', customManaCost = 25) => {
    setCoopCastWindow(null);
    setActivePacket({ from: casterSide, type: 'MAGIC_SYNC', label: '🌟 ASTRAL HARMONY' });
    setIsAuroraActive(true);
    if (auroraTimerRef.current) window.clearTimeout(auroraTimerRef.current);
    auroraTimerRef.current = window.setTimeout(() => {
      setIsAuroraActive(false);
    }, 4200);

    cydAudio.levelUp();
    setTimeout(() => cydAudio.magicCast(), 180);
    confetti({ particleCount: 110, spread: 85, origin: { y: 0.45 } });
    setTimeout(() => confetti({ particleCount: 70, spread: 70, origin: { y: 0.55 } }), 320);

    setLedDev1('gold');
    setLedDev2('cyan');
    setTimeout(() => {
      setLedDev1('cyan');
      setLedDev2('gold');
    }, 1200);
    setTimeout(() => {
      setActivePacket(null);
      setLedDev1('green');
      setLedDev2('green');
    }, 3800);

    // Choose a rare collectible gift or gourmet berry tart for each pet
    const pickCoopGift = (pet: PetState): InventoryItem => {
      const unownedRare = CATALOG_ITEMS.find(
        (c) => (c.rarity === 'rare' || c.rarity === 'legendary') && !pet.inventory.some((i) => i.id === c.id)
      );
      return unownedRare || FOOD_CATALOG.food_berry_pie;
    };

    const gift1 = pickCoopGift(dev1Pet);
    const gift2 = pickCoopGift(dev2Pet);

    setDev1Pet((prev) => {
      const manaSpent = casterSide === 'DEV1' ? Math.max(0, prev.stats.mana - customManaCost) : prev.stats.mana;
      const buffed: PetState = {
        ...prev,
        isSick: false,
        spellsCastCount: casterSide === 'DEV1' ? prev.spellsCastCount + 1 : prev.spellsCastCount,
        inventory: stackAddItem(prev.inventory, gift1, 1),
        stats: {
          ...prev.stats,
          mana: manaSpent,
          health: Math.min(100, prev.stats.health + 50),
          happiness: Math.min(100, prev.stats.happiness + 40),
        },
      };
      return applyXpToPetState(buffed, 100).pet;
    });

    setDev2Pet((prev) => {
      const manaSpent = casterSide === 'DEV2' ? Math.max(0, prev.stats.mana - customManaCost) : prev.stats.mana;
      const buffed: PetState = {
        ...prev,
        isSick: false,
        spellsCastCount: casterSide === 'DEV2' ? prev.spellsCastCount + 1 : prev.spellsCastCount,
        inventory: stackAddItem(prev.inventory, gift2, 1),
        stats: {
          ...prev.stats,
          mana: manaSpent,
          health: Math.min(100, prev.stats.health + 50),
          happiness: Math.min(100, prev.stats.happiness + 40),
        },
      };
      return applyXpToPetState(buffed, 100).pet;
    });

    recordMissionAction('cast_spell', 1);
    const gift1Name = language === 'de' ? (gift1.name_de || gift1.name) : gift1.name;
    const gift2Name = language === 'de' ? (gift2.name_de || gift2.name) : gift2.name;
    triggerNotificationDev1(`${t.coopHarmonyTriggered} (${gift1.icon} ${gift1Name})`);
    triggerNotificationDev2(`${t.coopHarmonyTriggered} (${gift2.icon} ${gift2Name})`);
  };

  const handleCoopCast = (device: 'DEV1' | 'DEV2', customManaCost = 25) => {
    const casterPet = device === 'DEV1' ? dev1Pet : dev2Pet;
    const setCasterPet = device === 'DEV1' ? setDev1Pet : setDev2Pet;
    const setPartnerPet = device === 'DEV1' ? setDev2Pet : setDev1Pet;
    const notifyCaster = device === 'DEV1' ? triggerNotificationDev1 : triggerNotificationDev2;
    const notifyPartner = device === 'DEV1' ? triggerNotificationDev2 : triggerNotificationDev1;
    const setCasterLed = device === 'DEV1' ? setLedDev1 : setLedDev2;
    const setPartnerLed = device === 'DEV1' ? setLedDev2 : setLedDev1;

    if (casterPet.stats.mana < customManaCost) {
      cydAudio.warning();
      notifyCaster(t.coopNeedMana);
      return;
    }

    const now = Date.now();
    // Case B: Partner already opened the 10s sync window -> Unleash Dual Astral Harmony!
    if (coopCastWindow && coopCastWindow.initiator !== device && now <= coopCastWindow.expiresAt) {
      triggerDualAstralHarmony(device, customManaCost);
      return;
    }

    // Case A: Open / refresh 10s Dual-Cast window + send immediate Friend Blessing wave
    cydAudio.magicCast();
    setCoopCastWindow({ initiator: device, expiresAt: now + 10000 });
    setActivePacket({ from: device, type: 'MAGIC_SYNC', label: '✨ MGC+ SYNC' });

    setCasterPet((prev) => {
      const updated: PetState = {
        ...prev,
        stats: { ...prev.stats, mana: Math.max(0, prev.stats.mana - customManaCost) },
        spellsCastCount: prev.spellsCastCount + 1,
      };
      return device === 'DEV2' ? applyXpToPetState(updated, 35).pet : updated;
    });
    if (device === 'DEV1') {
      addXPDev1(35);
      recordMissionAction('cast_spell', 1);
    }

    setCasterLed('gold');
    notifyCaster(t.coopSyncWaiting);

    setTimeout(() => {
      setActivePacket(null);
      setPartnerLed('cyan');
      confetti({ particleCount: 40, spread: 50 });
      setPartnerPet((prev) => ({
        ...prev,
        stats: {
          ...prev.stats,
          health: Math.min(100, prev.stats.health + 25),
          happiness: Math.min(100, prev.stats.happiness + 20),
        },
      }));
      notifyPartner(t.coopPartnerWaiting);
      setTimeout(() => {
        setCasterLed('green');
        setPartnerLed('green');
      }, 2200);
    }, 550);
  };

  // Recipe Cooking Handler
  const handleCookRecipe = (recipe: Recipe) => {
    setDev1Pet((prev) => {
      // Deduct ingredients
      const updatedInv = prev.inventory
        .map((item) => {
          const req = recipe.ingredients.find((ing) => ing.itemId === item.id);
          if (req) {
            return { ...item, quantity: Math.max(0, item.quantity - req.count) };
          }
          return item;
        })
        .filter((item) => item.quantity > 0);

      // Add result item
      const existing = updatedInv.find((i) => i.id === recipe.resultItem.id);
      if (existing) {
        existing.quantity += 1;
      } else {
        updatedInv.push({ ...recipe.resultItem, quantity: 1 });
      }

      return {
        ...prev,
        inventory: updatedInv,
        recipesCraftedCount: prev.recipesCraftedCount + 1,
      };
    });

    addXPDev1(recipe.xpReward);
    recordMissionAction('cook', 1);
    const cookedName = language === 'de' ? (recipe.name_de || recipe.name) : recipe.name;
    triggerNotificationDev1(
      language === 'de'
        ? `${cookedName} zubereitet! 🍳 +${recipe.xpReward} XP`
        : `Cooked ${cookedName}! 🍳 +${recipe.xpReward} XP`
    );
  };

  // Job Start Handler
  const handleStartJob = (job: Job) => {
    setDev1Pet((prev) => {
      if (prev.activeJob) return prev;
      return {
        ...prev,
        activeJob: {
          jobId: job.id,
          startedAt: Date.now(),
          durationSeconds: job.durationSeconds,
        },
      };
    });
    recordMissionAction('start_job', 1);
    const jobTitle = language === 'de' ? (job.title_de || job.title) : job.title;
    triggerNotificationDev1(
      language === 'de'
        ? `Beruf gestartet: ${jobTitle} (${job.durationSeconds}s) ⛏️`
        : `Dispatched to ${jobTitle} (${job.durationSeconds}s)! ⛏️`
    );
  };

  // Magic Spell Handler (Integrates target === 'both' Dual Astral Harmony with 10s Co-op Window)
  const handleCastSpell = (spell: MagicSpell) => {
    if (spell.target === 'both') {
      handleCoopCast('DEV1', spell.manaCost);
      return;
    }

    setDev1Pet((prev) => {
      const stats = { ...prev.stats, mana: Math.max(0, prev.stats.mana - spell.manaCost) };
      let nextSatiety = prev.satietyTicks || 0;
      let nextSick = prev.isSick;

      if (spell.target === 'self') {
        stats.health = Math.min(100, stats.health + (spell.healthGain ?? (spell.curesSickness ? 10 : 25)));
        if (spell.hungerGain) {
          stats.hunger = Math.min(100, stats.hunger + spell.hungerGain);
        }
        if (spell.happinessGain) {
          stats.happiness = Math.min(100, stats.happiness + spell.happinessGain);
        }
        if (spell.satietyTicks) {
          nextSatiety = Math.max(nextSatiety, spell.satietyTicks);
        }
        if (spell.curesSickness) {
          nextSick = false;
          stats.hygiene = 100;
        }
      }
      return {
        ...prev,
        stats,
        satietyTicks: nextSatiety,
        isSick: nextSick,
        spellsCastCount: prev.spellsCastCount + 1,
      };
    });

    const spellTitle = language === 'de' ? (spell.name_de || spell.name) : spell.name;

    if (spell.target === 'friend') {
      setActivePacket({ from: 'DEV1', type: 'MAGIC', label: '✨ MAGIC' });
      setTimeout(() => {
        setActivePacket(null);
        setDev2Pet((p) => ({
          ...p,
          stats: {
            ...p.stats,
            health: Math.min(100, p.stats.health + (spell.healthGain ?? 30)),
            happiness: Math.min(100, p.stats.happiness + (spell.happinessGain ?? 20)),
          },
        }));
        triggerNotificationDev2(
          language === 'de' ? `${spellTitle} empfangen! ✨` : `Received ${spellTitle}! ✨`
        );
      }, 600);
    }

    addXPDev1(35);
    recordMissionAction('cast_spell', 1);
    triggerNotificationDev1(
      spell.curesSickness && dev1Pet.isSick
        ? `${spellTitle}: ${t.curedSickness}`
        : language === 'de'
        ? `${spellTitle} gewirkt! ✨`
        : `Cast ${spellTitle}! ✨`
    );
  };

  // Potion / Medicine Item Usage Handler
  const handleUsePotionItem = (item: InventoryItem) => {
    const owned = dev1Pet.inventory.find((i) => i.id === item.id && i.quantity > 0);
    if (!owned) return;

    cydAudio.clean();
    setDev1Pet((prev) => {
      const updatedInv = prev.inventory
        .map((it) => (it.id === item.id ? { ...it, quantity: it.quantity - 1 } : it))
        .filter((it) => it.quantity > 0);

      const extraMana = item.id === 'item_energy_elixir' ? 15 : 0;

      return {
        ...prev,
        inventory: updatedInv,
        isSick: item.cureSickness ? false : prev.isSick,
        stats: {
          ...prev.stats,
          health: Math.min(100, prev.stats.health + (item.healthGain || 0)),
          energy: Math.min(100, prev.stats.energy + (item.energyGain || 0)),
          mana: Math.min(100, prev.stats.mana + extraMana),
        },
      };
    });

    const itemName = language === 'de' ? (item.name_de || item.name) : item.name;
    triggerNotificationDev1(
      item.cureSickness
        ? t.curedSickness
        : language === 'de'
        ? `${itemName} angewendet! ✨`
        : `Used ${itemName}! ✨`
    );
  };

  // Shop Purchase Handler
  const handleBuyShopItem = (shopEntry: ShopCatalogItem) => {
    if (dev1Pet.level < shopEntry.minLevel) {
      cydAudio.warning();
      triggerNotificationDev1(`${t.shopLevelReq}${shopEntry.minLevel}`);
      return;
    }
    if (dev1Pet.coins < shopEntry.priceCoins) {
      cydAudio.warning();
      triggerNotificationDev1(t.shopNotEnoughCoins);
      return;
    }

    cydAudio.fanfare();
    setDev1Pet((prev) => {
      const updatedInv = prev.inventory.map((it) => ({ ...it }));
      const existing = updatedInv.find((it) => it.id === shopEntry.item.id);
      if (existing) {
        existing.quantity += 1;
      } else {
        updatedInv.push({ ...shopEntry.item, quantity: 1 });
      }
      return {
        ...prev,
        coins: prev.coins - shopEntry.priceCoins,
        inventory: updatedInv,
      };
    });

    const purchasedName = language === 'de' ? (shopEntry.item.name_de || shopEntry.item.name) : shopEntry.item.name;
    triggerNotificationDev1(
      language === 'de'
        ? `Gekauft: ${purchasedName} (-${shopEntry.priceCoins} 🪙)`
        : `Purchased: ${purchasedName} (-${shopEntry.priceCoins} 🪙)`
    );
  };

  // Arcane Magic Research Handler
  const handleResearchMagic = () => {
    if (dev1Pet.stats.energy < 15) {
      triggerNotificationDev1(
        language === 'de'
          ? 'Nicht genug Energie zum Erforschen! (min. 15 EN)'
          : 'Not enough energy to research magic! (min. 15 EN)'
      );
      return;
    }
    cydAudio.magicCast();
    setDev1Pet((prev) => ({
      ...prev,
      stats: {
        ...prev.stats,
        energy: Math.max(0, prev.stats.energy - 15),
        mana: Math.min(100, prev.stats.mana + 15),
      },
      spellsCastCount: (prev.spellsCastCount || 0) + 1,
    }));
    addXPDev1(35);
    recordMissionAction('cast_spell', 1);
    confetti({ particleCount: 50, spread: 55, origin: { y: 0.55 } });
    triggerNotificationDev1(
      language === 'de'
        ? 'Arkane Magie erforscht! +35 XP ✨'
        : 'Arcane magic researched! +35 XP ✨'
    );
  };

  // Equipment Wardrobe Handlers
  const handleEquipItem = (item: InventoryItem) => {
    if (!item.slot) return;
    cydAudio.click();
    setDev1Pet((prev) => ({
      ...prev,
      equipped: {
        ...prev.equipped,
        [item.slot!]: item.id,
      },
    }));
    triggerNotificationDev1(`Equipped ${item.name}! 🎩`);
  };

  const handleUnequipItem = (slot: 'head' | 'face' | 'back' | 'environment') => {
    cydAudio.back();
    setDev1Pet((prev) => {
      const newEquipped = { ...prev.equipped };
      delete newEquipped[slot];
      return { ...prev, equipped: newEquipped };
    });
  };

  // Bidirectional Escrow Trading Handlers
  const handleSendTradeOffer = (
    fromSide: 'DEV1' | 'DEV2',
    offeredItem: InventoryItem,
    requestedItemName: string,
    requestedItemId?: string
  ) => {
    const senderPet = fromSide === 'DEV1' ? dev1Pet : dev2Pet;
    const receiverPet = fromSide === 'DEV1' ? dev2Pet : dev1Pet;
    const setSenderPet = fromSide === 'DEV1' ? setDev1Pet : setDev2Pet;
    const notifySender = fromSide === 'DEV1' ? triggerNotificationDev1 : triggerNotificationDev2;
    const notifyReceiver = fromSide === 'DEV1' ? triggerNotificationDev2 : triggerNotificationDev1;

    // Verify & lock offered item in escrow immediately
    const { inventory: lockedInv, equipped: updatedEquipped, removed } = deductInventoryItem(
      senderPet,
      offeredItem.id,
      1
    );
    if (!removed) {
      cydAudio.warning();
      notifySender(t.tradeEmptyBag);
      return;
    }

    setSenderPet((prev) => ({
      ...prev,
      inventory: lockedInv,
      equipped: updatedEquipped,
    }));

    const offer: TradeOffer = {
      id: `trade-${Date.now()}-${Math.random().toString(36).slice(2, 6)}`,
      fromDeviceId: senderPet.deviceId,
      fromPetName: senderPet.name,
      toDeviceId: receiverPet.deviceId,
      offeredItem: { ...offeredItem, quantity: 1 },
      requestedItemId: requestedItemId || '',
      requestedItemName,
      status: 'pending',
      timestamp: Date.now(),
    };

    setActivePacket({ from: fromSide, type: 'TRADE_OFFER', label: '🎁 TRADE OFFER' });
    notifySender(t.tradeEscrowedMsg);

    const itemName = language === 'de' ? (offeredItem.name_de || offeredItem.name) : offeredItem.name;
    setTimeout(() => {
      setActivePacket(null);
      cydAudio.tradeBell();
      setTradeOffers((prev) => [...prev, offer]);
      notifyReceiver(`${senderPet.name}: ${itemName} 🎁`);
    }, 550);
  };

  const handleAcceptTrade = (offer: TradeOffer) => {
    const fromSide: 'DEV1' | 'DEV2' = offer.fromDeviceId === dev1Pet.deviceId ? 'DEV1' : 'DEV2';
    const receiverSide: 'DEV1' | 'DEV2' = fromSide === 'DEV1' ? 'DEV2' : 'DEV1';
    const senderPet = fromSide === 'DEV1' ? dev1Pet : dev2Pet;
    const receiverPet = receiverSide === 'DEV1' ? dev1Pet : dev2Pet;
    const setSenderPet = fromSide === 'DEV1' ? setDev1Pet : setDev2Pet;
    const setReceiverPet = receiverSide === 'DEV1' ? setDev1Pet : setDev2Pet;
    const notifySender = fromSide === 'DEV1' ? triggerNotificationDev1 : triggerNotificationDev2;
    const notifyReceiver = receiverSide === 'DEV1' ? triggerNotificationDev1 : triggerNotificationDev2;

    // Validate receiver owns requested item (unless Free Gift)
    let matchedRequestItem: InventoryItem | undefined;
    if (offer.requestedItemId) {
      matchedRequestItem = receiverPet.inventory.find(
        (i) => i.id === offer.requestedItemId && i.quantity > 0
      );
      if (!matchedRequestItem) {
        cydAudio.warning();
        notifyReceiver(t.tradeMissingRequested);
        return;
      }
    }

    // Atomic escrow swap execution
    if (matchedRequestItem) {
      const { inventory: recAfterDeduct, equipped: recEquippedAfter } = deductInventoryItem(
        receiverPet,
        matchedRequestItem.id,
        1
      );
      const recFinalInv = stackAddItem(recAfterDeduct, offer.offeredItem, 1);

      setReceiverPet((prev) => {
        const updated: PetState = {
          ...prev,
          inventory: recFinalInv,
          equipped: recEquippedAfter,
        };
        return applyXpToPetState(updated, 35).pet;
      });

      setSenderPet((prev) => {
        const updated: PetState = {
          ...prev,
          inventory: stackAddItem(prev.inventory, matchedRequestItem!, 1),
        };
        return applyXpToPetState(updated, 35).pet;
      });
    } else {
      // Free gift escrow release: receiver keeps offered item, sender earns friendship XP
      setReceiverPet((prev) => {
        const updated: PetState = {
          ...prev,
          inventory: stackAddItem(prev.inventory, offer.offeredItem, 1),
        };
        return applyXpToPetState(updated, 35).pet;
      });

      setSenderPet((prev) => applyXpToPetState(prev, 35).pet);
    }

    setTradeOffers((prev) => prev.filter((o) => o.id !== offer.id));
    cydAudio.tradeBell();
    confetti({ particleCount: 95, spread: 70 });
    setActivePacket({ from: receiverSide, type: 'TRADE_RESPONSE', label: '🤝 SWAP OK' });
    setLedDev1('gold');
    setLedDev2('gold');
    setTimeout(() => {
      setActivePacket(null);
      setLedDev1('green');
      setLedDev2('green');
    }, 2400);

    const offName = language === 'de' ? (offer.offeredItem.name_de || offer.offeredItem.name) : offer.offeredItem.name;
    const reqName = matchedRequestItem
      ? language === 'de'
        ? (matchedRequestItem.name_de || matchedRequestItem.name)
        : matchedRequestItem.name
      : offer.requestedItemName;

    notifyReceiver(`${t.notifTradeAccepted} (+${offer.offeredItem.icon} ${offName}, +35 XP)`);
    notifySender(
      matchedRequestItem
        ? `${t.notifTradeAccepted} (+${matchedRequestItem.icon} ${reqName}, +35 XP)`
        : `${t.notifTradeAccepted} (+35 XP)`
    );
    void senderPet;
  };

  const handleDeclineTrade = (offer: TradeOffer) => {
    const fromSide: 'DEV1' | 'DEV2' = offer.fromDeviceId === dev1Pet.deviceId ? 'DEV1' : 'DEV2';
    const setSenderPet = fromSide === 'DEV1' ? setDev1Pet : setDev2Pet;
    const notifySender = fromSide === 'DEV1' ? triggerNotificationDev1 : triggerNotificationDev2;
    const notifyReceiver = fromSide === 'DEV1' ? triggerNotificationDev2 : triggerNotificationDev1;

    // Refund escrowed item back to proposer's bag
    setSenderPet((prev) => ({
      ...prev,
      inventory: stackAddItem(prev.inventory, offer.offeredItem, 1),
    }));

    cydAudio.back();
    setTradeOffers((prev) => prev.filter((o) => o.id !== offer.id));
    notifyReceiver(t.notifTradeDeclined);
    notifySender(t.tradeCancelledMsg);
  };

  const handleCancelTrade = (offer: TradeOffer) => {
    const fromSide: 'DEV1' | 'DEV2' = offer.fromDeviceId === dev1Pet.deviceId ? 'DEV1' : 'DEV2';
    const setSenderPet = fromSide === 'DEV1' ? setDev1Pet : setDev2Pet;
    const notifySender = fromSide === 'DEV1' ? triggerNotificationDev1 : triggerNotificationDev2;

    // Refund escrowed item back to sender
    setSenderPet((prev) => ({
      ...prev,
      inventory: stackAddItem(prev.inventory, offer.offeredItem, 1),
    }));

    cydAudio.back();
    setTradeOffers((prev) => prev.filter((o) => o.id !== offer.id));
    notifySender(t.tradeCancelledMsg);
  };

  // Chat Send Handler
  const handleSendMessage = (message: string) => {
    setActivePacket({ from: 'DEV1', type: 'CHAT', label: '💬 CHAT' });
    triggerNotificationDev1('Transmitting chat packet...');
    setChatBalloonDev1(message);
    setTimeout(() => setChatBalloonDev1(null), 3000);

    setTimeout(() => {
      setActivePacket(null);
      cydAudio.chatPing();
      setChatBalloonDev2(message);
      setLedDev2('blue');
      setTimeout(() => setLedDev2('green'), 4000);
      setTimeout(() => setChatBalloonDev2(null), 6000);
    }, 600);
  };

  // WiFi AP Web Portal Configuration Handler
  const handleSaveWiFiConfig = (cfg: {
    ssid: string;
    password: string;
    deviceId: string;
    trainerName: string;
    friendId: string;
    biome: BiomeType;
    serverUrl: string;
  }) => {
    setWifiConfig({
      ssid: cfg.ssid,
      deviceId: cfg.deviceId,
      trainerName: cfg.trainerName,
      friendId: cfg.friendId,
      serverUrl: cfg.serverUrl,
    });
    setDev1Pet((prev) => ({
      ...prev,
      deviceId: cfg.deviceId,
      trainerName: cfg.trainerName,
      biome: cfg.biome,
      name: BIOMES[cfg.biome]?.defaultPetName || prev.name,
    }));
    triggerNotificationDev1(`Saved to NVS! Connected to ${cfg.ssid} 📶`);
    setLedDev1('gold');
    setTimeout(() => setLedDev1('green'), 2500);
  };

  // Hot-Reload Game Mechanics Rules from .cfg Text File at Runtime (Including Recipes)
  const handleApplyRules = (newConfigText: string): boolean => {
    const success = engineRef.current.loadConfigText(newConfigText);
    if (success) {
      try {
        const parsed = engineRef.current.getRules().recipes;
        if (parsed && parsed.length > 0) {
          setActiveRecipes(parsed.map(convertParsedRecipeToRecipe));
        }
      } catch (e) {
        console.error('Failed to sync recipes from rules:', e);
      }
      triggerNotificationDev1(
        language === 'de'
          ? 'Spielregeln & Rezepte zur Laufzeit aktualisiert! ⚡'
          : 'Mechanics rules & recipes reloaded at runtime! ⚡'
      );
    }
    return success;
  };

  return (
    <div className="min-h-screen bg-slate-950 text-slate-100 flex flex-col font-sans-clean antialiased selection:bg-amber-500 selection:text-slate-950">
      {/* ================================================================= */}
      {/* TOP BAR CONTRACT:                                                 */}
      {/* 3 zones: [Brand wordmark] - [4 single-line nav links] - [Action]  */}
      {/* ================================================================= */}
      <header className="flex items-center justify-between gap-8 px-6 py-4 border-b border-slate-800 bg-slate-950/90 backdrop-blur sticky top-0 z-40">
        {/* Zone 1: Single text wordmark */}
        <div className="flex items-center gap-2.5 shrink-0">
          <span className="text-xl">👾</span>
          <div className="flex items-baseline gap-2">
            <span className="text-base font-bold tracking-tight text-white whitespace-nowrap">
              PetDuet
            </span>
            <span className="text-[10px] font-mono font-medium text-amber-400 bg-amber-950/60 border border-amber-800/60 px-1.5 py-0.5 rounded">
              Dual-Device RPG
            </span>
          </div>
        </div>

        {/* Zone 2: 4 concise single-line text links */}
        <nav className="hidden md:flex items-center gap-6 text-xs font-semibold text-slate-400">
          <button
            onClick={() => {
              cydAudio.click();
              setCurrentTab('dual_cyd');
            }}
            className={`transition-colors whitespace-nowrap shrink-0 hover:text-white ${
              currentTab === 'dual_cyd' ? 'text-amber-400 font-bold' : ''
            }`}
          >
            {t.navDualSim}
          </button>
          <button
            onClick={() => {
              cydAudio.click();
              setActiveModal('rpg');
            }}
            className="transition-colors whitespace-nowrap shrink-0 hover:text-white"
          >
            {t.navRpgWorkshop}
          </button>
          <button
            onClick={() => {
              cydAudio.click();
              setCurrentTab('firmware');
            }}
            className={`transition-colors whitespace-nowrap shrink-0 hover:text-white ${
              currentTab === 'firmware' ? 'text-amber-400 font-bold' : ''
            }`}
          >
            {t.navFirmware}
          </button>
          <button
            onClick={() => {
              cydAudio.click();
              setCurrentTab('php_server');
            }}
            className={`transition-colors whitespace-nowrap shrink-0 hover:text-white ${
              currentTab === 'php_server' ? 'text-amber-400 font-bold' : ''
            }`}
          >
            {t.navPhpServer}
          </button>
          <button
            onClick={() => {
              cydAudio.click();
              setCurrentTab('rules_editor');
            }}
            className={`transition-colors whitespace-nowrap shrink-0 hover:text-white ${
              currentTab === 'rules_editor' ? 'text-emerald-400 font-bold' : ''
            }`}
          >
            {t.navMechanicsCfg}
          </button>
        </nav>

        {/* Zone 3: Actions + Language Switcher */}
        <div className="flex items-center gap-2.5 shrink-0">
          {/* Language Switcher Toggle */}
          <button
            onClick={() => {
              cydAudio.click();
              const nextLang: Language = language === 'de' ? 'en' : 'de';
              setLanguage(nextLang);
              localStorage.setItem('cyd_language', nextLang);
            }}
            className="px-2.5 py-1.5 text-xs font-bold text-amber-300 bg-slate-900 hover:bg-slate-800 border border-slate-700 hover:border-amber-500/50 rounded-lg transition-colors whitespace-nowrap flex items-center gap-1.5 shadow-sm active:scale-95"
            title={language === 'de' ? 'Auf Englisch umschalten / Switch to English' : 'Auf Deutsch umschalten / Switch to German'}
          >
            <span>{language === 'de' ? '🇩🇪 DE' : '🇺🇸 EN'}</span>
          </button>

          <button
            onClick={() => {
              cydAudio.click();
              setActiveModal('wifi');
            }}
            className="px-3 py-1.5 text-xs font-semibold text-slate-200 bg-slate-800 hover:bg-slate-700 border border-slate-700 rounded-lg transition-colors whitespace-nowrap shrink-0 flex items-center gap-1.5"
            title="Open ESP32 SoftAP Web Configuration Portal (192.168.4.1)"
          >
            <span>📶</span> {t.btnWifiAp}
          </button>
          <button
            onClick={() => {
              const next = toggleAudio();
              setSoundOn(next);
            }}
            className="w-8 h-8 rounded-lg bg-slate-800 hover:bg-slate-700 text-slate-300 flex items-center justify-center text-xs transition-colors"
            title={soundOn ? 'Sound On' : 'Muted'}
          >
            {soundOn ? '🔊' : '🔇'}
          </button>
          <button
            onClick={() => {
              cydAudio.click();
              setActiveModal('leaderboard');
            }}
            className="px-3.5 py-2 text-xs font-semibold text-slate-950 bg-amber-500 hover:bg-amber-400 rounded-lg transition-colors whitespace-nowrap shrink-0 shadow-sm"
          >
            {t.btnLeaderboard}
          </button>
        </div>
      </header>

      {/* Main Container */}
      <main className="flex-1 max-w-7xl w-full mx-auto p-4 sm:p-6 lg:p-8">
        {/* TAB 1: DUAL CYD SIMULATOR */}
        {currentTab === 'dual_cyd' && (
          <DualDeviceView
            device1Pet={dev1Pet}
            device2Pet={dev2Pet}
            device1FriendSummary={dev1FriendSummary}
            device2FriendSummary={dev2FriendSummary}
            language={language}
            activePacket={activePacket}
            notificationDev1={notifDev1}
            notificationDev2={notifDev2}
            chatBalloonDev1={chatBalloonDev1}
            chatBalloonDev2={chatBalloonDev2}
            ledColorDev1={ledDev1}
            ledColorDev2={ledDev2}
            onDev1Action={handleDev1Action}
            onDev2Action={handleDev2Action}
            onDev1FeedItem={(itemId) => handleFeedWithItem('dev1', itemId)}
            onDev2FeedItem={(itemId) => handleFeedWithItem('dev2', itemId)}
            onOpenShop={() => {
              setRpgInitialTab('shop');
              setActiveModal('rpg');
            }}
            onDev1BiomeChange={(biome) => {
              cydAudio.click();
              setDev1Pet((prev) => ({
                ...prev,
                biome,
                name: BIOMES[biome].defaultPetName,
              }));
              triggerNotificationDev1(`${language === 'de' ? 'Biom gewechselt zu' : 'Switched to'} ${BIOMES[biome].name}!`);
            }}
            onDev2BiomeChange={(biome) => {
              cydAudio.click();
              setDev2Pet((prev) => ({
                ...prev,
                biome,
                name: BIOMES[biome].defaultPetName,
              }));
              triggerNotificationDev2(`${language === 'de' ? 'Freund wechselte zu' : 'Friend moved to'} ${BIOMES[biome].name}!`);
            }}
            onSeasonChange={(season) => {
              cydAudio.click();
              setDev1Pet((prev) => ({ ...prev, season }));
              setDev2Pet((prev) => ({ ...prev, season }));
              triggerNotificationDev1(`${language === 'de' ? 'Jahreszeit gewechselt zu' : 'Season changed to'} ${SEASONS[season].name}! ${SEASONS[season].icon}`);
            }}
            coopCastWindow={coopCastWindow}
            coopRemainingSec={coopRemainingSec}
            isAuroraActive={isAuroraActive}
            incomingTradeCountDev1={
              tradeOffers.filter((o) => o.toDeviceId === dev1Pet.deviceId && o.status === 'pending').length
            }
            incomingTradeCountDev2={
              tradeOffers.filter((o) => o.toDeviceId === dev2Pet.deviceId && o.status === 'pending').length
            }
          />
        )}

        {/* TAB 2: ESP32 FIRMWARE (ESP_CoGo CYD C++) */}
        {currentTab === 'firmware' && <FirmwareTab language={language} />}

        {/* TAB 3: PHP SERVER (register.php, sync.php, cloud_save.php) */}
        {currentTab === 'php_server' && <PHPServerTab myPet={dev1Pet} />}

        {/* TAB 4: GAME MECHANICS CONFIG STUDIO */}
        {currentTab === 'rules_editor' && (
          <RulesEditorTab
            currentConfigText={engineRef.current.getRawConfigText()}
            language={language}
            onApplyConfig={handleApplyRules}
          />
        )}
      </main>

      {/* Interactive Modals */}
      {activeModal === 'rpg' && (
        <RPGSubsystemsModal
          pet={dev1Pet}
          recipes={activeRecipes}
          language={language}
          initialTab={rpgInitialTab}
          onClose={() => setActiveModal(null)}
          onCookRecipe={handleCookRecipe}
          onStartJob={handleStartJob}
          onCastSpell={handleCastSpell}
          onResearchMagic={handleResearchMagic}
          onEquipItem={handleEquipItem}
          onUnequipItem={handleUnequipItem}
          onFeedItem={(item) => handleFeedWithItem('dev1', item.id)}
          onUseItem={handleUsePotionItem}
          onBuyShopItem={handleBuyShopItem}
          onClaimMission={handleClaimMission}
          onClaimStreakBonus={handleClaimStreakBonus}
          coopCastWindow={coopCastWindow}
          coopRemainingSec={coopRemainingSec}
          isAuroraActive={isAuroraActive}
          onTriggerCoopCast={() => handleCoopCast('DEV1')}
        />
      )}

      {activeModal === 'trade' && (
        <TradingModal
          dev1Pet={dev1Pet}
          dev2Pet={dev2Pet}
          activePerspective={tradePerspective}
          onChangePerspective={setTradePerspective}
          pendingOffers={tradeOffers}
          language={language}
          onClose={() => setActiveModal(null)}
          onSendTradeOffer={handleSendTradeOffer}
          onAcceptTrade={handleAcceptTrade}
          onDeclineTrade={handleDeclineTrade}
          onCancelTrade={handleCancelTrade}
        />
      )}

      {activeModal === 'chat' && (
        <ChatModal
          friendPet={dev1FriendSummary}
          language={language}
          onClose={() => setActiveModal(null)}
          onSendMessage={handleSendMessage}
        />
      )}

      {activeModal === 'leaderboard' && (
        <LeaderboardModal
          myPet={dev1Pet}
          language={language}
          onClose={() => setActiveModal(null)}
        />
      )}

      {activeModal === 'wifi' && (
        <WiFiAPModal
          currentSSID={wifiConfig.ssid}
          currentDeviceID={dev1Pet.deviceId}
          currentTrainerName={dev1Pet.trainerName}
          currentFriendID={wifiConfig.friendId}
          currentBiome={dev1Pet.biome}
          currentServerURL={wifiConfig.serverUrl}
          language={language}
          onClose={() => setActiveModal(null)}
          onSaveConfig={handleSaveWiFiConfig}
        />
      )}

      {/* Footer */}
      <footer className="border-t border-slate-800 py-6 px-6 text-center text-xs text-slate-500">
        <div className="flex flex-wrap items-center justify-center gap-3 font-mono">
          <span>PetDuet · ESP32-2432S028R Cheap Yellow Display Dual Companion</span>
          <span aria-hidden="true">·</span>
          <span>ILI9341 320x240 LCD</span>
          <span aria-hidden="true">·</span>
          <span>XPT2046 Touch</span>
          <span aria-hidden="true">·</span>
          <span>A/B OTA Updates</span>
          <span aria-hidden="true">·</span>
          <span>Direct Socket & Server Sync</span>
        </div>
      </footer>
    </div>
  );
}
