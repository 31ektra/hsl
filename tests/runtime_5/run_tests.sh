#!/usr/bin/env bash
set -euo pipefail
HSL=${1:?missing hsl executable}
TMP=$(mktemp -d); trap 'rm -rf "$TMP"' EXIT
cat > "$TMP/runtime.hsl" <<'HSL'
fn main():
    print(arg_count());
    print(arg(1));
    print(utf8_length("Aé🙂"));
    print(utf8_scalar_at("Aé🙂", 1));
    print(env_set("HSL_RUNTIME_5", "ready"));
    print(env_get("HSL_RUNTIME_5").unwrap());
    print(env_unset("HSL_RUNTIME_5"));
HSL
"$HSL" check "$TMP/runtime.hsl"
HSL_CXX=${HSL_CXX:-g++} "$HSL" build "$TMP/runtime.hsl" >/dev/null
"$TMP/runtime" payload > "$TMP/out"
printf '%s\n' 2 payload 3 233 1 ready 1 > "$TMP/expected"
cmp "$TMP/expected" "$TMP/out"
echo 'HSL 5 runtime tests passed'
