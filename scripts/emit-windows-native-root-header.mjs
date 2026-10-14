import { open } from 'node:fs/promises';
import { createHash } from 'node:crypto';
import path from 'node:path';
import { locateWindowsNativeRoot, emitWindowsNativeRootHeader } from './windows-native-root-locator.mjs';

// Build-time association only. Exact-command admission and held source/provider
// custody belong to the producer; this offline emitter grants no runtime entry.
const args = process.argv.slice(2);
if (args.length !== 2 || args.some(value => !path.isAbsolute(value) || value.includes('\0')) ||
    path.resolve(args[0]) === path.resolve(args[1])) throw new Error('root header input denied');
const input = await open(args[0], 'r');
let output;
try {
  const before = await input.stat({ bigint: true });
  if (!before.isFile() || before.size === 0n) throw new Error('payload original denied');
  const bytes = await input.readFile();
  const after = await input.stat({ bigint: true });
  if (before.dev !== after.dev || before.ino !== after.ino || before.size !== after.size ||
      before.mtimeNs !== after.mtimeNs || before.ctimeNs !== after.ctimeNs ||
      BigInt(bytes.length) !== before.size) throw new Error('payload changed during locator read');
  const sha256 = createHash('sha256').update(bytes).digest('hex');
  const locator = locateWindowsNativeRoot(bytes);
  const header = emitWindowsNativeRootHeader(locator, sha256);
  // Never overwrite a preceding attempt's output. Failed partial output remains
  // under the producer's attempt custody; this emitter does not remove it.
  output = await open(args[1], 'wx');
  await output.writeFile(header, 'utf8');
  await output.sync();
  process.stdout.write(JSON.stringify({ kind: 'windows-root-build-association',
    payloadSHA256: sha256, payloadBytes: bytes.length, rootRVA: locator.rva,
    rootExtent: locator.extent, imageSize: locator.imageSize }) + '\n');
} finally {
  try { if (output) await output.close(); }
  finally { await input.close(); }
}
