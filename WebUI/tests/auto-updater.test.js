import { describe, it, expect } from 'vitest';
import fs from 'fs';
import path from 'path';
import { NAVBAR_MENUS } from '../src/contracts/navbarModel.js';

describe('AutoUpdater Integration in ABDCZ101 WebUI', () => {
  it('exposes "Check for Updates..." in the Help navbar menu', () => {
    const helpMenu = NAVBAR_MENUS.find(m => m.name === 'Help');
    expect(helpMenu).toBeDefined();

    const checkItem = helpMenu.items.find(i => i.action === 'checkForUpdates');
    expect(checkItem).toBeDefined();
    expect(checkItem.id).toBe(902);
    expect(checkItem.label).toBe('Check for Updates...');

    const actions = helpMenu.items.filter(i => !i.separator).map(i => i.action);
    expect(actions).toEqual(['manual', 'checkForUpdates', 'about']);
  });

  it('contains the update section and elements inside #about-modal in index.html', () => {
    const htmlPath = path.resolve(__dirname, '../index.html');
    const html = fs.readFileSync(htmlPath, 'utf8');

    expect(html).toContain('id="about-update-section"');
    expect(html).toContain('id="about-version-text"');
    expect(html).toContain('id="btn-check-updates"');
    expect(html).toContain('id="about-update-status"');
    expect(html).toContain('id="about-update-action"');
    expect(html).toContain('id="btn-download-update"');
  });

  it('defines CSS styles for the AutoUpdater UI in overlays.css', () => {
    const cssPath = path.resolve(__dirname, '../src/styles/overlays.css');
    const css = fs.readFileSync(cssPath, 'utf8');

    expect(css).toContain('.about-update-section');
    expect(css).toContain('.about-update-row');
    expect(css).toContain('.about-update-status');
    expect(css).toContain('.btn-update-avail');
  });

  it('wires the checkForUpdates and about actions in navbar.js', () => {
    const navbarPath = path.resolve(__dirname, '../src/ui/navbar.js');
    const js = fs.readFileSync(navbarPath, 'utf8');

    expect(js).toContain('checkForUpdates: () =>');
    expect(js).toContain('triggerUpdateCheck()');
    expect(js).toContain('queryUpdateStatus()');
    expect(js).toContain('updateCheckResult');
  });
});
