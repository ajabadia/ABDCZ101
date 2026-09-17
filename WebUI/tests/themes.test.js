import { describe, it, expect } from 'vitest';
import { THEMES, THEME_EFFECTS, getTheme, applyTheme } from '../src/contracts/themes.js';

// Minimal stand-in for document.documentElement (the vitest suite runs with
// environment: 'node', so there is no real DOM). applyTheme only touches
// style.setProperty and classList, which this fake mirrors.
function createFakeRoot() {
  const style = {};
  const classes = new Set();
  return {
    style: {
      setProperty: (prop, value) => { style[prop] = value; },
      getPropertyValue: (prop) => style[prop] || ''
    },
    classList: {
      add: (...names) => names.forEach(n => classes.add(n)),
      remove: (...names) => names.forEach(n => classes.delete(n)),
      contains: (name) => classes.has(name)
    }
  };
}

// Parity with the native SkinManager (Source/UI/SkinManager.h): exactly 9
// themes in the same order as SkinManager::Theme, each with the DesignTokens
// palette mapped to CSS variables, and the effect class per palette.

describe('Theme manager (native SkinManager parity)', () => {
  it('exposes exactly the 6 native themes in the same order', () => {
    expect(THEMES.map(t => t.id)).toEqual([
      'dark', 'vintage', 'cyberglow',
      'neonretro', 'steampunk', 'retroterminal'
    ]);
    expect(THEMES.map(t => t.name)).toEqual([
      'Dark', 'Vintage', 'CyberGlow',
      'Neon Retro', 'Steampunk', 'Retro Terminal'
    ]);
  });

  it('each theme defines the full CSS variable map with valid colors', () => {
    const requiredVars = [
      '--bg-dark', '--bg-panel', '--color-section', '--color-surface-light',
      '--border-light', '--color-accent', '--color-accent-dim',
      '--color-accent-2', '--color-accent-3', '--color-glow', '--color-modern',
      '--color-lcd-bg', '--color-lcd-text', '--text-primary', '--text-secondary',
      '--text-on-accent'
    ];
    for (const theme of THEMES) {
      requiredVars.forEach(v => {
        expect(theme.vars, `${theme.id} missing ${v}`).toHaveProperty(v);
        expect(theme.vars[v], `${theme.id}.${v} empty`).toBeTruthy();
      });
    }
  });

  it('text-on-accent keeps ≥ WCAG AA contrast against its accent', () => {
    // Relative luminance + contrast ratio per WCAG 2.x.
    const lum = (hex) => {
      const c = [1, 3, 5].map(i => parseInt(hex.slice(i, i + 2), 16) / 255);
      const lin = c.map(v => (v <= 0.03928 ? v / 12.92 : Math.pow((v + 0.055) / 1.055, 2.4)));
      return 0.2126 * lin[0] + 0.7152 * lin[1] + 0.0722 * lin[2];
    };
    const contrast = (a, b) => {
      const [hi, lo] = [lum(a), lum(b)].sort((x, y) => y - x);
      return (hi + 0.05) / (lo + 0.05);
    };
    for (const theme of THEMES) {
      const ratio = contrast(theme.vars['--text-on-accent'], theme.vars['--color-accent']);
      expect(ratio, `${theme.id}: --text-on-accent vs --color-accent = ${ratio.toFixed(2)}:1`)
        .toBeGreaterThanOrEqual(4.5);
    }
  });

  it('body text has decent contrast against the panel surface', () => {
    // --text-primary / --text-secondary on the panel (surface) — secondary is
    // labels/muted text, require ≥ 4.5:1 like the native UI.
    const lum = (hex) => {
      const c = [1, 3, 5].map(i => parseInt(hex.slice(i, i + 2), 16) / 255);
      const lin = c.map(v => (v <= 0.03928 ? v / 12.92 : Math.pow((v + 0.055) / 1.055, 2.4)));
      return 0.2126 * lin[0] + 0.7152 * lin[1] + 0.0722 * lin[2];
    };
    const contrast = (a, b) => {
      const [hi, lo] = [lum(a), lum(b)].sort((x, y) => y - x);
      return (hi + 0.05) / (lo + 0.05);
    };
    for (const theme of THEMES) {
      const primary = contrast(theme.vars['--text-primary'], theme.vars['--bg-panel']);
      const secondary = contrast(theme.vars['--text-secondary'], theme.vars['--bg-panel']);
      expect(primary, `${theme.id}: text-primary on panel = ${primary.toFixed(2)}:1`)
        .toBeGreaterThanOrEqual(4.5);
      expect(secondary, `${theme.id}: text-secondary on panel = ${secondary.toFixed(2)}:1`)
        .toBeGreaterThanOrEqual(4.5);
    }
  });

  it('reproduces the native Dark palette values (DesignTokens.h)', () => {
    const dark = THEMES[0].vars;
    expect(dark['--bg-dark']).toBe('#3a3a3a');
    expect(dark['--color-accent']).toBe('#4a9eff');      // czBlue
    expect(dark['--color-accent-2']).toBe('#f5a623');    // czOrange
    expect(dark['--color-accent-3']).toBe('#50e3c2');    // czGreen
    expect(dark['--color-lcd-bg']).toBe('#d2e4c8');      // lcdBackground
    expect(dark['--color-lcd-text']).toBe('#1a1a1a');    // lcdText
    expect(dark['--text-primary']).toBe('#ffffff');
  });

  it('matches the native palettes for the distinctive themes', () => {
    const cyber = getTheme('cyberglow').vars;
    expect(cyber['--bg-dark']).toBe('#0b0e14');
    expect(cyber['--color-accent']).toBe('#00f2ff');
    expect(cyber['--color-accent-2']).toBe('#ff007a');

    const terminal = getTheme('retroterminal').vars;
    expect(terminal['--bg-dark']).toBe('#000000');
    expect(terminal['--color-accent']).toBe('#00ff41');
    expect(terminal['--color-lcd-bg']).toBe('#000000');
    expect(terminal['--color-lcd-text']).toBe('#00ff41');

    const steampunk = getTheme('steampunk').vars;
    expect(steampunk['--color-lcd-bg']).toBe('#263238');
    expect(steampunk['--color-lcd-text']).toBe('#ffab40'); // amber LCD
  });

  it('assigns the native visual effects (DesignTokens Palette.effect)', () => {
    expect(getTheme('neonretro').effect).toBe('scanlines');
    expect(getTheme('retroterminal').effect).toBe('scanlines');
    expect(getTheme('steampunk').effect).toBe('none');
    expect(getTheme('dark').effect).toBe('none');
    expect(getTheme('cyberglow').effect).toBe('none');
  });

  it('getTheme falls back to Dark for unknown ids (native default)', () => {
    expect(getTheme('nonexistent').id).toBe('dark');
    expect(getTheme(undefined).id).toBe('dark');
  });

  it('applyTheme sets the CSS variables and effect class on a root element', () => {
    const root = createFakeRoot();
    const theme = applyTheme(root, 'neonretro');

    expect(root.style.getPropertyValue('--bg-dark')).toBe('#120458');
    expect(root.style.getPropertyValue('--color-accent')).toBe('#ff00c8');
    expect(root.classList.contains('theme-effect-scanlines')).toBe(true);
    expect(theme.id).toBe('neonretro');
  });

  it('applyTheme swaps effect classes when switching themes', () => {
    const root = createFakeRoot();
    applyTheme(root, 'neonretro');
    expect(root.classList.contains('theme-effect-scanlines')).toBe(true);

    applyTheme(root, 'cyberglow');
    expect(root.classList.contains('theme-effect-glass')).toBe(false);
    expect(root.classList.contains('theme-effect-none')).toBe(true);
    expect(root.classList.contains('theme-effect-scanlines')).toBe(false);
  });

  it('THEME_EFFECTS lists the supported effect classes', () => {
    expect(THEME_EFFECTS).toEqual(['none', 'scanlines', 'glass']);
  });
});
