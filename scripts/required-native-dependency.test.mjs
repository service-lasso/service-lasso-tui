import test from "node:test";
import assert from "node:assert/strict";
import { readFile } from "node:fs/promises";

const workflow = await readFile(new URL("../.github/workflows/ci.yml", import.meta.url), "utf8");
function job(id) {
  const marker = `  ${id}:\n`;
  const start = workflow.indexOf(marker);
  assert.notEqual(start, -1, `missing ${id}`);
  const rest = workflow.slice(start + marker.length);
  const next = rest.search(/^  [a-z][a-z0-9-]*:\s*$/mu);
  return next === -1 ? rest : rest.slice(0, next);
}
function steps(body) { return body.slice(body.indexOf("    steps:\n")).trimEnd(); }
function platforms(body) { return [...body.matchAll(/^          - name: (.+)$/gmu)].map(match => match[1]); }

test("required native dependency excludes deferred macOS without masking required failures", () => {
  const requiredTest = job("test");
  const deferredTest = job("deferred-macos-test");
  const requiredNative = job("native-posix-lifecycle");
  const deferredNative = job("deferred-macos-native-posix-lifecycle");
  assert.deepEqual(platforms(requiredTest), ["Linux", "Windows"]);
  assert.deepEqual(platforms(deferredTest), ["macOS"]);
  assert.deepEqual(platforms(requiredNative), ["Linux"]);
  assert.deepEqual(platforms(deferredNative), ["macOS"]);
  assert.match(requiredNative, /^    needs: test$/mu);
  assert.match(deferredNative, /^    needs: deferred-macos-test$/mu);
  for (const body of [requiredTest, deferredTest, requiredNative, deferredNative]) {
    const beforeSteps = body.slice(0, body.indexOf("    steps:\n"));
    assert.doesNotMatch(beforeSteps, /^    (?:if:|continue-on-error:)/mu);
    assert.match(body, /^      fail-fast: false$/mu);
  }
  // Exact duplicated steps preserve the original diagnostic/native semantics.
  // Ignore only the comment between job blocks; it carries no step behavior.
  assert.equal(steps(requiredTest).replace(/\n  # Darwin diagnostics.*$/u, "").trimEnd(), steps(deferredTest));
  assert.equal(steps(requiredNative), steps(deferredNative));
  for (const body of [requiredNative, deferredNative]) {
    assert.match(body, /^      CORE_COMMIT: 2633c07be25512d0a84f9bfa28de6be5edff35e8$/mu);
    for (const phase of ["preflight", "five-actions", "finalization-failure", "adverse-controller", "adverse-owner-death", "handoff-timeout", "shutdown-pipe-failure"]) {
      assert.ok(body.includes(`prepare_phase "$attempt/${phase}"`), `missing ${phase}`);
    }
    assert.ok(body.includes("node scripts/verify-tui20-native-public-receipt.mjs"));
    assert.ok(body.includes("name: Retain native direct-evidence receipt\n        if: always()"));
    assert.ok(body.includes("**/native-public-projection.json"));
    assert.doesNotMatch(body, /continue-on-error:/u);
  }
});