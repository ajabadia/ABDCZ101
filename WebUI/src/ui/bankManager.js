// Enhanced Bank Manager (ABDEEP-style multi-bank library) — extracted module.
// Every bank is tagged with its synth model (fixing the "sysex de un modelo u
// otro" mess). The library is persisted in localStorage; the engine stays the
// source of truth for sound data and reports preset names back after every
// load. Extracted from app.js to keep responsibilities separated.
//
// Factory pattern (same as envelopeEditor / lcdPanel / modMatrix): the module
// owns the library state (bankLib) and receives the live app references (engine
// getter, LCD lines, overlay opener) through deps. Mutable state that app.js
// must read (saveActivePresetSlot, the LCD panel wiring) comes back through
// getBankLib(); the load/sync helpers used by the engine callbacks are
// returned directly.
import { PARAMETER_REGISTRY, PARAM_MAP } from '../contracts/registry.gen.js';
import { FACTORY_PRESETS } from '../contracts/factoryPresets.js';
import { parseSyxFile, humanizePatchNames } from '../contracts/syxNames.js';
import { CZ230S_BANK_NAMES, CZ_PACK1_NAMES, factoryBankNames } from '../contracts/factoryBankNames.js';
import { addCZ5000Banks } from '../contracts/cz5000Banks.js';
import {
  BANK_MODELS,
  BANK_SLOTS as BANK_SLOTS_CONST,
  createBank,
  findBank,
  getActiveBank,
  setActiveBankId,
  addBank,
  removeBank,
  renameBank,
  setBankPresets,
  setBankSnapshot,
  loadLibrary as loadBankLibrary,
  saveLibrary as saveBankLibrary,
  loadActiveBankId,
  saveActiveBankId,
  detectBankModel,
  describeModel,
  searchBank as searchBankInLibrary,
  occupiedCount,
  suggestBankName,
  emptyPresets
} from '../contracts/bankLibrary.js';
import {
  buildBankMenu,
  movePreset as movePresetInBank,
  deletePreset as deletePresetInBank,
  renamePreset as renamePresetInBank,
  clampMoveToPosition
} from '../contracts/bankManager.js';

export function createBankManager(deps) {
  const getEngine = deps.getAudioEngine;
  const audioEngine = () => getEngine(); // section uses `audioEngine.` / `!audioEngine`
  const flashLcd = (...args) => deps.getFlashLcd()(...args);
  const openNameEditor = (...args) => deps.getOpenNameEditor()(...args);
  const selectPresetSlot = deps.selectPresetSlot;
  const lcdLine1 = deps.lcdLine1;
  const lcdLine2 = deps.lcdLine2;
  const makeDefaultBankJson = deps.makeDefaultBankJson;
  const loadPresetSlot = deps.loadPresetSlot;

// ─── Enhanced Bank Manager (ABDEEP-style multi-bank library) ───
// Every bank is tagged with its synth model — fixing the "sysex de un modelo u
// otro" mess where CZ-101 (16-patch), CZ-5000 (32-patch), CZ-230S and CZ Pack 1
// data all landed unlabelled in one 64-slot bank. The library is persisted in
// localStorage; the engine stays the source of truth for sound data and reports
// preset names back after every load.
const bankManagerModal = document.getElementById('bank-manager-modal');
const bankSlotList = document.getElementById('bank-slot-list');
const btnBankClose = document.getElementById('btn-bank-close');
const bankContextMenu = document.getElementById('bank-context-menu');
const bankSelect = document.getElementById('bank-select');
const bankModelFilter = document.getElementById('bank-model-filter');
const bankModelBadge = document.getElementById('bank-model-badge');
const bankSearchInput = document.getElementById('bank-search-input');
const bankSearchClear = document.getElementById('bank-search-clear');
const bankChipList = document.getElementById('bank-chip-list');
const bankMgrStatus = document.getElementById('bank-mgr-status');
const btnBankNew = document.getElementById('btn-bank-new');
const btnBankRename = document.getElementById('btn-bank-rename');
const btnBankDelete = document.getElementById('btn-bank-delete');
const btnBankImport = document.getElementById('btn-bank-import');
const btnBankExport = document.getElementById('btn-bank-export');
const inputBankImport = document.getElementById('input-bank-import');

let bankMenuRow = 0;
let bankSearchTerm = '';
let bankModelFilterValue = '';
let bankLib = null;
let pendingSyxImport = null; // { name } while a dropped .syx bank parses in the engine

// ── Library bootstrap ──
// The default library ships read-only factory banks: the built-in CZ-101
// factory presets plus the bundled CZ-230S banks and CZ Pack 1 patches (their
// preset names are filled in lazily once the bank is loaded into the engine).
// All of these stay reachable through the Bank Manager; the old header
// dropdowns (Factory Presets / CZ-230S Banks / CZ Pack 1 Banks) were removed.
// Stable ids for the read-only factory banks so the persisted activeBankId
// keeps matching across restarts (genId() would mint a NEW id on every boot
// and the manager would always fall back to Factory CZ-101).
const FACTORY_BANK_IDS = {
  cz101: 'factory_cz101',
  czpack1: 'factory_czpack1',
  cz230s: (file) => `factory_cz230s_${file.replace('.syx', '')}`
};

const buildDefaultLibrary = () => {
  const cz101Names = [
    "BRASS ENS", "TRUMPET", "VIOLIN", "STRING ENS",
    "ELEC PIANO", "SYNTH STRINGS", "SYNTH CHIMES", "GUITAR",
    "SYNTH BASS", "ELEC BASS", "FUNKY CLAVI", "VIBRAPHONE",
    "ORGAN", "BELLS", "STEEL DRUM", "PERCUSSION"
  ];
  
  const banks = [createBank({
    id: FACTORY_BANK_IDS.cz101,
    name: 'Factory CZ-101',
    model: 'cz101',
    readOnly: true,
    source: 'factory',
    presets: cz101Names.map(name => ({ name }))
  })];

  // CZ-230S official factory banks, one bank per .syx (the CZ-230S factory
  // index gives every slot its REAL name: Brass Ens. 1…Sweep).
  Object.keys(CZ230S_BANK_NAMES).forEach(file => {
    const src = `presets/cz230s/${file}`;
    const names = factoryBankNames(src);
    banks.push(createBank({
      id: FACTORY_BANK_IDS.cz230s(file),
      name: `CZ-230S Bank ${file.replace('CZ230S-', '').replace('.syx', '')}`,
      model: 'cz230s',
      readOnly: true,
      source: src,
      // The CZ-230S factory index gives every slot its REAL name (Brass
      // Ens. 1…Sweep), so these banks are never "Init N" placeholders.
      presets: names ? names.map(n => ({ name: n })) : undefined
    }));
  });

  // CZ Pack 1 is grouped into ONE bank (CZPack1-All.syx holds all 20 patches);
  // the 20 individual .syx stay on disk but the bank manager shows the full
  // bank with the real patch names (mapped 1:1 to the All file's order).
  const pack1Src = 'presets/czpack1/CZPack1-All.syx';
  banks.push(createBank({
    id: FACTORY_BANK_IDS.czpack1,
    name: 'CZ Pack 1',
    model: 'czpack1',
    readOnly: true,
    source: pack1Src,
    presets: factoryBankNames(pack1Src).map(n => ({ name: n }))
  }));

  // VirtualCZ User converted Bank A
  const virtualCzSrc = 'presets/virtualcz/Bank_A_OL.syx';
  banks.push(createBank({
    id: 'factory_virtualcz_banka',
    name: 'VirtualCZ Bank A',
    model: 'cz101', // Treated as CZ101 format
    readOnly: true,
    source: virtualCzSrc,
    presets: factoryBankNames(virtualCzSrc).map(n => ({ name: n }))
  }));

  // VirtualCZ User converted Bank B
  const virtualCzSrcB = 'presets/virtualcz/Bank_B_DS.syx';
  banks.push(createBank({
    id: 'factory_virtualcz_bankb',
    name: 'VirtualCZ Bank B',
    model: 'cz101', // Treated as CZ101 format
    readOnly: true,
    source: virtualCzSrcB,
    presets: factoryBankNames(virtualCzSrcB).map(n => ({ name: n }))
  }));

  // VirtualCZ User converted Bank C
  const virtualCzSrcC = 'presets/virtualcz/Bank_C_DS.syx';
  banks.push(createBank({
    id: 'factory_virtualcz_bankc',
    name: 'VirtualCZ Bank C',
    model: 'cz101', // Treated as CZ101 format
    readOnly: true,
    source: virtualCzSrcC,
    presets: factoryBankNames(virtualCzSrcC).map(n => ({ name: n }))
  }));

  // VirtualCZ User converted Bank D
  const virtualCzSrcD = 'presets/virtualcz/Bank_D_AC.syx';
  banks.push(createBank({
    id: 'factory_virtualcz_bankd',
    name: 'VirtualCZ Bank D (AC)',
    model: 'cz101', // Treated as CZ101 format
    readOnly: true,
    source: virtualCzSrcD,
    presets: factoryBankNames(virtualCzSrcD).map(n => ({ name: n }))
  }));

  // VirtualCZ User converted Bank E
  const virtualCzSrcE = 'presets/virtualcz/Bank_E_DS.syx';
  banks.push(createBank({
    id: 'factory_virtualcz_banke',
    name: 'VirtualCZ Bank E (DS)',
    model: 'cz101', // Treated as CZ101 format
    readOnly: true,
    source: virtualCzSrcE,
    presets: factoryBankNames(virtualCzSrcE).map(n => ({ name: n }))
  }));

  // CZ.Pats Bank 1
  const czPatsSrc1 = 'presets/virtualcz/Bank_CZPats_1.syx';
  banks.push(createBank({
    id: 'factory_czpats_1',
    name: 'CZ Pats 1',
    model: 'cz101',
    readOnly: true,
    source: czPatsSrc1,
    presets: factoryBankNames(czPatsSrc1).map(n => ({ name: n }))
  }));

  // CZ.Pats Bank 2
  const czPatsSrc2 = 'presets/virtualcz/Bank_CZPats_2.syx';
  banks.push(createBank({
    id: 'factory_czpats_2',
    name: 'CZ Pats 2',
    model: 'cz101',
    readOnly: true,
    source: czPatsSrc2,
    presets: factoryBankNames(czPatsSrc2).map(n => ({ name: n }))
  }));

  // CZ.Pats Bank 3
  const czPatsSrc3 = 'presets/virtualcz/Bank_CZPats_3.syx';
  banks.push(createBank({
    id: 'factory_czpats_3',
    name: 'CZ Pats 3',
    model: 'cz101',
    readOnly: true,
    source: czPatsSrc3,
    presets: factoryBankNames(czPatsSrc3).map(n => ({ name: n }))
  }));

  // Magazine Patches
  const magazineSrc = 'presets/virtualcz/Bank_Magazine.syx';
  banks.push(createBank({
    id: 'factory_magazine',
    name: 'Magazine Patches',
    model: 'cz101',
    readOnly: true,
    source: magazineSrc,
    presets: factoryBankNames(magazineSrc).map(n => ({ name: n }))
  }));

  // RC-20 Cartridge
  const rc20Src = 'presets/virtualcz/Bank_RC20.syx';
  banks.push(createBank({
    id: 'factory_rc20',
    name: 'Casio RC-20 Cartridge',
    model: 'cz101', // May contain CZ-1 extra velocity bytes, engine will parse as CZ-101
    readOnly: true,
    source: rc20Src,
    presets: factoryBankNames(rc20Src).map(n => ({ name: n }))
  }));

  // RC-30 Cartridge
  const rc30Src = 'presets/virtualcz/Bank_RC30.syx';
  banks.push(createBank({
    id: 'factory_rc30',
    name: 'Casio RC-30 Cartridge',
    model: 'cz101', // May contain CZ-1 extra velocity bytes, engine will parse as CZ-101
    readOnly: true,
    source: rc30Src,
    presets: factoryBankNames(rc30Src).map(n => ({ name: n }))
  }));

  // BEW Patches
  const bewSrc = 'presets/virtualcz/Bank_BEW.syx';
  banks.push(createBank({
    id: 'factory_bew',
    name: 'BEW Patches',
    model: 'cz101',
    readOnly: true,
    source: bewSrc,
    presets: factoryBankNames(bewSrc).map(n => ({ name: n }))
  }));

  // CZ-5000 Factory Banks
  addCZ5000Banks(banks, createBank);

  return { activeBankId: null, banks };
};

const persistBankLibrary = () => {
  if (!bankLib) return;
  saveBankLibrary(bankLib);
  if (bankLib.activeBankId) saveActiveBankId(bankLib.activeBankId);
};

// Resolves the persisted active bank to a bank in the freshly built library.
// Factory banks now use stable ids (factory_cz101, factory_cz230s_*, ...) but
// older persisted libraries saved RANDOM genId() ids — in that case fall back
// to matching the stored bank by its .syx source, then by name.
const resolveActiveBankId = (bankLib, stored, savedActive) => {
  const candidates = [savedActive, stored.activeBankId];
  const byId = candidates.find(id => id && findBank(bankLib, id));
  if (byId) return byId;
  // Migration: the stored id was minted by genId() before the stable-id fix.
  // Look up the bank it pointed to in the OLD library and match it to a default
  // factory bank by source (.syx path), falling back to the display name.
  const oldId = candidates.find(id => id && stored.banks.some(b => b.id === id));
  const oldBank = oldId ? stored.banks.find(b => b.id === oldId) : null;
  if (oldBank) {
    const bySource = oldBank.source && (bankLib.banks || []).find(b => b.source === oldBank.source);
    if (bySource) return bySource.id;
    const byName = (bankLib.banks || []).find(b => b.name === oldBank.name);
    if (byName) return byName.id;
  }
  return null;
};

const initBankLibrary = () => {
  const stored = loadBankLibrary();
  const savedActive = loadActiveBankId();
  const defaults = buildDefaultLibrary();
  if (stored && Array.isArray(stored.banks) && stored.banks.length) {
    // Migration: factory banks always come from the current defaults (they carry
    // the .syx source needed for warm-up — older persisted libraries dropped
    // `source` and had one bank per CZ Pack 1 patch). Only user banks persist.
    const userBanks = stored.banks.filter(b => !b.readOnly);
    bankLib = { ...stored, banks: [...defaults.banks, ...userBanks] };
    // Restore the last visited bank (stable ids, with migration for old ids).
    bankLib.activeBankId = resolveActiveBankId(bankLib, stored, savedActive) || defaults.banks[0].id;
  } else {
    bankLib = defaults;
  }
  if (!bankLib.activeBankId && bankLib.banks.length) bankLib.activeBankId = bankLib.banks[0].id;
  persistBankLibrary();
};
initBankLibrary();

const activeBank = () => getActiveBank(bankLib);

// ── Factory bank warm-up ──
// The bundled .syx files carry no patch names (the CZ-101 format stores them in
// the hardware, not the dump), so the engine labels every patch "Imported
// Preset". Instead of leaving the factory banks as 64 "Init N" slots (which
// made the manager look full of empty banks), parse each .syx once with the
// real WASM (no audio needed), keep the actual patch count and name the slots
// "Patch NN". The result is persisted so it only happens once per bank.
const warmFactoryBankNames = async () => {
  const targets = (bankLib.banks || []).filter(b => b.readOnly && b.source && b.source !== 'factory');
  let changed = false;
  for (const bank of targets) {
    // Skip banks already warmed (names not Init-prefixed)
    if (occupiedCount(bank) > 0) continue;
    try {
      const res = await fetch(bank.source);
      if (!res.ok) continue;
      const bytes = new Uint8Array(await res.arrayBuffer());
      const { count, names } = await parseSyxFile(bytes);
      if (count > 0) {
        // The CZ-101 format has no names; label each slot Patch NN (the bank
        // name is already shown by the selector + model badge).
        const pretty = humanizePatchNames(names.slice(0, count), 'Patch');
        bankLib = setBankPresets(bankLib, bank.id, pretty);
        changed = true;
      }
    } catch (err) {
      console.warn('Factory bank warm-up failed for', bank.name, err);
    }
  }
  if (changed) {
    persistBankLibrary();
    if (!bankManagerModal.hidden) renderBankManager();
    // The header preset dropdown should pick up the warmed names too (it only
    // shows the ACTIVE bank; at startup that is the factory bank with real
    // names, later banks get their Patch NN labels here).
    syncHeaderPresetNames();
  }
};
// warmFactoryBankNames(); // Disabled to prevent 404 flooding on startup

const downloadBankJson = (jsonStr, name) => {
  const blob = new Blob([jsonStr], { type: 'application/json' });
  const url = URL.createObjectURL(blob);
  const a = document.createElement('a');
  a.href = url;
  a.download = `${(name || 'cz101_bank').replace(/[^a-zA-Z0-9 _-]/g, '_')}.json`;
  a.click();
  URL.revokeObjectURL(url);
};

// ── Rendering ──
const updateBankModelBadge = () => {
  const b = activeBank();
  if (bankModelBadge) {
    bankModelBadge.textContent = b ? describeModel(b.model) : '';
    bankModelBadge.title = b && b.readOnly ? 'Read-only factory bank' : 'User bank';
  }
};

const renderBankSelect = () => {
  if (!bankSelect) return;
  bankSelect.innerHTML = '';
  const model = bankModelFilterValue;
  (bankLib.banks || [])
    .filter(b => !model || b.model === model)
    .forEach(b => {
      const opt = new Option(`${b.readOnly ? '🔒 ' : ''}${b.name} (${describeModel(b.model)})`, b.id);
      if (b.id === bankLib.activeBankId) opt.selected = true;
      bankSelect.appendChild(opt);
    });
};

// Model filter: restricts the bank selector to one synth model. When the filter
// hides the active bank, switch to the first bank of the filtered model so the
// slot grid always matches what the selector shows.
const applyBankModelFilter = () => {
  if (!bankSelect) return;
  renderBankSelect();
  const active = activeBank();
  if (bankModelFilterValue && active && active.model !== bankModelFilterValue) {
    const firstMatch = (bankLib.banks || []).find(b => b.model === bankModelFilterValue);
    if (firstMatch) {
      activateBank(firstMatch.id);
      return;
    }
  }
  refreshBankSlotList();
  updateBankModelBadge();
  syncHeaderPresetNames();
};

if (bankModelFilter) {
  // Populate the model filter with the known models (ALL MODELS already present).
  Object.keys(BANK_MODELS).forEach(key => {
    bankModelFilter.appendChild(new Option(describeModel(key), key));
  });
  bankModelFilter.addEventListener('change', (e) => {
    bankModelFilterValue = e.target.value;
    applyBankModelFilter();
  });
}

const renderBankStatus = () => {
  if (!bankMgrStatus) return;
  const b = activeBank();
  if (!b) { bankMgrStatus.textContent = ''; return; }
  bankMgrStatus.textContent =
    `${occupiedCount(b)}/64 occupied · ${b.readOnly ? 'factory' : 'user'} bank` +
    (bankModelFilterValue ? ` · model: ${describeModel(bankModelFilterValue)}` : '') +
    (bankSearchTerm ? ` · filtered` : '');
};

// Renders the slot grid from the ACTIVE LIBRARY BANK (never empty: it shows the
// factory bank names even before the audio engine starts), filtered by search.
const refreshBankSlotList = () => {
  if (!bankSlotList) return;
  bankSlotList.innerHTML = '';
  const b = activeBank();
  if (!b) return;
  const visible = searchBankInLibrary(b, bankSearchTerm);
  const activeSlot = parseInt(selectPresetSlot.value);
  visible.forEach(i => {
    const p = b.presets[i] || { name: `Slot ${i + 1}` };
    const row = document.createElement('div');
    row.className = 'bank-slot-row';
    if (i === activeSlot) row.classList.add('selected');
    if ((p.name || '').startsWith('Init')) row.classList.add('empty');
    const num = document.createElement('span');
    num.className = 'bank-slot-num';
    num.textContent = String(i + 1).padStart(2, '0');
    const name = document.createElement('span');
    name.className = 'bank-slot-name';
    name.textContent = p.name || `Slot ${i + 1}`;
    row.appendChild(num);
    row.appendChild(name);
    row.addEventListener('contextmenu', (ev) => {
      ev.preventDefault();
      bankMenuRow = i;
      showBankContextMenu(ev.clientX, ev.clientY);
    });
    row.addEventListener('click', () => {
      loadPresetSlot(i);
      refreshBankSlotList();
      syncHeaderPresetNames();
    });
    row.addEventListener('dblclick', () => {
      loadPresetSlot(i);
      syncHeaderPresetNames();
      if (bankManagerModal) bankManagerModal.hidden = true;
    });
    bankSlotList.appendChild(row);
  });
  renderBankStatus();
};

// Per-bank chips: coloured model tag + name + occupied-patch count. Clicking a
// chip activates that bank (same as the selector). Respects the model filter.
const renderBankChips = () => {
  if (!bankChipList) return;
  bankChipList.innerHTML = '';
  const model = bankModelFilterValue;
  (bankLib.banks || [])
    .filter(b => !model || b.model === model)
    .forEach(b => {
      const chip = document.createElement('button');
      chip.type = 'button';
      chip.className = 'bank-chip';
      if (b.id === bankLib.activeBankId) chip.classList.add('active');

      const modelTag = document.createElement('span');
      modelTag.className = `bank-chip-model bank-chip-model-${b.model === 'user' ? 'user' : 'factory'}`;
      modelTag.textContent = describeModel(b.model);

      const name = document.createElement('span');
      name.className = 'bank-chip-name';
      name.textContent = b.name;

      const occ = document.createElement('span');
      const n = occupiedCount(b);
      occ.className = `bank-chip-occ ${n > 0 ? 'occ-ok' : 'occ-empty'}`;
      occ.textContent = `${n}/${BANK_SLOTS_CONST}`;

      chip.appendChild(modelTag);
      chip.appendChild(name);
      chip.appendChild(occ);
      chip.addEventListener('click', () => {
        if (b.id !== bankLib.activeBankId) activateBank(b.id);
      });
      bankChipList.appendChild(chip);
    });
};

const renderBankManager = () => {
  renderBankSelect();
  renderBankChips();
  updateBankModelBadge();
  refreshBankSlotList();
  syncHeaderPresetNames();
};

// Keep the header preset dropdown (selectPresetSlot) in sync with the ACTIVE
// library bank even before the audio engine is running. The engine labels every
// imported CZ-101 patch "Imported Preset", so for factory banks we prefer the
// library's human names (Patch NN / real preset names).
const syncHeaderPresetNames = () => {
  const b = activeBank();
  if (!b || !selectPresetSlot) return;
  for (let i = 0; i < selectPresetSlot.options.length; i++) {
    const p = b.presets[i];
    const name = p ? (p.name || `Slot ${i + 1}`) : `Slot ${i + 1}`;
    selectPresetSlot.options[i].text = `Slot ${i + 1}: ${name}`;
  }
};
// Fill the header preset dropdown with the active library bank's names right at
// startup, so it never shows 64 "(Empty)" before the audio engine is running.
syncHeaderPresetNames();

// ── Engine loading ──
// Loads a bank into the engine. Factory banks are fetched from their bundled
// .syx; user banks restore their stored JSON snapshot. Names come back through
// onBankLoaded and are synced into the library (persisted).
const loadBankIntoEngine = (bank) => {
  if (!audioEngine() || !bank) return false;
  if (bank.source === 'factory') {
    audioEngine().loadFactoryBank(PARAMETER_REGISTRY.parameters.map(p => p.id));
    return true;
  }
  if (bank.snapshot) {
    audioEngine().loadBank(bank.snapshot, PARAMETER_REGISTRY.parameters.map(p => p.id));
    return true;
  }
  if (bank.source) {
    fetch(bank.source)
      .then(r => { if (!r.ok) throw new Error(`HTTP ${r.status}`); return r.arrayBuffer(); })
      .then(buf => {
        if (audioEngine()) audioEngine().loadSysEx(new Uint8Array(buf), PARAMETER_REGISTRY.parameters.map(p => p.id));
      })
      .catch(err => {
        console.error('Failed to load factory bank:', err);
        lcdLine1.innerText = 'BANK LOAD FAILED';
      });
    return true;
  }
  // User bank without a snapshot yet: start from the factory defaults.
  audioEngine().loadFactoryBank(PARAMETER_REGISTRY.parameters.map(p => p.id));
  return true;
};

const activateBank = (id) => {
  if (!findBank(bankLib, id)) return;
  bankLib = setActiveBankId(bankLib, id);
  persistBankLibrary();
  renderBankManager();
  loadBankIntoEngine(activeBank());
};

// Names came back from the engine (bank load / sysex bank / rename/delete/move).
// Sync them into the active library bank, capture a snapshot for user banks and
// refresh the grid. A pending .syx import turns into a new user bank here.
const syncBankNamesToLibrary = (names) => {
  if (!bankLib || !bankLib.activeBankId) return;
  const b = getActiveBank(bankLib);
  if (!b) return;

  if (pendingSyxImport) {
    const model = detectBankModel(names.filter(n => !n.startsWith('Init')).length || 1, 'user');
    const bank = createBank({ name: pendingSyxImport.name, model, readOnly: false });
    bank.snapshot = null;
    bankLib = addBank(bankLib, bank);
    bankLib = setBankPresets(bankLib, bank.id, names);
    bankLib = setActiveBankId(bankLib, bank.id);
    pendingSyxImport = null;
    if (audioEngine() && typeof audioEngine().captureBankJson === 'function') {
      audioEngine().captureBankJson((jsonStr) => {
        bankLib = setBankSnapshot(bankLib, bank.id, jsonStr);
        persistBankLibrary();
      });
    }
  } else if (b.readOnly && occupiedCount(b) > 0) {
    // Factory bank already warmed with "Patch NN" labels: keep them. The engine
    // reports "Imported Preset" for every CZ-101 patch (the format stores no
    // names) which would wipe the human labels we warmed in.
    renderBankManager();
    return;
  } else {
    bankLib = setBankPresets(bankLib, b.id, names);
    if (!b.readOnly && audioEngine() && typeof audioEngine().captureBankJson === 'function') {
      audioEngine().captureBankJson((jsonStr) => {
        bankLib = setBankSnapshot(bankLib, b.id, jsonStr);
        persistBankLibrary();
      });
    }
  }
  persistBankLibrary();
  renderBankManager();
};

// ── Bank CRUD ──
if (btnBankNew) {
  btnBankNew.addEventListener('click', () => {
    const name = suggestBankName(bankLib);
    const bank = createBank({ name, model: 'user', readOnly: false });
    bankLib = addBank(bankLib, bank);
    persistBankLibrary();
    activateBank(bank.id);
    openNameEditor({
      title: 'RENAME BANK',
      value: name,
      onSave: (newName) => {
        if (newName.trim()) {
          bankLib = renameBank(bankLib, bank.id, newName.trim());
          persistBankLibrary();
          renderBankManager();
        }
      }
    });
  });
}

if (btnBankRename) {
  btnBankRename.addEventListener('click', () => {
    const b = activeBank();
    if (!b) return;
    if (b.readOnly) { flashLcd('READ ONLY', 'FACTORY BANK'); return; }
    openNameEditor({
      title: 'RENAME BANK',
      value: b.name,
      onSave: (newName) => {
        if (newName.trim()) {
          bankLib = renameBank(bankLib, b.id, newName.trim());
          persistBankLibrary();
          renderBankManager();
        }
      }
    });
  });
}

if (btnBankDelete) {
  btnBankDelete.addEventListener('click', () => {
    const b = activeBank();
    if (!b) return;
    if (b.readOnly) { flashLcd('READ ONLY', 'FACTORY BANK'); return; }
    if (window.confirm(`Delete bank "${b.name}"?`)) {
      bankLib = removeBank(bankLib, b.id);
      persistBankLibrary();
      renderBankManager();
      loadBankIntoEngine(activeBank());
    }
  });
}

// ── Import / export ──
if (btnBankImport) {
  btnBankImport.addEventListener('click', () => inputBankImport && inputBankImport.click());
}

if (inputBankImport) {
  inputBankImport.addEventListener('change', (e) => {
    const file = e.target.files[0];
    if (!file) return;
    const isSyx = /\.(syx|sysex)$/i.test(file.name);
    const reader = new FileReader();
    reader.onload = (event) => {
      if (isSyx) {
        // Load into the engine to parse the patch names, then create a user bank
        // tagged with the detected model (syncBankNamesToLibrary does it).
        if (!audioEngine()) { flashLcd('INIT AUDIO', 'FIRST'); return; }
        pendingSyxImport = { name: file.name.replace(/\.(syx|sysex)$/i, '') };
        lcdLine1.innerText = 'PARSING SYSEX...';
        lcdLine2.innerText = file.name;
        audioEngine().loadSysEx(new Uint8Array(event.target.result), PARAMETER_REGISTRY.parameters.map(p => p.id));
      } else {
        const jsonStr = event.target.result;
        let parsed;
        try { parsed = JSON.parse(jsonStr); } catch (err) { flashLcd('BAD JSON', 'INVALID BANK'); return; }
        const presets = Array.isArray(parsed) ? parsed : (parsed && parsed.presets) || [];
        const bank = createBank({
          name: file.name.replace(/\.json$/i, '') || 'Imported Bank',
          model: detectBankModel(presets.length),
          readOnly: false,
          presets: presets.map(p => ({ name: (p && p.name) || 'Init' }))
        });
        bank.snapshot = jsonStr;
        bankLib = addBank(bankLib, bank);
        persistBankLibrary();
        activateBank(bank.id);
      }
    };
    reader[isSyx ? 'readAsArrayBuffer' : 'readAsText'](file);
    e.target.value = '';
  });
}

if (btnBankExport) {
  btnBankExport.addEventListener('click', () => {
    const b = activeBank();
    if (!b) return;
    if (!audioEngine()) { flashLcd('INIT AUDIO', 'FIRST'); return; }
    // Re-sync the snapshot from the engine (in case it was edited), then download.
    audioEngine().captureBankJson((jsonStr) => {
      bankLib = setBankSnapshot(bankLib, b.id, jsonStr);
      persistBankLibrary();
      downloadBankJson(jsonStr, b.name);
      flashLcd('BANK EXPORTED', b.name);
    });
  });
}

// ── Bank selector + search ──
if (bankSelect) {
  bankSelect.addEventListener('change', (e) => {
    if (e.target.value) activateBank(e.target.value);
  });
}

if (bankSearchInput) {
  bankSearchInput.addEventListener('input', (e) => {
    bankSearchTerm = e.target.value;
    refreshBankSlotList();
  });
}

if (bankSearchClear) {
  bankSearchClear.addEventListener('click', () => {
    bankSearchTerm = '';
    if (bankSearchInput) bankSearchInput.value = '';
    refreshBankSlotList();
  });
}

const closeBankContextMenu = () => {
  if (bankContextMenu) bankContextMenu.hidden = true;
};

const showBankContextMenu = (x, y) => {
  if (!bankContextMenu) return;
  const count = BANK_SLOTS_CONST;
  const items = buildBankMenu(bankMenuRow, count);
  bankContextMenu.innerHTML = '';
  items.forEach(item => {
    if (item.separator) {
      const sep = document.createElement('div');
      sep.className = 'bank-context-separator';
      bankContextMenu.appendChild(sep);
      return;
    }
    const el = document.createElement('div');
    el.className = 'bank-context-item' + (item.disabled ? ' disabled' : '');
    el.textContent = item.label;
    el.addEventListener('click', () => {
      closeBankContextMenu();
      handleBankMenuAction(item.id);
    });
    bankContextMenu.appendChild(el);
  });
  bankContextMenu.hidden = false;
  const rect = bankContextMenu.getBoundingClientRect();
  bankContextMenu.style.left = Math.min(x, window.innerWidth - rect.width - 8) + 'px';
  bankContextMenu.style.top = Math.min(y, window.innerHeight - rect.height - 8) + 'px';
};

// Native PopupMenu ids: 1=Move Up, 2=Move Down, 4=Move to Position...,
// 5=Rename..., 3=Delete.
const handleBankMenuAction = (actionId) => {
  const row = bankMenuRow;
  const count = BANK_SLOTS_CONST;
  const b = activeBank();
  const preset = b && b.presets[row] ? b.presets[row] : { name: `Slot ${row + 1}` };
  if (actionId === 1) { // Move Up
    if (row > 0 && audioEngine) audioEngine().movePreset(row, row - 1);
  } else if (actionId === 2) { // Move Down
    if (row < count - 1 && audioEngine) audioEngine().movePreset(row, row + 1);
  } else if (actionId === 4) { // Move to Position...
    openNameEditor({
      title: 'MOVE PRESET',
      value: String(row + 1),
      onSave: (target) => {
        const newPos = clampMoveToPosition(target, count);
        if (newPos !== null && audioEngine) audioEngine().movePreset(row, newPos);
      }
    });
  } else if (actionId === 5) { // Rename...
    openNameEditor({
      title: 'RENAME PRESET',
      value: preset.name,
      onSave: (newName) => {
        if (newName.trim() && audioEngine()) {
          audioEngine().renamePreset(row, newName.trim());
        }
      }
    });
  } else if (actionId === 3) { // Delete
    if (audioEngine()) audioEngine().deletePreset(row);
  }
};

// ── Modal close + context-menu dismissal (module-owned DOM) ──
if (btnBankClose) {
  btnBankClose.addEventListener('click', () => { bankManagerModal.hidden = true; });
}

document.addEventListener('click', (ev) => {
  if (bankContextMenu && !bankContextMenu.hidden && !bankContextMenu.contains(ev.target)) {
    closeBankContextMenu();
  }
});

document.addEventListener('contextmenu', (ev) => {
  // Close a stale menu when right-clicking elsewhere. The row handler already
  // preventDefault()s for bank rows, so the browser menu stays suppressed there;
  // here we only close (and never re-open) when clicking outside the list.
  const inBankList = ev.target && ev.target.closest && ev.target.closest('#bank-slot-list');
  if (bankContextMenu && !bankContextMenu.hidden && !bankContextMenu.contains(ev.target) && !inBankList) {
    closeBankContextMenu();
  }
});

  return {
    getBankLib: () => bankLib,
    activeBank,
    activateBank,
    loadBankIntoEngine,
    syncBankNamesToLibrary,
    renderBankManager,
    getBankManagerModal: () => bankManagerModal
  };
}
