// validate_css_order.js
//
// Verifies that index.html and main.css load the CSS partials in the SAME
// cascade order. The browser applies the partials in document order, so if
// someone reorders the <link> tags but forgets the @import list (or vice
// versa), later rules can silently override earlier ones differently between
// the two entry points.
//
// Usage:
//   node scripts/validate_css_order.js        (exit 0/1)
//   import { validateCssOrder } from '../scripts/validate_css_order.js' (tests)

import fs from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';

const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);
const ROOT_DIR = path.resolve(__dirname, '..');

// Canonical order — the single source of truth. The 11 partials that make up
// the stylesheet split (see WebUI/src/styles/main.css).
export const EXPECTED_ORDER = [
  'base.css',
  'lcd.css',
  'panels.css',
  'controls.css',
  'keyboard.css',
  'envelopes.css',
  'midi.css',
  'menu.css',
  'bankManager.css',
  'overlays.css',
  'themes.css',
];

export function readCssEntryPoints(rootDir = ROOT_DIR) {
  const indexPath = path.join(rootDir, 'WebUI', 'index.html');
  const mainCssPath = path.join(rootDir, 'WebUI', 'src', 'styles', 'main.css');

  const html = fs.readFileSync(indexPath, 'utf8');
  const mainCss = fs.readFileSync(mainCssPath, 'utf8');

  // <link rel="stylesheet" href="./src/styles/xxx.css"> → ['xxx.css', ...]
  // Only local styles (skip the Google Fonts <link>).
  const htmlOrder = [...html.matchAll(/<link[^>]*rel="stylesheet"[^>]*href="\.\/src\/styles\/([^"]+\.css)"/g)]
    .map(m => m[1]);

  // @import './xxx.css'; → ['xxx.css', ...]
  const cssOrder = [...mainCss.matchAll(/@import\s+['"]\.\/([^'"]+\.css)['"]\s*;/g)]
    .map(m => m[1]);

  return { htmlOrder, cssOrder };
}

// Returns { ok, problems } where problems is an array of human-readable
// messages (empty when ok).
export function validateCssOrder(rootDir = ROOT_DIR) {
  const { htmlOrder, cssOrder } = readCssEntryPoints(rootDir);
  const problems = [];

  const same = (a, b) => a.length === b.length && a.every((v, i) => v === b[i]);

  if (!same(htmlOrder, EXPECTED_ORDER)) {
    problems.push(
      `index.html <link> order diverges from canonical.\n` +
      `  expected: ${EXPECTED_ORDER.join(', ')}\n` +
      `  actual:   ${htmlOrder.join(', ') || '(none)'}`
    );
  }
  if (!same(cssOrder, EXPECTED_ORDER)) {
    problems.push(
      `main.css @import order diverges from canonical.\n` +
      `  expected: ${EXPECTED_ORDER.join(', ')}\n` +
      `  actual:   ${cssOrder.join(', ') || '(none)'}`
    );
  }
  if (!same(htmlOrder, cssOrder)) {
    problems.push(
      `index.html and main.css disagree with each other.\n` +
      `  index.html: ${htmlOrder.join(', ') || '(none)'}\n` +
      `  main.css:   ${cssOrder.join(', ') || '(none)'}`
    );
  }
  if (new Set(htmlOrder).size !== htmlOrder.length || new Set(cssOrder).size !== cssOrder.length) {
    problems.push('duplicate stylesheet entries found (same partial listed more than once).');
  }

  return { ok: problems.length === 0, problems };
}

// CLI entry point
const isMain = process.argv[1] && path.resolve(process.argv[1]) === __filename;
if (isMain) {
  try {
    const { ok, problems } = validateCssOrder();
    if (ok) {
      console.log(`OK — ${EXPECTED_ORDER.length} partials in canonical order in both index.html and main.css.`);
      process.exit(0);
    }
    console.error('CSS order validation FAILED:\n');
    for (const p of problems) console.error(`  - ${p.replaceAll('\n', '\n    ')}`);
    process.exit(1);
  } catch (err) {
    console.error(`CSS order validation could not run: ${err.message}`);
    process.exit(2);
  }
}
