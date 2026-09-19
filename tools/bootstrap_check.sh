#!/usr/bin/env bash
set -euo pipefail
HSL=${1:?missing hsl executable}
ROOT=${2:?missing source root}
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT
cp "$ROOT/self_hosted/compiler_model.hsl" "$TMP/stage_input.hsl"
"$HSL" check "$TMP/stage_input.hsl"
"$HSL" tokens "$TMP/stage_input.hsl" > "$TMP/tokens1.log"
"$HSL" tokens "$TMP/stage_input.hsl" > "$TMP/tokens2.log"
cmp "$TMP/tokens1.log" "$TMP/tokens2.log"
"$HSL" ast "$TMP/stage_input.hsl" > "$TMP/ast1.log"
"$HSL" ast "$TMP/stage_input.hsl" > "$TMP/ast2.log"
cmp "$TMP/ast1.log" "$TMP/ast2.log"
"$HSL" build "$TMP/stage_input.hsl" > "$TMP/build.log"
"$TMP/stage_input" > "$TMP/output.log"
printf '%s\n' '2' '13' > "$TMP/expected.log"
cmp "$TMP/expected.log" "$TMP/output.log"
echo 'deterministic stage-zero compiler-model check passed'
