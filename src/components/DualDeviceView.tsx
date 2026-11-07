import React from 'react';
import { PetState, FriendPetSummary, BiomeType, SeasonType } from '../types/pet';
import { CYDDeviceFrame } from './CYDDeviceFrame';
import { CYDScreen } from './CYDScreen';
import { BIOMES, SEASONS } from '../utils/petLogic';
import { cydAudio } from '../utils/audio';
import { translations, Language } from '../utils/i18n';

interface DualDeviceViewProps {
  device1Pet: PetState;
  device2Pet: PetState;
  device1FriendSummary: FriendPetSummary;
  device2FriendSummary: FriendPetSummary;
  language?: Language;
  activePacket: { from: 'DEV1' | 'DEV2'; type: string; label: string } | null;
  notificationDev1: string | null;
  notificationDev2: string | null;
  chatBalloonDev1: string | null;
  chatBalloonDev2: string | null;
  ledColorDev1: 'off' | 'red' | 'green' | 'blue' | 'cyan' | 'gold';
  ledColorDev2: 'off' | 'red' | 'green' | 'blue' | 'cyan' | 'gold';
  onDev1Action: (action: string) => void;
  onDev2Action: (action: string) => void;
  onDev1BiomeChange: (biome: BiomeType) => void;
  onDev2BiomeChange: (biome: BiomeType) => void;
  onSeasonChange: (season: SeasonType) => void;
}

export const DualDeviceView: React.FC<DualDeviceViewProps> = ({
  device1Pet,
  device2Pet,
  device1FriendSummary,
  device2FriendSummary,
  language = 'de',
  activePacket,
  notificationDev1,
  notificationDev2,
  chatBalloonDev1,
  chatBalloonDev2,
  ledColorDev1,
  ledColorDev2,
  onDev1Action,
  onDev2Action,
  onDev1BiomeChange,
  onDev2BiomeChange,
  onSeasonChange,
}) => {
  const t = translations[language] || translations.de;
  const currentSeason = SEASONS[device1Pet.season] || SEASONS.spring;

  return (
    <div className="space-y-6 font-sans-clean">
      {/* Top Configuration & Environment Controls */}
      <div className="bg-slate-900 border border-slate-800 p-4 rounded-2xl flex flex-wrap items-center justify-between gap-4">
        {/* Season Cycle Selector */}
        <div className="flex items-center gap-3">
          <span className="text-xs font-semibold text-slate-300">{t.seasonalCycleTitle}</span>
          <div className="flex items-center gap-1 bg-slate-950 p-1 rounded-xl border border-slate-800">
            {(['spring', 'summer', 'autumn', 'winter'] as SeasonType[]).map((s) => {
              const info = SEASONS[s];
              const isActive = device1Pet.season === s;
              const seasonLabel =
                s === 'spring' ? t.seasonSpring :
                s === 'summer' ? t.seasonSummer :
                s === 'autumn' ? t.seasonAutumn : t.seasonWinter;

              return (
                <button
                  key={s}
                  onClick={() => {
                    cydAudio.click();
                    onSeasonChange(s);
                  }}
                  className={`px-3 py-1.5 rounded-lg text-xs font-medium transition-all flex items-center gap-1 ${
                    isActive
                      ? 'bg-amber-500/20 text-amber-300 border border-amber-500/50 shadow-sm'
                      : 'text-slate-400 hover:text-slate-200'
                  }`}
                >
                  <span>{info.icon}</span>
                  <span className="capitalize">{seasonLabel}</span>
                </button>
              );
            })}
          </div>
        </div>

        {/* Biome Selectors for Device 1 & Device 2 */}
        <div className="flex flex-wrap items-center gap-4 text-xs">
          <div className="flex items-center gap-1.5">
            <span className="text-slate-400 font-mono">{t.dev1BiomeTitle}</span>
            <select
              value={device1Pet.biome}
              onChange={(e) => onDev1BiomeChange(e.target.value as BiomeType)}
              className="bg-slate-950 border border-slate-700 rounded-lg px-2.5 py-1 text-slate-200 text-xs focus:outline-none focus:border-amber-500"
            >
              {(Object.keys(BIOMES) as BiomeType[]).map((b) => {
                const bName =
                  b === 'desert' ? t.biomeDesert :
                  b === 'grassland' ? t.biomeGrassland :
                  b === 'water' ? t.biomeWater : t.biomeForest;
                return (
                  <option key={b} value={b}>
                    {bName}
                  </option>
                );
              })}
            </select>
          </div>

          <div className="flex items-center gap-1.5">
            <span className="text-slate-400 font-mono">{t.dev2BiomeTitle}</span>
            <select
              value={device2Pet.biome}
              onChange={(e) => onDev2BiomeChange(e.target.value as BiomeType)}
              className="bg-slate-950 border border-slate-700 rounded-lg px-2.5 py-1 text-slate-200 text-xs focus:outline-none focus:border-cyan-500"
            >
              {(Object.keys(BIOMES) as BiomeType[]).map((b) => {
                const bName =
                  b === 'desert' ? t.biomeDesert :
                  b === 'grassland' ? t.biomeGrassland :
                  b === 'water' ? t.biomeWater : t.biomeForest;
                return (
                  <option key={b} value={b}>
                    {bName}
                  </option>
                );
              })}
            </select>
          </div>
        </div>
      </div>

      {/* Active Season Perk Banner */}
      <div className="bg-slate-950/70 border border-slate-800/80 px-4 py-2.5 rounded-xl flex items-center justify-between text-xs text-slate-300">
        <div className="flex items-center gap-2">
          <span className="text-base">{currentSeason.icon}</span>
          <span className="font-semibold text-slate-200">
            {device1Pet.season === 'spring' ? t.seasonSpring :
             device1Pet.season === 'summer' ? t.seasonSummer :
             device1Pet.season === 'autumn' ? t.seasonAutumn : t.seasonWinter} {t.seasonActiveLabel}
          </span>
          <span className="text-slate-400 hidden sm:inline">·</span>
          <span className="text-amber-300/90 hidden sm:inline">{currentSeason.resourceBonus}</span>
        </div>
        <div className="text-[11px] font-mono text-emerald-400 flex items-center gap-1.5">
          <span className="w-2 h-2 rounded-full bg-emerald-400 animate-pulse" />
          {t.directSocketTitle}: Connected (127.0.0.1:8888)
        </div>
      </div>

      {/* Dual CYD Hardware Stages & Socket Bridge */}
      <div className="grid grid-cols-1 xl:grid-cols-[1fr_auto_1fr] items-center justify-center gap-6 p-2">
        {/* ======================================================== */}
        {/* CYD DEVICE 1 (YOUR HARDWARE DISPLAY)                     */}
        {/* ======================================================== */}
        <div className="flex flex-col items-center">
          <div className="mb-2 text-center">
            <div className="text-xs font-bold text-amber-400 font-mono">{t.dev1Label}</div>
            <div className="text-[11px] text-slate-400 font-mono">IP: 192.168.1.142 · Port: 8888</div>
          </div>

          <CYDDeviceFrame
            deviceName="CYD-LOCAL"
            deviceId="ESP32-2432S028R"
            ledColor={ledColorDev1}
            onReset={() => onDev1Action('reset')}
            onBoot={() => onDev1Action('boot')}
          >
            <CYDScreen
              myPet={device1Pet}
              friendPet={device1FriendSummary}
              language={language}
              onFeed={() => onDev1Action('feed')}
              onPlay={() => onDev1Action('play')}
              onBath={() => onDev1Action('bath')}
              onOpenRPG={() => onDev1Action('open_rpg')}
              onCloudSave={() => onDev1Action('cloud_save')}
              onEggTap={() => onDev1Action('egg_tap')}
              onPetTouch={() => onDev1Action('pet_touch')}
              onSendTreat={() => onDev1Action('send_treat')}
              onOpenChat={() => onDev1Action('open_chat')}
              onCastFriendMagic={() => onDev1Action('cast_magic')}
              onStartQuest={() => onDev1Action('start_quest')}
              onOpenTrade={() => onDev1Action('open_trade')}
              connectionType="direct_socket"
              activeNotification={notificationDev1}
              friendChatBalloon={chatBalloonDev1}
            />
          </CYDDeviceFrame>
        </div>

        {/* ======================================================== */}
        {/* REAL-TIME DIRECT SOCKET TRANSMISSION BRIDGE              */}
        {/* ======================================================== */}
        <div className="hidden xl:flex flex-col items-center justify-center w-36 px-2 space-y-3">
          <div className="text-center font-mono">
            <div className="text-[10px] text-slate-400 uppercase tracking-widest font-bold">
              {t.directSocketTitle}
            </div>
            <div className="text-[11px] text-emerald-400 font-semibold">
              TCP : 8888
            </div>
          </div>

          {/* Animated Packet Tube */}
          <div className="relative w-full h-8 bg-slate-950 border border-slate-800 rounded-full flex items-center justify-between px-3 overflow-hidden shadow-inner">
            <div className="w-2 h-2 rounded-full bg-emerald-500 animate-pulse" />
            
            {/* Flying Packet */}
            {activePacket && (
              <div
                className={`absolute text-xs bg-amber-500 text-slate-950 font-bold px-2 py-0.5 rounded-full shadow-lg z-10 font-mono text-[9px] ${
                  activePacket.from === 'DEV1'
                    ? 'animate-[bounce_0.6s_ease-in-out_infinite]'
                    : 'animate-[bounce_0.6s_ease-in-out_infinite]'
                }`}
              >
                {activePacket.label}
              </div>
            )}

            <div className="w-full border-t border-dashed border-slate-700 mx-2" />
            <div className="w-2 h-2 rounded-full bg-cyan-400 animate-pulse" />
          </div>

          <div className="text-[10px] text-slate-400 text-center font-mono leading-tight">
            {t.directSocketDesc}
          </div>
        </div>

        {/* ======================================================== */}
        {/* CYD DEVICE 2 (FRIEND'S HARDWARE DISPLAY)                 */}
        {/* ======================================================== */}
        <div className="flex flex-col items-center">
          <div className="mb-2 text-center">
            <div className="text-xs font-bold text-cyan-400 font-mono">{t.dev2Label}</div>
            <div className="text-[11px] text-slate-400 font-mono">IP: 192.168.1.189 · Port: 8888</div>
          </div>

          <CYDDeviceFrame
            deviceName="CYD-FRIEND"
            deviceId="ESP32-2432S028R"
            ledColor={ledColorDev2}
            onReset={() => onDev2Action('reset')}
            onBoot={() => onDev2Action('boot')}
          >
            <CYDScreen
              myPet={device2Pet}
              friendPet={device2FriendSummary}
              language={language}
              onFeed={() => onDev2Action('feed')}
              onPlay={() => onDev2Action('play')}
              onBath={() => onDev2Action('bath')}
              onOpenRPG={() => onDev2Action('open_rpg')}
              onCloudSave={() => onDev2Action('cloud_save')}
              onEggTap={() => onDev2Action('egg_tap')}
              onPetTouch={() => onDev2Action('pet_touch')}
              onSendTreat={() => onDev2Action('send_treat')}
              onOpenChat={() => onDev2Action('open_chat')}
              onCastFriendMagic={() => onDev2Action('cast_magic')}
              onStartQuest={() => onDev2Action('start_quest')}
              onOpenTrade={() => onDev2Action('open_trade')}
              connectionType="direct_socket"
              activeNotification={notificationDev2}
              friendChatBalloon={chatBalloonDev2}
            />
          </CYDDeviceFrame>
        </div>
      </div>
    </div>
  );
};
