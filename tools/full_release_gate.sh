#!/usr/bin/env bash
set -Eeuo pipefail
ROOT=$(cd "$(dirname "$0")/.." && pwd)
cd "$ROOT"
trap 'printf "full release gate failed at line %s: %s\n" "$LINENO" "$BASH_COMMAND" >&2' ERR

rm -rf build-final build-clang build-asan build-ubsan release-install dist

python3 tools/check_readme.py

CXX=c++ HSL_FUZZ_INPUTS=${HSL_FUZZ_INPUTS:-10000} bash tools/release_gate.sh build-final
python3 tools/check_readme_examples.py build-final/hsl

cmake -S . -B build-clang -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_CXX_FLAGS='-Wall -Wextra -Wpedantic -Werror'
cmake --build build-clang --parallel
ctest --test-dir build-clang --output-on-failure
python3 tools/check_readme_examples.py build-clang/hsl
test "$(build-clang/hsl --version)" = 'HSL 9.0.0'
test "$(build-clang/hsl verify-determinism examples/hello.hsl)" = deterministic

cmake -S . -B build-asan -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_CXX_FLAGS='-fsanitize=address -fno-omit-frame-pointer -O1 -g' -DCMAKE_EXE_LINKER_FLAGS='-fsanitize=address'
cmake --build build-asan --parallel
ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 ctest --test-dir build-asan --output-on-failure

cmake -S . -B build-ubsan -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_CXX_FLAGS='-fsanitize=undefined -fno-omit-frame-pointer -O1 -g' -DCMAKE_EXE_LINKER_FLAGS='-fsanitize=undefined'
cmake --build build-ubsan --parallel
UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1 ctest --test-dir build-ubsan --output-on-failure

cmake --install build-final --prefix "$ROOT/release-install"
test "$(release-install/bin/hsl --version)" = 'HSL 9.0.0'
release-install/bin/hsl check examples/hello.hsl

bash tools/package_release.sh dist
sha256sum --check dist/hsl-9.0.0-source.tar.gz.sha256
if tar -tzf dist/hsl-9.0.0-source.tar.gz | grep -E '/(\.git|build|build-[^/]*|dist|release-install)/'; then
    echo 'release archive contains excluded output' >&2
    exit 1
fi

python3 tools/audit_repository.py
git diff --check
printf 'HSL 9.0.0 full release gate passed\n'
