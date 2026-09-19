#!/usr/bin/env bash
set -euo pipefail
HSL=${1:?missing hsl executable}
TMP=$(mktemp -d); trap 'rm -rf "$TMP"' EXIT
cat > "$TMP/program.hsl" <<'HSL'
fn main():
    print("HSL 6");
HSL
test "$("$HSL" verify-determinism "$TMP/program.hsl")" = deterministic
HSL_CXX=${HSL_CXX:-g++} "$HSL" emit-object "$TMP/program.hsl" >/dev/null
HSL_CXX=${HSL_CXX:-g++} "$HSL" emit-assembly "$TMP/program.hsl" >/dev/null
test -s "$TMP/program.o"
test -s "$TMP/program.s"
file "$TMP/program.o" | grep -Fq 'relocatable'
grep -Eq 'main|\.text' "$TMP/program.s"
echo 'HSL 6 toolchain tests passed'
