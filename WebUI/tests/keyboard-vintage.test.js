import { describe, it, expect } from 'vitest';
import fs from 'fs';
import path from 'path';

describe('Vintage keyboard (aged ivory + deterministic wear)', () => {
  const appJs = fs.readFileSync(path.resolve(__dirname, '../src/app.js'), 'utf8');
  const kbdJs = fs.readFileSync(path.resolve(__dirname, '../src/ui/keyboard.js'), 'utf8');
  const css = fs.readFileSync(path.resolve(__dirname, '../src/styles/keyboard.css'), 'utf8');

  it('keyboard module assigns deterministic wear classes per key (stable across reloads)', () => {
    expect(kbdJs).toContain('function keyWear(');
    // Deterministic: no Math.random at key build time.
    expect(kbdJs).toContain('(midiNote * 2654435761) % 100');
    // Applied to both generated keys and the top C.
    expect(kbdJs).toContain('key.classList.add(...keyWear(midiNote, n.black));');
    expect(kbdJs).toContain('topC.classList.add(...keyWear(topCNote));');
    // White-key marks…
    expect(kbdJs).toContain("classes.push('stain-yellow')");
    expect(kbdJs).toContain("classes.push('stain-scuff')");
    expect(kbdJs).toContain("classes.push('stain-ding')");
    // …and a separate mark for black keys.
    expect(kbdJs).toContain("classes.push('stain-worn')");
    // app.js wires the module with the live engine + LCD + MIDI activity refs.
    expect(appJs).toContain("import { createKeyboard } from './ui/keyboard.js';");
    expect(appJs).toContain('createKeyboard({');
    expect(appJs).toContain('getAudioEngine: () => audioEngine,');
    expect(appJs).toContain('triggerMidiActivity');
  });

  it('white keys use an aged ivory gradient instead of flat white', () => {
    const whiteRule = css.slice(css.indexOf('.key.white {'), css.indexOf('.key.white:hover'));
    expect(whiteRule).toContain('linear-gradient(180deg, #fdfaf1');
    expect(whiteRule).toContain('#e6dabf'); // yellowed base
  });

  it('stain overlays are defined for all wear classes and never intercept clicks', () => {
    const whiteStains = ['stain-yellow', 'stain-scuff', 'stain-ding'];
    for (const cls of whiteStains) expect(css).toContain(`.key.white.${cls}::after`);
    expect(css).toContain('.key.black.stain-worn::after');
    // The shared overlay base applies to both key colours with multiply blending.
    expect(css).toMatch(/\.key\.white::after, \.key\.black::after \{[^}]*pointer-events: none/);
    expect(css).toContain('mix-blend-mode: multiply');
  });

  it('a global patina overlay sits above the keys but stays click-transparent', () => {
    expect(css).toMatch(/\.keyboard::after \{[^}]*z-index: 3/);
    expect(css).toMatch(/\.keyboard::after \{[^}]*pointer-events: none/);
    expect(css).toContain('repeating-linear-gradient(0deg, rgba(255, 255, 255, 0.015)');
  });
});
