import React, { useState } from 'react';
import { FriendPetSummary } from '../types/pet';
import { cydAudio } from '../utils/audio';
import { translations, Language } from '../utils/i18n';

interface ChatModalProps {
  friendPet: FriendPetSummary;
  language?: Language;
  onClose: () => void;
  onSendMessage: (message: string) => void;
}

export const ChatModal: React.FC<ChatModalProps> = ({
  friendPet,
  language = 'de',
  onClose,
  onSendMessage,
}) => {
  const [customText, setCustomText] = useState('');
  const t = translations[language] || translations.de;

  const quickMessages = [
    t.chatPreset1,
    t.chatPreset2,
    t.chatPreset3,
    t.chatPreset4,
    t.chatPreset5,
    t.chatPreset6,
    t.chatPreset7,
    t.chatPreset8,
  ];

  const handleSend = (text: string) => {
    if (!text.trim()) return;
    cydAudio.chatPing();
    onSendMessage(text.trim());
    onClose();
  };

  return (
    <div className="fixed inset-0 z-50 bg-black/80 backdrop-blur-sm flex items-center justify-center p-3 font-sans-clean">
      <div className="w-full max-w-md bg-slate-900 border border-slate-700 rounded-xl shadow-2xl flex flex-col overflow-hidden">
        {/* Header */}
        <div className="flex items-center justify-between px-5 py-3.5 bg-slate-950 border-b border-slate-800">
          <div className="flex items-center gap-2">
            <span className="font-pixel text-xs text-sky-400">
              {language === 'de' ? 'CYD SCHNELL-CHAT' : 'CYD QUICK CHAT'}
            </span>
            <span className="text-xs text-slate-400 font-mono">To: {friendPet.petName}</span>
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
        <div className="p-4 space-y-4">
          <div>
            <label className="text-xs font-semibold text-slate-300 block mb-2">
              {language === 'de' ? 'CYD LCD Schnellnachrichten (Direktsendung)' : 'Preset CYD LCD Messages (Instant send)'}
            </label>
            <div className="grid grid-cols-2 gap-2">
              {quickMessages.map((msg, idx) => (
                <button
                  key={idx}
                  onClick={() => handleSend(msg)}
                  className="p-2 text-left bg-slate-800/80 hover:bg-slate-700 active:bg-sky-900 border border-slate-700 hover:border-sky-500/50 rounded-lg text-xs text-slate-200 transition-colors"
                >
                  {msg}
                </button>
              ))}
            </div>
          </div>

          <div className="border-t border-slate-800 pt-3">
            <label className="text-xs font-semibold text-slate-300 block mb-1">
              {language === 'de' ? 'Eigene Nachricht (CYD 2,8" Bildschirmspeicher)' : 'Custom Message (CYD 2.8" LCD Screen Buffer)'}
            </label>
            <div className="flex gap-2">
              <input
                type="text"
                maxLength={30}
                value={customText}
                onChange={(e) => setCustomText(e.target.value)}
                onKeyDown={(e) => {
                  if (e.key === 'Enter') handleSend(customText);
                }}
                placeholder={language === 'de' ? 'Kurznachricht eingeben...' : 'Type short message...'}
                className="flex-1 bg-slate-950 border border-slate-700 rounded-lg px-3 py-2 text-xs text-slate-100 focus:outline-none focus:border-sky-500 font-pixel"
              />
              <button
                onClick={() => handleSend(customText)}
                disabled={!customText.trim()}
                className="px-4 py-2 bg-sky-600 hover:bg-sky-500 disabled:bg-slate-800 disabled:text-slate-500 text-white font-bold rounded-lg text-xs transition-transform active:scale-95"
              >
                {language === 'de' ? 'Senden 💬' : 'Send 💬'}
              </button>
            </div>
            <div className="text-[10px] text-slate-500 mt-1 font-mono">
              {language === 'de' ? 'Max. 30 Zeichen für ILI9341 320x240 LCD Puffer' : 'Max 30 chars for ILI9341 320x240 display buffer'}
            </div>
          </div>
        </div>
      </div>
    </div>
  );
};
