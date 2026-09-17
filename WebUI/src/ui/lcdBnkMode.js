// BNK (Bank Browse) mode (extracted from lcdPanel.js).
// The BNK button sits above SET. While active, ▲/▼ step through the LIBRARY
// banks (wrapping) and ◀/▶ step through the patches of the active bank
// (wrapping), loading each selection into the engine like the header select.
// The LCD shows the bank name on line 1 and the patch name on line 2; names
// longer than the display scroll character-by-character (LCD-style) after a
// short pause, reversing direction once the full name has been shown.
//
// Factory pattern: receives the shared LCD ctx (DOM refs, bank library deps
// and lazy getters to the sibling modes) and returns the BNK API the core
// wires. Cross-mode calls (exitMdlMode / exitSetupMode / exitWriteMode) go
// through getters so the sibling factories can be created in any order.
import { BANK_SLOTS as BANK_SLOTS_CONST, occupiedCount, describeModel } from '../contracts/bankLibrary.js';

export function createBnkMode(ctx) {
  const {
    lcdLine1,
    lcdLine2,
    selectPresetSlot,
    getBankLib,
    activeBank,
    activateBank,
    loadSavedPresetSlot,
    loadPresetSlot,
    bnkLine1,
    bnkLine2,
    // Lazy cross-mode getters (resolved at call time).
    getExitMdlMode,
    getExitSetupMode,
    getExitWriteMode,
    getUpdateLCD,
    getResetInactivity,
    getStopNormalLcdScroll,
    getLcdCompareMode,
    setLcdCompareMode,
  } = ctx;

  let lcdBnkMode = false;

  // Wrappers keep the original internal names so call sites read naturally.
  const stopNormalLcdScroll = () => getStopNormalLcdScroll()();
  const exitMdlMode = () => getExitMdlMode()();
  const exitSetupMode = () => getExitSetupMode()();
  const exitWriteMode = () => getExitWriteMode()();
  const updateLCD = () => getUpdateLCD()();
  const resetLCDInactivity = () => getResetInactivity()();

  // Bank + patch names to show, from the library (immediate, no engine round-trip).
  const bnkNames = () => {
    const b = activeBank();
    const bankName = b ? (b.name || 'BANK') : '—';
    const idx = parseInt(selectPresetSlot?.value || 0);
    const patchName = (b && b.presets && b.presets[idx] && b.presets[idx].name)
      ? b.presets[idx].name
      : (selectPresetSlot && selectPresetSlot.options[idx])
        ? selectPresetSlot.options[idx].text.split(': ')[1] || 'Init User'
        : 'Init User';
    return { bankName, patchName };
  };

  // ── Position badge (LCD corner) ──
  // Shared by Bank Browse (BNK), Write and Compare mode for a coherent readout:
  // the bank MODEL on top and the position below (bank N/total + slot/occupied
  // patches, e.g. B3/12 · P07/24). In Write mode the slot is the selected
  // destination; in Compare it is the slot being A/B-ed.
  //
  // Corner: BNK and Compare use the top-right corner; Write uses the bottom-left
  // so the badge never collides with the "WRITE: SELECT SLOT" line 1 readout.
  const showBnkBadge = (corner = 'top-right') => {
    const badge = document.getElementById('lcd-bnk-badge');
    if (!badge) return;
    badge.style.display = '';
    badge.classList.toggle('corner-bottom-left', corner === 'bottom-left');
  };

  const hideBnkBadge = () => {
    const badge = document.getElementById('lcd-bnk-badge');
    if (badge) badge.style.display = 'none';
    // Release the reserved corners (line 1 right in BNK/Compare, line 2 left in Write).
    if (lcdLine1) lcdLine1.style.paddingRight = '';
    if (lcdLine2) lcdLine2.style.paddingLeft = '';
  };

  const renderBnkBadge = (slotOverride, opts = {}) => {
    const badge = document.getElementById('lcd-bnk-badge');
    if (!badge) return;
    const { warnOccupiedSlot = false } = opts; // Write: highlight an overwrite target
    const lib = getBankLib();
    const banks = (lib && lib.banks) || [];
    const bank = banks.find(b => b.id === (lib && lib.activeBankId));
    const bankPos = banks.findIndex(b => b.id === (lib && lib.activeBankId)) + 1;
    const bankTotal = banks.length;
    const slot = slotOverride !== undefined ? slotOverride : (parseInt(selectPresetSlot?.value || 0) + 1);
    const occupied = bank ? occupiedCount(bank) : 0;
    const modelEl = document.getElementById('lcd-bnk-model');
    const posEl = document.getElementById('lcd-bnk-pos');
    if (modelEl) {
      modelEl.textContent = bank ? describeModel(bank.model) : '';
      // Model tag colour: user banks green, factory banks gray (glanceable).
      modelEl.classList.toggle('lcd-bnk-model-user', !!bank && bank.model === 'user');
      modelEl.classList.toggle('lcd-bnk-model-factory', !!bank && bank.model !== 'user');
    }
    // Position row: bank N/total, slot/occupied patches (e.g. B3/12 · P07/24).
    // The occupied count is coloured: green when the bank has patches, amber when
    // it is empty. In Write mode the slot number itself turns amber when it
    // already holds a patch — a clear "you are overwriting" warning.
    if (posEl) {
      const occClass = occupied > 0 ? 'lcd-occ-ok' : 'lcd-occ-empty';
      let slotHtml = `P${String(slot).padStart(2, '0')}`;
      if (warnOccupiedSlot && bank && bank.presets && bank.presets[slot - 1]) {
        const p = bank.presets[slot - 1];
        if (p && !(p.name || '').startsWith('Init')) {
          slotHtml = `<span class="lcd-slot-occupied">${slotHtml}</span>`;
        }
      }
      posEl.innerHTML = `B${bankPos}/${bankTotal} · ${slotHtml}<span class="lcd-occ ${occClass}">/${occupied}</span>`;
    }
    // Reserve exactly as much space as the badge occupies so the line text —
    // scrolling or not — never runs under it. Which line depends on the corner:
    // top-right reserves line 1 (BNK/Compare), bottom-left reserves line 2 (Write).
    const reserve = badge.style.display === 'none' ? '' : `${Math.max(40, badge.offsetWidth + 6)}px`;
    if (lcdLine1) lcdLine1.style.paddingRight = badge.classList.contains('corner-bottom-left') ? '' : reserve;
    if (lcdLine2) lcdLine2.style.paddingLeft = badge.classList.contains('corner-bottom-left') ? reserve : '';
  };

  const renderBnkLcd = () => {
    if (!lcdLine1 || !lcdLine2) return;
    const { bankName, patchName } = bnkNames();
    bnkLine1.set(bankName);
    bnkLine2.set(patchName);
    renderBnkBadge();
  };

  function enterBnkMode() {
    stopNormalLcdScroll();
    exitMdlMode(); // Browse modes are mutually exclusive
    setLcdCompareMode(false);
    exitSetupMode(); // unpress SET / leave System Mode
    exitWriteMode();
    lcdBnkMode = true;
    const btnBnk = document.getElementById('btn-lcd-bnk');
    if (btnBnk) btnBnk.classList.add('active');
    const display = document.getElementById('lcd-display');
    if (display) display.classList.add('bnk-active'); // reserve corner space
    showBnkBadge();
    renderBnkLcd();
    resetLCDInactivity();
  }

  function exitBnkMode() {
    if (!lcdBnkMode) return;
    lcdBnkMode = false;
    stopBnkScroll();
    const btnBnk = document.getElementById('btn-lcd-bnk');
    if (btnBnk) btnBnk.classList.remove('active');
    const display = document.getElementById('lcd-display');
    if (display) display.classList.remove('bnk-active');
    hideBnkBadge();
    updateLCD();
  }

  function toggleBnkMode() {
    if (lcdBnkMode) exitBnkMode(); else enterBnkMode();
  }

  // ▲/▼: previous/next bank (wrapping). Loads the bank (engine + library) and
  // shows bank/patch names; the slot resolves like onBankLoaded (per-bank
  // remembered slot, else 0).
  const cycleBank = (dir) => {
    const lib = getBankLib();
    const banks = (lib && lib.banks) || [];
    if (!banks.length) return;
    const curIdx = banks.findIndex(b => b.id === (lib && lib.activeBankId));
    if (curIdx === -1) return;
    const nextIdx = (curIdx + dir + banks.length) % banks.length;
    const nextBank = banks[nextIdx];
    if (nextBank.id === (lib && lib.activeBankId)) return;
    activateBank(nextBank.id);
    const saved = loadSavedPresetSlot();
    const slot = (saved && saved.bankId === nextBank.id && saved.slot >= 0 && saved.slot < BANK_SLOTS_CONST)
      ? saved.slot : 0;
    if (selectPresetSlot) selectPresetSlot.value = slot;
    renderBnkLcd();
  };

  // ◀/▶: previous/next patch inside the active bank (wrapping), loading it.
  const cyclePatch = (dir) => {
    const total = selectPresetSlot ? selectPresetSlot.options.length : 0;
    if (!total) return;
    const cur = parseInt(selectPresetSlot?.value || 0);
    loadPresetSlot((cur + dir + total) % total);
    renderBnkLcd();
  };

  const stopBnkScroll = () => {
    bnkLine1.stop();
    bnkLine2.stop();
  };

  return {
    bnkNames,
    showBnkBadge,
    hideBnkBadge,
    renderBnkBadge,
    renderBnkLcd,
    enterBnkMode,
    exitBnkMode,
    toggleBnkMode,
    cycleBank,
    cyclePatch,
    getLcdBnkMode: () => lcdBnkMode,
    setLcdBnkMode: (v) => { lcdBnkMode = v; },
  };
}
