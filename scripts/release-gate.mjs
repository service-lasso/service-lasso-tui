import { assessNativeV2Readiness } from "./native-v2-readiness.mjs";

/**
 * Code release gate. It never creates a GA declaration. The release owner must
 * supply an explicit declaration after readiness is admitted for review.
 */
export function releaseGate(status, declaration) {
  if (!status || status.status !== "NOT_GA" || status.ga !== false || status.declared !== false) {
    return { ok: false, code: "invalid_release_status" };
  }
  if (!status.admitted) {
    return { ok: false, code: "not_admitted", blockers: status.blockers ?? [] };
  }
  if (
    !declaration ||
    declaration.owner !== "release-owner" ||
    declaration.ga !== true ||
    typeof declaration.declarationRef !== "string" ||
    !declaration.declarationRef.length
  ) {
    return { ok: false, code: "release_owner_declaration_required" };
  }
  return { ok: true, code: "RELEASE_OWNER_DECLARED" };
}

export function releaseGateFromInputs(input = {}, declaration) {
  return releaseGate(assessNativeV2Readiness(input), declaration);
}
