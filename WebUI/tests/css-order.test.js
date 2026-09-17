import { describe, it, expect } from 'vitest';
import { readFileSync } from 'node:fs';
import { fileURLToPath } from 'node:url';
import { dirname, join } from 'node:path';
import { EXPECTED_ORDER, validateCssOrder } from '../../scripts/validate_css_order.js';

const __dirname = dirname(fileURLToPath(import.meta.url));
const ROOT = join(__dirname, '..', '..');

describe('CSS partial order (index.html ↔ main.css)', () => {
  it('both entry points match the canonical order of 11 partials', () => {
    const { ok, problems } = validateCssOrder(ROOT);
    expect(problems, problems.join('\n')).toEqual([]);
    expect(ok).toBe(true);
  });

  it('canonical list has no duplicates and covers the styles directory', () => {
    expect(new Set(EXPECTED_ORDER).size).toBe(EXPECTED_ORDER.length);
    expect(EXPECTED_ORDER).toContain('base.css');
    expect(EXPECTED_ORDER).toContain('themes.css');
  });

  it('every @import in main.css resolves to a real partial on disk', () => {
    const mainCss = readFileSync(join(ROOT, 'WebUI', 'src', 'styles', 'main.css'), 'utf8');
    const imported = [...mainCss.matchAll(/@import\s+['"]\.\/([^'"]+\.css)['"]\s*;/g)].map(m => m[1]);
    expect(imported.length).toBeGreaterThan(0);
    for (const f of imported) {
      expect(() => readFileSync(join(ROOT, 'WebUI', 'src', 'styles', f))).not.toThrow();
    }
  });
});
