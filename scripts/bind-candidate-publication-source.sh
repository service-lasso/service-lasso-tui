#!/usr/bin/env bash
set -euo pipefail

expected_sha="$1"
source_ref="$2"
receipt_path="$3"
artifact_name="$4"

test "$source_ref" = "refs/heads/develop"
[[ "$expected_sha" =~ ^[a-f0-9]{40}$ ]]
checked_out_sha="$(git rev-parse HEAD)"
test "$checked_out_sha" = "$expected_sha"
printf '{"sourceRef":"%s","sourceCommit":"%s","checkedOutCommit":"%s","artifactName":"%s"}\n' "$source_ref" "$expected_sha" "$checked_out_sha" "$artifact_name" > "$receipt_path"
