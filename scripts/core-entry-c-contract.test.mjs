import test from 'node:test';
import assert from 'node:assert/strict';
import { execFileSync } from 'node:child_process';
import { mkdtemp, rm } from 'node:fs/promises';
import os from 'node:os';
import path from 'node:path';
import { fileURLToPath } from 'node:url';

const root = path.dirname(path.dirname(fileURLToPath(import.meta.url)));

test('private Core entry ABI refuses missing originals and moves a valid record', async () => {
  const owned = await mkdtemp(path.join(os.tmpdir(), 'service-lasso-core-entry-'));
  try {
    const binary = path.join(owned, 'core-entry-contract');
    execFileSync('gcc', [
      '-std=c11',
      '-Wall',
      '-Wextra',
      '-Werror',
      '-I',
      path.join(root, 'native/entry'),
      path.join(root, 'native/entry/sl_core_initial_entry.c'),
      path.join(root, 'native/entry/sl_core_initial_entry_test.c'),
      '-o',
      binary,
    ], { stdio: 'pipe' });
    assert.doesNotThrow(() => execFileSync(binary, { stdio: 'pipe' }));
  } finally {
    await rm(owned, { recursive: true, force: true });
  }
});
