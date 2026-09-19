#!/usr/bin/env bash
set -euo pipefail
HSL=${1:?missing compiler}
ROOT=${2:?missing root}
test "$("$HSL" --version)" = 'HSL 9.0.0'
grep -Fq 'VERSION 9.0.0' "$ROOT/CMakeLists.txt"
grep -Fq 'VERSION = "9.0.0"' "$ROOT/src/main.cpp"
test -f "$ROOT/docs/freestanding.md"
test -f "$ROOT/docs/kernel_abi.md"
test -f "$ROOT/runtime/freestanding/hsl_kernel.h"
echo 'HSL 9.0.0 release metadata passed'
