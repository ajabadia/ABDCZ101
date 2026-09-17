import { defineConfig } from 'vitest/config';

export default defineConfig({
  test: {
    globals: true,
    environment: 'node',
    include: ['WebUI/tests/**/*.test.js'],
    // Guard against hangs (e.g. the hold-repeat timer tests): fail instead of
    // blocking the whole suite forever.
    testTimeout: 15000,
    hookTimeout: 15000
  }
});
