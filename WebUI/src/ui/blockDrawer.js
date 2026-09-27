// ─── Slide-out settings drawer (reusable per block) ───
// Each block keeps a compact title + EDIT button on the surface; its full
// controls live in a <div class="drawer-section" data-drawer-section="...">
// inside #block-drawer-body. openBlockDrawer(sectionId, title) shows the
// matching section and slides the panel in; closeBlockDrawer() collapses it.
// Extracted from app.js so the block panels (model/modes, DCO, LFO, filters,
// effects, envelopes, modulation) share one reusable drawer implementation.

/**
 * createBlockDrawer
 * @param {object} deps
 * @param {() => void} deps.redrawEnvelopes - re-crisps envelope canvases when
 *   the drawer makes them visible / resizes them.
 */
import { createOverlayFocus } from '@abdsynths/shared/components';
// El distintivo VIVO de la cabecera (celdas tocadas por bloque): cuenta sobre
// el contrato, no sobre el estado de este modulo.
import { createBlockBadge } from './blockBadges.js';

export function createBlockDrawer({ redrawEnvelopes }) {
  const blockDrawer = document.getElementById('block-drawer');
  const blockDrawerBackdrop = document.getElementById('block-drawer-backdrop');
  const blockDrawerTitle = document.getElementById('block-drawer-title');

  // 2026-09-27: el CONTRATO DE FOCO de la familia (overlayFocus.js, el mismo
  // del cajon compartido createDrawer): cerrado el cajon va `inert` +
  // `aria-hidden` (su contenido sigue en el DOM para el paint, pero fuera de
  // la tabulacion), al abrir el foco entra en el primer control de la seccion,
  // con el cajon abierto Tab no se escapa y al cerrar vuelve al disparador
  // (el boton EDIT que lo abrio). Sin #block-drawer en el DOM no hay contrato.
  const blockDrawerBody = document.getElementById('block-drawer-body');
  const focusContract = blockDrawer
    ? createOverlayFocus({
        root: blockDrawer,
        body: blockDrawerBody,
        isClosed: () => !blockDrawer.classList.contains('open'),
        onEscape: () => closeBlockDrawer(),
      })
    : null;
  focusContract?.attach();
  focusContract?.setInert();

  // Que seccion esta abierta lo sabe el cajon (el la cambia); el distintivo lo
  // lee por el getter. Se declara ANTES de la fabrica porque su closure lo
  // usa, y el modulo se monta una vez al cargar la pagina.
  let currentSectionId = null;
  const badge = createBlockBadge({
    badge: document.getElementById('block-drawer-badge'),
    body: blockDrawerBody,
    getSectionId: () => currentSectionId,
    isOpen: () => Boolean(blockDrawer) && blockDrawer.classList.contains('open'),
  });

  function openBlockDrawer(sectionId, title) {
    if (!blockDrawer) return;
    document.querySelectorAll('#block-drawer-body .drawer-section').forEach(sec => {
      sec.hidden = sec.dataset.drawerSection !== sectionId;
    });
    if (blockDrawerTitle && title) blockDrawerTitle.textContent = title;
    currentSectionId = sectionId;
    blockDrawer.classList.add('open');
    blockDrawer.setAttribute('aria-hidden', 'false');
    if (blockDrawerBackdrop) blockDrawerBackdrop.hidden = false;
    // Disparador primero (el activeElement de fuera aun es el boton EDIT) y
    // despues se suelta el inert; el foco entra en el primer control de la
    // seccion visible.
    focusContract?.rememberTrigger();
    focusContract?.releaseInert();
    // El distintivo se repinta ANTES de que entre el foco: la region viva
    // anuncia los datos de la seccion que se acaba de abrir, y el primer control
    // que busca el contrato de foco tiene que encontrarlos ya escritos.
    badge.refresh();
    focusContract?.focusFirst();
    // The envelope canvases just became visible at their real width: redraw so
    // the backing store matches the new display size (crisp, not upscaled).
    redrawEnvelopes();
    // The slide-in animation (transform 0.28s) can shift the layout afterwards
    // (scrollbar arrival, body width settle), leaving the store a few px off.
    // Re-crisp once the transition actually ends; fallback timer if it never
    // fires (e.g. reduced-motion / transitionend unsupported).
    if (blockDrawer) {
      const onEnvTransitionEnd = (e) => {
        if (e.target === blockDrawer && e.propertyName === 'transform') {
          redrawEnvelopes();
          blockDrawer.removeEventListener('transitionend', onEnvTransitionEnd);
        }
      };
      blockDrawer.addEventListener('transitionend', onEnvTransitionEnd);
      setTimeout(redrawEnvelopes, 350);
    }
  }

  // Keep envelope canvases crisp when the window (and drawer) is resized.
  let envResizeTimer = null;
  window.addEventListener('resize', () => {
    clearTimeout(envResizeTimer);
    envResizeTimer = setTimeout(redrawEnvelopes, 100);
  });

  function closeBlockDrawer() {
    if (!blockDrawer) return;
    // El distintivo se vacia CON el cajon: cerrado, el drawer lleva `inert` +
    // `aria-hidden` y no dice nada de ninguna seccion (tampoco a un lector de
    // pantalla, que no lo ve). Al reabrir se repinta con la seccion nueva.
    currentSectionId = null;
    badge.clear();
    // El foco sale ANTES del inert (hacer inerte un contenedor con el foco
    // dentro lo tiraria a <body>) y vuelve al boton EDIT que abrio el cajon.
    focusContract?.restoreFocus();
    focusContract?.setInert();
    blockDrawer.classList.remove('open');
    blockDrawer.setAttribute('aria-hidden', 'true');
    if (blockDrawerBackdrop) blockDrawerBackdrop.hidden = true;
  }

  function toggleBlockDrawer(sectionId, title) {
    if (!blockDrawer) return;
    if (blockDrawer.classList.contains('open') &&
        blockDrawerTitle && blockDrawerTitle.textContent === title) {
      closeBlockDrawer();
    } else {
      openBlockDrawer(sectionId, title);
    }
  }

  // --- Surface EDIT buttons (each block: title + edit) ---
  const wireEditButton = (btnId, sectionId, title) => {
    const btn = document.getElementById(btnId);
    if (btn) btn.addEventListener('click', () => toggleBlockDrawer(sectionId, title));
  };
  wireEditButton('btn-edit-sys', 'sys', 'SYSTEM');
  wireEditButton('btn-edit-voice', 'voice', 'VOICE ENGINE');
  wireEditButton('btn-edit-ctrl', 'ctrl', 'PERFORMANCE CONTROLS');
  wireEditButton('btn-edit-dco', 'dco-osc1', 'DCO OSCILLATOR 1');
  wireEditButton('btn-edit-dco-osc2', 'dco-osc2', 'DCO OSCILLATOR 2');
  wireEditButton('btn-edit-arp', 'arp', 'ARPEGGIATOR');
  wireEditButton('btn-edit-lfo', 'lfo', 'LFO / VIBRATO');
  wireEditButton('btn-edit-filter-lpf', 'filters-lpf', 'LOW-PASS FILTER');
  wireEditButton('btn-edit-filter-hpf', 'filters-hpf', 'HIGH-PASS FILTER');
  wireEditButton('btn-edit-fx-chorus', 'effects-chorus', 'CHORUS');
  wireEditButton('btn-edit-fx-drive', 'effects-drive', 'DRIVE');
  wireEditButton('btn-edit-fx-delay', 'effects-delay', 'DELAY');
  wireEditButton('btn-edit-fx-reverb', 'effects-reverb', 'REVERB');
  wireEditButton('btn-edit-mod', 'mod', 'MODULATION ROUTINGS');

  // Envelope drawers (one per envelope: DCA / DCW / PITCH), from the same
  // reusable drawer but each opens its own section.
  wireEditButton('btn-edit-env-dca', 'env-dca', 'DCA ENVELOPE');
  wireEditButton('btn-edit-env-dcw', 'env-dcw', 'DCW ENVELOPE');
  wireEditButton('btn-edit-env-pitch', 'env-pitch', 'PITCH ENVELOPE');

  const btnDrawerClose = document.getElementById('btn-block-drawer-close');
  if (btnDrawerClose) btnDrawerClose.addEventListener('click', closeBlockDrawer);
  if (blockDrawerBackdrop) blockDrawerBackdrop.addEventListener('click', closeBlockDrawer);

  // Escape closes the drawer (native Escape key parity) — lo lleva el
  // contrato compartido: abierto la pide, cerrado no hace nada.
  return {
    openBlockDrawer,
    closeBlockDrawer,
    toggleBlockDrawer,
    // Repintado desde fuera: los valores llegan al DOM por `applyParameterToUI`
    // (preset, banco, motor, INIT) sin evento, y ahi no hay delegacion que lo
    // escuche. app.js lo llama desde ese mismo embudo.
    refreshBlockBadge: badge.refresh,
  };
}
