#!/usr/bin/env bash
set -euo pipefail
HSL=${1:?missing hsl executable}
TMP=$(mktemp -d); trap 'rm -rf "$TMP"' EXIT
cat > "$TMP/program.hsl" <<'HSL'
fn main():
    print("HSL 4");
HSL
"$HSL" emit-cpp "$TMP/program.hsl" > "$TMP/emitted.cpp"
grep -Fq 'int main(' "$TMP/emitted.cpp"
"$HSL" build --keep-cpp "$TMP/program.hsl" >/dev/null
test -x "$TMP/program"
test -f "$TMP/program.generated.cpp"
test "$("$TMP/program")" = 'HSL 4'
echo 'HSL 4 CLI tests passed'
