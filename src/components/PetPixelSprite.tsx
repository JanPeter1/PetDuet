import React from 'react';
import { BiomeType, EvolutionPhase, EquippedItems, SeasonType } from '../types/pet';
import { BIOMES } from '../utils/petLogic';

interface PetPixelSpriteProps {
  biome: BiomeType;
  season?: SeasonType;
  phase: EvolutionPhase;
  isSleeping?: boolean;
  isHappy?: boolean;
  isEating?: boolean;
  isBath?: boolean;
  eggWarmth?: number;
  equipped?: EquippedItems;
  scale?: number;
  interactive?: boolean;
  onEggTap?: () => void;
}

export const PetPixelSprite: React.FC<PetPixelSpriteProps> = ({
  biome,
  phase,
  isSleeping = false,
  isHappy = false,
  isEating = false,
  isBath = false,
  eggWarmth = 0,
  equipped = {},
  scale = 1.0,
  onEggTap,
}) => {
  const biomeInfo = BIOMES[biome];

  // Colors per biome
  const colors: Record<BiomeType, { body: string; shadow: string; belly: string; accent: string; cheek: string }> = {
    desert: {
      body: '#f59e0b',
      shadow: '#b45309',
      belly: '#fef3c7',
      accent: '#d97706',
      cheek: '#f87171',
    },
    grassland: {
      body: '#10b981',
      shadow: '#047857',
      belly: '#d1fae5',
      accent: '#059669',
      cheek: '#fb7185',
    },
    water: {
      body: '#06b6d4',
      shadow: '#0e7490',
      belly: '#cffafe',
      accent: '#0284c7',
      cheek: '#f472b6',
    },
    forest: {
      body: '#8d6e63',
      shadow: '#5d4037',
      belly: '#efebe9',
      accent: '#4e342e',
      cheek: '#fb923c',
    },
  };

  const pal = colors[biome] || colors.grassland;

  // Render Egg Phase
  if (phase === 'Egg') {
    return (
      <div 
        onClick={onEggTap}
        className="relative flex flex-col items-center justify-center cursor-pointer select-none group"
        title="Tap egg repeatedly to warm & hatch!"
        style={{ transform: `scale(${scale})` }}
      >
        {/* Glow halo */}
        <div 
          className="absolute w-20 h-24 rounded-full blur-md opacity-40 animate-pulse pointer-events-none"
          style={{ backgroundColor: biomeInfo.primaryHex }}
        />

        {/* Pixelated Egg Graphic */}
        <svg
          width="64"
          height="80"
          viewBox="0 0 64 80"
          className="pixelated transition-transform duration-150 active:scale-95 group-hover:scale-105"
        >
          {/* Egg Shadow */}
          <ellipse cx="32" cy="74" rx="22" ry="5" fill="#000000" opacity="0.4" />

          {/* Egg Outer Shell */}
          <path
            d="M 32 4 C 14 4, 8 36, 8 52 C 8 68, 18 74, 32 74 C 46 74, 56 68, 56 52 C 56 36, 50 4, 32 4 Z"
            fill={pal.body}
            stroke="#1e293b"
            strokeWidth="3"
          />

          {/* Shading */}
          <path
            d="M 46 36 C 48 46, 48 58, 42 66 C 36 72, 28 73, 24 73 C 34 73, 48 68, 52 54 C 54 44, 50 36, 46 36 Z"
            fill={pal.shadow}
          />

          {/* Biome Egg Spots */}
          {biome === 'desert' && (
            <>
              <circle cx="26" cy="30" r="5" fill="#fef3c7" />
              <circle cx="40" cy="46" r="6" fill="#fef3c7" />
              <circle cx="22" cy="56" r="4" fill="#fef3c7" />
            </>
          )}
          {biome === 'grassland' && (
            <>
              {/* Leaf pattern */}
              <path d="M 28 26 Q 36 28 32 38 Q 24 36 28 26 Z" fill="#34d399" />
              <path d="M 38 46 Q 44 48 42 56 Q 36 54 38 46 Z" fill="#34d399" />
            </>
          )}
          {biome === 'water' && (
            <>
              {/* Wave ripples */}
              <path d="M 20 34 Q 30 38 42 34" stroke="#e0f2fe" strokeWidth="3" strokeLinecap="round" fill="none" />
              <path d="M 18 48 Q 30 52 44 48" stroke="#e0f2fe" strokeWidth="3" strokeLinecap="round" fill="none" />
            </>
          )}
          {biome === 'forest' && (
            <>
              {/* Wood rings */}
              <circle cx="32" cy="42" r="8" fill="none" stroke="#4e342e" strokeWidth="2" strokeDasharray="3,3" />
              <circle cx="32" cy="42" r="14" fill="none" stroke="#4e342e" strokeWidth="2" strokeDasharray="4,4" />
            </>
          )}

          {/* Crack lines if warmth > 50 */}
          {eggWarmth > 40 && (
            <path
              d="M 32 20 L 28 26 L 34 32 L 30 38"
              stroke="#0f172a"
              strokeWidth="2"
              fill="none"
              strokeLinecap="round"
            />
          )}
          {eggWarmth > 75 && (
            <path
              d="M 40 40 L 46 45 L 42 52 L 48 58"
              stroke="#0f172a"
              strokeWidth="2"
              fill="none"
              strokeLinecap="round"
            />
          )}

          {/* Specular highlight */}
          <ellipse cx="24" cy="18" rx="6" ry="10" transform="rotate(-20 24 18)" fill="#ffffff" opacity="0.45" />
        </svg>

        {/* Warmth & Tap prompt */}
        <div className="mt-1 flex flex-col items-center">
          <div className="text-[9px] font-pixel text-amber-300 animate-bounce">
            TAP TO HATCH!
          </div>
          <div className="w-16 h-1.5 bg-slate-900 border border-slate-700 rounded-full overflow-hidden mt-0.5">
            <div 
              className="h-full bg-gradient-to-r from-amber-500 to-rose-500 transition-all duration-200"
              style={{ width: `${eggWarmth}%` }}
            />
          </div>
        </div>
      </div>
    );
  }

  // Active Creature Sprite (Baby, Youth, Adult, Elder)
  const isElder = phase === 'Elder';
  const isAdult = phase === 'Adult' || isElder;
  const isYouth = phase === 'Youth' || isAdult;

  return (
    <div 
      className="relative flex items-center justify-center select-none"
      style={{ transform: `scale(${scale})` }}
    >
      {/* Elder Mystic Aura */}
      {isElder && (
        <div className="absolute inset-0 -m-3 rounded-full border border-amber-300/40 animate-ping pointer-events-none" />
      )}

      {/* Bubble Bath Animation */}
      {isBath && (
        <div className="absolute -inset-2 pointer-events-none flex items-center justify-center">
          <span className="absolute top-0 left-2 text-xs animate-bounce">🫧</span>
          <span className="absolute bottom-1 right-2 text-xs animate-pulse">🫧</span>
          <span className="absolute top-2 right-1 text-sm animate-bounce">🧼</span>
        </div>
      )}

      {/* Eating Animation */}
      {isEating && (
        <div className="absolute -top-3 -right-2 text-sm animate-bounce pointer-events-none">
          🍖
        </div>
      )}

      {/* Sleeping Animation */}
      {isSleeping && (
        <div className="absolute -top-4 -right-1 font-pixel text-[10px] text-cyan-300 animate-pulse pointer-events-none">
          Zzz...
        </div>
      )}

      {/* Happy Heart Animation */}
      {isHappy && (
        <div className="absolute -top-4 left-1/2 -translate-x-1/2 text-xs animate-bounce pointer-events-none">
          ❤️
        </div>
      )}

      {/* SVG Pixel Critter */}
      <svg
        width="80"
        height="76"
        viewBox="0 0 80 76"
        className="pixelated drop-shadow-[0_4px_6px_rgba(0,0,0,0.5)] transition-transform duration-200"
      >
        {/* Ground Shadow */}
        <ellipse cx="40" cy="71" rx="26" ry="4" fill="#000000" opacity="0.35" />

        {/* Back Accessory (Cape or Wings) */}
        {equipped.back === 'acc_cape' && (
          <path
            d="M 22 36 Q 14 56 18 70 L 32 68 Z"
            fill="#dc2626"
            stroke="#991b1b"
            strokeWidth="1.5"
          />
        )}
        {equipped.back === 'acc_wings' && (
          <g opacity="0.85">
            <path d="M 20 32 Q 4 18 10 38 Q 16 46 22 42 Z" fill="#e0f2fe" stroke="#38bdf8" strokeWidth="1" />
            <path d="M 60 32 Q 76 18 70 38 Q 64 46 58 42 Z" fill="#e0f2fe" stroke="#38bdf8" strokeWidth="1" />
          </g>
        )}

        {/* Tails & Biome Features */}
        {biome === 'desert' && (
          // Spiky lizard tail
          <path d="M 20 54 Q 8 60 12 70 Q 22 66 26 56 Z" fill={pal.body} stroke="#1e293b" strokeWidth="1.5" />
        )}
        {biome === 'grassland' && (
          // Fluffy bunny / leaf tail
          <circle cx="18" cy="54" r="6" fill={pal.belly} stroke="#1e293b" strokeWidth="1.5" />
        )}
        {biome === 'water' && (
          // Mermaid/Axolotl fin
          <path d="M 18 52 Q 6 56 10 68 Q 20 64 24 54 Z" fill={pal.accent} stroke="#1e293b" strokeWidth="1.5" />
        )}
        {biome === 'forest' && (
          // Bear/Wood spirit bushy tail
          <circle cx="18" cy="54" r="7" fill={pal.accent} stroke="#1e293b" strokeWidth="1.5" />
        )}

        {/* Ears / Horns based on Biome & Phase */}
        {biome === 'desert' && (
          // Solar Crest / Frill
          <g>
            <polygon points="32,20 40,6 48,20" fill={pal.accent} stroke="#1e293b" strokeWidth="1.5" />
            {isYouth && <polygon points="22,25 28,12 36,22" fill={pal.accent} stroke="#1e293b" strokeWidth="1.5" />}
            {isYouth && <polygon points="44,22 52,12 58,25" fill={pal.accent} stroke="#1e293b" strokeWidth="1.5" />}
          </g>
        )}
        {biome === 'grassland' && (
          // Long Rabbit/Leaf Ears
          <g>
            <path d="M 28 24 Q 22 4 28 4 Q 34 4 34 24 Z" fill={pal.body} stroke="#1e293b" strokeWidth="1.5" />
            <path d="M 46 24 Q 46 4 52 4 Q 58 4 52 24 Z" fill={pal.body} stroke="#1e293b" strokeWidth="1.5" />
            <path d="M 28 18 Q 25 8 28 8 Q 31 8 32 18 Z" fill={pal.cheek} opacity="0.6" />
            <path d="M 48 18 Q 49 8 52 8 Q 55 8 52 18 Z" fill={pal.cheek} opacity="0.6" />
          </g>
        )}
        {biome === 'water' && (
          // Axolotl Frilly Gills
          <g>
            <path d="M 22 28 Q 10 20 16 32 Q 12 36 22 36 Z" fill={pal.accent} stroke="#1e293b" strokeWidth="1.5" />
            <path d="M 58 28 Q 70 20 64 32 Q 68 36 58 36 Z" fill={pal.accent} stroke="#1e293b" strokeWidth="1.5" />
          </g>
        )}
        {biome === 'forest' && (
          // Round Bear Ears / Antler stubs
          <g>
            <circle cx="26" cy="22" r="7" fill={pal.accent} stroke="#1e293b" strokeWidth="1.5" />
            <circle cx="54" cy="22" r="7" fill={pal.accent} stroke="#1e293b" strokeWidth="1.5" />
            <circle cx="26" cy="22" r="4" fill={pal.belly} />
            <circle cx="54" cy="22" r="4" fill={pal.belly} />
            {isAdult && (
              // Tiny leafy antlers
              <path d="M 24 16 L 20 8 M 22 12 L 26 10 M 56 16 L 60 8 M 58 12 L 54 10" stroke="#4e342e" strokeWidth="2" strokeLinecap="round" />
            )}
          </g>
        )}

        {/* Main Body */}
        <ellipse
          cx="40"
          cy="46"
          rx={isAdult ? 24 : 20}
          ry={isAdult ? 22 : 19}
          fill={pal.body}
          stroke="#1e293b"
          strokeWidth="2"
        />

        {/* Belly Patch */}
        <ellipse
          cx="40"
          cy="50"
          rx={isAdult ? 15 : 12}
          ry={isAdult ? 14 : 11}
          fill={pal.belly}
        />

        {/* Body Shading */}
        <path
          d={
            isAdult
              ? "M 52 34 C 62 42, 62 56, 52 64 C 44 68, 38 68, 38 68 C 48 68, 60 62, 62 48 C 62 38, 56 34, 52 34 Z"
              : "M 50 36 C 58 42, 58 54, 50 60 C 42 64, 38 64, 38 64 C 46 64, 56 58, 58 46 C 58 38, 54 36, 50 36 Z"
          }
          fill={pal.shadow}
        />

        {/* Feet / Paws */}
        <ellipse cx="28" cy="67" rx="7" ry="4" fill={pal.accent} stroke="#1e293b" strokeWidth="1.5" />
        <ellipse cx="52" cy="67" rx="7" ry="4" fill={pal.accent} stroke="#1e293b" strokeWidth="1.5" />

        {/* Cheeks */}
        <circle cx="28" cy="46" r="3" fill={pal.cheek} opacity="0.65" />
        <circle cx="52" cy="46" r="3" fill={pal.cheek} opacity="0.65" />

        {/* Eyes (Sleeping vs Awake vs Happy) */}
        {isSleeping ? (
          // Sleeping curved eyes
          <g stroke="#1e293b" strokeWidth="2" strokeLinecap="round" fill="none">
            <path d="M 30 40 Q 33 44 36 40" />
            <path d="M 44 40 Q 47 44 50 40" />
          </g>
        ) : isHappy ? (
          // Joyful arch eyes (^ ^)
          <g stroke="#1e293b" strokeWidth="2.5" strokeLinecap="round" fill="none">
            <path d="M 30 42 Q 33 37 36 42" />
            <path d="M 44 42 Q 47 37 50 42" />
          </g>
        ) : (
          // Big Anime/Retro Pixels Eyes
          <g>
            <circle cx="33" cy="39" r="4.5" fill="#0f172a" />
            <circle cx="47" cy="39" r="4.5" fill="#0f172a" />
            {/* Eye Highlights */}
            <circle cx="32" cy="38" r="1.5" fill="#ffffff" />
            <circle cx="46" cy="38" r="1.5" fill="#ffffff" />
          </g>
        )}

        {/* Mouth */}
        {isEating ? (
          // Open munch mouth
          <circle cx="40" cy="47" r="3" fill="#e11d48" stroke="#1e293b" strokeWidth="1" />
        ) : isHappy ? (
          // Big smile
          <path d="M 37 46 Q 40 50 43 46" stroke="#1e293b" strokeWidth="2" strokeLinecap="round" fill="none" />
        ) : (
          // Subtle cute mouth (:3)
          <path d="M 38 46 Q 40 48 42 46" stroke="#1e293b" strokeWidth="1.5" strokeLinecap="round" fill="none" />
        )}

        {/* Elder Sage Whiskers / Beard */}
        {isElder && (
          <g stroke="#e2e8f0" strokeWidth="1.5" strokeLinecap="round">
            <path d="M 34 50 Q 28 54 22 52" />
            <path d="M 46 50 Q 52 54 58 52" />
            <path d="M 40 50 L 40 56" />
          </g>
        )}

        {/* Face Accessories */}
        {equipped.face === 'acc_shades' && (
          // Cool Pixel Shades
          <g>
            <rect x="26" y="35" width="12" height="7" rx="1" fill="#09090b" stroke="#27272a" strokeWidth="1" />
            <rect x="42" y="35" width="12" height="7" rx="1" fill="#09090b" stroke="#27272a" strokeWidth="1" />
            <line x1="38" y1="37" x2="42" y2="37" stroke="#09090b" strokeWidth="2" />
            {/* Specular glint */}
            <line x1="28" y1="36" x2="34" y2="36" stroke="#38bdf8" strokeWidth="1" />
            <line x1="44" y1="36" x2="50" y2="36" stroke="#38bdf8" strokeWidth="1" />
          </g>
        )}

        {/* Head Accessories */}
        {equipped.head === 'acc_tophat' && (
          <g>
            <rect x="28" y="10" width="24" height="15" fill="#18181b" stroke="#09090b" strokeWidth="1" />
            <rect x="22" y="23" width="36" height="4" rx="1" fill="#18181b" stroke="#09090b" strokeWidth="1" />
            <rect x="28" y="19" width="24" height="3" fill="#dc2626" />
          </g>
        )}
        {equipped.head === 'acc_wizard' && (
          <g>
            <polygon points="40,2 26,22 54,22" fill="#4338ca" stroke="#312e81" strokeWidth="1" />
            <ellipse cx="40" cy="22" rx="16" ry="3" fill="#4338ca" stroke="#312e81" strokeWidth="1" />
            <circle cx="40" cy="12" r="2" fill="#facc15" />
          </g>
        )}
        {equipped.head === 'acc_flower' && (
          <g transform="translate(44, 18) scale(0.8)">
            <circle cx="0" cy="0" r="3" fill="#facc15" />
            <circle cx="-4" cy="0" r="2.5" fill="#ffffff" />
            <circle cx="4" cy="0" r="2.5" fill="#ffffff" />
            <circle cx="0" cy="-4" r="2.5" fill="#ffffff" />
            <circle cx="0" cy="4" r="2.5" fill="#ffffff" />
          </g>
        )}
      </svg>
    </div>
  );
};
