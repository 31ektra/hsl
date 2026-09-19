#!/usr/bin/env bash
set -euo pipefail
HSL=${1:?missing hsl executable}
ROOT=${2:?missing source root}
test "$("$HSL" --version)" = "HSL 9.0.0"
grep -Fq '# HSL 7.0.0' "$ROOT/docs/release_7_0.md"
test -f "$ROOT/self_hosted/compiler.hsl"
echo 'HSL 7 release metadata tests passed'
