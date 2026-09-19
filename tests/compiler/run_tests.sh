#!/usr/bin/env bash
set -euo pipefail

HSL=${1:?missing hsl executable}
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT

check_failure() {
    local name=$1
    local expected=$2

    set +e

    "$HSL" check "$TMP/$name.hsl" \
        > "$TMP/$name.out" \
        2> "$TMP/$name.err"

    local status=$?

    set -e

    if [[ $status -eq 0 ]]; then
        echo "invalid program was accepted: $name" >&2
        exit 1
    fi

    grep -q "$expected" "$TMP/$name.err"
}

cat > "$TMP/lexical.hsl" <<'HSL'
fn main():
    print("unterminated);
HSL

check_failure lexical \
    'unterminated string literal'

cat > "$TMP/parser.hsl" <<'HSL'
fn main():
    let value: i64 = 42
HSL

check_failure parser \
    "expected ';' after variable declaration"

cat > "$TMP/semantic.hsl" <<'HSL'
fn main():
    print(missing);
HSL

check_failure semantic \
    'undefined identifier: missing'

cat > "$TMP/type.hsl" <<'HSL'
fn main():
    let value: bool = 42;
HSL

check_failure type \
    "cannot initialize 'value' of type bool with i64"

cat > "$TMP/math.hsl" <<'HSL'
fn add(a: i64, b: i64) -> i64:
    return a + b;
HSL

cat > "$TMP/valid.hsl" <<'HSL'
import math;

fn main():
    print(math.add(20, 22));
HSL

"$HSL" check "$TMP/valid.hsl"

"$HSL" build "$TMP/valid.hsl" \
    > "$TMP/valid_build.out"

test -x "$TMP/valid"

"$TMP/valid" > "$TMP/valid.out"

grep -qx '42' "$TMP/valid.out"

cat > "$TMP/failed_build.hsl" <<'HSL'
fn main():
    let value: bool = 42;
HSL

rm -f \
    "$TMP/failed_build" \
    "$TMP/failed_build.generated.cpp"

set +e

"$HSL" build "$TMP/failed_build.hsl" \
    > "$TMP/failed_build.out" \
    2> "$TMP/failed_build.err"

failed_build_status=$?

set -e

test "$failed_build_status" -eq 1
test ! -e "$TMP/failed_build"
test ! -e "$TMP/failed_build.generated.cpp"

grep -q \
    "cannot initialize 'value' of type bool with i64" \
    "$TMP/failed_build.err"

"$HSL" --version > "$TMP/version.out"
grep -qx 'HSL 9.0.0' "$TMP/version.out"

echo 'compiler tests passed'
