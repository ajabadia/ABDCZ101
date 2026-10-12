// @vitest-environment jsdom
/**
 * liveSysExInspector-integration.test.js
 * Verificación de integración del LiveSysExInspector transversal en ABDCZ101.
 */

import { describe, expect, it } from 'vitest';
import { readFileSync } from 'node:fs';
import { resolve } from 'node:path';
import { NAVBAR_MENUS } from '../src/contracts/navbarModel.js';
import { SHORTCUTS, matchShortcut } from '../src/contracts/shortcuts.js';
import { createLiveSysExInspector } from '@abdsynths/shared/components';

describe('LiveSysExInspector / Integración ABDCZ101', () => {
  it('el menú View incluye la opción Live SysEx Inspector', () => {
    const viewMenu = NAVBAR_MENUS.find((m) => m.name === 'View');
    expect(viewMenu).toBeDefined();
    const item = viewMenu.items.find((it) => it.action === 'liveSysExInspector');
    expect(item).toBeDefined();
    expect(item.label).toContain('Live SysEx Inspector');
    expect(item.shortcut).toBe('Ctrl+I');
  });

  it('el atajo Ctrl+I está mapeado a la acción liveSysExInspector', () => {
    const shortcut = SHORTCUTS.find((s) => s.action === 'liveSysExInspector');
    expect(shortcut).toBeDefined();
    expect(shortcut.key).toBe('i');
    expect(shortcut.primary).toBe(true);

    const ev = { key: 'i', ctrlKey: true };
    expect(matchShortcut(ev)).toBe('liveSysExInspector');
  });

  it('el componente LiveSysExInspector se exporta correctamente desde @abdsynths/shared/components', () => {
    expect(typeof createLiveSysExInspector).toBe('function');
  });

  it('los estilos live-sysex-inspector.css en WebUI están sincronizados con ABDShared', () => {
    const localCssPath = resolve(__dirname, '../src/styles/live-sysex-inspector.css');
    const sharedCssPath = resolve(__dirname, '../../../ABDShared/Components/LiveSysExInspector/LiveSysExInspector.css');

    const localCss = readFileSync(localCssPath, 'utf8');
    const sharedCss = readFileSync(sharedCssPath, 'utf8');

    expect(localCss).toBe(sharedCss);
  });

  it('la fábrica createLiveSysExInspector instancia el componente con soporte flotante', () => {
    const slot = document.createElement('div');
    document.body.appendChild(slot);

    const fakePatch = new Uint8Array([0xf0, 0x44, 0x01, 0x00, 0x70, 0x01, 0x02, 0xf7]);
    const inspector = createLiveSysExInspector({
      container: slot,
      title: 'CZ-101 Live SysEx Inspector',
      floating: true,
      initialBuffer: fakePatch,
    });

    expect(inspector).toBeDefined();
    expect(inspector.rootEl).toBeDefined();
    expect(inspector.rootEl.classList.contains('sysex-inspector--floating')).toBe(true);

    const sizeBadge = inspector.rootEl.querySelector('.sysex-inspector__badge--size');
    expect(sizeBadge.textContent).toBe('8 B');

    inspector.destroy();
    document.body.removeChild(slot);
  });
});
