import { describe, it, expect } from 'vitest';
import { readFileSync } from 'node:fs';
import { fileURLToPath } from 'node:url';
import { dirname, join } from 'node:path';
import { EXPECTED_ORDER, validateCssOrder, validateDistOrder } from '../../scripts/validate_css_order.js';

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

  it('el BUNDLE lleva los once parciales en el mismo orden canonico', () => {
    // El bundle copia los parciales verbatim a `dist/styles/` y reescribe sus
    // `<link>` (ver WebUI/vite.build.config.js). El validador del dist usa el
    // MISMO EXPECTED_ORDER: si el empaquetador fusionase los once en un
    // fichero o los reordenase, el bundle dejaria de ser la pagina que se ha
    // estado viendo en desarrollo, y aqui se ve sin abrir el plugin.
    const links = (prefix) => EXPECTED_ORDER
      .map((file) => `<link rel="stylesheet" href="${prefix}${file}">`)
      .join('\n');

    expect(validateDistOrder(links('./styles/')).ok).toBe(true);

    // Orden cambiado: falla y lo dice.
    const swapped = EXPECTED_ORDER.map((file, i) => (i === 0 ? EXPECTED_ORDER[1] : EXPECTED_ORDER[0]));
    const bad = validateDistOrder(swapped.map((file) => `<link rel="stylesheet" href="./styles/${file}">`).join('\n'));

    expect(bad.ok).toBe(false);
    expect(bad.problems[0]).toContain('diverges from canonical');

    // Y un bundle que se haya comido la hoja entera, tambien.
    expect(validateDistOrder('<link rel="stylesheet" href="./assets/index.css">').ok).toBe(false);
  });

  it('el build de la WebUI no deja que Vite se lleve la cascada', () => {
    // Los `<link>` de la hoja se apartan del grafo con un testigo y vuelven
    // DESPUES de que Vite escriba el HTML: es lo que garantiza que los once
    // parciales sigan siendo once ficheros en su orden (el riesgo propio de
    // empaquetar esta pagina). Sin esto, Vite los fusiona en un `index.css`.
    const config = readFileSync(join(ROOT, 'WebUI', 'vite.build.config.js'), 'utf8');

    expect(config).toContain("name: 'cz101-hide-from-graph'");
    expect(config).toContain("order: 'pre'");
    expect(config).toContain("name: 'cz101-restore-into-bundle'");
    expect(config).toContain("order: 'post'");
    // El `juce.js` del WebView vuelve con su URL original, no con la del testigo.
    expect(config).toContain(".replace(JUCE_MARKER, '<script src=\"juce.js\"></script>')");
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
