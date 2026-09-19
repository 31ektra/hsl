#!/usr/bin/env bash
set -euo pipefail
HSL=${1:?missing hsl executable}
TMP=$(mktemp -d); trap 'rm -rf "$TMP"' EXIT
cat > "$TMP/runtime.hsl" <<HSL
fn main():
    let directory: str = path_join("$TMP", "a/b");
    print(fs_create_directories(directory));
    let source: str = path_join(directory, "value.txt");
    let target: str = path_join(directory, "renamed.hsl");
    print(fs_write(source, "HSL 3"));
    print(fs_is_file(source));
    print(fs_is_directory(directory));
    print(path_filename(source));
    print(path_extension(target));
    print(path_parent(source));
    print(path_normalize(path_join(directory, "../b/value.txt")));
    print(fs_rename(source, target));
    print(fs_remove(target));
    print(process_run_program("true"));
HSL
"$HSL" check "$TMP/runtime.hsl"
"$HSL" build "$TMP/runtime.hsl" >/dev/null
"$TMP/runtime" > "$TMP/out"
printf '%s\n' 1 1 1 1 value.txt .hsl "$TMP/a/b" "$TMP/a/b/value.txt" 1 1 0 > "$TMP/expected"
cmp "$TMP/expected" "$TMP/out"
echo 'HSL 3 runtime tests passed'
