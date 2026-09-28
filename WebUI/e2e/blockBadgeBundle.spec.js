/**
 * El distintivo VIVO del cajon de bloques, en la pagina de DESARROLLO y en el
 * BUNDLE, comparado.
 *
 * Lo que se compara, y por que cada cosa:
 *
 *   1. EL DATO: el texto de la cabecera del cajon y el de cada celda, en las dos
 *      paginas. El distintivo sale del DOM vivo y del contrato generado; si el
 *      bundle cambiase el orden de la cascada, se compararian celdas distintas
 *      (los toggles son `<button>.active`, los deslizadores tienen `value`: el
 *      orden de los estilos decide quien pinta, no quien se mide).
 *   2. LA PINTURA: la cabecera del cajon abierta, por hash. Mismo dato con
 *      distinta pintura es un bundle que no se puede enviar: es justo el fallo
 *      que un test de DOM no ve.
 *
 * Las dos paginas se abren con el MISMO viewport y la misma seccion, y se
 * recorre mas de una seccion porque el numero depende de cuantas celdas tenga
 * cada una (`8/12`, `2/4`...): con una sola no se sabe si el bundle pinta bien
 * por casualidad.
 *
 * `npm run bundle` lo ejecuta el webServer del config; aqui solo se supone que
 * `dist/` esta al dia (si no, el caso falla contra el bundle viejo, que es el
 * aviso que hace falta).
 */

import { expect, test } from '@playwright/test';
import { createHash } from 'node:crypto';

const DEV_URL = process.env.CZ101_DEV_URL ?? `http://localhost:${process.env.CZ101_DEV_PORT ?? 5236}/`;
const DIST_URL = process.env.CZ101_DIST_URL ?? `http://localhost:${process.env.CZ101_DIST_PORT ?? 5239}/`;

/** Secciones con celdas que el contrato puede juzgar, con su boton EDIT. */
const SECTIONS = [
  { button: '#btn-edit-voice', section: 'voice', title: 'VOICE ENGINE' },
  { button: '#btn-edit-ctrl', section: 'ctrl', title: 'PERFORMANCE CONTROLS' },
  { button: '#btn-edit-dco', section: 'dco-osc1', title: 'DCO OSCILLATOR 1' },
];

const hash = (buffer) => createHash('sha256').update(buffer).digest('hex').slice(0, 16);

/**
 * Abre la seccion y devuelve lo que hay que comparar: el distintivo, el
 * aria-label de la cabecera y el estado de cada celda visible (id + valor), que
 * es de donde sale el recuento.
 */
async function readoutOf(page, url, section) {
  const errors = [];

  page.on('pageerror', (error) => errors.push(String(error.message ?? error)));

  await page.goto(url, { waitUntil: 'load' });
  await page.locator(section.button).click();
  await expect(page.locator('#block-drawer')).toHaveClass(/open/);

  const readout = await page.evaluate((sectionId) => {
    const drawer = document.getElementById('block-drawer');
    const body = document.querySelector(`[data-drawer-section="${sectionId}"]`);
    const cells = [...(body?.querySelectorAll('[id]') ?? [])]
      .filter((node) => node.id && !node.id.startsWith('val-'))
      .map((node) => `${node.id}=${node.tagName === 'BUTTON' ? (node.classList.contains('active') ? 'on' : 'off') : node.value}`);

    return {
      title: document.getElementById('block-drawer-title').textContent.trim(),
      badge: document.getElementById('block-drawer-badge').textContent.trim(),
      badgeLabel: document.getElementById('block-drawer-badge').getAttribute('aria-label') ?? '',
      ariaHidden: drawer.getAttribute('aria-hidden'),
      cells: cells.join(' | '),
    };
  }, section.section);

  return { ...readout, errors };
}

test.describe('el distintivo de bloques, en dev y en el bundle', () => {
  for (const section of SECTIONS) {
    test(`${section.title}: el dato es el MISMO en dev y en dist`, async ({ page }) => {
      const dev = await readoutOf(page, DEV_URL, section);
      const dist = await readoutOf(page, DIST_URL, section);

      // Sin errores de JS en ninguna de las dos: un bundle con un import roto
      // deja la pagina en pie pero sin distintivo, que es el fallo que se busca.
      expect(dev.errors, `errores en dev: ${dev.errors.join('; ')}`).toEqual([]);
      expect(dist.errors, `errores en dist: ${dist.errors.join('; ')}`).toEqual([]);

      expect(dist).toEqual(dev);
      // Y no es un vacio por las dos partes: el distintivo es una fraccion
      // (`1/6` en VOICE ENGINE, que nace con una celda fuera de fabrica por el
      // LINE_SELECT), con la frase larga en el aria-label.
      expect(dev.badge).toMatch(/^\d+\/\d+$/);
      expect(dev.badgeLabel).toMatch(/^\d+ de \d+ /);
    });
  }

  test('la PINTURA de la cabecera del cajon es la misma en dev y en dist', async ({ page }) => {
    const shot = async (url) => {
      await page.goto(url, { waitUntil: 'load' });
      await page.locator('#btn-edit-dco').click();
      await expect(page.locator('#block-drawer')).toHaveClass(/open/);

      return hash(await page.locator('.block-drawer-header').screenshot());
    };

    const dev = await shot(DEV_URL);
    const dist = await shot(DIST_URL);

    // Hashes, no bytes: la codificacion PNG puede variar entre capturas sin que
    // la pintura cambie, y lo que se compara es "se ve igual".
    expect(dist, `cabecera distinta (dev ${dev} vs dist ${dist})`).toBe(dev);
  });

  const CANONICAL = [
    'base.css', 'lcd.css', 'panels.css', 'controls.css', 'keyboard.css',
    'envelopes.css', 'midi.css', 'menu.css', 'bankManager.css', 'overlays.css',
    'themes.css',
  ];

  /** Los `<link>` de hoja LOCAL que la pagina tiene cargados, en orden. */
  const stylesheetsOf = (page, url) => page.goto(url).then(() => page.evaluate(() => [...document.styleSheets]
    .map((sheet) => (sheet.href ?? '').split('/').pop())
    .filter((name) => name.endsWith('.css') && !name.includes('fonts.googleapis'))));

  test('la hoja llega en el MISMO orden en dev y en el bundle', async ({ page }) => {
    // La cascada es el riesgo propio de empaquetar: si el bundler fusionase los
    // parciales o reordenase los `<link>`, quien gana una regla pasaria a
    // depender del empaquetador, y en desarrollo no se veria. Se compara lo que
    // cada pagina TIENE cargado, no lo que dice un fichero.
    const dev = await stylesheetsOf(page, DEV_URL);
    const dist = await stylesheetsOf(page, DIST_URL);

    // Los once parciales, en orden canonico, en las dos.
    expect(dev).toEqual(CANONICAL);
    expect(dist.slice(0, CANONICAL.length)).toEqual(CANONICAL);

    // Y la hoja que el grafo de JS importa (`@abdsynths/midi-keyb/keyboard.css`):
    // en desarrollo la inyecta el cliente de Vite como `<style>` sin href, y en
    // el bundle sale como un `index.css` mas, AL FINAL. Mismo sitio en la
    // cascada, que es lo que hace comparables las dos paginas.
    expect(dist.slice(CANONICAL.length)).toEqual(['index.css']);
  });
});
