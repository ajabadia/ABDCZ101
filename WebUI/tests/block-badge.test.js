import { describe, it, expect } from 'vitest';
import { readFileSync } from 'node:fs';
import { fileURLToPath } from 'node:url';
import { dirname, join } from 'node:path';

import {
  badgeText,
  cellsOf,
  countTouched,
  factoryValueOf,
  normalizedValueOf,
  rawValueOf,
} from '../src/ui/blockBadges.js';
import { PARAM_MAP } from '../src/contracts/registry.gen.js';

const __dirname = dirname(fileURLToPath(import.meta.url));
// Las fuentes llevan CRLF: los escaneos las normalizan a LF antes de anclar
// (mismo truco que drag-drop-preset-memory / factory-modern-keytrack).
const read = (relative) => readFileSync(join(__dirname, '..', relative), 'utf8').replace(/\r\n/g, '\n');
const indexHtml = read('index.html');
const appJs = read('src/app.js');
const drawerJs = read('src/ui/blockDrawer.js');
const panelsCss = read('src/styles/panels.css');

/** Celda falsa: el adaptador solo mira id, tagName, value y la clase `active`. */
const cell = (id, value, active = false) => ({
  id,
  tagName: 'INPUT',
  value,
  classList: { contains: (name) => name === 'active' && active },
});

/** Sección falsa: el adaptador solo la interroga con querySelectorAll('[id]'). */
const section = (...nodes) => ({ querySelectorAll: () => nodes });

describe('distintivo vivo del cajon de bloques: celdas tocadas por bloque', () => {
  it('compara en el espacio NORMALIZADO, con la misma regla de skew que applyParameterToUI', () => {
    // MODERN_LPF_CUTOFF lleva skew 0.3 (rango 20..20000): su control escribe el
    // valor YA normalizado, asi que 0.5 es la mitad del recorrido del motor.
    // Leido como crudo, rawToNormalized(0.5) seria pow de un numero negativo
    // (0.5 esta por debajo del minimo 20) y devolveria NaN.
    expect(normalizedValueOf('MODERN_LPF_CUTOFF', 0.5)).toBe(0.5);
    expect(normalizedValueOf('MODERN_LPF_CUTOFF', 1)).toBe(1);

    // Sin skew el control lleva el CRUDO y se convierte con el mapeo.
    expect(normalizedValueOf('MASTER_VOLUME', 0.5)).toBe(0.5);
    expect(normalizedValueOf('MASTER_VOLUME', 0)).toBe(0);

    // Un id fuera del contrato no se inventa: NaN, y la celda no cuenta.
    expect(normalizedValueOf('MACRO_CONTOUR', 1)).toBeNaN();
    expect(Number.isNaN(normalizedValueOf('MASTER_VOLUME', Number.NaN))).toBe(true);
  });

  it('el default de fabrica se compara con el MISMO mapeo que el valor', () => {
    // El cutoff del LPF tiene default 20000 = normalizado 1.0: un control en 1.0
    // esta en fabrica y uno en 0.98 ya esta tocado.
    const factory = factoryValueOf('MODERN_LPF_CUTOFF');

    expect(factory).toBe(1);

    const cells = [
      { id: 'MODERN_LPF_CUTOFF', normalized: factory },
      { id: 'MODERN_LPF_CUTOFF', normalized: factory - 0.02 },
    ];

    expect(countTouched(cells)).toEqual({ touched: 1, total: 2 });
  });

  it('REGRESION: la fabrica y el control no llevan la misma unidad', () => {
    // MODERN_HPF_CUTOFF: rango 20..10000 con skew 0.3 y default 20, o sea
    // normalizado 0.0. Su CONTROL lleva el valor normalizado, asi que un
    // control en 0.0 esta en fabrica. Con el default pasado por la regla del
    // control (20 -> 1.0) el distintivo de este bloque salia 1/1 al abrir,
    // senalando como tocado un control que nadie habia movido.
    expect(factoryValueOf('MODERN_HPF_CUTOFF')).toBe(0);
    expect(countTouched([{ id: 'MODERN_HPF_CUTOFF', normalized: 0 }])).toEqual({ touched: 0, total: 1 });
    expect(countTouched([{ id: 'MODERN_HPF_CUTOFF', normalized: 0.5 }])).toEqual({ touched: 1, total: 1 });

    // Y al reves: un control con skew en su tope SI esta en fabrica.
    expect(factoryValueOf('MODERN_LPF_CUTOFF')).toBe(1);
    expect(countTouched([{ id: 'MODERN_LPF_CUTOFF', normalized: 1 }])).toEqual({ touched: 0, total: 1 });
  });

  it('el margen absorbe el redondeo del slider pero no un gesto', () => {
    // 1/8192 es medio paso de la rejilla de 1/4096 del cable: generoso con dos
    // decimales escritos, estricto con un movimiento de verdad.
    const cells = [{ id: 'MASTER_VOLUME', normalized: 0.5 }];
    const factory = factoryValueOf('MASTER_VOLUME');

    expect(countTouched([{ ...cells[0], normalized: factory }])).toEqual({ touched: 0, total: 1 });
    expect(countTouched([{ id: 'MASTER_VOLUME', normalized: factory + 1 / 16384 }]))
      .toEqual({ touched: 0, total: 1 });
    expect(countTouched([{ id: 'MASTER_VOLUME', normalized: factory - 0.01 }]))
      .toEqual({ touched: 1, total: 1 });
  });

  it('cuenta las celdas CON contrato y deja fuera lo que el contrato no judgea', () => {
    // MACRO_CONTOUR no esta en el registro (no hay default con el que comparar) y
    // `val-MASTER_VOLUME` es un readout, no un control: ninguno cuenta.
    const cells = cellsOf(section(
      cell('MASTER_VOLUME', '0.5'),
      // En su default de fabrica: cuenta en el total y NO en el numerator.
      cell('MASTER_TUNE', String(PARAM_MAP.get('MASTER_TUNE').default)),
      { id: 'MACRO_CONTOUR', tagName: 'BUTTON', value: undefined, classList: { contains: () => false } },
      { id: 'val-MASTER_VOLUME', tagName: 'SPAN', value: undefined, classList: { contains: () => false } },
    ));

    expect(cells.map((entry) => entry.id)).toEqual(['MASTER_VOLUME', 'MASTER_TUNE']);
    expect(countTouched(cells)).toEqual({ touched: 1, total: 2 });

    // Sin seccion (el cajon cerrado) no hay celdas ni distintivo.
    expect(cellsOf(null)).toEqual([]);
  });

  it('un toggle es un <button> con clase active, no un input con valor', () => {
    const toggle = (on) => ({ tagName: 'BUTTON', value: undefined, classList: { contains: (name) => name === 'active' && on } });

    expect(rawValueOf(toggle(false))).toBe(0);
    expect(rawValueOf(toggle(true))).toBe(1);
    expect(Number.isNaN(rawValueOf({ tagName: 'CANVAS', value: undefined, classList: { contains: () => false } }))).toBe(true);
    expect(Number.isNaN(rawValueOf(null))).toBe(true);
  });

  it('el texto del distintivo es la fraccion, y vacio si la seccion no tiene celdas', () => {
    expect(badgeText({ touched: 3, total: 12 })).toBe('3/12');
    expect(badgeText({ touched: 0, total: 4 })).toBe('0/4');
    expect(badgeText({ touched: 0, total: 0 })).toBe('');
  });

  it('la plaza del distintivo esta en la cabecera y es region viva', () => {
    expect(indexHtml).toContain('id="block-drawer-badge"');
    // El texto lo escribe el modulo; el markup solo pone la plaza y la region.
    expect(indexHtml).toMatch(/<span class="block-drawer-badge" id="block-drawer-badge" role="status"/);
    expect(indexHtml).toMatch(/aria-live="polite" aria-atomic="true"><\/span>/);
    // Y vive en la cabecera, junto al titulo y al ✕.
    const header = indexHtml.split('<div class="block-drawer-header">')[1]?.split('</div>')[0] ?? '';
    expect(header).toContain('block-drawer-title');
    expect(header).toContain('block-drawer-badge');
    expect(header).toContain('btn-block-drawer-close');
  });

  it('el cajon repinta el distintivo al abrir (antes del foco) y lo vacia al cerrar', () => {
    // El contrato de foco sigue siendo el compartido (overlayFocus), no una copia.
    expect(drawerJs).toContain("import { createOverlayFocus } from '@abdsynths/shared/components';");
    expect(drawerJs).not.toMatch(/function\s+trapFocus|addEventListener\('keydown',\s*trapTab/);

    // Al abrir: se repinta DESPUES de soltar el inert y ANTES de focusFirst, para
    // que la region viva y el primer control yaenzen el dato de la seccion nueva.
    const abrir = drawerJs.slice(drawerJs.indexOf('function openBlockDrawer'), drawerJs.indexOf('function closeBlockDrawer'));
    expect(abrir.indexOf('currentSectionId = sectionId;')).toBeGreaterThan(-1);
    expect(abrir.indexOf('badge.refresh();')).toBeGreaterThan(abrir.indexOf('releaseInert()'));
    expect(abrir.indexOf('badge.refresh();')).toBeLessThan(abrir.indexOf('focusFirst()'));

    // Al cerrar: vacio, y la seccion olvidada (cerrado no se dice nada de ninguna).
    const cerrar = drawerJs.slice(drawerJs.indexOf('function closeBlockDrawer'), drawerJs.indexOf('function toggleBlockDrawer'));
    expect(cerrar).toContain('currentSectionId = null;');
    expect(cerrar).toContain('badge.clear();');
    expect(cerrar.indexOf('badge.clear();')).toBeLessThan(cerrar.indexOf("classList.remove('open')"));

    // El repintado sale del cajon para que app.js lo despierte.
    expect(drawerJs).toContain('refreshBlockBadge: badge.refresh,');
  });

  it('los valores que llegan SIN evento (preset, banco, motor) repintan por el embudo', () => {
    // applyParameterToUI es la puerta publica: pinta la celda y repinta el
    // distintivo, porque la delegacion del cajon solo oye input/change.
    expect(appJs).toMatch(/function applyParameterToUI\(paramId, normalizedValue\) \{\s*\n\s*applyParameterToControl\(paramId, normalizedValue\);\s*\n\s*blockBadgeApi\?\.refresh\(\);/);
    expect(appJs).toMatch(/function applyParameterToControl\(paramId, normalizedValue\) \{/);
    expect(appJs).toContain('blockBadgeApi = { refresh: refreshBlockBadge };');
  });

  it('el distintivo tiene hoja de estilo propia, con la voz de los badges de la familia', () => {
    expect(panelsCss).toContain('.block-drawer-badge {');
    expect(panelsCss).toMatch(/\.block-drawer-badge:empty \{\s*\n\s*display: none;/);
    // El margen auto deja el ✕ a la derecha (el header es flex con space-between).
    expect(panelsCss).toMatch(/\.block-drawer-badge \{[^}]*margin-right: auto;/);
  });

  it('casi todas las secciones del cajon tienen celdas que el contrato puede juzgar', () => {
    // El distintivo se lee del DOM VIVO, no del HTML: la seccion `mod` (la
    // matriz de modulacion) construye sus 24 celdas en tiempo de ejecucion, asi
    // que en el HTML estatico no aparece ninguna y aun asi tiene distintivo.
    const body = indexHtml.split('id="block-drawer-body"')[1] ?? '';
    const sections = [...indexHtml.matchAll(/data-drawer-section="([^"]+)"/g)]
      .map((match) => match[1])
      .filter((id) => id !== '...' && body.includes(`data-drawer-section="${id}"`));
    const withCells = sections.filter((id) => {
      const start = body.indexOf(`data-drawer-section="${id}"`);
      const rest = body.slice(start);
      const next = rest.slice(1).search(/data-drawer-section="/);
      const chunk = next < 0 ? rest : rest.slice(0, next + 1);
      const ids = [...new Set([...chunk.matchAll(/id="([A-Za-z0-9_]+)"/g)].map((match) => match[1]))];

      return ids.some((pid) => PARAM_MAP.has(pid));
    });

    expect(sections.length).toBe(17);
    expect(withCells.length).toBe(16);
    // La matriz se construye en runtime: por eso el HTML no la ve.
    expect(sections).toContain('mod');
    expect(withCells).not.toContain('mod');
  });
});
