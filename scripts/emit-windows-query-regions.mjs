import {open} from 'node:fs/promises';
import path from 'node:path';
import {createHash} from 'node:crypto';
import {generateWindowsQueryRegions} from './generate-windows-query-regions.mjs';

// Exact-command admitted producer owns profile provenance and all outputs.
// This emitter creates build association bytes, never runtime reservation.
const args = process.argv.slice(2);
if (args.length !== 3 || args.some(value => !path.isAbsolute(value) || value.includes('\0')) ||
    new Set(args.map(value => path.resolve(value).toLowerCase())).size !== 3) throw new Error('query emitter paths');
const input = await open(args[0], 'r');
let goOutput, headerOutput;
async function readBackOriginal(handle, expected) {
  const before = await handle.stat({bigint:true});
  if (!before.isFile() || before.size !== BigInt(expected.length)) throw new Error('emitted query extent changed');
  // Source-byte extent determines this separate original read buffer's size;
  // actual containing reservation funds it. Native file size never creates
  // allocation authority. A partial read is not retried.
  const backing = Buffer.alloc(expected.length);
  const returned = await handle.read(backing, 0, backing.length, 0);
  const after = await handle.stat({bigint:true});
  if (returned.bytesRead !== expected.length || !backing.equals(expected) ||
      before.dev !== after.dev || before.ino !== after.ino || before.size !== after.size ||
      before.mtimeNs !== after.mtimeNs || before.ctimeNs !== after.ctimeNs) throw new Error('emitted query readback changed');
  return {before, after, returned, backing};
}
try {
  const before = await input.stat({bigint:true});
  if (!before.isFile() || before.size === 0n) throw new Error('query profile original');
  const bytes = await input.readFile();
  const after = await input.stat({bigint:true});
  if (before.dev !== after.dev || before.ino !== after.ino || before.size !== after.size ||
      before.mtimeNs !== after.mtimeNs || before.ctimeNs !== after.ctimeNs ||
      BigInt(bytes.length) !== before.size) throw new Error('query profile changed');
  const generated = generateWindowsQueryRegions(bytes);
  // Failed partial output stays in this exact producer attempt's custody.
  // Neither an earlier attempt nor either other file may be overwritten.
  const goSource = Buffer.from(generated.go, 'utf8');
  const headerSource = Buffer.from(generated.header, 'utf8');
  goOutput = await open(args[1], 'wx+');
  await goOutput.writeFile(goSource);
  await goOutput.sync();
  const goOriginal = await readBackOriginal(goOutput, goSource);
  headerOutput = await open(args[2], 'wx+');
  await headerOutput.writeFile(headerSource);
  await headerOutput.sync();
  const headerOriginal = await readBackOriginal(headerOutput, headerSource);
  process.stdout.write(JSON.stringify({kind:'windows-query-build-association',
    profileSHA256:generated.profileSHA256, sourceCommit:generated.sourceCommit,
    providerProfileSHA256:generated.providerProfileSHA256, staticBytes:generated.staticBytes,
    argumentStaticBytes:generated.argumentStaticBytes,
    headerReaderStaticBytes:generated.headerReaderStaticBytes,
    headerInterpretStaticBytes:generated.headerInterpretStaticBytes,
    bodyReaderStaticBytes:generated.bodyReaderStaticBytes,
    creatorImageBackingBytes:generated.creatorImageBackingBytes,
    creatorImageMappingBytes:generated.creatorImageMappingBytes,
    creatorImageNameOriginalBytes:generated.creatorImageNameOriginalBytes,
    creatorImageFileOriginalBytes:generated.creatorImageFileOriginalBytes,
    creatorImageMetadataOriginalBytes:generated.creatorImageMetadataOriginalBytes,
    creatorImageFileReadOriginalBytes:generated.creatorImageFileReadOriginalBytes,
    creatorImageNativePEOriginalBytes:generated.creatorImageNativePEOriginalBytes,
    creatorImageExtentOriginalBytes:generated.creatorImageExtentOriginalBytes,
    creatorImageRegionOriginalBytes:generated.creatorImageRegionOriginalBytes,
    creatorImageTypedGeometryBytes:generated.creatorImageTypedGeometryBytes,
    argumentWriterDimensions:generated.argumentWriterDimensions,
    goSHA256:createHash('sha256').update(goOriginal.backing).digest('hex'),
    headerSHA256:createHash('sha256').update(headerOriginal.backing).digest('hex')})+'\n');
} finally {
  try { if (headerOutput) await headerOutput.close(); }
  finally {
    try { if (goOutput) await goOutput.close(); }
    finally { await input.close(); }
  }
}
