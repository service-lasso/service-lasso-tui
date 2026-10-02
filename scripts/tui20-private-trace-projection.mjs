// This is a lexical observation of private xtrace, never a parser-cause or
// process-custody receipt. No input-derived text crosses this boundary.
export function projectPrivateTrace(bytes) {
  const last = new Map(), lastNested = new Map();
  let recognized = 0, unclassified = 0, writerReadyReadObserved = false;
  for (const line of bytes.toString("utf8").split("\n")) {
    const frame = /^(\+{1,64})TUI20_TRACE_(caller|producer):([0-9]{1,6}): ?(.*)$/.exec(line);
    if (!frame) continue;
    const sourceLine = Number(frame[3]);
    if (!Number.isSafeInteger(sourceLine) || sourceLine < 1) continue;
    const command = frame[4];
    let category = "unclassified";
    if (/^read -r -t 1 -u 9 writer_ready$/.test(command)) category = "writer-ready-read";
    else if (/^wait(?: |$)/.test(command)) category = "wait-command";
    else if (/^jobs -pr$/.test(command)) category = "jobs-observation-command";
    else if (/^trap(?: |$)/.test(command)) category = "trap-command";
    else if (/^exec(?: |$)/.test(command)) category = "descriptor-command";
    else if (/^printf(?: |$)/.test(command)) category = "printf-command";
    else if (/^test(?: |$)/.test(command)) category = "test-command";
    else if (/^env -i(?: |$)/.test(command)) category = "fresh-environment-command";
    else if (/^python3 -(?: |$)/.test(command)) category = "python-stdin-command";
    else if (/^(?:uname|ps|awk|readlink|xargs)(?: |$)/.test(command)) category = "host-observation-command";
    else if (/^(?:git|sha256sum|cut|wc|go|dirname|mkdir)(?: |$)/.test(command)) category = "source-tool-command";
    recognized++; if (category === "unclassified") unclassified++;
    const role = frame[2];
    if (role === "producer" && category === "writer-ready-read") writerReadyReadObserved = true;
    const observation = { role, sourceLine, traceDepth: frame[1].length, category };
    last.set(role, observation);
    if (observation.traceDepth > 1) lastNested.set(role, observation);
  }
  // Fixed role order and fixed field names. Counts are observations, not proof
  // that verbose source lines or multiline command expansions were understood.
  return { confidence: "lexical-incomplete", recognizedFrames: recognized,
    unclassifiedFrames: unclassified, writerReadyReadObserved,
    lastCommands: ["caller", "producer"].flatMap(role => last.has(role) ? [last.get(role)] : []),
    lastNestedCommands: ["caller", "producer"].flatMap(role => lastNested.has(role) ? [lastNested.get(role)] : []) };
}
