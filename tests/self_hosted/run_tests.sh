#!/usr/bin/env bash
set -euo pipefail
HSL=${1:?missing hsl executable}
ROOT=${2:?missing source root}
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT
cp "$ROOT/self_hosted/compiler_model.hsl" "$TMP/compiler_model.hsl"
"$HSL" check "$TMP/compiler_model.hsl"
"$HSL" build "$TMP/compiler_model.hsl" > "$TMP/build.out"
"$TMP/compiler_model" > "$TMP/run.out"
printf '%s\n' '2' '13' > "$TMP/expected.out"
cmp "$TMP/expected.out" "$TMP/run.out"
echo 'self-hosted foundation tests passed'
