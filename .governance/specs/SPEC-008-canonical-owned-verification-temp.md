# SPEC-008: Canonical owned verification directories

Issue #131; protected ordinary publisher prerequisite for #129/#1992.

- TEMP001: freshly created publisher-owned draft/public download directories are passed as physical canonical paths to existing verification. This handles macOS symlinked temporary ancestors without permitting linked candidate inputs.
- TEMP002: preserve all verifier ancestor/symlink, regular-file, identity, held-original, byte-bound, cleanup and receipt checks unchanged. Never canonicalize external candidate inputs to bypass their denial.
- TEMP003: synthetic symlinked host-temp regression exercises complete actual scoped publication state machine with fake provider only, including draft/public original-byte receipts. Existing deliberate linked-input negatives continue failing closed.

Evidence baseline: develop 0f0fe55 (and parent 2fcdf2c) macOS native-pair/scoped four failures reach scoped publisher download reader through /var ancestry. Root fixture already realpaths its own directory. Correct the publisher-owned mkdtemp outputs only. No settings/credentials/publication or native qualification is authorized here.
