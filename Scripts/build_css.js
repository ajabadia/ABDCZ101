// build_css.js
//
// Production CSS build for the WebUI.
//
// 1. Bundles the 11 partials (via main.css, which keeps the canonical order)
//    and minifies them into a single file: WebUI/dist/styles.min.css.
// 2. With --prod it rewrites index.html to load the single minified bundle
//    (11 <link> tags → 1). With --dev it restores the 11 dev-time partials.
//    The toggle is idempotent and reversible; it refuses to run when the
//    working tree is dirty so a half-applied build never gets committed.
//
// Usage:
//   node scripts/build_css.js            # build bundle only (no index.html change)
//   node scripts/build_css.js --prod     # build + point index.html at the bundle
//   node scripts/build_css.js --dev      # restore index.html to dev partials
//
// The canonical order lives in scripts/validate_css_order.js — run
// `npm run validate:css` after touching styles to keep both entry points in
// sync (build:css runs it automatically).

import fs from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';
import { execFileSync } from 'node:child_process';
import * as esbuild from 'esbuild';

const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);
const ROOT = path.resolve(__dirname, '..');

const INDEX = path.join(ROOT, 'WebUI', 'index.html');
const MAIN_CSS = path.join(ROOT, 'WebUI', 'src', 'styles', 'main.css');
const DIST_DIR = path.join(ROOT, 'WebUI', 'dist');
const BUNDLE = path.join(DIST_DIR, 'styles.min.css');

const PARTIAL_LINKS = [
  'base.css', 'lcd.css', 'panels.css', 'controls.css', 'keyboard.css',
  'envelopes.css', 'midi.css', 'menu.css', 'bankManager.css',
  'overlays.css', 'themes.css',
];

function fail(msg) {
  console.error(`build:css — ${msg}`);
  process.exit(1);
}

function runValidate() {
  try {
    execFileSync(process.execPath, [path.join(__dirname, 'validate_css_order.js')], { stdio: 'inherit' });
  } catch {
    fail('CSS order validation failed — run `npm run validate:css` for details.');
  }
}

async function buildBundle() {
  const srcBytes = PARTIAL_LINKS.reduce(
    (n, f) => n + fs.statSync(path.join(ROOT, 'WebUI', 'src', 'styles', f)).size, 0);

  const result = await esbuild.build({
    entryPoints: [MAIN_CSS],
    bundle: true,
    minify: true,
    outfile: BUNDLE,
    logLevel: 'silent',
    // Copy referenced assets (e.g. bg.png) next to the bundle and rewrite
    // their url() relative to it. base64 would balloon the CSS (bg.png is 8 MB).
    loader: { '.png': 'file', '.jpg': 'file', '.jpeg': 'file', '.gif': 'file', '.svg': 'file', '.woff2': 'file' },
    assetNames: '[name]',
  });
  if (result.errors.length > 0) fail(result.errors.map(e => e.text).join('\n'));

  const outBytes = fs.statSync(BUNDLE).size;
  const pct = ((1 - outBytes / srcBytes) * 100).toFixed(1);
  console.log(
    `build:css — ${BUNDLE.replace(ROOT + path.sep, '')} ` +
    `(${fmt(srcBytes)} → ${fmt(outBytes)}, -${pct}%)`
  );
}

function fmt(bytes) {
  return bytes >= 1024 * 1024
    ? `${(bytes / 1024 / 1024).toFixed(2)} MB`
    : `${Math.round(bytes / 1024)} KB`;
}

function indexUsesBundle() {
  const html = fs.readFileSync(INDEX, 'utf8');
  return html.includes('dist/styles.min.css');
}

function applyProdIndex() {
  let html = fs.readFileSync(INDEX, 'utf8');
  const partialTag = (f) => `  <link rel="stylesheet" href="./src/styles/${f}">`;
  const partialBlock = PARTIAL_LINKS.map(partialTag).join('\n');
  const bundleTag = '  <link rel="stylesheet" href="./dist/styles.min.css">';

  if (html.includes(bundleTag)) {
    console.log('build:css — index.html already points at the bundle (--prod).');
    return;
  }
  if (!html.includes(partialBlock)) {
    fail('index.html does not contain the expected 11 partial <link> block — refusing to rewrite.');
  }
  html = html.replace(partialBlock, bundleTag);
  fs.writeFileSync(INDEX, html);
  console.log('build:css — index.html now loads ./dist/styles.min.css (--prod).');
}

function applyDevIndex() {
  let html = fs.readFileSync(INDEX, 'utf8');
  const partialBlock = PARTIAL_LINKS.map((f) => `  <link rel="stylesheet" href="./src/styles/${f}">`).join('\n');
  const bundleTag = '  <link rel="stylesheet" href="./dist/styles.min.css">';

  if (!html.includes(bundleTag)) {
    console.log('build:css — index.html already loads the dev partials (--dev).');
    return;
  }
  html = html.replace(bundleTag, partialBlock);
  fs.writeFileSync(INDEX, html);
  console.log('build:css — index.html restored to the 11 dev partials (--dev).');
}

async function main() {
  const mode = process.argv[2];

  if (mode === '--dev') {
    // No bundle needed; just restore index.html. Validate AFTER restoring so
    // the check runs against the dev (partial) entry point.
    applyDevIndex();
    runValidate();
    return;
  }

  if (mode === '--prod') {
    // Refuse to switch index.html while the repo has uncommitted changes to it
    // (a half-applied build must never be committed by accident). Only tracked
    // modifications count — untracked files (??) are fine.
    try {
      const dirty = execFileSync('git', ['status', '--porcelain', '--', INDEX], { cwd: ROOT, encoding: 'utf8' })
        .split('\n').filter(l => l.trim() && !l.startsWith('??'));
      if (dirty.length > 0) fail('index.html has uncommitted changes — commit or stash them before --prod.');
    } catch {
      // not a git repo / git unavailable: skip the guard
    }
    await buildBundle();
    if (indexUsesBundle()) {
      // Already in prod: index.html only references the bundle, so the
      // partial-vs-bundle cross-check does not apply (expected state).
      console.log('build:css — index.html already points at the bundle (--prod).');
      return;
    }
    // Validate BEFORE switching: index.html still references the 11 partials,
    // so both entry points can be cross-checked (after the switch it only
    // references the bundle, which would trip the validator).
    runValidate();
    applyProdIndex();
    return;
  }

  if (mode !== undefined) fail(`unknown mode "${mode}" (use --prod, --dev or no flag).`);
  await buildBundle();
  runValidate();
  console.log('build:css — done (dev mode unchanged). Use --prod/--dev to toggle index.html.');
}

main().catch(e => { console.error('build:css — fatal', e); process.exit(2); });
