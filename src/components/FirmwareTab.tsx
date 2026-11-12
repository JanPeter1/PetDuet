import React, { useState, useRef, useEffect } from 'react';
import { FIRMWARE_FILES } from '../hardware_and_server/firmware_code';
import { cydAudio } from '../utils/audio';
import { Language } from '../utils/i18n';
import confetti from 'canvas-confetti';
import { ESPLoader, Transport } from 'esptool-js';
import type { IEspLoaderTerminal, FlashOptions } from 'esptool-js';

interface FirmwareTabProps {
  language?: Language;
}

interface LogEntry {
  id: string;
  timestamp: string;
  text: string;
  type: 'info' | 'success' | 'warn' | 'error' | 'serial';
}

export const FirmwareTab: React.FC<FirmwareTabProps> = ({ language = 'de' }) => {
  const [selectedFile, setSelectedFile] = useState<string>('ESP32_Tamagotchi_CYD.ino');
  const [copied, setCopied] = useState(false);
  const [flashingState, setFlashingState] = useState<
    'idle' | 'connecting' | 'connected' | 'erasing' | 'flashing' | 'success' | 'error'
  >('idle');
  const [progressPercent, setProgressPercent] = useState<number>(0);
  const [progressDetail, setProgressDetail] = useState<string>('');
  const [chipInfo, setChipInfo] = useState<{
    chipName: string;
    mac: string;
    flashSize: string;
  } | null>(null);

  const [binarySource, setBinarySource] = useState<'bundled' | 'custom'>('bundled');
  const [customFile, setCustomFile] = useState<File | null>(null);
  const [flashOffsetHex, setFlashOffsetHex] = useState<string>('0x10000');
  const [flashBaudRate, setFlashBaudRate] = useState<number>(460800);
  const [isIframeBlocked, setIsIframeBlocked] = useState(false);

  // Serial Monitor state
  const [isMonitoring, setIsMonitoring] = useState<boolean>(false);
  const [monitorInput, setMonitorInput] = useState<string>('');
  const [activePort, setActivePort] = useState<any>(null);

  const [logs, setLogs] = useState<LogEntry[]>([
    {
      id: 'init-1',
      timestamp: new Date().toLocaleTimeString(),
      text:
        language === 'de'
          ? '[SYSTEM] ESP32 Web Flasher initialisiert. Bereit zur Verbindung via Web Serial.'
          : '[SYSTEM] ESP32 Web Flasher initialized. Ready to connect via Web Serial.',
      type: 'info',
    },
    {
      id: 'init-2',
      timestamp: new Date().toLocaleTimeString(),
      text:
        language === 'de'
          ? '[TIPP] Schließe dein Cheap Yellow Display per Micro-USB an deinen Computer an.'
          : '[TIP] Connect your Cheap Yellow Display to your PC via Micro-USB.',
      type: 'info',
    },
  ]);

  const logContainerRef = useRef<HTMLDivElement>(null);
  const espLoaderRef = useRef<ESPLoader | null>(null);
  const transportRef = useRef<Transport | null>(null);
  const monitorReaderRef = useRef<any>(null);

  const fileData = FIRMWARE_FILES[selectedFile] || FIRMWARE_FILES['ESP32_Tamagotchi_CYD.ino'];

  // Auto-scroll logs
  useEffect(() => {
    if (logContainerRef.current) {
      logContainerRef.current.scrollTop = logContainerRef.current.scrollHeight;
    }
  }, [logs]);

  const addLog = (
    text: string,
    type: 'info' | 'success' | 'warn' | 'error' | 'serial' = 'info'
  ) => {
    const entry: LogEntry = {
      id: `${Date.now()}-${Math.random().toString(36).slice(2, 7)}`,
      timestamp: new Date().toLocaleTimeString(),
      text,
      type,
    };
    setLogs((prev) => [...prev.slice(-300), entry]);
  };

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

  const handleCustomFileChange = (e: React.ChangeEvent<HTMLInputElement>) => {
    if (e.target.files && e.target.files[0]) {
      const file = e.target.files[0];
      setCustomFile(file);
      setBinarySource('custom');
      addLog(
        language === 'de'
          ? `[DATEI] Eigene Firmware geladen: ${file.name} (${(file.size / 1024).toFixed(1)} KB)`
          : `[FILE] Custom firmware loaded: ${file.name} (${(file.size / 1024).toFixed(1)} KB)`,
        'info'
      );
    }
  };

  // Helper to disconnect and clean up serial transport
  const cleanupTransport = async () => {
    try {
      if (monitorReaderRef.current) {
        await monitorReaderRef.current.cancel();
        monitorReaderRef.current = null;
      }
      if (transportRef.current) {
        await transportRef.current.disconnect();
        transportRef.current = null;
      }
      if (activePort) {
        try {
          await activePort.close();
        } catch {}
        setActivePort(null);
      }
      espLoaderRef.current = null;
      setIsMonitoring(false);
    } catch (e) {
      console.warn('Cleanup error:', e);
    }
  };

  // Web Serial Flasher Execution
  const handleFlashFirmware = async () => {
    cydAudio.click();

    if (!('serial' in navigator)) {
      addLog(
        language === 'de'
          ? '[FEHLER] Web Serial API wird von diesem Browser nicht unterstützt. Bitte nutze Google Chrome, Microsoft Edge oder Opera.'
          : '[ERROR] Web Serial API is not supported in this browser. Please use Google Chrome, Microsoft Edge, or Opera.',
        'error'
      );
      cydAudio.warning();
      return;
    }

    setFlashingState('connecting');
    setProgressPercent(0);
    setProgressDetail(language === 'de' ? 'Warte auf Portauswahl...' : 'Waiting for port selection...');
    addLog(
      language === 'de'
        ? '[SERIAL] Bitte wähle deinen ESP32 CYD im Browser-Dialog aus (z.B. CH340 / CP2102)...'
        : '[SERIAL] Please select your ESP32 CYD in the browser prompt (e.g. CH340 / CP2102)...',
      'info'
    );

    let device: any = null;

    try {
      device = await (navigator as any).serial.requestPort();
    } catch (err: any) {
      if (err.name === 'SecurityError') {
        setIsIframeBlocked(true);
        addLog(
          language === 'de'
            ? '[SICHERHEIT] Web Serial Zugriff wird im iFrame blockiert. Klicke auf "In neuem Tab öffnen" oben rechts!'
            : '[SECURITY] Web Serial access is restricted in iframe. Click "Open in New Tab" above!',
          'warn'
        );
      } else {
        addLog(
          language === 'de'
            ? `[ABBRUCH] Verbindung abgebrochen: ${err.message || 'Kein Port gewählt.'}`
            : `[CANCEL] Connection cancelled: ${err.message || 'No port selected.'}`,
          'warn'
        );
      }
      setFlashingState('idle');
      return;
    }

    try {
      await cleanupTransport();
      setActivePort(device);

      const terminal: IEspLoaderTerminal = {
        clean: () => {},
        writeLine: (data: string) => {
          if (data && data.trim()) {
            addLog(`[ESP] ${data.trim()}`, 'info');
          }
        },
        write: (data: string) => {
          if (data && data.trim()) {
            addLog(`[ESP] ${data.trim()}`, 'info');
          }
        },
      };

      const transport = new Transport(device, true);
      transportRef.current = transport;

      addLog(
        language === 'de'
          ? `[BOOTLOADER] Synchronisiere mit ROM-Bootloader bei 115200 Baud...`
          : `[BOOTLOADER] Synchronizing with ROM bootloader at 115200 baud...`,
        'info'
      );

      const espLoader = new ESPLoader({
        transport,
        baudrate: flashBaudRate,
        romBaudrate: 115200,
        terminal,
      });
      espLoaderRef.current = espLoader;

      // Connect and retrieve chip info
      const chipName = await espLoader.main();
      let macAddr = 'Unknown';
      let flashSizeStr = '4MB (Standard CYD)';

      try {
        if (espLoader.chip && typeof (espLoader.chip as any).getMacAddress === 'function') {
          macAddr = await (espLoader.chip as any).getMacAddress();
        }
      } catch {}

      try {
        await espLoader.flashId();
        flashSizeStr = '4MB (SPI Flash)';
      } catch {}

      setChipInfo({
        chipName: chipName || 'ESP32-D0WD-V3',
        mac: macAddr,
        flashSize: flashSizeStr,
      });

      addLog(
        language === 'de'
          ? `[CHIP ERKANNT] ${chipName} | MAC: ${macAddr} | Flash: ${flashSizeStr}`
          : `[CHIP DETECTED] ${chipName} | MAC: ${macAddr} | Flash: ${flashSizeStr}`,
        'success'
      );

      // Load binary data
      let binaryBuffer: ArrayBuffer;
      let binaryName = '';

      if (binarySource === 'custom' && customFile) {
        binaryName = customFile.name;
        addLog(
          language === 'de'
            ? `[FIRMWARE] Lese benutzerdefinierte Datei: ${binaryName} (${customFile.size} Bytes)...`
            : `[FIRMWARE] Reading custom file: ${binaryName} (${customFile.size} Bytes)...`,
          'info'
        );
        binaryBuffer = await customFile.arrayBuffer();
      } else {
        binaryName = 'cyd_tamagotchi.bin';
        addLog(
          language === 'de'
            ? `[FIRMWARE] Lade vorkompilierte CYD-Tamagotchi-Firmware vom Server...`
            : `[FIRMWARE] Fetching bundled CYD Tamagotchi firmware image from server...`,
          'info'
        );
        const resp = await fetch('/firmware/cyd_tamagotchi.bin');
        if (!resp.ok) {
          throw new Error(`Konnte Firmware nicht laden: HTTP ${resp.status}`);
        }
        binaryBuffer = await resp.arrayBuffer();
      }

      const flashData = new Uint8Array(binaryBuffer);
      const targetAddress = parseInt(flashOffsetHex, 16) || 0x10000;

      addLog(
        language === 'de'
          ? `[FLASHEN] Schreibe ${flashData.byteLength} Bytes an Adresse ${flashOffsetHex} (Baudrate: ${flashBaudRate})...`
          : `[FLASHING] Writing ${flashData.byteLength} bytes to offset ${flashOffsetHex} (Baudrate: ${flashBaudRate})...`,
        'info'
      );

      setFlashingState('flashing');

      const flashOptions: FlashOptions = {
        fileArray: [
          {
            data: flashData,
            address: targetAddress,
          },
        ],
        flashMode: 'dio',
        flashFreq: '40m',
        flashSize: '4MB',
        eraseAll: false,
        compress: true,
        reportProgress: (_fileIndex: number, written: number, total: number) => {
          const pct = Math.min(100, Math.floor((written / total) * 100));
          setProgressPercent(pct);
          setProgressDetail(`${(written / 1024).toFixed(0)} KB / ${(total / 1024).toFixed(0)} KB (${pct}%)`);
        },
      };

      await espLoader.writeFlash(flashOptions);

      setProgressPercent(100);
      setProgressDetail('100% - Verifiziert!');
      addLog(
        language === 'de'
          ? '[ERFOLG] Flash-Vorgang erfolgreich abgeschlossen! Starte Chip neu...'
          : '[SUCCESS] Firmware flashed successfully! Rebooting chip...',
        'success'
      );

      // Perform chip reset
      try {
        await espLoader.after('hard_reset');
        addLog(
          language === 'de'
            ? '[RESET] Hard-Reset gesendet. ESP32 startet nun das Tamagotchi RPG!'
            : '[RESET] Hard-reset command executed. ESP32 is now booting Tamagotchi RPG!',
          'success'
        );
      } catch (rstErr: any) {
        addLog(`[RESET] Hinweis: ${rstErr.message || 'Chip bootet neu.'}`, 'info');
      }

      setFlashingState('success');
      cydAudio.levelUp();
      confetti({ particleCount: 120, spread: 80, origin: { y: 0.6 } });
    } catch (err: any) {
      console.error('Flashing failed:', err);
      setFlashingState('error');
      cydAudio.warning();
      addLog(
        language === 'de'
          ? `[FLASH-FEHLER] ${err?.message || 'Unbekannter Flash-Fehler'}. Prüfe die USB-Kabelverbindung (BOOT-Taste gedrückt halten bei Flash-Start).`
          : `[FLASH ERROR] ${err?.message || 'Unknown flashing error'}. Check USB cable (hold BOOT button while connecting if needed).`,
        'error'
      );
    } finally {
      // Clean up after 2.5s so serial monitor can be used
      setTimeout(() => {
        cleanupTransport();
      }, 2500);
    }
  };

  // Erase entire flash
  const handleEraseFlash = async () => {
    cydAudio.click();

    if (!confirm(
      language === 'de'
        ? 'Möchtest du den gesamten Flash-Speicher des ESP32 wirklich löschen? Dies entfernt alle bestehenden Daten und NVS-Einstellungen.'
        : 'Do you really want to erase the entire flash memory of the ESP32? This will remove all existing data and NVS settings.'
    )) {
      return;
    }

    setFlashingState('erasing');
    addLog(
      language === 'de'
        ? '[ERASE] Verbinde mit Bootloader zum vollständigen Löschen...'
        : '[ERASE] Connecting to bootloader for full chip erase...',
      'warn'
    );

    try {
      const device = await (navigator as any).serial.requestPort();
      await cleanupTransport();
      setActivePort(device);

      const terminal: IEspLoaderTerminal = {
        clean: () => {},
        writeLine: (data: string) => addLog(`[ESP] ${data}`, 'info'),
        write: (data: string) => addLog(`[ESP] ${data}`, 'info'),
      };

      const transport = new Transport(device, true);
      transportRef.current = transport;

      const espLoader = new ESPLoader({
        transport,
        baudrate: 115200,
        terminal,
      });

      await espLoader.main();
      addLog(language === 'de' ? '[ERASE] Lösche Flash-Speicher (dauert ca. 10-20 Sekunden)...' : '[ERASE] Erasing entire flash (takes 10-20 seconds)...', 'warn');
      await espLoader.eraseFlash();

      addLog(language === 'de' ? '[ERFOLG] Flash-Speicher vollständig gelöscht!' : '[SUCCESS] Flash memory completely erased!', 'success');
      cydAudio.levelUp();
      setFlashingState('idle');
    } catch (err: any) {
      addLog(`[ERASE-FEHLER] ${err.message}`, 'error');
      setFlashingState('error');
    } finally {
      cleanupTransport();
    }
  };

  // Live Serial Monitor
  const handleToggleMonitor = async () => {
    cydAudio.click();

    if (isMonitoring) {
      await cleanupTransport();
      addLog(language === 'de' ? '[MONITOR] Serieller Monitor getrennt.' : '[MONITOR] Serial monitor disconnected.', 'info');
      return;
    }

    if (!('serial' in navigator)) {
      addLog('[ERROR] Web Serial API not supported in this browser.', 'error');
      return;
    }

    try {
      addLog(
        language === 'de'
          ? '[MONITOR] Wähle den seriellen Port für den Live-Monitor (115200 Baud)...'
          : '[MONITOR] Select serial port for live monitoring (115200 baud)...',
        'info'
      );
      const port = await (navigator as any).serial.requestPort();
      await port.open({ baudRate: 115200 });
      setActivePort(port);
      setIsMonitoring(true);
      addLog(
        language === 'de'
          ? '[MONITOR] Verbunden bei 115200 Baud! Empfange Live-Logs vom ESP32 CYD...'
          : '[MONITOR] Connected at 115200 baud! Receiving live output from ESP32 CYD...',
        'success'
      );

      const textDecoder = new TextDecoderStream();
      const readableStreamClosed = port.readable.pipeTo(textDecoder.writable);
      const reader = textDecoder.readable.getReader();
      monitorReaderRef.current = reader;

      (async () => {
        let buffer = '';
        try {
          while (true) {
            const { value, done } = await reader.read();
            if (done) break;
            if (value) {
              buffer += value;
              const lines = buffer.split('\n');
              buffer = lines.pop() || '';
              for (const line of lines) {
                if (line.trim()) {
                  addLog(line.replace(/\r/g, ''), 'serial');
                }
              }
            }
          }
        } catch (streamErr: any) {
          console.warn('Stream closed:', streamErr);
        } finally {
          reader.releaseLock();
          await readableStreamClosed.catch(() => {});
        }
      })();
    } catch (err: any) {
      addLog(`[MONITOR FEHLER] ${err.message}`, 'error');
      setIsMonitoring(false);
    }
  };

  // Send serial command to ESP32
  const handleSendSerialCommand = async () => {
    if (!monitorInput.trim() || !activePort || !activePort.writable) return;
    cydAudio.click();
    try {
      const encoder = new TextEncoder();
      const writer = activePort.writable.getWriter();
      await writer.write(encoder.encode(monitorInput + '\n'));
      writer.releaseLock();
      addLog(`> ${monitorInput}`, 'info');
      setMonitorInput('');
    } catch (e: any) {
      addLog(`[SEND ERROR] ${e.message}`, 'error');
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
                HW Reference:{' '}
                <a
                  href="https://github.com/JanPeter1/ESP_CoGo"
                  target="_blank"
                  rel="noreferrer"
                  className="underline hover:text-amber-400"
                >
                  JanPeter1/ESP_CoGo
                </a>
              </span>
            </div>
            <h2 className="text-xl font-bold text-white mt-1.5">
              {language === 'de'
                ? 'ESP32 Cheap Yellow Display Firmware & Web-Flasher'
                : 'Production C++ Firmware & Web Flasher for Cheap Yellow Display'}
            </h2>
            <p className="text-sm text-slate-300 mt-1 max-w-2xl leading-relaxed">
              {language === 'de'
                ? 'Flashe die Tamagotchi-RPG-Firmware direkt aus dem Browser per Web Serial auf deinen ESP32 CYD, oder lade das fertige PlatformIO-/Arduino-Projekt herunter.'
                : 'Flash the Tamagotchi RPG firmware directly from your browser via Web Serial to your ESP32 CYD, or download the preconfigured PlatformIO/Arduino project.'}
            </p>
          </div>

          {/* Primary Action Buttons */}
          <div className="flex flex-wrap items-center gap-2 shrink-0">
            <a
              href="/api/download/firmware"
              onClick={() => cydAudio.levelUp()}
              className="px-4 py-2.5 bg-amber-500 hover:bg-amber-400 active:scale-95 text-slate-950 text-xs font-bold rounded-lg shadow-lg transition-all flex items-center gap-1.5"
            >
              <span>📦</span>{' '}
              {language === 'de' ? 'Komplettes Firmware-Paket (.ZIP)' : 'Download Firmware Package (.ZIP)'}
            </a>
            <button
              onClick={handleCopy}
              className="px-3.5 py-2.5 bg-slate-800 hover:bg-slate-700 active:scale-95 text-slate-200 text-xs font-semibold rounded-lg border border-slate-700 transition-all flex items-center gap-1.5"
            >
              {copied ? '✓ Copied!' : language === 'de' ? '📋 Datei kopieren' : '📋 Copy File'}
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

      {/* Web Serial Flasher & Diagnostics Studio */}
      <div className="bg-slate-900 border border-slate-800 p-6 rounded-2xl shadow-xl space-y-5">
        <div className="flex flex-col sm:flex-row sm:items-center justify-between gap-4 border-b border-slate-800 pb-4">
          <div>
            <div className="text-base font-bold text-white flex items-center gap-2">
              <span className="text-xl">⚡</span>
              <span>
                {language === 'de'
                  ? 'Web Serial ESP32 Flasher & Live-Monitor'
                  : 'Web Serial ESP32 Flasher & Live Monitor'}
              </span>
              <span className="px-2 py-0.5 rounded text-[10px] font-mono font-bold bg-sky-500/20 text-sky-300 border border-sky-500/30">
                esptool-js v0.7
              </span>
            </div>
            <p className="text-xs text-slate-400 mt-1">
              {language === 'de'
                ? 'Verbinde dein Cheap Yellow Display per USB. Der Browser überträgt die Firmware direkt auf das Board.'
                : 'Connect your Cheap Yellow Display via USB. The browser flashes the firmware directly to the board.'}
            </p>
          </div>

          {/* Status Badge */}
          <div className="flex items-center gap-3">
            {chipInfo && (
              <div className="px-3 py-1 bg-emerald-950 border border-emerald-500/40 rounded-lg text-[11px] font-mono text-emerald-300">
                ✓ {chipInfo.chipName} ({chipInfo.flashSize})
              </div>
            )}
            <div
              className={`px-3 py-1 rounded-lg text-xs font-mono font-bold uppercase tracking-wider ${
                flashingState === 'flashing'
                  ? 'bg-amber-500 text-slate-950 animate-pulse'
                  : flashingState === 'success'
                  ? 'bg-emerald-500 text-slate-950'
                  : flashingState === 'error'
                  ? 'bg-rose-500 text-white'
                  : 'bg-slate-800 text-slate-300'
              }`}
            >
              {flashingState}
            </div>
          </div>
        </div>

        {/* Security / Iframe Notice */}
        {isIframeBlocked && (
          <div className="p-4 bg-amber-950/60 border border-amber-500/40 rounded-xl text-xs text-amber-200 flex flex-col sm:flex-row items-start sm:items-center justify-between gap-3">
            <div>
              <strong className="font-bold">
                {language === 'de' ? 'Browser-Berechtigung:' : 'Browser Permission:'}
              </strong>{' '}
              {language === 'de'
                ? 'Web-Serial ist in eingebetteten iFrames oft blockiert. Öffne die App direkt in einem neuen Vollbild-Tab, um uneingeschränkt auf USB-Ports zuzugreifen!'
                : 'Web Serial is often restricted in embedded iframes. Open the app directly in a full browser tab for full USB device access!'}
            </div>
            <button
              onClick={() => window.open(window.location.href, '_blank')}
              className="px-3.5 py-1.5 bg-amber-500 hover:bg-amber-400 text-slate-950 font-bold rounded-lg shrink-0 active:scale-95 transition-all"
            >
              ↗ {language === 'de' ? 'In neuem Tab öffnen' : 'Open in New Tab'}
            </button>
          </div>
        )}

        {/* Flasher Controls Grid */}
        <div className="grid grid-cols-1 md:grid-cols-3 gap-4">
          {/* Binary Source Card */}
          <div className="bg-slate-950 p-4 rounded-xl border border-slate-800 space-y-3">
            <div className="text-xs font-bold text-slate-200 flex items-center justify-between">
              <span>{language === 'de' ? '1. Firmware-Auswahl' : '1. Firmware Binary'}</span>
              <span className="text-[10px] font-mono text-slate-500">.bin</span>
            </div>

            <div className="space-y-2 text-xs">
              <label className="flex items-center gap-2 cursor-pointer p-2 rounded-lg bg-slate-900 border border-slate-800 hover:border-amber-500/50">
                <input
                  type="radio"
                  name="binSource"
                  checked={binarySource === 'bundled'}
                  onChange={() => setBinarySource('bundled')}
                  className="text-amber-500 focus:ring-amber-500"
                />
                <div>
                  <div className="font-semibold text-slate-200">
                    {language === 'de' ? 'Vorkompiliertes Image' : 'Bundled CYD Firmware'}
                  </div>
                  <div className="text-[10px] text-slate-400">cyd_tamagotchi.bin (Auto-Fetch)</div>
                </div>
              </label>

              <label className="flex items-center gap-2 cursor-pointer p-2 rounded-lg bg-slate-900 border border-slate-800 hover:border-amber-500/50">
                <input
                  type="radio"
                  name="binSource"
                  checked={binarySource === 'custom'}
                  onChange={() => setBinarySource('custom')}
                  className="text-amber-500 focus:ring-amber-500"
                />
                <div className="flex-1">
                  <div className="font-semibold text-slate-200">
                    {language === 'de' ? 'Eigene .bin Datei' : 'Upload custom .bin'}
                  </div>
                  <div className="text-[10px] text-slate-400 truncate">
                    {customFile ? customFile.name : language === 'de' ? 'Keine Datei gewählt' : 'No file chosen'}
                  </div>
                </div>
              </label>

              {binarySource === 'custom' && (
                <input
                  type="file"
                  accept=".bin"
                  onChange={handleCustomFileChange}
                  className="w-full text-[11px] text-slate-400 file:mr-2 file:py-1 file:px-2 file:rounded file:border-0 file:text-[10px] file:bg-slate-800 file:text-slate-200 hover:file:bg-slate-700"
                />
              )}
            </div>
          </div>

          {/* Flash Settings Card */}
          <div className="bg-slate-950 p-4 rounded-xl border border-slate-800 space-y-3">
            <div className="text-xs font-bold text-slate-200 flex items-center justify-between">
              <span>{language === 'de' ? '2. Flash-Parameter' : '2. Flash Parameters'}</span>
              <span className="text-[10px] font-mono text-slate-500">ESP32</span>
            </div>

            <div className="space-y-2.5 text-xs">
              <div>
                <label className="block text-[11px] text-slate-400 mb-1">
                  {language === 'de' ? 'Flash-Offset / Adresse' : 'Flash Memory Offset'}
                </label>
                <select
                  value={flashOffsetHex}
                  onChange={(e) => setFlashOffsetHex(e.target.value)}
                  className="w-full bg-slate-900 border border-slate-800 rounded-lg px-2.5 py-1.5 text-xs text-slate-200 font-mono focus:border-amber-500"
                >
                  <option value="0x10000">0x10000 (Standard App Partition)</option>
                  <option value="0x0">0x0 (Factory Merged Binary)</option>
                  <option value="0x1000">0x1000 (Bootloader)</option>
                  <option value="0x8000">0x8000 (Partition Table)</option>
                </select>
              </div>

              <div>
                <label className="block text-[11px] text-slate-400 mb-1">
                  {language === 'de' ? 'Baudrate (Geschwindigkeit)' : 'Upload Speed'}
                </label>
                <select
                  value={flashBaudRate}
                  onChange={(e) => setFlashBaudRate(Number(e.target.value))}
                  className="w-full bg-slate-900 border border-slate-800 rounded-lg px-2.5 py-1.5 text-xs text-slate-200 font-mono focus:border-amber-500"
                >
                  <option value={460800}>460800 Baud (Empfohlen / Fast)</option>
                  <option value={921600}>921600 Baud (Ultra-Fast)</option>
                  <option value={115200}>115200 Baud (Sicher / Safe)</option>
                </select>
              </div>
            </div>
          </div>

          {/* Action Execution Card */}
          <div className="bg-slate-950 p-4 rounded-xl border border-slate-800 space-y-3 flex flex-col justify-between">
            <div className="text-xs font-bold text-slate-200 flex items-center justify-between">
              <span>{language === 'de' ? '3. Flashen & Steuerung' : '3. Flash & Control'}</span>
              <span className="text-[10px] font-mono text-emerald-400 font-bold">1-Click</span>
            </div>

            <div className="space-y-2">
              <button
                onClick={handleFlashFirmware}
                disabled={flashingState === 'flashing' || flashingState === 'connecting'}
                className="w-full py-2.5 bg-gradient-to-r from-amber-500 to-amber-600 hover:from-amber-400 hover:to-amber-500 disabled:opacity-50 text-slate-950 font-bold text-xs rounded-xl shadow-lg transition-transform active:scale-95 flex items-center justify-center gap-2"
              >
                <span>⚡</span>
                <span>
                  {flashingState === 'flashing'
                    ? language === 'de'
                      ? 'Flashe ESP32...'
                      : 'Flashing ESP32...'
                    : language === 'de'
                    ? 'Firmware jetzt auf ESP32 flashen'
                    : 'Flash Firmware to ESP32 Now'}
                </span>
              </button>

              <div className="grid grid-cols-2 gap-2">
                <button
                  onClick={handleEraseFlash}
                  disabled={flashingState === 'flashing'}
                  className="py-1.5 bg-rose-950/60 hover:bg-rose-900/80 border border-rose-500/40 text-rose-300 font-semibold text-[11px] rounded-lg transition-all active:scale-95"
                >
                  🧹 {language === 'de' ? 'Flash löschen' : 'Erase Flash'}
                </button>
                <button
                  onClick={handleToggleMonitor}
                  className={`py-1.5 border font-semibold text-[11px] rounded-lg transition-all active:scale-95 ${
                    isMonitoring
                      ? 'bg-emerald-600 text-slate-950 border-emerald-400 font-bold'
                      : 'bg-slate-900 hover:bg-slate-800 text-slate-300 border-slate-700'
                  }`}
                >
                  📺 {isMonitoring ? (language === 'de' ? 'Monitor stoppen' : 'Stop Monitor') : language === 'de' ? 'Live Monitor' : 'Live Monitor'}
                </button>
              </div>
            </div>
          </div>
        </div>

        {/* Flashing Progress Bar */}
        {(flashingState === 'flashing' || flashingState === 'success') && (
          <div className="space-y-1.5 bg-slate-950 p-4 rounded-xl border border-slate-800">
            <div className="flex items-center justify-between text-xs font-mono">
              <span className="text-amber-400 font-bold">
                {flashingState === 'success'
                  ? language === 'de'
                    ? '✓ Flash erfolgreich verifiziert!'
                    : '✓ Flash successfully verified!'
                  : language === 'de'
                  ? '⚡ Übertrage Firmware-Blöcke...'
                  : '⚡ Writing firmware blocks...'}
              </span>
              <span className="text-slate-300">{progressDetail}</span>
            </div>
            <div className="w-full bg-slate-900 rounded-full h-3 overflow-hidden border border-slate-800">
              <div
                className="bg-gradient-to-r from-amber-500 to-emerald-400 h-full transition-all duration-150 ease-out"
                style={{ width: `${progressPercent}%` }}
              />
            </div>
          </div>
        )}

        {/* Live Serial Console & Monitor */}
        <div className="bg-slate-950 rounded-xl border border-slate-800 overflow-hidden">
          <div className="px-4 py-2 bg-slate-950 border-b border-slate-800 flex items-center justify-between text-xs font-mono text-slate-400">
            <div className="flex items-center gap-2">
              <span className="w-2 h-2 rounded-full bg-emerald-500 animate-pulse" />
              <span>{language === 'de' ? 'Serielle Konsole (115200 Baud)' : 'Serial Console (115200 Baud)'}</span>
            </div>
            <div className="flex items-center gap-2">
              <button
                onClick={() => setLogs([])}
                className="hover:text-slate-200 text-[11px] underline"
              >
                {language === 'de' ? 'Log leeren' : 'Clear log'}
              </button>
            </div>
          </div>

          <div
            ref={logContainerRef}
            className="p-3.5 max-h-52 min-h-36 overflow-y-auto font-mono text-[11px] space-y-1"
          >
            {logs.map((log) => (
              <div
                key={log.id}
                className={`leading-relaxed ${
                  log.type === 'success'
                    ? 'text-emerald-400 font-semibold'
                    : log.type === 'error'
                    ? 'text-rose-400 font-bold'
                    : log.type === 'warn'
                    ? 'text-amber-400'
                    : log.type === 'serial'
                    ? 'text-cyan-300'
                    : 'text-slate-400'
                }`}
              >
                <span className="text-slate-600 mr-2 select-none">[{log.timestamp}]</span>
                <span>{log.text}</span>
              </div>
            ))}
          </div>

          {/* Serial Monitor Input Command Bar */}
          {isMonitoring && (
            <div className="p-2 border-t border-slate-800 bg-slate-900 flex items-center gap-2">
              <span className="text-xs font-mono text-slate-400 px-1">&gt;</span>
              <input
                type="text"
                value={monitorInput}
                onChange={(e) => setMonitorInput(e.target.value)}
                onKeyDown={(e) => e.key === 'Enter' && handleSendSerialCommand()}
                placeholder={language === 'de' ? 'Befehl an ESP32 senden (z.B. STATUS, RESET)...' : 'Send command to ESP32 (e.g. STATUS, RESET)...'}
                className="flex-1 bg-slate-950 border border-slate-800 rounded px-2.5 py-1 text-xs text-slate-200 font-mono focus:outline-none focus:border-amber-500"
              />
              <button
                onClick={handleSendSerialCommand}
                className="px-3 py-1 bg-amber-500 hover:bg-amber-400 text-slate-950 font-bold text-xs rounded active:scale-95"
              >
                Send
              </button>
            </div>
          )}
        </div>
      </div>

      {/* Alternative Flashing Methods (CLI & IDE) */}
      <div className="grid grid-cols-1 md:grid-cols-2 gap-4">
        {/* PlatformIO (Recommended) */}
        <div className="bg-slate-900 border border-slate-800 p-5 rounded-2xl space-y-3">
          <div className="flex items-center justify-between">
            <span className="font-bold text-white text-sm flex items-center gap-2">
              <span>🚀</span> PlatformIO (1-Klick Upload in VS Code)
            </span>
            <span className="px-2 py-0.5 rounded text-[10px] font-mono bg-emerald-500/20 text-emerald-300 font-bold">
              {language === 'de' ? 'Empfohlen' : 'Recommended'}
            </span>
          </div>
          <p className="text-xs text-slate-400 leading-relaxed">
            {language === 'de'
              ? 'Die beiliegende platformio.ini konfiguriert alle TFT_eSPI Flags, Display-Pins und Bibliotheken vollautomatisch.'
              : 'The included platformio.ini sets up all TFT_eSPI flags, display pins, and libraries automatically.'}
          </p>
          <div className="bg-slate-950 p-3 rounded-xl border border-slate-800 font-mono text-xs text-amber-300 flex items-center justify-between">
            <code>pio run --target upload</code>
            <button
              onClick={() => {
                cydAudio.click();
                navigator.clipboard.writeText('pio run --target upload');
              }}
              className="text-[10px] text-slate-400 hover:text-white px-2 py-1 bg-slate-800 rounded border border-slate-700 active:scale-95"
            >
              Copy
            </button>
          </div>
        </div>

        {/* esptool CLI */}
        <div className="bg-slate-900 border border-slate-800 p-5 rounded-2xl space-y-3">
          <div className="flex items-center justify-between">
            <span className="font-bold text-white text-sm flex items-center gap-2">
              <span>💻</span> esptool.py (Terminal / Command-Line)
            </span>
            <span className="px-2 py-0.5 rounded text-[10px] font-mono bg-cyan-500/20 text-cyan-300 font-bold">
              Python
            </span>
          </div>
          <p className="text-xs text-slate-400 leading-relaxed">
            {language === 'de'
              ? 'Direktes Flashen über die offizielle Espressif Python-CLI für Linux, macOS und Windows.'
              : 'Direct flashing using the official Espressif Python CLI for Linux, macOS, and Windows.'}
          </p>
          <div className="bg-slate-950 p-3 rounded-xl border border-slate-800 font-mono text-xs text-cyan-300 flex items-center justify-between">
            <code className="truncate max-w-[260px]">
              esptool.py --port COM3 --baud 460800 write_flash 0x10000 firmware.bin
            </code>
            <button
              onClick={() => {
                cydAudio.click();
                navigator.clipboard.writeText(
                  'esptool.py --chip esp32 --port COM3 --baud 460800 write_flash -z 0x10000 firmware.bin'
                );
              }}
              className="text-[10px] text-slate-400 hover:text-white px-2 py-1 bg-slate-800 rounded border border-slate-700 active:scale-95 shrink-0 ml-2"
            >
              Copy
            </button>
          </div>
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
    </div>
  );
};
