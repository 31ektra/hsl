#!/usr/bin/env bash
set -euo pipefail
HSL=${1:?missing hsl executable}
ROOT=${2:?missing source root}
test "$("$HSL" --version)" = "HSL 9.0.0"
test -f "$ROOT/docs/release_5_0.md"
grep -Fq '# HSL 5.0.0' "$ROOT/docs/release_5_0.md"
echo 'HSL 5 release-history validation passed'
