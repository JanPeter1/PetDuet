import React from 'react';

interface CYDDeviceFrameProps {
  children: React.ReactNode;
  deviceName?: string;
  deviceId?: string;
  ledColor?: 'off' | 'red' | 'green' | 'blue' | 'cyan' | 'gold';
  onReset?: () => void;
  onBoot?: () => void;
  onBeepTest?: () => void;
}

export const CYDDeviceFrame: React.FC<CYDDeviceFrameProps> = ({
  children,
  deviceName = 'CYD-DEV1',
  deviceId = 'ESP32-2432S028R',
  ledColor = 'green',
  onReset,
  onBoot,
  onBeepTest,
}) => {
  // LED glow styling
  const ledStyles: Record<string, string> = {
    off: 'bg-slate-700 opacity-30 shadow-none',
    red: 'bg-rose-500 shadow-[0_0_12px_#f43f5e] opacity-100',
    green: 'bg-emerald-400 shadow-[0_0_12px_#34d399] opacity-100',
    blue: 'bg-sky-400 shadow-[0_0_12px_#38bdf8] opacity-100',
    cyan: 'bg-cyan-300 shadow-[0_0_12px_#67e8f9] opacity-100',
    gold: 'bg-amber-400 shadow-[0_0_12px_#fbbf24] opacity-100',
  };

  return (
    <div className="relative inline-flex flex-col items-center">
      {/* Physical Cheap Yellow Display (CYD) PCB Enclosure */}
      <div 
        className="relative bg-gradient-to-b from-[#eab308] via-[#ca8a04] to-[#a16207] p-5 sm:p-6 rounded-2xl shadow-2xl border-4 border-[#854d0e] select-none text-slate-950 font-mono"
        style={{
          boxShadow: '0 20px 40px -10px rgba(0,0,0,0.7), inset 0 2px 4px rgba(255,255,255,0.4), inset 0 -3px 6px rgba(0,0,0,0.3)',
        }}
      >
        {/* 4 Brass Corner Screw Mounting Holes */}
        <div className="absolute top-2.5 left-2.5 w-4 h-4 rounded-full bg-gradient-to-tr from-amber-700 to-amber-200 border-2 border-amber-900 shadow-inner flex items-center justify-center">
          <div className="w-2 h-2 rounded-full bg-slate-950" />
        </div>
        <div className="absolute top-2.5 right-2.5 w-4 h-4 rounded-full bg-gradient-to-tr from-amber-700 to-amber-200 border-2 border-amber-900 shadow-inner flex items-center justify-center">
          <div className="w-2 h-2 rounded-full bg-slate-950" />
        </div>
        <div className="absolute bottom-2.5 left-2.5 w-4 h-4 rounded-full bg-gradient-to-tr from-amber-700 to-amber-200 border-2 border-amber-900 shadow-inner flex items-center justify-center">
          <div className="w-2 h-2 rounded-full bg-slate-950" />
        </div>
        <div className="absolute bottom-2.5 right-2.5 w-4 h-4 rounded-full bg-gradient-to-tr from-amber-700 to-amber-200 border-2 border-amber-900 shadow-inner flex items-center justify-center">
          <div className="w-2 h-2 rounded-full bg-slate-950" />
        </div>

        {/* Top Header Silkscreen & Hardware Peripherals */}
        <div className="w-full flex items-center justify-between mb-2 px-3 text-[10px] tracking-tight font-bold text-slate-900/90">
          <div className="flex items-center gap-2">
            <span className="font-extrabold tracking-wider">{deviceId}</span>
            <span className="text-[8px] bg-slate-950/20 px-1 rounded text-slate-900">ESP_CoGo</span>
          </div>

          {/* Onboard Hardware Status: RGB LED & LDR */}
          <div className="flex items-center gap-3">
            {/* LDR (Light sensor) simulation dot */}
            <div className="flex items-center gap-1" title="LDR Ambient Light Sensor (GPIO 34)">
              <div className="w-2.5 h-2.5 rounded-full bg-gradient-to-br from-amber-800 to-amber-950 border border-amber-900" />
              <span className="text-[7px]">LDR</span>
            </div>

            {/* Hardware RGB LED (GPIO 4 / 16 / 17) */}
            <div className="flex items-center gap-1" title="RGB Status LED (GPIO 4/16/17)">
              <div 
                className={`w-3.5 h-3.5 rounded-full border border-slate-900/50 transition-all duration-200 ${
                  ledStyles[ledColor] || ledStyles.green
                }`}
              />
              <span className="text-[7px]">RGB</span>
            </div>
          </div>
        </div>

        {/* 2.8" LCD Screen Housing (Bezel with 320x240 display) */}
        <div className="p-2 sm:p-2.5 bg-gradient-to-b from-slate-900 to-slate-950 rounded-lg shadow-2xl border-2 border-slate-800">
          {children}
        </div>

        {/* Bottom Silkscreen, Micro-USB port, and Tactile Push Buttons */}
        <div className="w-full flex items-center justify-between mt-2.5 px-3 text-[9px] font-bold text-slate-900/80">
          {/* Micro-USB Port & SD Card Label */}
          <div className="flex items-center gap-2">
            <div className="w-5 h-2.5 bg-slate-900 rounded-sm border border-slate-700 shadow-inner" title="USB-C / CH340 Port" />
            <span className="text-[8px] font-mono">USB/PWR</span>
          </div>

          {/* Device Name */}
          <span className="text-[9px] font-extrabold text-slate-900 tracking-wide uppercase">
            {deviceName}
          </span>

          {/* Physical Buttons: BEEP, BOOT & RST */}
          <div className="flex items-center gap-2">
            {onBeepTest && (
              <button
                onClick={onBeepTest}
                className="px-1.5 py-0.5 bg-amber-900 text-amber-100 hover:bg-amber-800 active:bg-amber-950 text-[8px] rounded border border-slate-950 shadow-sm transition-transform active:scale-95"
                title="ESP32 Buzzer Audio Test (GPIO 26)"
              >
                🔊 BEEP
              </button>
            )}
            <button
              onClick={onBoot}
              className="px-2 py-0.5 bg-slate-800 text-slate-100 hover:bg-slate-700 active:bg-slate-900 text-[8px] rounded border border-slate-950 shadow-sm transition-transform active:scale-95"
              title="ESP32 BOOT button (GPIO 0)"
            >
              BOOT
            </button>
            <button
              onClick={onReset}
              className="px-2 py-0.5 bg-rose-900 text-rose-100 hover:bg-rose-800 active:bg-rose-950 text-[8px] rounded border border-slate-950 shadow-sm transition-transform active:scale-95"
              title="ESP32 EN / RESET button"
            >
              RST
            </button>
          </div>
        </div>
      </div>
    </div>
  );
};
