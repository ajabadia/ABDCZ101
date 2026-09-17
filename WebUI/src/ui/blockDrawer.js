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
export function createBlockDrawer({ redrawEnvelopes }) {
  const blockDrawer = document.getElementById('block-drawer');
  const blockDrawerBackdrop = document.getElementById('block-drawer-backdrop');
  const blockDrawerTitle = document.getElementById('block-drawer-title');

  function openBlockDrawer(sectionId, title) {
    if (!blockDrawer) return;
    document.querySelectorAll('#block-drawer-body .drawer-section').forEach(sec => {
      sec.hidden = sec.dataset.drawerSection !== sectionId;
    });
    if (blockDrawerTitle && title) blockDrawerTitle.textContent = title;
    blockDrawer.classList.add('open');
    blockDrawer.setAttribute('aria-hidden', 'false');
    if (blockDrawerBackdrop) blockDrawerBackdrop.hidden = false;
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

  // Escape closes the drawer (native Escape key parity).
  document.addEventListener('keydown', (e) => {
    if (e.key === 'Escape') closeBlockDrawer();
  });

  return { openBlockDrawer, closeBlockDrawer, toggleBlockDrawer };
}
