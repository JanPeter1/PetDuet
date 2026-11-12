import React, { useState } from 'react';
import { BiomeType } from '../types/pet';
import { cydAudio } from '../utils/audio';
import { translations, Language } from '../utils/i18n';

interface WiFiAPModalProps {
  currentSSID: string;
  currentDeviceID: string;
  currentTrainerName: string;
  currentFriendID: string;
  currentBiome: BiomeType;
  currentServerURL: string;
  language?: Language;
  onClose: () => void;
  onSaveConfig: (config: {
    ssid: string;
    password: string;
    deviceId: string;
    trainerName: string;
    friendId: string;
    biome: BiomeType;
    serverUrl: string;
  }) => void;
}

export const WiFiAPModal: React.FC<WiFiAPModalProps> = ({
  currentSSID,
  currentDeviceID,
  currentTrainerName,
  currentFriendID,
  currentBiome,
  currentServerURL,
  language = 'de',
  onClose,
  onSaveConfig,
}) => {
  const t = translations[language] || translations.de;
  const [selectedSSID, setSelectedSSID] = useState(currentSSID);
  const [customSSID, setCustomSSID] = useState('');
  const [password, setPassword] = useState('');
  const [showPassword, setShowPassword] = useState(false);
  const [deviceId, setDeviceId] = useState(currentDeviceID);
  const [trainerName, setTrainerName] = useState(currentTrainerName);
  const [friendId, setFriendId] = useState(currentFriendID);
  const [biome, setBiome] = useState<BiomeType>(currentBiome);
  const [serverUrl, setServerUrl] = useState(currentServerURL);

  const scannedNetworks = [
    { ssid: 'Home-WiFi-2.4G', rssi: -45, quality: 'Strong' },
    { ssid: 'MakerLab_IoT', rssi: -58, quality: 'Good' },
    { ssid: 'PixelCafe_Guest', rssi: -72, quality: 'Medium' },
    { ssid: 'RetroLounge_Net', rssi: -80, quality: 'Weak' },
  ];

  const handleSubmit = (e: React.FormEvent) => {
    e.preventDefault();
    cydAudio.levelUp();
    onSaveConfig({
      ssid: customSSID.trim() || selectedSSID,
      password,
      deviceId: deviceId.trim() || 'CYD-DEV1',
      trainerName: trainerName.trim() || 'Trainer',
      friendId: friendId.trim() || 'CYD-DEV2',
      biome,
      serverUrl,
    });
    onClose();
  };

  return (
    <div className="fixed inset-0 z-50 bg-black/80 backdrop-blur-sm flex items-center justify-center p-3 font-sans-clean">
      <div className="w-full max-w-md bg-slate-900 border border-slate-700 rounded-2xl shadow-2xl overflow-hidden flex flex-col max-h-[92vh]">
        {/* Browser URL Bar Mockup */}
        <div className="bg-slate-950 px-4 py-3 border-b border-slate-800 flex items-center justify-between">
          <div className="flex items-center gap-2">
            <div className="w-3 h-3 rounded-full bg-rose-500/80" />
            <div className="w-3 h-3 rounded-full bg-amber-500/80" />
            <div className="w-3 h-3 rounded-full bg-emerald-500/80" />
            <div className="ml-2 bg-slate-900 border border-slate-800 px-3 py-1 rounded-md text-[11px] font-mono text-emerald-400 flex items-center gap-1.5">
              <span>🔒</span> http://192.168.4.1 (CYD-Tamagotchi-AP)
            </div>
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

        {/* Embedded Web Form */}
        <form onSubmit={handleSubmit} className="p-5 overflow-y-auto space-y-4 text-xs">
          <div>
            <div className="text-base font-bold text-amber-400 flex items-center gap-2">
              <span>📶</span> {t.wifiPortalTitle}
            </div>
            <p className="text-slate-400 text-[11px] mt-0.5 leading-relaxed">
              {t.wifiPortalDesc}
            </p>
          </div>

          {/* Network Selection */}
          <div className="space-y-1.5">
            <label className="font-semibold text-slate-300 block">
              {t.labelScannedNetworks}
            </label>
            <select
              value={selectedSSID}
              onChange={(e) => {
                setSelectedSSID(e.target.value);
                setCustomSSID('');
              }}
              className="w-full bg-slate-950 border border-slate-700 rounded-lg p-2.5 text-slate-200 focus:outline-none focus:border-amber-500"
            >
              <option value="">{language === 'de' ? '-- WLAN-Netz wählen --' : '-- Choose Network --'}</option>
              {scannedNetworks.map((net) => (
                <option key={net.ssid} value={net.ssid}>
                  {net.ssid} ({net.rssi} dBm · {net.quality})
                </option>
              ))}
            </select>
            <input
              type="text"
              placeholder={language === 'de' ? 'Oder versteckte / eigene SSID eingeben...' : 'Or enter hidden / custom SSID...'}
              value={customSSID}
              onChange={(e) => setCustomSSID(e.target.value)}
              className="w-full bg-slate-950 border border-slate-700 rounded-lg p-2 text-slate-200 text-xs focus:outline-none focus:border-amber-500"
            />
          </div>

          {/* Password */}
          <div className="space-y-1.5">
            <label className="font-semibold text-slate-300 block">
              {t.labelWifiPass}
            </label>
            <div className="relative">
              <input
                type={showPassword ? 'text' : 'password'}
                placeholder={language === 'de' ? 'WPA2-Schlüssel eingeben...' : 'Enter WPA2 passphrase...'}
                value={password}
                onChange={(e) => setPassword(e.target.value)}
                className="w-full bg-slate-950 border border-slate-700 rounded-lg p-2.5 pr-10 text-slate-200 focus:outline-none focus:border-amber-500 font-mono"
              />
              <button
                type="button"
                onClick={() => setShowPassword(!showPassword)}
                className="absolute right-2.5 top-1/2 -translate-y-1/2 text-slate-400 hover:text-slate-200 text-xs"
              >
                {showPassword ? (language === 'de' ? 'Verbergen' : 'Hide') : (language === 'de' ? 'Anzeigen' : 'Show')}
              </button>
            </div>
          </div>

          {/* Device & Trainer IDs */}
          <div className="grid grid-cols-2 gap-3">
            <div className="space-y-1">
              <label className="font-semibold text-slate-300 block">
                {t.labelDeviceId}
              </label>
              <input
                type="text"
                value={deviceId}
                onChange={(e) => setDeviceId(e.target.value)}
                className="w-full bg-slate-950 border border-slate-700 rounded-lg p-2 text-slate-200 focus:outline-none focus:border-amber-500 font-mono"
              />
            </div>
            <div className="space-y-1">
              <label className="font-semibold text-slate-300 block">
                {t.labelTrainerName}
              </label>
              <input
                type="text"
                value={trainerName}
                onChange={(e) => setTrainerName(e.target.value)}
                className="w-full bg-slate-950 border border-slate-700 rounded-lg p-2 text-slate-200 focus:outline-none focus:border-amber-500"
              />
            </div>
          </div>

          {/* Friend ID & Biome */}
          <div className="grid grid-cols-2 gap-3">
            <div className="space-y-1">
              <label className="font-semibold text-slate-300 block">
                {t.labelFriendId}
              </label>
              <input
                type="text"
                value={friendId}
                onChange={(e) => setFriendId(e.target.value)}
                className="w-full bg-slate-950 border border-slate-700 rounded-lg p-2 text-slate-200 focus:outline-none focus:border-amber-500 font-mono"
              />
            </div>
            <div className="space-y-1">
              <label className="font-semibold text-slate-300 block">
                {t.labelStartupBiome}
              </label>
              <select
                value={biome}
                onChange={(e) => setBiome(e.target.value as BiomeType)}
                className="w-full bg-slate-950 border border-slate-700 rounded-lg p-2 text-slate-200 focus:outline-none focus:border-amber-500"
              >
                <option value="grassland">{t.biomeGrassland} ({language === 'de' ? 'Grün' : 'Green'})</option>
                <option value="desert">{t.biomeDesert} ({language === 'de' ? 'Gelb' : 'Yellow'})</option>
                <option value="water">{t.biomeWater} ({language === 'de' ? 'Blau' : 'Blue'})</option>
                <option value="forest">{t.biomeForest} ({language === 'de' ? 'Braun' : 'Brown'})</option>
              </select>
            </div>
          </div>

          {/* PHP Server URL */}
          <div className="space-y-1">
            <label className="font-semibold text-slate-300 block">
              {t.labelServerUrl}
            </label>
            <input
              type="text"
              value={serverUrl}
              onChange={(e) => setServerUrl(e.target.value)}
              className="w-full bg-slate-950 border border-slate-700 rounded-lg p-2 text-slate-200 focus:outline-none focus:border-amber-500 font-mono"
            />
          </div>

          {/* Save Button */}
          <button
            type="submit"
            className="w-full py-3 bg-amber-500 hover:bg-amber-400 text-slate-950 font-bold rounded-xl text-sm transition-transform active:scale-95 shadow-lg mt-2"
          >
            {t.btnSaveReboot}
          </button>
        </form>
      </div>
    </div>
  );
};
