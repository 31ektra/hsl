#!/usr/bin/env bash
set -euo pipefail
HSL=${1:?missing hsl executable}
ROOT=${2:?missing source root}
HSL_CXX=${HSL_CXX:-g++} "$ROOT/tools/bootstrap_8.sh" "$HSL" "$ROOT"
