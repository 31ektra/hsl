#!/usr/bin/env bash
set -euo pipefail
ROOT=${2:?missing source root}
TMP=$(mktemp -d); trap 'rm -rf "$TMP"' EXIT
"${CXX:-c++}" -std=c++20 -Wall -Wextra -Wpedantic -I"$ROOT/include" "$ROOT/tests/arena/arena_test.cpp" -o "$TMP/arena_test"
"$TMP/arena_test"
echo 'arena tests passed'
