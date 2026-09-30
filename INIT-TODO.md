# Governance and adoption TODO

- [x] Initialize repo-local governance for Issue #1 on the authorized `develop`
  development baseline.
- [x] Record project intent, active specification, and issue-to-spec mapping.
- [x] Install strict pull-request and branch-protection guidance.
- [x] Issue #20 verified Core `develop` `55848ec178b5fb05826192c8a2db576eaa8848dc`
  publishes availability, preview/confirmation, durable submission/readback,
  and advertised cancellation for lifecycle operations. Update/update
  cancellation remains Core #1538; removal and source-safe admission remain
  separate dependencies.
- [ ] Retain exact-source Core lifecycle acceptance for Issue #20 (authenticated
  allow/deny, preview, one submission, readback/reconnect, cancellation, and
  unrelated-service preservation) separately from fixtures, source CI, package
  qualification, release, and GA.
- [x] Issue #7 verified Core `develop` GET routes for capability/setup metadata,
  authenticated runtime identity, inbox list, and service health history. The
  TUI consumes only bounded display fields and no inbox detail or mutation.
- [ ] Validate Windows, Linux, and macOS release executables against a real
  supported Core runtime, including resize and reconnect scenarios.
- [x] Issue #13 defines a direct Windows read-only ConPTY acceptance path for
  the current TUI binary against exact Core `develop` `d9e2ae799244317940c862fe1261dfd22b7bdda1`.
  It uses only a disposable loopback API and temporary roots; it does not
  qualify release assets, lifecycle mutations, other platforms, or GA.
- [x] Issue #6 defines a develop-dispatched, checksum-bound four-asset
  prerelease-candidate workflow. Its source/platform smoke and structural
  checks are not real terminal/Core acceptance, which remains the open item
  above.
