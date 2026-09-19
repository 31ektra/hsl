#!/usr/bin/env bash
set -euo pipefail
HSL=${1:?missing hsl executable}
TMP=$(mktemp -d); trap 'rm -rf "$TMP"' EXIT
cat > "$TMP/collections.hsl" <<'HSL'
fn main():
    let some: Option<i64> = Option<i64>.Some(42);
    let none: Option<i64> = Option<i64>.None();
    print(some.is_some());
    print(some.unwrap());
    print(none.unwrap_or(7));
    let ok: Result<i64, str> = Result<i64, str>.Ok(9);
    print(ok.is_ok());
    print(ok.unwrap());
    var values: Map<str, i64> = Map<str, i64>();
    values.insert("answer", 42);
    print(values.contains("answer"));
    print(values.get("answer").unwrap());
    var unique: Set<i64> = Set<i64>();
    unique.insert(4);
    unique.insert(4);
    print(unique.length());
HSL
"$HSL" check "$TMP/collections.hsl"
"$HSL" build "$TMP/collections.hsl" >/dev/null
"$TMP/collections" > "$TMP/out"
printf '%s\n' 1 42 7 1 9 1 42 1 > "$TMP/expected"
cmp "$TMP/expected" "$TMP/out"
echo 'collection tests passed'
