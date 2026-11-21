import React, { useState, useEffect } from 'react';
import { PetState, InventoryItem, TradeOffer } from '../types/pet';
import { CATALOG_ITEMS } from '../utils/petLogic';
import { cydAudio } from '../utils/audio';
import { translations, Language } from '../utils/i18n';

interface TradingModalProps {
  dev1Pet: PetState;
  dev2Pet: PetState;
  activePerspective: 'DEV1' | 'DEV2';
  onChangePerspective: (side: 'DEV1' | 'DEV2') => void;
  pendingOffers: TradeOffer[];
  language?: Language;
  onClose: () => void;
  onSendTradeOffer: (
    fromSide: 'DEV1' | 'DEV2',
    offeredItem: InventoryItem,
    requestedItemName: string,
    requestedItemId?: string
  ) => void;
  onAcceptTrade: (offer: TradeOffer) => void;
  onDeclineTrade: (offer: TradeOffer) => void;
  onCancelTrade: (offer: TradeOffer) => void;
}

export const TradingModal: React.FC<TradingModalProps> = ({
  dev1Pet,
  dev2Pet,
  activePerspective,
  onChangePerspective,
  pendingOffers,
  language = 'de',
  onClose,
  onSendTradeOffer,
  onAcceptTrade,
  onDeclineTrade,
  onCancelTrade,
}) => {
  const t = translations[language] || translations.de;

  const senderPet = activePerspective === 'DEV1' ? dev1Pet : dev2Pet;
  const partnerPet = activePerspective === 'DEV1' ? dev2Pet : dev1Pet;

  const tradeableInventory = senderPet.inventory.filter((i) => i.quantity > 0);
  const partnerInventory = partnerPet.inventory.filter((i) => i.quantity > 0);

  const [selectedOfferItemId, setSelectedOfferItemId] = useState<string>(
    tradeableInventory[0]?.id || ''
  );

  // Request value encoding: '__FREE__' | 'inv:<id>' | 'cat:<id>'
  const [selectedRequestValue, setSelectedRequestValue] = useState<string>(() => {
    if (partnerInventory[0]) return `inv:${partnerInventory[0].id}`;
    return `cat:${CATALOG_ITEMS[0].id}`;
  });

  // Synchronize selected items cleanly whenever inventory or perspective changes
  useEffect(() => {
    if (!tradeableInventory.some((i) => i.id === selectedOfferItemId)) {
      setSelectedOfferItemId(tradeableInventory[0]?.id || '');
    }
  }, [activePerspective, senderPet.inventory]);

  useEffect(() => {
    if (selectedRequestValue.startsWith('inv:')) {
      const reqId = selectedRequestValue.slice(4);
      if (!partnerInventory.some((i) => i.id === reqId)) {
        setSelectedRequestValue(
          partnerInventory[0] ? `inv:${partnerInventory[0].id}` : `cat:${CATALOG_ITEMS[0].id}`
        );
      }
    }
  }, [activePerspective, partnerPet.inventory]);

  const selectedOfferItem =
    tradeableInventory.find((i) => i.id === selectedOfferItemId) || tradeableInventory[0] || null;

  const incomingForActive = pendingOffers.filter(
    (o) => o.toDeviceId === senderPet.deviceId && o.status === 'pending'
  );
  const outgoingFromActive = pendingOffers.filter(
    (o) => o.fromDeviceId === senderPet.deviceId && o.status === 'pending'
  );

  const getLocalizedName = (item: { name: string; name_de?: string }) =>
    language === 'de' ? item.name_de || item.name : item.name;

  const getLocalizedDesc = (item: { description: string; description_de?: string }) =>
    language === 'de' ? item.description_de || item.description : item.description;

  const doesActiveOwnRequestedItem = (offer: TradeOffer): boolean => {
    if (!offer.requestedItemId) return true; // Free gift or name-only fallback
    return senderPet.inventory.some(
      (i) => i.id === offer.requestedItemId && i.quantity > 0
    );
  };

  const resolveRequestSelection = (): { requestedId: string; requestedName: string } => {
    if (selectedRequestValue === '__FREE__') {
      return { requestedId: '', requestedName: t.freeGiftLabel };
    }
    if (selectedRequestValue.startsWith('inv:')) {
      const id = selectedRequestValue.slice(4);
      const found = partnerInventory.find((i) => i.id === id);
      if (found) {
        return { requestedId: found.id, requestedName: getLocalizedName(found) };
      }
    }
    if (selectedRequestValue.startsWith('cat:')) {
      const id = selectedRequestValue.slice(4);
      const found = CATALOG_ITEMS.find((i) => i.id === id);
      if (found) {
        return { requestedId: found.id, requestedName: getLocalizedName(found) };
      }
    }
    return { requestedId: '', requestedName: t.freeGiftLabel };
  };

  return (
    <div className="fixed inset-0 z-50 bg-black/80 backdrop-blur-sm flex items-center justify-center p-3 font-sans-clean">
      <div className="w-full max-w-xl bg-slate-900 border border-slate-700 rounded-xl shadow-2xl flex flex-col overflow-hidden max-h-[90vh]">
        {/* Header */}
        <div className="flex flex-wrap items-center justify-between gap-2 px-5 py-3.5 bg-slate-950 border-b border-slate-800">
          <div className="flex items-center gap-2">
            <span className="font-pixel text-xs text-amber-400">{t.tradingTitle}</span>
            <span className="text-xs text-slate-400 font-mono">
              {senderPet.name} ⇄ {partnerPet.name}
            </span>
          </div>

          <div className="flex items-center gap-2">
            {/* Bidirectional Device Switcher */}
            <div className="flex items-center gap-1 bg-slate-900 border border-slate-800 rounded-lg p-0.5 text-[11px] font-mono">
              <button
                onClick={() => {
                  cydAudio.click();
                  onChangePerspective('DEV1');
                }}
                className={`px-2 py-1 rounded transition-colors ${
                  activePerspective === 'DEV1'
                    ? 'bg-amber-500 text-slate-950 font-bold'
                    : 'text-slate-400 hover:text-white'
                }`}
              >
                CYD #1 ({dev1Pet.name})
              </button>
              <button
                onClick={() => {
                  cydAudio.click();
                  onChangePerspective('DEV2');
                }}
                className={`px-2 py-1 rounded transition-colors ${
                  activePerspective === 'DEV2'
                    ? 'bg-cyan-500 text-slate-950 font-bold'
                    : 'text-slate-400 hover:text-white'
                }`}
              >
                CYD #2 ({dev2Pet.name})
              </button>
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
        </div>

        {/* Content */}
        <div className="p-4 sm:p-5 overflow-y-auto space-y-4">
          {/* Incoming Offers for Active Device */}
          <div className={`p-3.5 rounded-xl border space-y-2 ${
            incomingForActive.length > 0
              ? 'bg-amber-950/40 border-amber-500/60'
              : 'bg-slate-950/60 border-slate-800'
          }`}>
            <div className="text-xs font-semibold text-amber-300 flex items-center justify-between">
              <span className="flex items-center gap-1.5">
                <span>📥</span> {t.pendingTradeFrom} {senderPet.name} ({incomingForActive.length})
              </span>
            </div>

            {incomingForActive.length === 0 ? (
              <div className="text-xs text-slate-500 italic">{t.tradeNoOffersIncoming}</div>
            ) : (
              incomingForActive.map((offer) => {
                const canFulfill = doesActiveOwnRequestedItem(offer);
                return (
                  <div
                    key={offer.id}
                    className="bg-slate-900 p-3 rounded-lg border border-slate-700 flex flex-col sm:flex-row sm:items-center justify-between gap-2"
                  >
                    <div className="text-xs space-y-0.5">
                      <div>
                        <span className="text-slate-400">
                          {offer.fromPetName} {language === 'de' ? 'bietet: ' : 'offers: '}
                        </span>
                        <span className="text-amber-300 font-semibold">
                          {offer.offeredItem.icon} {getLocalizedName(offer.offeredItem)}
                        </span>
                      </div>
                      <div className="text-slate-400 text-[11px]">
                        {language === 'de' ? 'Gegenleistung: ' : 'Wants: '}
                        <span className={offer.requestedItemId ? 'text-sky-300 font-medium' : 'text-emerald-300 font-medium'}>
                          {offer.requestedItemName}
                        </span>
                        {!canFulfill && (
                          <span className="ml-2 text-rose-400 font-mono text-[10px]">
                            ({language === 'de' ? 'Fehlt im Beutel' : 'Missing in bag'})
                          </span>
                        )}
                      </div>
                    </div>

                    <div className="flex items-center gap-2 shrink-0">
                      <button
                        onClick={() => onAcceptTrade(offer)}
                        className={`px-3 py-1.5 font-bold rounded text-xs transition-transform active:scale-95 ${
                          canFulfill
                            ? 'bg-emerald-600 hover:bg-emerald-500 text-slate-950'
                            : 'bg-rose-950/70 hover:bg-rose-900 text-rose-300 border border-rose-700/60'
                        }`}
                      >
                        {t.btnAcceptTrade}
                      </button>
                      <button
                        onClick={() => onDeclineTrade(offer)}
                        className="px-2.5 py-1.5 bg-slate-800 hover:bg-slate-700 text-slate-300 rounded text-xs"
                      >
                        {t.btnDeclineTrade}
                      </button>
                    </div>
                  </div>
                );
              })
            )}
          </div>

          {/* Outgoing Escrowed Offers (Locked until Accepted/Declined/Cancelled) */}
          {outgoingFromActive.length > 0 && (
            <div className="bg-cyan-950/30 border border-cyan-500/40 p-3.5 rounded-xl space-y-2">
              <div className="text-xs font-semibold text-cyan-300 flex items-center gap-1.5">
                <span>🔒</span> {t.outgoingEscrowTitle} ({outgoingFromActive.length})
              </div>
              {outgoingFromActive.map((offer) => (
                <div
                  key={offer.id}
                  className="bg-slate-900 p-3 rounded-lg border border-slate-700 flex items-center justify-between gap-2"
                >
                  <div className="text-xs space-y-0.5">
                    <div>
                      <span className="text-cyan-300 font-semibold">
                        {offer.offeredItem.icon} {getLocalizedName(offer.offeredItem)}
                      </span>
                      <span className="text-slate-400 mx-1.5">→</span>
                      <span className="text-amber-300">{offer.requestedItemName}</span>
                    </div>
                    <div className="text-[10px] text-slate-400 font-mono">
                      Escrow → {partnerPet.name} ({offer.toDeviceId})
                    </div>
                  </div>
                  <button
                    onClick={() => onCancelTrade(offer)}
                    className="px-3 py-1.5 bg-rose-900/80 hover:bg-rose-800 text-rose-100 font-semibold rounded text-xs transition-colors shrink-0"
                  >
                    {t.btnCancelTrade}
                  </button>
                </div>
              ))}
            </div>
          )}

          {/* Create New Escrow Trade Offer */}
          <div className="bg-slate-950 p-4 rounded-xl border border-slate-800 space-y-3">
            <div className="text-xs font-semibold text-slate-200 flex items-center justify-between">
              <span>
                {t.proposeTradeTitle} {partnerPet.name}
              </span>
              <span className="text-[10px] font-mono text-slate-400">
                {senderPet.deviceId} → {partnerPet.deviceId}
              </span>
            </div>

            <div className="grid grid-cols-1 sm:grid-cols-2 gap-3">
              {/* Select from Active Device's Bag */}
              <div>
                <label className="text-[11px] text-slate-400 block mb-1">
                  {t.labelYouOffer}
                </label>
                {tradeableInventory.length > 0 ? (
                  <select
                    value={selectedOfferItem?.id || ''}
                    onChange={(e) => setSelectedOfferItemId(e.target.value)}
                    className="w-full bg-slate-900 border border-slate-700 rounded-lg p-2 text-xs text-slate-200 focus:outline-none focus:border-amber-500"
                  >
                    {tradeableInventory.map((item) => (
                      <option key={item.id} value={item.id}>
                        {item.icon} {getLocalizedName(item)} (x{item.quantity})
                      </option>
                    ))}
                  </select>
                ) : (
                  <div className="p-2 rounded-lg bg-slate-900 border border-slate-800 text-xs text-slate-500 italic">
                    {t.tradeEmptyBag}
                  </div>
                )}
                {selectedOfferItem && (
                  <div className="text-[10px] text-slate-400 mt-1 italic">
                    {getLocalizedDesc(selectedOfferItem)}
                  </div>
                )}
              </div>

              {/* Select Desired Return Item */}
              <div>
                <label className="text-[11px] text-slate-400 block mb-1">
                  {t.labelYouRequest}
                </label>
                <select
                  value={selectedRequestValue}
                  onChange={(e) => setSelectedRequestValue(e.target.value)}
                  className="w-full bg-slate-900 border border-slate-700 rounded-lg p-2 text-xs text-slate-200 focus:outline-none focus:border-amber-500"
                >
                  <option value="__FREE__">🎁 {t.freeGiftLabel}</option>
                  {partnerInventory.length > 0 && (
                    <optgroup label={t.tradePartnerBagOptGroup}>
                      {partnerInventory.map((item) => (
                        <option key={`inv-${item.id}`} value={`inv:${item.id}`}>
                          {item.icon} {getLocalizedName(item)} (x{item.quantity})
                        </option>
                      ))}
                    </optgroup>
                  )}
                  <optgroup label={t.tradeCatalogOptGroup}>
                    {CATALOG_ITEMS.map((item) => (
                      <option key={`cat-${item.id}`} value={`cat:${item.id}`}>
                        {item.icon} {getLocalizedName(item)} ({item.rarity})
                      </option>
                    ))}
                  </optgroup>
                </select>
                <div className="text-[10px] text-slate-400 mt-1 italic">
                  {selectedRequestValue === '__FREE__'
                    ? t.freeGiftLabel
                    : language === 'de'
                    ? 'Partner muss Gegenstand zum Annehmen besitzen'
                    : 'Partner must hold requested item to accept'}
                </div>
              </div>
            </div>

            <button
              disabled={!selectedOfferItem}
              onClick={() => {
                if (selectedOfferItem) {
                  cydAudio.click();
                  const { requestedId, requestedName } = resolveRequestSelection();
                  onSendTradeOffer(activePerspective, selectedOfferItem, requestedName, requestedId);
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
                    <div className="text-xs font-medium text-slate-200 truncate">
                      {getLocalizedName(item)}
                    </div>
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
