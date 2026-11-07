import React, { useState } from 'react';
import { PetState, InventoryItem, FriendPetSummary, TradeOffer } from '../types/pet';
import { CATALOG_ITEMS } from '../utils/petLogic';
import { cydAudio } from '../utils/audio';
import { translations, Language } from '../utils/i18n';

interface TradingModalProps {
  myPet: PetState;
  friendPet: FriendPetSummary;
  pendingOffers: TradeOffer[];
  language?: Language;
  onClose: () => void;
  onSendTradeOffer: (offeredItem: InventoryItem, requestedItemName: string) => void;
  onAcceptTrade: (offer: TradeOffer) => void;
  onDeclineTrade: (offer: TradeOffer) => void;
}

export const TradingModal: React.FC<TradingModalProps> = ({
  myPet,
  friendPet,
  pendingOffers,
  language = 'de',
  onClose,
  onSendTradeOffer,
  onAcceptTrade,
  onDeclineTrade,
}) => {
  const t = translations[language] || translations.de;
  const [selectedOfferItem, setSelectedOfferItem] = useState<InventoryItem | null>(
    myPet.inventory[0] || null
  );
  const [selectedRequestItem, setSelectedRequestItem] = useState<string>(CATALOG_ITEMS[0].name);

  const tradeableInventory = myPet.inventory.filter((i) => i.quantity > 0);

  return (
    <div className="fixed inset-0 z-50 bg-black/80 backdrop-blur-sm flex items-center justify-center p-3 font-sans-clean">
      <div className="w-full max-w-lg bg-slate-900 border border-slate-700 rounded-xl shadow-2xl flex flex-col overflow-hidden max-h-[90vh]">
        {/* Header */}
        <div className="flex items-center justify-between px-5 py-3.5 bg-slate-950 border-b border-slate-800">
          <div className="flex items-center gap-2">
            <span className="font-pixel text-xs text-amber-400">{t.tradingTitle}</span>
            <span className="text-xs text-slate-400 font-mono">⇄ {friendPet.deviceName}</span>
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
          {/* Pending Offers from Friend */}
          {pendingOffers.length > 0 && (
            <div className="bg-amber-950/40 border border-amber-500/50 p-3.5 rounded-xl space-y-2">
              <div className="text-xs font-semibold text-amber-300 flex items-center gap-1.5">
                <span>🎁</span> {t.pendingTradeFrom} {friendPet.petName}!
              </div>
              {pendingOffers.map((offer) => (
                <div
                  key={offer.id}
                  className="bg-slate-900 p-3 rounded-lg border border-slate-700 flex items-center justify-between"
                >
                  <div className="text-xs">
                    <span className="text-slate-300">{language === 'de' ? 'Bietet: ' : 'Offers: '}</span>
                    <span className="text-amber-300 font-semibold">{offer.offeredItem.icon} {offer.offeredItem.name}</span>
                    <div className="text-slate-400 text-[11px] mt-0.5">
                      {language === 'de' ? 'Möchte: ' : 'Wants: '}<span className="text-sky-300">{offer.requestedItemName}</span>
                    </div>
                  </div>
                  <div className="flex items-center gap-2">
                    <button
                      onClick={() => onAcceptTrade(offer)}
                      className="px-3 py-1.5 bg-emerald-600 hover:bg-emerald-500 text-slate-950 font-bold rounded text-xs active:scale-95"
                    >
                      {t.btnAcceptTrade}
                    </button>
                    <button
                      onClick={() => onDeclineTrade(offer)}
                      className="px-2.5 py-1.5 bg-slate-800 hover:bg-slate-700 text-slate-400 rounded text-xs"
                    >
                      {t.btnDeclineTrade}
                    </button>
                  </div>
                </div>
              ))}
            </div>
          )}

          {/* Create New Trade Offer */}
          <div className="bg-slate-950 p-4 rounded-xl border border-slate-800 space-y-3">
            <div className="text-xs font-semibold text-slate-200">
              {t.proposeTradeTitle} {friendPet.petName}
            </div>

            <div className="grid grid-cols-1 sm:grid-cols-2 gap-3">
              {/* Select from Your Bag */}
              <div>
                <label className="text-[11px] text-slate-400 block mb-1">
                  {t.labelYouOffer}
                </label>
                <select
                  value={selectedOfferItem?.id || ''}
                  onChange={(e) => {
                    const item = myPet.inventory.find((i) => i.id === e.target.value);
                    if (item) setSelectedOfferItem(item);
                  }}
                  className="w-full bg-slate-900 border border-slate-700 rounded-lg p-2 text-xs text-slate-200 focus:outline-none focus:border-amber-500"
                >
                  {tradeableInventory.map((item) => (
                    <option key={item.id} value={item.id}>
                      {item.icon} {item.name} (x{item.quantity})
                    </option>
                  ))}
                </select>
                {selectedOfferItem && (
                  <div className="text-[10px] text-slate-400 mt-1 italic">
                    {selectedOfferItem.description}
                  </div>
                )}
              </div>

              {/* Select Desired Item */}
              <div>
                <label className="text-[11px] text-slate-400 block mb-1">
                  {t.labelYouRequest}
                </label>
                <select
                  value={selectedRequestItem}
                  onChange={(e) => setSelectedRequestItem(e.target.value)}
                  className="w-full bg-slate-900 border border-slate-700 rounded-lg p-2 text-xs text-slate-200 focus:outline-none focus:border-amber-500"
                >
                  {CATALOG_ITEMS.map((item) => (
                    <option key={item.id} value={item.name}>
                      {item.icon} {item.name} ({item.rarity})
                    </option>
                  ))}
                </select>
                <div className="text-[10px] text-slate-400 mt-1 italic">
                  {language === 'de' ? 'Seltenes Zubehör oder Deko-Gegenstand' : 'Rare accessory or habitat decoration'}
                </div>
              </div>
            </div>

            <button
              disabled={!selectedOfferItem}
              onClick={() => {
                if (selectedOfferItem) {
                  cydAudio.click();
                  onSendTradeOffer(selectedOfferItem, selectedRequestItem);
                }
              }}
              className="w-full py-2.5 bg-amber-600 hover:bg-amber-500 disabled:bg-slate-800 disabled:text-slate-500 text-slate-950 font-bold rounded-lg text-xs transition-transform active:scale-95 shadow"
            >
              {t.btnSendTrade}
            </button>
          </div>

          {/* Rare Accessories & Habitat Decorations Showcase */}
          <div className="space-y-2">
            <div className="text-xs font-semibold text-slate-300">
              {t.collectiblesCatalog}
            </div>
            <div className="grid grid-cols-2 sm:grid-cols-3 gap-2">
              {CATALOG_ITEMS.map((item) => (
                <div
                  key={item.id}
                  className="p-2.5 rounded-lg bg-slate-800/60 border border-slate-700/60 flex items-center gap-2"
                >
                  <span className="text-xl">{item.icon}</span>
                  <div className="overflow-hidden">
                    <div className="text-xs font-medium text-slate-200 truncate">{item.name}</div>
                    <div className="text-[9px] uppercase tracking-wider text-amber-400 font-mono">
                      {item.rarity}
                    </div>
                  </div>
                </div>
              ))}
            </div>
          </div>
        </div>
      </div>
    </div>
  );
};
