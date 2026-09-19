#!/usr/bin/env bash
set -Eeuo pipefail
ROOT=$(cd "$(dirname "$0")/.." && pwd)
BUILD=${1:-$ROOT/build-final}
CXX_COMPILER=${CXX:-c++}
trap 'printf "release gate failed at line %s: %s\n" "$LINENO" "$BASH_COMMAND" >&2' ERR
rm -rf "$BUILD"
cmake -S "$ROOT" -B "$BUILD" -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER="$CXX_COMPILER"
cmake --build "$BUILD" --parallel
ctest --test-dir "$BUILD" --output-on-failure
python3 "$ROOT/tools/fuzz_smoke.py" "$BUILD/hsl" "${HSL_FUZZ_INPUTS:-10000}"
python3 "$ROOT/tools/audit_repository.py"
HSL_CXX="$CXX_COMPILER" "$ROOT/tools/bootstrap_8.sh" "$BUILD/hsl" "$ROOT"
test "$("$BUILD/hsl" --version)" = "HSL 9.0.0"
"$BUILD/hsl" verify-determinism "$ROOT/examples/hello.hsl" | grep -Fxq deterministic
cmake --install "$BUILD" --prefix "$BUILD/install"
test "$("$BUILD/install/bin/hsl" --version)" = "HSL 9.0.0"
echo "HSL 9.0.0 release gate passed with $CXX_COMPILER"
