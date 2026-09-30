import assert from "node:assert/strict";
import test from "node:test";

import { admittedGoEnvironment, assertAmd64Evidence, assertCleanBuildMetadata, buildAdmittedTUI, isAmd64Architecture } from "./verify-real-core-conpty.mjs";

test("recognizes the Windows AMD64 spellings used by host and helper", () => {
  assert.equal(isAmd64Architecture("AMD64"), true);
  assert.equal(isAmd64Architecture("x86_64"), true);
  assert.equal(isAmd64Architecture("arm64"), false);
});

test("rejects evidence that would mislabel a non-AMD64 probe", () => {
  assert.throws(() => assertAmd64Evidence({ hostArchitecture: "ARM64", nodeArchitecture: "x64", helperArchitecture: { machine: "AMD64", pointerBits: 64 } }), /host architecture/);
  assert.throws(() => assertAmd64Evidence({ hostArchitecture: "AMD64", nodeArchitecture: "ia32", helperArchitecture: { machine: "AMD64", pointerBits: 64 } }), /Node process architecture/);
  assert.throws(() => assertAmd64Evidence({ hostArchitecture: "AMD64", nodeArchitecture: "x64", helperArchitecture: { machine: "AMD64", pointerBits: 32 } }), /ConPTY helper architecture/);
});

test("accepts only complete AMD64 probe evidence", () => {
  assert.doesNotThrow(() => assertAmd64Evidence({ hostArchitecture: "X64", nodeArchitecture: "x64", helperArchitecture: { machine: "AMD64", pointerBits: 64 } }));
});

test("direct real-Core builds refuse ambient Go overlays before any child command", async () => {
  const calls = [];
  await assert.rejects(
    () => buildAdmittedTUI({ executable: "ignored.exe", environment: { GOFLAGS: "-overlay=foreign.json", GOWORK: "off" }, command: async (...args) => calls.push(args) }),
    /ambient GOFLAGS/u,
  );
  assert.deepEqual(calls, []);
});

test("direct real-Core builds use the source gate, readonly VCS build, and matching metadata", async () => {
  const calls = [];
  const command = async (program, args, options) => {
    calls.push({ program, args, options });
    if (program === "git") return { stdout: "0123456789abcdef0123456789abcdef01234567\n" };
    if (program === "go" && args[0] === "version") return { stdout: "build\tvcs.revision=0123456789abcdef0123456789abcdef01234567\nbuild\tvcs.modified=false\n" };
    return { stdout: "" };
  };
  await buildAdmittedTUI({ executable: "verified.exe", environment: { GOFLAGS: "", GOWORK: "off" }, command });
  assert.deepEqual(calls.map(call => call.program), ["node", "git", "go", "go"]);
  assert.deepEqual(calls[2].args, ["build", "-mod=readonly", "-buildvcs=true", "-o", "verified.exe", "./cmd/service-lasso-tui"]);
  for (const call of calls) assert.equal(call.options.env.GOWORK, "off");
});

test("direct real-Core builds reject a mismatched or dirty VCS stamp", () => {
  const source = "0123456789abcdef0123456789abcdef01234567";
  assert.throws(() => assertCleanBuildMetadata(`build\tvcs.revision=${source}\nbuild\tvcs.modified=true`, source), /clean checked-out VCS identity/u);
  assert.throws(() => assertCleanBuildMetadata("build\tvcs.revision=abcdef", source), /clean checked-out VCS identity/u);
  assert.doesNotThrow(() => admittedGoEnvironment({ GOFLAGS: "", GOWORK: "off" }));
});
