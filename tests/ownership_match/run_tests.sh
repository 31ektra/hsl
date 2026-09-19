#!/usr/bin/env bash
set -euo pipefail
HSL=${1:?missing hsl executable}
TMP=$(mktemp -d); trap 'rm -rf "$TMP"' EXIT
cat > "$TMP/all.hsl" <<'HSL'
enum Choice:
    Left;
    Right;
fn consume(value: List<i64>):
    print(value.length());
fn main():
    let values: List<i64> = List<i64>();
    let choice: Choice = Choice.Left;
    match choice:
        Choice.Left:
            consume(move values);
        Choice.Right:
            consume(move values);
    print(values.length());
HSL
set +e
"$HSL" check "$TMP/all.hsl" >"$TMP/out" 2>"$TMP/err"
status=$?
set -e
test "$status" -ne 0
grep -Fq 'use of moved value: values' "$TMP/err"
cat > "$TMP/one.hsl" <<'HSL'
enum Choice:
    Left;
    Right;
fn consume(value: List<i64>):
    print(value.length());
fn main():
    let values: List<i64> = List<i64>();
    let choice: Choice = Choice.Left;
    match choice:
        Choice.Left:
            consume(move values);
        Choice.Right:
            print(0);
    print(values.length());
HSL
"$HSL" check "$TMP/one.hsl"
echo 'match ownership tests passed'
