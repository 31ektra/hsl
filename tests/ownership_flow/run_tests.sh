#!/usr/bin/env bash
set -euo pipefail
HSL=${1:?missing hsl executable}
TMP=$(mktemp -d); trap 'rm -rf "$TMP"' EXIT
cat > "$TMP/one_branch.hsl" <<'HSL'
fn consume(value: List<i64>):
    print(value.length());
fn main():
    let values: List<i64> = List<i64>();
    if true:
        consume(move values);
    else:
        print(0);
    print(values.length());
HSL
"$HSL" check "$TMP/one_branch.hsl"
cat > "$TMP/both_branches.hsl" <<'HSL'
fn consume(value: List<i64>):
    print(value.length());
fn main():
    let values: List<i64> = List<i64>();
    if true:
        consume(move values);
    else:
        consume(move values);
    print(values.length());
HSL
set +e
"$HSL" check "$TMP/both_branches.hsl" >/dev/null 2>"$TMP/error"
status=$?
set -e
test "$status" -ne 0
grep -Fq 'use of moved value: values' "$TMP/error"
echo 'ownership flow tests passed'
