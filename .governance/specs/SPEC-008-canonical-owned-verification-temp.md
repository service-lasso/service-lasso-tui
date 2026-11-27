# SPEC-008: Canonical owned verification directories

Issue #131; protected ordinary publisher prerequisite for #129/#1992.

- TEMP001: freshly created publisher-owned draft/public download directories are passed as physical canonical paths to existing verification. This handles macOS symlinked temporary ancestors without permitting linked candidate inputs.
- TEMP002: preserve all verifier ancestor/symlink, regular-file, identity, held-original, byte-bound, cleanup and receipt checks unchanged. Never canonicalize external candidate inputs to bypass their denial.
- TEMP003: synthetic symlinked host-temp regression exercises complete actual scoped publication state machine with fake provider only, including draft/public original-byte receipts. Existing deliberate linked-input negatives continue failing closed.

Evidence baseline: develop 0f0fe55 (and parent 2fcdf2c) macOS native-pair/scoped four failures reach scoped publisher download reader through /var ancestry. Root fixture already realpaths its own directory. Correct the publisher-owned mkdtemp outputs only. No settings/credentials/publication or native qualification is authorized here.

TEMP004 diagnostic: after original four failures clear, macOS reaches an existing controlled pipe-fault test where a fixture child exits before readiness. Retain its actual exit code/signal and fixed allowlisted synthetic stderr classification in the failure so the runtime cause can be diagnosed; no new skip, altered success assertion or runtime behavior.

TEMP005 / linked #133: retain the actual stdout stream before closing the fixture-owned original fd0. Darwin libuv stdout probing must occur while stdio descriptor numbers remain occupied. Emit readiness only after the unchanged original close and EBADF observation. Keep actual parent EPIPE, same live JWKS readback and original 250ms natural teardown. No stdin replacement/reopen or production TUI runtime change.

TEMP006 / Bash3.2 custody prerequisite: exact unexpected-EOF runtime failure reproduced under GNU3.2 while Bash5 passes. Parse the unchanged prefetch custody body completely as one named function, then invoke it in SAME original Bash/PID/environment with unchanged FIFO/read/tool/identity/negative closure requirements. No command-string substitution, parser upgrade, input restriction or success waiver. Actual3.2/5 controls and matching-host Darwin CI required.
