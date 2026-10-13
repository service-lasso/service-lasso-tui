// Source fixtures only until whole review and fresh exact-command admission. UNRUN.
import test from 'node:test';
import assert from 'node:assert/strict';
import { createHash } from 'node:crypto';
import { WINDOWS_NATIVE_ROOT_SYMBOL, locateWindowsNativeRoot, emitWindowsNativeRootHeader } from './windows-native-root-locator.mjs';

function image() {
  const bytes = Buffer.alloc(0x800), pe = 0x80, optional = pe + 24, table = optional + 240;
  bytes.writeUInt16LE(0x5a4d, 0); bytes.writeUInt32LE(pe, 60);
  bytes.writeUInt32LE(0x4550, pe); bytes.writeUInt16LE(0x8664, pe + 4);
  bytes.writeUInt16LE(3, pe + 6); bytes.writeUInt32LE(0x600, pe + 12);
  bytes.writeUInt32LE(1, pe + 16); bytes.writeUInt16LE(240, pe + 20);
  bytes.writeUInt16LE(2, pe + 22); bytes.writeUInt16LE(0x20b, optional);
  bytes.writeUInt32LE(4096, optional + 32); bytes.writeUInt32LE(512, optional + 36);
  bytes.writeUInt32LE(0x4000, optional + 56); bytes.writeUInt32LE(0x200, optional + 60);
  bytes.writeUInt32LE(16, optional + 108);
  function section(index, name, rva, raw, flags) {
    const at = table + index * 40; bytes.write(name, at, 'ascii');
    bytes.writeUInt32LE(0x1000, at + 8); bytes.writeUInt32LE(rva, at + 12);
    bytes.writeUInt32LE(0x200, at + 16); bytes.writeUInt32LE(raw, at + 20);
    bytes.writeUInt32LE(flags, at + 36);
  }
  section(0, '.text', 0x1000, 0x200, 0x60000020);
  section(1, '.data', 0x2000, 0x400, 0xc0000040);
  section(2, '.symtab', 0x3000, 0x600, 0x42000040);
  // Root is in the virtual-only suffix, beyond .data's file-backed bytes.
  bytes.writeUInt32LE(4, 0x604); bytes.writeUInt32LE(0x300, 0x608);
  bytes.writeUInt16LE(2, 0x60c); bytes[0x610] = 2;
  const name = Buffer.from(WINDOWS_NATIVE_ROOT_SYMBOL + '\0', 'ascii');
  bytes.writeUInt32LE(4 + name.length, 0x612); name.copy(bytes, 0x616);
  return { bytes, pe, optional, table };
}

test('virtual BSS root has source extent; header binds actual payload digest', () => {
  const { bytes } = image(), locator = locateWindowsNativeRoot(bytes, 128);
  assert.equal(locator.rva, 0x2300); assert.equal(locator.extent, 128);
  assert.equal(locator.payloadOriginal, bytes);
  const digest = createHash('sha256').update(bytes).digest('hex');
  assert.match(emitWindowsNativeRootHeader(locator, digest), /SL_WINDOWS_ROOT_RVA UINT32_C\(8960\)/);
  bytes[0x210] ^= 1;
  assert.throws(() => emitWindowsNativeRootHeader(locator, digest));
});

test('malformed original image geometry and symbol shape refuse', () => {
  const mutations = [
    ({ bytes, pe }) => bytes.writeUInt16LE(0x14c, pe + 4),
    ({ bytes, pe }) => bytes.writeUInt32LE(0xffffffff, pe + 16),
    ({ bytes }) => bytes.writeUInt32LE(0xffffffff, 0x604),
    ({ bytes }) => { bytes[0x611] = 1; },
    ({ bytes }) => bytes.writeUInt16LE(0x20, 0x60e),
    ({ bytes }) => bytes.writeUInt16LE(0xffff, 0x60c),
    ({ bytes, table }) => bytes.writeUInt32LE(0x60000040, table + 40 + 36),
    ({ bytes, table }) => bytes.writeUInt32LE(0x2200, table + 12),
    ({ bytes, table }) => { bytes[table + 40] |= 0x80; },
    ({ bytes }) => bytes.writeUInt32LE(0x1000, 0x608),
  ];
  for (const mutate of mutations) { const value = image(); mutate(value); assert.throws(() => locateWindowsNativeRoot(value.bytes, 128)); }
  const { bytes } = image();
  assert.throws(() => locateWindowsNativeRoot(bytes, 0));
  assert.throws(() => locateWindowsNativeRoot(bytes.subarray(0, 0x620), 128));
});

test('duplicate exact root symbol refuses instead of choosing first', () => {
  const { bytes, pe } = image();
  const name = Buffer.from(WINDOWS_NATIVE_ROOT_SYMBOL + '\0', 'ascii');
  bytes.writeUInt32LE(2, pe + 16);
  bytes.copy(bytes, 0x612, 0x600, 0x612);
  bytes.writeUInt32LE(4 + name.length, 0x624); name.copy(bytes, 0x628);
  assert.throws(() => locateWindowsNativeRoot(bytes, 128));
});
