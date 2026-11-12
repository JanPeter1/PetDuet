import React from 'react';
import { PetState, FriendPetSummary, BiomeType } from '../types/pet';
import { PetPixelSprite } from './PetPixelSprite';
import { BIOMES, SEASONS } from '../utils/petLogic';
import { cydAudio } from '../utils/audio';

import { translations, Language } from '../utils/i18n';

interface CYDScreenProps {
  myPet: PetState;
  friendPet: FriendPetSummary;
  language?: Language;
  onFeed: () => void;
  onPlay: () => void;
  onBath: () => void;
  onOpenRPG: () => void;
  onCloudSave: () => void;
  onEggTap: () => void;
  onPetTouch: () => void;
  onSendTreat: () => void;
  onOpenChat: () => void;
  onCastFriendMagic: () => void;
  onStartQuest: () => void;
  onOpenTrade: () => void;
  connectionType?: 'direct_socket' | 'server_relay' | 'disconnected';
  activeNotification?: string | null;
  friendChatBalloon?: string | null;
  isActionAnimating?: 'feed' | 'play' | 'bath' | 'magic' | null;
}

export const CYDScreen: React.FC<CYDScreenProps> = ({
  myPet,
  friendPet,
  language = 'en',
  onFeed,
  onPlay,
  onBath,
  onOpenRPG,
  onCloudSave,
  onEggTap,
  onPetTouch,
  onSendTreat,
  onOpenChat,
  onCastFriendMagic,
  onStartQuest,
  onOpenTrade,
  connectionType = 'direct_socket',
  activeNotification = null,
  friendChatBalloon = null,
  isActionAnimating = null,
}) => {
  const t = translations[language] || translations.en;
  const myBiome = BIOMES[myPet.biome] || BIOMES.grassland;
  const friendBiome = BIOMES[friendPet.biome] || BIOMES.desert;
  const seasonInfo = SEASONS[myPet.season] || SEASONS.spring;

  // Background styling based on biomes
  const biomeBgStyles: Record<BiomeType, string> = {
    desert: 'from-amber-950/90 via-amber-900/60 to-yellow-950/90',
    grassland: 'from-emerald-950/90 via-emerald-900/60 to-teal-950/90',
    water: 'from-cyan-950/90 via-blue-900/60 to-sky-950/90',
    forest: 'from-stone-950/90 via-amber-950/60 to-stone-900/90',
  };

  return (
    <div 
      className="relative w-[320px] h-[240px] bg-slate-950 overflow-hidden select-none border-2 border-slate-800 font-pixel text-slate-100 shadow-inner"
      style={{
        boxShadow: 'inset 0 0 12px rgba(0, 0, 0, 0.85)',
      }}
    >
      {/* 2.8" LCD Pixel Grid Scanlines Overlay */}
      <div className="absolute inset-0 cyd-scanlines pointer-events-none z-30 opacity-40" />

      {/* Main 320x240 Layout Split: Left (Your Pet) + Right (Friend's Pet) */}
      <div className="relative w-full h-full flex">
        {/* ========================================================= */}
        {/* LEFT FAR-EDGE BUTTONS (X: 0 to 36)                        */}
        {/* User Pet Primary Actions                                  */}
        {/* ========================================================= */}
        <div className="w-[36px] h-full bg-slate-900 border-r border-slate-700/80 flex flex-col justify-between p-1 z-20 shrink-0">
          {/* FEED */}
          <button
            onClick={() => {
              cydAudio.click();
              onFeed();
            }}
            className="w-full h-[40px] bg-emerald-950/90 hover:bg-emerald-900 active:bg-emerald-700 border border-emerald-500/80 rounded flex flex-col items-center justify-center text-[7px] text-emerald-200 transition-transform active:scale-90"
            title="Feed your pet"
          >
            <span className="text-[12px] leading-none">🍖</span>
            <span className="mt-0.5 tracking-tighter">{t.btnFeed}</span>
          </button>

          {/* PLAY */}
          <button
            onClick={() => {
              cydAudio.click();
              onPlay();
            }}
            className="w-full h-[40px] bg-sky-950/90 hover:bg-sky-900 active:bg-sky-700 border border-sky-500/80 rounded flex flex-col items-center justify-center text-[7px] text-sky-200 transition-transform active:scale-90"
            title="Play outdoor minigame"
          >
            <span className="text-[12px] leading-none">🎾</span>
            <span className="mt-0.5 tracking-tighter">{t.btnPlay}</span>
          </button>

          {/* CLEAN */}
          <button
            onClick={() => {
              cydAudio.click();
              onBath();
            }}
            className="w-full h-[40px] bg-cyan-950/90 hover:bg-cyan-900 active:bg-cyan-700 border border-cyan-500/80 rounded flex flex-col items-center justify-center text-[7px] text-cyan-200 transition-transform active:scale-90"
            title="Clean & bubble bath"
          >
            <span className="text-[12px] leading-none">🫧</span>
            <span className="mt-0.5 tracking-tighter">{t.btnBath}</span>
          </button>

          {/* RPG / MAGIC */}
          <button
            onClick={() => {
              cydAudio.click();
              onOpenRPG();
            }}
            className="w-full h-[40px] bg-purple-950/90 hover:bg-purple-900 active:bg-purple-700 border border-purple-500/80 rounded flex flex-col items-center justify-center text-[7px] text-purple-200 transition-transform active:scale-90"
            title="RPG Crafting, Jobs & Magic"
          >
            <span className="text-[12px] leading-none">✨</span>
            <span className="mt-0.5 tracking-tighter">{t.btnRpg}</span>
          </button>

          {/* SAVE */}
          <button
            onClick={() => {
              cydAudio.click();
              onCloudSave();
            }}
            className="w-full h-[40px] bg-amber-950/90 hover:bg-amber-900 active:bg-amber-700 border border-amber-500/80 rounded flex flex-col items-center justify-center text-[7px] text-amber-200 transition-transform active:scale-90"
            title="Save to Cloud & SD"
          >
            <span className="text-[12px] leading-none">💾</span>
            <span className="mt-0.5 tracking-tighter">{t.btnSave}</span>
          </button>
        </div>

        {/* ========================================================= */}
        {/* LEFT VIEWPORT: YOUR PET & STATS (X: 36 to 160)            */}
        {/* Uses user's chosen biome color scheme                      */}
        {/* ========================================================= */}
        <div 
          className={`flex-1 h-full relative p-1.5 flex flex-col justify-between bg-gradient-to-b ${biomeBgStyles[myPet.biome]} overflow-hidden`}
        >
          {/* Ambient Biome Weather / Seasonal Particle Effect */}
          {myPet.season === 'spring' && (
            <div className="absolute top-2 left-6 text-[8px] opacity-40 animate-pulse pointer-events-none">🌸</div>
          )}
          {myPet.season === 'summer' && (
            <div className="absolute top-1 right-2 text-[9px] opacity-50 pointer-events-none">☀️</div>
          )}
          {myPet.season === 'autumn' && (
            <div className="absolute bottom-6 left-2 text-[8px] opacity-40 pointer-events-none">🍂</div>
          )}
          {myPet.season === 'winter' && (
            <div className="absolute top-2 left-4 text-[8px] opacity-50 pointer-events-none">❄️</div>
          )}

          {/* Top Info Header */}
          <div className="relative z-10 flex flex-col gap-0.5">
            <div className="flex items-center justify-between text-[8px]">
              <div className="flex items-center gap-1 min-w-0">
                <span className="px-1 py-0.2 bg-emerald-500/30 text-emerald-300 rounded text-[5px] font-bold border border-emerald-500/40">
                  {language === 'de' ? 'DU' : 'YOU'}
                </span>
                <span className="truncate max-w-[55px] text-white font-bold tracking-tight">
                  {myPet.name}
                </span>
              </div>
              <span className="text-amber-300 text-[7px] shrink-0 font-bold">
                Lv.{myPet.level}
              </span>
            </div>

            <div className="flex items-center justify-between text-[6px] text-slate-300">
              <span style={{ color: myBiome.primaryHex }} className="uppercase font-bold">
                {myBiome.name}
              </span>
              <span className="text-slate-400">
                {seasonInfo.icon} {myPet.phase}
              </span>
            </div>

            {/* Tiny Pixel Stat Bars */}
            <div className="grid grid-cols-2 gap-1 mt-0.5">
              {/* HP Bar */}
              <div className="flex items-center gap-1 bg-black/50 px-1 py-0.5 rounded border border-slate-700/60">
                <span className="text-[6px] text-rose-400">{t.statHealth}</span>
                <div className="flex-1 h-1.5 bg-slate-900 rounded-sm overflow-hidden border border-slate-800">
                  <div
                    className="h-full bg-rose-500 transition-all duration-300"
                    style={{ width: `${Math.min(100, Math.max(0, myPet.stats.health))}%` }}
                  />
                </div>
              </div>

              {/* Hunger Bar */}
              <div className="flex items-center gap-1 bg-black/50 px-1 py-0.5 rounded border border-slate-700/60">
                <span className="text-[6px] text-amber-400">{t.statHunger}</span>
                <div className="flex-1 h-1.5 bg-slate-900 rounded-sm overflow-hidden border border-slate-800">
                  <div
                    className="h-full bg-amber-500 transition-all duration-300"
                    style={{ width: `${Math.min(100, Math.max(0, myPet.stats.hunger))}%` }}
                  />
                </div>
              </div>
            </div>

            {/* EXP Bar (full width) */}
            <div className="flex items-center gap-1 bg-black/40 px-1 py-0.5 rounded border border-slate-800">
              <span className="text-[5px] text-purple-300">{t.statXp}</span>
              <div className="flex-1 h-1 bg-slate-900 rounded-sm overflow-hidden">
                <div
                  className="h-full bg-purple-500 transition-all duration-300"
                  style={{ width: `${Math.min(100, Math.round((myPet.xp / myPet.xpNext) * 100))}%` }}
                />
              </div>
            </div>
          </div>

          {/* Center Pet Sprite & Interaction Area (Your Pet) */}
          <div 
            onClick={() => {
              if (myPet.phase === 'Egg') {
                cydAudio.click();
                onEggTap();
              } else {
                cydAudio.click();
                onPetTouch();
              }
            }}
            className="flex-1 flex flex-col items-center justify-center cursor-pointer transition-transform active:scale-95 py-1"
            title="Tap your pet to cuddle / warm egg"
          >
            <PetPixelSprite
              biome={myPet.biome}
              season={myPet.season}
              phase={myPet.phase}
              isSleeping={myPet.isSleeping}
              isHappy={isActionAnimating === 'play'}
              isEating={isActionAnimating === 'feed'}
              isBath={isActionAnimating === 'bath'}
              eggWarmth={myPet.eggWarmth}
              equipped={myPet.equipped}
              scale={0.9}
            />
          </div>

          {/* Bottom Bar: Coins & Mood */}
          <div className="flex items-center justify-between text-[6px] text-slate-300 bg-black/50 px-1 py-0.5 rounded border border-slate-800">
            <span className="text-amber-400">🪙 {myPet.coins}</span>
            <span>❤️ {myPet.stats.happiness}%</span>
            <span>⚡ {myPet.stats.energy}%</span>
          </div>
        </div>

        {/* ========================================================= */}
        {/* CENTER DIVIDER: DUAL-PET HARDWARE WIRELESS SYNC BRIDGE     */}
        {/* ========================================================= */}
        <div className="w-[8px] h-full bg-slate-900 flex flex-col items-center justify-between py-2 border-x border-slate-800 z-20">
          <div className="w-1 h-1 rounded-full bg-emerald-400 animate-pulse" title="Your Pet (Online)" />
          <div className="flex flex-col items-center gap-1.5 my-auto">
            <div className="w-0.5 h-6 bg-slate-700" />
            <span 
              className="text-[5px] text-slate-400 rotate-90 whitespace-nowrap font-bold tracking-widest cursor-default"
              title={`Hardware Link: ${connectionType === 'direct_socket' ? 'Direct Socket (WiFi 8888)' : 'PHP Server Relay'}`}
            >
              SYNC
            </span>
            <div className="w-0.5 h-6 bg-slate-700" />
          </div>
          <div 
            className={`w-1 h-1 rounded-full ${
              friendPet.isOnline ? 'bg-cyan-400 animate-ping' : 'bg-rose-500'
            }`} 
            title={friendPet.isOnline ? `Partner Connected via ${connectionType}` : 'Seeking Partner'}
          />
        </div>

        {/* ========================================================= */}
        {/* RIGHT VIEWPORT: PARTNER'S PET (Right of Your Own Pet)     */}
        {/* Displays friend's live pet sprite & status in real-time   */}
        {/* ========================================================= */}
        <div 
          className={`flex-1 h-full relative p-1.5 flex flex-col justify-between bg-gradient-to-b ${biomeBgStyles[friendPet.biome]} overflow-hidden`}
        >
          {/* Top Info Header: Friend/Partner Pet */}
          <div className="relative z-10 flex flex-col gap-0.5">
            <div className="flex items-center justify-between text-[8px]">
              <div className="flex items-center gap-1 min-w-0">
                <span className="px-1 py-0.2 bg-cyan-500/30 text-cyan-300 rounded text-[5px] font-bold border border-cyan-500/40">
                  {language === 'de' ? 'PARTNER' : 'PARTNER'}
                </span>
                <span className="truncate max-w-[55px] text-cyan-200 font-bold tracking-tight">
                  {friendPet.petName}
                </span>
              </div>
              <span className="text-amber-300 text-[7px] shrink-0 font-bold">
                Lv.{friendPet.level}
              </span>
            </div>

            <div className="flex items-center justify-between text-[6px] text-slate-300">
              <span style={{ color: friendBiome.primaryHex }} className="uppercase font-bold">
                {friendBiome.name}
              </span>
              <span className={friendPet.isOnline ? 'text-emerald-400 font-semibold' : 'text-slate-500'}>
                {friendPet.isOnline ? `● ${t.statusOnline}` : `○ ${t.statusAsleep}`}
              </span>
            </div>

            {/* Friend Health & Hunger Bars */}
            <div className="grid grid-cols-2 gap-1 mt-0.5">
              <div className="flex items-center gap-1 bg-black/50 px-1 py-0.5 rounded border border-slate-700/60">
                <span className="text-[6px] text-rose-400">{t.statHealth}</span>
                <div className="flex-1 h-1.5 bg-slate-900 rounded-sm overflow-hidden border border-slate-800">
                  <div
                    className="h-full bg-rose-500 transition-all duration-300"
                    style={{ width: `${Math.min(100, Math.max(0, friendPet.stats.health))}%` }}
                  />
                </div>
              </div>

              <div className="flex items-center gap-1 bg-black/50 px-1 py-0.5 rounded border border-slate-700/60">
                <span className="text-[6px] text-amber-400">{t.statHunger}</span>
                <div className="flex-1 h-1.5 bg-slate-900 rounded-sm overflow-hidden border border-slate-800">
                  <div
                    className="h-full bg-amber-500 transition-all duration-300"
                    style={{ width: `${Math.min(100, Math.max(0, friendPet.stats.hunger))}%` }}
                  />
                </div>
              </div>
            </div>
          </div>

          {/* Friend Speech Balloon / Status Alert */}
          {friendChatBalloon && (
            <div className="absolute top-12 left-2 right-2 z-20 bg-white text-slate-950 px-1.5 py-1 rounded shadow-lg border border-slate-400 text-[7px] leading-tight animate-bounce">
              <span className="font-bold text-sky-800">💬 </span>
              {friendChatBalloon}
            </div>
          )}

          {/* Friend Sprite */}
          <div className="flex-1 flex flex-col items-center justify-center py-1">
            <PetPixelSprite
              biome={friendPet.biome}
              phase={friendPet.phase}
              equipped={friendPet.equipped}
              isHappy={isActionAnimating === 'magic'}
              scale={0.9}
            />
          </div>

          {/* Friend Bottom Footer */}
          <div className="flex items-center justify-between text-[6px] text-slate-300 bg-black/50 px-1 py-0.5 rounded border border-slate-800">
            <span className="text-cyan-300">{friendPet.deviceName}</span>
            <span>❤️ {friendPet.stats.happiness}%</span>
          </div>
        </div>

        {/* ========================================================= */}
        {/* RIGHT FAR-EDGE BUTTONS (X: 284 to 320)                    */}
        {/* Friend Direct Interactions                                */}
        {/* ========================================================= */}
        <div className="w-[36px] h-full bg-slate-900 border-l border-slate-700/80 flex flex-col justify-between p-1 z-20 shrink-0">
          {/* TREAT FRIEND */}
          <button
            onClick={() => {
              cydAudio.click();
              onSendTreat();
            }}
            className="w-full h-[40px] bg-rose-950/90 hover:bg-rose-900 active:bg-rose-700 border border-rose-500/80 rounded flex flex-col items-center justify-center text-[7px] text-rose-200 transition-transform active:scale-90"
            title="Send treat to friend's CYD"
          >
            <span className="text-[12px] leading-none">🍩</span>
            <span className="mt-0.5 tracking-tighter">{t.btnTreat}</span>
          </button>

          {/* CHAT */}
          <button
            onClick={() => {
              cydAudio.click();
              onOpenChat();
            }}
            className="w-full h-[40px] bg-blue-950/90 hover:bg-blue-900 active:bg-blue-700 border border-blue-500/80 rounded flex flex-col items-center justify-center text-[7px] text-blue-200 transition-transform active:scale-90"
            title="Send quick chat to friend"
          >
            <span className="text-[12px] leading-none">💬</span>
            <span className="mt-0.5 tracking-tighter">{t.btnChat}</span>
          </button>

          {/* CO-OP MAGIC */}
          <button
            onClick={() => {
              cydAudio.click();
              onCastFriendMagic();
            }}
            className="w-full h-[40px] bg-indigo-950/90 hover:bg-indigo-900 active:bg-indigo-700 border border-indigo-500/80 rounded flex flex-col items-center justify-center text-[7px] text-indigo-200 transition-transform active:scale-90"
            title="Cast shared magic blessing"
          >
            <span className="text-[12px] leading-none">🌟</span>
            <span className="mt-0.5 tracking-tighter">{t.btnMagicPlus}</span>
          </button>

          {/* CO-OP QUEST / JOBS */}
          <button
            onClick={() => {
              cydAudio.click();
              onStartQuest();
            }}
            className="w-full h-[40px] bg-emerald-950/90 hover:bg-emerald-900 active:bg-emerald-700 border border-emerald-500/80 rounded flex flex-col items-center justify-center text-[7px] text-emerald-200 transition-transform active:scale-90"
            title="Shared co-op quest"
          >
            <span className="text-[12px] leading-none">⚔️</span>
            <span className="mt-0.5 tracking-tighter">{t.btnQuest}</span>
          </button>

          {/* TRADE */}
          <button
            onClick={() => {
              cydAudio.click();
              onOpenTrade();
            }}
            className="w-full h-[40px] bg-amber-950/90 hover:bg-amber-900 active:bg-amber-700 border border-amber-500/80 rounded flex flex-col items-center justify-center text-[7px] text-amber-200 transition-transform active:scale-90"
            title="Trade rare accessories"
          >
            <span className="text-[12px] leading-none">🎁</span>
            <span className="mt-0.5 tracking-tighter">{t.btnTrade}</span>
          </button>
        </div>
      </div>

      {/* Screen Overlay Notification Toast */}
      {activeNotification && (
        <div className="absolute top-2 left-10 right-10 z-40 bg-slate-900/95 text-amber-300 border border-amber-500 px-2 py-1 rounded text-center text-[7px] tracking-wide shadow-2xl animate-fade-in">
          {activeNotification}
        </div>
      )}
    </div>
  );
};
