#!/usr/bin/env bash
set -euo pipefail
HSL=${1:?missing hsl executable}
TMP=$(mktemp -d); trap 'rm -rf "$TMP"' EXIT
cat > "$TMP/system.hsl" <<HSL
fn main():
    let path: str = path_join("$TMP", "value.txt");
    print(path_is_absolute(path));
    print(fs_write(path, "Helios"));
    print(fs_exists(path));
    print(fs_read(path));
    print(fs_exists(current_directory()));
    print(env_get("PATH").is_some());
    print(process_run("true"));
HSL
"$HSL" check "$TMP/system.hsl"
"$HSL" build "$TMP/system.hsl" >/dev/null
"$TMP/system" > "$TMP/out"
printf '%s\n' 1 1 1 Helios 1 1 0 > "$TMP/expected"
cmp "$TMP/expected" "$TMP/out"
echo 'system api tests passed'
