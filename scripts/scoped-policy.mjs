import { createHash } from "node:crypto";
import { readFile } from "node:fs/promises";
import { parseStrictJSON } from "./scoped-json.mjs";
export const POLICY_SHA256 = "159d644c161cf532c94d3bfe17ed55e32bf94c5d2843928945c450f6d8140c12";
export const POLICY_SOURCE = Object.freeze({repository:"service-lasso/service-lasso",commit:"a0384e676c1b2dbf66b915563ea70c176c09d598",path:".governance/project/ga-platform-scope.json",blob:"e694c3e314c1bf11e03dc4656730a96467a765b8"});
export const SCOPE = Object.freeze({policyId:"ga-windows-linux-2026-10-04",policySha256:POLICY_SHA256,policySource:POLICY_SOURCE,requiredPlatforms:Object.freeze(["win32","linux"]),deferredPlatforms:Object.freeze(["darwin"])});
export function assertScope(value) {
  const equal = (left,right) => {
    if (Array.isArray(right)) return Array.isArray(left) && left.length===right.length && right.every((item,index)=>equal(left[index],item));
    if (right && typeof right === "object") return left && typeof left === "object" && !Array.isArray(left) && Object.keys(left).length===Object.keys(right).length && Object.keys(right).every(key=>Object.hasOwn(left,key)&&equal(left[key],right[key]));
    return left === right;
  };
  if (!equal(value,SCOPE)) throw new Error("source-owned scope denied"); return value;
}
export async function assertSourcePolicy() {
  const bytes = await readFile(new URL("../.governance/project/ga-platform-scope.json",import.meta.url));
  if(createHash("sha256").update(bytes).digest("hex")!==POLICY_SHA256 || createHash("sha1").update(Buffer.from(`blob ${bytes.length}\0`)).update(bytes).digest("hex")!==POLICY_SOURCE.blob) throw new Error("canonical policy bytes denied");
  const policy=parseStrictJSON(bytes);
  if (policy.policyId!==SCOPE.policyId || JSON.stringify(policy.requiredPlatforms)!==JSON.stringify(SCOPE.requiredPlatforms) || JSON.stringify(policy.deferredPlatforms)!==JSON.stringify(SCOPE.deferredPlatforms)) throw new Error("policy contract denied");
  return SCOPE;
}
