import { createHash } from "node:crypto";

// Source-owned quotas, never configurable through raw argv/ENV or trace text.
export const PRIVATE_TRACE_MAX_BYTES = 1_048_576;
export const PRIVATE_TRACE_MAX_LINE_BYTES = 8_192;
export const PRIVATE_TRACE_MAX_LINES = 32_768;
const unavailable = disposition => ({ confidence: "untrusted-lexical", capture: disposition,
  origin: "unknown", reachedRead: "unknown", dispatch: "unknown", childParserIdentity: "unknown",
  matchedFrames: 0, unclassifiedFrames: 0, writerReadyReadTextMatched: false,
  lastMarkerMatches: [], lastNestedMarkerMatches: [] });

// No marker in combined stderr authenticates an executed command or its owner.
// Iterate bounded byte offsets; never convert/split a whole unbounded stream.
export function projectPrivateTrace(bytes) {
  if (!Buffer.isBuffer(bytes)) return unavailable("unavailable-input");
  if (bytes.length > PRIVATE_TRACE_MAX_BYTES) return unavailable("incomplete-byte-quota");
  const last = new Map(), lastNested = new Map();
  let matched = 0, unclassified = 0, writerReadyReadTextMatched = false, lines = 0;
  for (let start = 0; start < bytes.length;) {
    let end = bytes.indexOf(10, start);
    if (end < 0) end = bytes.length;
    if (++lines > PRIVATE_TRACE_MAX_LINES) return unavailable("incomplete-line-count-quota");
    if (end - start > PRIVATE_TRACE_MAX_LINE_BYTES) return unavailable("incomplete-line-byte-quota");
    const line = bytes.toString("utf8", start, end);
    start = end + 1;
    const frame = /^(\+{1,64})TUI20_TRACE_(caller|producer):([0-9]{1,6}): ?(.*)$/.exec(line);
    if (!frame) continue;
    const markerSourceLine = Number(frame[3]);
    if (!Number.isSafeInteger(markerSourceLine) || markerSourceLine < 1) continue;
    const command = frame[4];
    let commandTextCategory = "unclassified";
    if (/^read -r -t 1 -u 9 writer_ready$/.test(command)) commandTextCategory = "writer-ready-read";
    else if (/^wait(?: |$)/.test(command)) commandTextCategory = "wait-command";
    else if (/^jobs -pr$/.test(command)) commandTextCategory = "jobs-observation-command";
    else if (/^trap(?: |$)/.test(command)) commandTextCategory = "trap-command";
    else if (/^exec(?: |$)/.test(command)) commandTextCategory = "descriptor-command";
    else if (/^printf(?: |$)/.test(command)) commandTextCategory = "printf-command";
    else if (/^test(?: |$)/.test(command)) commandTextCategory = "test-command";
    else if (/^env -i(?: |$)/.test(command)) commandTextCategory = "fresh-environment-command";
    else if (/^python3 -(?: |$)/.test(command)) commandTextCategory = "python-stdin-command";
    else if (/^(?:uname|ps|awk|readlink|xargs)(?: |$)/.test(command)) commandTextCategory = "host-observation-command";
    else if (/^(?:git|sha256sum|cut|wc|go|dirname|mkdir)(?: |$)/.test(command)) commandTextCategory = "source-tool-command";
    matched++; if (commandTextCategory === "unclassified") unclassified++;
    const markerRole = frame[2];
    if (markerRole === "producer" && commandTextCategory === "writer-ready-read") writerReadyReadTextMatched = true;
    const observation = { markerRole, markerSourceLine, markerPrefixDepth: frame[1].length, commandTextCategory };
    last.set(markerRole, observation);
    if (observation.markerPrefixDepth > 1) lastNested.set(markerRole, observation);
  }
  return { ...unavailable("bounded-input-scanned"), matchedFrames: matched,
    unclassifiedFrames: unclassified, writerReadyReadTextMatched,
    lastMarkerMatches: ["caller", "producer"].flatMap(role => last.has(role) ? [last.get(role)] : []),
    lastNestedMarkerMatches: ["caller", "producer"].flatMap(role => lastNested.has(role) ? [lastNested.get(role)] : []) };
}

// Called only after the secondary spawn returns, on its original wx+ handle.
// No mutable pathname reopen and no readFile allocation from untrusted size.
export async function inspectPrivateTrace(file, project = false) {
  await file.sync();
  const before = await file.stat({ bigint: true });
  const size = before.size >= 0n && before.size <= BigInt(Number.MAX_SAFE_INTEGER) ? Number(before.size) : null;
  const incomplete = capture => ({ stream: { persistence: "fsynced-readback-incomplete", completeness: "incomplete", capture, sizeBeforeRead: size },
    ...(project ? { projection: unavailable(capture) } : {}) });
  if (!before.isFile() || size === null) return incomplete("unavailable-size-or-type");
  if (size > PRIVATE_TRACE_MAX_BYTES) return incomplete("incomplete-byte-quota");
  const bytes = Buffer.alloc(size + 1);
  let read = 0;
  while (read < bytes.length) {
    const { bytesRead } = await file.read(bytes, read, Math.min(65_536, bytes.length - read), read);
    if (bytesRead === 0) break;
    read += bytesRead;
  }
  const after = await file.stat({ bigint: true });
  if (read !== size || !after.isFile() || ["dev", "ino", "size", "mtimeNs", "ctimeNs"].some(key => before[key] !== after[key])) {
    return incomplete("incomplete-readback-changed");
  }
  const actual = bytes.subarray(0, size);
  return { stream: { persistence: "fsynced-readback", completeness: "complete-bounded-observation",
    sha256: createHash("sha256").update(actual).digest("hex"), size },
    ...(project ? { projection: projectPrivateTrace(actual) } : {}) };
}
