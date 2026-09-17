import { describe, it, expect } from 'vitest';
import fs from 'fs';
import path from 'path';

describe('BNK Bank Browse mode (LCD bank/patch browser)', () => {
  const appJs = fs.readFileSync(path.resolve(__dirname, '../src/app.js'), 'utf8');
  // The LCD panel internals (keypad state machine, scroller, badge) live in the
  // extracted LCD modules (core + scroller + the BNK/MDL/Write/Setup mode
  // factories); app.js wires the factory and the engine callbacks.
  const lcdJs = [
    '../src/ui/lcdPanel.js',
    '../src/ui/lcdScroller.js',
    '../src/ui/lcdBnkMode.js',
    '../src/ui/lcdMdlMode.js',
    '../src/ui/lcdWriteMode.js',
    '../src/ui/lcdSetupMode.js'
  ].map(p => fs.readFileSync(path.resolve(__dirname, p), 'utf8')).join('\n');
  const html = fs.readFileSync(path.resolve(__dirname, '../index.html'), 'utf8');
  const css = ['lcd.css', 'keyboard.css'].map(f => fs.readFileSync(path.resolve(__dirname, '../src/styles', f), 'utf8')).join('\n');

  it('index.html places the BNK button above SET in the keypad grid', () => {
    // Row 1 column 1 of the 3x2 keypad is directly above SET (row 2 col 1).
    const row1 = html.slice(html.indexOf('Row 1'), html.indexOf('Row 2'));
    expect(row1).toContain('id="btn-lcd-bnk"');
    expect(row1).toContain('btn-lcd-bnk');
    const setIdx = html.indexOf('id="btn-lcd-set"');
    const bnkIdx = html.indexOf('id="btn-lcd-bnk"');
    expect(bnkIdx).toBeGreaterThan(-1);
    expect(setIdx).toBeGreaterThan(bnkIdx); // BNK markup precedes SET markup
  });

  it('BNK button gets the amber accent + pressed (active) styling', () => {
    expect(css).toMatch(/\.lcd-btn-bnk \{[^}]*#e8a33d/);
    expect(css).toMatch(/\.lcd-btn-bnk\.active \{[^}]*background: #e8a33d/);
  });

  it('implements bank + patch navigation wired to the keypad', () => {
    // ▲/▼ cycle banks; ◀/▶ cycle patches; both wrap.
    expect(lcdJs).toContain('cycleBank(isUp ? 1 : -1)');
    expect(lcdJs).toContain('cyclePatch(-1)');
    expect(lcdJs).toContain('cyclePatch(1)');
    // Navigation actually loads the selection (bank via activateBank, patch via
    // loadPresetSlot) — connected to the engine like the header select.
    expect(lcdJs).toContain('activateBank(nextBank.id);');
    expect(lcdJs).toContain('loadPresetSlot((cur + dir + total) % total);');
    // Per-bank remembered slot resolution (same rule as onBankLoaded).
    expect(lcdJs).toContain("saved.bankId === nextBank.id && saved.slot >= 0 && saved.slot < BANK_SLOTS_CONST");
  });

  it('LCD shows bank name on line 1 and patch name on line 2 (library names)', () => {
    expect(lcdJs).toContain('bnkLine1.set(bankName);');
    expect(lcdJs).toContain('bnkLine2.set(patchName);');
    // Names come from the library immediately (no engine round-trip).
    expect(lcdJs).toContain('const b = activeBank();');
    expect(lcdJs).toContain('b.presets[idx].name');
  });

  it('overlong names scroll character-by-character with a pause and reverse', () => {
    // LCD-style: static for ~2s, then one char per tick.
    expect(lcdJs).toContain('const LCD_SCROLL_DELAY_MS = 2000');
    expect(lcdJs).toContain('const LCD_SCROLL_STEP_MS = 180');
    // Ping-pong: reverses when the full name has been shown.
    expect(lcdJs).toContain('if (offset >= maxOffset) { offset = maxOffset; dir = -1; }');
    expect(lcdJs).toContain('else if (offset <= 0) { offset = 0; dir = 1; }');
    // Text that fits is shown statically (no scroll).
    expect(lcdJs).toContain('if (text.length <= fit) {');
  });

  it('shows a position indicator (bank N/total + patch slot) in the LCD corner', () => {
    // Badge lives in the LCD display, hidden until BNK mode, with two rows:
    // the bank MODEL on top and the position below.
    expect(html).toMatch(/<span class="lcd-bnk-badge" id="lcd-bnk-badge" style="display: none;">/);
    expect(html).toContain('id="lcd-bnk-model"');
    expect(html).toContain('id="lcd-bnk-pos"');
    // Position row content: "B3/12 · P07/24" — bank index/total, patch slot and
    // the number of OCCUPIED patches in the bank (non-"Init" slots).
    expect(lcdJs).toContain('posEl.innerHTML = `B${bankPos}/${bankTotal} · ${slotHtml}<span class="lcd-occ ${occClass}">/${occupied}</span>`;');
    expect(lcdJs).toContain('const occupied = bank ? occupiedCount(bank) : 0;');
    expect(lcdJs).toContain('let slotHtml = `P${String(slot).padStart(2, \'0\')}`;');
    // Model row shows the bank's model tag (CZ-101 / CZ-5000 / CZ-230S / Pack / User).
    expect(lcdJs).toContain('modelEl.textContent = bank ? describeModel(bank.model) : \'\';');
    // Model tag colour: USER green, factory gray; occupancy green/amber.
    expect(lcdJs).toContain("modelEl.classList.toggle('lcd-bnk-model-user', !!bank && bank.model === 'user');");
    expect(lcdJs).toContain("modelEl.classList.toggle('lcd-bnk-model-factory', !!bank && bank.model !== 'user');");
    expect(lcdJs).toContain("const occClass = occupied > 0 ? 'lcd-occ-ok' : 'lcd-occ-empty';");
    expect(css).toMatch(/\.lcd-bnk-badge \.lcd-bnk-model-user \{[^}]*#4ade80/);
    expect(css).toMatch(/\.lcd-bnk-badge \.lcd-bnk-model-factory \{[^}]*#9ca3af/);
    expect(css).toMatch(/\.lcd-bnk-badge \.lcd-occ-ok \{[^}]*#4ade80/);
    expect(css).toMatch(/\.lcd-bnk-badge \.lcd-occ-empty \{[^}]*#fbbf24/);
    // Refresh happens on every BNK render (bank and patch navigation included).
    expect(lcdJs).toContain('renderBnkBadge();');
    expect(lcdJs).toContain('const renderBnkLcd = () => {');
    // Shown on enter, hidden on exit; line 1 reserves the corner so the name
    // never runs under the badge (measurement uses the content box).
    expect(lcdJs).toContain("display.classList.add('bnk-active')");
    expect(lcdJs).toContain("display.classList.remove('bnk-active')");
    expect(lcdJs).toContain("badge.style.display = ''");
    expect(lcdJs).toContain("badge.style.display = 'none'");
    expect(lcdJs).toContain('const contentW = Math.max(0, el.clientWidth - padL - padR);');
    // Line 1 reserves exactly what the (variable-width) badge occupies, and the
    // reserve is released on exit so the measurement stays correct.
    expect(lcdJs).toContain('lcdLine1.style.paddingRight = badge.classList.contains(\'corner-bottom-left\') ? \'\' : reserve;');
    expect(lcdJs).toContain('`${Math.max(40, badge.offsetWidth + 6)}px`');
    expect(lcdJs).toContain("lcdLine1.style.paddingRight = '';");
    expect(lcdJs).toContain("lcdLine2.style.paddingLeft = '';");
    expect(css).toMatch(/#lcd-display\.bnk-active \.lcd-line-1 \{[^}]*padding-right: 56px/);
    expect(css).toMatch(/\.lcd-bnk-badge \{[^}]*position: absolute/);
    expect(css).toMatch(/\.lcd-bnk-badge \{[^}]*flex-direction: column/);
  });

  it('Bank Manager shows per-bank chips (model tag + name + occupied count)', () => {
    // Chips container lives in the Bank Manager modal; the render logic moved
    // to the extracted src/ui/bankManager.js module (post-refactor).
    const bankJs = fs.readFileSync(path.resolve(__dirname, '../src/ui/bankManager.js'), 'utf8');
    expect(html).toContain('class="bank-chip-list" id="bank-chip-list"');
    expect(bankJs).toContain('const bankChipList = document.getElementById(\'bank-chip-list\');');
    expect(bankJs).toContain('const renderBankChips = () => {');
    // Each chip: coloured model tag (user green / factory gray) + occupied count.
    expect(bankJs).toContain("modelTag.className = `bank-chip-model bank-chip-model-${b.model === 'user' ? 'user' : 'factory'}`;");
    expect(bankJs).toContain("occ.className = `bank-chip-occ ${n > 0 ? 'occ-ok' : 'occ-empty'}`;");
    expect(bankJs).toContain('occ.textContent = `${n}/${BANK_SLOTS_CONST}`;');
    // Chips respect the model filter and activate banks on click.
    expect(bankJs).toContain('.filter(b => !model || b.model === model)');
    expect(bankJs).toContain("if (b.id !== bankLib.activeBankId) activateBank(b.id);");
    // renderBankManager paints the chips alongside the selector.
    expect(bankJs).toContain('const renderBankManager = () => {');
    expect(bankJs).toContain('renderBankSelect();');
    expect(bankJs).toContain('renderBankChips();');
    expect(css).toMatch(/\.bank-chip \{[^}]*cursor: pointer/);
    expect(css).toMatch(/\.bank-chip-model-user \{[^}]*#4ade80/);
    expect(css).toMatch(/\.bank-chip-model-factory \{[^}]*#9ca3af/);
  });

  it('shares the position badge with Write mode (bottom-left, overwrite warning)', () => {
    // Visibility helpers shared by all modes, corner-parameterized.
    expect(lcdJs).toContain("const showBnkBadge = (corner = 'top-right') => {");
    expect(lcdJs).toContain('const hideBnkBadge = () => {');
    // Write mode paints the badge with the selected destination slot in the
    // BOTTOM-LEFT corner (so it never collides with "WRITE: SELECT SLOT").
    expect(lcdJs).toContain("showBnkBadge('bottom-left');");
    expect(lcdJs).toContain('renderBnkBadge(ws.targetSlot + 1, { warnOccupiedSlot: true });');
    // Overwrite warning: an occupied target slot is highlighted (amber).
    expect(lcdJs).toContain("if (p && !(p.name || '').startsWith('Init')) {");
    expect(lcdJs).toContain('<span class="lcd-slot-occupied">${slotHtml}</span>');
    // The slot can be overridden (Write) or defaults to the header selection (BNK).
    expect(lcdJs).toContain('const slot = slotOverride !== undefined ? slotOverride : (parseInt(selectPresetSlot?.value || 0) + 1);');
    // Invariant: the badge only shows in BNK, Write or Compare; every other
    // readout hides it (covers the commit() path, which resets internally).
    expect(lcdJs).toContain('if (!lcdBnkMode && !writeManager.isActive && !lcdCompareMode) hideBnkBadge();');
    expect(lcdJs).toContain('hideBnkBadge();'); // confirmWriteSave hides after commit
    expect(lcdJs).toContain('if (!ok) {');
    // CSS: bottom-left corner variant + amber overwrite slot.
    expect(css).toMatch(/\.lcd-bnk-badge\.corner-bottom-left \{[^}]*bottom: 3px/);
    expect(css).toMatch(/\.lcd-bnk-badge \.lcd-slot-occupied \{[^}]*#fbbf24/);
  });

  it('shows the badge in Compare mode (the compared slot)', () => {
    // CompareManager exposes the active compared slot (0-based).
    const flow = fs.readFileSync(path.resolve(__dirname, '../src/contracts/patchFlow.js'), 'utf8');
    expect(flow).toContain('get slot() { return active ? active.slot : null; }');
    // The Compare LCD branch paints the badge with that slot (top-right, like BNK).
    expect(lcdJs).toContain('const cmpSlot = compareManager.slot;');
    expect(lcdJs).toContain('renderBnkBadge(cmpSlot + 1);');
    // The badge-visibility invariant also allows Compare mode.
    expect(lcdJs).toContain('if (!lcdBnkMode && !writeManager.isActive && !lcdCompareMode) hideBnkBadge();');
  });

  it('other LCD modes and inactivity leave Bank Browse mode', () => {
    // CMP/WRT/SET take over the LCD and unpress BNK.
    expect(lcdJs).toContain("if (lcdBnkModeActive()) exitBnkMode(); // Compare takes over the LCD");
    expect(lcdJs).toContain("if (lcdBnkModeActive()) exitBnkMode(); // Write takes over the LCD");
    expect(lcdJs).toContain("if (getLcdBnkMode()) exitBnkMode(); // Setup takes over the LCD (any entry path)");
    // Inactivity returns to Program Select mode (like SET/Write).
    expect(lcdJs).toContain('exitBnkMode();  // unpress BNK and leave Bank Browse mode');
    // Engine echoes don't fight the BNK readout (app.js callbacks).
    expect(appJs).toContain('if (getLcdBnkMode()) renderBnkLcd();');
  });
});
