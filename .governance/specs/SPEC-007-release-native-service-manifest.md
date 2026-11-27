# SPEC-007: Release-native TUI service manifest

Issue: service-lasso/service-lasso-tui#129; Core consumer: service-lasso/service-lasso#1992.
Reference: service-template develop 6785b43a4aa5497d21cc3068eb61a91f71472337.

## Requirements

- MANIFEST001: canonical root service.json identifies @tui as a provider. It installs a prebuilt runtime, launched on demand; no daemon or consumer source build.
- MANIFEST002: released service.json binds exact candidate version/tag, archive names and computed SHA-256, using win32-x64, linux-x64, darwin-arm64 and additionally actually built darwin-x64. No OS fallback, latest channel, or invented compatibility.
- MANIFEST003: schema3 candidate publication adds service.json to the exact protected inventory. Its bytes join original-held-byte upload, draft verification, immutable public receipt and recovery. Existing schema2 read verification remains compatible; new publication requires schema3.
- MANIFEST004: malformed identity, unknown/missing platforms, path/name/hash mismatches, altered manifest or inventory fail before provider access. Existing immutable, environment, clean-source and provenance controls remain unchanged.

## Proof and authority

No-publish Node fixtures exercise deterministic assembly, actual file hashes and publisher refusal before provider access. Build/extract actual four-platform archives locally and verify embedded source identity. Hosted ordinary CI must pass before normal develop merge. Publication, protected workflow dispatch, native acceptance, promotion and GA remain outside this issue.
