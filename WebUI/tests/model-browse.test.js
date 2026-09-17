import { describe, it, expect } from 'vitest';
import fs from 'fs';
import path from 'path';

describe('MDL Model Browse mode + generic LCD scroller', () => {
  const appJs = fs.readFileSync(path.resolve(__dirname, '../src/app.js'), 'utf8');
  // The keypad state machine + scroller live in the extracted LCD modules
  // (core + generic scroller + the MDL mode factory).
  const lcdJs = [
    '../src/ui/lcdPanel.js',
    '../src/ui/lcdScroller.js',
    '../src/ui/lcdBnkMode.js',
    '../src/ui/lcdMdlMode.js',
    '../src/ui/lcdWriteMode.js',
    '../src/ui/lcdSetupMode.js'
  ].map(p => fs.readFileSync(path.resolve(__dirname, p), 'utf8')).join('\n');
  const html = fs.readFileSync(path.resolve(__dirname, '../index.html'), 'utf8');
  const css = fs.readFileSync(path.resolve(__dirname, '../src/styles/lcd.css'), 'utf8');

  it('index.html places the MDL button next to BNK in the keypad grid', () => {
    const row1 = html.slice(html.indexOf('Row 1'), html.indexOf('Row 2'));
    expect(row1).toContain('id="btn-lcd-mdl"');
    const bnkIdx = html.indexOf('id="btn-lcd-bnk"');
    const mdlIdx = html.indexOf('id="btn-lcd-mdl"');
    expect(bnkIdx).toBeGreaterThan(-1);
    expect(mdlIdx).toBeGreaterThan(bnkIdx); // MDL sits right after BNK
    expect(mdlIdx).toBeLessThan(html.indexOf('id="btn-lcd-val-up"'));
  });

  it('MDL button gets its own accent + pressed (active) styling', () => {
    expect(css).toMatch(/\.lcd-btn-mdl \{[^}]*#b388ff/);
    expect(css).toMatch(/\.lcd-btn-mdl\.active \{[^}]*background: #b388ff/);
  });

  it('the LCD scroll is generalized into a reusable scroller (createLcdScroller)', () => {
    // BNK, MDL and the normal readout all share the same generic scroller.
    expect(lcdJs).toContain('const createLcdScroller = (el) => ({');
    expect(lcdJs).toContain('const bnkLine1 = createLcdScroller(lcdLine1);');
    expect(lcdJs).toContain('const normalLine2 = createLcdScroller(lcdLine2);');
    expect(lcdJs).toContain('const mdlLine1 = createLcdScroller(lcdLine1);');
    // The old element+slot API is gone.
    expect(lcdJs).not.toContain('const setScrollingText = (el, slot, text) => {');
    // The scroller keeps the running scroll on unchanged text and stops cleanly.
    expect(lcdJs).toContain('if (this.text === text) return;');
    expect(lcdJs).toContain('set(text) {');
    expect(lcdJs).toContain('stop() {');
  });

  it('MDL navigates the four OPERATION_MODE models (up/right advance, down/left back)', () => {
    // The OPERATION_MODE registry choices: Classic 101 / Classic CZ-1 / Classic 5000 / Modern.
    expect(lcdJs).toContain("const MDL_NAMES = ['CLASSIC 101', 'CLASSIC CZ-1', 'CLASSIC 5000', 'MODERN'];");
    // ▲/▼: up advances (+1), down goes back (-1) — same wrap as banks.
    expect(lcdJs).toContain('cycleModel(isUp ? 1 : -1)');
    // ◀/▶: right advances (+1), left goes back (-1).
    expect(lcdJs).toContain('cycleModel(1); // next model');
    expect(lcdJs).toContain('cycleModel(-1); // previous model');
    // The change goes through the OPERATION_MODE control (engine + UI sync).
    expect(lcdJs).toContain("inputEl.dispatchEvent(new Event('input'));");
    expect(lcdJs).toContain("inputEl.dispatchEvent(new Event('change'));");
  });

  it('MDL readout shows the model name (scrolling) and the M<mode>/<total> position', () => {
    expect(lcdJs).toContain('mdlLine1.set(name);');
    expect(lcdJs).toContain('lcdLine2.innerText = `M${mode + 1}/${total}`;');
  });

  it('MDL is mutually exclusive with the other LCD modes (BNK/SET/Write/Compare)', () => {
    // Entering MDL leaves Bank Browse; entering BNK leaves MDL.
    expect(lcdJs).toContain('exitBnkMode(); // Browse modes are mutually exclusive');
    expect(lcdJs).toContain('exitMdlMode(); // Browse modes are mutually exclusive');
    // CMP/WRT/SET take over the LCD and unpress MDL; inactivity exits it too.
    expect(lcdJs).toContain('if (lcdMdlMode) exitMdlMode();');
    expect(lcdJs).toContain('exitMdlMode();  // unpress MDL and leave Model Browse mode');
    // The normal-readout guard also excludes MDL so its scroll never leaks.
    expect(lcdJs).toContain('!lcdBnkMode && !lcdMdlMode && !writeManager.isActive');
  });
});
