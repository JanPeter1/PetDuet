import React, { useState } from 'react';
import { PHP_SERVER_FILES } from '../hardware_and_server/php_code';
import { PetState } from '../types/pet';
import { cydAudio } from '../utils/audio';

interface PHPServerTabProps {
  myPet: PetState;
}

export const PHPServerTab: React.FC<PHPServerTabProps> = ({ myPet }) => {
  const [selectedFile, setSelectedFile] = useState<string>('register.php');
  const [copied, setCopied] = useState(false);
  const [apiResponse, setApiResponse] = useState<string | null>(null);
  const [apiLoading, setApiLoading] = useState(false);

  const fileData = PHP_SERVER_FILES[selectedFile] || PHP_SERVER_FILES['register.php'];

  const handleCopy = () => {
    cydAudio.click();
    navigator.clipboard.writeText(fileData.code);
    setCopied(true);
    setTimeout(() => setCopied(false), 2000);
  };

  const handleDownload = () => {
    cydAudio.click();
    const blob = new Blob([fileData.code], { type: 'text/plain;charset=utf-8' });
    const url = URL.createObjectURL(blob);
    const link = document.createElement('a');
    link.href = url;
    link.download = fileData.filename;
    link.click();
    URL.revokeObjectURL(url);
  };

  // Live test against the server endpoints
  const testApi = async (endpoint: string, method = 'GET', body?: any) => {
    cydAudio.click();
    setApiLoading(true);
    try {
      const res = await fetch(endpoint, {
        method,
        headers: { 'Content-Type': 'application/json' },
        body: body ? JSON.stringify(body) : undefined,
      });
      const data = await res.json();
      setApiResponse(JSON.stringify(data, null, 2));
    } catch (err: any) {
      setApiResponse(`Error calling API: ${err?.message}`);
    } finally {
      setApiLoading(false);
    }
  };

  return (
    <div className="space-y-6 font-sans-clean">
      {/* Overview Banner */}
      <div className="bg-gradient-to-r from-blue-950/60 via-slate-900 to-slate-950 p-6 rounded-2xl border border-blue-500/30">
        <div className="flex flex-col md:flex-row md:items-center justify-between gap-4">
          <div>
            <div className="flex items-center gap-2">
              <span className="px-2 py-0.5 rounded text-[10px] font-mono font-bold bg-blue-500/20 text-blue-300 border border-blue-500/40">
                PHP 7.4+ / 8.x Zero-Dependency
              </span>
              <span className="text-xs text-slate-400 font-mono">
                Supports cPanel, Apache, Nginx, or Raspberry Pi
              </span>
            </div>
            <h2 className="text-xl font-bold text-white mt-1.5">
              Production PHP Server for CYD Registration & Peer Sync
            </h2>
            <p className="text-sm text-slate-300 mt-1 max-w-2xl leading-relaxed">
              Provides lightweight endpoints for CYDs to register IP addresses, locate friend devices on LAN/WAN, relay treat/chat packets, maintain cloud saves, and power global leaderboards.
            </p>
          </div>

          <div className="flex flex-wrap items-center gap-2 shrink-0">
            <a
              href="/api/download/php-server"
              onClick={() => cydAudio.levelUp()}
              className="px-4 py-2.5 bg-blue-600 hover:bg-blue-500 active:scale-95 text-white text-xs font-bold rounded-lg shadow-lg transition-all flex items-center gap-1.5"
            >
              <span>📦</span> Download PHP Server (.ZIP)
            </a>
            <button
              onClick={handleCopy}
              className="px-3.5 py-2.5 bg-slate-800 hover:bg-slate-700 active:scale-95 text-slate-200 text-xs font-semibold rounded-lg border border-slate-700 transition-all flex items-center gap-1.5"
            >
              {copied ? '✓ Copied!' : '📋 Copy File'}
            </button>
            <button
              onClick={handleDownload}
              className="px-3.5 py-2.5 bg-slate-800 hover:bg-slate-700 active:scale-95 text-slate-200 text-xs font-semibold rounded-lg border border-slate-700 transition-all flex items-center gap-1.5"
            >
              ⬇️ {fileData.filename}
            </button>
          </div>
        </div>
      </div>

      {/* Live API Testing Console */}
      <div className="bg-slate-900 border border-slate-800 p-5 rounded-2xl space-y-4">
        <div className="flex items-center justify-between">
          <div className="flex items-center gap-2">
            <span className="text-sm font-bold text-white">🧪 Live PHP API Console</span>
            <span className="text-xs text-slate-400 font-mono">Test server endpoints right now</span>
          </div>
          {apiLoading && <span className="text-xs text-amber-400 animate-pulse font-mono">Calling endpoint...</span>}
        </div>

        <div className="flex flex-wrap gap-2">
          {/* Test 1: Register */}
          <button
            onClick={() =>
              testApi('/api/register', 'POST', {
                deviceId: myPet.deviceId,
                deviceName: myPet.trainerName,
                petName: myPet.name,
                biome: myPet.biome,
                level: myPet.level,
                phase: myPet.phase,
                ipAddress: '192.168.1.142',
                port: 8888,
                stats: myPet.stats,
              })
            }
            className="px-3 py-1.5 bg-slate-800 hover:bg-slate-700 text-slate-200 rounded-lg text-xs font-mono border border-slate-700"
          >
            POST /register.php
          </button>

          {/* Test 2: Peers */}
          <button
            onClick={() => testApi('/api/peers', 'GET')}
            className="px-3 py-1.5 bg-slate-800 hover:bg-slate-700 text-slate-200 rounded-lg text-xs font-mono border border-slate-700"
          >
            GET /peers.php
          </button>

          {/* Test 3: Leaderboard */}
          <button
            onClick={() => testApi('/api/leaderboard', 'GET')}
            className="px-3 py-1.5 bg-slate-800 hover:bg-slate-700 text-slate-200 rounded-lg text-xs font-mono border border-slate-700"
          >
            GET /leaderboard.php
          </button>

          {/* Test 4: Cloud Save */}
          <button
            onClick={() =>
              testApi('/api/cloud_save', 'POST', {
                deviceId: myPet.deviceId,
                petData: myPet,
              })
            }
            className="px-3 py-1.5 bg-slate-800 hover:bg-slate-700 text-slate-200 rounded-lg text-xs font-mono border border-slate-700"
          >
            POST /cloud_save.php
          </button>
        </div>

        {/* Console Output Window */}
        {apiResponse && (
          <div className="p-3 bg-slate-950 rounded-xl border border-slate-800 max-h-48 overflow-y-auto">
            <div className="text-[10px] text-slate-500 font-mono mb-1">Server JSON Response:</div>
            <pre className="font-code text-xs text-emerald-400 whitespace-pre-wrap">
              {apiResponse}
            </pre>
          </div>
        )}
      </div>

      {/* Code Viewer */}
      <div className="bg-slate-900 border border-slate-800 rounded-2xl overflow-hidden shadow-xl">
        {/* File Tabs */}
        <div className="flex border-b border-slate-800 bg-slate-950 p-2 gap-2 overflow-x-auto">
          {Object.keys(PHP_SERVER_FILES).map((filename) => (
            <button
              key={filename}
              onClick={() => {
                cydAudio.click();
                setSelectedFile(filename);
              }}
              className={`px-3 py-1.5 rounded-lg text-xs font-mono transition-colors whitespace-nowrap ${
                selectedFile === filename
                  ? 'bg-blue-500/20 text-blue-300 border border-blue-500/40 font-bold'
                  : 'text-slate-400 hover:text-slate-200'
              }`}
            >
              🐘 {filename}
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
