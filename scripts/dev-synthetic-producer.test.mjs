import test from 'node:test';
import assert from 'node:assert/strict';
import { execFileSync, spawnSync } from 'node:child_process';
import { mkdtemp, rm, readFile } from 'node:fs/promises';
import os from 'node:os';
import path from 'node:path';
import { fileURLToPath } from 'node:url';

const root = path.dirname(path.dirname(fileURLToPath(import.meta.url)));
const bundle = path.join(root, 'fixtures/dev-synthetic/bundle');
const go = process.env.GO || 'go';
const goAvailable = spawnSync(go, ['version'], { stdio: 'ignore' }).status === 0;
const linuxOnly = process.platform === 'linux' && goAvailable ? {} : { skip: 'Linux producer fixture with Go' };

test('development-synthetic fixture bundle is clearly classified', async () => {
  const record = JSON.parse(await readFile(path.join(bundle, 'native-inputs.json'), 'utf8'));
  assert.equal(record.classification, 'development-synthetic');
  assert.equal(record.schema, 'service-lasso.tui-native-input-bundle.v1');
});

test('native producer binary consumes the development-synthetic bundle only explicitly', linuxOnly, async () => {
  const directory = await mkdtemp(path.join(os.tmpdir(), 'service-lasso-producer-test-'));
  const binary = path.join(directory, 'service-lasso-entry-producer');
  try {
    execFileSync(go, ['build', '-o', binary, './cmd/service-lasso-entry-producer'], { cwd: root, stdio: 'pipe' });
    const handoff = Buffer.from(JSON.stringify({
      contract: 'service-lasso.candidate-entry-handoff.v1',
      lease: { leaseId: 'lease-dev' },
    })).toString('base64');
    const input = `${JSON.stringify({ kind: 'materialize', handoffLine: handoff, observation: { pid: 7 } })}\n`;
    const accepted = spawnSync(binary, ['--development-synthetic', bundle], { input, maxBuffer: 8 * 1024 * 1024 });
    assert.equal(accepted.status, 0, accepted.stderr.toString());
    const separator = accepted.stdout.indexOf(0x0a);
    const response = JSON.parse(accepted.stdout.subarray(0, separator).toString('utf8'));
    assert.equal(response.ok, true);
    assert.equal(accepted.stdout.length, separator + 1 + response.imageBytes);

    const refused = spawnSync(binary, [bundle], { input, maxBuffer: 8 * 1024 * 1024 });
    assert.equal(refused.status, 0);
    assert.deepEqual(JSON.parse(refused.stdout.toString('utf8')), {
      ok: false,
      code: 'binder_originals_unavailable',
    });
  } finally {
    await rm(directory, { recursive: true, force: true });
  }
});
