// ─── Distintivo VIVO del cajon de bloques (parametros tocados por bloque) ───
// El cajon de bloques es UNO para toda la maquina: muchas secciones, una
// visible cada vez. Este modulo le da a la cabecera el dato VIVO de la seccion
// abierta: cuantas de sus celdas se han apartado del default de FABRICA del
// contrato generado (registry.gen.js). Es la misma pregunta que se hace el
// distintivo `touched` de NEURONiK, con el contrato de este proyecto.
//
// LA VERDAD ES LA DEL MOTOR: los dos lados se comparan ya NORMALIZADOS con el
// mapeo del contrato (su skew incluido), no en crudo. Un parametro cuenta como
// tocado cuando lo que OYE el motor se aparta de la fabrica, no cuando el HTML
// escribe otro redondeo.
//
// Y AQUI ESTA LA TRAMPA (medida: con el default pasado por la regla del DOM, el
// corte del paso alto salia 1/1 al abrir, sin que nadie lo hubiera tocado): los
// dos lados NO llevan la misma unidad. El CONTROL lleva el valor ya normalizado
// si el parametro tiene skew (es lo que escribe `applyParameterToUI` y lo que lee
// el motor), y el DEFAULT del registro llega siempre en CRUDO, skew o no. Por eso
// son dos funciones y no una: `normalizedValueOf` para el control,
// `factoryValueOf` para la fabrica.
//
// FUERA DEL CONTRATO NO SE CUENTA: un id que el registro no declara no trae
// default con el que compararse (el `MACRO_CONTOUR` de la ficha VOICE es el
// caso) y un control sin valor legible (un canvas, un readout) tampoco. No se
// inventa un default para poder marcarlo como tocado.

import { PARAM_MAP, rawToNormalized, usesSkew } from '../contracts/registry.gen.js';

/**
 * Margen de "tocado": medio paso de la rejilla de 1/4096 con la que viaja el
 * cable es generoso con el redondeo de los sliders (escriben 2 decimales) y
 * estricto con el gesto. Mismo margen que el distintivo `touched` de NEURONiK.
 */
const TOUCH_EPSILON = 1 / 8192;

const clamp01 = (value) => Math.min(1, Math.max(0, value));

/**
 * Valor CRUDO de un control del cajon, o NaN si no hay nada legible.
 * Los toggles son `<button>` con clase `active` (asi los pinta
 * `applyParameterToUI`), no un input con `value`.
 *
 * @param {Element} element
 * @returns {number}
 */
export function rawValueOf(element) {
  if (!element) return Number.NaN;
  if (element.tagName === 'BUTTON') return element.classList.contains('active') ? 1 : 0;

  const value = Number.parseFloat(element.value);

  return Number.isFinite(value) ? value : Number.NaN;
}

/**
 * Valor NORMALIZADO de un CONTROL, en el espacio en el que vive. Con skew el
 * control lleva el valor ya normalizado (lo que escribe `applyParameterToUI` y
 * lo que lee el motor); sin skew lleva el crudo y se convierte aqui. Sin
 * normalizar, un cutoff a 20 Hz y otro a 20000 en un rango 20..20000 con skew
 * 0.3 no son el mismo numero, y el distintivo contaria como tocado un control
 * que el motor ve en su sitio.
 *
 * @param {string} id
 * @param {number} raw  valor tal cual vive en el control
 * @returns {number} 0..1, o NaN si no hay valor legible
 */
export function normalizedValueOf(id, raw) {
  const spec = PARAM_MAP.get(id);

  if (!spec || !Number.isFinite(raw)) return Number.NaN;
  if (usesSkew(spec)) return clamp01(raw);

  return rawToNormalized(id, raw);
}

/**
 * Las celdas de una seccion que el CONTRATO puede juzgar: `{ id, normalized }`
 * con el valor YA normalizado. Un id fuera del registro o sin default no
 * cuenta, y tampoco un control sin valor.
 *
 * @param {Element|null} section  el `.drawer-section[data-drawer-section]`
 * @returns {Array<{id: string, normalized: number}>}
 */
export function cellsOf(section) {
  if (!section) return [];

  const cells = [];

  for (const element of section.querySelectorAll('[id]')) {
    const spec = PARAM_MAP.get(element.id);

    if (!spec || spec.default === undefined) continue;

    const normalized = normalizedValueOf(element.id, rawValueOf(element));

    if (Number.isFinite(normalized)) cells.push({ id: element.id, normalized });
  }

  return cells;
}

/**
 * La FABRICA de un parametro, normalizada. El `default` del registro es siempre
 * un valor CRUDO (asi lo declara la APVTS), con skew o sin el, asi que pasa por
 `rawToNormalized` siempre: pasarlo por la regla del control daria un numero
 equivocado (y un distintivo que senala como tocado lo que esta en fabrica).
 *
 * @param {string} id
 * @returns {number} 0..1, o NaN si el parametro no esta en el contrato
 */
export function factoryValueOf(id) {
  const spec = PARAM_MAP.get(id);

  if (!spec || spec.default === undefined) return Number.NaN;

  return rawToNormalized(id, spec.default);
}

/**
 * Cuantas de esas celdas se han apartado de la fabrica. Los dos lados van ya
 * normalizados (el control por su regla, la fabrica por la del contrato), asi
 * que la comparacion es entre numeros que el motor ya entendu.
 *
 * @param {Array<{id: string, normalized: number}>} cells
 * @returns {{ touched: number, total: number }}
 */
export function countTouched(cells) {
  let touched = 0;

  for (const { id, normalized } of cells) {
    const factory = factoryValueOf(id);

    if (!Number.isFinite(factory)) continue;


    if (Math.abs(normalized - factory) > TOUCH_EPSILON) touched += 1;
  }

  return { touched, total: cells.length };
}

/** El texto del distintivo: `3/12`, o cadena vacia si la seccion no tiene celdas. */
export function badgeText({ touched, total }) {
  return total === 0 ? '' : `${touched}/${total}`;
}

/**
 * El distintivo, montado sobre el `<span>` que pinta la cabecera del cajon.
 *
 * El CONTRATO DE FOCO es el de la familia (overlayFocus.js) y el distintivo se
 * respeta desde dentro: el cajon cerrado lleva `inert` + `aria-hidden`, asi que
 * un distintivo con valor viejo no se anuncia a nadie; al abrir lo repinta el
 * propio `openBlockDrawer` ANTES de que entre el foco (la region viva dice la
 * verdad de la seccion que se esta anunciando) y al cerrar se vacia. Los gestos
 * se escuchan por delegacion en el cuerpo del cajon (`input` y `change`), que
 * es por donde pasan tanto los controles de la seccion como los slots de la
 * matriz de modulacion (se siembran despachando `input` + `change`).
 *
 * @param {object} deps
 * @param {Element|null} deps.badge                 el `<span>` del distintivo
 * @param {Element|null} deps.body                  #block-drawer-body
 * @param {() => (string|null)} deps.getSectionId   que seccion esta abierta
 * @param {() => boolean} [deps.isOpen]             el cajon esta abierto?
 * @returns {{ refresh: Function, clear: Function, destroy: Function }}
 */
export function createBlockBadge({ badge, body, getSectionId, isOpen = () => true }) {
  if (!badge) return { refresh: () => {}, clear: () => {}, destroy: () => {} };

  /** Repinta el distintivo con la seccion abierta. Sin seccion, lo vacia. */
  function refresh() {
    const sectionId = getSectionId();

    if (!sectionId) {
      clear();

      return;
    }

    const section = body?.querySelector(`.drawer-section[data-drawer-section="${sectionId}"]`);
    const counts = countTouched(cellsOf(section));

    badge.textContent = badgeText(counts);
    badge.title = describe(counts);
    // Region viva y POLITA (la lleva el markup): el numero se anuncia al abrir
    // el cajon, no en cada celda de una carga de preset, que llega en rafaga.
    badge.setAttribute('aria-label', describe(counts));
  }

  /** Vacia el distintivo: el cajon cerrado no dice nada de ninguna seccion. */
  function clear() {
    badge.textContent = '';
    badge.removeAttribute('title');
    badge.removeAttribute('aria-label');
  }

  const onChange = () => {
    if (isOpen()) refresh();
  };

  body?.addEventListener('input', onChange);
  body?.addEventListener('change', onChange);

  return {
    refresh,
    clear,
    destroy() {
      body?.removeEventListener('input', onChange);
      body?.removeEventListener('change', onChange);
    },
  };
}

/** La frase que va en el title y en la etiqueta accesible (el texto visible es la fraccion). */
function describe({ touched, total }) {
  return `${touched} de ${total} parámetros tocados de fábrica`;
}
