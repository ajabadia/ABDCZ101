import { describe, it, expect } from 'vitest';
import { execSync } from 'child_process';
import path from 'path';
import { fileURLToPath } from 'url';

const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);
const ROOT_DIR = path.resolve(__dirname, '..', '..');

describe('Parameter Registry --check Mode & SSOT Parity', () => {
  it('executing registry_generator.js with --check terminates with code 0', () => {
    const scriptPath = path.join(ROOT_DIR, 'Scripts', 'registry_generator.js');
    const stdout = execSync(`node "${scriptPath}" --check`, {
      cwd: ROOT_DIR,
      encoding: 'utf8',
    });
    expect(stdout).toContain('CHECK OK');
  });
});
