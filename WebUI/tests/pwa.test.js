import { describe, it, expect } from 'vitest';
import { readFileSync, existsSync } from 'node:fs';
import { fileURLToPath } from 'node:url';
import { dirname, join } from 'node:path';
import { EXPECTED_ORDER } from '../../scripts/validate_css_order.js';

const __dirname = dirname(fileURLToPath(import.meta.url));
const WEBUI = join(__dirname, '..');

const indexHtml = readFileSync(join(WEBUI, 'index.html'), 'utf8');
const swJs = readFileSync(join(WEBUI, 'sw.js'), 'utf8');
const manifestRaw = readFileSync(join(WEBUI, 'manifest.webmanifest'), 'utf8');
const manifest = JSON.parse(manifestRaw);

describe('PWA manifest + service worker', () => {
  it('index.html links the manifest and unregisters the service worker', () => {
    expect(indexHtml).toContain('rel="manifest" href="./manifest.webmanifest"');
    expect(indexHtml).toMatch(/navigator\.serviceWorker\.getRegistrations/);
  });

  it('manifest is valid JSON with app metadata and icons', () => {
    expect(manifest.name).toContain('CZ-101');
    expect(manifest.start_url).toBe('./index.html');
    expect(manifest.display).toBe('standalone');
    expect(manifest.icons.length).toBeGreaterThanOrEqual(2);
    for (const icon of manifest.icons) {
      expect(existsSync(join(WEBUI, icon.src.replace('./', '')))).toBe(true);
    }
  });

  it('sw.js precaches the 11 CSS partials (sync with EXPECTED_ORDER)', () => {
    for (const partial of EXPECTED_ORDER) {
      expect(swJs, `sw.js missing precache of ${partial}`).toContain(`'${partial}'`);
    }
    // the precache must map them under ./src/styles/
    expect(swJs).toContain(`.map(f => \`./src/styles/\${f}\`)`);
  });

  it('sw.js precaches the app shell (index, manifest, icons, bg)', () => {
    for (const asset of ['./index.html', './manifest.webmanifest', './icons/icon-192.png', './icons/icon-512.png', './bg.png']) {
      expect(swJs).toContain(`'${asset}'`);
    }
  });

  it('sw.js falls back to the cached index.html for offline navigations', () => {
    expect(swJs).toMatch(/req\.mode === 'navigate'/);
    expect(swJs).toContain("return cached || (await caches.match('./index.html'));");
  });
});
