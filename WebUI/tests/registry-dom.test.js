import { describe, it, expect } from 'vitest';
import fs from 'fs';
import path from 'path';
import { PARAMETER_REGISTRY, PARAM_MAP } from '../src/contracts/registry.gen.js';

describe('DOM to Registry Parity Tests', () => {
  const htmlPath = path.resolve(__dirname, '../index.html');
  const htmlContent = fs.readFileSync(htmlPath, 'utf8');

  // Parse all IDs inside input, select, and button elements in the HTML
  const domIdRegex = /<(?:input|select|button)\s+[^>]*id=["']([^"']+)["']/gi;
  const foundDomIds = [];
  let match;
  while ((match = domIdRegex.exec(htmlContent)) !== null) {
    foundDomIds.push(match[1]);
  }

  // Non-parameter IDs used for UI wiring/controls
  const uiExclusions = new Set([
    'lcd-line1',
    'lcd-line2',
    'btn-init-audio',
    'input-sysex',
    'oscilloscope',
    'keyboard',
    'select-preset-slot',
    'input-preset-name',
    'input-bank-file',
    'midi-status-badge',
    'midi-learn-badge',
    'midi-activity-led',
    'btn-random',
    'btn-panic',
    'btn-lcd-bnk',
    'btn-lcd-mdl',
    'btn-about-close',
    'virtual-pitch-bend',
    'virtual-mod-wheel',
    'btn-edit-env-dca',
    'btn-edit-env-dcw',
    'btn-edit-env-pitch',
    'btn-edit-dco-osc2',
    'canvas-env-dca-line1',
    'canvas-env-dca-line2',
    'canvas-env-dcw-line1',
    'canvas-env-dcw-line2',
    'canvas-env-pitch-line1',
    'canvas-env-pitch-line2',
    'btn-copy-env-dca-line1',
    'btn-paste-env-dca-line1',
    'btn-copy-env-dca-line2',
    'btn-paste-env-dca-line2',
    'btn-copy-env-dcw-line1',
    'btn-paste-env-dcw-line1',
    'btn-copy-env-dcw-line2',
    'btn-paste-env-dcw-line2',
    'btn-copy-env-pitch-line1',
    'btn-paste-env-pitch-line1',
    'btn-copy-env-pitch-line2',
    'btn-paste-env-pitch-line2',
    'btn-sus-env-dca-line1',
    'btn-end-env-dca-line1',
    'btn-sus-env-dca-line2',
    'btn-end-env-dca-line2',
    'btn-sus-env-dcw-line1',
    'btn-end-env-dcw-line1',
    'btn-sus-env-dcw-line2',
    'btn-end-env-dcw-line2',
    'btn-sus-env-pitch-line1',
    'btn-end-env-pitch-line1',
    'btn-sus-env-pitch-line2',
    'btn-end-env-pitch-line2',
    'slider-rate-env-dca-line1',
    'slider-level-env-dca-line1',
    'slider-rate-env-dca-line2',
    'slider-level-env-dca-line2',
    'slider-rate-env-dcw-line1',
    'slider-level-env-dcw-line1',
    'slider-rate-env-dcw-line2',
    'slider-level-env-dcw-line2',
    'slider-rate-env-pitch-line1',
    'slider-level-env-pitch-line1',
    'slider-rate-env-pitch-line2',
    'slider-level-env-pitch-line2',
    'btn-lcd-compare',
    'btn-lcd-write',
    'btn-lcd-cur-left',
    'btn-lcd-cur-right',
    'btn-lcd-val-up',
    'btn-lcd-val-down',
    'btn-lcd-set',
    'audio-output-device',
    'btn-refresh-audio-devices',
    'btn-preset-prev',
    'btn-edit-model-modes',
    'btn-edit-dco',
    'btn-edit-arp',
    'btn-edit-lfo',
    'btn-edit-filters',
    'btn-edit-effects',
    'btn-edit-mod',
    'mod-matrix',
    'mod-slot-badge-1',
    'mod-slot-badge-2',
    'mod-slot-badge-3',
    'mod-slot-badge-4',
    'mod-slot-badge-5',
    'mod-slot-badge-6',
    'mod-slot-badge-7',
    'mod-slot-badge-8',
    'block-drawer',
    'block-drawer-backdrop',
    'btn-block-drawer-close',
    'block-drawer-title',
    'block-drawer-body',
    'btn-preset-next',
    'bank-manager-modal',
    'bank-slot-list',
    'btn-bank-close',
    'bank-context-menu',
    'bank-select',
    'bank-model-filter',
    'bank-model-badge',
    'bank-search-input',
    'bank-search-clear',
    'bank-mgr-status',
    'btn-bank-new',
    'btn-bank-rename',
    'btn-bank-delete',
    'btn-bank-import',
    'btn-bank-export',
    'input-bank-import',
    'name-editor-modal',
    'name-editor-input',
    'btn-name-editor-cancel',
    'btn-name-editor-save',
    'btn-copy-sysex',
    'btn-edit-sys',
    'btn-edit-voice',
    'btn-edit-ctrl',
    'btn-edit-filter-lpf',
    'btn-edit-filter-hpf',
    'btn-edit-fx-chorus',
    'btn-edit-fx-reverb',
    'btn-edit-fx-delay',
    'btn-edit-fx-drive',
    'MACRO_CONTOUR',
    'ARP_OCTAVES',
    'LINE1_KF_PITCH', 'LINE1_KF_DCW', 'LINE1_KF_DCA',
    'LINE2_KF_PITCH', 'LINE2_KF_DCW', 'LINE2_KF_DCA',
    'octave-up-btn', 'octave-down-btn'
  ]);

  // Parameters defined in registry that do not have UI controls in index.html.
  // 1. The free mod-matrix slots (MOD_SLOT_<n>_SRC/DEST/DEPTH) are created
  //    dynamically at runtime (ABDEEP-style 8-slot matrix in the drawer).
  // 2. The fixed HARDWARE ROUTES (MOD_VELO_*, MOD_WHEEL_*, MOD_AT_*, KEY_*)
  //    had their drawer controls removed: every route is replicable with the
  //    free matrix, and the engine keeps them as the authentic base behavior
  //    (values come from presets/sysex + the LCD menu in Classic modes).
  const expectedMissingFromUI = new Set([
    ...Array.from({ length: 8 }, (_, i) => [`MOD_SLOT_${i + 1}_SRC`, `MOD_SLOT_${i + 1}_DEST`, `MOD_SLOT_${i + 1}_DEPTH`]).flat(),
    'MOD_VELO_DCW', 'MOD_VELO_DCA',
    'MOD_WHEEL_VIB', 'MOD_WHEEL_DCW', 'MOD_WHEEL_LFORATE',
    'KEY_TRACK_DCW', 'KEY_TRACK_PITCH',
    'MOD_AT_DCW', 'MOD_AT_VIB',
    'KEY_FOLLOW_DCO', 'KEY_FOLLOW_DCW', 'KEY_FOLLOW_DCA',
    'DCA_ATTACK', 'DCA_DECAY', 'DCA_SUSTAIN', 'DCA_RELEASE',
    'DCW_ATTACK', 'DCW_DECAY', 'DCW_SUSTAIN', 'DCW_RELEASE',
    'ARP_OCTAVE',
    'BYPASS', 'SYSTEM_PRG'
  ]);

  it('should ensure every parameter ID in the HTML DOM exists in the registry', () => {
    foundDomIds.forEach(id => {
      if (uiExclusions.has(id)) return;
      
      const hasParam = PARAM_MAP.has(id);
      expect(hasParam, `DOM element with ID "${id}" has no corresponding parameter in the registry.`).toBe(true);
    });
  });

  it('should verify registry parameters are represented in the HTML DOM unless explicitly excluded', () => {
    const domIdsSet = new Set(foundDomIds);
    PARAMETER_REGISTRY.parameters.forEach(p => {
      const isExpectedMissing = expectedMissingFromUI.has(p.id);
      const isPresentInDOM = domIdsSet.has(p.id);
      
      if (!isExpectedMissing) {
        expect(isPresentInDOM, `Registry parameter "${p.id}" is missing from the index.html DOM.`).toBe(true);
      }
    });
  });
});
