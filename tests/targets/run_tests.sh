#!/usr/bin/env bash
set -euo pipefail
HSL=${1:?missing hsl executable}
TMP=$(mktemp -d); trap 'rm -rf "$TMP"' EXIT
cat > "$TMP/program.hsl" <<'HSL'
fn main():
    print(42);
HSL
cat > "$TMP/compiler" <<'SH2'
#!/usr/bin/env bash
printf '%s\n' "$@" > "$HSL_TARGET_LOG"
exit 0
SH2
chmod +x "$TMP/compiler"
for spec in \
  'x86_64-portable|-march=x86-64' \
  'x86_64-v2|-march=x86-64-v2' \
  'aarch64-portable|-march=armv8-a' \
  'armv7-portable|-march=armv7-a'; do
  profile=${spec%%|*}; expected=${spec#*|}
  HSL_TARGET_LOG="$TMP/$profile.log" HSL_CXX="$TMP/compiler" HSL_ARCH_PROFILE="$profile" "$HSL" build "$TMP/program.hsl" >/dev/null
  grep -Fxq -- "$expected" "$TMP/$profile.log"
done
HSL_TARGET_LOG="$TMP/cross.log" HSL_CXX="$TMP/compiler" HSL_TARGET=aarch64-linux-gnu HSL_SYSROOT=/opt/aarch64-sysroot HSL_CPU=cortex-a53 "$HSL" build "$TMP/program.hsl" >/dev/null
grep -Fxq -- '--target=aarch64-linux-gnu' "$TMP/cross.log"
grep -Fxq -- '--sysroot=/opt/aarch64-sysroot' "$TMP/cross.log"
grep -Fxq -- '-mcpu=cortex-a53' "$TMP/cross.log"
echo 'target configuration tests passed'
