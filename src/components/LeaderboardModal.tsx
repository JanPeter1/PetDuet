import React, { useState, useEffect } from 'react';
import { LeaderboardEntry, PetState } from '../types/pet';
import { BIOMES } from '../utils/petLogic';
import { cydAudio } from '../utils/audio';
import { translations, Language } from '../utils/i18n';

interface LeaderboardModalProps {
  myPet: PetState;
  language?: Language;
  onClose: () => void;
  onRefresh?: () => void;
}

export const LeaderboardModal: React.FC<LeaderboardModalProps> = ({
  myPet,
  language = 'de',
  onClose,
}) => {
  const t = translations[language] || translations.de;
  const [leaderboard, setLeaderboard] = useState<LeaderboardEntry[]>([]);
  const [, setLoading] = useState(true);

  useEffect(() => {
    fetchLeaderboard();
  }, []);

  const fetchLeaderboard = async () => {
    setLoading(true);
    try {
      const res = await fetch('/api/leaderboard');
      if (res.ok) {
        const data = await res.json();
        if (data.leaderboard) {
          setLeaderboard(data.leaderboard);
        }
      }
    } catch {
      // Fallback local list
    } finally {
      setLoading(false);
    }
  };

  return (
    <div className="fixed inset-0 z-50 bg-black/80 backdrop-blur-sm flex items-center justify-center p-3 font-sans-clean">
      <div className="w-full max-w-2xl bg-slate-900 border border-slate-700 rounded-xl shadow-2xl flex flex-col overflow-hidden max-h-[90vh]">
        {/* Header */}
        <div className="flex items-center justify-between px-5 py-3.5 bg-slate-950 border-b border-slate-800">
          <div className="flex items-center gap-2">
            <span className="font-pixel text-xs text-amber-400">{t.leaderboardTitle}</span>
            <span className="text-xs text-slate-400 font-mono">{t.leaderboardSubtitle}</span>
          </div>
          <button
            onClick={() => {
              cydAudio.back();
              onClose();
            }}
            className="w-7 h-7 flex items-center justify-center rounded-lg bg-slate-800 hover:bg-slate-700 text-slate-300 font-bold transition-colors"
          >
            ✕
          </button>
        </div>

        {/* Content */}
        <div className="p-4 sm:p-5 overflow-y-auto space-y-4">
          <div className="flex items-center justify-between text-xs text-slate-400">
            <span>{t.leaderboardDesc}</span>
            <button
              onClick={fetchLeaderboard}
              className="text-amber-400 hover:text-amber-300 font-mono text-[11px]"
            >
              {t.btnRefreshScores}
            </button>
          </div>

          {/* Table */}
          <div className="overflow-x-auto rounded-lg border border-slate-800">
            <table className="w-full text-left text-xs font-sans-clean">
              <thead className="bg-slate-950 text-slate-400 border-b border-slate-800 font-mono text-[11px]">
                <tr>
                  <th className="py-2.5 px-3">{t.colRank}</th>
                  <th className="py-2.5 px-3">{t.colPetTrainer}</th>
                  <th className="py-2.5 px-3">{t.colBiome}</th>
                  <th className="py-2.5 px-3">{t.colPhase}</th>
                  <th className="py-2.5 px-3 text-right">{t.colLevel}</th>
                  <th className="py-2.5 px-3 text-right">{t.colXp}</th>
                  <th className="py-2.5 px-3 text-right">{t.colTreatsSent}</th>
                </tr>
              </thead>
              <tbody className="divide-y divide-slate-800 font-mono text-[11px]">
                {leaderboard.map((entry, idx) => {
                  const isCurrent = entry.deviceId === myPet.deviceId;
                  const biomeInfo = BIOMES[entry.biome] || BIOMES.grassland;
                  const biomeName =
                    entry.biome === 'desert' ? t.biomeDesert :
                    entry.biome === 'grassland' ? t.biomeGrassland :
                    entry.biome === 'water' ? t.biomeWater : t.biomeForest;

                  return (
                    <tr
                      key={entry.id || idx}
                      className={
                        isCurrent
                          ? 'bg-amber-950/30 font-semibold text-amber-200'
                          : 'hover:bg-slate-800/50 text-slate-200'
                      }
                    >
                      <td className="py-2.5 px-3">
                        {idx === 0 ? '🥇 #1' : idx === 1 ? '🥈 #2' : idx === 2 ? '🥉 #3' : `#${idx + 1}`}
                      </td>
                      <td className="py-2.5 px-3 font-sans-clean">
                        <div className="font-semibold text-slate-100 flex items-center gap-1.5">
                          {entry.petName}
                          {isCurrent && (
                            <span className="text-[9px] bg-amber-500/20 text-amber-300 px-1 rounded">
                              {language === 'de' ? 'DU' : 'YOU'}
                            </span>
                          )}
                        </div>
                        <div className="text-[10px] text-slate-400 font-mono">{entry.trainerName}</div>
                      </td>
                      <td className="py-2.5 px-3">
                        <span style={{ color: biomeInfo.primaryHex }}>
                          {biomeName}
                        </span>
                      </td>
                      <td className="py-2.5 px-3 text-slate-300">
                        {entry.phase}
                      </td>
                      <td className="py-2.5 px-3 text-right font-bold text-amber-400 tabular-nums">
                        Lv.{entry.level}
                      </td>
                      <td className="py-2.5 px-3 text-right text-purple-300 tabular-nums">
                        {entry.xp}
                      </td>
                      <td className="py-2.5 px-3 text-right text-rose-300 tabular-nums">
                        {entry.friendTreatsSent || 0}
                      </td>
                    </tr>
                  );
                })}
              </tbody>
            </table>
          </div>
        </div>
      </div>
    </div>
  );
};
