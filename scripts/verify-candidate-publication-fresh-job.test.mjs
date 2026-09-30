import assert from "node:assert/strict";
import { createHash } from "node:crypto";
import { chmod, cp, mkdtemp, mkdir, readFile, rm, writeFile } from "node:fs/promises";
import { execFile as execFileCallback } from "node:child_process";
import os from "node:os";
import path from "node:path";
import test from "node:test";
import { fileURLToPath, pathToFileURL } from "node:url";
import { promisify } from "node:util";

const execFile = promisify(execFileCallback);
const scriptsDirectory = path.dirname(fileURLToPath(import.meta.url));
const digest = bytes => createHash("sha256").update(bytes).digest("hex");
const bash = process.platform === "win32" ? path.join(process.env.ProgramFiles ?? "C:\\Program Files", "Git", "bin", "bash.exe") : "bash";

async function run(command, args, options) {
  return execFile(command, args, { windowsHide: true, ...options });
}

test("fresh job binds checked source before consuming its artifact and runs the actual verifier CLI", async t => {
  const jobRoot = await mkdtemp(path.join(os.tmpdir(), "candidate-fresh-job-"));
  t.after(() => rm(jobRoot, { recursive: true, force: true }));
  const source = path.join(jobRoot, "checked-source");
  const artifact = path.join(jobRoot, "downloaded-artifact");
  const sourceScripts = path.join(source, "scripts");
  await mkdir(sourceScripts, { recursive: true });
  await mkdir(path.join(artifact, "provider-bodies"), { recursive: true });
  await Promise.all([
    cp(path.join(scriptsDirectory, "verify-candidate-publication.mjs"), path.join(sourceScripts, "verify-candidate-publication.mjs")),
    cp(path.join(scriptsDirectory, "bind-candidate-publication-source.sh"), path.join(sourceScripts, "bind-candidate-publication-source.sh")),
  ]);
  await chmod(path.join(sourceScripts, "bind-candidate-publication-source.sh"), 0o755);
  await run("git", ["init", "--quiet"], { cwd: source });
  await run("git", ["config", "user.email", "candidate-fixture@example.invalid"], { cwd: source });
  await run("git", ["config", "user.name", "Candidate fixture"], { cwd: source });
  await run("git", ["add", "."], { cwd: source });
  await run("git", ["commit", "--quiet", "-m", "fixture source"], { cwd: source });
  const { stdout: shaOutput } = await run("git", ["rev-parse", "HEAD"], { cwd: source });
  const sha = shaOutput.trim();
  const version = `2026.10.1-${sha.slice(0, 7)}`;
  const tag = `candidate-${version}`;
  const names = [
    `service-lasso-tui-${version}-win32-amd64.zip`,
    `service-lasso-tui-${version}-linux-amd64.tar.gz`,
    `service-lasso-tui-${version}-darwin-amd64.tar.gz`,
    `service-lasso-tui-${version}-darwin-arm64.tar.gz`,
    "SHA256SUMS.txt",
    "candidate-manifest.json",
  ];
  const archiveNames = names.slice(0, 4);
  const archiveBodies = Object.fromEntries(archiveNames.map(name => [name, Buffer.from(`fresh-job-${name}`, "utf8")]));
  const checksumBody = Buffer.from(archiveNames.map(name => `${digest(archiveBodies[name])}  ${name}`).join("\n") + "\n", "utf8");
  const manifest = {
    schemaVersion: 2,
    kind: "develop-prerelease-candidate",
    source: { repository: "service-lasso/service-lasso-tui", ref: "refs/heads/develop", commit: sha },
    release: { tag, prerelease: true, draft: false, immutable: true },
    version,
    checksumManifest: { name: "SHA256SUMS.txt", sha256: digest(checksumBody) },
    corePackagingIssue: "service-lasso/service-lasso#1461",
    assets: [
      { platform: "win32-amd64", name: names[0], sha256: digest(archiveBodies[names[0]]), executable: "service-lasso-tui.exe" },
      { platform: "linux-amd64", name: names[1], sha256: digest(archiveBodies[names[1]]), executable: "service-lasso-tui" },
      { platform: "darwin-amd64", name: names[2], sha256: digest(archiveBodies[names[2]]), executable: "service-lasso-tui" },
      { platform: "darwin-arm64", name: names[3], sha256: digest(archiveBodies[names[3]]), executable: "service-lasso-tui" },
    ],
  };
  const bodies = { ...archiveBodies, "SHA256SUMS.txt": checksumBody, "candidate-manifest.json": Buffer.from(JSON.stringify(manifest), "utf8") };
  const localAssets = Object.fromEntries(names.map(name => [name, { sha256: digest(bodies[name]), size: bodies[name].length }]));
  const release = {
    tag_name: tag, target_commitish: sha, draft: false, prerelease: true, immutable: true,
    assets: names.map((name, index) => ({
      id: index + 401,
      url: `https://api.github.com/repos/service-lasso/service-lasso-tui/releases/assets/${index + 401}`,
      name, digest: `sha256:${localAssets[name].sha256}`, size: localAssets[name].size,
      browser_download_url: `https://github.com/service-lasso/service-lasso-tui/releases/download/${tag}/${name}`,
    })),
  };
  const preflight = {
    immutableReleases: { enabled: true },
    environment: { name: "development-candidate", protection_rules: [{ type: "wait_timer", wait_timer: 10 }], deployment_branch_policy: { protected_branches: true, custom_branch_policies: false } },
    branchProtection: { required_status_checks: { strict: true, contexts: ["Linux test and build", "Windows test and build", "macOS test and build", "Release asset cross-compilation"] }, required_pull_request_reviews: { required_approving_review_count: 0 }, allow_force_pushes: { enabled: false } },
  };
  await Promise.all([
    writeFile(path.join(artifact, "immutable-releases.json"), JSON.stringify(preflight.immutableReleases)),
    writeFile(path.join(artifact, "development-candidate.json"), JSON.stringify(preflight.environment)),
    writeFile(path.join(artifact, "develop-protection.json"), JSON.stringify(preflight.branchProtection)),
    writeFile(path.join(artifact, "candidate-manifest.json"), bodies["candidate-manifest.json"]),
    writeFile(path.join(artifact, "candidate-local-assets.json"), JSON.stringify(localAssets)),
    writeFile(path.join(artifact, "release.json"), JSON.stringify(release)),
    ...names.map(name => writeFile(path.join(artifact, name), bodies[name])),
    ...names.map(name => writeFile(path.join(artifact, "provider-bodies", name), bodies[name])),
  ]);
  const fixtureFetch = path.join(jobRoot, "fixture-fetch.mjs");
  await writeFile(fixtureFetch, `import { appendFile, readFile } from "node:fs/promises";\nimport path from "node:path";\nconst artifact = process.env.FRESH_JOB_ARTIFACT_DIR;\nglobalThis.fetch = async (url, options) => {\n  const value = String(url);\n  await appendFile(path.join(artifact, "fetch-calls.ndjson"), JSON.stringify({ url: value, options }) + "\\n");\n  const parsed = new URL(value);\n  const name = decodeURIComponent(parsed.pathname.split("/").at(-1));\n  if (parsed.hostname === "github.com") return { status: 302, headers: { get: key => key === "location" ? \`https://release-assets.githubusercontent.com/github-production-release-asset-2e65be/123/\${encodeURIComponent(name)}?X-Amz-Algorithm=AWS4-HMAC-SHA256&X-Amz-Credential=ABCDEFGHIJKLMNOP%2F20261001%2Fus-east-1%2Fs3%2Faws4_request&X-Amz-Date=20261001T000000Z&X-Amz-Expires=300&X-Amz-SignedHeaders=host&X-Amz-Signature=\${"a".repeat(64)}\` : null } };\n  const body = await readFile(path.join(artifact, "provider-bodies", name));\n  return { status: 200, headers: { get: key => key === "content-length" ? String(body.length) : null }, body: (async function* () { yield body; })() };\n};\n`);
  const verifier = path.join(sourceScripts, "verify-candidate-publication.mjs");
  const sourceReceipt = path.join(jobRoot, "candidate-source-binding.json");
  const artifactName = `service-lasso-tui-candidate-${version}-${sha}`;
  await run(bash, [path.join(sourceScripts, "bind-candidate-publication-source.sh"), sha, "refs/heads/develop", sourceReceipt, artifactName], { cwd: source });
  assert.deepEqual(JSON.parse(await readFile(sourceReceipt, "utf8")), { sourceRef: "refs/heads/develop", sourceCommit: sha, checkedOutCommit: sha, artifactName });
  let artifactRetrieved = false;
  const retrieveArtifact = () => {
    artifactRetrieved = true;
    return artifact;
  };
  await writeFile(path.join(source, "unexpected-untracked-input"), "dirty\n");
  await assert.rejects(async () => {
    await run(bash, [path.join(sourceScripts, "bind-candidate-publication-source.sh"), sha, "refs/heads/develop", sourceReceipt, artifactName], { cwd: source });
    retrieveArtifact();
  }, /candidate publication source must have no tracked or untracked changes/u);
  assert.equal(artifactRetrieved, false);
  await rm(path.join(source, "unexpected-untracked-input"));
  const cleanVerifier = await readFile(verifier);
  await writeFile(verifier, `${cleanVerifier}\ntracked dirty input\n`);
  await assert.rejects(async () => {
    await run(bash, [path.join(sourceScripts, "bind-candidate-publication-source.sh"), sha, "refs/heads/develop", sourceReceipt, artifactName], { cwd: source });
    retrieveArtifact();
  }, /candidate publication source must have no tracked or untracked changes/u);
  assert.equal(artifactRetrieved, false);
  await writeFile(verifier, cleanVerifier);
  const retrievedArtifact = retrieveArtifact();
  const preflightResult = await run(process.execPath, [verifier, "--mode", "preflight", "--source-ref", "refs/heads/develop", "--source-commit", sha, "--version", version, "--tag", tag, "--immutable-releases", path.join(retrievedArtifact, "immutable-releases.json"), "--environment", path.join(retrievedArtifact, "development-candidate.json"), "--branch-protection", path.join(retrievedArtifact, "develop-protection.json")], { cwd: source });
  assert.equal(JSON.parse(preflightResult.stdout).environment, "development-candidate");
  const receiptResult = await run(process.execPath, ["--import", pathToFileURL(fixtureFetch).href, verifier, "--mode", "receipt", "--source-ref", "refs/heads/develop", "--source-commit", sha, "--version", version, "--tag", tag, "--manifest", path.join(artifact, "candidate-manifest.json"), "--release", path.join(artifact, "release.json"), "--local-assets", path.join(artifact, "candidate-local-assets.json"), "--asset-directory", artifact, "--download-dir", path.join(jobRoot, "public-receipt")], { cwd: source, env: { ...process.env, FRESH_JOB_ARTIFACT_DIR: artifact } });
  assert.equal(JSON.parse(receiptResult.stdout).verified.length, 6);
  const calls = (await readFile(path.join(artifact, "fetch-calls.ndjson"), "utf8")).trim().split("\n").map(JSON.parse);
  assert.equal(calls.length, 12);
  assert.ok(calls.every(call => call.options.method === "GET" && call.options.redirect === "manual" && Object.keys(call.options.headers).length === 0));
  const assertLocalFailureBeforeProviderRead = async mutate => {
    await mutate();
    await assert.rejects(() => run(process.execPath, [verifier, "--mode", "local-assets", "--source-ref", "refs/heads/develop", "--source-commit", sha, "--version", version, "--tag", tag, "--manifest", path.join(artifact, "candidate-manifest.json"), "--local-assets", path.join(artifact, "candidate-local-assets.json"), "--asset-directory", artifact], { cwd: source }), /actual local|checksum manifest|inventory|candidate manifest/u);
    assert.equal((await readFile(path.join(artifact, "fetch-calls.ndjson"), "utf8")).trim().split("\n").length, 12);
  };
  await assertLocalFailureBeforeProviderRead(() => writeFile(path.join(artifact, names[0]), Buffer.from("substituted archive", "utf8")));
  await writeFile(path.join(artifact, names[0]), bodies[names[0]]);
  await assertLocalFailureBeforeProviderRead(() => writeFile(path.join(artifact, "SHA256SUMS.txt"), Buffer.from("0".repeat(64) + `  ${names[0]}\n`, "utf8")));
  await writeFile(path.join(artifact, "SHA256SUMS.txt"), bodies["SHA256SUMS.txt"]);
  await assertLocalFailureBeforeProviderRead(() => writeFile(path.join(artifact, "candidate-manifest.json"), Buffer.from("{}", "utf8")));
  await writeFile(path.join(artifact, "candidate-manifest.json"), bodies["candidate-manifest.json"]);
  await assertLocalFailureBeforeProviderRead(() => writeFile(path.join(artifact, "candidate-local-assets.json"), JSON.stringify({ ...localAssets, [names[1]]: { ...localAssets[names[1]], sha256: "f".repeat(64) } })));
  assert.notEqual(path.resolve(verifier), path.resolve(scriptsDirectory, "verify-candidate-publication.mjs"));
});
