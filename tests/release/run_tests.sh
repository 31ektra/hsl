#!/usr/bin/env bash
set -euo pipefail

HSL=${1:?missing HSL executable}
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
    "$ROOT/docs/release_9_0.md"

grep -Eq \
    '^(version|VERSION)=9\.0\.0$' \
    "$ROOT/tools/package_release.sh"

test -f "$ROOT/docs/freestanding.md"
test -f "$ROOT/docs/kernel_abi.md"
test -f "$ROOT/runtime/freestanding/hsl_kernel.h"

test -x "$ROOT/tests/kernel_types/run_tests.sh"
test -x "$ROOT/tests/release_9/run_tests.sh"

test -x "$ROOT/tools/check_readme.py"
test -x "$ROOT/tools/check_readme_examples.py"
test -x "$ROOT/tools/full_release_gate.sh"
test -x "$ROOT/tools/package_release.sh"
test -x "$ROOT/tools/release_gate.sh"

echo "HSL 9.0.0 active release metadata passed"
