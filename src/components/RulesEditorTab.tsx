import React, { useState } from 'react';
import { parseGameRules, GameRulesConfig } from '../engine/ConfigParser';
import { cydAudio } from '../utils/audio';
import { Language } from '../utils/i18n';

interface RulesEditorTabProps {
  currentConfigText: string;
  language?: Language;
  onApplyConfig: (newConfigText: string) => boolean;
}

export const RulesEditorTab: React.FC<RulesEditorTabProps> = ({
  currentConfigText,
  language = 'de',
  onApplyConfig,
}) => {
  const [configText, setConfigText] = useState(currentConfigText);
  const [parsedPreview, setParsedPreview] = useState<GameRulesConfig | null>(() => {
    try {
      return parseGameRules(currentConfigText);
    } catch {
      return null;
    }
  });
  const [statusMessage, setStatusMessage] = useState<string | null>(null);

  const handleApply = () => {
    cydAudio.click();
    try {
      const parsed = parseGameRules(configText);
      setParsedPreview(parsed);
      const success = onApplyConfig(configText);
      if (success) {
        cydAudio.levelUp();
        setStatusMessage(
          language === 'de'
            ? '✓ Spielmechanik & Rezepte erfolgreich zur Laufzeit angewendet!'
            : '✓ Game mechanics & recipes applied successfully at runtime!'
        );
        setTimeout(() => setStatusMessage(null), 3500);
      }
    } catch (err: any) {
      cydAudio.warning();
      setStatusMessage(`Error parsing config: ${err?.message}`);
    }
  };

  const addRecipeSnippet = () => {
    cydAudio.click();
    const newRecipeKey = `rcp_custom_${Date.now().toString().slice(-4)}`;
    const newSnippet = `${newRecipeKey} = "Golden Honey Waffle | de:Goldene Honigwaffel | min_level:2 | ingredients:mat_herb:1,mat_sunstone:1 | hunger:40 | hp:20 | xp:55 | icon:🧇 | desc:Crisp sweet honey waffle | desc_de:Knusprig-süße Honigwaffel"\n`;

    let updated = configText;
    if (updated.includes('[recipes]')) {
      updated = updated.replace('[recipes]\n', `[recipes]\n${newSnippet}`);
    } else {
      updated += `\n[recipes]\n${newSnippet}`;
    }

    setConfigText(updated);
    try {
      setParsedPreview(parseGameRules(updated));
      setStatusMessage(
        language === 'de'
          ? 'Neues zweisprachiges Rezept eingefügt! Klicke auf "Regeln anwenden".'
          : 'New bilingual recipe inserted! Click "Apply Rules to Game".'
      );
      setTimeout(() => setStatusMessage(null), 3500);
    } catch {}
  };

  const loadPreset = (presetName: string) => {
    cydAudio.click();
    let modified = configText;

    if (presetName === 'fast') {
      modified = modified
        .replace(/hunger_decay = \d+/, 'hunger_decay = 4')
        .replace(/energy_decay = \d+/, 'energy_decay = 3')
        .replace(/egg_warmth_required = \d+/, 'egg_warmth_required = 40')
        .replace(/base_xp_requirement = \d+/, 'base_xp_requirement = 40')
        .replace(/natural_mana_regen = \d+/, 'natural_mana_regen = 8');
    } else if (presetName === 'hardcore') {
      modified = modified
        .replace(/hunger_decay = \d+/, 'hunger_decay = 2')
        .replace(/health_starve_decay = \d+/, 'health_starve_decay = 5')
        .replace(/starve_threshold = \d+/, 'starve_threshold = 35')
        .replace(/base_xp_requirement = \d+/, 'base_xp_requirement = 200');
    } else if (presetName === 'cozy') {
      modified = modified
        .replace(/hunger_decay = \d+/, 'hunger_decay = 1')
        .replace(/happiness_decay = \d+/, 'happiness_decay = 0')
        .replace(/egg_warmth_required = \d+/, 'egg_warmth_required = 50')
        .replace(/natural_mana_regen = \d+/, 'natural_mana_regen = 5');
    }

    setConfigText(modified);
    try {
      setParsedPreview(parseGameRules(modified));
    } catch {}
  };

  const handleDownload = () => {
    cydAudio.click();
    const blob = new Blob([configText], { type: 'text/plain;charset=utf-8' });
    const url = URL.createObjectURL(blob);
    const link = document.createElement('a');
    link.href = url;
    link.download = 'tamagotchi_rules.cfg';
    link.click();
    URL.revokeObjectURL(url);
  };

  return (
    <div className="space-y-6 font-sans-clean">
      {/* Banner */}
      <div className="bg-gradient-to-r from-emerald-950/60 via-slate-900 to-slate-950 p-6 rounded-2xl border border-emerald-500/30">
        <div className="flex flex-col md:flex-row md:items-center justify-between gap-4">
          <div>
            <div className="flex items-center gap-2">
              <span className="px-2 py-0.5 rounded text-[10px] font-mono font-bold bg-emerald-500/20 text-emerald-300 border border-emerald-500/40">
                Runtime Config Engine (.cfg)
              </span>
              <span className="text-xs text-slate-400 font-mono">
                Decoupled from UI & Hardware
              </span>
            </div>
            <h2 className="text-xl font-bold text-white mt-1.5">
              Live Game Mechanics Configuration Studio
            </h2>
            <p className="text-sm text-slate-300 mt-1 max-w-2xl leading-relaxed">
              Every mechanic—stat decay, evolution thresholds, XP scaling formulas, recipes, jobs, and magic spells—is defined in a human-readable text file read and interpreted at runtime.
            </p>
          </div>

          <div className="flex flex-wrap items-center gap-2 shrink-0">
            <button
              onClick={handleApply}
              className="px-4 py-2.5 bg-emerald-500 hover:bg-emerald-400 active:scale-95 text-slate-950 text-xs font-bold rounded-lg shadow-lg transition-all flex items-center gap-1.5"
            >
              <span>⚡</span> Apply Rules to Game
            </button>
            <button
              onClick={handleDownload}
              className="px-3.5 py-2.5 bg-slate-800 hover:bg-slate-700 active:scale-95 text-slate-200 text-xs font-semibold rounded-lg border border-slate-700 transition-all flex items-center gap-1.5"
            >
              ⬇️ Download .cfg
            </button>
          </div>
        </div>
      </div>

      {/* Preset Quick Selectors & Status */}
      <div className="flex flex-wrap items-center justify-between gap-3 bg-slate-900 border border-slate-800 p-3.5 rounded-xl">
        <div className="flex flex-wrap items-center gap-2 text-xs">
          <span className="font-semibold text-slate-300">
            {language === 'de' ? 'Voreinstellungen:' : 'Quick Presets:'}
          </span>
          <button
            onClick={() => loadPreset('fast')}
            className="px-2.5 py-1 bg-slate-800 hover:bg-slate-700 rounded text-amber-300 text-[11px] font-mono border border-slate-700 active:scale-95"
          >
            ⚡ {language === 'de' ? 'Schnell' : 'Fast'}
          </button>
          <button
            onClick={() => loadPreset('hardcore')}
            className="px-2.5 py-1 bg-slate-800 hover:bg-slate-700 rounded text-rose-300 text-[11px] font-mono border border-slate-700 active:scale-95"
          >
            💀 Hardcore
          </button>
          <button
            onClick={() => loadPreset('cozy')}
            className="px-2.5 py-1 bg-slate-800 hover:bg-slate-700 rounded text-cyan-300 text-[11px] font-mono border border-slate-700 active:scale-95"
          >
            🌸 {language === 'de' ? 'Entspannt' : 'Cozy'}
          </button>
          <span className="text-slate-600 hidden sm:inline">|</span>
          <button
            onClick={addRecipeSnippet}
            className="px-3 py-1 bg-emerald-600/30 hover:bg-emerald-600/50 text-emerald-300 font-bold rounded text-[11px] border border-emerald-500/50 flex items-center gap-1 active:scale-95 transition-all"
          >
            <span>🍳</span> {language === 'de' ? '+ Neues Rezept (Zweisprachig)' : '+ New Recipe (Bilingual)'}
          </button>
        </div>

        {statusMessage && (
          <div className="text-xs font-mono text-emerald-400 animate-fade-in font-bold">
            {statusMessage}
          </div>
        )}
      </div>

      {/* Editor & Parsed Telemetry Grid */}
      <div className="grid grid-cols-1 lg:grid-cols-3 gap-6">
        {/* Editor Box */}
        <div className="lg:col-span-2 bg-slate-900 border border-slate-800 rounded-2xl overflow-hidden shadow-xl flex flex-col">
          <div className="px-4 py-2.5 bg-slate-950 border-b border-slate-800 flex items-center justify-between text-xs font-mono text-slate-400">
            <span>📄 tamagotchi_rules.cfg</span>
            <span>{language === 'de' ? 'Mensch-lesbare INI/TOML Konfiguration' : 'Human-Readable INI/TOML Config'}</span>
          </div>
          <div className="p-3 bg-slate-950 flex-1">
            <textarea
              value={configText}
              onChange={(e) => setConfigText(e.target.value)}
              rows={24}
              spellCheck={false}
              className="w-full bg-slate-950 text-slate-200 font-code text-xs p-2 rounded-lg border border-slate-800 focus:outline-none focus:border-emerald-500 leading-relaxed resize-none"
            />
          </div>
        </div>

        {/* Live Parsed Metrics & System Blueprint */}
        <div className="bg-slate-900 border border-slate-800 rounded-2xl p-5 space-y-4">
          <div className="text-sm font-bold text-white flex items-center justify-between">
            <span className="flex items-center gap-2">
              <span>🔍</span> {language === 'de' ? 'Geparster Laufzeit-Zustand' : 'Parsed Runtime State'}
            </span>
            <span className="text-[10px] font-mono px-2 py-0.5 rounded bg-emerald-950 text-emerald-400 border border-emerald-500/40">
              Hot-Reload Active
            </span>
          </div>

          {parsedPreview ? (
            <div className="space-y-3 text-xs max-h-[580px] overflow-y-auto pr-1">
              {/* General */}
              <div className="bg-slate-950 p-3 rounded-xl border border-slate-800 space-y-1 font-mono text-[11px]">
                <div className="text-emerald-400 font-bold uppercase tracking-wider text-[10px]">
                  General & XP
                </div>
                <div>Tick Rate: {parsedPreview.general.tickRateSeconds}s</div>
                <div>Base XP: {parsedPreview.general.baseXpRequirement} XP</div>
                <div>Level Multiplier: x{parsedPreview.general.xpLevelMultiplier}</div>
                <div>Max Stat: {parsedPreview.general.maxStatValue}</div>
              </div>

              {/* Decay Rates */}
              <div className="bg-slate-950 p-3 rounded-xl border border-slate-800 space-y-1 font-mono text-[11px]">
                <div className="text-amber-400 font-bold uppercase tracking-wider text-[10px]">
                  Decay Rates (per tick)
                </div>
                <div>Hunger Decay: -{parsedPreview.decayRates.hungerDecay} / tick</div>
                <div>Energy Decay: -{parsedPreview.decayRates.energyDecay} / tick</div>
                <div>Starve Threshold: &lt; {parsedPreview.decayRates.starveThreshold}</div>
                <div>Mana Regen: +{parsedPreview.decayRates.naturalManaRegen} / tick</div>
              </div>

              {/* Active Bilingual Recipes */}
              <div className="bg-slate-950 p-3 rounded-xl border border-slate-800 space-y-2">
                <div className="flex items-center justify-between">
                  <span className="text-amber-300 font-bold uppercase tracking-wider text-[10px] font-mono">
                    🍳 {language === 'de' ? 'Aktive Rezepte' : 'Active Recipes'} ({parsedPreview.recipes.length})
                  </span>
                  <span className="text-[10px] font-mono text-slate-400">EN / DE</span>
                </div>
                <div className="space-y-1.5">
                  {parsedPreview.recipes.map((r) => (
                    <div
                      key={r.id}
                      className="bg-slate-900/90 border border-slate-800 p-2 rounded-lg text-[11px]"
                    >
                      <div className="flex items-center justify-between">
                        <span className="font-semibold text-slate-200">
                          {r.icon || '🍲'} {language === 'de' ? (r.name_de || r.name) : r.name}
                        </span>
                        <span className="text-[10px] font-mono text-amber-400 font-bold">
                          Lv.{r.minLevel}
                        </span>
                      </div>
                      <div className="text-[10px] text-slate-400 font-mono flex items-center justify-between mt-1">
                        <span>
                          {language === 'de' ? 'Hunger' : 'Hunger'}: +{r.hungerGain} · XP: +{r.xpReward}
                        </span>
                        <span className="text-slate-500 truncate max-w-[120px]">
                          {r.ingredients.map((i) => `${i.count}x ${i.itemId.replace('mat_', '')}`).join(', ')}
                        </span>
                      </div>
                      {language === 'de' && r.name_de && (
                        <div className="text-[10px] text-slate-500 italic mt-0.5">
                          EN: {r.name}
                        </div>
                      )}
                      {language === 'en' && r.name_de && (
                        <div className="text-[10px] text-slate-500 italic mt-0.5">
                          DE: {r.name_de}
                        </div>
                      )}
                    </div>
                  ))}
                </div>
              </div>

              {/* Evolution Phases */}
              <div className="bg-slate-950 p-3 rounded-xl border border-slate-800 space-y-1 font-mono text-[11px]">
                <div className="text-purple-400 font-bold uppercase tracking-wider text-[10px]">
                  Evolution Milestones
                </div>
                <div>Egg Warmth: {parsedPreview.evolutionPhases.eggWarmthRequired}%</div>
                <div>Youth: Lv.{parsedPreview.evolutionPhases.youthMinLevel}+</div>
                <div>Adult: Lv.{parsedPreview.evolutionPhases.adultMinLevel}+</div>
                <div>Elder: Lv.{parsedPreview.evolutionPhases.elderMinLevel}+</div>
              </div>

              {/* Catalog Counts */}
              <div className="bg-slate-950 p-3 rounded-xl border border-slate-800 space-y-1 font-mono text-[11px]">
                <div className="text-cyan-400 font-bold uppercase tracking-wider text-[10px]">
                  Mechanics Catalog
                </div>
                <div>Biomes Defined: {Object.keys(parsedPreview.biomes).length}</div>
                <div>Active Recipes: {parsedPreview.recipes.length}</div>
                <div>Active Jobs: {parsedPreview.jobs.length}</div>
                <div>Active Magic Spells: {parsedPreview.magicSpells.length}</div>
              </div>
            </div>
          ) : (
            <div className="text-xs text-rose-400 font-mono">
              Syntax error in configuration file.
            </div>
          )}
        </div>
      </div>
    </div>
  );
};
