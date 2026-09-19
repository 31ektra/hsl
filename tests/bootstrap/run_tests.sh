#!/usr/bin/env bash
set -euo pipefail
HSL=${1:?missing hsl executable}
ROOT=${2:?missing source root}
"$ROOT/tools/bootstrap_check.sh" "$HSL" "$ROOT"
echo 'bootstrap validation tests passed'
