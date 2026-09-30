import assert from "node:assert/strict";
import { execFile } from "node:child_process";
import { createHash } from "node:crypto";
import { mkdir, readFile, rm, writeFile } from "node:fs/promises";
import os from "node:os";
import path from "node:path";
import test from "node:test";
import { fileURLToPath } from "node:url";

const repoRoot = path.resolve(path.dirname(fileURLToPath(import.meta.url)), "..");
const sourceGate = path.join(repoRoot, "scripts", "assert-go-source-provenance.mjs");
const nonce = "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef";

function run(program, args, options = {}) {
  return new Promise((resolve, reject) => {
    execFile(program, args, { windowsHide: true, encoding: "utf8", ...options }, (error, stdout, stderr) => {
      if (error) {
        error.stdout = stdout;
        error.stderr = stderr;
        reject(error);
      } else resolve({ stdout, stderr });
    });
  });
}

test("an overlay can forge an api_client_error under a clean VCS stamp, and the source gate rejects it", async () => {
  const root = path.join(os.tmpdir(), `tui-go-overlay-${process.pid}-${Date.now()}`);
  const checkout = path.join(root, "checkout");
  try {
    await mkdir(root, { recursive: true });
    const sourceCommit = (await run("git", ["rev-parse", "HEAD"], { cwd: repoRoot })).stdout.trim();
    await run("git", ["init", checkout]);
    await run("git", ["-C", checkout, "fetch", "--no-tags", "--depth=1", repoRoot, sourceCommit]);
    await run("git", ["-C", checkout, "checkout", "--detach", "FETCH_HEAD"]);
    const sourcePath = path.join(checkout, "internal", "api", "client.go");
    const altered = (await readFile(sourcePath, "utf8")).replace(
      "return nil, &ConfigurationError{Kind: ConfigurationErrorInvalidURL}",
      'return nil, fmt.Errorf("foreign client failure")',
    );
    assert.notEqual(altered, await readFile(sourcePath, "utf8"), "overlay fixture did not alter the imported client");
    const replacement = path.join(root, "client.go");
    const overlay = path.join(root, "overlay.json");
    const executable = path.join(root, process.platform === "win32" ? "service-lasso-tui.exe" : "service-lasso-tui");
    await writeFile(replacement, altered);
    await writeFile(overlay, JSON.stringify({ Replace: { [sourcePath]: replacement } }));

    await assert.rejects(
      () => run("node", [sourceGate], { cwd: checkout, env: { ...process.env, GOFLAGS: `-overlay=${overlay}`, GOWORK: "off" } }),
      /ambient GOFLAGS are not admitted/u,
    );
    await assert.doesNotReject(() => run("node", [sourceGate], { cwd: checkout, env: { ...process.env, GOFLAGS: "", GOWORK: "off" } }));

    await run("go", ["build", "-mod=readonly", "-buildvcs=true", "-o", executable, "./cmd/service-lasso-tui"], {
      cwd: checkout, env: { ...process.env, GOFLAGS: `-overlay=${overlay}`, GOWORK: "off" },
    });
    const metadata = (await run("go", ["version", "-m", executable], { cwd: checkout })).stdout;
    assert.match(metadata, new RegExp(`vcs\\.revision=${sourceCommit}`));
    assert.match(metadata, /vcs\.modified=false/u);
    const binarySHA256 = createHash("sha256").update(await readFile(executable)).digest("hex");
    const result = await run(executable, ["--api", "://invalid"], {
      cwd: checkout,
      env: { ...process.env, SERVICE_LASSO_STARTUP_PROBE_NONCE: nonce },
    }).catch(error => ({ stdout: error.stdout, stderr: error.stderr, code: error.code }));
    assert.equal(result.code, 2);
    assert.match(result.stderr, new RegExp(`SERVICE_LASSO_TUI_STARTUP_BOUNDARY:${nonce}:api_client_error:${sourceCommit}:${binarySHA256}`));
  } finally {
    await rm(root, { recursive: true, force: true });
  }
});
