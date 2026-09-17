// Theme manager for the WebUI — parity with the native SkinManager
// (Source/UI/SkinManager.h) + DesignTokens (Source/UI/DesignTokens.h).
//
// The native plugin exposes 9 themes through SkinManager::Theme and the
// DesignTokens::Colors palettes. Each palette is converted here to a CSS
// custom-property map so the whole WebUI re-themes via document.documentElement
// style.setProperty, and a `theme-effect` class drives the visual effect
// (Scanlines / Glass / None) exactly like DesignTokens::Colors::Palette::effect.
//
// JUCE colours (0xaarrggbb) map to CSS hex; the computed palette entries
// (withMultipliedSaturation / brighter / withAlpha) were converted with the
// same HSL math JUCE uses.

function withAlpha(hex, alpha) {
  const r = parseInt(hex.slice(1, 3), 16);
  const g = parseInt(hex.slice(3, 5), 16);
  const b = parseInt(hex.slice(5, 7), 16);
  return `rgba(${r}, ${g}, ${b}, ${alpha})`;
}

// Relative luminance (WCAG 2.x) for a #rrggbb colour, 0..1.
function luminance(hex) {
  const c = [1, 3, 5].map(i => parseInt(hex.slice(i, i + 2), 16) / 255);
  const lin = c.map(v => (v <= 0.03928 ? v / 12.92 : Math.pow((v + 0.055) / 1.055, 2.4)));
  return 0.2126 * lin[0] + 0.7152 * lin[1] + 0.0722 * lin[2];
}

// Text colour that keeps a readable contrast on top of `hex`: dark text on
// bright accents, white on dark accents (used for hovered/accent buttons).
// The white-vs-dark contrast curves cross at L ≈ 0.179, so anything brighter
// than that takes dark text to stay ≥ ~4.5:1 (WCAG AA).
function textOn(hex) {
  return luminance(hex) > 0.18 ? '#000000' : '#ffffff';
}

// Build the CSS variable map for a DesignTokens palette. Keys match the
// variables used by main.css (plus new ones for the full palette coverage).
function toVars(p) {
  return {
    '--bg-dark': p.background,
    '--bg-panel': p.surface,
    '--color-section': p.sectionBackground,
    '--color-surface-light': p.surfaceLight,
    '--border-light': p.border,
    '--color-accent': p.accentCyan,
    '--color-accent-dim': withAlpha(p.accentCyan, 0.15),
    '--color-accent-2': p.accentOrange,
    '--color-accent-3': p.accentTertiary,
    '--color-glow': p.glowColor,
    '--color-modern': p.modernIndicator,
    '--color-lcd-bg': p.lcdBg,
    '--color-lcd-text': p.lcdText,
    '--text-primary': p.textPrimary,
    '--text-secondary': p.textSecondary,
    '--text-on-accent': textOn(p.accentCyan)
  };
}

export const THEME_EFFECTS = ['none', 'scanlines', 'glass'];

export const THEMES = [
  {
    id: 'dark',
    name: 'Dark',
    effect: 'none',
    vars: toVars({
      background: '#3a3a3a',
      surface: '#2a2a2a',
      accentCyan: '#4a9eff',
      accentOrange: '#f5a623',
      accentTertiary: '#50e3c2',
      border: '#202020',
      textPrimary: '#ffffff',
      textSecondary: '#d3d3d3',
      lcdBg: '#d2e4c8',
      lcdText: '#1a1a1a',
      glowColor: 'transparent',
      sectionBackground: '#2a2a2a',
      surfaceLight: '#4a4a4a',
      modernIndicator: withAlpha('#f5a623', 0.15)
    })
  },
  {
    id: 'vintage',
    name: 'Vintage',
    effect: 'none',
    vars: toVars({
      background: '#8c8c8c',
      surface: '#545454', // darkened for text-secondary (≥4.5:1) + accent-title contrast
      accentCyan: '#1dbb9b',
      accentOrange: '#d35400',
      accentTertiary: '#2980b9',
      border: '#4d4d4d',
      textPrimary: '#ffffff',
      textSecondary: '#dddddd',
      lcdBg: '#66bb6a',
      lcdText: '#000000',
      glowColor: 'transparent',
      sectionBackground: '#555555',
      surfaceLight: '#777777',
      modernIndicator: withAlpha('#ffd700', 0.2)
    })
  },
  {
    id: 'cyberglow',
    name: 'CyberGlow',
    effect: 'none',
    vars: toVars({
      background: '#0b0e14',
      surface: '#161b22',
      accentCyan: '#00f2ff',
      accentOrange: '#ff007a',
      accentTertiary: '#bc13fe',
      border: '#30363d',
      textPrimary: '#c9d1d9',
      textSecondary: '#8b949e',
      lcdBg: '#000000',
      lcdText: '#00f2ff',
      glowColor: '#00f2ff',
      sectionBackground: '#0d1117',
      surfaceLight: '#21262d',
      modernIndicator: withAlpha('#ff007a', 0.2)
    })
  },
  {
    id: 'neonretro',
    name: 'Neon Retro',
    effect: 'scanlines',
    vars: toVars({
      background: '#120458',
      surface: '#2d025d',
      accentCyan: '#ff00c8',
      accentOrange: '#39ff14',
      accentTertiary: '#7b00ff',
      border: '#ff00c8',
      textPrimary: '#ffffff',
      textSecondary: '#f0f0f0',
      lcdBg: '#000000',
      lcdText: '#ff00c8',
      glowColor: '#ff00c8',
      sectionBackground: '#1b0044',
      surfaceLight: '#40058b',
      modernIndicator: withAlpha('#39ff14', 0.2)
    })
  },
  {
    id: 'steampunk',
    name: 'Steampunk',
    effect: 'none',
    vars: toVars({
      background: '#3e2723',
      surface: '#4e342e',
      accentCyan: '#cd7f32',
      accentOrange: '#b87333',
      accentTertiary: '#8b4513',
      border: '#211a17',
      textPrimary: '#d7ccc8',
      textSecondary: '#b8a89e', // lightened for ≥4.5:1 on the #4e342e surface
      lcdBg: '#263238',
      lcdText: '#ffab40',
      glowColor: '#ffab40',
      sectionBackground: '#321a11',
      surfaceLight: '#5d4037',
      modernIndicator: withAlpha('#cd7f32', 0.25)
    })
  },
  {
    id: 'retroterminal',
    name: 'Retro Terminal',
    effect: 'scanlines',
    vars: toVars({
      background: '#000000',
      surface: '#0c0c0c',
      accentCyan: '#00ff41',
      accentOrange: '#008f11',
      accentTertiary: '#003b00',
      border: '#00ff41',
      textPrimary: '#00ff41',
      textSecondary: '#00ff41',
      lcdBg: '#000000',
      lcdText: '#00ff41',
      glowColor: '#00ff41',
      sectionBackground: '#050505',
      surfaceLight: '#121212',
      modernIndicator: withAlpha('#ffffff', 0.2)
    })
  }
];

/** Look up a theme by id; falls back to the first theme (Dark, like the native default). */
export function getTheme(themeId) {
  return THEMES.find(t => t.id === themeId) || THEMES[0];
}

/** Apply a theme to `root` (documentElement) via CSS custom properties + effect class. */
export function applyTheme(root, themeId) {
  const theme = getTheme(themeId);
  if (!root || typeof root.style.setProperty !== 'function') return theme;
  Object.entries(theme.vars).forEach(([prop, value]) => {
    root.style.setProperty(prop, value);
  });
  THEME_EFFECTS.forEach(effect => root.classList.remove(`theme-effect-${effect}`));
  root.classList.add(`theme-effect-${theme.effect}`);
  return theme;
}
