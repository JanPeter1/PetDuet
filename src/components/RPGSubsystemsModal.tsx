import React, { useState } from 'react';
import { PetState, Recipe, Job, MagicSpell, InventoryItem } from '../types/pet';
import { RECIPES, JOBS, SPELLS, CATALOG_ITEMS } from '../utils/petLogic';
import { cydAudio } from '../utils/audio';
import { translations, Language } from '../utils/i18n';

interface RPGSubsystemsModalProps {
  pet: PetState;
  recipes?: Recipe[];
  language?: Language;
  onClose: () => void;
  onCookRecipe: (recipe: Recipe) => void;
  onStartJob: (job: Job) => void;
  onCastSpell: (spell: MagicSpell) => void;
  onEquipItem: (item: InventoryItem) => void;
  onUnequipItem: (slot: 'head' | 'face' | 'back' | 'environment') => void;
  onClaimMission?: (missionId: string) => void;
  onClaimStreakBonus?: () => void;
}

export const RPGSubsystemsModal: React.FC<RPGSubsystemsModalProps> = ({
  pet,
  recipes = RECIPES,
  language = 'de',
  onClose,
  onCookRecipe,
  onStartJob,
  onCastSpell,
  onEquipItem,
  onUnequipItem,
  onClaimMission,
  onClaimStreakBonus,
}) => {
  const t = translations[language] || translations.de;
  const [activeTab, setActiveTab] = useState<'recipes' | 'jobs' | 'magic' | 'inventory' | 'missions'>('recipes');

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
                  ? 'Schicke dein Haustier auf kurze Tagesberufe, um Münzen, XP und seltene Zutaten zu verdienen!'
                  : 'Send your pet on quick daily jobs to earn Coins, EXP, and rare crafting materials!'}
              </div>
              <div className="grid grid-cols-1 sm:grid-cols-2 gap-3">
                {JOBS.map((job) => {
                  const isLocked = pet.level < job.minLevel;
                  const isWorking = pet.activeJob?.jobId === job.id;

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
                            <span className="font-semibold text-slate-100 text-sm">{job.title}</span>
                          </div>
                          {isLocked ? (
                            <span className="text-[10px] font-mono text-rose-400">Lv.{job.minLevel} Req</span>
                          ) : (
                            <span className="text-[10px] font-mono text-emerald-400">🪙 +{job.coinReward}</span>
                          )}
                        </div>
                        <p className="text-xs text-slate-400 mt-1.5 leading-relaxed">
                          {job.description}
                        </p>

                        <div className="mt-3 flex items-center justify-between text-xs text-slate-400 font-mono">
                          <span>⏱️ {job.durationSeconds}s</span>
                          <span>XP +{job.xpReward}</span>
                          <span>Drop: {job.possibleDrop.name}</span>
                        </div>
                      </div>

                      <button
                        disabled={isLocked || isWorking}
                        onClick={() => {
                          cydAudio.click();
                          onStartJob(job);
                        }}
                        className={`mt-3 w-full py-2 px-3 rounded-lg text-xs font-medium transition-all ${
                          isLocked
                            ? 'bg-slate-800 text-slate-500 cursor-not-allowed'
                            : isWorking
                            ? 'bg-emerald-900 text-emerald-300 animate-pulse'
                            : 'bg-emerald-600 hover:bg-emerald-500 text-slate-950 font-bold active:scale-95'
                        }`}
                      >
                        {isLocked ? (language === 'de' ? `Freigeschaltet ab Lv.${job.minLevel}` : `Unlocks at Lv.${job.minLevel}`) : isWorking ? (language === 'de' ? 'Im Dienst... ⛏️' : 'On Duty... ⛏️') : t.btnStartJob}
                      </button>
                    </div>
                  );
                })}
              </div>
            </div>
          )}

          {/* TAB 3: MAGIC */}
          {activeTab === 'magic' && (
            <div className="space-y-3">
              <div className="flex items-center justify-between text-xs text-slate-400">
                <span>{language === 'de' ? 'Wirke Elementarzauber mit dem Manavorrat deines Haustiers.' : "Cast elemental spells powered by your pet's Mana pool."}</span>
                <span className="text-purple-400 font-mono">Mana: {pet.stats.mana}/100</span>
              </div>
              <div className="grid grid-cols-1 sm:grid-cols-2 gap-3">
                {SPELLS.map((spell) => {
                  const isLocked = pet.level < spell.minLevel;
                  const hasMana = pet.stats.mana >= spell.manaCost;

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
                            <span className="font-semibold text-slate-100 text-sm">{spell.name}</span>
                          </div>
                          <span className="text-[10px] font-mono text-purple-300">⚡ {spell.manaCost} MP</span>
                        </div>
                        <p className="text-xs text-slate-400 mt-1.5 leading-relaxed">
                          {spell.description}
                        </p>
                        <div className="mt-2 text-xs text-indigo-300 font-medium">
                          {language === 'de' ? 'Effekt: ' : 'Effect: '}{spell.effectDescription}
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
                  {pet.inventory.map((item) => (
                    <div
                      key={item.id}
                      className="p-2.5 rounded-lg bg-slate-800/80 border border-slate-700/80 flex items-center justify-between"
                    >
                      <div className="flex items-center gap-2">
                        <span className="text-xl">{item.icon}</span>
                        <div>
                          <div className="text-xs font-semibold text-slate-200">{item.name}</div>
                          <div className="text-[10px] text-slate-400">{item.description}</div>
                        </div>
                      </div>
                      <div className="flex items-center gap-2">
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
                      </div>
                    </div>
                  ))}
                </div>
              </div>
            </div>
          )}

          {/* TAB 5: DAILY MISSIONS & LOGIN STREAK */}
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
