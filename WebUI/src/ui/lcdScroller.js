// LCD char-by-char scroller + scroll settings (extracted from lcdPanel.js).
// Reusable ping-pong scroll bound to one LCD line element, used by Bank Browse
// (BNK), Model Browse (MDL), the menu readout and the normal program-mode
// readout. Scroll speed/pause are WebUI-only UI settings persisted in
// localStorage (like the native "Scroll" UI preference) and read live by
// startLcdScroll so changes apply to the running readout at once.

// Scroll timing defaults (adjustable from SET > SYSTEM: SCROLL SPEED / SCROLL
// PAUSE — the native has a "Scroll" UI preference; here it persists in
// localStorage and startLcdScroll reads the live settings each time).
const LCD_SCROLL_DELAY_MS = 2000; // default pause before the name starts scrolling
const LCD_SCROLL_STEP_MS = 180;   // default one character per tick (LCD feel)
const LCD_NAME_MAX_FIT = 21;      // fallback char budget if measurement fails

// ── LCD scroll settings (WebUI-only UI setting, persisted) ──
const LCD_SCROLL_SETTINGS_KEY = 'cz101.lcdScroll';

const clampInt = (v, min, max, fallback) =>
  (Number.isFinite(v) ? Math.min(max, Math.max(min, Math.round(v))) : fallback);

let lcdScrollSettings = (() => {
  try {
    const raw = JSON.parse(localStorage.getItem(LCD_SCROLL_SETTINGS_KEY) || '{}');
    return {
      speed: clampInt(raw.speed, 40, 400, LCD_SCROLL_STEP_MS),
      pause: clampInt(raw.pause, 0, 5000, LCD_SCROLL_DELAY_MS),
    };
  } catch (err) {
    return { speed: LCD_SCROLL_STEP_MS, pause: LCD_SCROLL_DELAY_MS };
  }
})();

export const saveLcdScrollSettings = () => {
  try { localStorage.setItem(LCD_SCROLL_SETTINGS_KEY, JSON.stringify(lcdScrollSettings)); } catch (err) { /* ignore */ }
};

export const getScrollSpeed = () => lcdScrollSettings.speed;
export const getScrollPause = () => lcdScrollSettings.pause;

// Steps a scroll setting (speed/pause) and persists it. Used by the LCD menu
// SET > SYSTEM: SCROLL SPEED / SCROLL PAUSE entries (param.uiSetting).
export const setScrollSetting = (key, val) => {
  lcdScrollSettings[key] = val;
  saveLcdScrollSettings();
};

// How many monospace characters fit on a LCD line (measured, theme-proof).
// Uses the CONTENT box (clientWidth minus padding) so lines that reserve space
// for the BNK position badge measure a shorter, correct budget.
export const measureLcdChars = (el) => {
  if (!el) return LCD_NAME_MAX_FIT;
  const style = getComputedStyle(el);
  const padL = parseFloat(style.paddingLeft) || 0;
  const padR = parseFloat(style.paddingRight) || 0;
  const contentW = Math.max(0, el.clientWidth - padL - padR);
  const canvas = measureLcdChars.canvas || (measureLcdChars.canvas = document.createElement('canvas'));
  const ctx = canvas.getContext('2d');
  ctx.font = `${style.fontWeight || 'bold'} ${style.fontSize} ${style.fontFamily}`;
  const charW = ctx.measureText('W').width;
  if (!charW || charW <= 0) return LCD_NAME_MAX_FIT;
  return Math.max(4, Math.floor(contentW / charW));
};

// Renders `text` into `el`, scrolling it ping-pong when it overflows (LCD
// char-by-char scroll). Returns a cancel handle, or null when it fits.
export const startLcdScroll = (el, text) => {
  const fit = measureLcdChars(el);
  if (text.length <= fit) {
    el.innerText = text;
    return null;
  }
  const maxOffset = text.length - fit;
  let offset = 0;
  let dir = 1;
  let timer = null;
  el.innerText = text.slice(0, fit);
  const delayTimer = setTimeout(() => {
    timer = setInterval(() => {
      offset += dir;
      if (offset >= maxOffset) { offset = maxOffset; dir = -1; }
      else if (offset <= 0) { offset = 0; dir = 1; }
      el.innerText = text.slice(offset, offset + fit);
    }, getScrollSpeed());
  }, getScrollPause());
  return () => {
    clearTimeout(delayTimer);
    if (timer) clearInterval(timer);
  };
};

// ── Generic LCD scroller ──
// Reusable char-by-char ping-pong scroll bound to one LCD line element. Used by
// Bank Browse (BNK), Model Browse (MDL), the LCD menu readout and the normal
// program-mode readout. Each owner keeps its own scroller; only one mode is
// active at a time and switching modes stops the previous owner's scroller.
export const createLcdScroller = (el) => ({
  text: null,
  cancel: null,
  // Idempotent: unchanged text keeps the running scroll; new text cancels the
  // old scroll and starts fresh (pause + ping-pong).
  set(text) {
    if (this.text === text) return;
    if (this.cancel) { this.cancel(); this.cancel = null; }
    this.text = text;
    this.cancel = startLcdScroll(el, text);
  },
  stop() {
    if (this.cancel) { this.cancel(); this.cancel = null; }
    this.text = null;
  },
  // Re-run the current text with the current scroll settings (used after the
  // user changes SCROLL SPEED / PAUSE so the running readout picks it up).
  restart() {
    if (this.text == null) return;
    if (this.cancel) { this.cancel(); this.cancel = null; }
    this.cancel = startLcdScroll(el, this.text);
  }
});
