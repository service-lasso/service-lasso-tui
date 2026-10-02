import assert from "node:assert/strict";
import { createHash } from "node:crypto";
import { mkdtemp, mkdir, readFile, realpath, rm, stat, symlink, writeFile } from "node:fs/promises";
import { spawnSync } from "node:child_process";
import { tmpdir } from "node:os";
import path from "node:path";
import test from "node:test";
import { fileURLToPath } from "node:url";

const scripts = path.dirname(fileURLToPath(import.meta.url));
const digest = bytes => createHash("sha256").update(bytes).digest("hex");
const quote = value => `'${value.replaceAll("'", "'\\''")}'`;

test("actual fresh native build rejects persisted flags and ambient workspace influence", async t => {
  assert.ok(["linux", "darwin"].includes(process.platform));
  const root = await mkdtemp(path.join(await realpath(tmpdir()), "tui20-native-go-admission-"));
  t.after(() => rm(root, { recursive: true, force: true }));
  const checkout = path.join(root, "checkout"), home = path.join(root, "home"), phase = path.join(root, "phase");
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
  t.after(() => rm(root, { recursive: true, force: true }));
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
  // Execute the complete actual pre-fetch producer, including fresh env,
  // real tool printf, Python parser, and both fsync custody writes. Only the
  // subsequent external Core acquisition/build is outside this unit boundary.
  run("bash", ["-euo", "pipefail", "-c", `phase=${quote(phase)}\n${producer}TUI20_PHASE\n`], source, { ...process.env, CI_SOURCE_SHA: commit, CORE_COMMIT: commit });
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
  assert.notEqual(writer.image, parent.image, "Python identity cannot reuse Bash executable");
  // Extracted actual producer, with one explicit failure inserted at the
  // specified boundary. No fixture identity or successful receipt is supplied.
  for (const [name, failedProducer] of [
    ["writer-death-before-ready", producer.replace("writer=observed(os.getpid()); producer=observed(os.getppid())", "raise RuntimeError('controlled writer death before ready')\nwriter=observed(os.getpid()); producer=observed(os.getppid())")],
    ["bash-failure-before-observation", producer.replace('test "$writer_ready" = "$writer_pid"', 'test "$writer_ready" = "$writer_pid"\nfalse # controlled parent observation failure')]
  ]) {
    const failurePhase=path.join(root,name);
    const failed=spawnSync("bash",["-euo","pipefail","-c",`phase=${quote(failurePhase)}\n${failedProducer}TUI20_PHASE\n`],{cwd:source,env:{...process.env,CI_SOURCE_SHA:commit,CORE_COMMIT:commit},encoding:"utf8",timeout:10000});
    assert.equal(failed.error,undefined,"actual rendezvous must finish through owned failure closure, not timeout");
    assert.equal(failed.signal,null);
    assert.notEqual(failed.status,0,name);
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
  const rejected = spawnSync("bash", ["-euo", "pipefail", "-c", `phase=${quote(rejectedPhase)}\n${producer}TUI20_PHASE\n`], { cwd: source, env: { ...process.env, CI_SOURCE_SHA: commit, CORE_COMMIT: commit }, encoding: "utf8" });
  assert.equal(rejected.error, undefined);
  assert.notEqual(rejected.status, 0, "actual producer must reject linked parents");
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
});
