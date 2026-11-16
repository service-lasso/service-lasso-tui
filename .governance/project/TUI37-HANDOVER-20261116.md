# TUI37 handover — 2026-11-16

Handover from session `2d121ca1-f273-4fc4-9977-07f7896be2f8` (cancelled; last turn cut short).
State below was verified against the live repository on 2026-11-16, not copied from session history.

## Repository state at handover

- `origin/develop` = `c82f690` (merge of #48). Local checkout `main` = `ff17b2a`, clean.
- Local `develop` ref is stale at `714e5a5`; fetch before branching.
- No open pull requests. Merged: #39 `d606229`, #41 `6d19ad8`, #40, #43, #45, #46, #47, #48.
- Latest `develop` CI run `43702278820`: Linux, Windows, owned-overlay, both cross-compilation jobs and the Linux native PTY lifecycle are green. Only `macOS test and build` fails, the known pre-existing durable-store deferral.
- Issues: #23 CLOSED (Windows five-action acceptance executed and passed), #37 OPEN `BLOCKED/UNRUN`, #42/#44 CLOSED.
- The old worktree `/home/maxbarrass/.codex/worktrees/tui-37-acquisition/` no longer exists.
- Nothing was left running.

## Objective

Delivery owner for #37 (TUI v2 native-owner production acquisition, win32/linux pair). Windows execution belongs to a separate Windows agent; Linux work is driven here.

## Delivered

- Linux executor and legacy lifecycle repair (#39), Linux lifecycle post-merge fix (#41).
- Windows source-only five-action driver and tests (#46, #47), plus Core/UNRUN disposition docs (#48).
- #23 closed on a real Windows native run: TUI `714e5a5`, binary SHA `020a8cce0e062febee39ba71272e812f28b04b35787049c39d1adaa849406eb5`, Core `2633c07be25512d0a84f9bfa28de6be5edff35e8`, real ConPTY, five actions once each, 5 operations and 5 audit entries, frozen previews, durable readback, no replay, unrelated service unchanged, real resize/reconnect, owned exits zero.

## Hard boundaries retained

- `produceNative` stays closed; no synthetic or supplied-authority inputs.
- #37 genuine v2 needs authentic SLCPROF1, closed ABI, complete native geometry, genuine constructors, Windows inputs, matching-host ownership, independent admission, and Core same-published-byte disposition.
- macOS is deferred/N/A, never PASS.
- Delivery ownership is not input custody or admission authority.

## Current work in this branch

`codex/37-readiness-admission-validator` adds a fail-closed readiness gate:

- `scripts/native-v2-readiness.mjs` — validates profile, ABI and geometry with the existing decoders and requires closed custody records for the remaining groups. It returns `NOT_READY_NOT_ADMITTED` unless every group is `READY`; it never claims native proof.
- `scripts/native-v2-readiness.test.mjs` — covers empty, partial, complete, malformed profile/ABI/geometry, mutated custody, file input and CLI exit codes.
- Wired into both CI Node test groups.
- Refreshed ledger: `TUI37-NATIVE-V2-READINESS-LEDGER.md`.

## Known stale claim

A pasted report said “#39 OPEN/RED — Linux adverse-controller receipt set”. That is false for this repository: #39 is merged and its Linux lifecycle run passed all seven phases, including `adverse-controller` with a complete public receipt set. The accompanying statement that the merged Windows driver is the legacy `tui28`/observer ConPTY path is correct; the adapter-based `local-e2e-conpty.py` / `run-local-e2e.mjs` driver remains optional source work.

## Toolchain

```
export PATH=/tmp/tui37-bin:/home/maxbarrass/.cache/tui37-tools/go/bin:/home/maxbarrass/.cache/tui37-tools/node-v22.23.2-linux-x64/bin:$PATH
export PYTHONDONTWRITEBYTECODE=1   # /tmp/tui37-bin/python -> /usr/bin/python3
go test -mod=readonly ./...
node --test scripts/<suite>.test.mjs
python3 scripts/<suite>.test.py
```

Pinned lifecycle Core: `2633c07be25512d0a84f9bfa28de6be5edff35e8`.

## Source-only next actions

1. Have an input custodian supply the authentic originals and custody references; run the readiness gate before any target execution.
2. Optional: implement the adapter-based Windows five-action driver on the #42/#44 ConPTY adapter (source-only, native UNRUN).
3. Keep Linux green against new `develop` heads; review the Windows agent PR as a distinct reviewer.

## Adapter lifecycle contract (source-only)

- `scripts/local-e2e-lifecycle.mjs` adds a fail-closed five-action contract (install, config, start, stop, restart) over the ordinary ConPTY adapter. It requires frozen preview/confirmation, exactly one durable operation and one Core audit entry per action, reload refusal, no replay after reconnect, unchanged unrelated state, a real resize, and an observed zero exit.
- `scripts/local-e2e-lifecycle.test.mjs` exercises the contract with fake adapters, including every fail-closed path.
- Wired into `.github/workflows/local-e2e.yml` source contracts.
- Native Windows lifecycle remains UNRUN. The concrete ConPTY binding still needs a matching Windows host; the contract source does not claim native evidence.
