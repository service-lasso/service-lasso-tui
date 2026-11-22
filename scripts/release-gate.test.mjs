import test from "node:test";
import assert from "node:assert/strict";
import { assessNativeV2Readiness } from "./native-v2-readiness.mjs";
import { releaseGate, releaseGateFromInputs } from "./release-gate.mjs";

test("release gate refuses unready inputs", () => {
  assert.deepEqual(releaseGateFromInputs(), {
    ok: false,
    code: "not_admitted",
    blockers: assessNativeV2Readiness().blockers,
  });
});

test("release gate still refuses without release-owner declaration", () => {
  const status = { ...assessNativeV2Readiness(), admitted: true, disposition: "ADMITTED_FOR_REVIEW", blockers: [] };
  assert.deepEqual(releaseGate(status), { ok: false, code: "release_owner_declaration_required" });
});

test("release gate accepts only an explicit release-owner declaration", () => {
  const status = { ...assessNativeV2Readiness(), admitted: true, disposition: "ADMITTED_FOR_REVIEW", blockers: [] };
  assert.deepEqual(
    releaseGate(status, { owner: "release-owner", ga: true, declarationRef: "custody/release-owner/ga" }),
    { ok: true, code: "RELEASE_OWNER_DECLARED" },
  );
  assert.deepEqual(releaseGate(status, { owner: "someone-else", ga: true, declarationRef: "x" }), {
    ok: false,
    code: "release_owner_declaration_required",
  });
});

test("release gate refuses a mutated status", () => {
  assert.deepEqual(releaseGate({ status: "GA", ga: true, declared: true }, null), {
    ok: false,
    code: "invalid_release_status",
  });
});
