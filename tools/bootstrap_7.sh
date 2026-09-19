#!/usr/bin/env bash
set -euo pipefail
HSL=${1:?missing stage-zero hsl executable}
ROOT=${2:?missing source root}
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT
cp "$ROOT/self_hosted/compiler.hsl" "$TMP/compiler.hsl"
cp "$ROOT/examples/hello.hsl" "$TMP/input.hsl"
"$HSL" check "$TMP/compiler.hsl"
"$HSL" verify-determinism "$TMP/compiler.hsl" | grep -Fxq deterministic
HSL_CXX=${HSL_CXX:-g++} "$HSL" build --keep-cpp "$TMP/compiler.hsl" >/dev/null
cp "$TMP/compiler.generated.cpp" "$TMP/stage_one_a.cpp"
rm -f "$TMP/compiler" "$TMP/compiler.generated.cpp"
HSL_CXX=${HSL_CXX:-g++} "$HSL" build --keep-cpp "$TMP/compiler.hsl" >/dev/null
cmp "$TMP/stage_one_a.cpp" "$TMP/compiler.generated.cpp"
"$TMP/compiler" "$TMP/input.hsl" > "$TMP/manifest_a"
"$TMP/compiler" "$TMP/input.hsl" > "$TMP/manifest_b"
cmp "$TMP/manifest_a" "$TMP/manifest_b"
grep -Fxq 'hsl-stage=1' "$TMP/manifest_a"
HSL_CXX=${HSL_CXX:-g++} "$HSL" emit-object "$TMP/compiler.hsl" >/dev/null
HSL_CXX=${HSL_CXX:-g++} "$HSL" emit-assembly "$TMP/compiler.hsl" >/dev/null
test -s "$TMP/compiler.o"
test -s "$TMP/compiler.s"
echo 'HSL 7 staged bootstrap validation passed'
