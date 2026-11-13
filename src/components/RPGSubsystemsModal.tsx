import React, { useState, useEffect } from 'react';
import { PetState, Recipe, Job, MagicSpell, InventoryItem } from '../types/pet';
import { RECIPES, JOBS, SPELLS, CATALOG_ITEMS, SHOP_ITEMS_CATALOG, ShopCatalogItem } from '../utils/petLogic';
import { cydAudio } from '../utils/audio';
import { translations, Language } from '../utils/i18n';

export type RPGModalTab = 'stats' | 'recipes' | 'jobs' | 'magic' | 'inventory' | 'shop' | 'missions';

interface RPGSubsystemsModalProps {
  pet: PetState;
  recipes?: Recipe[];
  language?: Language;
  initialTab?: RPGModalTab;
  onClose: () => void;
  onCookRecipe: (recipe: Recipe) => void;
  onStartJob: (job: Job) => void;
  onCastSpell: (spell: MagicSpell) => void;
  onResearchMagic?: () => void;
  onEquipItem: (item: InventoryItem) => void;
  onUnequipItem: (slot: 'head' | 'face' | 'back' | 'environment') => void;
  onFeedItem?: (item: InventoryItem) => void;
  onUseItem?: (item: InventoryItem) => void;
  onBuyShopItem?: (shopItem: ShopCatalogItem) => void;
  onClaimMission?: (missionId: string) => void;
  onClaimStreakBonus?: () => void;
}

export const RPGSubsystemsModal: React.FC<RPGSubsystemsModalProps> = ({
  pet,
  recipes = RECIPES,
  language = 'de',
  initialTab = 'stats',
  onClose,
  onCookRecipe,
  onStartJob,
  onCastSpell,
  onResearchMagic,
  onEquipItem,
  onUnequipItem,
  onFeedItem,
  onUseItem,
  onBuyShopItem,
  onClaimMission,
  onClaimStreakBonus,
}) => {
  const t = translations[language] || translations.de;
  const [activeTab, setActiveTab] = useState<RPGModalTab>(initialTab);
  const [nowTs, setNowTs] = useState<number>(() => Date.now());

  useEffect(() => {
    setActiveTab(initialTab);
  }, [initialTab]);

  useEffect(() => {
    if (!pet.activeJob) return;
    setNowTs(Date.now());
    const id = setInterval(() => setNowTs(Date.now()), 1000);
    return () => clearInterval(id);
  }, [pet.activeJob]);

  const formatDurationLabel = (totalSec: number) => {
    const s = Math.max(0, Math.ceil(totalSec));
    const m = Math.floor(s / 60);
    const rem = s % 60;
    if (m === 0) return `${rem}s`;
    return `${m}:${String(rem).padStart(2, '0')} min`;
  };

  const formatCountdown = (totalSec: number) => {
    const s = Math.max(0, Math.ceil(totalSec));
    const m = Math.floor(s / 60);
    const rem = s % 60;
    return `${String(m).padStart(2, '0')}:${String(rem).padStart(2, '0')}`;
  };

  const activeJobInfo = pet.activeJob ? JOBS.find((j) => j.id === pet.activeJob!.jobId) : null;
  const activeJobRemainingSec = pet.activeJob
    ? Math.max(0, Math.ceil((pet.activeJob.startedAt + pet.activeJob.durationSeconds * 1000 - nowTs) / 1000))
    : 0;
  const activeJobProgressPct = pet.activeJob
    ? Math.min(100, Math.max(0, Math.round(((pet.activeJob.durationSeconds - activeJobRemainingSec) / Math.max(1, pet.activeJob.durationSeconds)) * 100)))
    : 0;

  // Streak calculations
  const today = new Date().toISOString().split('T')[0];
  const consecutiveDays = pet.dailyMissions?.consecutiveLoginDays || 1;
  const streakClaimedToday = pet.dailyMissions?.lastStreakClaimedDate === today;
  const canClaimStreak = !streakClaimedToday;

  const getStreakRewards = (day: number) => {
    const cycleDay = ((Math.max(1, day) - 1) % 7) + 1;
    switch (cycleDay) {
      case 1:
        return { coins: 15, xp: 10 };
      case 2:
        return { coins: 25, xp: 15 };
      case 3:
        return { coins: 40, xp: 25 };
      case 4:
        return { coins: 60, xp: 35 };
      case 5:
        return { coins: 85, xp: 50 };
      case 6:
        return { coins: 115, xp: 65 };
      case 7:
      default: {
        const extraWeeks = Math.floor((Math.max(1, day) - 1) / 7);
        const bonus = Math.min(100, extraWeeks * 15);
        return { coins: 150 + bonus, xp: 100 };
      }
    }
  };

  const currentStreakReward = getStreakRewards(consecutiveDays);
  const nextStreakReward = getStreakRewards(consecutiveDays + 1);
  const cycleDay = ((consecutiveDays - 1) % 7) + 1;

  // Count uncompleted or claimable missions for badge
  const claimableMissionsCount = pet.dailyMissions?.missions.filter((m) => m.completed && !m.claimed).length || 0;
  const totalBadges = claimableMissionsCount + (canClaimStreak ? 1 : 0);

  // Helper to count item quantity in inventory
  const getItemCount = (itemId: string) => {
    const item = pet.inventory.find((i) => i.id === itemId);
    return item ? item.quantity : 0;
  };

  return (
    <div className="fixed inset-0 z-50 bg-black/80 backdrop-blur-sm flex items-center justify-center p-3 font-sans-clean">
      <div className="w-full max-w-xl bg-slate-900 border border-slate-700 rounded-xl shadow-2xl flex flex-col overflow-hidden max-h-[90vh]">
        {/* Modal Top Bar */}
        <div className="flex items-center justify-between px-5 py-3.5 bg-slate-950 border-b border-slate-800">
          <div className="flex items-center gap-2 sm:gap-3 flex-wrap">
            <span className="font-pixel text-xs text-amber-400">{t.rpgTitle}</span>
            <span className="text-xs text-slate-400 font-mono">Lv.{pet.level} · {pet.name}</span>
            {/* Top Bar Daily Streak Badge */}
            <div
              className={`flex items-center gap-1 px-2 py-0.5 rounded-full text-[10px] font-mono font-bold border transition-all ${
                canClaimStreak
                  ? 'bg-amber-500/20 text-amber-300 border-amber-500/50 shadow-sm shadow-amber-500/20'
                  : 'bg-slate-800/80 text-slate-400 border-slate-700'
              }`}
            >
              <span>🔥</span>
              <span>{consecutiveDays}d {t.loginStreak}</span>
              {canClaimStreak && (
                <span className="w-1.5 h-1.5 rounded-full bg-amber-400 animate-pulse ml-0.5" />
              )}
            </div>
          </div>
          <button
            onClick={() => {
              cydAudio.back();
              onClose();
            }}
            className="w-7 h-7 flex items-center justify-center rounded-lg bg-slate-800 hover:bg-slate-700 text-slate-300 font-bold transition-colors cursor-pointer"
          >
            ✕
          </button>
        </div>


        {/* Tab Selector */}
        <div className="flex border-b border-slate-800 bg-slate-950/60 p-1.5 gap-1 text-xs font-medium overflow-x-auto">
          <button
            onClick={() => {
              cydAudio.click();
              setActiveTab('stats');
            }}
            className={`flex-1 min-w-[70px] py-2 px-2.5 rounded-lg flex items-center justify-center gap-1.5 transition-colors ${
              activeTab === 'stats'
                ? 'bg-cyan-500/20 text-cyan-300 border border-cyan-500/40'
                : 'text-slate-400 hover:text-slate-200'
            }`}
          >
            <span>📊</span> {t.tabStats}
          </button>
          <button
            onClick={() => {
              cydAudio.click();
              setActiveTab('recipes');
            }}
            className={`flex-1 min-w-[70px] py-2 px-2.5 rounded-lg flex items-center justify-center gap-1.5 transition-colors ${
              activeTab === 'recipes'
                ? 'bg-amber-500/20 text-amber-300 border border-amber-500/40'
                : 'text-slate-400 hover:text-slate-200'
            }`}
          >
            <span>🍳</span> {t.tabRecipes}
          </button>
          <button
            onClick={() => {
              cydAudio.click();
              setActiveTab('jobs');
            }}
            className={`flex-1 min-w-[70px] py-2 px-2.5 rounded-lg flex items-center justify-center gap-1.5 transition-colors ${
              activeTab === 'jobs'
                ? 'bg-emerald-500/20 text-emerald-300 border border-emerald-500/40'
                : 'text-slate-400 hover:text-slate-200'
            }`}
          >
            <span>🧺</span> {t.tabJobs}
          </button>
          <button
            onClick={() => {
              cydAudio.click();
              setActiveTab('magic');
            }}
            className={`flex-1 min-w-[70px] py-2 px-2.5 rounded-lg flex items-center justify-center gap-1.5 transition-colors ${
              activeTab === 'magic'
                ? 'bg-purple-500/20 text-purple-300 border border-purple-500/40'
                : 'text-slate-400 hover:text-slate-200'
            }`}
          >
            <span>✨</span> {t.tabMagic}
          </button>
          <button
            onClick={() => {
              cydAudio.click();
              setActiveTab('inventory');
            }}
            className={`flex-1 min-w-[70px] py-2 px-2.5 rounded-lg flex items-center justify-center gap-1.5 transition-colors ${
              activeTab === 'inventory'
                ? 'bg-sky-500/20 text-sky-300 border border-sky-500/40'
                : 'text-slate-400 hover:text-slate-200'
            }`}
          >
            <span>🎒</span> {t.tabGear}
          </button>
          <button
            onClick={() => {
              cydAudio.click();
              setActiveTab('shop');
            }}
            className={`flex-1 min-w-[70px] py-2 px-2.5 rounded-lg flex items-center justify-center gap-1.5 transition-colors ${
              activeTab === 'shop'
                ? 'bg-emerald-500/20 text-emerald-300 border border-emerald-500/40'
                : 'text-slate-400 hover:text-slate-200'
            }`}
          >
            <span>🛒</span> {t.tabShop}
          </button>
          <button
            onClick={() => {
              cydAudio.click();
              setActiveTab('missions');
            }}
            className={`flex-1 min-w-[75px] py-2 px-2.5 rounded-lg flex items-center justify-center gap-1.5 transition-colors relative ${
              activeTab === 'missions'
                ? 'bg-rose-500/20 text-rose-300 border border-rose-500/40'
                : 'text-slate-400 hover:text-slate-200'
            }`}
          >
            <span>🎯</span> {t.tabMissions}
            {totalBadges > 0 && (
              <span className="w-2 h-2 rounded-full bg-rose-500 absolute top-1.5 right-1.5 animate-pulse" />
            )}
          </button>

        </div>

        {/* Content Body */}
        <div className="p-4 sm:p-5 overflow-y-auto flex-1 space-y-4">
          {/* TAB 0: ALL STATS & OVERVIEW */}
          {activeTab === 'stats' && (
            <div className="space-y-4">
              {/* Pet Overview & Auto-Save Banner */}
              <div className="bg-slate-950 p-4 rounded-xl border border-slate-800 flex flex-wrap items-center justify-between gap-3">
                <div className="flex items-center gap-3">
                  <div className="w-12 h-12 rounded-xl bg-slate-900 border border-slate-700 flex items-center justify-center text-2xl shadow-inner">
                    {pet.phase === 'Egg' ? '🥚' : pet.phase === 'Baby' ? '🐣' : pet.phase === 'Youth' ? '🐾' : pet.phase === 'Adult' ? '🦊' : '🐉'}
                  </div>
                  <div>
                    <div className="flex items-center gap-2">
                      <span className="font-bold text-slate-100 text-base">{pet.name}</span>
                      <span className="px-1.5 py-0.5 rounded bg-amber-500/20 text-amber-300 border border-amber-500/40 text-xs font-mono font-bold">
                        Lv.{pet.level}
                      </span>
                      <span className="px-1.5 py-0.5 rounded bg-cyan-500/20 text-cyan-300 border border-cyan-500/40 text-[10px] uppercase font-bold">
                        {pet.phase}
                      </span>
                    </div>
                    <div className="text-xs text-slate-400 mt-0.5 flex items-center gap-2">
                      <span className="capitalize text-slate-300 font-medium">Biom: {pet.biome}</span>
                      <span>·</span>
                      <span className="capitalize text-slate-300 font-medium">Saison: {pet.season}</span>
                      <span>·</span>
                      <span className="text-amber-400 font-bold">🪙 {pet.coins}</span>
                    </div>
                  </div>
                </div>

                {/* Auto-Save Indicator Badge */}
                <div className="flex items-center gap-2 px-3 py-1.5 rounded-lg bg-emerald-950/80 border border-emerald-600/70 text-emerald-300 text-xs font-mono shadow-sm">
                  <span className="w-2 h-2 rounded-full bg-emerald-400 animate-pulse" />
                  <span>⚡ {t.autoSaveIndicator}</span>
                </div>
              </div>

              {/* EXP Progression Bar */}
              <div className="bg-slate-950 p-3 rounded-xl border border-slate-800">
                <div className="flex items-center justify-between text-xs text-slate-300 mb-1.5">
                  <span className="font-semibold text-purple-300 flex items-center gap-1">
                    <span>✨</span> {t.statXp}
                  </span>
                  <span className="font-mono text-purple-200">
                    {pet.xp} / {pet.xpNext} XP ({Math.min(100, Math.round((pet.xp / pet.xpNext) * 100))}%)
                  </span>
                </div>
                <div className="w-full h-2.5 bg-slate-900 rounded-full overflow-hidden border border-slate-800">
                  <div
                    className="h-full bg-gradient-to-r from-purple-600 to-indigo-500 transition-all duration-300"
                    style={{ width: `${Math.min(100, Math.round((pet.xp / pet.xpNext) * 100))}%` }}
                  />
                </div>
              </div>

              {/* Complete 6 Vitals Grid */}
              <div className="grid grid-cols-1 sm:grid-cols-2 gap-3">
                {/* Health / HP */}
                <div className="bg-slate-950/80 p-3 rounded-xl border border-slate-800">
                  <div className="flex items-center justify-between text-xs mb-1.5">
                    <span className="text-rose-400 font-semibold flex items-center gap-1">
                      <span>❤️</span> {t.statHealth}
                    </span>
                    <span className="font-mono text-rose-300">{Math.round(pet.stats.health)}%</span>
                  </div>
                  <div className="w-full h-2 bg-slate-900 rounded-full overflow-hidden">
                    <div
                      className="h-full bg-rose-500 transition-all duration-300"
                      style={{ width: `${Math.min(100, Math.max(0, pet.stats.health))}%` }}
                    />
                  </div>
                </div>

                {/* Hunger / Food */}
                <div className="bg-slate-950/80 p-3 rounded-xl border border-slate-800">
                  <div className="flex items-center justify-between text-xs mb-1.5">
                    <span className="text-amber-400 font-semibold flex items-center gap-1">
                      <span>🍖</span> {t.statHunger}
                    </span>
                    <span className="font-mono text-amber-300">{Math.round(pet.stats.hunger)}%</span>
                  </div>
                  <div className="w-full h-2 bg-slate-900 rounded-full overflow-hidden">
                    <div
                      className="h-full bg-amber-500 transition-all duration-300"
                      style={{ width: `${Math.min(100, Math.max(0, pet.stats.hunger))}%` }}
                    />
                  </div>
                </div>

                {/* Happiness */}
                <div className="bg-slate-950/80 p-3 rounded-xl border border-slate-800">
                  <div className="flex items-center justify-between text-xs mb-1.5">
                    <span className="text-pink-400 font-semibold flex items-center gap-1">
                      <span>😊</span> {t.statHappiness}
                    </span>
                    <span className="font-mono text-pink-300">{Math.round(pet.stats.happiness)}%</span>
                  </div>
                  <div className="w-full h-2 bg-slate-900 rounded-full overflow-hidden">
                    <div
                      className="h-full bg-pink-500 transition-all duration-300"
                      style={{ width: `${Math.min(100, Math.max(0, pet.stats.happiness))}%` }}
                    />
                  </div>
                </div>

                {/* Energy */}
                <div className="bg-slate-950/80 p-3 rounded-xl border border-slate-800">
                  <div className="flex items-center justify-between text-xs mb-1.5">
                    <span className="text-yellow-400 font-semibold flex items-center gap-1">
                      <span>⚡</span> {t.statEnergy}
                    </span>
                    <span className="font-mono text-yellow-300">{Math.round(pet.stats.energy)}%</span>
                  </div>
                  <div className="w-full h-2 bg-slate-900 rounded-full overflow-hidden">
                    <div
                      className="h-full bg-yellow-500 transition-all duration-300"
                      style={{ width: `${Math.min(100, Math.max(0, pet.stats.energy))}%` }}
                    />
                  </div>
                </div>

                {/* Hygiene / Clean */}
                <div className="bg-slate-950/80 p-3 rounded-xl border border-slate-800">
                  <div className="flex items-center justify-between text-xs mb-1.5">
                    <span className="text-teal-400 font-semibold flex items-center gap-1">
                      <span>🫧</span> {t.statHygiene}
                    </span>
                    <span className="font-mono text-teal-300">{Math.round(pet.stats.hygiene)}%</span>
                  </div>
                  <div className="w-full h-2 bg-slate-900 rounded-full overflow-hidden">
                    <div
                      className="h-full bg-teal-500 transition-all duration-300"
                      style={{ width: `${Math.min(100, Math.max(0, pet.stats.hygiene))}%` }}
                    />
                  </div>
                </div>

                {/* Mana */}
                <div className="bg-slate-950/80 p-3 rounded-xl border border-slate-800">
                  <div className="flex items-center justify-between text-xs mb-1.5">
                    <span className="text-purple-400 font-semibold flex items-center gap-1">
                      <span>🔮</span> {t.statMana}
                    </span>
                    <span className="font-mono text-purple-300">{Math.round(pet.stats.mana)}%</span>
                  </div>
                  <div className="w-full h-2 bg-slate-900 rounded-full overflow-hidden">
                    <div
                      className="h-full bg-purple-500 transition-all duration-300"
                      style={{ width: `${Math.min(100, Math.max(0, pet.stats.mana))}%` }}
                    />
                  </div>
                </div>
              </div>

              {/* Satiety Shield & Health Condition Cards */}
              <div className="grid grid-cols-1 sm:grid-cols-2 gap-3">
                <div className={`p-3 rounded-xl border flex items-center justify-between ${
                  (pet.satietyTicks || 0) > 0
                    ? 'bg-emerald-950/40 border-emerald-500/40 text-emerald-300'
                    : 'bg-slate-950/80 border-slate-800 text-slate-400'
                }`}>
                  <div className="flex items-center gap-2.5">
                    <span className="text-xl">🛡️</span>
                    <div>
                      <div className="text-xs font-bold">{t.satietyLabel}</div>
                      <div className="text-[10px] opacity-80">
                        {(pet.satietyTicks || 0) > 0
                          ? (language === 'de' ? 'Kein Hunger-Abzug aktiv!' : 'Hunger decay paused!')
                          : (language === 'de' ? 'Kochgerichte aktivieren Schutz' : 'Cooked meals pause decay')}
                      </div>
                    </div>
                  </div>
                  <span className="text-xs font-mono font-bold">
                    {pet.satietyTicks || 0} {language === 'de' ? 'Ticks' : 'Ticks'}
                  </span>
                </div>

                <div className={`p-3 rounded-xl border flex items-center justify-between ${
                  pet.isSick
                    ? 'bg-rose-950/50 border-rose-500/60 text-rose-300 animate-pulse'
                    : 'bg-slate-950/80 border-slate-800 text-emerald-400'
                }`}>
                  <div className="flex items-center gap-2.5">
                    <span className="text-xl">{pet.isSick ? '🤢' : '💚'}</span>
                    <div>
                      <div className="text-xs font-bold">
                        {pet.isSick ? t.sickStatus : t.healthyStatus}
                      </div>
                      <div className="text-[10px] text-slate-400">
                        {pet.isSick
                          ? (language === 'de' ? 'Heile mit Heilsalbe / Läuterung!' : 'Cure via Healing Salve or Purify!')
                          : (language === 'de' ? 'Keine Bauchschmerzen' : 'No tummyache')}
                      </div>
                    </div>
                  </div>
                  {pet.isSick && (
                    <button
                      onClick={() => {
                        cydAudio.click();
                        setActiveTab('shop');
                      }}
                      className="text-[10px] px-2 py-1 rounded bg-rose-600 hover:bg-rose-500 text-white font-bold"
                    >
                      {language === 'de' ? 'Zur Medizin' : 'Get Cure'}
                    </button>
                  )}
                </div>
              </div>

              {/* Active Job Remaining Duration Card in Status Tab */}
              <div className={`p-3.5 rounded-xl border ${
                pet.activeJob && activeJobInfo
                  ? 'bg-emerald-950/30 border-emerald-500/50 text-emerald-200'
                  : 'bg-slate-950/80 border-slate-800 text-slate-400'
              }`}>
                <div className="flex items-center justify-between gap-3">
                  <div className="flex items-center gap-2.5">
                    <span className="text-xl">{activeJobInfo ? activeJobInfo.icon : '🧺'}</span>
                    <div>
                      <div className="text-xs font-bold text-slate-200 flex items-center gap-2">
                        <span>
                          {activeJobInfo
                            ? (language === 'de' ? (activeJobInfo.title_de || activeJobInfo.title) : activeJobInfo.title)
                            : (language === 'de' ? 'Aktiver Beruf' : 'Active Job')}
                        </span>
                        {pet.activeJob && (
                          <span className="px-1.5 py-0.5 rounded bg-emerald-500/20 border border-emerald-500/40 text-emerald-300 text-[10px] font-mono">
                            {language === 'de' ? 'Läuft' : 'Active'}
                          </span>
                        )}
                      </div>
                      <div className="text-[10px] text-slate-400 mt-0.5">
                        {pet.activeJob && activeJobInfo
                          ? (language === 'de'
                              ? `Belohnung: +${activeJobInfo.coinReward} 🪙 & +${activeJobInfo.xpReward} XP`
                              : `Reward: +${activeJobInfo.coinReward} 🪙 & +${activeJobInfo.xpReward} XP`)
                          : (language === 'de'
                              ? 'Derzeit kein Beruf aktiv — starte einen im Tab „Berufe“'
                              : 'No job running — send your pet to work in Jobs')}
                      </div>
                    </div>
                  </div>
                  {pet.activeJob ? (
                    <div className="text-right shrink-0">
                      <div className="text-sm font-mono font-bold text-emerald-300">
                        ⏱️ {formatCountdown(activeJobRemainingSec)}
                      </div>
                      <div className="text-[10px] text-slate-400 font-mono">
                        {language === 'de' ? 'Restzeit' : 'Remaining'}
                      </div>
                    </div>
                  ) : (
                    <button
                      onClick={() => {
                        cydAudio.click();
                        setActiveTab('jobs');
                      }}
                      className="text-[10px] px-2.5 py-1 rounded-lg bg-emerald-600/80 hover:bg-emerald-500 text-slate-950 font-bold transition-colors shrink-0"
                    >
                      {t.tabJobs}
                    </button>
                  )}
                </div>
                {pet.activeJob && (
                  <div className="mt-2.5 w-full h-1.5 bg-slate-900 rounded-full overflow-hidden border border-slate-800">
                    <div
                      className="h-full bg-emerald-500 transition-all duration-500"
                      style={{ width: `${activeJobProgressPct}%` }}
                    />
                  </div>
                )}
              </div>

              {/* Login Streak & Activity Summary */}
              <div className="grid grid-cols-1 sm:grid-cols-2 gap-3">
                {/* Streak Card */}
                <div className="bg-slate-950 p-3.5 rounded-xl border border-slate-800 flex items-center justify-between">
                  <div className="flex items-center gap-2.5">
                    <div className="w-9 h-9 rounded-lg bg-amber-500/10 border border-amber-500/30 flex items-center justify-center text-lg">
                      🔥
                    </div>
                    <div>
                      <div className="text-xs font-bold text-amber-300">{consecutiveDays} {t.daysStreak}</div>
                      <div className="text-[10px] text-slate-400">{t.dailyLoginStreak}</div>
                    </div>
                  </div>
                  {canClaimStreak && onClaimStreakBonus ? (
                    <button
                      onClick={() => {
                        cydAudio.click();
                        onClaimStreakBonus();
                      }}
                      className="px-2.5 py-1.5 rounded-lg bg-amber-500 hover:bg-amber-400 text-slate-950 text-xs font-bold transition-all active:scale-95 shadow-sm"
                    >
                      {t.btnClaimStreakBonus}
                    </button>
                  ) : (
                    <span className="text-[10px] text-emerald-400 font-mono">
                      ✓ {t.streakClaimedToday}
                    </span>
                  )}
                </div>

                {/* Game Stats Counters */}
                <div className="bg-slate-950 p-3.5 rounded-xl border border-slate-800 flex items-center justify-around text-center">
                  <div>
                    <div className="text-xs font-bold text-slate-200">🧺 {pet.jobsCompletedCount || 0}</div>
                    <div className="text-[10px] text-slate-400">{t.tabJobs}</div>
                  </div>
                  <div className="w-px h-6 bg-slate-800" />
                  <div>
                    <div className="text-xs font-bold text-slate-200">✨ {pet.spellsCastCount || 0}</div>
                    <div className="text-[10px] text-slate-400">{t.tabMagic}</div>
                  </div>
                  <div className="w-px h-6 bg-slate-800" />
                  <div>
                    <div className="text-xs font-bold text-slate-200">🎒 {pet.inventory.length}</div>
                    <div className="text-[10px] text-slate-400">{t.tabGear}</div>
                  </div>
                </div>
              </div>

              {/* Sub-Screen Shortcuts */}
              <div className="pt-2">
                <div className="text-[11px] font-semibold text-slate-400 mb-2 uppercase tracking-wider font-mono">
                  {language === 'de' ? 'Schnellzugriff auf Subsysteme:' : 'Subsystem Shortcuts:'}
                </div>
                <div className="grid grid-cols-2 sm:grid-cols-5 gap-2">
                  <button
                    onClick={() => {
                      cydAudio.click();
                      setActiveTab('inventory');
                    }}
                    className="p-2.5 rounded-xl bg-slate-950 border border-slate-800 hover:border-sky-500/50 flex flex-col items-center justify-center text-center transition-all group"
                  >
                    <span className="text-xl group-hover:scale-110 transition-transform">🎒</span>
                    <span className="text-xs text-slate-300 font-medium mt-1">{t.tabGear}</span>
                    <span className="text-[10px] text-slate-500">{pet.inventory.length} {language === 'de' ? 'Objekte' : 'Items'}</span>
                  </button>

                  <button
                    onClick={() => {
                      cydAudio.click();
                      setActiveTab('jobs');
                    }}
                    className="p-2.5 rounded-xl bg-slate-950 border border-slate-800 hover:border-emerald-500/50 flex flex-col items-center justify-center text-center transition-all group"
                  >
                    <span className="text-xl group-hover:scale-110 transition-transform">🧺</span>
                    <span className="text-xs text-slate-300 font-medium mt-1">{t.tabJobs}</span>
                    <span className="text-[10px] text-slate-500">{JOBS.length} {language === 'de' ? 'Berufe' : 'Jobs'}</span>
                  </button>

                  <button
                    onClick={() => {
                      cydAudio.click();
                      setActiveTab('magic');
                    }}
                    className="p-2.5 rounded-xl bg-slate-950 border border-slate-800 hover:border-purple-500/50 flex flex-col items-center justify-center text-center transition-all group"
                  >
                    <span className="text-xl group-hover:scale-110 transition-transform">✨</span>
                    <span className="text-xs text-slate-300 font-medium mt-1">{t.tabMagic}</span>
                    <span className="text-[10px] text-slate-500">{SPELLS.length} {language === 'de' ? 'Zauber' : 'Spells'}</span>
                  </button>

                  <button
                    onClick={() => {
                      cydAudio.click();
                      setActiveTab('recipes');
                    }}
                    className="p-2.5 rounded-xl bg-slate-950 border border-slate-800 hover:border-amber-500/50 flex flex-col items-center justify-center text-center transition-all group"
                  >
                    <span className="text-xl group-hover:scale-110 transition-transform">🍳</span>
                    <span className="text-xs text-slate-300 font-medium mt-1">{t.tabRecipes}</span>
                    <span className="text-[10px] text-slate-500">{recipes.length} {language === 'de' ? 'Rezepte' : 'Recipes'}</span>
                  </button>

                  <button
                    onClick={() => {
                      cydAudio.click();
                      setActiveTab('shop');
                    }}
                    className="p-2.5 rounded-xl bg-slate-950 border border-slate-800 hover:border-emerald-500/50 flex flex-col items-center justify-center text-center transition-all group col-span-2 sm:col-span-1"
                  >
                    <span className="text-xl group-hover:scale-110 transition-transform">🛒</span>
                    <span className="text-xs text-slate-300 font-medium mt-1">{t.tabShop}</span>
                    <span className="text-[10px] text-amber-400 font-mono">🪙 {pet.coins}</span>
                  </button>
                </div>
              </div>
            </div>
          )}

          {/* TAB 1: RECIPES (Configurable & Bilingual) */}
          {activeTab === 'recipes' && (
            <div className="space-y-3">
              <div className="flex items-center justify-between text-xs text-slate-400">
                <span>
                  {language === 'de'
                    ? 'Koche nahrhafte Biom-Mahlzeiten aus deiner .cfg-Konfiguration für Werte & XP.'
                    : "Cook nutritious biome dishes configured from your .cfg rules for stats & XP."}
                </span>
                <span className="text-[10px] font-mono text-emerald-400 bg-emerald-950/60 border border-emerald-800/80 px-2 py-0.5 rounded">
                  {recipes.length} {language === 'de' ? 'Rezepte geladen' : 'Recipes active'}
                </span>
              </div>
              <div className="grid grid-cols-1 sm:grid-cols-2 gap-3">
                {recipes.map((recipe) => {
                  const isLocked = pet.level < recipe.minLevel;
                  const hasIngredients = recipe.ingredients.every(
                    (ing) => getItemCount(ing.itemId) >= ing.count
                  );
                  const recipeTitle = language === 'de' ? (recipe.name_de || recipe.name) : recipe.name;
                  const recipeDesc = language === 'de' ? (recipe.description_de || recipe.description) : recipe.description;

                  return (
                    <div
                      key={recipe.id}
                      className={`p-3.5 rounded-xl border flex flex-col justify-between ${
                        isLocked
                          ? 'bg-slate-900/50 border-slate-800 opacity-60'
                          : 'bg-slate-800/80 border-slate-700/80 hover:border-amber-500/50'
                      }`}
                    >
                      <div>
                        <div className="flex items-center justify-between">
                          <div className="flex items-center gap-2">
                            <span className="text-2xl">{recipe.icon}</span>
                            <span className="font-semibold text-slate-100 text-sm">{recipeTitle}</span>
                          </div>
                          {isLocked ? (
                            <span className="text-[10px] font-mono text-rose-400">Lv.{recipe.minLevel} Req</span>
                          ) : (
                            <span className="text-[10px] font-mono text-amber-400">+{recipe.xpReward} XP</span>
                          )}
                        </div>
                        <p className="text-xs text-slate-400 mt-1.5 leading-relaxed">
                          {recipeDesc}
                        </p>

                        {/* Ingredients List */}
                        <div className="mt-3 flex flex-wrap gap-1.5">
                          {recipe.ingredients.map((ing) => {
                            const have = getItemCount(ing.itemId);
                            const sufficient = have >= ing.count;
                            const ingName = language === 'de' ? (ing.name_de || ing.name) : ing.name;
                            return (
                              <span
                                key={ing.itemId}
                                className={`text-[11px] px-2 py-0.5 rounded font-mono ${
                                  sufficient
                                    ? 'bg-slate-700/80 text-emerald-300'
                                    : 'bg-rose-950/60 text-rose-400 border border-rose-900'
                                }`}
                              >
                                {ingName}: {have}/{ing.count}
                              </span>
                            );
                          })}
                        </div>
                      </div>

                      <button
                        disabled={isLocked || !hasIngredients}
                        onClick={() => {
                          cydAudio.munch();
                          onCookRecipe(recipe);
                        }}
                        className={`mt-3 w-full py-2 px-3 rounded-lg text-xs font-medium transition-all ${
                          isLocked
                            ? 'bg-slate-800 text-slate-500 cursor-not-allowed'
                            : hasIngredients
                            ? 'bg-amber-600 hover:bg-amber-500 text-slate-950 font-bold active:scale-95'
                            : 'bg-slate-800 text-slate-400 border border-slate-700 cursor-not-allowed'
                        }`}
                      >
                        {isLocked ? (language === 'de' ? `Freigeschaltet ab Lv.${recipe.minLevel}` : `Unlocks at Lv.${recipe.minLevel}`) : hasIngredients ? t.btnCook : (language === 'de' ? 'Fehlende Zutaten' : 'Missing Ingredients')}
                      </button>
                    </div>
                  );
                })}
              </div>
            </div>
          )}

          {/* TAB 2: JOBS */}
          {activeTab === 'jobs' && (
            <div className="space-y-3">
              <div className="text-xs text-slate-400">
                {language === 'de'
                  ? 'Schicke dein Haustier auf kurze Tagesberufe, um Münzen, XP, Zutaten & Essen fürs Inventar zu verdienen!'
                  : 'Send your pet on quick daily jobs to earn Coins, EXP, crafting materials, and food drops!'}
              </div>
              <div className="grid grid-cols-1 sm:grid-cols-2 gap-3">
                {JOBS.map((job) => {
                  const isLocked = pet.level < job.minLevel;
                  const isWorking = pet.activeJob?.jobId === job.id;
                  const jobTitle = language === 'de' ? (job.title_de || job.title) : job.title;
                  const jobDesc = language === 'de' ? (job.description_de || job.description) : job.description;
                  const matName = language === 'de' ? (job.possibleDrop.name_de || job.possibleDrop.name) : job.possibleDrop.name;
                  const foodDropName = job.bonusFoodDrop
                    ? (language === 'de' ? (job.bonusFoodDrop.name_de || job.bonusFoodDrop.name) : job.bonusFoodDrop.name)
                    : null;

                  return (
                    <div
                      key={job.id}
                      className={`p-3.5 rounded-xl border flex flex-col justify-between ${
                        isLocked
                          ? 'bg-slate-900/50 border-slate-800 opacity-60'
                          : 'bg-slate-800/80 border-slate-700/80 hover:border-emerald-500/50'
                      }`}
                    >
                      <div>
                        <div className="flex items-center justify-between">
                          <div className="flex items-center gap-2">
                            <span className="text-2xl">{job.icon}</span>
                            <span className="font-semibold text-slate-100 text-sm">{jobTitle}</span>
                          </div>
                          {isLocked ? (
                            <span className="text-[10px] font-mono text-rose-400">Lv.{job.minLevel} Req</span>
                          ) : (
                            <span className="text-[10px] font-mono text-emerald-400">🪙 +{job.coinReward}</span>
                          )}
                        </div>
                        <p className="text-xs text-slate-400 mt-1.5 leading-relaxed">
                          {jobDesc}
                        </p>

                        <div className="mt-3 flex flex-wrap items-center gap-2 text-[11px] text-slate-300 font-mono">
                          <span className="bg-slate-900/80 px-1.5 py-0.5 rounded">⏱️ {formatDurationLabel(job.durationSeconds)}</span>
                          <span className="bg-slate-900/80 px-1.5 py-0.5 rounded text-purple-300">+{job.xpReward} XP</span>
                          <span className="bg-slate-900/80 px-1.5 py-0.5 rounded text-emerald-300">
                            🌿 {matName} ({Math.round(job.possibleDrop.chance * 100)}%)
                          </span>
                          {job.bonusFoodDrop && foodDropName && (
                            <span className="bg-amber-950/60 border border-amber-700/50 px-1.5 py-0.5 rounded text-amber-300">
                              🍽️ +{job.bonusFoodDrop.quantity}x {foodDropName}
                            </span>
                          )}
                        </div>
                      </div>

                      <button
                        disabled={isLocked || Boolean(pet.activeJob)}
                        onClick={() => {
                          cydAudio.click();
                          onStartJob(job);
                        }}
                        className={`mt-3 w-full py-2 px-3 rounded-lg text-xs font-medium transition-all ${
                          isLocked
                            ? 'bg-slate-800 text-slate-500 cursor-not-allowed'
                            : isWorking
                            ? 'bg-emerald-900 text-emerald-300 animate-pulse'
                            : pet.activeJob
                            ? 'bg-slate-800 text-slate-500 cursor-not-allowed'
                            : 'bg-emerald-600 hover:bg-emerald-500 text-slate-950 font-bold active:scale-95'
                        }`}
                      >
                        {isLocked
                          ? (language === 'de' ? `Freigeschaltet ab Lv.${job.minLevel}` : `Unlocks at Lv.${job.minLevel}`)
                          : isWorking
                          ? (language === 'de' ? `Im Dienst... (${formatCountdown(activeJobRemainingSec)})` : `On Duty... (${formatCountdown(activeJobRemainingSec)})`)
                          : t.btnStartJob}
                      </button>
                    </div>
                  );
                })}
              </div>
            </div>
          )}

          {/* TAB 3: MAGIC */}
          {activeTab === 'magic' && (
            <div className="space-y-4">
              {/* Magic Research Card */}
              <div className="p-4 rounded-xl bg-gradient-to-r from-purple-950/70 via-indigo-950/60 to-purple-950/70 border border-purple-500/50 shadow-lg">
                <div className="flex flex-col sm:flex-row sm:items-center justify-between gap-3">
                  <div className="space-y-1">
                    <div className="flex items-center gap-2">
                      <span className="text-xl">🔮</span>
                      <span className="font-bold text-slate-100 text-sm">{t.magicResearchTitle}</span>
                      <span className="px-1.5 py-0.5 rounded bg-purple-500/20 text-purple-300 border border-purple-500/40 text-[10px] font-mono font-bold">
                        +35 XP
                      </span>
                    </div>
                    <p className="text-xs text-slate-300 leading-relaxed max-w-md">
                      {t.magicResearchDesc}
                    </p>
                    <div className="text-[11px] text-purple-300/80 font-mono flex items-center gap-3">
                      <span>⚡ {language === 'de' ? 'Kosten: 15 Energie' : 'Cost: 15 Energy'}</span>
                      <span>·</span>
                      <span>🔋 {language === 'de' ? 'Verfügbar: ' : 'Available: '}{Math.round(pet.stats.energy)} EN</span>
                    </div>
                  </div>

                  <button
                    disabled={pet.stats.energy < 15 || pet.isSleeping}
                    onClick={() => {
                      if (onResearchMagic) {
                        onResearchMagic();
                      }
                    }}
                    className={`px-4 py-2.5 rounded-xl text-xs font-bold transition-all shrink-0 flex items-center justify-center gap-1.5 shadow-md ${
                      pet.stats.energy < 15 || pet.isSleeping
                        ? 'bg-slate-800 text-slate-500 border border-slate-700 cursor-not-allowed'
                        : 'bg-purple-600 hover:bg-purple-500 active:bg-purple-700 text-white active:scale-95 border border-purple-400/50'
                    }`}
                  >
                    <span>✨</span>
                    <span>{t.btnResearchMagic}</span>
                  </button>
                </div>
              </div>

              <div className="flex items-center justify-between text-xs text-slate-400">
                <span>{language === 'de' ? 'Wirke Elementarzauber mit dem Manavorrat deines Haustiers.' : "Cast elemental spells powered by your pet's Mana pool."}</span>
                <span className="text-purple-400 font-mono">Mana: {pet.stats.mana}/100</span>
              </div>
              <div className="grid grid-cols-1 sm:grid-cols-2 gap-3">
                {SPELLS.map((spell) => {
                  const isLocked = pet.level < spell.minLevel;
                  const hasMana = pet.stats.mana >= spell.manaCost;
                  const spellName = language === 'de' ? (spell.name_de || spell.name) : spell.name;
                  const spellDesc = language === 'de' ? (spell.description_de || spell.description) : spell.description;
                  const spellEffect = language === 'de' ? (spell.effectDescription_de || spell.effectDescription) : spell.effectDescription;

                  return (
                    <div
                      key={spell.id}
                      className={`p-3.5 rounded-xl border flex flex-col justify-between ${
                        isLocked
                          ? 'bg-slate-900/50 border-slate-800 opacity-60'
                          : 'bg-slate-800/80 border-slate-700/80 hover:border-purple-500/50'
                      }`}
                    >
                      <div>
                        <div className="flex items-center justify-between">
                          <div className="flex items-center gap-2">
                            <span className="text-2xl">{spell.icon}</span>
                            <span className="font-semibold text-slate-100 text-sm">{spellName}</span>
                          </div>
                          <span className="text-[10px] font-mono text-purple-300">⚡ {spell.manaCost} MP</span>
                        </div>
                        <p className="text-xs text-slate-400 mt-1.5 leading-relaxed">
                          {spellDesc}
                        </p>
                        <div className="mt-2 text-xs text-indigo-300 font-medium">
                          {language === 'de' ? 'Effekt: ' : 'Effect: '}{spellEffect}
                        </div>
                      </div>

                      <button
                        disabled={isLocked || !hasMana}
                        onClick={() => {
                          cydAudio.magicCast();
                          onCastSpell(spell);
                        }}
                        className={`mt-3 w-full py-2 px-3 rounded-lg text-xs font-medium transition-all ${
                          isLocked
                            ? 'bg-slate-800 text-slate-500 cursor-not-allowed'
                            : hasMana
                            ? 'bg-purple-600 hover:bg-purple-500 text-white font-bold active:scale-95'
                            : 'bg-slate-800 text-slate-400 border border-slate-700 cursor-not-allowed'
                        }`}
                      >
                        {isLocked ? (language === 'de' ? `Freigeschaltet ab Lv.${spell.minLevel}` : `Unlocks at Lv.${spell.minLevel}`) : hasMana ? t.btnCastSpell : (language === 'de' ? 'Zu wenig Mana' : 'Need More Mana')}
                      </button>
                    </div>
                  );
                })}
              </div>
            </div>
          )}

          {/* TAB 4: INVENTORY & WARDROBE */}
          {activeTab === 'inventory' && (
            <div className="space-y-4">
              {/* Equipped Gear Section */}
              <div className="bg-slate-950 p-3.5 rounded-xl border border-slate-800">
                <div className="text-xs font-semibold text-slate-300 mb-2">{t.equippedAccessories}</div>
                <div className="grid grid-cols-3 gap-2">
                  {/* Head */}
                  <div className="p-2 rounded bg-slate-900 border border-slate-800 flex items-center justify-between">
                    <span className="text-xs text-slate-400">{language === 'de' ? 'Kopf:' : 'Head:'}</span>
                    {pet.equipped.head ? (
                      <div className="flex items-center gap-1.5">
                        <span className="text-xs text-amber-300 truncate max-w-[80px]">
                          {CATALOG_ITEMS.find((c) => c.id === pet.equipped.head)?.name || 'Equipped'}
                        </span>
                        <button
                          onClick={() => onUnequipItem('head')}
                          className="text-[10px] text-rose-400 hover:text-rose-300"
                        >
                          ✕
                        </button>
                      </div>
                    ) : (
                      <span className="text-xs text-slate-600 font-mono">{language === 'de' ? 'Kein' : 'None'}</span>
                    )}
                  </div>

                  {/* Face */}
                  <div className="p-2 rounded bg-slate-900 border border-slate-800 flex items-center justify-between">
                    <span className="text-xs text-slate-400">{language === 'de' ? 'Gesicht:' : 'Face:'}</span>
                    {pet.equipped.face ? (
                      <div className="flex items-center gap-1.5">
                        <span className="text-xs text-amber-300 truncate max-w-[80px]">
                          {CATALOG_ITEMS.find((c) => c.id === pet.equipped.face)?.name || 'Equipped'}
                        </span>
                        <button
                          onClick={() => onUnequipItem('face')}
                          className="text-[10px] text-rose-400 hover:text-rose-300"
                        >
                          ✕
                        </button>
                      </div>
                    ) : (
                      <span className="text-xs text-slate-600 font-mono">{language === 'de' ? 'Kein' : 'None'}</span>
                    )}
                  </div>

                  {/* Back */}
                  <div className="p-2 rounded bg-slate-900 border border-slate-800 flex items-center justify-between">
                    <span className="text-xs text-slate-400">{language === 'de' ? 'Rücken:' : 'Back:'}</span>
                    {pet.equipped.back ? (
                      <div className="flex items-center gap-1.5">
                        <span className="text-xs text-amber-300 truncate max-w-[80px]">
                          {CATALOG_ITEMS.find((c) => c.id === pet.equipped.back)?.name || 'Equipped'}
                        </span>
                        <button
                          onClick={() => onUnequipItem('back')}
                          className="text-[10px] text-rose-400 hover:text-rose-300"
                        >
                          ✕
                        </button>
                      </div>
                    ) : (
                      <span className="text-xs text-slate-600 font-mono">{language === 'de' ? 'Kein' : 'None'}</span>
                    )}
                  </div>
                </div>
              </div>

              {/* Inventory List */}
              <div className="space-y-2">
                <div className="text-xs font-semibold text-slate-300">{t.bagItems} ({pet.inventory.length})</div>
                <div className="grid grid-cols-1 sm:grid-cols-2 gap-2">
                  {pet.inventory.map((item) => {
                    const itemName = language === 'de' ? (item.name_de || item.name) : item.name;
                    const itemDesc = language === 'de' ? (item.description_de || item.description) : item.description;
                    return (
                      <div
                        key={item.id}
                        className="p-2.5 rounded-lg bg-slate-800/80 border border-slate-700/80 flex items-center justify-between gap-2"
                      >
                        <div className="flex items-start gap-2 min-w-0">
                          <span className="text-xl shrink-0">{item.icon}</span>
                          <div className="min-w-0">
                            <div className="text-xs font-semibold text-slate-200 truncate">{itemName}</div>
                            <div className="text-[10px] text-slate-400 line-clamp-2">{itemDesc}</div>
                            {(item.hungerGain || item.satietyTicks || item.sickChance || item.cureSickness) && (
                              <div className="flex flex-wrap gap-1 mt-1 font-mono text-[9px]">
                                {item.hungerGain ? (
                                  <span className="px-1 py-0.2 bg-amber-950/60 text-amber-300 rounded">+{item.hungerGain} FD</span>
                                ) : null}
                                {item.satietyTicks ? (
                                  <span className="px-1 py-0.2 bg-emerald-950/60 text-emerald-300 rounded">🛡️ {item.satietyTicks}T</span>
                                ) : null}
                                {item.sickChance ? (
                                  <span className="px-1 py-0.2 bg-rose-950/70 text-rose-300 rounded">⚠️ {Math.round(item.sickChance * 100)}%</span>
                                ) : null}
                                {item.cureSickness ? (
                                  <span className="px-1 py-0.2 bg-teal-950/70 text-teal-300 rounded">💊 Cure</span>
                                ) : null}
                              </div>
                            )}
                          </div>
                        </div>
                        <div className="flex flex-col items-end gap-1.5 shrink-0">
                          <span className="text-xs font-mono text-amber-400 bg-slate-900 px-1.5 py-0.5 rounded">
                            x{item.quantity}
                          </span>
                          {item.type === 'accessory' && (
                            <button
                              onClick={() => onEquipItem(item)}
                              className="text-xs bg-sky-600 hover:bg-sky-500 text-white px-2 py-1 rounded active:scale-95 font-medium"
                            >
                              {t.btnEquip}
                            </button>
                          )}
                          {item.type === 'food' && onFeedItem && (
                            <button
                              onClick={() => onFeedItem(item)}
                              className="text-xs bg-amber-600 hover:bg-amber-500 text-slate-950 font-bold px-2 py-1 rounded active:scale-95"
                            >
                              {t.btnFeedItem}
                            </button>
                          )}
                          {item.type === 'potion' && onUseItem && (
                            <button
                              onClick={() => onUseItem(item)}
                              className="text-xs bg-teal-600 hover:bg-teal-500 text-slate-950 font-bold px-2 py-1 rounded active:scale-95"
                            >
                              {t.btnUseItem}
                            </button>
                          )}
                        </div>
                      </div>
                    );
                  })}
                </div>
              </div>
            </div>
          )}

          {/* TAB 5: SHOP / BAZAR */}
          {activeTab === 'shop' && (
            <div className="space-y-4">
              <div className="flex flex-col sm:flex-row sm:items-center justify-between gap-2 bg-slate-950 p-3.5 rounded-xl border border-slate-800">
                <div>
                  <div className="text-sm font-bold text-emerald-300 flex items-center gap-2">
                    <span>🛒</span> {t.shopTitle}
                  </div>
                  <div className="text-xs text-slate-400 mt-0.5">{t.shopDesc}</div>
                </div>
                <div className="px-3 py-1.5 rounded-lg bg-amber-500/15 border border-amber-500/40 text-amber-300 font-mono text-sm font-bold shrink-0">
                  🪙 {pet.coins} Coins
                </div>
              </div>

              <div className="grid grid-cols-1 sm:grid-cols-2 gap-3">
                {SHOP_ITEMS_CATALOG.map((shopEntry) => {
                  const sItem = shopEntry.item;
                  const ownedQty = getItemCount(sItem.id);
                  const isLocked = pet.level < shopEntry.minLevel;
                  const canAfford = pet.coins >= shopEntry.priceCoins;
                  const itemName = language === 'de' ? (sItem.name_de || sItem.name) : sItem.name;
                  const itemDesc = language === 'de' ? (sItem.description_de || sItem.description) : sItem.description;

                  return (
                    <div
                      key={shopEntry.id}
                      className={`p-3.5 rounded-xl border flex flex-col justify-between ${
                        isLocked
                          ? 'bg-slate-900/50 border-slate-800 opacity-60'
                          : 'bg-slate-800/80 border-slate-700/80 hover:border-emerald-500/50'
                      }`}
                    >
                      <div>
                        <div className="flex items-start justify-between gap-2">
                          <div className="flex items-center gap-2">
                            <span className="text-2xl">{sItem.icon}</span>
                            <div>
                              <div className="font-semibold text-slate-100 text-sm">{itemName}</div>
                              <div className="text-[10px] font-mono text-slate-400">
                                {language === 'de' ? 'Im Inventar:' : 'Owned:'} <span className="text-amber-300">x{ownedQty}</span>
                              </div>
                            </div>
                          </div>
                          <span className={`text-xs font-mono font-bold px-2 py-0.5 rounded ${
                            canAfford ? 'bg-amber-500/20 text-amber-300' : 'bg-rose-950/60 text-rose-400'
                          }`}>
                            🪙 {shopEntry.priceCoins}
                          </span>
                        </div>
                        <p className="text-xs text-slate-400 mt-1.5 leading-relaxed">{itemDesc}</p>

                        <div className="mt-2 flex flex-wrap gap-1.5 text-[10px] font-mono">
                          {sItem.hungerGain ? (
                            <span className="px-1.5 py-0.5 rounded bg-amber-950/60 text-amber-300">+{sItem.hungerGain} FD</span>
                          ) : null}
                          {sItem.healthGain ? (
                            <span className="px-1.5 py-0.5 rounded bg-rose-950/60 text-rose-300">+{sItem.healthGain} LP</span>
                          ) : null}
                          {sItem.energyGain ? (
                            <span className="px-1.5 py-0.5 rounded bg-yellow-950/60 text-yellow-300">+{sItem.energyGain} EN</span>
                          ) : null}
                          {sItem.satietyTicks ? (
                            <span className="px-1.5 py-0.5 rounded bg-emerald-950/60 text-emerald-300 border border-emerald-700/40">
                              🛡️ {sItem.satietyTicks} Ticks
                            </span>
                          ) : null}
                          {sItem.sickChance ? (
                            <span className="px-1.5 py-0.5 rounded bg-rose-950/80 text-rose-300 border border-rose-700/40">
                              ⚠️ {Math.round(sItem.sickChance * 100)}% Sick
                            </span>
                          ) : null}
                          {sItem.cureSickness ? (
                            <span className="px-1.5 py-0.5 rounded bg-teal-950/80 text-teal-300 border border-teal-700/40">
                              💊 {language === 'de' ? 'Heilt Bauchweh' : 'Cures Sickness'}
                            </span>
                          ) : null}
                        </div>
                      </div>

                      <button
                        disabled={isLocked || !canAfford}
                        onClick={() => {
                          cydAudio.click();
                          onBuyShopItem?.(shopEntry);
                        }}
                        className={`mt-3 w-full py-2 px-3 rounded-lg text-xs font-bold transition-all ${
                          isLocked
                            ? 'bg-slate-800 text-slate-500 cursor-not-allowed'
                            : canAfford
                            ? 'bg-emerald-600 hover:bg-emerald-500 text-slate-950 active:scale-95 cursor-pointer'
                            : 'bg-slate-800 text-rose-400 border border-rose-900/60 cursor-not-allowed'
                        }`}
                      >
                        {isLocked
                          ? `${t.shopLevelReq}${shopEntry.minLevel}`
                          : canAfford
                          ? `${t.btnBuy} · ${shopEntry.priceCoins} 🪙`
                          : t.shopNotEnoughCoins}
                      </button>
                    </div>
                  );
                })}
              </div>
            </div>
          )}

          {/* TAB 6: DAILY MISSIONS & LOGIN STREAK */}
          {activeTab === 'missions' && (
            <div className="space-y-4">
              {/* Daily Login Streak Counter & Bonus Card */}
              <div className="p-4 bg-gradient-to-br from-amber-950/40 via-slate-900 to-slate-950 border border-amber-500/40 rounded-2xl shadow-xl space-y-3.5 relative overflow-hidden">
                {/* Subtle background glow */}
                <div className="absolute -right-6 -top-6 w-28 h-28 bg-amber-500/10 rounded-full blur-2xl pointer-events-none" />

                {/* Header info */}
                <div className="flex flex-col sm:flex-row sm:items-center justify-between gap-2.5">
                  <div className="flex items-center gap-3">
                    <div className="w-11 h-11 rounded-xl bg-gradient-to-br from-amber-500/20 to-orange-500/20 border border-amber-500/50 flex items-center justify-center text-2xl shadow-inner shadow-amber-500/20 shrink-0">
                      🔥
                    </div>
                    <div>
                      <div className="flex items-center gap-2">
                        <span className="text-sm font-bold text-white tracking-wide">{t.dailyLoginStreak}</span>
                        <span className="px-2 py-0.5 rounded-full text-[10px] font-mono font-bold bg-amber-500/20 text-amber-300 border border-amber-500/40">
                          {consecutiveDays} {t.daysStreak}
                        </span>
                      </div>
                      <div className="text-[11px] text-slate-400 mt-0.5">
                        {t.streakBonusDesc}
                      </div>
                    </div>
                  </div>

                  <div className="sm:text-right shrink-0">
                    <span className="text-[10px] font-mono px-2 py-1 rounded bg-slate-950/80 border border-slate-700 text-slate-300">
                      {pet.dailyMissions?.missions.filter((m) => m.claimed).length || 0} / {pet.dailyMissions?.missions.length || 0} {t.missionClaimed}
                    </span>
                  </div>
                </div>

                {/* 7-Day Visual Streak Milestone Progress */}
                <div className="grid grid-cols-7 gap-1.5 pt-1">
                  {[1, 2, 3, 4, 5, 6, 7].map((dayNum) => {
                    const r = getStreakRewards(dayNum);
                    const isReached = cycleDay >= dayNum;
                    const isToday = cycleDay === dayNum;
                    const isPinnacle = dayNum === 7;

                    return (
                      <div
                        key={dayNum}
                        className={`p-1.5 sm:p-2 rounded-lg border text-center flex flex-col items-center justify-between transition-all ${
                          isToday
                            ? 'bg-amber-500/25 border-amber-400 text-amber-200 shadow-md shadow-amber-500/20 ring-1 ring-amber-400/50'
                            : isReached
                            ? 'bg-emerald-950/30 border-emerald-500/40 text-emerald-300'
                            : 'bg-slate-900/60 border-slate-800 text-slate-500'
                        }`}
                      >
                        <div className="text-[9px] font-mono font-semibold uppercase tracking-wider flex items-center gap-0.5">
                          {isPinnacle ? '👑' : isToday ? '🔥' : isReached ? '✓' : ''}
                          <span>D{dayNum}</span>
                        </div>
                        <div className="my-1 text-sm sm:text-base">
                          {isPinnacle ? '🏆' : isReached ? '🪙' : '🔒'}
                        </div>
                        <div className="text-[10px] font-mono font-bold leading-none">
                          +{r.coins}
                        </div>
                      </div>
                    );
                  })}
                </div>

                {/* Streak Bonus Claim Action */}
                <div className="pt-1">
                  {canClaimStreak ? (
                    <button
                      onClick={() => {
                        cydAudio.fanfare();
                        onClaimStreakBonus?.();
                      }}
                      className="w-full py-2.5 px-4 rounded-xl bg-gradient-to-r from-amber-500 via-amber-400 to-yellow-500 hover:from-amber-400 hover:to-yellow-400 text-slate-950 font-bold text-xs flex items-center justify-center gap-2 shadow-lg shadow-amber-500/25 active:scale-[0.98] transition-all cursor-pointer"
                    >
                      <span className="text-base">🎁</span>
                      <span>
                        {t.btnClaimStreakBonus}: +{currentStreakReward.coins} 🪙 ({t.streakBonusCoins}) & +{currentStreakReward.xp} XP!
                      </span>
                    </button>
                  ) : (
                    <div className="flex flex-col sm:flex-row sm:items-center justify-between gap-1.5 p-2.5 rounded-xl bg-slate-950/80 border border-slate-800/80 text-[11px] font-mono">
                      <div className="flex items-center gap-1.5 text-emerald-400 font-semibold">
                        <span>✓</span> {t.streakClaimedToday} (+{currentStreakReward.coins} 🪙, +{currentStreakReward.xp} XP)
                      </div>
                      <div className="text-slate-400 text-[10px]">
                        {t.streakNextTomorrow}: <strong className="text-amber-400">+{nextStreakReward.coins} 🪙</strong>
                      </div>
                    </div>
                  )}
                </div>
              </div>


              {/* Missions List */}
              <div className="space-y-2.5">
                {(pet.dailyMissions?.missions || []).map((m) => {
                  const title = language === 'de' && m.title_de ? m.title_de : m.title;
                  const desc = language === 'de' && m.description_de ? m.description_de : m.description;
                  const pct = Math.min(100, Math.round((m.currentCount / Math.max(1, m.targetCount)) * 100));

                  return (
                    <div
                      key={m.id}
                      className={`p-3 rounded-lg border transition-all ${
                        m.claimed
                          ? 'bg-slate-950/40 border-slate-800/60 opacity-60'
                          : m.completed
                          ? 'bg-amber-950/20 border-amber-500/50 shadow-md shadow-amber-950/30'
                          : 'bg-slate-800/50 border-slate-700/60'
                      } flex flex-col sm:flex-row sm:items-center justify-between gap-3`}
                    >
                      <div className="flex items-start sm:items-center gap-3 flex-1 min-w-0">
                        <span className="text-2xl p-2 rounded-lg bg-slate-900 border border-slate-700 shrink-0">
                          {m.icon}
                        </span>
                        <div className="min-w-0 flex-1">
                          <div className="flex items-center gap-2">
                            <span className="text-xs font-bold text-slate-100 truncate">{title}</span>
                            <span className="text-[10px] font-mono text-slate-400">
                              ({m.currentCount}/{m.targetCount})
                            </span>
                          </div>
                          <div className="text-[11px] text-slate-400 mt-0.5 line-clamp-1">{desc}</div>
                          {/* Progress bar */}
                          <div className="w-full h-1.5 bg-slate-900 rounded-full mt-2 overflow-hidden border border-slate-700/50">
                            <div
                              className={`h-full transition-all duration-300 ${
                                m.completed ? 'bg-amber-400' : 'bg-rose-500'
                              }`}
                              style={{ width: `${pct}%` }}
                            />
                          </div>
                        </div>
                      </div>

                      <div className="flex items-center justify-between sm:justify-end gap-3 shrink-0 pt-2 sm:pt-0 border-t sm:border-t-0 border-slate-800">
                        <div className="flex items-center gap-2 text-[11px] font-mono">
                          <span className="text-amber-400">+{m.coinReward} 🪙</span>
                          <span className="text-purple-400">+{m.xpReward} XP</span>
                        </div>
                        {m.claimed ? (
                          <span className="text-[11px] font-mono text-slate-500 py-1 px-2.5 rounded bg-slate-900/80 border border-slate-800">
                            ✓ {t.missionClaimed}
                          </span>
                        ) : m.completed ? (
                          <button
                            onClick={() => {
                              cydAudio.fanfare();
                              onClaimMission?.(m.id);
                            }}
                            className="text-xs font-bold px-3 py-1.5 rounded-lg bg-gradient-to-r from-amber-500 to-amber-600 hover:from-amber-400 hover:to-amber-500 text-slate-950 shadow-md shadow-amber-500/20 active:scale-95 transition-all flex items-center gap-1.5 cursor-pointer"
                          >
                            <span>🎁</span> {t.btnClaimReward}
                          </button>
                        ) : (
                          <span className="text-[10px] font-mono text-slate-400 py-1 px-2 rounded bg-slate-900/60 border border-slate-800">
                            {pct}%
                          </span>
                        )}
                      </div>
                    </div>
                  );
                })}
              </div>
            </div>
          )}
        </div>
      </div>
    </div>
  );
};
