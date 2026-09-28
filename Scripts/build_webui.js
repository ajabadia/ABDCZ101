// scripts/build_webui.js — el BUNDLE de la WebUI del CZ-101 (`npm run bundle`).
//
// Que es: `vite build` con `WebUI/vite.build.config.js` (el mismo patron que el
// hermano ABDMS2000) y, despues, la COMPROBACION de que lo que salio sirve:
// los once parciales de la hoja en orden canonico y el estatico que la pagina
// pide por URL. Sin esa comprobacion, un bundle puede salir "verde" yervative
// de reglas en un orden distinto al de la pagina de desarrollo, que es el fallo
// mas caro y mas invisible de este tipo de pagina.
//
// Que NO es, a diferencia de ABDMS2000: un sello de version. Alli
// `build_webui.js` genera `Source/Core/BuildVersion.h` y
// `contracts/buildVersion.js`; aqui la version la lleva `Scripts/UpdateVersion
// .cmake` desde CMake y NADIE consume un `buildVersion.js`, asi que aqui no se
// inventa.
//
// Uso:
//   node scripts/build_webui.js            (lo llama `npm run bundle`)
//   node scripts/build_webui.js --serve    (ademas, levanta `vite preview`)

import fs from 'node:fs';
import path from 'node:path';
import { spawnSync } from 'node:child_process';
import { createRequire } from 'node:module';
import { fileURLToPath } from 'node:url';

import { EXPECTED_ORDER, validateDistOrder } from './validate_css_order.js';

const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);
const rootDir = path.resolve(__dirname, '..');
const webUiRoot = path.join(rootDir, 'WebUI');
const distDir = path.join(webUiRoot, 'dist');

// Vite por su binario resuelto, no por `npx`: desde Node 18.20 lanzar un `.cmd`
// sin `shell` esta bloqueado por seguridad, y `npx` ademas podria resolver otro
// binario que el del lockfile. Se ejecuta con el MISMO node que corre este
// script, que es el que guarantee el arbol de `node_modules`.
const require = createRequire(import.meta.url);
// El subpath `vite/bin/vite.js` no esta en el `exports` del paquete, asi que se
// busca `bin/vite.js` subiendo desde su entrada principal: la profundidad cambia
// entre pnpm (`.pnpm/vite@x/node_modules/vite/index.cjs`) y un arbol plano
// (`node_modules/vite/dist/node/index.js`), y no conviene atar el script a una.
// Se comprueba que exista, para que el fallo sea un mensaje claro y no un
// `MODULE_NOT_FOUND` en mitad del build.
function findViteBin(from) {
  let dir = path.dirname(from);

  for (let depth = 0; depth < 4; depth += 1) {
    const candidate = path.join(dir, 'bin', 'vite.js');

    if (fs.existsSync(candidate)) return candidate;

    const parent = path.dirname(dir);

    if (parent === dir) break;
    dir = parent;
  }

  return null;
}

const viteBin = findViteBin(require.resolve('vite'));

if (!viteBin) {
  console.error('[build-webui] FAIL: no se encuentra el binario de vite (pnpm install en el monorepo)');
  process.exit(1);
}

const runVite = (args) => spawnSync(process.execPath, [viteBin, ...args], {
  cwd: rootDir,
  stdio: 'inherit',
});

// ── 1. El build ───────────────────────────────────────────────────────────────
const vite = runVite(['build', '--config', path.join(webUiRoot, 'vite.build.config.js')]);

if (vite.error) {
  console.error('[build-webui] FAIL: no se pudo lanzar vite: ' + vite.error.message);
  process.exit(1);
}

if (vite.status !== 0) {
  console.error('[build-webui] FAIL: `vite build` devolvio ' + vite.status);
  process.exit(1);
}

// ── 2. Lo que tiene que haber salido ──────────────────────────────────────────
const problems = [];
const distIndex = path.join(distDir, 'index.html');

if (!fs.existsSync(distIndex)) {
  problems.push('dist/index.html no existe tras el build');
} else {
  // La cascada: los once parciales, en orden, y EN SUS FICHEROS. Este es el
  // punto donde un bundle se diferencia de la pagina de desarrollo sin que se
  // note mirando la pagina.
  const { ok, problems: orderProblems } = validateDistOrder(fs.readFileSync(distIndex, 'utf8'));

  if (!ok) problems.push(...orderProblems);

  for (const partial of EXPECTED_ORDER) {
    const file = path.join(distDir, 'styles', partial);

    if (!fs.existsSync(file)) problems.push(`dist/styles/${partial} no existe`);

    const source = path.join(webUiRoot, 'src', 'styles', partial);

    if (fs.existsSync(file) && fs.existsSync(source)
        && !fs.readFileSync(file).equals(fs.readFileSync(source))) {
      problems.push(`dist/styles/${partial} no es identico al parcial de origen (la cascada cambio)`);
    }
  }
}

// El PEGAMENTO del motor: sin el, la pagina abre y no suena, que es justo lo
// que un bundle "verde" puede esconder. El BINARIO (`cz101_dsp.wasm`) no esta en
// el arbol —lo produce el build de emscripten—, asi que solo se exige si el
// original esta: cuando aparezca, tiene que viajar al bundle al lado del
// pegamento, que es donde lo busca `new URL(..., import.meta.url)`.
if (!fs.existsSync(path.join(distDir, 'wasm', 'cz101_dsp.js'))) {
  problems.push('dist/wasm/cz101_dsp.js no existe (el pegamento del motor se pide por fetch)');
}

const sourceWasm = path.join(webUiRoot, 'wasm', 'cz101_dsp.wasm');
const distWasm = path.join(distDir, 'wasm', 'cz101_dsp.wasm');

if (fs.existsSync(sourceWasm) && !fs.existsSync(distWasm)) {
  problems.push('WebUI/wasm/cz101_dsp.wasm existe pero no llego al bundle');
}

if (problems.length > 0) {
  console.error('\n[build-webui] FAIL: el bundle no cumple lo que la pagina de desarrollo cumple.');
  for (const problem of problems) console.error(`  - ${problem}`);
  process.exit(1);
}

// ── 3. Que se ha construido, en cifras ────────────────────────────────────────
const sizeOf = (file) => (fs.existsSync(file) ? `${(fs.statSync(file).size / 1024).toFixed(1)} KB` : '—');
const entry = path.join(distDir, 'assets', 'index.js');

console.log(`[build-webui] OK — bundle en WebUI/dist/`);
console.log(`[build-webui]   entrada:  assets/index.js ${sizeOf(entry)} (${sizeOf(path.join(distDir, 'assets', 'index.css'))} de CSS importada desde JS)`);
console.log(`[build-webui]   hoja:     ${EXPECTED_ORDER.length} parciales verbatim en dist/styles/`);
console.log(`[build-webui]   estatico: wasm/, presets/, assets/, icons/ y los ficheros sueltos`);

if (process.argv.includes('--serve')) {
  const port = process.env.CZ101_PREVIEW_PORT ?? '4173';
  const preview = runVite(['preview', '--port', port, '--strictPort']);

  process.exit(preview.status ?? 0);
}
