import assert from "node:assert/strict";
import { createHash } from "node:crypto";
import { access, lstat, mkdtemp, mkdir, open, readFile, realpath, rm, stat, symlink, writeFile } from "node:fs/promises";
import { constants } from "node:fs";
import { spawnSync } from "node:child_process";
import { tmpdir } from "node:os";
import path from "node:path";
import test from "node:test";
import { fileURLToPath } from "node:url";
import { projectPrivateTrace, inspectPrivateTrace, PRIVATE_TRACE_MAX_BYTES, PRIVATE_TRACE_MAX_LINE_BYTES, PRIVATE_TRACE_MAX_LINES } from "./tui20-private-trace-projection.mjs";

const scripts = path.dirname(fileURLToPath(import.meta.url));
const digest = bytes => createHash("sha256").update(bytes).digest("hex");
const quote = value => `'${value.replaceAll("'", "'\\''")}'`;

test("private trace projector keeps hostile values and verbose source outside its closed lexical output", () => {
  const secret = "private-token-/host/path-https://private.invalid";
  const raw = Buffer.from(`PS4=${secret}\n+TUI20_TRACE_caller:2: env -i TOKEN=${secret}\n++TUI20_TRACE_producer:18: ps -o comm= -p ${secret}\n+TUI20_TRACE_producer:90: read -r -t 1 -u 9 writer_ready\n${secret}\n`);
  assert.deepEqual(projectPrivateTrace(raw), { confidence: "untrusted-lexical", capture: "bounded-input-scanned", origin: "unknown", reachedRead: "unknown", dispatch: "unknown", childParserIdentity: "unknown", matchedFrames: 3, unclassifiedFrames: 0, writerReadyReadTextMatched: true,
    lastMarkerMatches: [{ markerRole: "caller", markerSourceLine: 2, markerPrefixDepth: 1, commandTextCategory: "fresh-environment-command" },
      { markerRole: "producer", markerSourceLine: 90, markerPrefixDepth: 1, commandTextCategory: "writer-ready-read" }],
    lastNestedMarkerMatches: [{ markerRole: "producer", markerSourceLine: 18, markerPrefixDepth: 2, commandTextCategory: "host-observation-command" }] });
  assert.ok(!JSON.stringify(projectPrivateTrace(raw)).includes(secret));
  const hostile = Buffer.from(`+TUI20_TRACE_foreign:1: ${secret}\n+TUI20_TRACE_producer:9999999: ${secret}\n+TUI20_TRACE_producer:0: read -r -t 1 -u 9 writer_ready\n${"+".repeat(65)}TUI20_TRACE_producer:1: ${secret}\n+TUI20_TRACE_producer:7: ${secret}\n+TUI20_TRACE_producer:8: read -r -t 1 -u 9 writer_ready ${secret}\n`);
  assert.deepEqual(projectPrivateTrace(hostile), { confidence: "untrusted-lexical", capture: "bounded-input-scanned", origin: "unknown", reachedRead: "unknown", dispatch: "unknown", childParserIdentity: "unknown", matchedFrames: 2, unclassifiedFrames: 2, writerReadyReadTextMatched: false,
    lastMarkerMatches: [{ markerRole: "producer", markerSourceLine: 8, markerPrefixDepth: 1, commandTextCategory: "unclassified" }], lastNestedMarkerMatches: [] });
  assert.ok(!JSON.stringify(projectPrivateTrace(hostile)).includes(secret));
  assert.deepEqual(projectPrivateTrace(Buffer.from(secret)), { confidence: "untrusted-lexical", capture: "bounded-input-scanned", origin: "unknown", reachedRead: "unknown", dispatch: "unknown", childParserIdentity: "unknown", matchedFrames: 0, unclassifiedFrames: 0, writerReadyReadTextMatched: false, lastMarkerMatches: [], lastNestedMarkerMatches: [] });
});


test("private held trace readback treats forged PS4 argv ENV verbose and stderr as unauthenticated text", async t => {
  const root = await mkdtemp(path.join(await realpath(tmpdir()), "tui20-private-trace-forgery-"));
  let complete = false;
  t.after(() => complete ? rm(root, { recursive: true, force: true }) : undefined);
  const secret = "private-token-/host/path-https://private.invalid";
  const marker = "+TUI20_TRACE_producer:90: read -r -t 1 -u 9 writer_ready";
  // Inert bytes: none of these marker-looking strings is an executed read.
  const vectors = [marker, `PS4='${marker}'\n${marker}`, `argv='${secret}\n${marker}\n'`,
    `ENV='${secret}\n${marker}\n'`, `printf '%s' '${secret}\n${marker}\n'`, `command stderr\n${marker}`];
  for (let i = 0; i < vectors.length; i++) {
    const bytes = Buffer.from(vectors[i]);
    const file = await open(path.join(root, `${i}.private`), "wx+", 0o600);
    try {
      await file.writeFile(bytes);
      const observation = await inspectPrivateTrace(file, true);
      assert.equal(observation.stream.sha256, digest(bytes));
      assert.equal(observation.stream.size, bytes.length);
      assert.equal(observation.stream.completeness, "complete-bounded-observation");
      assert.equal(observation.projection.writerReadyReadTextMatched, true);
      for (const field of ["origin", "reachedRead", "dispatch", "childParserIdentity"]) assert.equal(observation.projection[field], "unknown");
      const publicText = JSON.stringify(observation);
      assert.ok(!publicText.includes(secret));
      assert.ok(!publicText.includes(marker));
      assert.ok(!publicText.includes("writerReadyReadObserved"));
      assert.deepEqual(await readFile(path.join(root, `${i}.private`)), bytes, "private originals remain retained and unchanged");
      if (process.platform !== "win32") assert.equal((await file.stat()).mode & 0o777, 0o600);
    } finally { await file.close(); }
  }
  complete = true;
});

test("private trace quotas reject actual oversize before reads and bound line conversion and counts", async t => {
  const root = await mkdtemp(path.join(await realpath(tmpdir()), "tui20-private-trace-quota-"));
  let complete = false;
  t.after(() => complete ? rm(root, { recursive: true, force: true }) : undefined);
  const filename = path.join(root, "oversized.private");
  const file = await open(filename, "wx+", 0o600);
  try {
    // Actual quota+1 bytes deny before readback; no huge disk allocation or sparse claim.
    await file.truncate(PRIVATE_TRACE_MAX_BYTES + 1);
    let reads = 0;
    const held = { sync: () => file.sync(), stat: options => file.stat(options),
      read: () => { reads++; throw new Error("oversize must deny before any read/allocation based on its size"); } };
    const observed = await inspectPrivateTrace(held, true);
    assert.equal(reads, 0);
    assert.equal(observed.stream.sizeBeforeRead, PRIVATE_TRACE_MAX_BYTES + 1);
    assert.equal(observed.stream.completeness, "incomplete");
    assert.equal(observed.stream.capture, "incomplete-byte-quota");
    assert.ok(!Object.hasOwn(observed.stream, "sha256"));
    assert.equal(observed.projection.capture, "incomplete-byte-quota");
    assert.equal((await stat(filename)).size, PRIVATE_TRACE_MAX_BYTES + 1, "quota never truncates/deletes retained raw failure evidence");
    // Explicit metadata-only surrogate for a huge size, never physical allocation
    // or actual huge-file/native no-OOM proof. Real quota+1 IO control above remains.
    const actualStat = await file.stat({ bigint: true });
    const hugeStat = Object.assign(Object.create(Object.getPrototypeOf(actualStat)), actualStat, { size: 2_147_483_648n });
    const huge = await inspectPrivateTrace({ ...held, stat: async () => hugeStat }, true);
    assert.equal(reads, 0, "huge-size metadata surrogate denies before read");
    assert.equal(huge.stream.sizeBeforeRead, 2_147_483_648);
    assert.equal(huge.stream.capture, "incomplete-byte-quota");
    assert.ok(!Object.hasOwn(huge.stream, "sha256"));
    await file.truncate(PRIVATE_TRACE_MAX_BYTES);
    const boundary = await inspectPrivateTrace(file, true);
    assert.equal(boundary.stream.completeness, "complete-bounded-observation");
    assert.match(boundary.stream.sha256, /^[a-f0-9]{64}$/);
    assert.equal(boundary.projection.capture, "incomplete-line-byte-quota");
    await file.truncate(PRIVATE_TRACE_MAX_BYTES + 1);
    assert.equal((await inspectPrivateTrace(file, true)).stream.capture, "incomplete-byte-quota");
  } finally { await file.close(); }
  const marker = Buffer.from("+TUI20_TRACE_producer:90: read -r -t 1 -u 9 writer_ready\n");
  for (const [bytes, capture] of [[Buffer.alloc(PRIVATE_TRACE_MAX_BYTES + 1), "incomplete-byte-quota"],
    [Buffer.concat([marker, Buffer.alloc(PRIVATE_TRACE_MAX_LINE_BYTES + 1, 97)]), "incomplete-line-byte-quota"],
    [Buffer.alloc(PRIVATE_TRACE_MAX_LINES + 1, 10), "incomplete-line-count-quota"]]) {
    const observed = projectPrivateTrace(bytes);
    assert.equal(observed.capture, capture);
    assert.equal(observed.matchedFrames, 0, "incomplete quota never publishes a partial match set");
    assert.equal(observed.writerReadyReadTextMatched, false);
    assert.deepEqual(observed.lastMarkerMatches, []);
  }
  assert.equal(projectPrivateTrace(Buffer.alloc(PRIVATE_TRACE_MAX_LINE_BYTES, 97)).capture, "bounded-input-scanned");
  assert.equal(projectPrivateTrace(Buffer.alloc(PRIVATE_TRACE_MAX_LINES, 10)).capture, "bounded-input-scanned");
  complete = true;
});

test("held trace readback rejects growth short read identity changes and real IO errors without a full hash", async t => {
  const root = await mkdtemp(path.join(await realpath(tmpdir()), "tui20-private-trace-readback-"));
  let complete = false;
  t.after(() => complete ? rm(root, { recursive: true, force: true }) : undefined);
  const file = await open(path.join(root, "trace.private"), "wx+", 0o600);
  try {
    await file.writeFile("private original");
    for (const fault of ["growth", "short-read", "identity-change"]) {
      let stats = 0, injected = false;
      const held = { sync: () => file.sync(), stat: async options => {
        const actual = await file.stat(options);
        if (++stats === 2 && fault === "identity-change") return Object.assign(Object.create(Object.getPrototypeOf(actual)), actual, { ino: actual.ino + 1n });
        return actual;
      }, read: async (...args) => {
        if (fault === "short-read") return { bytesRead: 0 };
        if (fault === "growth" && !injected) { injected = true; await file.write(Buffer.from("extra"), 0, 5, 16); }
        return file.read(...args);
      } };
      const observed = await inspectPrivateTrace(held, true);
      assert.equal(observed.stream.capture, "incomplete-readback-changed", fault);
      assert.equal(observed.stream.completeness, "incomplete");
      assert.ok(!Object.hasOwn(observed.stream, "sha256"));
      assert.equal(observed.projection.matchedFrames, 0);
      await file.truncate(16);
    }
    // Actual closed-descriptor failure, not manufactured success/receipt.
    await file.close();
    await assert.rejects(() => inspectPrivateTrace(file, true));
    assert.equal((await stat(path.join(root, "trace.private"))).size, 16, "IO failure retains original raw file");
  } finally { if (file.fd !== -1) await file.close(); }
  complete = true;
});
test("actual fresh native build rejects persisted flags and ambient workspace influence", async t => {
  assert.ok(["linux", "darwin"].includes(process.platform));
  const root = await mkdtemp(path.join(await realpath(tmpdir()), "tui20-native-go-admission-"));
  const checkout = path.join(root, "checkout"), home = path.join(root, "home"), phase = path.join(root, "phase");
  let completed = false;
  t.after(async () => {
    // Failed fixtures remain private evidence. Only the cache created by this
    // successful real build under this owned HOME is eligible for cleanup.
    if (!completed) return;
    const moduleCache = path.join(home, "go/pkg/mod");
    for (const directory of [root, home, path.join(home, "go"), path.join(home, "go/pkg"), moduleCache]) {
      const entry = await lstat(directory);
      assert.ok(entry.isDirectory() && !entry.isSymbolicLink(), "owned module-cache ancestry must remain directories");
      assert.equal(await realpath(directory), directory);
    }
    const cleanupEnvironment = { ...ambient, GOENV: "off", GOMODCACHE: moduleCache };
    assert.equal(run("go", ["env", "GOMODCACHE"], checkout, cleanupEnvironment), moduleCache);
    // Go removes its read-only module files itself; no shared-cache sweep or
    // permission change is made. Any failure retains the root and fails the hook.
    run("go", ["clean", "-modcache"], checkout, cleanupEnvironment);
    await assert.rejects(() => lstat(moduleCache), { code: "ENOENT" });
    await rm(root, { recursive: true, force: true });
  });
  await Promise.all([mkdir(home), mkdir(phase)]);
  const repository = path.resolve(scripts, "..");
  const commit = run("git", ["rev-parse", "HEAD"], repository);
  run("git", ["init", "--quiet", checkout], root);
  run("git", ["-C", checkout, "fetch", "--no-tags", "--depth=1", repository, commit], root);
  run("git", ["-C", checkout, "checkout", "--detach", "--quiet", "FETCH_HEAD"], root);
  const ambient = { ...process.env, HOME: home, GOFLAGS: "", GOWORK: "off" };
  delete ambient.GOENV; delete ambient.XDG_CONFIG_HOME;
  const config = run("go", ["env", "GOENV"], checkout, ambient);
  await mkdir(path.dirname(config), { recursive: true });
  const overlay = path.join(root, "overlay.json"), foreign = path.join(root, "foreign");
  await mkdir(foreign);
  await writeFile(path.join(foreign, "go.mod"), "module github.com/service-lasso/service-lasso-tui\n\ngo 1.26.0\n");
  await writeFile(path.join(root, "go.work"), "go 1.26.0\nuse ./foreign\n");
  await writeFile(overlay, JSON.stringify({ Replace: { [path.join(checkout, "cmd/service-lasso-tui/main.go")]: path.join(root, "missing-foreign.go") } }));
  await writeFile(config, `GOFLAGS=-overlay=${overlay}\n`);
  // Confirm these real ambient inputs would be active without the phase's
  // explicit environment. No synthetic effective-Go receipt is supplied.
  const uncontrolled = { ...process.env, HOME: home }; delete uncontrolled.GOFLAGS; delete uncontrolled.GOWORK; delete uncontrolled.GOENV; delete uncontrolled.XDG_CONFIG_HOME;
  assert.equal(run("go", ["env", "GOFLAGS"], checkout, uncontrolled), `-overlay=${overlay}`);
  assert.equal(run("go", ["env", "GOWORK"], checkout, uncontrolled), path.join(root, "go.work"));
  const workflow = (await readFile(path.join(scripts, "../.github/workflows/ci.yml"), "utf8")).replaceAll("\r\n", "\n");
  const envStart = workflow.indexOf('            env -i PATH="$PATH"');
  const envEnd = workflow.indexOf("\n", envStart);
  const buildStart = workflow.indexOf('          binary="$phase/service-lasso-tui"', envEnd);
  const buildEnd = workflow.indexOf('          TUI20_BINARY_DIGEST=', buildStart);
  assert.ok(envStart >= 0 && buildStart > envEnd && buildEnd > buildStart);
  const freshEnvironment = workflow.slice(envStart, envEnd).trimStart();
  const actualBuild = workflow.slice(buildStart, buildEnd).split("\n").map(line => line.startsWith("          ") ? line.slice(10) : line).join("\n");
  assert.ok(actualBuild.indexOf("node scripts/assert-go-source-provenance.mjs") < actualBuild.indexOf("go build "));
  const invoke = () => spawnSync("bash", ["-euo", "pipefail", "-c", `phase=${quote(phase)}\n${freshEnvironment}\nphase="$PHASE"\n${actualBuild}TUI20_PHASE\n`], { cwd: checkout, env: { ...uncontrolled, CI_SOURCE_SHA: commit, CORE_COMMIT: commit }, encoding: "utf8" });
  const rejected = invoke();
  assert.equal(rejected.error, undefined);
  assert.notEqual(rejected.status, 0, "persisted Go flags must deny actual native build");
  assert.match(rejected.stderr, /effective GOFLAGS are not empty/);
  await assert.rejects(() => stat(path.join(phase, "service-lasso-tui")), { code: "ENOENT" });
  await writeFile(config, "");
  const observed = invoke();
  assert.equal(observed.error, undefined);
  assert.equal(observed.status, 0, observed.stderr);
  assert.match(observed.stdout, /"goflags":"","gowork":"off"/);
  assert.ok((await stat(path.join(phase, "service-lasso-tui"))).size > 0, "actual admitted native build must run");
  completed = true;
});
function run(program, args, cwd, env = process.env) {
  const result = spawnSync(program, args, { cwd, env, encoding: "utf8" });
  assert.equal(result.error, undefined);
  assert.equal(result.status, 0, result.stderr);
  return result.stdout.trim();
}

test("actual workflow custody producer preserves literals and consumes every real tab tool record with genuine Git trees", async t => {
  assert.ok(["linux", "darwin"].includes(process.platform), "this actual POSIX producer guard must run on Linux or macOS");
  const root = await mkdtemp(path.join(await realpath(tmpdir()), "tui20-custody-boundary-"));
  let completed = false;
  t.after(() => completed ? rm(root, { recursive: true, force: true }) : undefined);
  const source = path.join(root, "source");
  // Spaces and a dollar expression are pathname bytes, never shell input.
  const phase = path.join(root, "phase $literal space");
  await mkdir(source);
  await writeFile(path.join(source, "fixture.txt"), "actual Git tree and actual tool inventory\n");
  const gitEnvironment = { ...process.env, GIT_AUTHOR_DATE: "2026-10-02T00:00:00Z", GIT_COMMITTER_DATE: "2026-10-02T00:00:00Z" };
  const initialize = cwd => {
    run("git", ["init", "--quiet", "--object-format=sha1", "--initial-branch=develop"], cwd);
    run("git", ["add", "fixture.txt"], cwd);
    run("git", ["-c", "user.name=Native custody fixture", "-c", "user.email=native-custody@example.invalid", "-c", "commit.gpgsign=false", "commit", "--quiet", "-m", "actual tree fixture"], cwd, gitEnvironment);
    return run("git", ["rev-parse", "HEAD"], cwd);
  };
  const commit = initialize(source);
  const tree = run("git", ["rev-parse", "HEAD^{tree}"], source);
  assert.match(tree, /^[a-f0-9]{40}$/);
  const workflow = (await readFile(path.join(scripts, "../.github/workflows/ci.yml"), "utf8")).replaceAll("\r\n", "\n");
  const envStart = workflow.indexOf('            env -i PATH="$PATH"');
  // The quoted heredoc body is at YAML's run indentation (10 spaces).
  const actualCustodyEnd = workflow.indexOf('          git -C "$phase/core-source" init -q', envStart);
  assert.ok(envStart >= 0 && actualCustodyEnd > envStart, "actual native producer boundary moved; update the guard explicitly");
  const producer = workflow.slice(envStart, actualCustodyEnd).split("\n").map(line => line.startsWith("          ") ? line.slice(10) : line).join("\n");
  assert.match(producer, /bash -euo pipefail <<'TUI20_PHASE'/);
  // Consume the owned ready FIFO directly; do not replace the Bash script's
  // fd0 during a builtin. The complete actual runtime producer and adverse
  // child/parent closure assertions below remain the mandatory evidence.
  assert.equal(producer.split("while ! read -r -t 1 -u 9 writer_ready; do").length, 2);
  assert.ok(!producer.includes("read -r -t 1 writer_ready <&9"));
  // Select exactly the existing PATH parser, then invoke that explicit image
  // at both caller and env-i seams. This binds evidence; it changes no parser.
  let parser;
  for (const entry of process.env.PATH.split(path.delimiter)) {
    const candidate = path.resolve(source, entry, "bash");
    try { await access(candidate, constants.X_OK); if ((await stat(candidate)).isFile()) { parser = await realpath(candidate); break; } }
    catch (error) { if (!["ENOENT", "ENOTDIR", "EACCES"].includes(error.code)) throw error; }
  }
  assert.ok(parser, "actual PATH-selected Bash must be available");
  const parserBytes = await readFile(parser);
  const parserVersion = run(parser, ["--version"], source);
  const diagnostics = path.join(root, "private-parser-diagnostics");
  await mkdir(diagnostics, { mode: 0o700 });
  const persist = async (name, bytes) => {
    const file = await open(path.join(diagnostics, name), "wx", 0o600);
    try { await file.writeFile(bytes); await file.sync(); } finally { await file.close(); }
    const directory = await open(diagnostics, "r");
    try { await directory.sync(); } finally { await directory.close(); }
  };
  let invocation = 0;
  // t.diagnostic may be buffered until test completion. Write a TAP comment to
  // the existing stdout channel and await its write callback before spawning.
  // This is caller-stream completion, not a provider delivery acknowledgement.
  const emitDiagnostic = value => new Promise((resolve, reject) => {
    process.stdout.write(`# ${JSON.stringify(value)}\n`, error => error ? reject(new Error("safe diagnostic output failed")) : resolve());
  });
  const invokeProducer = async (selectedPhase, selectedProducer, timeout) => {
    const name = String(++invocation);
    const explicitProducer = selectedProducer.replace(" bash -euo pipefail <<'TUI20_PHASE'", ` ${quote(parser)} -euo pipefail <<'TUI20_PHASE'`);
    assert.notEqual(explicitProducer, selectedProducer, "actual inner parser seam must be explicitly bound");
    const callerSource = Buffer.from(`phase=${quote(selectedPhase)}\n${explicitProducer}TUI20_PHASE\n`);
    const fedSource = Buffer.from(selectedProducer.slice(selectedProducer.indexOf("\n") + 1));
    assert.deepEqual(Buffer.from(explicitProducer.slice(explicitProducer.indexOf("\n") + 1)), fedSource, "parser binding must not alter any byte fed to the inner producer");
    const environment = { ...process.env, CI_SOURCE_SHA: commit, CORE_COMMIT: commit };
    const freshEnvironment = { PATH: environment.PATH, HOME: environment.HOME, LANG: environment.LANG || "C.UTF-8", LC_ALL: environment.LC_ALL || "C.UTF-8", GOFLAGS: "", GOWORK: "off", CI_SOURCE_SHA: commit, CORE_COMMIT: commit, PHASE: selectedPhase, SERVICE_LASSO_WORKSPACE_ROOT: path.join(selectedPhase, "workspace"), SERVICE_LASSO_INSTANCE_REGISTRY_PATH: path.join(selectedPhase, "registry/instances.json"), SERVICE_LASSO_HOST_PORT_REGISTRY_PATH: path.join(selectedPhase, "registry/ports.json") };
    const args = ["-euo", "pipefail", "-c", callerSource.toString("utf8")];
    // Only closed versions, hashes and result categories enter existing test
    // output. Raw paths/version text/argv/environment remain owner-private.
    const parserVersionNumber = /^GNU bash, version ([0-9]+(?:\.[0-9]+)+(?:\([0-9]+\))?)/.exec(parserVersion)?.[1] || "unclassified";
    const safeBinding = { schemaVersion: 1, kind: "tui20-parser-diagnostic", invocation: invocation,
      platform: process.platform, parser: { sha256: digest(parserBytes), size: parserBytes.length, version: parserVersionNumber, versionSHA256: digest(Buffer.from(parserVersion)) },
      node: { version: /^v[0-9]+\.[0-9]+\.[0-9]+$/.test(process.version) ? process.version : "unclassified", imageSHA256: digest(await readFile(process.execPath)), sourceSHA256: digest(await readFile(fileURLToPath(import.meta.url))) },
      workflowSHA256: digest(await readFile(path.join(scripts, "../.github/workflows/ci.yml"))), callerSourceSHA256: digest(callerSource), producerSourceSHA256: digest(Buffer.from(selectedProducer)), fedSourceSHA256: digest(fedSource), fedSourceSize: fedSource.length };
    await emitDiagnostic({ ...safeBinding, stage: "before-invocation", result: "pending" });
    // Private exact bytes/environment are fsynced BEFORE parsing can fail.
    // None of these files is a public receipt or an uploaded artifact.
    try {
      await persist(`${name}-caller.source`, callerSource);
      await persist(`${name}-producer.source`, Buffer.from(selectedProducer));
      await persist(`${name}-fed.source`, fedSource);
      await persist(`${name}-binding.json`, JSON.stringify({ classification: "owner-private-parser-diagnostic", platform: process.platform, parser: { path: parser, sha256: digest(parserBytes), size: parserBytes.length, version: parserVersion }, caller: { path: process.execPath, version: process.version, sha256: digest(await readFile(process.execPath)), sourceSHA256: digest(await readFile(fileURLToPath(import.meta.url))) }, workflowSHA256: digest(await readFile(path.join(scripts, "../.github/workflows/ci.yml"))), extraction: "LF normalization; env-i through pre-Core git init; remove ten-space YAML prefix; append closing delimiter", cwd: source, argv: args, innerArgv: ["-euo", "pipefail"], environment, freshEnvironment, callerSourceSHA256: digest(callerSource), fedSourceSHA256: digest(fedSource), fedSourceSize: fedSource.length }));
    } catch {
      try { await emitDiagnostic({ ...safeBinding, stage: "private-preflight-persistence", result: "failed", invocationReached: false }); } catch { /* no invocation occurred */ }
      throw new Error("private parser diagnostic preflight persistence failed");
    }
    const result = spawnSync(parser, args, { cwd: source, env: environment, encoding: "utf8", ...(timeout === undefined ? {} : { timeout }) });
    result.diagnosticFailures = [];
    for (const [suffix, bytes] of [["stdout.private", result.stdout || ""], ["stderr.private", result.stderr || ""], ["result.json", JSON.stringify({ status: result.status, signal: result.signal, errorCode: result.error?.code || null, error: result.error ? { name: result.error.name, message: result.error.message, stack: result.error.stack, code: result.error.code } : null })]]) {
      try { await persist(`${name}-${suffix}`, bytes); }
      catch { result.diagnosticFailures.push({ sink: suffix, code: "persistence_failed" }); }
    }
    let parserReadback = "unavailable";
    try { parserReadback = digest(await readFile(parser)) === safeBinding.parser.sha256 ? "matched" : "changed"; }
    catch { result.diagnosticFailures.push({ sink: "parser-image-readback", code: "readback_failed" }); }
    if (parserReadback === "changed") result.diagnosticFailures.push({ sink: "parser-image-readback", code: "image_changed" });
    try { await emitDiagnostic({ ...safeBinding, stage: "after-invocation", invocationReached: true,
      result: result.error ? "spawn_failed" : result.signal ? "signaled" : result.status === 0 ? "exited_zero" : Number.isInteger(result.status) ? "exited_nonzero" : "exit_unknown",
      exitStatus: Number.isInteger(result.status) ? result.status : null, parserImageReadback: parserReadback, diagnosticFailures: result.diagnosticFailures }); }
    catch { result.diagnosticFailures.push({ sink: "safe-output", code: "output_failed" }); }
    // One paired parse-only experiment on the original producer, AFTER its
    // mandatory invocation. Both routes receive the identical fed buffer and
    // fresh environment; only stdin versus -c differs. GNU Bash -n reads without
    // executing commands. This cannot reproduce expansion/runtime behavior or
    // establish equivalence to the original outer caller/heredoc context.
    if (invocation === 1) {
      for (const route of ["stdin", "command-string"]) {
        const parseArgs = ["-n", "-euo", "pipefail", ...(route === "command-string" ? ["-c", fedSource.toString("utf8")] : [])];
        const binding = { schemaVersion: 1, kind: "tui20-parser-differential", hypothesis: "fed-parse-route-independent",
          route, parserSHA256: safeBinding.parser.sha256, fedSourceSHA256: digest(fedSource), fedSourceSize: fedSource.length,
          environmentSHA256: digest(Buffer.from(JSON.stringify(freshEnvironment))), options: ["-n", "-euo", "pipefail"] };
        let reached = false;
        try {
          if (!Buffer.from(fedSource.toString("utf8")).equals(fedSource)) throw new Error("fed source is not lossless UTF-8");
          if (digest(await readFile(parser)) !== binding.parserSHA256) throw new Error("bound parser image changed before parse");
          // Each route independently persists its exact argv/environment/input
          // before any parsing. Failure denies that route; no private values
          // are projected onto stdout and no new upload channel is introduced.
          await persist(`${name}-${route}-preflight.json`, JSON.stringify({ classification: "owner-private-parser-differential", parser, cwd: source,
            argv: parseArgs, environment: freshEnvironment, fedSourceSHA256: digest(fedSource), fedSourceSize: fedSource.length }));
          await persist(`${name}-${route}-fed.source`, fedSource);
          await emitDiagnostic({ ...binding, stage: "before-parse", result: "pending" });
          const parsed = spawnSync(parser, parseArgs, { cwd: source, env: freshEnvironment, input: route === "stdin" ? fedSource : Buffer.alloc(0), encoding: "utf8" });
          reached = true;
          const failures = [];
          for (const [suffix, bytes] of [["stdout.private", parsed.stdout || ""], ["stderr.private", parsed.stderr || ""],
            ["result.private.json", JSON.stringify({ status: parsed.status, signal: parsed.signal, error: parsed.error ? { message: parsed.error.message, code: parsed.error.code } : null })]]) {
            try { await persist(`${name}-${route}-${suffix}`, bytes); }
            catch { failures.push({ sink: suffix, code: "persistence_failed" }); }
          }
          let image = "unavailable";
          try { image = digest(await readFile(parser)) === binding.parserSHA256 ? "matched" : "changed"; }
          catch { failures.push({ sink: "parser-image-readback", code: "readback_failed" }); }
          if (image === "changed") failures.push({ sink: "parser-image-readback", code: "image_changed" });
          await emitDiagnostic({ ...binding, stage: "after-parse", invocationReached: true,
            result: parsed.error ? "spawn_failed" : parsed.signal ? "signaled" : parsed.status === 0 ? "exited_zero" : Number.isInteger(parsed.status) ? "exited_nonzero" : "exit_unknown",
            exitStatus: Number.isInteger(parsed.status) ? parsed.status : null, parserImageReadback: image, diagnosticFailures: failures });
          result.diagnosticFailures.push(...failures.map(failure => ({ ...failure, route })));
        } catch {
          result.diagnosticFailures.push({ sink: "parser-differential", route, code: "diagnostic_failed" });
          try { await emitDiagnostic({ ...binding, stage: "diagnostic-failure", result: "failed", invocationReached: reached }); }
          catch { result.diagnosticFailures.push({ sink: "safe-output", route, code: "output_failed" }); }
        }
      }
    }
    // One materially new runtime observation, only on the original observed
    // Darwin failure. It is inside the already-retained failed fixture, never
    // a retry, primary replacement, or successful-run retention mechanism.
    if (invocation === 1 && process.platform === "darwin" && Number.isInteger(result.status) && result.status !== 0) {
      const tracePhase = path.join(root, "secondary-private-trace-phase");
      const callerPS4 = '+TUI20_TRACE_caller:${LINENO}: ';
      const producerPS4 = '+TUI20_TRACE_producer:${LINENO}: ';
      const tracedProducer = explicitProducer.replace("env -i PATH=", `env -i PS4=${quote(producerPS4)} PATH=`)
        .replace(` ${quote(parser)} -euo pipefail <<'TUI20_PHASE'`, ` ${quote(parser)} -x -v -euo pipefail <<'TUI20_PHASE'`);
      const tracedCaller = Buffer.from(`phase=${quote(tracePhase)}\n${tracedProducer}TUI20_PHASE\n`);
      const traceEnvironment = { ...environment, PS4: callerPS4 };
      const traceArgs = ["-x", "-v", "-euo", "pipefail", "-c", tracedCaller.toString("utf8")];
      const binding = { schemaVersion: 1, kind: "tui20-private-runtime-trace", primaryInvocation: 1,
        parserSHA256: safeBinding.parser.sha256, originalCallerSHA256: digest(callerSource), callerSHA256: digest(tracedCaller),
        fedSourceSHA256: digest(fedSource), fedSourceSize: fedSource.length, options: ["-x", "-v", "-euo", "pipefail"] };
      let reached = false, stdoutFile, stderrFile;
      const failures = [];
      try {
        assert.notEqual(tracedProducer, explicitProducer);
        assert.ok(tracedProducer.split("\n")[0].includes(`env -i PS4=${quote(producerPS4)} PATH=`), "inner private trace role seam must be reached");
        assert.ok(tracedProducer.split("\n")[0].includes(` ${quote(parser)} -x -v -euo pipefail <<'TUI20_PHASE'`), "inner selected parser instrumentation seam must be reached");
        assert.deepEqual(Buffer.from(tracedProducer.slice(tracedProducer.indexOf("\n") + 1)), fedSource, "secondary trace must preserve every fed byte");
        await assert.rejects(() => lstat(tracePhase), { code: "ENOENT" });
        if (digest(await readFile(parser)) !== binding.parserSHA256) throw new Error("bound trace parser changed");
        await persist(`${name}-trace-caller.source`, tracedCaller);
        await persist(`${name}-trace-fed.source`, fedSource);
        await persist(`${name}-trace-preflight.private.json`, JSON.stringify({ classification: "owner-private-runtime-trace",
          parser, cwd: source, argv: traceArgs, innerArgv: ["-x", "-v", "-euo", "pipefail"], environment: traceEnvironment,
          freshEnvironment: { ...freshEnvironment, PS4: producerPS4, PHASE: tracePhase,
            SERVICE_LASSO_WORKSPACE_ROOT: path.join(tracePhase, "workspace"), SERVICE_LASSO_INSTANCE_REGISTRY_PATH: path.join(tracePhase, "registry/instances.json"),
            SERVICE_LASSO_HOST_PORT_REGISTRY_PATH: path.join(tracePhase, "registry/ports.json") }, ...binding }));
        stdoutFile = await open(path.join(diagnostics, `${name}-trace-stdout.private`), "wx+", 0o600);
        stderrFile = await open(path.join(diagnostics, `${name}-trace-stderr.private`), "wx+", 0o600);
        await stdoutFile.sync(); await stderrFile.sync();
        const preflightDirectory = await open(diagnostics, "r");
        try { await preflightDirectory.sync(); } finally { await preflightDirectory.close(); }
        await emitDiagnostic({ ...binding, stage: "before-secondary", result: "pending" });
        // Direct private descriptors avoid both public/raw assertion output and
        // an artificial maxBuffer failure. Original caller stdin pipe remains.
        const traced = spawnSync(parser, traceArgs, { cwd: source, env: traceEnvironment, stdio: ["pipe", stdoutFile.fd, stderrFile.fd] });
        reached = true;
        await persist(`${name}-trace-result.private.json`, JSON.stringify({ status: traced.status, signal: traced.signal,
          error: traced.error ? { message: traced.error.message, code: traced.error.code } : null }));
        const streams = {};
        for (const [sink, file] of [["stdout", stdoutFile], ["stderr", stderrFile]]) {
          try {
            const observed = await inspectPrivateTrace(file, sink === "stderr");
            streams[sink] = observed.stream;
            if (sink === "stderr") streams.projection = observed.projection;
            if (observed.stream.completeness !== "complete-bounded-observation") failures.push({ sink, code: "readback_incomplete" });
          } catch { failures.push({ sink, code: "persistence_or_readback_failed" }); streams[sink] = { persistence: "failed", completeness: "unavailable", capture: "persistence-or-readback-failed" }; }
        }
        const directory = await open(diagnostics, "r");
        try { await directory.sync(); } finally { await directory.close(); }
        let image = "unavailable";
        try { image = digest(await readFile(parser)) === binding.parserSHA256 ? "matched" : "changed"; }
        catch { failures.push({ sink: "parser-image-readback", code: "readback_failed" }); }
        if (image === "changed") failures.push({ sink: "parser-image-readback", code: "image_changed" });
        await emitDiagnostic({ ...binding, stage: "after-secondary", invocationReached: true,
          result: traced.error ? "spawn_failed" : traced.signal ? "signaled" : traced.status === 0 ? "exited_zero" : Number.isInteger(traced.status) ? "exited_nonzero" : "exit_unknown",
          parserImageReadback: image, stdout: streams.stdout, stderr: streams.stderr,
          projection: streams.projection || projectPrivateTrace(null), diagnosticFailures: failures });
      } catch {
        failures.push({ sink: "runtime-trace", code: "diagnostic_failed" });
        try { await emitDiagnostic({ ...binding, stage: "secondary-failure", result: "failed", invocationReached: reached, diagnosticFailures: failures }); }
        catch { failures.push({ sink: "safe-output", code: "output_failed" }); }
      } finally {
        for (const file of [stdoutFile, stderrFile]) if (file) {
          try { await file.sync(); } catch { failures.push({ sink: "trace-file", code: "sync_failed" }); }
          try { await file.close(); } catch { failures.push({ sink: "trace-file", code: "close_failed" }); }
        }
        result.diagnosticFailures.push(...failures.map(failure => ({ ...failure, route: "secondary-private-runtime-trace" })));
      }
      try { await emitDiagnostic({ ...binding, stage: "secondary-persistence-closure", invocationReached: reached, diagnosticFailures: failures }); }
      catch { result.diagnosticFailures.push({ sink: "safe-output", route: "secondary-private-runtime-trace", code: "output_failed" }); }
    }
    return result;
  };
  // Execute the complete actual pre-fetch producer, including fresh env,
  // real tool printf, Python parser, and both fsync custody writes. Only the
  // subsequent external Core acquisition/build is outside this unit boundary.
  const actualProducer = await invokeProducer(phase, producer);
  assert.equal(actualProducer.error, undefined);
  assert.equal(actualProducer.status, 0, actualProducer.stderr);
  assert.deepEqual(actualProducer.diagnosticFailures, [], "independent diagnostic persistence must also succeed");
  const privateInput = JSON.parse(await readFile(path.join(phase, "private-input-custody.json"), "utf8"));
  const input = JSON.parse(await readFile(path.join(phase, "input-custody.json"), "utf8"));
  const { producer: parent, writer, writerObservedByProducer } = privateInput.process;
  assert.deepEqual(writer, writerObservedByProducer, "actual live Bash parent must observe the same Python writer PID/birth/image");
  assert.equal(writer.ppid, parent.pid);
  assert.notEqual(writer.pid, parent.pid);
  for (const identity of [parent, writer]) {
    assert.ok(Number.isSafeInteger(identity.pid) && identity.pid > 0);
    assert.ok(Number.isSafeInteger(identity.ppid) && identity.ppid > 0);
    assert.ok(identity.birth.length > 0);
    assert.ok(path.isAbsolute(identity.image));
  }
  assert.equal(await realpath(writer.image), await realpath(privateInput.tools.python3.path));
  assert.equal(await realpath(parent.image), await realpath(privateInput.tools.bash.path));
  assert.equal(await realpath(parent.image), parser, "actual observed producer image must match the explicitly bound parser");
  assert.equal(privateInput.tools.bash.sha256, digest(parserBytes));
  assert.equal(digest(await readFile(parser)), digest(parserBytes), "bound parser bytes must still match after invocation");
  assert.notEqual(writer.image, parent.image, "Python identity cannot reuse Bash executable");
  // Extracted actual producer, with one explicit failure inserted at the
  // specified boundary. No fixture identity or successful receipt is supplied.
  for (const [name, failedProducer] of [
    ["writer-death-before-ready", producer.replace("writer=observed(os.getpid()); producer=observed(os.getppid())", "raise RuntimeError('controlled writer death before ready')\nwriter=observed(os.getpid()); producer=observed(os.getppid())")],
    ["bash-failure-before-observation", producer.replace('test "$writer_ready" = "$writer_pid"', 'test "$writer_ready" = "$writer_pid"\nfalse # controlled parent observation failure')]
  ]) {
    const failurePhase=path.join(root,name);
    const failed=await invokeProducer(failurePhase,failedProducer,10000);
    assert.equal(failed.error,undefined,"actual rendezvous must finish through owned failure closure, not timeout");
    assert.equal(failed.signal,null);
    assert.notEqual(failed.status,0,name);
    assert.deepEqual(failed.diagnosticFailures, [], name);
    const closure=JSON.parse(await readFile(path.join(failurePhase,"private-writer-failure.json"),"utf8"));
    assert.equal(closure.actualWaitObserved,true,name);
    assert.notEqual(closure.writerExit,0,name);
    assert.notEqual(closure.producerExit,0,name);
    assert.throws(()=>process.kill(closure.writerPID,0),{code:"ESRCH"},"actual child must be absent after parent wait");
    await assert.rejects(()=>readFile(path.join(failurePhase,"input-custody.json")),{code:"ENOENT"},name);
  }
  assert.equal(input.source.tuiTree, tree);
  assert.equal(input.source.tuiDirtyHash, digest(Buffer.alloc(0)));
  assert.equal(input.source.tuiInventoryHash, digest(Buffer.from(`${digest(await readFile(path.join(source, "fixture.txt")))}  fixture.txt\n`)));
  assert.deepEqual(privateInput.ownedPaths, { workspaceRoot: path.join(phase, "workspace"), instanceRegistryPath: path.join(phase, "registry/instances.json"), hostPortRegistryPath: path.join(phase, "registry/ports.json"), allParentsNonLink: true, registriesInitiallyAbsent: true });
  assert.deepEqual(privateInput.literalCommands, ["mkdir -p", "git rev-parse HEAD^{tree}", "git status --porcelain", "git ls-files -z", "go env GOTOOLDIR", "git -C $phase/core-source init -q", "git -C $phase/core-source remote add origin", "git -C $phase/core-source fetch --no-tags origin develop", "git -C $phase/core-source merge-base --is-ancestor $CORE_COMMIT origin/develop", "git -C $phase/core-source fetch --depth=1 origin $CORE_COMMIT", "git -C $phase/core-source checkout --detach -q FETCH_HEAD", "npm ci", "npm run build", "node scripts/assert-go-source-provenance.mjs", "go build -mod=readonly -buildvcs=true -trimpath"]);
  const expectedTools = ["bash", "dirname", "mkdir", "env", "git", "sha256sum", "cut", "xargs", "awk", "wc", "uname", "ps", "readlink", "node", "npm", "go", "python3", "go-compile"].sort();
  assert.deepEqual(Object.keys(privateInput.tools).sort(), expectedTools);
  assert.deepEqual(input.verification.requiredToolsVerified, expectedTools);
  for (const tool of expectedTools) {
    const record = privateInput.tools[tool];
    const bytes = await readFile(record.path);
    assert.equal(record.sha256, digest(bytes), tool);
    assert.equal(record.size, (await stat(record.path)).size, tool);
  }
  // The repaired terminating walk still rejects an actual linked parent.
  const actualParent = path.join(root, "actual-parent"), linkedParent = path.join(root, "linked-parent");
  await mkdir(actualParent);
  await symlink(actualParent, linkedParent);
  const rejectedPhase = path.join(linkedParent, "phase");
  const rejected = await invokeProducer(rejectedPhase, producer);
  assert.equal(rejected.error, undefined);
  assert.notEqual(rejected.status, 0, "actual producer must reject linked parents");
  assert.deepEqual(rejected.diagnosticFailures, []);
  await assert.rejects(() => readFile(path.join(rejectedPhase, "input-custody.json")), { code: "ENOENT" });
  const core = path.join(phase, "core-source");
  await writeFile(path.join(core, "fixture.txt"), await readFile(path.join(source, "fixture.txt")));
  assert.equal(initialize(core), commit);
  // Run the actual post-checkout Core binding producer against a real tree;
  // no fetch, install, compiler, runtime or PTY acceptance is imitated here.
  const bindingStart = workflow.indexOf('          core_tree="$(git -C "$phase/core-source" rev-parse HEAD^{tree})"', actualCustodyEnd);
  const bindingEnd = workflow.indexOf('          (cd "$phase/core-source" && npm ci && npm run build)', bindingStart);
  assert.ok(bindingStart > actualCustodyEnd && bindingEnd > bindingStart);
  const binding = workflow.slice(bindingStart, bindingEnd).split("\n").map(line => line.startsWith("          ") ? line.slice(10) : line).join("\n");
  run("bash", ["-euo", "pipefail", "-c", `phase=${quote(phase)}\n${binding}`], source, { ...process.env, CORE_COMMIT: commit });
  assert.equal(JSON.parse(await readFile(path.join(phase, "core-source-binding.json"), "utf8")).coreTree, tree);
  // The other three records are explicit verifier fixtures, not native
  // acceptance evidence. The two custody records above are real producer bytes.
  const bytes = await readFile(path.join(source, "fixture.txt")), sha256 = digest(bytes);
  const records = {
    "binary-digest.json": { schemaVersion: 1, kind: "tui20-native-binary-digest", sha256, size: bytes.length },
    "build-output.json": { schemaVersion: 1, kind: "tui20-native-build-output", tuiCommit: commit, nativeBinary: { sha256, size: bytes.length } },
    "native-public-projection.json": { schemaVersion: 1, kind: "tui20-native-public-result", sourceCommit: commit, binarySHA256: sha256, result: "succeeded", actionsPassed: true, ownedRuntimeClosed: true }
  };
  for (const [name, value] of Object.entries(records)) await writeFile(path.join(phase, name), JSON.stringify(value));
  run(process.execPath, [path.join(scripts, "verify-tui20-native-public-receipt.mjs"), phase], source);
  completed = true;
});
