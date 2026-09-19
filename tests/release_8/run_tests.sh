#!/usr/bin/env bash
set -euo pipefail

HSL=${1:?missing hsl executable}
ROOT=${2:?missing source root}

test "$("$HSL" --version)" = "HSL 9.0.0"

grep -Fq \
    'VERSION 9.0.0' \
    "$ROOT/CMakeLists.txt"

grep -Fq \
    'constexpr std::string_view VERSION = "9.0.0";' \
    "$ROOT/src/main.cpp"

grep -Fq \
    'Current stable version: `9.0.0`.' \
    "$ROOT/README.md"

grep -Fq \
    '# HSL 9.0.0' \
    "$ROOT/docs/release_8_0.md"

test -x "$ROOT/tools/release_gate.sh"
test -x "$ROOT/tools/full_release_gate.sh"
test -x "$ROOT/tools/bootstrap_8.sh"
test -x "$ROOT/tools/check_readme.py"
test -x "$ROOT/tools/check_readme_examples.py"

test -f "$ROOT/docs/support_policy.md"
test -f "$ROOT/docs/language_reference.md"
test -f "$ROOT/docs/stage_binary.md"

test -d "$ROOT/tests/readme_examples"
test -f "$ROOT/tests/readme_examples/minimal.hsl"
test -f "$ROOT/tests/readme_examples/functions.hsl"
test -f "$ROOT/tests/readme_examples/reference.hsl"
test -f "$ROOT/tests/readme_examples/move.hsl"

echo 'HSL 9.0.0 release metadata passed'
