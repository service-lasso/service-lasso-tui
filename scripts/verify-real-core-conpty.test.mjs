import assert from "node:assert/strict";
import test from "node:test";

import { assertAmd64Evidence, isAmd64Architecture } from "./verify-real-core-conpty.mjs";

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
