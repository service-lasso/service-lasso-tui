import crypto from 'node:crypto';

const names = Object.freeze(['endpointSecurity', 'processSecurity', 'tokenAccess']);
const identifiers = Object.freeze(['EndpointSecurity', 'ProcessSecurity', 'TokenAccess']);
function exact(value, keys) {
  if (!value || typeof value !== 'object' || Array.isArray(value) ||
      Object.keys(value).sort().join('\0') !== [...keys].sort().join('\0')) throw new Error('query profile shape');
}

// Input association checks are not native-original authentication or financing.
// The containing caller owns full reviewed profile/provider/source provenance,
// exact emitted bytes and aggregate reservation before any native effect.
export function generateWindowsQueryRegions(originalBytes) {
  if (!Buffer.isBuffer(originalBytes)) throw new Error('missing query profile original');
  const profile = JSON.parse(new TextDecoder('utf-8', {fatal: true}).decode(originalBytes));
  exact(profile, ['schemaVersion', 'sourceCommit', 'providerProfileSHA256', 'regions', 'arguments', 'argumentWriter', 'creatorImages']);
  if (profile.schemaVersion !== 3 || typeof profile.sourceCommit !== 'string' ||
      typeof profile.providerProfileSHA256 !== 'string' || !/^[a-f0-9]{40}$/u.test(profile.sourceCommit) ||
      !/^[a-f0-9]{64}$/u.test(profile.providerProfileSHA256)) throw new Error('query profile association');
  exact(profile.regions, names);
  const dimensions = names.map(name => {
    const region = profile.regions[name];
    exact(region, ['capacity', 'originalSHA256']);
    // DWORD is the actual native query length domain, not a supported-host cap.
    if (!Number.isSafeInteger(region.capacity) || region.capacity <= 0 || region.capacity > 0xffffffff ||
        typeof region.originalSHA256 !== 'string' || !/^[a-f0-9]{64}$/u.test(region.originalSHA256)) throw new Error('query region dimension');
    return region.capacity;
  });
  const digest = crypto.createHash('sha256').update(originalBytes).digest('hex');
  exact(profile.argumentWriter, ['stackReservationBytes', 'turnCapacity', 'originalSHA256']);
  if(typeof profile.argumentWriter.originalSHA256!=='string'||!/^[a-f0-9]{64}$/u.test(profile.argumentWriter.originalSHA256))throw new Error('argument writer association');
  function nativeSize(name){
    const text=profile.argumentWriter[name];
    if(typeof text!=='string'||!/^[1-9][0-9]{0,19}$/u.test(text))throw new Error('argument writer SIZE_T representation');
    const value=BigInt(text);
    if(value>0xffffffffffffffffn)throw new Error('argument writer SIZE_T extent');
    return value;
  }
  const writerStack=nativeSize('stackReservationBytes'),writerTurns=nativeSize('turnCapacity');
  exact(profile.creatorImages, ['moduleCapacity','mappedNameUnitsIncludingNUL','fileBytes','memoryBytes','memoryRegionCapacity','originalSHA256']);
  if(typeof profile.creatorImages.originalSHA256!=='string'||!/^[a-f0-9]{64}$/u.test(profile.creatorImages.originalSHA256))throw new Error('creator image association');
  function imageSize(name) {
    const text=profile.creatorImages[name];
    if(typeof text!=='string'||!/^[1-9][0-9]{0,19}$/u.test(text))throw new Error('creator image SIZE_T representation');
    const value=BigInt(text);
    if(value>0xffffffffffffffffn)throw new Error('creator image SIZE_T extent');
    return value;
  }
  const imageModules=imageSize('moduleCapacity'),imageFileBytes=imageSize('fileBytes'),imageMemoryBytes=imageSize('memoryBytes');
  const imageRegions=imageSize('memoryRegionCapacity');
  const imageNameUnits=profile.creatorImages.mappedNameUnitsIncludingNUL;
  if(!Number.isSafeInteger(imageNameUnits)||imageNameUnits<=0||imageNameUnits>0xffffffff)throw new Error('creator mapped name DWORD extent');
  const imageModuleBytes=8n*imageModules;
  if(imageModuleBytes>0xffffffffn)throw new Error('creator module roster DWORD extent');
  // These source dimensions never supply runtime/provider/finance authority.
  const imagePathUnits=BigInt(imageNameUnits)+14n;
  const imageBackingBytes=2n*imageModuleBytes+2n*imageModules*(BigInt(imageNameUnits)+imagePathUnits)*2n+imageFileBytes+imageMemoryBytes;
  if(imageBackingBytes>0xffffffffffffffffn)throw new Error('creator image aggregate extent');
  const imageFileReadRecordsBytes=128n*imageFileBytes;
  if(imageFileReadRecordsBytes>0xffffffffffffffffn)throw new Error('creator file read roster extent');
  const imageRegionOriginalBytes=imageRegions*(512n+128n+96n)+2n*imageModules*96n+16n;
  const imageGeometryBytes=imageBackingBytes+imageFileReadRecordsBytes+2n*imageModules*(496n+456n+480n+1336n+96n+472n+176n)+2n*456n+8n+imageRegionOriginalBytes;
  if(imageGeometryBytes>0xffffffffffffffffn)throw new Error('creator image typed geometry aggregate');
  exact(profile.arguments, ['moduleUnitsIncludingNUL', 'originalSHA256']);
  const moduleUnits=profile.arguments.moduleUnitsIncludingNUL;
  if (!Number.isSafeInteger(moduleUnits)||moduleUnits<=0||moduleUnits>0xffffffff||
      typeof profile.arguments.originalSHA256!=='string'||!/^[a-f0-9]{64}$/u.test(profile.arguments.originalSHA256)) throw new Error('module source envelope');
  // M includes the terminator: a successful returned count is at most M-1.
  // Each original Go query supplies a fresh n=1024,2048,... until n>count.
  // This is native DWORD representability, not an independently admitted M.
  const M=BigInt(moduleUnits),K=(M+1023n)/1024n;
  if(K*1024n>0xffffffffn)throw new Error('module query native DWORD extent');
  const queryUnits=1024n*K*(K+1n)/2n;
  // At most N=C-1 non-NUL units. Each argument needs at least one unit and
  // every subsequent argument needs a delimiter: ceil(N/2)=floor(C/2).
  const rawUnits=32767n,maximumPublicArguments=rawUnits/2n;
  const bodyBytes=2n*(rawUnits+M);
  const bodyReadRecordsBytes=128n*bodyBytes;
  if(bodyReadRecordsBytes>0xffffffffffffffffn)throw new Error('body read roster native extent');
  const utf8Bytes=3n*(M-1n>rawUnits-1n?M-1n:rawUnits-1n);
  const go = ['//go:build windows && amd64', '', '// Generated from exact reviewed build-private inputs; no runtime authority.',
    'package windows', '', 'import "unsafe"', ''];
  const header = ['#ifndef SERVICE_LASSO_GENERATED_QUERY_REGIONS_H', '#define SERVICE_LASSO_GENERATED_QUERY_REGIONS_H',
    '#include <stdint.h>', '#include <stddef.h>',
    '/* Build association/geometry only. Native factory reservation is separate. */'];
  let staticBytes = 0;
  go.push(`const serviceLassoCreatorModuleCapacity = ${imageModules}`,
    `const serviceLassoCreatorModuleBytes = ${imageModuleBytes}`,
    `const serviceLassoCreatorMappedNameUnits = ${imageNameUnits}`,
    `const serviceLassoCreatorPathUnits = ${imagePathUnits}`,
    `const serviceLassoCreatorFileBytesCapacity = ${imageFileBytes}`,
    `const serviceLassoCreatorMemoryBytesCapacity = ${imageMemoryBytes}`,
    `const serviceLassoCreatorRegionCapacity = ${imageRegions}`,
    `var serviceLassoCreatorModulesBefore [${imageModules}]uint64`,
    `var serviceLassoCreatorModulesAfter [${imageModules}]uint64`,
    `var serviceLassoCreatorNamesBefore [${imageModules}][${imageNameUnits}]uint16`,
    `var serviceLassoCreatorNamesAfter [${imageModules}][${imageNameUnits}]uint16`,
    `var serviceLassoCreatorFileBytes [${imageFileBytes}]byte`,
    `var serviceLassoCreatorMemoryBytes [${imageMemoryBytes}]byte`,
    `var serviceLassoCreatorMappingsBefore [${imageModules}]serviceLassoCreatorImageMappingStorage`,
    `var serviceLassoCreatorMappingsAfter [${imageModules}]serviceLassoCreatorImageMappingStorage`,
    `var serviceLassoCreatorNamesBeforeOriginal [${imageModules}]serviceLassoCreatorNameStorage`,
    `var serviceLassoCreatorNamesAfterOriginal [${imageModules}]serviceLassoCreatorNameStorage`,
    `var serviceLassoCreatorPathsBefore [${imageModules}][${imagePathUnits}]uint16`,
    `var serviceLassoCreatorPathsAfter [${imageModules}][${imagePathUnits}]uint16`,
    `var serviceLassoCreatorFilesBeforeOriginal [${imageModules}]serviceLassoCreatorFileStorage`,
    `var serviceLassoCreatorFilesAfterOriginal [${imageModules}]serviceLassoCreatorFileStorage`,
    `var serviceLassoCreatorMetadataBeforeOriginal [${imageModules}]serviceLassoCreatorMetadataStorage`,
    `var serviceLassoCreatorMetadataAfterOriginal [${imageModules}]serviceLassoCreatorMetadataStorage`,
    `var serviceLassoCreatorFileReadsBeforeOriginal [${imageModules}]serviceLassoCreatorFileReadStorage`,
    `var serviceLassoCreatorFileReadsAfterOriginal [${imageModules}]serviceLassoCreatorFileReadStorage`,
    `var serviceLassoCreatorNativePEBeforeOriginal [${imageModules}]serviceLassoCreatorNativePE`,
    `var serviceLassoCreatorNativePEAfterOriginal [${imageModules}]serviceLassoCreatorNativePE`,
    `var _ [${imageModules*176n}-unsafe.Sizeof(serviceLassoCreatorNativePEBeforeOriginal)]byte`,
    `var _ [unsafe.Sizeof(serviceLassoCreatorNativePEBeforeOriginal)-${imageModules*176n}]byte`,
    `var serviceLassoCreatorExtentsBeforeOriginal [${imageModules}]serviceLassoCreatorExtentStorage`,
    `var serviceLassoCreatorExtentsAfterOriginal [${imageModules}]serviceLassoCreatorExtentStorage`,
    `var serviceLassoCreatorWalksBeforeOriginal [${imageModules}]serviceLassoCreatorWalkStorage`,
    `var serviceLassoCreatorWalksAfterOriginal [${imageModules}]serviceLassoCreatorWalkStorage`,
    `var serviceLassoCreatorRegionQueries [${imageRegions}]serviceLassoCreatorRegionQuery`,
    `var serviceLassoCreatorRegionReads [${imageRegions}]serviceLassoCreatorMemoryCall`,
    `var serviceLassoCreatorRegionOriginals [${imageRegions}]serviceLassoCreatorRegionStorage`,
    `var serviceLassoCreatorFileReadCalls [${imageFileBytes}]serviceLassoCreatorFileReadCall`,
    `var _ [${imageFileReadRecordsBytes}-unsafe.Sizeof(serviceLassoCreatorFileReadCalls)]byte`,
    `var _ [unsafe.Sizeof(serviceLassoCreatorFileReadCalls)-${imageFileReadRecordsBytes}]byte`,
    '// Typed geometry only: no containing reservation or complete image graph.',
    `var _ [${imageModuleBytes}-unsafe.Sizeof(serviceLassoCreatorModulesBefore)]byte`,
    `var _ [unsafe.Sizeof(serviceLassoCreatorModulesBefore)-${imageModuleBytes}]byte`, '');
  for (const [index, capacity] of dimensions.entries()) {
    const identifier = identifiers[index];
    const extent = Math.ceil((8 + capacity) / 8) * 8;
    staticBytes += extent;
    go.push(`const serviceLasso${identifier}Capacity = ${capacity}`,
      `type serviceLasso${identifier}Region struct { Anchor uint64; Bytes [${capacity}]byte }`,
      `var serviceLasso${identifier}Original serviceLasso${identifier}Region`,
      `var _ [${extent}-unsafe.Sizeof(serviceLasso${identifier}Region{})]byte`,
      `var _ [unsafe.Sizeof(serviceLasso${identifier}Region{})-${extent}]byte`,
      `var _ [8-unsafe.Alignof(serviceLasso${identifier}Region{})]byte`,
      `var _ [unsafe.Alignof(serviceLasso${identifier}Region{})-8]byte`,
      `var _ [8-unsafe.Offsetof(serviceLasso${identifier}Region{}.Bytes)]byte`,
      `var _ [unsafe.Offsetof(serviceLasso${identifier}Region{}.Bytes)-8]byte`, '');
    header.push(`#define SL_CHILD_${names[index].replace(/[A-Z]/gu, c => '_'+c.toLowerCase()).toUpperCase()}_CAPACITY UINT64_C(${capacity})`,
      `typedef struct sl_child_${names[index]}_region { uint64_t anchor; unsigned char bytes[${capacity}]; } sl_child_${names[index]}_region;`,
      `_Static_assert(sizeof(sl_child_${names[index]}_region)==${extent},"query region extent");`,
      `_Static_assert(_Alignof(sl_child_${names[index]}_region)==8,"query region alignment");`,
      `_Static_assert(offsetof(sl_child_${names[index]}_region,bytes)==8,"query region offset");`);
  }
  // String headers are ordinary typed Go globals, not numeric pointer words
  // in NOPTR/BSS. Native fill touches only the separate pointer-free regions;
  // publication and all immutable last-borrower lifetimes remain caller work.
  go.push(`const serviceLassoModuleUnitsIncludingNUL = ${M}`,
    `const serviceLassoModuleQueryCount = ${K}`,
    `const serviceLassoModuleQueryWCHARs = ${queryUnits}`,
    `const serviceLassoRawArgumentUnits = ${rawUnits}`,
    `const serviceLassoMaximumPublicArguments = ${maximumPublicArguments}`,
    `const serviceLassoArgumentBodyBytes = ${bodyBytes}`,
    `const serviceLassoBodyReadRecordsBytes = ${bodyReadRecordsBytes}`,
    `var serviceLassoBodyReadCalls [${bodyBytes}]serviceLassoBodyReadCall`,
    `var _ [${bodyReadRecordsBytes}-unsafe.Sizeof(serviceLassoBodyReadCalls)]byte`,
    `var _ [unsafe.Sizeof(serviceLassoBodyReadCalls)-${bodyReadRecordsBytes}]byte`,
    'type serviceLassoModuleQueryOriginal struct { Call serviceLassoNativeCallStorage; Supplied uint32; Count uint32; ProjectionFailure uint64 }',
    'var _ [416-unsafe.Sizeof(serviceLassoModuleQueryOriginal{})]byte',
    'var _ [unsafe.Sizeof(serviceLassoModuleQueryOriginal{})-416]byte',
    'var _ [400-unsafe.Offsetof(serviceLassoModuleQueryOriginal{}.Supplied)]byte',
    'var _ [unsafe.Offsetof(serviceLassoModuleQueryOriginal{}.Supplied)-400]byte',
    'var _ [404-unsafe.Offsetof(serviceLassoModuleQueryOriginal{}.Count)]byte',
    'var _ [unsafe.Offsetof(serviceLassoModuleQueryOriginal{}.Count)-404]byte',
    'var _ [408-unsafe.Offsetof(serviceLassoModuleQueryOriginal{}.ProjectionFailure)]byte',
    'var _ [unsafe.Offsetof(serviceLassoModuleQueryOriginal{}.ProjectionFailure)-408]byte',
    'var _ [16-unsafe.Sizeof(string(""))]byte',
    'var _ [unsafe.Sizeof(string(""))-16]byte',
    `var serviceLassoModuleQueries [${K}]serviceLassoModuleQueryOriginal`,
    `var serviceLassoModuleQueryUnits [${queryUnits}]uint16`,
    `var serviceLassoRawArgumentOriginal [${rawUnits}]uint16`,
    `var serviceLassoArgumentBodyOriginal [${bodyBytes}]byte`,
    `var serviceLassoArgumentUTF8Original [${utf8Bytes}]byte`,
    `var serviceLassoArgumentParsedOriginal [${utf8Bytes}]byte`,
    `var serviceLassoArgumentPublicOriginal [${maximumPublicArguments}]string`,
    '// Fallback needs exactly one string header even when its contents are empty.',
    'var serviceLassoArgumentFallbackOriginal [1]string',
    '// Fixed source reader/header subtotal only; full containing finance is separate.',
    'const serviceLassoHeaderReaderStaticBytes = 20448',
    'var _ [serviceLassoHeaderReaderStaticBytes-unsafe.Sizeof(serviceLassoHeaderReadOriginal)-unsafe.Sizeof(serviceLassoArgumentHeaderOriginal)]byte',
    'var _ [unsafe.Sizeof(serviceLassoHeaderReadOriginal)+unsafe.Sizeof(serviceLassoArgumentHeaderOriginal)-serviceLassoHeaderReaderStaticBytes]byte','');
  header.push(`#define SL_ARGUMENT_MODULE_UNITS_INCLUDING_NUL UINT64_C(${M})`,
    `#define SL_ARGUMENT_MODULE_QUERY_COUNT UINT64_C(${K})`,
    `#define SL_ARGUMENT_MODULE_QUERY_WCHARS UINT64_C(${queryUnits})`,
    `#define SL_ARGUMENT_RAW_UNITS UINT64_C(${rawUnits})`,
    `#define SL_ARGUMENT_BODY_BYTES UINT64_C(${bodyBytes})`,
    `#define SL_ARGUMENT_UTF8_BYTES UINT64_C(${utf8Bytes})`,
    `#define SL_ARGUMENT_MAXIMUM_PUBLIC_ARGUMENTS UINT64_C(${maximumPublicArguments})`,
    '#define SL_ARGUMENT_FALLBACK_ARGUMENTS UINT64_C(1)',
    `#define SL_ARGUMENT_WRITER_STACK_RESERVATION_BYTES UINT64_C(${writerStack})`,
    `#define SL_ARGUMENT_WRITER_TURN_CAPACITY UINT64_C(${writerTurns})`,
    '#endif', '');
  const argumentStaticBytes=416n*K+2n*queryUnits+2n*rawUnits+bodyBytes+2n*utf8Bytes+16n*maximumPublicArguments+16n;
  return Object.freeze({go: go.join('\n'), header: header.join('\n'),
    profileSHA256: digest, sourceCommit: profile.sourceCommit,
    providerProfileSHA256: profile.providerProfileSHA256, staticBytes,
    argumentStaticBytes:argumentStaticBytes.toString(),
    headerReaderStaticBytes:'20448',
    headerInterpretStaticBytes:'64',
    bodyReaderStaticBytes:(bodyReadRecordsBytes+48n).toString(),
    creatorImageBackingBytes:imageBackingBytes.toString(),
    creatorImageMappingBytes:(2n*imageModules*496n).toString(),
    creatorImageNameOriginalBytes:(2n*imageModules*456n).toString(),
    creatorImageFileOriginalBytes:(2n*imageModules*480n).toString(),
    creatorImageMetadataOriginalBytes:(2n*imageModules*1336n).toString(),
    creatorImageFileReadOriginalBytes:(2n*imageModules*96n+imageFileReadRecordsBytes+8n).toString(),
    creatorImageNativePEOriginalBytes:(2n*imageModules*176n).toString(),
    creatorImageExtentOriginalBytes:(2n*imageModules*472n).toString(),
    creatorImageRegionOriginalBytes:imageRegionOriginalBytes.toString(),
    // Named arrays/owners only; imports, stack, runtime and overlap are separate.
    creatorImageTypedGeometryBytes:imageGeometryBytes.toString(),
    argumentWriterDimensions:Object.freeze({stackReservationBytes:writerStack.toString(),turnCapacity:writerTurns.toString()}),
    classification: 'build-association-only'});
}
