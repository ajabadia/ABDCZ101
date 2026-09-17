/* sw.js — ABD CZ-101 WebUI service worker (PWA).
 *
 * Precache strategy:
 *  - On install we cache the app shell: index.html, manifest, icons, and the
 *    11 CSS partials (plus the production bundle dist/styles.min.css when the
 *    build has been run). bg.png is large (8 MB) and cached as well so the app
 *    works offline.
 *  - Runtime: stale-while-revalidate for assets (fast first paint, updates in
 *    the background); network-first with cache fallback for navigations so
 *    index.html edits are never stuck behind a stale cache.
 */

const VERSION = 'cz101-v1';
const CACHE_NAME = `cz101-shell-${VERSION}`;

// The 11 CSS partials — keep in sync with scripts/validate_css_order.js
// (EXPECTED_ORDER) and WebUI/src/styles/main.css.
const CSS_PARTIALS = [
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
].map(f => `./src/styles/${f}`);

// App shell — everything needed for the first offline paint.
const PRECACHE = [
  './',
  './index.html',
  './manifest.webmanifest',
  './icons/icon-192.png',
  './icons/icon-512.png',
  './bg.png',
  ...CSS_PARTIALS,
];

self.addEventListener('install', (event) => {
  event.waitUntil(
    (async () => {
      const cache = await caches.open(CACHE_NAME);
      // Core shell first; missing entries (e.g. dist/ when not built) must not
      // fail the whole install, so add them one by one.
      for (const url of PRECACHE) {
        try { await cache.add(url); } catch (_) { /* optional asset */ }
      }
      // Production bundle — only present after `npm run build:css`.
      for (const url of ['./dist/styles.min.css', './dist/bg.png']) {
        try { await cache.add(url); } catch (_) { /* not built */ }
      }
      await self.skipWaiting();
    })()
  );
});

self.addEventListener('activate', (event) => {
  event.waitUntil(
    (async () => {
      const keys = await caches.keys();
      await Promise.all(keys.filter(k => k !== CACHE_NAME).map(k => caches.delete(k)));
      await self.clients.claim();
    })()
  );
});

self.addEventListener('fetch', (event) => {
  const req = event.request;
  if (req.method !== 'GET') return;

  const url = new URL(req.url);
  if (url.origin !== self.location.origin) return; // skip cross-origin (fonts, wasm CDN)

  // Navigations: network-first, fall back to cached index.html offline.
  if (req.mode === 'navigate') {
    event.respondWith(
      (async () => {
        try {
          const fresh = await fetch(req);
          const cache = await caches.open(CACHE_NAME);
          cache.put(req, fresh.clone());
          return fresh;
        } catch (_) {
          const cached = await caches.match(req);
          return cached || (await caches.match('./index.html'));
        }
      })()
    );
    return;
  }

  // Assets: stale-while-revalidate.
  event.respondWith(
    (async () => {
      const cache = await caches.open(CACHE_NAME);
      const cached = await cache.match(req);
      const network = fetch(req).then((res) => {
        if (res.ok) cache.put(req, res.clone());
        return res;
      }).catch(() => null);
      return cached || network;
    })()
  );
});
