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
} from './utils/petLogic';
import { cydAudio, toggleAudio } from './utils/audio';
import { DualDeviceView } from './components/DualDeviceView';
import { RPGSubsystemsModal } from './components/RPGSubsystemsModal';
import { TradingModal } from './components/TradingModal';
import { ChatModal } from './components/ChatModal';
import { LeaderboardModal } from './components/LeaderboardModal';
import { WiFiAPModal } from './components/WiFiAPModal';
import { FirmwareTab } from './components/FirmwareTab';
import { PHPServerTab } from './components/PHPServerTab';
import { RulesEditorTab } from './components/RulesEditorTab';
import { TamagotchiCoreEngine } from './engine/TamagotchiCoreEngine';
import { convertParsedRecipeToRecipe } from './engine/ConfigParser';
import { RECIPES } from './utils/petLogic';
import { translations, Language } from './utils/i18n';

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
        return JSON.parse(saved);
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
        return JSON.parse(saved);
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

  // Pending Trade Offers
  const [tradeOffers, setTradeOffers] = useState<TradeOffer[]>([]);

  // Auto-save to LocalStorage
  useEffect(() => {
    localStorage.setItem('cyd_dev1_pet', JSON.stringify(dev1Pet));
  }, [dev1Pet]);

  useEffect(() => {
    localStorage.setItem('cyd_dev2_pet', JSON.stringify(dev2Pet));
  }, [dev2Pet]);

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

  // Lifecycle Game Loop (Hunger, Energy & Jobs)
  useEffect(() => {
    const interval = setInterval(() => {
      // 1. Check Dev 1 decay
      setDev1Pet((prev) => {
        if (prev.phase === 'Egg') return prev; // Eggs don't decay stats
        const newStats = { ...prev.stats };
        // Gradual decay
        if (newStats.hunger > 0) newStats.hunger = Math.max(0, newStats.hunger - 1);
        if (newStats.energy > 0) newStats.energy = Math.max(0, newStats.energy - 1);
        if (newStats.hunger < 20 && newStats.health > 0) newStats.health = Math.max(0, newStats.health - 1);

        // Natural mana regeneration
        if (newStats.mana < 100) newStats.mana = Math.min(100, newStats.mana + 2);

        // Job completion check
        let jobFinished = false;
        if (prev.activeJob) {
          const elapsed = (Date.now() - prev.activeJob.startedAt) / 1000;
          if (elapsed >= prev.activeJob.durationSeconds) {
            jobFinished = true;
          }
        }

        if (jobFinished) {
          cydAudio.levelUp();
          triggerNotificationDev1('Job Completed! +Coins & XP 🧺');
          return {
            ...prev,
            stats: newStats,
            activeJob: null,
            coins: prev.coins + 30,
            xp: prev.xp + 45,
            jobsCompletedCount: prev.jobsCompletedCount + 1,
          };
        }

        return { ...prev, stats: newStats };
      });

      // 2. Check Dev 2 decay
      setDev2Pet((prev) => {
        if (prev.phase === 'Egg') return prev;
        const newStats = { ...prev.stats };
        if (newStats.hunger > 0) newStats.hunger = Math.max(0, newStats.hunger - 1);
        if (newStats.energy > 0) newStats.energy = Math.max(0, newStats.energy - 1);
        if (newStats.mana < 100) newStats.mana = Math.min(100, newStats.mana + 2);
        return { ...prev, stats: newStats };
      });
    }, 8000);

    return () => clearInterval(interval);
  }, []);

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

  // Helper to add XP & check level up
  const addXPDev1 = (amount: number) => {
    setDev1Pet((prev) => {
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

      const newPhase = checkEvolution(currentLevel, prev.phase);

      if (leveledUp) {
        cydAudio.levelUp();
        confetti({ particleCount: 80, spread: 60, origin: { y: 0.6 } });
        triggerNotificationDev1(`LEVEL UP! Now Lv.${currentLevel} 🌟`);
        setLedDev1('gold');
        setTimeout(() => setLedDev1('green'), 3000);
      }

      return {
        ...prev,
        level: currentLevel,
        xp: currentXP,
        xpNext: nextReq,
        phase: newPhase,
      };
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

  // =========================================================================
  // ACTIONS DISPATCHER FOR DEVICE 1 (YOU)
  // =========================================================================

  const handleDev1Action = (action: string) => {
    switch (action) {
      case 'feed': {
        cydAudio.munch();
        setDev1Pet((prev) => ({
          ...prev,
          stats: {
            ...prev.stats,
            hunger: Math.min(100, prev.stats.hunger + 25),
            health: Math.min(100, prev.stats.health + 10),
          },
        }));
        triggerNotificationDev1(t.notifFed);
        addXPDev1(15);
        recordMissionAction('feed', 1);
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
      case 'feed':
        cydAudio.munch();
        setDev2Pet((prev) => ({
          ...prev,
          stats: { ...prev.stats, hunger: Math.min(100, prev.stats.hunger + 25) },
        }));
        triggerNotificationDev2('Friend fed their pet!');
        break;

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
        cydAudio.magicCast();
        triggerNotificationDev2('Friend casted Sun Ward! ☀️');
        setDev1Pet((prev) => ({
          ...prev,
          stats: { ...prev.stats, health: Math.min(100, prev.stats.health + 20) },
        }));
        break;
      }

      default:
        cydAudio.click();
        break;
    }
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
    setDev1Pet((prev) => ({
      ...prev,
      activeJob: {
        jobId: job.id,
        startedAt: Date.now(),
        durationSeconds: job.durationSeconds,
      },
    }));
    recordMissionAction('start_job', 1);
    triggerNotificationDev1(`Dispatched to ${job.title}! ⛏️`);
  };

  // Magic Spell Handler
  const handleCastSpell = (spell: MagicSpell) => {
    setDev1Pet((prev) => {
      const stats = { ...prev.stats, mana: prev.stats.mana - spell.manaCost };
      if (spell.target === 'self' || spell.target === 'both') {
        stats.health = Math.min(100, stats.health + 25);
      }
      return {
        ...prev,
        stats,
        spellsCastCount: prev.spellsCastCount + 1,
      };
    });

    if (spell.target === 'friend' || spell.target === 'both') {
      setActivePacket({ from: 'DEV1', type: 'MAGIC', label: '✨ MAGIC' });
      setTimeout(() => {
        setActivePacket(null);
        setDev2Pet((p) => ({
          ...p,
          stats: {
            ...p.stats,
            health: Math.min(100, p.stats.health + 30),
            happiness: Math.min(100, p.stats.happiness + 20),
          },
        }));
        triggerNotificationDev2(`Received ${spell.name}! ✨`);
      }, 600);
    }

    addXPDev1(35);
    recordMissionAction('cast_spell', 1);
    triggerNotificationDev1(`Cast ${spell.name}! ✨`);
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

  // Trading Handlers
  const handleSendTradeOffer = (offeredItem: InventoryItem, requestedItemName: string) => {
    const offer: TradeOffer = {
      id: `trade-${Date.now()}`,
      fromDeviceId: dev1Pet.deviceId,
      fromPetName: dev1Pet.name,
      toDeviceId: dev2Pet.deviceId,
      offeredItem,
      requestedItemName,
      status: 'pending',
      timestamp: Date.now(),
    };

    setActivePacket({ from: 'DEV1', type: 'TRADE', label: '🎁 TRADE' });
    triggerNotificationDev1('Transmitting trade proposal...');

    setTimeout(() => {
      setActivePacket(null);
      cydAudio.tradeBell();
      setTradeOffers((prev) => [...prev, offer]);
      triggerNotificationDev2(`Trade Offer: ${offeredItem.name}! 🎁`);
    }, 700);
  };

  const handleAcceptTrade = (offer: TradeOffer) => {
    cydAudio.tradeBell();
    confetti({ particleCount: 100, spread: 70 });

    // Find requested item in catalog or friend bag
    const requestedItem =
      CATALOG_ITEMS.find((c) => c.name === offer.requestedItemName) ||
      CATALOG_ITEMS[0];

    // Transfer items
    setDev1Pet((prev) => {
      // Remove offered item
      const inv = prev.inventory
        .map((i) => (i.id === offer.offeredItem.id ? { ...i, quantity: i.quantity - 1 } : i))
        .filter((i) => i.quantity > 0);
      // Add requested item
      inv.push({ ...requestedItem, quantity: 1 });
      return { ...prev, inventory: inv };
    });

    setDev2Pet((prev) => {
      const inv = [...prev.inventory];
      inv.push({ ...offer.offeredItem, quantity: 1 });
      return { ...prev, inventory: inv };
    });

    setTradeOffers((prev) => prev.filter((o) => o.id !== offer.id));
    triggerNotificationDev1(`Trade Completed! Received ${requestedItem.name} 🤝`);
    triggerNotificationDev2(`Trade Completed! Received ${offer.offeredItem.name} 🤝`);
  };

  const handleDeclineTrade = (offer: TradeOffer) => {
    cydAudio.back();
    setTradeOffers((prev) => prev.filter((o) => o.id !== offer.id));
    triggerNotificationDev1('Trade offer was declined.');
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
          onClose={() => setActiveModal(null)}
          onCookRecipe={handleCookRecipe}
          onStartJob={handleStartJob}
          onCastSpell={handleCastSpell}
          onEquipItem={handleEquipItem}
          onUnequipItem={handleUnequipItem}
          onClaimMission={handleClaimMission}
          onClaimStreakBonus={handleClaimStreakBonus}
        />

      )}

      {activeModal === 'trade' && (
        <TradingModal
          myPet={dev1Pet}
          friendPet={dev1FriendSummary}
          pendingOffers={tradeOffers}
          language={language}
          onClose={() => setActiveModal(null)}
          onSendTradeOffer={handleSendTradeOffer}
          onAcceptTrade={handleAcceptTrade}
          onDeclineTrade={handleDeclineTrade}
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
