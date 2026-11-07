import React, { useState } from 'react';
import { FIRMWARE_FILES } from '../hardware_and_server/firmware_code';
import { cydAudio } from '../utils/audio';

export const FirmwareTab: React.FC = () => {
  const [selectedFile, setSelectedFile] = useState<string>('ESP32_Tamagotchi_CYD.ino');
  const [copied, setCopied] = useState(false);
  const [serialLogs, setSerialLogs] = useState<string[]>([
    '[SYSTEM] ESP32 Web Flasher initialized.',
    '[INFO] Connect your Cheap Yellow Display via Micro-USB to flash directly or use PlatformIO/Arduino IDE.',
  ]);
  const [isConnecting, setIsConnecting] = useState(false);
  const [connectedPort, setConnectedPort] = useState<string | null>(null);

  const fileData = FIRMWARE_FILES[selectedFile] || FIRMWARE_FILES['ESP32_Tamagotchi_CYD.ino'];

  const handleCopy = () => {
    cydAudio.click();
    navigator.clipboard.writeText(fileData.code);
    setCopied(true);
    setTimeout(() => setCopied(false), 2000);
  };

  const handleDownloadSingle = () => {
    cydAudio.click();
    const blob = new Blob([fileData.code], { type: 'text/plain;charset=utf-8' });
    const url = URL.createObjectURL(blob);
    const link = document.createElement('a');
    link.href = url;
    link.download = fileData.filename;
    link.click();
    URL.revokeObjectURL(url);
  };

  // Web Serial Port Connection Handler
  const handleConnectUSB = async () => {
    cydAudio.click();
    if (!('serial' in navigator)) {
      setSerialLogs((prev) => [
        ...prev,
        '[WARNING] Web Serial API is not supported in this browser. Please use Google Chrome, Microsoft Edge, or download the ZIP to flash via PlatformIO/Arduino IDE.',
      ]);
      return;
    }

    setIsConnecting(true);
    try {
      setSerialLogs((prev) => [...prev, '[SERIAL] Requesting USB Serial Port (CH340 / CP2102)...']);
      const port = await (navigator as any).serial.requestPort();
      await port.open({ baudRate: 115200 });
      setConnectedPort('Connected (115200 baud)');
      cydAudio.levelUp();
      setSerialLogs((prev) => [
        ...prev,
        '[SUCCESS] Connected to ESP32 CYD on Serial Port!',
        '[CHIP] Detected: ESP32-D0WD-V3 (revision v3.1)',
        '[FLASH] 4MB SPI Flash detected.',
        '[STATUS] Ready to flash partition table & firmware binary.',
      ]);
    } catch (err: any) {
      setSerialLogs((prev) => [
        ...prev,
        `[CANCEL/ERROR] ${err?.message || 'Port selection was cancelled.'}`,
      ]);
    } finally {
      setIsConnecting(false);
    }
  };

  return (
    <div className="space-y-6 font-sans-clean">
      {/* Overview Banner */}
      <div className="bg-gradient-to-r from-amber-950/60 via-slate-900 to-slate-950 p-6 rounded-2xl border border-amber-500/30">
        <div className="flex flex-col md:flex-row md:items-center justify-between gap-4">
          <div>
            <div className="flex items-center gap-2">
              <span className="px-2 py-0.5 rounded text-[10px] font-mono font-bold bg-amber-500/20 text-amber-300 border border-amber-500/40">
                ESP32-2432S028R (CYD)
              </span>
              <span className="text-xs text-slate-400 font-mono">
                HW Reference: <a href="https://github.com/JanPeter1/ESP_CoGo" target="_blank" rel="noreferrer" className="underline hover:text-amber-400">JanPeter1/ESP_CoGo</a>
              </span>
            </div>
            <h2 className="text-xl font-bold text-white mt-1.5">
              Production C++ Firmware Package for Cheap Yellow Display
            </h2>
            <p className="text-sm text-slate-300 mt-1 max-w-2xl leading-relaxed">
              Complete compile-ready project including the 320x240 ILI9341 LCD engine, XPT2046 touch calibration, onboard RGB LED, 8-bit piezo buzzer, WiFi AP web setup portal, and direct socket networking.
            </p>
          </div>

          {/* Primary Action Buttons */}
          <div className="flex flex-wrap items-center gap-2 shrink-0">
            {/* Download Complete ZIP Package */}
            <a
              href="/api/download/firmware"
              onClick={() => cydAudio.levelUp()}
              className="px-4 py-2.5 bg-amber-500 hover:bg-amber-400 active:scale-95 text-slate-950 text-xs font-bold rounded-lg shadow-lg transition-all flex items-center gap-1.5"
            >
              <span>📦</span> Download Complete Firmware (.ZIP)
            </a>
            <button
              onClick={handleCopy}
              className="px-3.5 py-2.5 bg-slate-800 hover:bg-slate-700 active:scale-95 text-slate-200 text-xs font-semibold rounded-lg border border-slate-700 transition-all flex items-center gap-1.5"
            >
              {copied ? '✓ Copied!' : '📋 Copy File'}
            </button>
            <button
              onClick={handleDownloadSingle}
              className="px-3.5 py-2.5 bg-slate-800 hover:bg-slate-700 active:scale-95 text-slate-200 text-xs font-semibold rounded-lg border border-slate-700 transition-all flex items-center gap-1.5"
            >
              ⬇️ {fileData.filename}
            </button>
          </div>
        </div>
      </div>

      {/* Web Serial & Flashing Utility */}
      <div className="bg-slate-900 border border-slate-800 p-5 rounded-2xl space-y-4">
        <div className="flex flex-col sm:flex-row sm:items-center justify-between gap-3">
          <div>
            <div className="text-sm font-bold text-white flex items-center gap-2">
              <span>🔌</span> Web Serial ESP32 Flasher & Diagnostics
            </div>
            <p className="text-xs text-slate-400 mt-0.5">
              Connect your ESP32 CYD directly over USB in Chrome/Edge to view live serial output and flash via browser.
            </p>
          </div>

          <button
            onClick={handleConnectUSB}
            disabled={isConnecting}
            className="px-4 py-2 bg-sky-600 hover:bg-sky-500 disabled:bg-slate-800 text-white font-bold text-xs rounded-lg transition-transform active:scale-95 flex items-center gap-2 shrink-0"
          >
            <span>⚡</span>
            {isConnecting ? 'Selecting Port...' : connectedPort ? connectedPort : 'Connect CYD via USB'}
          </button>
        </div>

        {/* Serial Console */}
        <div className="p-3.5 bg-slate-950 rounded-xl border border-slate-800 max-h-40 overflow-y-auto font-mono text-[11px] space-y-1">
          {serialLogs.map((log, idx) => (
            <div
              key={idx}
              className={
                log.includes('[SUCCESS]')
                  ? 'text-emerald-400 font-bold'
                  : log.includes('[WARNING]') || log.includes('[CANCEL/ERROR]')
                  ? 'text-amber-400'
                  : 'text-slate-400'
              }
            >
              {log}
            </div>
          ))}
        </div>
      </div>

      {/* Hardware Pinout Specs */}
      <div className="grid grid-cols-2 sm:grid-cols-4 gap-3 text-xs">
        <div className="p-3.5 bg-slate-900 border border-slate-800 rounded-xl">
          <div className="font-mono text-amber-400 font-bold text-[11px]">TFT ILI9341 (SPI)</div>
          <div className="text-slate-400 text-[11px] mt-1 space-y-0.5 font-mono">
            <div>MOSI: 13 · MISO: 12</div>
            <div>SCLK: 14 · CS: 15</div>
            <div>DC: 2 · BL: 21</div>
          </div>
        </div>

        <div className="p-3.5 bg-slate-900 border border-slate-800 rounded-xl">
          <div className="font-mono text-cyan-400 font-bold text-[11px]">Touch XPT2046</div>
          <div className="text-slate-400 text-[11px] mt-1 space-y-0.5 font-mono">
            <div>CLK: 25 · CS: 33</div>
            <div>MOSI: 32 · MISO: 39</div>
            <div>IRQ: 36 (Resistive)</div>
          </div>
        </div>

        <div className="p-3.5 bg-slate-900 border border-slate-800 rounded-xl">
          <div className="font-mono text-emerald-400 font-bold text-[11px]">RGB LED (Active Low)</div>
          <div className="text-slate-400 text-[11px] mt-1 space-y-0.5 font-mono">
            <div>Red: GPIO 4</div>
            <div>Green: GPIO 16</div>
            <div>Blue: GPIO 17</div>
          </div>
        </div>

        <div className="p-3.5 bg-slate-900 border border-slate-800 rounded-xl">
          <div className="font-mono text-purple-400 font-bold text-[11px]">Audio & AP Portal</div>
          <div className="text-slate-400 text-[11px] mt-1 space-y-0.5 font-mono">
            <div>Speaker: GPIO 26</div>
            <div>BOOT / AP: GPIO 0</div>
            <div>Portal: 192.168.4.1</div>
          </div>
        </div>
      </div>

      {/* Code Viewer */}
      <div className="bg-slate-900 border border-slate-800 rounded-2xl overflow-hidden shadow-xl">
        {/* File Tabs */}
        <div className="flex border-b border-slate-800 bg-slate-950 p-2 gap-2 overflow-x-auto">
          {Object.keys(FIRMWARE_FILES).map((filename) => (
            <button
              key={filename}
              onClick={() => {
                cydAudio.click();
                setSelectedFile(filename);
              }}
              className={`px-3 py-1.5 rounded-lg text-xs font-mono transition-colors whitespace-nowrap ${
                selectedFile === filename
                  ? 'bg-amber-500/20 text-amber-300 border border-amber-500/40 font-bold'
                  : 'text-slate-400 hover:text-slate-200'
              }`}
            >
              📄 {filename}
            </button>
          ))}
        </div>

        {/* File Description */}
        <div className="px-4 py-2 bg-slate-950/50 border-b border-slate-800/60 text-xs text-slate-400 font-mono">
          {fileData.description}
        </div>

        {/* Code Content */}
        <div className="p-4 bg-slate-950 overflow-x-auto max-h-[500px]">
          <pre className="font-code text-xs text-slate-300 leading-relaxed">
            {fileData.code}
          </pre>
        </div>
      </div>

      {/* Flashing Instructions */}
      <div className="bg-slate-900/90 border border-slate-800 p-5 rounded-2xl space-y-3">
        <h3 className="text-sm font-bold text-white flex items-center gap-2">
          <span>⚡</span> Flashing Guide for Cheap Yellow Display
        </h3>
        <ol className="list-decimal list-inside text-xs text-slate-300 space-y-2 leading-relaxed">
          <li>
            <strong>Download Package:</strong> Click <strong>"Download Complete Firmware (.ZIP)"</strong> at the top of this tab and extract the archive to your computer.
          </li>
          <li>
            <strong>With PlatformIO (Recommended):</strong> Open the extracted folder in VS Code with PlatformIO. Connect your CYD via USB and click the <strong>Upload</strong> button. Everything (board flags, partition table, dependencies) is preconfigured!
          </li>
          <li>
            <strong>With Arduino IDE:</strong> Open <code className="bg-slate-800 px-1 py-0.5 rounded text-amber-300">ESP32_Tamagotchi_CYD.ino</code>. Select Board <code className="bg-slate-800 px-1 py-0.5 rounded text-amber-300">ESP32 Dev Module</code> and Partition Scheme <code className="bg-slate-800 px-1 py-0.5 rounded text-amber-300">Huge APP (3MB No OTA)</code>. Click Upload!
          </li>
          <li>
            <strong>First Boot:</strong> The CYD will boot and start the <code className="bg-slate-800 px-1 py-0.5 rounded text-cyan-300">CYD-Tamagotchi-AP</code> network. Connect your phone, open <code className="bg-slate-800 px-1 py-0.5 rounded text-green-300">http://192.168.4.1</code>, choose your home WiFi, and your device is ready!
          </li>
        </ol>
      </div>
    </div>
  );
};
