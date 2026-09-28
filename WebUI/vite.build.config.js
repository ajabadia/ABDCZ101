import { defineConfig } from 'vite';
import fs from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';

const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);
const webUiRoot = __dirname; // .../ABDCZ101/WebUI

/**
 * Build de PRODUCCION de la WebUI del CZ-101 (`npm run bundle`).
 *
 * Mismo patron que el hermano ABDMS2000 (`vite.build.config.js` +
 * `Scripts/build_webui.js`): Vite resuelve los bare imports (`@abdsynths/shared`,
 * `@abdsynths/midi-keyb`) y empaqueta el grafo de JS en `dist/`. Lo que NO se
 * copia de ABDMS2000 es su sello de version: alli `build_webui.js` genera
 * `Source/Core/BuildVersion.h` y `contracts/buildVersion.js`; aqui la version
 * la lleva `Scripts/UpdateVersion.cmake` desde CMake y NADIE consume un
 * `buildVersion.js`, asi que inventarlo seria escribir un fichero que no lee
 * nadie.
 *
 * ── LO QUE ESTE FICHERO PROTEGE, Y POR QUE ──
 *
 * 1. LA CASCADA DE CSS NO SE EMPAQUETA. La hoja son ONCE parciales en orden
 *    canonico (`scripts/validate_css_order.js` lo fija) y el navegador los
 *    aplica en orden de documento: Vite trataria cada `<link rel=stylesheet>`
 *    como entrada de su grafo CSS y los FUSIONARIA en un unico
 *    `assets/index.css` (medido: asi lo hace, y de paso se come el `<link>` de
 *    Google Fonts). Con eso, quien gana una regla pasaria a depender de la
 *    reescritura y no del HTML. Aqui los `<link>` se apartan del grafo con un
 *    testigo en la fase `pre` y se vuelven a poner, en el MISMO orden y con
 *    `href="./styles/<parcial>.css"`, en la fase `post`. Los once parciales se
 *    copian verbatim a `dist/styles/`: la cascada del bundle es, por
 *    construccion, la de la pagina de desarrollo, y `build_webui.js` lo
 *    comprueba (mismo EXPECTED_ORDER) sobre el `dist/index.html` YA escrito.
 *
 * 2. `juce.js` NO esta en el repo: lo inyecta el WebView nativo en tiempo de
 *    ejecucion, y en el navegador es un 404 inocuo que el codigo tolera
 *    (`getJuceBackend()` devuelve null y la pagina sigue). Vite, en cambio,
 *    intenta resolverlo como asset y avisa de que no se puede empaquetar. Se
 *    aparta con el mismo truco de testigo y vuelve con su URL ORIGINAL.
 *
 * Nombres SIN hash (`assets/index.js`): el proveedor nativo de recursos resuelve
 * por ruta y no hay mapping de hashes embebido.
 */
const STYLE_MARKER = '<!--CZ101_STYLE_LINKS-->';
const JUCE_MARKER = '<!--CZ101_JUCE_TAG-->';
const JUCE_RE = /<script\s+src="juce\.js"\s*><\/script>/;

// Los `<link rel=stylesheet>` del HTML de ORIGEN, capturados tal cual en la fase
// `pre` (con su texto y su orden) para reponerlos en la fase `post`. Variable
// de modulo a proposito: un build por proceso, y asi el `pre` y el `post` se
// pasan la lista sin ir a buscarla al disco en la fase final.
let capturedStyleLinks = '';

export default defineConfig({
  root: webUiRoot,
  base: './',
  plugins: [
    {
      // Fase PRE: la hoja y el tag de JUCE salen del grafo. Los `<link>` se
      // guardan TAL CUAL (con su texto y su orden) en un testigo; en la fase
      // post se vuelven a escribir con la ruta del bundle.
      name: 'cz101-hide-from-graph',
      transformIndexHtml: {
        order: 'pre',
        handler: (html) => {
          const stripped = html.replace(/<link[^>]*rel="stylesheet"[^>]*>/g, (tag) => {
            capturedStyleLinks += tag;
            return '';
          });

          return stripped
            .replace(JUCE_RE, JUCE_MARKER)
            .replace('</head>', `${STYLE_MARKER}</head>`);
        },
      },
    },
    {
      name: 'cz101-restore-into-bundle',
      enforce: 'post',
      transformIndexHtml: {
        order: 'post',
        handler: (html) => html
          // La hoja vuelve donde estaba (al final de <head>, que es donde
          // estaba en el origen) con la ruta del bundle y EN SU ORDEN. Cada
          // parcial conserva su texto: solo cambia el prefijo del href.
          .replace(
            STYLE_MARKER,
            capturedStyleLinks
              .split('<link')
              .filter((chunk) => chunk !== '')
              .map((chunk) => `<link${chunk}`.replace('href="./src/styles/', 'href="./styles/'))
              .join('\n  ') + '\n  ',
          )
          .replace(JUCE_MARKER, '<script src="juce.js"></script>'),
      },
    },
    {
      // Los parciales verbatim y el estatico que la pagina pide por URL
      // (`wasm/`, `presets/`, `assets/`, `icons/`, manifest, sw.js, PNG). El
      // binario del motor (`cz101_dsp.wasm`) NO esta en el arbol —lo produce
      // el build de emscripten—: si un dia aparece al lado del pegamento, esta
      // copia lo arrastra con el, que es lo que necesita el pegamento para
      // resolverlo con `new URL(..., import.meta.url)`.
      name: 'cz101-static-copy',
      apply: 'build',
      closeBundle() {
        const dist = path.join(webUiRoot, 'dist');
        const styles = path.join(webUiRoot, 'src', 'styles');
        const copied = [];

        fs.mkdirSync(path.join(dist, 'styles'), { recursive: true });

        for (const entry of fs.readdirSync(styles)) {
          if (!entry.endsWith('.css')) continue;
          fs.copyFileSync(path.join(styles, entry), path.join(dist, 'styles', entry));
          copied.push(`styles/${entry}`);
        }

        for (const dir of ['wasm', 'presets', 'assets', 'icons']) {
          const from = path.join(webUiRoot, dir);

          if (!fs.existsSync(from)) continue;
          fs.cpSync(from, path.join(dist, dir), { recursive: true, verbatimSymlinks: true });
          copied.push(`${dir}/`);
        }

        for (const file of ['manifest.webmanifest', 'sw.js', 'bg.png', 'czwavs.png']) {
          const from = path.join(webUiRoot, file);

          if (!fs.existsSync(from)) continue;
          fs.copyFileSync(from, path.join(dist, file));
          copied.push(file);
        }

        console.log(`[cz101-static-copy] ${copied.length} entradas copiadas a dist/ (${copied.slice(0, 4).join(', ')}...)`);
      },
    },
  ],
  build: {
    outDir: 'dist',
    emptyOutDir: true,
    // Sin inlining base64: el proveedor nativo sirve cada fichero por ruta.
    assetsInlineLimit: 0,
    rollupOptions: {
      output: {
        entryFileNames: 'assets/[name].js',
        chunkFileNames: 'assets/[name].js',
        assetFileNames: 'assets/[name][extname]',
      },
    },
  },
});
