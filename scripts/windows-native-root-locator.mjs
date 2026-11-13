import { createHash } from 'node:crypto';
import { WINDOWS_NATIVE_ROOT_EXTENT, WINDOWS_NATIVE_ROOT_ALIGNMENT } from './windows-native-root-layout.mjs';

// Offline build input only. No locator, symbol or digest supplies runtime authority.
export const WINDOWS_NATIVE_ROOT_SYMBOL = 'internal/runtime/syscall/windows.serviceLassoNativeRoot';
const refuse = () => { throw new Error('Windows native root locator denied'); };
const uint32 = value => Number.isSafeInteger(value) && value >= 0 && value <= 0xffffffff;

export function locateWindowsNativeRoot(bytes, sourceExtent = WINDOWS_NATIVE_ROOT_EXTENT) {
  if (!Buffer.isBuffer(bytes) || bytes.length < 64 ||
      sourceExtent !== WINDOWS_NATIVE_ROOT_EXTENT) refuse();
  const span = (offset, length) => {
    if (!Number.isSafeInteger(offset) || !Number.isSafeInteger(length) ||
        offset < 0 || length < 0 || offset > bytes.length || length > bytes.length - offset) refuse();
  };
  const u16 = offset => { span(offset, 2); return bytes.readUInt16LE(offset); };
  const u32 = offset => { span(offset, 4); return bytes.readUInt32LE(offset); };
  if (u16(0) !== 0x5a4d) refuse();
  const pe = u32(60);
  if (pe < 64 || u32(pe) !== 0x4550 || u16(pe + 4) !== 0x8664) refuse();
  span(pe, 24);
  const sectionCount = u16(pe + 6), symbolOffset = u32(pe + 12), symbolCount = u32(pe + 16);
  const optionalSize = u16(pe + 20), characteristics = u16(pe + 22), optional = pe + 24;
  if (!sectionCount || !symbolCount || !symbolOffset || !(characteristics & 2) ||
      (characteristics & 0x2000) || optionalSize < 112) refuse();
  span(optional, optionalSize);
  if (u16(optional) !== 0x20b) refuse();
  const imageSize = u32(optional + 56), headerSize = u32(optional + 60);
  const sectionAlignment = u32(optional + 32), fileAlignment = u32(optional + 36);
  const directoryCount = u32(optional + 108);
  // The selected ordinary internal Go PE producer uses these source-defined alignments.
  if (sectionAlignment !== 4096 || fileAlignment !== 512 || imageSize % sectionAlignment !== 0 ||
      headerSize % fileAlignment !== 0) refuse();
  if (!imageSize || !headerSize || headerSize > imageSize || headerSize > bytes.length ||
      directoryCount > Math.floor((optionalSize - 112) / 8)) refuse();
  const sectionsOffset = optional + optionalSize, sectionBytes = sectionCount * 40;
  span(sectionsOffset, sectionBytes);
  if (sectionsOffset + sectionBytes > headerSize) refuse();
  const sections = [];
  for (let index = 0; index < sectionCount; index++) {
    const at = sectionsOffset + index * 40;
    const virtualSize = u32(at + 8), rva = u32(at + 12), rawSize = u32(at + 16), rawOffset = u32(at + 20);
    const mappedSize = Math.ceil(Math.max(virtualSize, rawSize) / sectionAlignment) * sectionAlignment;
    if (rva % sectionAlignment !== 0 || rawSize % fileAlignment !== 0 ||
        rva < headerSize || rva > imageSize || mappedSize > imageSize - rva) refuse();
    if (rawSize) {
      if (rawOffset < headerSize || rawOffset % fileAlignment !== 0) refuse();
      span(rawOffset, rawSize);
    }
    const nameBytes = bytes.subarray(at, at + 8);
    const zero = nameBytes.indexOf(0), end = zero < 0 ? 8 : zero;
    if (!end || nameBytes.subarray(0, end).some(value => value > 0x7f) ||
        (zero >= 0 && nameBytes.subarray(zero).some(value => value !== 0))) refuse();
    sections.push({ name: nameBytes.subarray(0, end).toString('ascii'), virtualSize, mappedSize, rva,
      rawSize, rawOffset, flags: u32(at + 36) });
  }
  const disjoint = (startKey, sizeKey) => {
    const ordered = sections.filter(section => section[sizeKey] !== 0)
      .sort((a, b) => a[startKey] - b[startKey]);
    for (let index = 1; index < ordered.length; index++) {
      const prior = ordered[index - 1];
      if (prior[startKey] + prior[sizeKey] > ordered[index][startKey]) refuse();
    }
  };
  disjoint('rva', 'mappedSize');
  disjoint('rawOffset', 'rawSize');
  // Every row belongs to the selected internal Go linker: no auxiliary rows.
  const symbolBytes = symbolCount * 18;
  span(symbolOffset, symbolBytes);
  const stringsOffset = symbolOffset + symbolBytes, stringsSize = u32(stringsOffset);
  if (stringsSize < 4) refuse();
  span(stringsOffset, stringsSize);
  if (symbolOffset < headerSize) refuse();
  for (const section of sections) {
    if (section.rawSize && symbolOffset < section.rawOffset + section.rawSize &&
        stringsOffset + stringsSize > section.rawOffset) {
      // Go's internal .symtab section deliberately contains this whole table.
      if (section.name !== '.symtab' || symbolOffset !== section.rawOffset ||
          stringsOffset + stringsSize > section.rawOffset + section.rawSize) refuse();
    }
  }
  const wanted = Buffer.from(WINDOWS_NATIVE_ROOT_SYMBOL, 'ascii');
  let found;
  for (let index = 0; index < symbolCount; index++) {
    const at = symbolOffset + index * 18;
    if (bytes[at + 17] !== 0) refuse();
    let name;
    if (u32(at) === 0) {
      const offset = u32(at + 4);
      if (offset < 4 || offset >= stringsSize) refuse();
      const start = stringsOffset + offset, end = bytes.indexOf(0, start);
      if (end < start || end >= stringsOffset + stringsSize) refuse();
      name = bytes.subarray(start, end);
    } else {
      const field = bytes.subarray(at, at + 8), zero = field.indexOf(0);
      if (zero >= 0 && field.subarray(zero).some(value => value !== 0)) refuse();
      name = field.subarray(0, zero < 0 ? 8 : zero);
    }
    if (!name.length) refuse();
    if (!name.equals(wanted)) continue;
    if (found) refuse();
    const sectionNumber = bytes.readInt16LE(at + 12), offset = u32(at + 8);
    const type = u16(at + 14), storage = bytes[at + 16];
    if (sectionNumber <= 0 || sectionNumber > sections.length || type !== 0 ||
        (storage !== 2 && storage !== 3)) refuse();
    const section = sections[sectionNumber - 1];
    if (section.name !== '.data' || !(section.flags & 0x80000000) ||
        (section.flags & (0x20000000 | 0x10000000 | 0x02000000)) || offset > section.virtualSize ||
        sourceExtent > section.virtualSize - offset) refuse();
    const rva = section.rva + offset;
    if (!uint32(rva) || rva % WINDOWS_NATIVE_ROOT_ALIGNMENT !== 0 || sourceExtent > imageSize - rva) refuse();
    found = { rva, extent: sourceExtent, sectionNumber, imageSize };
  }
  if (!found) refuse();
  return Object.freeze({ ...found, symbol: WINDOWS_NATIVE_ROOT_SYMBOL, payloadOriginal: bytes });
}

export function emitWindowsNativeRootHeader(locator, payloadSHA256) {
  if (!locator || !Buffer.isBuffer(locator.payloadOriginal) ||
      locator.symbol !== WINDOWS_NATIVE_ROOT_SYMBOL || !uint32(locator.rva) ||
      !uint32(locator.extent) || locator.extent === 0 ||
      typeof payloadSHA256 !== 'string' || !/^[a-f0-9]{64}$/.test(payloadSHA256)) refuse();
  // Re-read SAME original bytes; caller separately binds their held digest/source/tool provenance.
  const checked = locateWindowsNativeRoot(locator.payloadOriginal, locator.extent);
  if (checked.rva !== locator.rva || checked.imageSize !== locator.imageSize ||
      checked.sectionNumber !== locator.sectionNumber ||
      createHash('sha256').update(locator.payloadOriginal).digest('hex') !== payloadSHA256) refuse();
  return '#ifndef SERVICE_LASSO_GENERATED_WINDOWS_ROOT_H\n#define SERVICE_LASSO_GENERATED_WINDOWS_ROOT_H\n' +
    '#include <stdint.h>\n#include "windows_root_layout.h"\n' +
    `#define SL_WINDOWS_ROOT_RVA UINT32_C(${checked.rva})\n` +
    `#define SL_WINDOWS_ROOT_EXTENT UINT32_C(${checked.extent})\n` +
    `#define SL_WINDOWS_PAYLOAD_IMAGE_SIZE UINT32_C(${checked.imageSize})\n` +
    `#define SL_WINDOWS_PAYLOAD_SHA256 "${payloadSHA256}"\n` +
    '_Static_assert(SL_WINDOWS_ROOT_EXTENT == SL_WINDOWS_NATIVE_ROOT_SOURCE_EXTENT, "generated root extent");\n' +
    '#endif\n';
}
