import { describe, it, expect } from 'vitest';
import { readFileSync } from 'node:fs';
import { fileURLToPath } from 'node:url';
import { dirname, join } from 'node:path';

const __dirname = dirname(fileURLToPath(import.meta.url));
const indexHtml = readFileSync(join(__dirname, '..', 'index.html'), 'utf8');
const appJs = readFileSync(join(__dirname, '..', 'src', 'app.js'), 'utf8');
// The envelope editor internals (state, canvas drawing, controls, undo-arming)
// live in src/ui/envelopeEditor.js after the app.js split; the wiring
// (drawer EDIT buttons, openBlockDrawer redraw, undo snapshots) stays in app.js.
const envJs = readFileSync(join(__dirname, '..', 'src', 'ui', 'envelopeEditor.js'), 'utf8');

// Cada envolvente (DCA/DCW/Pitch) vive en un panel y una sección del drawer
// independientes, con su propio juego de canvas + controles (IDs sufijados).
// Esto reemplaza el antiguo editor único con tabs compartidos.
describe('envelope editor split by type (DCA/DCW/Pitch)', () => {
  const types = ['dca', 'dcw', 'pitch'];
  const titles = { dca: 'DCA ENVELOPE', dcw: 'DCW ENVELOPE', pitch: 'PITCH ENVELOPE' };

  it('has one surface panel per type with an EDIT button', () => {
    types.forEach(t => {
      expect(indexHtml).toContain(`panel-env-${t}`);
      expect(indexHtml).toContain(titles[t]);
      expect(indexHtml).toContain(`id="btn-edit-env-${t}"`);
    });
  });

  it('has one drawer section per type', () => {
    types.forEach(t => {
      expect(indexHtml).toContain(`data-drawer-section="env-${t}"`);
    });
  });

  it('has a full editor (2 lines × canvas/sliders/node/sus/end/copy/paste) per type with suffixed ids', () => {
    const bases = ['canvas-env', 'slider-rate-env', 'slider-level-env', 'lbl-node-env', 'btn-sus-env', 'btn-end-env', 'btn-copy-env', 'btn-paste-env'];
    types.forEach(t => {
      [1, 2].forEach(line => {
        bases.forEach(base => {
          expect(indexHtml).toContain(`id="${base}-${t}-line${line}"`);
        });
      });
    });
  });

  it('no longer has the old shared tabs or shared ids', () => {
    expect(indexHtml).not.toContain('tab-env-dca');
    expect(indexHtml).not.toContain('tab-env-dcw');
    expect(indexHtml).not.toContain('tab-env-pitch');
    expect(indexHtml).not.toContain('id="canvas-env-line1"');
    expect(indexHtml).not.toContain('id="slider-rate-line1"');
  });

  it('the editor module parametrizes by type key instead of a global activeEnvType', () => {
    expect(envJs).toContain('const ENV_TYPES = [\'dca\', \'dcw\', \'pitch\']');
    expect(envJs).toContain('ENV_TYPE_INDEX');
    expect(envJs).toContain('ENV_ACCENTS');
    // No more global active env type / shared node selection
    expect(envJs).not.toContain('let activeEnvType');
    expect(envJs).not.toContain('let selectedNodeLine1');
    // All editor helpers take (typeKey, lineNum) and build suffixed ids
    expect(envJs).toContain('function drawEnvelope8(typeKey, lineNum)');
    expect(envJs).toContain('function updateEnvelopeControls(typeKey, lineNum)');
    expect(envJs).toContain('function setupCanvasInteractions(typeKey, lineNum)');
    expect(envJs).toContain('function setupEnvelopeControlsListeners(typeKey, lineNum)');
    expect(envJs).toContain('`canvas-env-${typeKey}-line${lineNum}`');
    expect(envJs).toContain('`slider-rate-env-${typeKey}-line${lineNum}`');
    // Every control lookup carries the -env- infix like the canvas id, so the
    // SUS/END/COPY/PASTE handlers and sliders actually bind to the buttons
    // (they silently no-op'd before when the ids didn't match).
    expect(envJs).not.toContain('`slider-rate-${typeKey}-line${lineNum}`');
    expect(envJs).toContain('`btn-copy-env-${typeKey}-line${lineNum}`');
    expect(envJs).toContain('`btn-paste-env-${typeKey}-line${lineNum}`');
    expect(envJs).toContain('`btn-sus-env-${typeKey}-line${lineNum}`');
    expect(envJs).toContain('`btn-end-env-${typeKey}-line${lineNum}`');
  });

  it('setup loops over all types × lines in the engine init', () => {
    expect(envJs).toContain('ENV_TYPES.forEach(typeKey => {');
    expect(envJs).toContain('setupCanvasInteractions(typeKey, 1);');
    expect(envJs).toContain('setupEnvelopeControlsListeners(typeKey, 2);');
    expect(envJs).toContain('updateEnvelopeControls(typeKey, 1);');
  });

  it('per-type node selection is kept in a selectedNode map', () => {
    expect(envJs).toContain('selectedNode[typeKey][lineKey]');
    expect(envJs).toContain('dca: { line1: 0, line2: 0 }');
    expect(envJs).toContain('pitch: { line1: 0, line2: 0 }');
  });

  it('drawer wiring maps the 3 EDIT buttons to their sections', () => {
    // The drawer EDIT-button wiring moved to the extracted blockDrawer module.
    const drawerJs = readFileSync(join(__dirname, '..', 'src', 'ui', 'blockDrawer.js'), 'utf8');
    expect(drawerJs).toContain("'btn-edit-env-dca', 'env-dca', 'DCA ENVELOPE'");
    expect(drawerJs).toContain("'btn-edit-env-dcw', 'env-dcw', 'DCW ENVELOPE'");
    expect(drawerJs).toContain("'btn-edit-env-pitch', 'env-pitch', 'PITCH ENVELOPE'");
  });

  it('drawEnvelope8 backs the canvas at display size × devicePixelRatio (no pixelation)', () => {
    // Regression: the fixed 180×50 backing store was stretched by flex-grow and
    // the browser upscaled it -> pixelated, worse on HiDPI. The backing store
    // must track the displayed rect × dpr and draw through a dpr transform.
    expect(envJs).toContain('const dpr = window.devicePixelRatio || 1;');
    expect(envJs).toContain('Math.round(cssW * dpr)');
    expect(envJs).toContain('ctx.setTransform(dpr, 0, 0, dpr, 0, 0);');
    // Canvas interaction (hit-testing) stays in CSS pixels via getBoundingClientRect.
    expect(envJs).toContain('const rect = canvas.getBoundingClientRect();');
  });

  it('redraws envelopes when the drawer opens / window resizes (fresh backing store)', () => {
    // The canvases are hidden until their drawer section opens; redrawing on
    // open + on resize re-sizes the backing store to the real displayed width.
    // The drawer-open redraw + resize listener moved to the blockDrawer module.
    const drawerJs = readFileSync(join(__dirname, '..', 'src', 'ui', 'blockDrawer.js'), 'utf8');
    expect(drawerJs).toContain('// The envelope canvases just became visible at their real width: redraw so');
    expect(drawerJs).toContain("window.addEventListener('resize', () => {",
    );
  });

  it('envelope edits push undo snapshots (drag/sliders/SUS/END/paste are custom controls)', () => {
    // Regression: the global undo-arm listeners only fire for ids in PARAM_MAP,
    // so envelope drags/sliders/SUS/END/paste were never undoable even though
    // capturePatchSnapshot/restorePatchSnapshot already handle envelopeState.
    // Each custom control must arm pushUndo() the same way.
    expect(envJs).toContain("if (!canvas.dataset.undoArmed) {");
    expect(envJs).toContain('canvas.dataset.undoArmed = \'1\';');
    expect(envJs).toContain('pushUndo();');
    expect(envJs).toContain("if (canvas.dataset.undoArmed) canvas.dataset.undoArmed = '';");
    expect(envJs).toContain("if (!el.dataset.undoArmed) {").length > 0;
    expect(envJs).toContain("sliderRate.addEventListener('change', () => { sliderRate.dataset.undoArmed = ''; });");
    expect(envJs).toContain("sliderLevel.addEventListener('change', () => { sliderLevel.dataset.undoArmed = ''; });");
    // Discrete actions arm once per click
    expect(envJs).toContain("btnSus.addEventListener('click', () => {").length > 0;
    expect(envJs).toContain("btnEnd.addEventListener('click', () => {").length > 0;
    expect(envJs).toContain('pushUndo();\n        env.rates = [...envClipboard.rates];');
    // capturePatchSnapshot already captures envelopeState for undo/redo parity
    const navbarJs = readFileSync(join(__dirname, '..', 'src', 'ui', 'navbar.js'), 'utf8');
    expect(navbarJs).toContain('envelopes: JSON.parse(JSON.stringify(envelopeState))');
  });

  it('COPY/PASTE sit in the Active Node row (grouped with SUS/END), not a vertical column', () => {
    // The old .graph-buttons column (rotated 14px-wide text) next to each graph
    // is gone; the canvas now spans the full graph row and COPY/PASTE live next
    // to SUS/END in the controls row.
    types.forEach(t => {
      [1, 2].forEach(line => {
        expect(indexHtml).not.toContain(`graph-buttons`);
        const controlsRow = indexHtml.slice(
          indexHtml.indexOf(`id="btn-sus-env-${t}-line${line}"`) - 400,
          indexHtml.indexOf(`id="btn-sus-env-${t}-line${line}"`) + 300
        );
        expect(controlsRow).toContain(`btn-copy-env-${t}-line${line}`);
        expect(controlsRow).toContain(`btn-paste-env-${t}-line${line}`);
      });
    });
    expect(appJs).not.toContain('graph-buttons');
  });
});
