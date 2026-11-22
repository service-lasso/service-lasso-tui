import test from 'node:test';
import assert from 'node:assert/strict';
import { execFileSync } from 'node:child_process';
import { mkdtemp, rm } from 'node:fs/promises';
import os from 'node:os';
import path from 'node:path';
import { fileURLToPath } from 'node:url';
import { nativeOwnerCompilePlan } from './native-owner-build-plan.mjs';

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

test('native owner build plan includes the private Core entry contract', () => {
  const plan = nativeOwnerCompilePlan('linux-x64', 'clang', '/tmp/generated-entry.c', '/tmp/objects');
  for (const source of ['native/entry/sl_core_initial_entry.c', 'native/entry/sl_core_entry_construct.c', 'native/entry/sl_core_entry_native.c']) {
    const row = plan.commands.find((item) => item.source === source);
    assert.ok(row, source);
    assert.ok(row.argv.includes('-Inative/entry'));
  }
});

test('native construction adapter compiles against the owner contracts', async () => {
  const owned = await mkdtemp(path.join(os.tmpdir(), 'service-lasso-construct-'));
  try {
    execFileSync('gcc', [
      '-std=c11', '-Wall', '-Wextra', '-Werror',
      '-Inative/owner', '-Inative/entry',
      '-c', path.join(root, 'native/entry/sl_core_entry_construct.c'),
      '-o', path.join(owned, 'construct.o'),
    ], { cwd: root, stdio: 'pipe' });
  } finally {
    await rm(owned, { recursive: true, force: true });
  }
});

test('native view adapter maps validated views to operands', async () => {
  const owned = await mkdtemp(path.join(os.tmpdir(), 'service-lasso-native-'));
  try {
    const binary = path.join(owned, 'native');
    execFileSync('gcc', [
      '-std=c11', '-Wall', '-Wextra', '-Werror', '-Inative/entry',
      path.join(root, 'native/entry/sl_core_initial_entry.c'),
      path.join(root, 'native/entry/sl_core_entry_native.c'),
      path.join(root, 'native/entry/sl_core_entry_native_test.c'),
      '-o', binary,
    ], { cwd: root, stdio: 'pipe' });
    assert.doesNotThrow(() => execFileSync(binary, { stdio: 'pipe' }));
  } finally {
    await rm(owned, { recursive: true, force: true });
  }
});
